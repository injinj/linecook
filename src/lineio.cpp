#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#ifdef LC_SHARED
#define LC_API __declspec(dllexport)
#endif
#include <linecook/linecook.h>

extern "C" {

int
lc_line_length( LineCook *state )
{
  return static_cast<linecook::State *>( state )->line_length();
}

int
lc_line_copy( LineCook *state,  char *out )
{
  return static_cast<linecook::State *>( state )->line_copy( out );
}

int
lc_edit_length( LineCook *state )
{
  return static_cast<linecook::State *>( state )->edit_length();
}

int
lc_edit_copy( LineCook *state,  char *out )
{
  return static_cast<linecook::State *>( state )->edit_copy( out );
}

int
lc_complete_term_length( LineCook *state )
{
  return static_cast<linecook::State *>( state )->complete_term_length();
}

int
lc_complete_term_copy( LineCook *state,  char *out )
{
  return static_cast<linecook::State *>( state )->complete_term_copy( out );
}

int
lc_get_complete_geom( LineCook *state, int *arg_num, int *arg_count,
                      int *arg_off, int *arg_len, size_t args_size )
{
  return static_cast<linecook::State *>( state )->
    get_complete_geom( *arg_num, *arg_count, arg_off, arg_len, args_size );
}

} /* extern "C" */

using namespace linecook;

int State::line_length( void ) noexcept {
  return this->line_length( 0, this->line_len ); }
int State::line_copy( char *out ) noexcept {
  return this->line_copy( out, 0, this->line_len ); }
int State::edit_length( void ) noexcept {
  return this->line_length( 0, this->edited_len ); }
int State::edit_copy( char *out ) noexcept {
  return this->line_copy( out, 0, this->edited_len ); }
int State::complete_term_length( void ) noexcept {
  return this->line_length( this->complete_off,
                            this->complete_off + this->complete_len ); }
int State::complete_term_copy( char *out ) noexcept {
  return this->line_copy( out, this->complete_off,
                          this->complete_off + this->complete_len ); }

int
State::line_length( size_t from,  size_t to ) noexcept
{
  int size = 0;
  for ( size_t i = from; i < to; i++ ) {
    if ( this->line[ i ] != 0 ) {
      int n = ku_utf32_to_utf8_len( &this->line[ i ], 1 );
      if ( n > 0 )
        size += n;
    }
  }
  return size;
}

int
State::line_copy( char *out,  size_t from,  size_t to ) noexcept
{
  int size = 0;
  for ( size_t i = from; i < to; i++ ) {
    if ( this->line[ i ] != 0 ) {
      int n = ku_utf32_to_utf8( this->line[ i ], &out[ size ] );
      if ( n > 0 )
        size += n;
    }
  }
  return size;
}

void
State::reset_input( LineCookInput &input ) noexcept
{
  input.input_mode = 0;     /* clear modes */
  input.cur_input  = NULL;
  input.cur_recipe = NULL;
  input.pcnt       = 0;     /* clear putback buffer */
  input.putb       = 0;
  input.cur_char   = 0;
}

char32_t
State::next_input_char( LineCookInput &input ) noexcept
{
  for (;;) {
    char32_t c;
    int n = ku_utf8_to_utf32( &input.input_buf[ input.input_off ],
                              input.input_len - input.input_off, &c );
    if ( n > 0 ) {
      input.input_off += n;
      input.cur_char = c;
      return c;
    }
    if ( n < 0 ) {
      input.input_off++; /* can't wait with 4 chars, illegal utf8 */
      this->error = LINE_STATUS_BAD_INPUT;
    }
    return 0;
  }
}
/* multichar actions */
KeyAction
State::eat_multichar( LineCookInput &input ) noexcept
{
  char32_t c = input.cur_char;
  if ( input.pcnt == 0 ) {
    input.pending[ 0 ] = c;
    input.pcnt = 1;
    return ACTION_PENDING;
  }
  size_t potential_match = 0;
  KeyRecipe ** mc = input.cur_input->mc;
  size_t       sz = input.cur_input->mc_size;
  input.pending[ input.pcnt++ ] = c;
  input.pending[ input.pcnt ] = 0;
  /* linear scan of multichar sequences, nothing fancy;  the database is likely
   * to be less than 50 multichar actions since there are not that many keys on
   * a keyboard */
  for ( size_t i = 0; i < sz; i++ ) {
    KeyRecipe  & r   = *mc[ i ];
    const char * seq = r.char_sequence;
    int options;
    if ( char32_eq( seq[ 0 ], input.pending[ 0 ] ) &&
         char32_eq( seq[ 1 ], input.pending[ 1 ] ) ) { /* at least 2 chars eq */
      bool match = true;
      uint8_t k  = 2;
      for ( ; k < input.pcnt; k++ ) { /* check if all match */
        if ( ! char32_eq( seq[ k ], input.pending[ k ] ) ) {
          match = false;
          break;
        }
      }
      if ( match ) {
        if ( seq[ k ] == 0 ) { /* if all match */
          options = lc_action_options( r.action );
          if ( ( options & OPT_VI_CHAR_ARG ) == 0 ) { /* if not a char arg */
          is_match:;
            input.pcnt = 0;
            input.cur_recipe = &r;
            return r.action;
          }
        }
        potential_match++;
      }
      else {
        if ( seq[ k ] == 0 && k + 1 == input.pcnt ) { /* the char arg */
          options = lc_action_options( r.action );
          if ( ( options & OPT_VI_CHAR_ARG ) != 0 )
            goto is_match;
        }
      }
    }
  }
  /* no matches, putback the chars */
  if ( potential_match == 0 )
    return ACTION_PUTBACK;
  return ACTION_PENDING;
}
/* eat chars from input */
KeyAction
State::eat_input( LineCookInput &input ) noexcept
{
  char32_t  c;
  bool      putback = false;
  uint8_t * charto;
  int       m = this->test( input.mode,
                            EMACS_MODE | VI_INSERT_MODE | VI_COMMAND_MODE |
                            VISUAL_MODE | SEARCH_MODE );
  /* if mode changed, select correct tables */
  if ( input.input_mode != m || input.cur_input == NULL ) {
    /* pick a translation table */
    if ( ( input.mode & VISUAL_MODE ) != 0 )
      input.cur_input = &this->visual;
    else if ( ( input.mode & SEARCH_MODE ) != 0 )
      input.cur_input = &this->search;
    else if ( ( input.mode & EMACS_MODE ) != 0 )
      input.cur_input = &this->emacs;
    else if ( ( input.mode & VI_INSERT_MODE ) != 0 )
      input.cur_input = &this->vi_insert;
    else
      input.cur_input = &this->vi_command;
    input.input_mode = m;
  }
  charto = input.cur_input->recipe;
  input.cur_recipe = NULL;

  /* eat chars from putback pending first */
  if ( input.putb > 0 ) {
    c = input.pending[ input.putb++ ];
    input.cur_char = c;
    putback = true;
    /* if end of putback mode */
    if ( input.putb == input.pcnt )
      input.putb = input.pcnt = 0;
  }
  /* else next input char to process */
  else {
    c = this->next_input_char( input );
    if ( c == 0 )
      return ACTION_PENDING;
    if ( input.pcnt != 0 ) { /* if pending buffer waiting for more chars */
    eat_multi:; /* a recipe below calls for multichar translation */
      for (;;) {
        KeyAction act = this->eat_multichar( input );
        if ( act == ACTION_PUTBACK ) { /* if no multichar matches */
          c = input.pending[ 0 ];
          input.cur_char = c;
          input.putb = 1;
          putback = true; /* prevent infinite pending loop */
          break;
        }
        if ( act != ACTION_PENDING ) /* something matched */
          return act;
        if ( input.input_off == input.input_len ) /* eat more if available */
          return ACTION_PENDING;
        c = this->next_input_char( input );
        if ( c == 0 )
          return ACTION_PENDING;
      }
    }
  }
  /* match a char to an action, multi-byte chars use default recipe */
  uint8_t uc = ( c < sizeof( input.cur_input->recipe ) ?
                 (uint8_t) c : input.cur_input->def );
  KeyRecipe &r = this->recipe[ charto[ uc ] ];

  if ( r.char_sequence == NULL ) { /* if default action, no further processing*/
    input.cur_recipe = &r;
    input.cur_char = c;
    return r.action;
  }
  /* if single char transition (ctrl codes, escape) */
  if ( r.char_sequence[ 1 ] == 0 ) {
    /* for escape in insert mode, check if arrow keys are used */
    if ( r.action == ACTION_VI_ESC ) {
      if ( ! putback && ( this->input_available( input ) ||
                          this->is_emacs_mode( input.mode ) ) )
        goto eat_multi;
    }
    else {
      int options = lc_action_options( r.action );
      /* get the second char by using putback */
      if ( ( options & OPT_VI_CHAR_ARG ) != 0 ) {
        if ( ! putback )
          goto eat_multi;
        /* take the second char from pending */
        c = input.pending[ 1 ];
        input.cur_char = c;
        input.putb = input.pcnt = 0;
      }
    }
    input.cur_recipe = &r;
    return r.action;
  }
  /* otherwise multichar transition */
  if ( ! putback )
    goto eat_multi;
  input.cur_recipe = &this->recipe[ input.cur_input->def ]; /* bell or insert */
  return input.cur_recipe->action;
}
/* subtract from current position */
void
State::move_cursor_back( size_t amt ) noexcept
{
  if ( amt == 0 || this->cursor_pos == 0 )
    return;
  if ( amt == 1 ) {
    this->cursor_pos -= 1;
    if ( ( ( this->cursor_pos + 1 ) % this->cols ) == 0 ) { /* up a line */
      this->output_str( ANSI_CURSOR_UP_ONE, ANSI_CURSOR_UP_ONE_SIZE );
      this->output_fmt( ANSI_CURSOR_FWD_FMT, this->cols - 1 );
    }
    else
      this->output_str( ANSI_CURSOR_BACK_ONE, ANSI_CURSOR_BACK_ONE_SIZE );
  }
  else
    this->move_cursor( this->cursor_pos - amt );
}
/* add to current position */
void
State::move_cursor_fwd( size_t amt ) noexcept
{
  if ( amt == 0 )
    return;
  if ( amt == 1 ) {
    this->cursor_pos += 1;
    if ( ( this->cursor_pos % this->cols ) == 0 ) { /* going down a line */
      this->output_str( "\r" ANSI_CURSOR_DOWN_ONE,
                        ANSI_CURSOR_DOWN_ONE_SIZE + 1 );
    }
    else
      this->output_str( ANSI_CURSOR_FWD_ONE, ANSI_CURSOR_FWD_ONE_SIZE );
  }
  else
    this->move_cursor( this->cursor_pos + amt );
}
/* move from current position to new pos */
void
State::move_cursor( size_t new_pos ) noexcept
{
  size_t amt;
  for (;;) {
    size_t new_row = new_pos / this->cols,
           cur_row = this->cursor_pos / this->cols;
    /* if cursor is on the same line */
    if ( new_row == cur_row ) {
      if ( new_pos > this->cursor_pos ) {
        amt = new_pos - this->cursor_pos;
        this->output_fmt( ANSI_CURSOR_FWD_FMT, amt );
      }
      else if ( new_pos < this->cursor_pos ) {
        amt = this->cursor_pos - new_pos;
        this->output_fmt( ANSI_CURSOR_BACK_FMT, amt );
      }
      this->cursor_pos = new_pos;
      return;
    }
    /* cursor is on a different line, move up or down lines */
    if ( new_row > cur_row ) {
      amt = new_row - cur_row;
      this->output_fmt( ANSI_CURSOR_DOWN_FMT, amt );
      this->cursor_pos += this->cols * amt;
    }
    else { /* move up */
      amt = cur_row - new_row;
      this->output_fmt( ANSI_CURSOR_UP_FMT, amt );
      this->cursor_pos -= this->cols * amt;
    }
  }
}

bool
State::input_available( LineCookInput &input ) noexcept
{
  if ( input.input_off < input.input_len ) {
    if ( input.input_off + 3 < input.input_len )
      return true;
    char32_t c;
    return ku_utf8_to_utf32( &input.input_buf[ input.input_off ],
                             input.input_len - input.input_off, &c ) != 0;
  }
#if 0
  if ( input.putb < input.pcnt ) /* pending avail */
    return true;
  /* fix with a callback, this may be a blocking call */
  /*return this->fill_input() > 0;*/
#endif
  return false;
}

int
State::fill_input( void ) noexcept
{
  if ( this->in.input_off != this->in.input_len ) {
    ::memmove( this->in.input_buf, &this->in.input_buf[ this->in.input_off ],
               this->in.input_len - this->in.input_off );
  }
  this->in.input_len -= this->in.input_off;
  this->in.input_off  = 0;

  if ( ! this->realloc_input( this->in.input_len + LINE_BUF_LEN_INCR ))
    return LINE_STATUS_ALLOC_FAIL;
  int n = this->read( &this->in.input_buf[ this->in.input_len ],
                      this->in.input_buflen - this->in.input_len );
  if ( n <= 0 ) {
    if ( n < -1 )
      return this->error = LINE_STATUS_RD_FAIL;
    return LINE_STATUS_RD_EAGAIN;
  }
  this->in.input_len += n;
  return n;
}

void
State::color_output( char32_t c, 
                     void (State::*output_char)( char32_t ) ) noexcept
{
  uint32_t pos     = (uint32_t) c >> LC_COLOR_SHIFT;
  bool     is_norm = (pos & LC_COLOR_NORMAL) != 0,
           is_bold = (pos & LC_COLOR_BOLD) != 0;
  c &= LC_COLOR_CHAR_MASK;

  pos &= LC_COLOR_POS_MASK;
  if ( pos != 0 ) {
    ColorNode *n = this->color_ht[ pos % LC_COLOR_SIZE ];
    if ( n != NULL )
      this->output_str( (char *) n->color_buf, n->len );
  }
  if ( is_bold )
    this->output_str( ANSI_BOLD, ANSI_BOLD_SIZE );

  (this->*output_char)( c );

  if ( is_norm )
    this->output_str( ANSI_NORMAL, ANSI_NORMAL_SIZE );
}

void
State::cursor_output( char32_t c ) noexcept
{
  if ( ((uint32_t) c >> LC_COLOR_SHIFT) != 0 )
    this->color_output( c, &State::cursor_output );
  else {
    static const size_t pad = 2 + 4;
    if ( ! this->realloc_output( this->output_off + pad ) )
      return;
    int n = ku_utf32_to_utf8( c, &this->output_buf[ this->output_off ] );
    if ( n > 0 && c != 0 ) {
      this->output_off += n;
      this->cursor_pos++;
      if ( ( this->cursor_pos % this->cols ) == 0 ) {
        this->output_buf[ this->output_off++ ] = '\r';
        this->output_buf[ this->output_off++ ] = '\n';
        this->right_prompt_needed = true;
      }
    }
  }
}
/* output at cursor */
void
State::cursor_output( const char32_t *str,  size_t len ) noexcept
{
  static const size_t pad = 2;
  size_t left = this->cols - ( this->cursor_pos % this->cols );
  for (;;) {
    if ( len < left ) {
      this->output( str, len );
      this->cursor_pos += len;
      return;
    }
    if ( ! this->realloc_output( this->output_off + left * 4 + pad ) )
      return;
    /* use \r\n when crossing a row/line boundary, otherwise cursor
     * hangs at the edge of the screen when at the last column */
    this->output( str, left );
    this->output_buf[ this->output_off++ ] = '\r';
    this->output_buf[ this->output_off++ ] = '\n';
    this->right_prompt_needed = true;
    this->cursor_pos += left;
    str  = &str[ left ];
    len -= left;
    left = this->cols;
  }
}
/* erase from cursor to erase_len col/row */
void
State::cursor_erase_eol( void ) noexcept
{
  this->erase_eol_with_right_prompt();
  /* erase to the max extent of multiline input */
  size_t curs_row = this->cursor_row(),
         max_row  = this->erase_rows();
  if ( curs_row < max_row ) {
    for ( size_t row = curs_row; row < max_row; row++ ) {
      this->output_str( ANSI_CURSOR_DOWN_ONE ANSI_ERASE_LINE,
                        ANSI_CURSOR_DOWN_ONE_SIZE + ANSI_ERASE_LINE_SIZE );
    }
    this->output_fmt( ANSI_CURSOR_UP_FMT, max_row - curs_row );
  }
  /* reset, since lines are cleared */
  this->erase_len = this->edited_len;
}

void
State::output_show_string( const char32_t *str,  size_t off,
                           size_t len ) noexcept
{
  const char32_t * s;
  size_t           slen,
                   i = 0,
                   j = 0;
  if ( this->show_mode == SHOW_HISTORY && this->search_len > 0 ) {
    s    = this->search_buf;
    slen = this->search_len;
    if ( off == 0 ) {
      for ( i = 2; i < len; i++ )
        if ( str[ off + i - 2 ] == '.' ) /* skip over <num>. [hist line] */
          break;
    }
  match_substr:;
    while ( i + slen <= len ) {
      if ( casecmp<char32_t>( s, &str[ off + i ], slen ) == 0 ) {
        if ( i > j )
          this->cursor_output( &str[ off + j ], i - j );
        this->output_str( ANSI_HILITE_SELECT, ANSI_HILITE_SELECT_SIZE );
        this->cursor_output( &str[ off + i ], slen );
        this->output_str( ANSI_NORMAL, ANSI_NORMAL_SIZE );
        i += slen;
        j  = i;
      }
      else {
        i++;
      }
    }
  }
  else if ( this->show_mode == SHOW_COMPLETION && this->comp_len > 0 ) {
    /* match is a prefix of the current line buf */
    if ( this->complete_is_prefix ) {
      size_t match_len, k = 0;
      if ( off == 0 ) {
        this->cursor_output( str[ 0 ] );
        k = 1;
      }
      if ( len > k && off <= 1 ) {
        match_len = min_int<size_t>( len - k, this->comp_len );
        this->output_str( ANSI_HILITE_SELECT, ANSI_HILITE_SELECT_SIZE );
        for ( ; i < match_len; i++ )
          if ( this->comp_buf[ i ] != str[ i + 1 ] )
            break;
        this->cursor_output( &str[ 1 ], i );
        this->output_str( ANSI_NORMAL, ANSI_NORMAL_SIZE );
      }
      j = i + k;
    }
    /* match is a substring of current line buf */
    else if ( ! this->complete_has_glob ) {
      s    = this->comp_buf;
      slen = this->comp_len;
      goto match_substr;
    }
    /* otherwise, is a pattern match */
  }
  if ( j < len )
    this->cursor_output( &str[ off + j ], len - j );
}

size_t
State::output_show_line( const char32_t *show_line,  size_t len ) noexcept
{
  static const size_t SHOW_PAD = 3;
  LinePrompt &l = this->sel_left,
             &r = this->sel_right;
  size_t off = 0, j;
  if ( len > 0 && l.prompt_cols == 1 ) {
    if ( show_line[ 0 ] == '*' &&
         this->show_mode != SHOW_HELP && this->show_mode != SHOW_KEYS ) {
      this->output( l.prompt, l.prompt_len );
      this->cursor_pos++;
      off++; len--;
      j = len;
      if ( j >= SHOW_PAD && r.prompt_cols == 1 ) {
        j -= SHOW_PAD - 1;
        while ( j > 0 && show_line[ off + j - 1 ] == ' ' )
          j--;
        this->output_show_string( show_line, off, j );
        this->output( r.prompt, r.prompt_len );
        this->cursor_pos++;
        off += j + 1;
        if ( len > j + 1 )
          len -= j + 1;
        else
          len = 0;
      }
    }
  }
  this->output_show_string( show_line, off, len );
  return off + len;
}

void
State::output_show( void ) noexcept
{
  if ( this->show_len > 0 ) {
    size_t row   = this->next_row(),
           start = this->show_row_start;
    this->show_row_start = row;
    if ( row < start ) {
      row--;
      this->show_clear_lines( row + this->show_rows, start + this->show_rows );
      this->refresh_line(); /* recursive, calls output_show() again */
    }
    else {
      size_t save = this->cursor_pos,
             pos  = row * this->cols;
      this->move_cursor( this->prompt.cols + this->edited_len );
      this->cursor_erase_eol();
      this->move_cursor( pos - 1 );
      this->output_str( "\r\n", 2 );
      this->cursor_pos = pos;
      for ( size_t off = 0; off < this->show_len; ) {
        size_t len = min_int<size_t>( this->show_len - off, this->cols );
        off += this->output_show_line( &this->show_buf[ off ], len );
      }
      this->erase_len = this->edited_len;
      if ( ( this->cursor_pos % this->cols ) != 0 )
        this->output_str( ANSI_ERASE_EOL, ANSI_ERASE_EOL_SIZE );
      this->move_cursor( save );
    }
  }
}

void
State::output( char32_t c ) noexcept /* send char to output */
{
  if ( ((uint32_t) c >> LC_COLOR_SHIFT) != 0 )
    this->color_output( c, &State::output );
  else {
    if ( ! this->realloc_output( this->output_off + 4 ) )
      return;
    int n = ku_utf32_to_utf8( c, &this->output_buf[ this->output_off ] );
    if ( n > 0 && c != 0 )
      this->output_off += n;
  }
}

void
State::output( const char32_t *str,  size_t len ) noexcept /* send to output */
{
  if ( ! this->realloc_output( this->output_off + len * 4 ) )
    return;
  for ( size_t i = 0; i < len; i++ ) {
    char32_t c = str[ i ];
    if ( ((uint32_t) c >> LC_COLOR_SHIFT) != 0 )
      this->color_output( c, &State::output );
    else {
      int n = ku_utf32_to_utf8( c, &this->output_buf[ this->output_off ] );
      if ( n > 0 && c != 0 )
        this->output_off += n;
    }
  }
}

void
State::output_str( const char *str,  size_t len ) noexcept
{
  if ( ! this->realloc_output( this->output_off + len ) )
    return;
  ::memcpy( &this->output_buf[ this->output_off ], str, len );
  this->output_off += len;
}

void
State::output_fmt( const char *fmt,  size_t d ) noexcept
{
  char buf[ 40 ];
  size_t n = 0;
  /* only one number: "\033[%dA" */
  while ( *fmt != '\0' ) {
    if ( ( fmt[ 0 ] == '%' && fmt[ 1 ] == 'd' ) ||
         ( fmt[ 0 ] == '%' && fmt[ 1 ] == 'u' ) ) {
      n += uint_to_str( d, &buf[ n ], uint_digits( d ) );
      fmt += 2;
      while ( *fmt != '\0' )
        buf[ n++ ] = *fmt++;
      break;
    }
    else {
      buf[ n++ ] = *fmt++;
    }
  }
  this->output_str( buf, n );
}
/* end of current line edit */
void
State::output_newline( size_t count ) noexcept
{
  this->output_str( "\r\n", 2 );
  for ( ; count > 1; count-- ) /* if multiline and cursor is not at the end */
    this->output_str( "\n", 1 );
  this->output_flush();
}
/* send current output buf to terminal */
void
State::output_flush( void ) noexcept
{
  for (;;) {
    size_t n = this->output_off;
    if ( n == 0 )
      return;
    int nbytes = this->write( this->output_buf, n );
    if ( nbytes < 0 || (size_t) nbytes > n ) {
      this->error = LINE_STATUS_WR_FAIL;
      return;
    }
    if ( n == (size_t) nbytes ) {
      this->output_off = 0;
      return;
    }
    if ( n > (size_t) nbytes ) {
      if ( nbytes == 0 ) {
        this->error = LINE_STATUS_WR_EAGAIN;
        return;
      }
      else { /* some bytes transferred, continue trying */
        n -= (size_t) nbytes;
        ::memmove( this->output_buf, &this->output_buf[ nbytes ], n );
        this->output_off = n;
      }
    }
  }
}
