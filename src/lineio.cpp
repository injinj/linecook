#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
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

} /* extern "C" */

using namespace linecook;

int
State::line_length( void )
{
  int size = 0;
  for ( size_t i = 0; i < this->line_len; i++ ) {
    if ( this->line[ i ] != 0 ) {
      int n = ku_utf32_to_utf8_len( &this->line[ i ], 1 );
      if ( n > 0 )
        size += n;
    }
  }
  return size;
}

int
State::line_copy( char *out )
{
  int size = 0;
  for ( size_t i = 0; i < this->line_len; i++ ) {
    if ( this->line[ i ] != 0 ) {
      int n = ku_utf32_to_utf8( this->line[ i ], &out[ size ] );
      if ( n > 0 )
        size += n;
    }
  }
  return size;
}

char32_t
State::next_input_char( void )
{
  for (;;) {
    char32_t c;
    int n = ku_utf8_to_utf32( &this->input_buf[ this->input_off ],
                              this->input_len - this->input_off, &c );
    if ( n > 0 ) {
      this->input_off += n;
      return c;
    }
    if ( n < 0 ) {
      this->input_off++; /* can't wait with 4 chars, illegal utf8 */
      this->error = LINE_STATUS_BAD_INPUT;
    }
    return 0;
  }
}

int
State::eat_multichar( char32_t &c ) /* multichar actions */
{
  if ( this->pcnt == 0 ) {
    this->pending[ 0 ] = c;
    this->pcnt = 1;
    return ACTION_PENDING;
  }
  size_t potential_match = 0;
  KeyRecipe ** mc = this->cur_input->mc;
  size_t       sz = this->cur_input->mc_size;
  this->pending[ this->pcnt++ ] = c;
  this->pending[ this->pcnt ] = 0;
  /* linear scan of multichar sequences, nothing fancy;  the database is likely
   * to be less than 50 multichar actions since there are not that many keys on
   * a keyboard */
  for ( size_t i = 0; i < sz; i++ ) {
    KeyRecipe  & r   = *mc[ i ];
    const char * seq = r.char_sequence;
    if ( char32_eq( seq[ 0 ], this->pending[ 0 ] ) &&
         char32_eq( seq[ 1 ], this->pending[ 1 ] ) ) { /* at least 2 chars eq */
      bool match = true;
      uint8_t k  = 2;
      for ( ; k < this->pcnt; k++ ) { /* check if all match */
        if ( ! char32_eq( seq[ k ], this->pending[ k ] ) ) {
          match = false;
          break;
        }
      }
      if ( match ) {
        if ( seq[ k ] == 0 ) { /* if all match */
          if ( ( r.options & OPT_VI_CHAR_OP ) == 0 ) { /* if not a char arg */
          is_match:;
            this->pcnt = 0;
            this->cur_recipe = &r;
            return r.action;
          }
        }
        potential_match++;
      }
      else {
        if ( seq[ k ] == 0 && k + 1 == this->pcnt && /* the char arg */
             ( r.options & OPT_VI_CHAR_OP ) != 0 )
          goto is_match;
      }
    }
  }
  /* no matches, putback the chars */
  if ( potential_match == 0 )
    return ACTION_PUTBACK;
  return ACTION_PENDING;
}

int
State::eat_input( char32_t &c ) /* eat chars from input */
{
  bool      putback = false;
  uint8_t * charto;
  int       m = this->test( EMACS_MODE | VI_INSERT_MODE | VI_COMMAND_MODE |
                            VISUAL_MODE | SEARCH_MODE );
  /* if mode changed, select correct tables */
  if ( this->input_mode != m || this->cur_input == NULL ) {
    /* pick a translation table */
    if ( ( this->mode & VISUAL_MODE ) != 0 )
      this->cur_input = &this->visual;
    else if ( ( this->mode & SEARCH_MODE ) != 0 )
      this->cur_input = &this->search;
    else if ( ( this->mode & EMACS_MODE ) != 0 )
      this->cur_input = &this->emacs;
    else if ( ( this->mode & VI_INSERT_MODE ) != 0 )
      this->cur_input = &this->vi_insert;
    else
      this->cur_input = &this->vi_command;
    this->input_mode = m;
  }
  charto = this->cur_input->recipe;
  this->cur_recipe = NULL;

  /* eat chars from putback pending first */
  if ( this->putb > 0 ) {
    c = this->pending[ this->putb++ ];
    putback = true;
    /* if end of putback mode */
    if ( this->putb == this->pcnt )
      this->putb = this->pcnt = 0;
  }
  /* else next input char to process */
  else {
    if ( (c = this->next_input_char()) == 0 )
      return ACTION_PENDING;
    if ( this->pcnt != 0 ) { /* if pending buffer waiting for more chars */
    eat_multi:; /* a recipe below calls for multichar translation */
      for (;;) {
        int act = this->eat_multichar( c );
        if ( act == ACTION_PUTBACK ) { /* if no multichar matches */
          c = this->pending[ 0 ];
          this->putb = 1;
          putback = true; /* prevent infinite pending loop */
          break;
        }
        if ( act != ACTION_PENDING ) /* something matched */
          return act;
        if ( this->input_off == this->input_len ) /* eat more if available */
          return ACTION_PENDING;
        if ( (c = this->next_input_char()) == 0 )
          return ACTION_PENDING;
      }
    }
  }
  /* match a char to an action, multi-byte chars use default recipe */
  uint8_t uc = ( c < sizeof( this->cur_input->recipe ) ?
                 (uint8_t) c : this->cur_input->def );
  KeyRecipe &r = this->recipe[ charto[ uc ] ];

  if ( r.char_sequence == NULL ) { /* if default action, no further processing*/
    this->cur_recipe = &r;
    return r.action;
  }
  /* if single char transition (ctrl codes, escape) */
  if ( r.char_sequence[ 1 ] == 0 ) {
    /* for escape in insert mode, check if arrow keys are used */
    if ( r.action == ACTION_VI_ESC ) {
      if ( ! putback && ( this->input_available() || this->is_emacs_mode() ) )
        goto eat_multi;
    }
    else {
      /* get the second char by using putback */
      if ( ( r.options & OPT_VI_CHAR_OP ) != 0 ) {
        if ( ! putback )
          goto eat_multi;
        c = this->pending[ 1 ];     /* take the second char from pending */
        this->putb = this->pcnt = 0;
      }
      else if ( this->is_vi_command_mode() ) {
        if ( ( r.options & OPT_VI_REPC_OP ) != 0 ) {
          if ( this->vi_repeat_cnt != 0 || ( c >= '1' && c <= '9' ) ) {
            this->vi_repeat_cnt = this->vi_repeat_cnt * 10 + ( c - '0' );
            this->cur_recipe = this->vi_repeat_default;
            return this->cur_recipe->action;
          }
        }
      }
    }
    this->cur_recipe = &r;
    return r.action;
  }
  /* otherwise multichar transition */
  if ( ! putback )
    goto eat_multi;
  this->cur_recipe = &this->recipe[ this->cur_input->def ]; /* bell or insert */
  return this->cur_recipe->action;
}

void
State::move_cursor_back( size_t amt ) /* subtract from current position */
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

void
State::move_cursor_fwd( size_t amt ) /* add to current position */
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

void
State::move_cursor( size_t new_pos ) /* move from current position to new pos */
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
State::input_available( void )
{
  if ( this->input_off < this->input_len ) {
    if ( this->input_off + 3 < this->input_len )
      return true;
    char32_t c;
    return ku_utf8_to_utf32( &this->input_buf[ this->input_off ],
                             this->input_len - this->input_off, &c ) != 0;
  }
#if 0
  if ( this->putb < this->pcnt ) /* pending avail */
    return true;
  /* fix with a callback, this may be a blocking call */
  /*return this->fill_input() > 0;*/
#endif
  return false;
}

int
State::fill_input( void ) /* read input, move input bytes to make space */
{
  if ( this->input_off != this->input_len ) {
    ::memmove( this->input_buf, &this->input_buf[ this->input_off ],
               this->input_len - this->input_off );
  }
  this->input_len -= this->input_off;
  this->input_off  = 0;

  if ( ! this->realloc_input( this->input_len + LINE_BUF_LEN_INCR * 16 ) )
    return LINE_STATUS_ALLOC_FAIL;
  int n = this->read( &this->input_buf[ this->input_len ],
                      this->input_buflen - this->input_len );
  if ( n <= 0 ) {
    if ( n < -1 )
      return this->error = LINE_STATUS_RD_FAIL;
    return LINE_STATUS_RD_EAGAIN;
  }
  this->input_len += n;
  return n;
}

void
State::cursor_output( char32_t c )
{
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

void
State::cursor_output( const char32_t *str,  size_t len ) /* output at cursor */
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

void
State::cursor_erase_eol( void ) /* erase from cursor to erase_len col/row */
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
State::output_show_string( const char32_t *str,  size_t off,  size_t len )
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
        this->output_str( ANSI_VISUAL_SELECT, ANSI_VISUAL_SELECT_SIZE );
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
        match_len = min<size_t>( len - k, this->comp_len );
        this->output_str( ANSI_VISUAL_SELECT, ANSI_VISUAL_SELECT_SIZE );
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
State::output_show_line( const char32_t *show_line,  size_t len )
{
  LinePrompt &l = this->sel_left,
             &r = this->sel_right;
  size_t off = 0, j;
  if ( len > 0 && l.prompt_cols == 1 ) {
    if ( show_line[ 0 ] == '*' ) {
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
State::output_show( void )
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
        size_t len = min<size_t>( this->show_len - off, this->cols );
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
State::output( char32_t c ) /* send char to output */
{
  if ( ! this->realloc_output( this->output_off + 4 ) )
    return;
  int n = ku_utf32_to_utf8( c, &this->output_buf[ this->output_off ] );
  if ( n > 0 && c != 0 )
    this->output_off += n;
}

void
State::output( const char32_t *str,  size_t len ) /* send bytes to output */
{
  if ( ! this->realloc_output( this->output_off + len * 4 ) )
    return;
  for ( size_t i = 0; i < len; i++ ) {
    int n = ku_utf32_to_utf8( str[ i ], &this->output_buf[ this->output_off ] );
    if ( n > 0 && str[ i ] != 0 )
      this->output_off += n;
  }
}

void
State::output_str( const char *str,  size_t len ) /* send bytes to output */
{
  if ( ! this->realloc_output( this->output_off + len ) )
    return;
  ::memcpy( &this->output_buf[ this->output_off ], str, len );
  this->output_off += len;
}

void
State::output_fmt( const char *fmt,  size_t d )
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

void
State::output_newline( size_t count ) /* end of current line edit */
{
  this->output_str( "\r\n", 2 );
  for ( ; count > 1; count-- ) /* if multiline and cursor is not at the end */
    this->output_str( "\n", 1 );
  this->output_flush();
}

void
State::output_flush( void ) /* send current output buf to terminal */
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
