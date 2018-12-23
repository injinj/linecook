#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <wctype.h>
#include <linecook/linecook.h>
#include <linecook/xwcwidth9.h>

extern "C" {

LineCook *
lc_create_state( int cols,  int lines )
{
  void *p = ::malloc( sizeof( linecook::State ) );
  if ( p == NULL )
    return NULL;
  return new ( p ) linecook::State( cols, lines );
}

void
lc_release_state( LineCook *state )
{
  delete static_cast<linecook::State *>( state );
}

int
lc_set_geom( LineCook *state,  int cols,  int lines )
{
  return static_cast<linecook::State *>( state )->set_geom( cols, lines );
}

void
lc_set_word_break( LineCook *state,  const char *brk,  size_t brk_len )
{
  return static_cast<linecook::State *>( state )->
    set_word_break( brk, brk_len );
}

void
lc_set_completion_break( LineCook *state,  const char *brk,  size_t brk_len )
{
  return static_cast<linecook::State *>( state )->
    set_completion_break( brk, brk_len );
}

void
lc_set_quotables( LineCook *state,  const char *qc,  size_t qc_len,
                  char quote )
{
  return static_cast<linecook::State *>( state )->
    set_quotables( qc, qc_len, quote );
}

int
lc_get_line( LineCook *state )
{
  return static_cast<linecook::State *>( state )->get_line();
}

int
lc_continue_get_line( LineCook *state )
{
  return static_cast<linecook::State *>( state )->continue_get_line();
}

int
lc_max_timeout( LineCook *state,  int time_ms )
{
  return static_cast<linecook::State *>( state )->max_timeout( time_ms );
}

void
lc_clear_line( LineCook *state )
{
  return static_cast<linecook::State *>( state )->clear_line();
}
} /* extern "C" */

using namespace linecook;

State::State( int num_cols,  int num_lines )
{
  static const char def_brk[]   = " \t\n!\"#$%&'()*+,-./:;<=>?@[\\]^`{|}~";
  static const char def_plete[] = " \t\n\\\"';:{[($`?*|";
  static const char def_quote[] = " \t\n\\\"'@<>=;:|&#$`{}[]()";
  ::memset( this, 0, sizeof( *this ) );
  this->left_prompt_needed = true;
  this->mode       = VI_INSERT_MODE;
  this->cols       = num_cols;
  this->lines      = num_lines;
  this->show_lines = num_lines / 2;
  this->set_word_break( def_brk, sizeof( def_brk ) - 1 );
  this->set_completion_break( def_plete, sizeof( def_plete ) - 1 );
  this->set_quotables( def_quote, sizeof( def_quote ) - 1, '\"' );
  this->set_recipe( lc_default_key_recipe, lc_default_key_recipe_size );
}

State::~State()
{
  if ( this->line )             ::free( this->line );
  if ( this->input_buf )        ::free( this->input_buf );
  if ( this->output_buf )       ::free( this->output_buf );
  if ( this->undo.buf )         ::free( this->undo.buf );
  if ( this->hist.buf )         ::free( this->hist.buf );
  if ( this->comp.buf )         ::free( this->comp.buf );
  if ( this->edit.buf )         ::free( this->edit.buf );
  if ( this->keys.buf )         ::free( this->keys.buf );
  if ( this->yank.buf )         ::free( this->yank.buf );
  if ( this->search_buf )       ::free( this->search_buf );
  if ( this->comp_buf )         ::free( this->comp_buf );
  if ( this->show_buf )         ::free( this->show_buf );
  if ( this->prompt.str )       ::free( this->prompt.str );
  if ( this->prompt.fmt )       ::free( this->prompt.fmt );
  if ( this->prompt.fmt2 )      ::free( this->prompt.fmt2 );
  if ( this->prompt.host )      ::free( this->prompt.host );
  if ( this->prompt.user )      ::free( this->prompt.user );
  if ( this->prompt.home )      ::free( this->prompt.home );
  if ( this->prompt.cwd )       ::free( this->prompt.cwd );
  if ( this->prompt.date )      ::free( this->prompt.date );
  if ( this->prompt.shname )    ::free( this->prompt.shname );
  if ( this->prompt.ttyname )   ::free( this->prompt.ttyname );
  if ( this->r_ins.prompt )     ::free( this->r_ins.prompt );
  if ( this->r_cmd.prompt )     ::free( this->r_cmd.prompt );
  if ( this->r_emacs.prompt )   ::free( this->r_emacs.prompt );
  if ( this->r_srch.prompt )    ::free( this->r_srch.prompt );
  if ( this->r_comp.prompt )    ::free( this->r_comp.prompt );
  if ( this->r_visu.prompt )    ::free( this->r_visu.prompt );
  if ( this->r_ouch.prompt )    ::free( this->r_ouch.prompt );
  if ( this->sel_left.prompt )  ::free( this->sel_left.prompt );
  if ( this->sel_right.prompt ) ::free( this->sel_right.prompt );
  if ( this->mark )             ::free( this->mark );
  if ( this->cvt )              ::free( this->cvt );
  if ( this->multichar )        ::free( this->multichar );
}

void
State::set_word_break( const char *brk,  size_t brk_len )
{
  ::memset( this->word_brk, 0, sizeof( this->word_brk ) );
  for ( size_t i = 0; i < brk_len; i++ )
    this->set_char_bit( this->word_brk, brk[ i ] );
}

void
State::set_completion_break( const char *brk,  size_t brk_len )
{
  ::memset( this->plete_brk, 0, sizeof( this->plete_brk ) );
  for ( size_t i = 0; i < brk_len; i++ )
    this->set_char_bit( this->plete_brk, brk[ i ] );
}

void
State::set_quotables( const char *qc,  size_t qc_len,  char quote )
{
  this->quote_char = quote;
  ::memset( this->quotable, 0, sizeof( this->quotable ) );
  for ( size_t i = 0; i < qc_len; i++ )
    this->set_char_bit( this->quotable, qc[ i ] );
}

int
State::set_geom( int num_cols,  int num_lines )
{
  this->error = 0;
  if ( (size_t) num_cols != this->cols || (size_t) num_lines != this->lines ) {
    double ratio = (double) this->show_lines / (double) this->lines;
    this->cols              = num_cols;
    this->lines             = num_lines;
    this->show_lines        = (size_t) ( (double) num_lines * ratio );
    if ( this->show_lines < 4 && num_lines > 5 )
      this->show_lines = 4;
    this->prompt.flags_mask = 0;
    this->prompt.cols      -= this->prompt.pad_cols;
    this->prompt.pad_cols   = 0;
    this->update_prompt( true );
    if ( this->cursor_pos == 0 && this->edited_len == 0 )
      return 0;
    if ( this->show_mode != SHOW_NONE )
      this->show_clear(); /* calls refresh_line */
    else
      this->refresh_line();
  }
  return this->error;
}

void
State::reset_state( void )
{
  this->left_prompt_needed = true;
  this->prompt.cols -= this->prompt.pad_cols;
  this->prompt.pad_cols = 0;
  this->edited_len  = 0;   /* reset line state for next get_line call */
  this->erase_len   = 0;
  this->cursor_pos  = 0;
  this->refresh_pos = 0;

  if ( this->is_emacs_mode() )
    this->reset_emacs_mode();     /* reset search / replace flags */
  else
    this->reset_vi_insert_mode(); /* go back to vi insert mode */
  this->right_prompt_needed = false;

  LineSave::reset( this->undo );  /* clear undo buffer */

  /*this->yank_off   = 0;   ... don't clear yank buffer */
  this->pcnt        = 0;             /* clear putback buffer */
  this->putb        = 0;
  this->cur_char    = 0;
  this->action      = ACTION_PENDING;/* clear current action, recipe */
  this->cur_recipe  = NULL;
  this->last_action = ACTION_PENDING;

  this->last_repeat_action = ACTION_PENDING; /* clear repeating with '.' */
  this->last_repeat_recipe = NULL;
  this->last_repeat_char   = 0;
  this->vi_repeat_cnt      = 0;  /* digits input */
  this->emacs_arg_cnt      = 0;  /* meta digits input */
  this->emacs_arg_neg      = 0;  /* meta minus */
  this->last_yank_start    = 0;  /* saved offsets of yank paste */
  this->last_yank_size     = 0;

  this->input_mode = 0;        /* reset input, mode is set in eat_input */
  this->cur_input  = NULL;

  this->hist.off = this->hist.max; /* set history to the last entry */
  this->hist.idx = 0;

  this->save_mode   = this->mode; /* save mode for hist searches */
  this->save_action = this->action;

  LineSave::reset( this->comp ); /* reset any completions */
  LineSave::reset( this->edit ); /* reset any history edits */

  this->reset_complete();
  this->reset_yank();
  this->visual_off   = 0;  /* reset visual position */
}

void
State::clear_line( void )
{
  if ( ! this->refresh_needed && ! this->left_prompt_needed ) {
    this->refresh_needed = true;
    this->refresh_pos = this->cursor_pos;
    this->move_cursor( 0 );
    this->cursor_erase_eol();
    this->output_str( ANSI_ERASE_LINE, ANSI_ERASE_LINE_SIZE );
    this->output_flush();
  }
}

void
State::refresh_line( void )
{
  size_t save = this->cursor_pos,
         ext  = this->erase_len;
  if ( this->refresh_pos != 0 ) {
    save = this->refresh_pos;
    this->refresh_pos = 0;
  }
  this->cursor_erase_eol(); /* fix display flakyness */
  this->move_cursor( 0 );   /* redisplay prompt */
  if ( this->prompt.lines > 0 )
    this->output_fmt( ANSI_CURSOR_UP_FMT, this->prompt.lines );
  this->output_prompt();
  this->cursor_pos = this->prompt.cols;
  if ( this->edited_len > 0 ) {         /* redisplay line */
    if ( this->is_visual_mode() )
      this->refresh_visual_line(); /* visual mode highlighted */
    else
      this->cursor_output( this->line, this->edited_len );
  }
  this->erase_len = ext;              /* reclear the cleared space */
  if ( this->show_mode != SHOW_NONE )
    this->output_show();          /* show the show buffer */
  else
    this->cursor_erase_eol();     /* erase the rest of the line */
  this->move_cursor( save ); /* move cursor back to save */
  //this->output_flush();
}

void
State::visual_bounds( size_t off,  size_t &start,  size_t &end )
{
  if ( off < this->visual_off ) {
    start = off;
    end   = this->visual_off;
  }
  else {
    start = this->visual_off;
    end   = off;
  }
  if ( end < this->edited_len )
    end += this->char_width_next( end );
}

void
State::refresh_visual_line( void )
{
  size_t save = this->cursor_pos,
         start, end;
  this->visual_bounds( save - this->prompt.cols, start, end );
  this->move_cursor( this->prompt.cols );
  this->cursor_output( this->line, start );
  if ( this->is_visual_mode() )
    this->output_str( ANSI_VISUAL_SELECT, ANSI_VISUAL_SELECT_SIZE );
  this->cursor_output( &this->line[ start ], end - start );
  if ( this->is_visual_mode() )
    this->output_str( ANSI_NORMAL, ANSI_NORMAL_SIZE );
  if ( end < this->edited_len )
    this->cursor_output( &this->line[ end ], this->edited_len - end );
  this->move_cursor( save );
}

int
State::get_line( void ) /* main processing loop, eat chars until input empty */
{
  if ( this->prompt.is_continue ) {
    this->prompt.is_continue = false;
    this->init_lprompt();
  }
  return this->do_get_line();
}

int
State::max_timeout( int time_ms )
{
  uint64_t ms = 0, left;
  if ( this->bell_cnt ) {
    ms = State::time_ms();
    if ( ms >= this->bell_time + 500 )
      return 0;
    left = ( this->bell_time + 500 ) - ms;
    if ( left < (uint64_t) time_ms )
      time_ms = (int) left;
  }
  uint32_t fl = this->prompt.flags & ~this->prompt.flags_mask;
  if ( ( fl & ( P_HAS_ANY_TIME | P_HAS_DATE ) ) != 0 ) {
    if ( ms == 0 )
      ms = State::time_ms();
    left = 1000 - ( ms % 1000 );
    if ( left < (uint64_t) time_ms ) {
      time_ms = (int) left;
      if ( time_ms == 0 )
        time_ms = 1;
    }
  }
  return time_ms;
}

int
State::continue_get_line( void ) /* use prompt2 with get_line */
{
  if ( ! this->prompt.is_continue ) {
    this->prompt.is_continue = true;
    this->init_lprompt();
  }
  return this->do_get_line();
}

int
State::do_get_line( void )
{
  size_t save_edited_len  = this->edited_len;
  int    status;
  bool   repeat_countdown = false;

  this->error = 0;
  if ( this->output_off > 0 ) { /* check if output ready */
    this->output_flush();
    if ( this->error != 0 )
      return this->error;
  }
  if ( this->refresh_needed ) {
    this->refresh_line();
    this->refresh_needed = false;
  }
  if ( ! this->left_prompt_needed && this->update_prompt_time() ) {
    size_t save = this->cursor_pos;
    this->move_cursor( 0 );
    if ( this->prompt.lines > 0 )
      this->output_fmt( ANSI_CURSOR_UP_FMT, this->prompt.lines );
    this->output_prompt();
    this->cursor_pos = this->prompt.cols;
    this->move_cursor( save );
  }
  /* emit prompt if necessary */
  if ( this->left_prompt_needed ) {
    this->left_prompt_needed = false;
    this->update_prompt( false );
    this->output_prompt();
    this->cursor_pos = this->prompt.cols;
    this->right_prompt_needed = true;
    if ( this->mark_upd > 0 ) /* reset temporary marks, this is here so that */
      this->reset_marks();  /* lc_add_history() can send a mark into history */
  }
  if ( this->bell_cnt && State::time_ms() >= this->bell_time + 500 ) {
    this->bell_cnt = 1; /* clear bell, next rprompt output */
    this->right_prompt_needed = true;
  }
  for (;;) {
    if ( this->cursor_pos < this->prompt.cols )
      return LINE_STATUS_BAD_CURSOR;
    /* catch any error that occured last iteration */
    if ( this->error != 0 ) /* LINE_STATUS_WR_EAGAIN, LINE_STATUS_ALLOC_FAIL */
      return this->error;
    if ( this->last_action == ACTION_VI_REPEAT_DIGIT &&
         this->action != ACTION_PENDING &&
         this->action != ACTION_VI_REPEAT_DIGIT &&
         this->action != ACTION_GOTO_ENTRY ) {
      if ( this->vi_repeat_cnt > 1 ) {
        this->vi_repeat_cnt--;
        repeat_countdown = true;
      }
      else {
        this->vi_repeat_cnt = 0;
        repeat_countdown = false;
      }
    }
    else if ( this->last_action == ACTION_EMACS_ARG_DIGIT &&
              this->action != ACTION_PENDING &&
              this->action != ACTION_EMACS_ARG_DIGIT ) {
      if ( this->emacs_arg_cnt > 1 ) {
        this->emacs_arg_cnt--;
        repeat_countdown = true;
      }
      else {
        this->emacs_arg_cnt = 0;
        this->emacs_arg_neg = 0;
        repeat_countdown = false;
      }
    }
    /* read input if empty and not repeating a command */
    if ( ! repeat_countdown && ! this->input_available() ) {
      int n;
      if ( (n = this->fill_input()) <= 0 ) {
        /* check whether last char on line is double wide, if so, increase
         * prompt cols so that line fits without the double char overflowing */
        if ( this->edited_len + this->prompt.cols >= this->cols - 1 ||
             this->prompt.pad_cols != 0 ) {
          uint8_t old_pad = this->prompt.pad_cols,
                  new_pad = 0;
          this->prompt.cols -= old_pad;
          this->prompt.pad_cols = 0;
          /* if line fits without padding */
          if ( this->edited_len + this->prompt.cols < this->cols ) {
            this->refresh_needed = true;
          }
          /* check if overflow */
          else if ( this->prompt.cols < this->cols ) {
            for (;;) {
              size_t i       = this->cols - ( this->prompt.cols + new_pad );
              bool   is_over = false;
              if ( i == 0 ) {
                new_pad = 0;
                break;
              }
              for ( size_t j = i; j - 1 < this->edited_len; j += this->cols ) {
                if ( wcwidth9( this->line[ j - 1 ] ) == 2 ) {
                  is_over = true;
                  break;
                }
              }
              if ( ! is_over )
                break;
              new_pad++;
            }
            this->prompt.cols += new_pad;
            this->prompt.pad_cols = new_pad;
            if ( old_pad != new_pad )
              this->refresh_needed = true;
          }
          /* fresh prompt and line */
          if ( this->refresh_needed ) {
            size_t save_pos = this->cursor_pos;
            this->erase_len  -= old_pad;
            this->erase_len  += new_pad;
            this->move_cursor( 0 );
            this->refresh_line();
            this->move_cursor( save_pos + new_pad - old_pad );
            this->refresh_needed = false;
          }
        }
        /* if line wrapped or shrunk, show buffer needs adjusting */
        if ( save_edited_len != this->edited_len && this->show_len != 0 ) {
          size_t old_pos = save_edited_len + this->prompt.cols,
                 new_pos = this->edited_len + this->prompt.cols;
          if ( old_pos / this->cols != new_pos / this->cols )
            this->output_show();
        }
        /* re-highlight the visual area after cursor movement */
        if ( this->is_visual_mode() ) {
          this->refresh_visual_line();
        }
        /* right prompt needs updating on erase eol / newline conditions */
        if ( this->right_prompt_needed ) {
          this->output_right_prompt();
          this->right_prompt_needed = false;
        }
        /* some insert mode commands which are enabled for command mode move
         * the cursor at the end of the line, this corrects that */
        if ( this->is_vi_command_mode() && this->edited_len > 0 &&
             this->cursor_pos == this->edited_len + this->prompt.cols ) {
          int w = this->char_width_back( this->edited_len );
          this->move_cursor_back( w );
        }
        this->output_flush();
        if ( this->error != 0 )
          return this->error;
        return n; /* <= 0 */
      }
    }
    if ( ! repeat_countdown ) {
      /* process next char or function */
      if ( this->cur_recipe != NULL &&
           ( this->cur_recipe->options & OPT_REPEAT ) != 0 ) {
        this->last_repeat_action = this->action;
        this->last_repeat_recipe = this->cur_recipe;
        this->last_repeat_char   = this->cur_char;
      }
      int a = this->eat_input( this->cur_char );
      if ( a == ACTION_PENDING )
        continue;
      if ( a == ACTION_DECR_SHOW || a == ACTION_INCR_SHOW ) {
        this->incr_show_size( a == ACTION_DECR_SHOW ? -1 : 1 );
        continue;
      }
      this->last_action = this->action;
      this->action = a;
      if ( this->action == ACTION_REPEAT_LAST ) {
        this->action     = this->last_repeat_action;
        this->cur_recipe = this->last_repeat_recipe;
        this->cur_char   = this->last_repeat_char;
      }
    }
    status = this->dispatch();
    if ( status != LINE_STATUS_OK )
      break;
  }
  return status;
}

size_t
State::match_paren( size_t off )
{
  size_t   tos = 0;
  int      dir;
  char32_t c = this->line[ off ], d;

  switch ( c ) {
    case '(': dir =  1; d = ')'; break;
    case ')': dir = -1; d = '('; break;
    case '{': dir =  1; d = '}'; break;
    case '}': dir = -1; d = '{'; break;
    case '[': dir =  1; d = ']'; break;
    case ']': dir = -1; d = '['; break;
    default:  return off;
  }
  /* a recursive matcher, doesn't skip over quotes or anything fancy */
  for (;;) {
    if ( dir < 0 ) {
      if ( off == 0 )
        return off;
      off -= 1;
    }
    else {
      if ( off == this->edited_len )
        return off;
      off += 1;
    }
    if ( d == this->line[ off ] ) {
      if ( tos == 0 )
        return off;
      tos -= 1;
    }
    else if ( c == this->line[ off ] )
      tos += 1;
  }
}

void
State::incr_decr( int64_t delta )
{
  size_t off    = this->cursor_pos - this->prompt.cols, /* idx in line[] */
         istart = off;
  if ( istart < this->edited_len ) {
    /* find the start of the number (no hex here) */
    while ( istart > 0 && isdig<char32_t>( this->line[ istart - 1 ] ) )
      istart--;
    if ( istart == off && ! isdig<char32_t>( this->line[ istart ] ) )
      return this->bell(); /* no number found */
    if ( istart > 0 && this->line[ istart - 1 ] == '-' )
      istart--;
    /* find the widths of the old int and the new int */
    int64_t ival = str_to_int( &this->line[ istart ],
                               this->edited_len - istart );
    size_t  d1 = int_digits( ival ),
            d2 = int_digits( ival + delta ),
            d3 = max<size_t>( d1, d2 );
    /* insert a char for an extra digit or '-' sign */
    if ( d1 != d3 ) {
      if ( ! this->realloc_line( this->edited_len + 1 ) )
        return;
      size_t size = this->edited_len - istart;
      move<char32_t>( &this->line[ istart + 1 ], &this->line[ istart ], size );
      this->extend( this->edited_len + 1 );
      int_to_str( ival + delta, &this->line[ istart ], d2 );
      if ( d2 < d3 )
        this->line[ istart + d2 ] = ' ';
      this->move_cursor_back( off - istart );
      this->cursor_output( &this->line[ istart ], size + 1 );
      this->move_cursor_back( size - ( off - istart ) + 1 );
    }
    /* replace existing digits */
    else {
      int_to_str( ival + delta, &this->line[ istart ], d2 );
      if ( d2 < d3 )
        this->line[ istart + d2 ] = ' ';
      this->move_cursor_back( off - istart );
      this->cursor_output( &this->line[ istart ], d3 );
      this->move_cursor_back( d3 - ( off - istart ) );
    }
  }
}

bool
State::do_realloc( void *buf,  size_t &len,  size_t newlen )
{
  void * newbuf;
  if ( newlen >= LINE_BUF_BIG_INCR )
    newlen = align<size_t>( newlen, LINE_BUF_BIG_INCR );
  else
    newlen = align<size_t>( newlen, LINE_BUF_LEN_INCR );
  newbuf = ::realloc( *(void **) buf, newlen );
  if ( newbuf == NULL ) {
    this->error = LINE_STATUS_ALLOC_FAIL;
    return false;
  }
  *(void **) buf = newbuf;
  len = newlen;
  return true;
}

void
State::show_clear( void )
{
  this->show_mode = SHOW_NONE;
  this->erase_len = this->edited_len +
    ( this->cols -
      ( this->edited_len + this->prompt.cols ) % this->cols ) +
    this->show_len;
  this->show_len       = 0;
  this->show_rows      = 0;
  this->show_row_start = 0;
  this->refresh_line();
}

void
State::show_clear_lines( size_t from_row,  size_t to_row )
{
  if ( from_row >= to_row )
    return;

  size_t save = this->cursor_pos,
         row  = this->next_row() + from_row,
         pos  = row * this->cols;
  for (;;) {
    this->move_cursor( pos );
    this->output_str( ANSI_ERASE_LINE, ANSI_ERASE_LINE_SIZE );
    if ( ++from_row == to_row )
      break;
    pos += this->cols;
  }
  this->move_cursor( save );
}

void
State::incr_show_size( int amt )
{
  ShowMode m = this->show_mode;
  if ( m != SHOW_NONE )
    this->show_clear();
  if ( amt < 0 ) {
    if ( this->show_lines > 4 )
      this->show_lines--;
  }
  else {
    if ( this->show_lines + this->prompt.lines + 1 < this->lines )
      this->show_lines++;
  }
  switch ( m ) {
    case SHOW_NONE:       return;
    case SHOW_UNDO:       this->show_undo(); break;
    case SHOW_YANK:       this->show_yank(); break;
    case SHOW_HISTORY:    this->show_history_index(); break;
    case SHOW_COMPLETION: return;
    case SHOW_KEYS:       this->show_keys(); break;
  }
  this->output_show();
}

void
State::push_undo( void )
{
  if ( ! this->realloc_lsb( this->undo, this->undo.off +
                             LineSave::size( this->edited_len ) ) )
    return;
  /* check if undo is the same as the last undo */
  if ( LineSave::equals( this->undo, this->undo.off, this->line,
                         this->edited_len ) ) {
    LineSave &ls = LineSave::line( this->undo, this->undo.off );
    ls.cursor_off = this->cursor_pos - this->prompt.cols; /* save cursor */
    return;
  }
  this->undo.idx = ++this->undo.cnt + 1;
  this->undo.max = this->undo.off;
  this->undo.off = LineSave::make( this->undo, this->line, this->edited_len,
                                   this->cursor_pos - this->prompt.cols,
                                   this->undo.cnt );
}

LineSave *
State::pop_undo( void )
{
  size_t off = 0;
  if ( this->undo.idx > 0 )
    off = LineSave::find_lt( this->undo, this->undo.max, this->undo.idx );
  if ( off == 0 ) {
    this->undo.idx = 0;
    return NULL;
  }
  if ( this->undo.idx > this->undo.cnt )
    this->push_undo();
  /* pop one undo from top of stack */
  LineSave & ls = LineSave::line( this->undo, off );
  this->undo.off = ls.line_off;
  this->undo.idx = ls.index;
  return &ls;
}

LineSave *
State::revert_undo( void )
{
  if ( this->undo.first > 0 ) {
    LineSave &ls = LineSave::line( this->undo, this->undo.first );
    this->undo.off = ls.line_off;
    this->undo.idx = ls.index;
    return &ls;
  }
  return NULL;
}

LineSave *
State::peek_undo( void )
{
  size_t off = 0;
  if ( this->undo.idx > 0 )
    off = LineSave::find_lt( this->undo, this->undo.max, this->undo.idx );
  if ( off == 0 )
    return NULL;
  return &LineSave::line( this->undo, off );
}

LineSave *
State::push_redo( void )
{
  size_t off;
  if ( this->undo.max == this->undo.off )
    return NULL;

  off = LineSave::find_gteq( this->undo, this->undo.max, this->undo.idx+1 );
  if ( off != 0 ) {
    LineSave &undo = LineSave::line( this->undo, off );
    this->undo.off = off;
    this->undo.idx = undo.index;
    return &undo;
  }
  return NULL;
}

bool
State::show_undo( void )
{
  size_t cur_rows = this->show_rows;
  this->show_mode = SHOW_UNDO;
  this->show_pg = this->pgcount( this->undo ) - 1;
  if ( this->show_save( this->undo.idx, 1 ) ) {
    if ( this->show_rows < cur_rows )
      this->show_clear_lines( this->show_rows, cur_rows );
    return true;
  }
  if ( cur_rows > 0 )
    this->show_clear_lines( 0, cur_rows );
  return false;
}

bool
State::show_lsb( ShowMode m,  LineSaveBuf &lsb )
{
  size_t lines_per_pg = this->max_show_lines(),
         idx_start    = ( this->show_pg + 1 ) * lines_per_pg;
  if ( idx_start < lsb.cnt )
    idx_start = lsb.cnt - idx_start + 1;
  else
    idx_start = 1;
  this->show_mode = m;
  if ( this->show_save( lsb.idx, idx_start ) )
    return true;
  this->show_mode = SHOW_NONE;
  return false;
}

ShowState::ShowState( State &state )
{
  this->zero();
  switch ( state.show_mode ) {
    default: break;
    case SHOW_HISTORY:
      this->lsb = &state.hist;
      this->off = state.hist.off;
      this->cnt = state.hist.cnt;
      this->has_local_edit = true;
      this->show_index     = true;
      break;
    case SHOW_COMPLETION:
      this->lsb = &state.comp;
      this->off = state.comp.off;
      this->cnt = state.comp.cnt;
      this->left_overflow = true;
      break;
    case SHOW_UNDO:
      this->lsb = &state.undo;
      this->off = state.undo.first;
      this->cnt = LineSave::card( state.undo );
      break;
    case SHOW_KEYS:
      this->lsb = &state.keys;
      this->off = state.keys.first;
      this->cnt = state.keys.cnt;
      break;
    case SHOW_YANK:
      this->lsb = &state.yank;
      this->off = state.yank.first;
      this->cnt = state.yank.cnt;
      break;
  }
}

size_t
State::pgcount( LineSaveBuf &lsb )
{
  size_t lines_per_pg = this->max_show_lines();
  size_t count = lsb.cnt / lines_per_pg;
  if ( count * lines_per_pg < lsb.cnt )
    count++;
  return count;
}

size_t
State::pgnum( LineSaveBuf &lsb )
{
  if ( lsb.cnt > 0 ) {
    size_t lines_per_pg = this->max_show_lines();
    LineSave &line = LineSave::line( lsb, lsb.first );
    if ( lsb.idx >= line.index ) {
      size_t off = 1 + lsb.idx - line.index;
      if ( lsb.cnt >= off )
        return ( lsb.cnt - off ) / lines_per_pg;
    }
    return this->pgcount( lsb );
  }
  return 0;
}

bool
State::show_update( size_t old_idx,  size_t new_idx )
{
  ShowState  state( *this );
  size_t     save  = this->cursor_pos,
             row   = this->next_row(),
             pos   = row * this->cols;
  char32_t * buf   = this->show_buf;
  size_t     idx   = this->show_start;
  bool       found;

  if ( state.lsb == NULL )
    return false;
  state.off = LineSave::find( *state.lsb, state.off, idx );
  if ( state.off == 0 )
    return false;
  for ( found = false; ; ) {
    LineSave &ls = LineSave::line( *state.lsb, state.off );
    if ( ls.index > this->show_end )
      break;
    bool display = false;
    if ( new_idx == ls.index ) {
      buf[ 0 ] = '*';
      display = true;
      found = true;
    }
    else if ( old_idx == ls.index ) {
      this->show_line( state, buf, new_idx, NULL ); /* there may be an edit */
      display = true;
    }
    if ( display ) {
      this->move_cursor( pos );
      this->output_show_line( buf, this->cols - SHOW_PAD + 2 );
    }
    if ( ls.index == this->show_end )
      break;
    buf = &buf[ this->cols ];
    pos += this->cols;
    state.off = LineSave::find_gteq( *state.lsb, state.off, ls.index + 1 );
    if ( state.off == 0 )
      break;
  }
  this->move_cursor( save );
  return found;
}

bool
State::show_save( size_t cur_idx,  size_t start_idx )
{
  ShowState  state( *this );
  LineSave * ls;
  size_t     rows, row_off, max_rows;
  char32_t * buf;
  bool       is_first,
             is_last = false;

  if ( state.lsb == NULL )
    return false;
  this->show_rows = 0;
  this->show_cols = this->cols;
  if ( state.off == 0 ) {
    if ( (state.off = state.lsb->first) == 0 )
      return false;
  }
  state.off = LineSave::find_gteq( *state.lsb, state.off, start_idx );
  if ( state.off == 0 )
    return false;
  is_first = ( state.off == state.lsb->first );

  max_rows = this->max_show_lines();
  rows     = max_rows;
  if ( state.cnt < rows )
    rows = state.cnt;
  this->show_rows = rows;
  if ( rows == 0 )
    return false;
  if ( ! this->realloc_show( this->cols * rows ) )
    return false;

  buf = this->show_buf;
  set<char32_t>( buf, ' ', this->cols * rows );
  this->show_len = this->cols * rows;

  this->show_line( state, buf, cur_idx, &ls );
  this->show_start = ls->index;
  row_off = 0;
  for ( char32_t *b = buf; ; ) {
    b = &b[ this->cols ];
    this->show_end = ls->index;
    state.off = LineSave::find_gteq( *state.lsb, state.off, ls->index + 1 );
    row_off++;
    if ( state.off == 0 || row_off == state.cnt ) {
      is_last = true;
      break;
    }
    if ( row_off == rows )
      break;
    this->show_line( state, b, cur_idx, &ls );
  }
  if ( this->show_rows == max_rows ) {
    size_t cnt = this->pgcount( *state.lsb ),
           pg  = cnt - ( this->show_pg + 1 );
    if ( pg < cnt ) {
      size_t off1 = pg * this->show_rows / cnt,
             off2 = ( pg + 1 ) * this->show_rows / cnt;
      for ( ; off1 <= off2; off1++ ) {
        if ( off1 > 0 && off1 < this->show_rows )
          buf[ this->cols - 1 + this->cols * off1 ] = '|';
      }
    }
    buf[ this->cols - 1 ] = ( is_first ? '-' : '^' );
    buf[ this->cols * this->show_rows - 1 ] = ( is_last ? '-' : 'v' );
  }
  return true;
}

bool
State::show_line( ShowState &state,  char32_t *buf,  size_t cur_idx,
                  LineSave **lsptr )
{
  LineSave & ls            = LineSave::line( *state.lsb, state.off );
  bool       is_cur_pos    = ( ls.index == cur_idx );
  size_t     ls_edited_len = ls.edited_len;
  char32_t * ls_line       = &state.lsb->buf[ ls.line_off ];
  size_t     spc           = this->cols - SHOW_PAD;

  if ( lsptr != NULL )
    *lsptr = &ls;
  if ( this->cols <= SHOW_PAD + 1 )
    return false;
  if ( state.has_local_edit ) {
    size_t off = LineSave::scan( this->edit, ls.index );
    if ( off != 0 ) {
      LineSave & local = LineSave::line( this->edit, off );
      ls_edited_len = local.edited_len;
      ls_line       = &this->edit.buf[ local.line_off ];
    }
  }
  if ( ! is_cur_pos )
    *buf = ' ';
  else
    *buf = '*';
  size_t i = 1;
  if ( state.show_index ) {
    size_t d = uint_digits( ls.index );
    if ( d + 1 < spc ) {
      i += uint_to_str( ls.index, &buf[ 1 ], d );
      if ( i + 2 < spc ) {
        buf[ i++ ] = '.';
        buf[ i++ ] = ' ';
      }
    }
  }
  if ( i < spc ) {
    size_t len   = spc - i,
           white = 0,
           off   = 0;
    if ( len > ls_edited_len ) {
      white = len - ls_edited_len;
      len   = ls_edited_len;
    }
    else if ( state.left_overflow ) {
      off = ls_edited_len - len;
    }
    copy<char32_t>( &buf[ i ], &ls_line[ off ], len );
    set<char32_t>( &buf[ i + len ], ' ', white );
    i += len;
  }
  if ( i >= spc ) {
    if ( state.left_overflow )
      buf[ 1 ] = '<';
    else
      buf[ i-1 ] = '>';
  }
  return true;
}

void
State::reset_yank( void )
{
  if ( this->yank.max > 0 ) {
    if ( this->yank.max > 4 * 1024 ) {
      size_t off;
      for ( off = this->yank.first; off < this->yank.max; ) {
        if ( this->yank.max - off <= 4 * 1024 )
          break;
        LineSave &ls = LineSave::line( this->yank, off );
        if ( ls.next_off == 0 )
          break;
        off = ls.next_off;
      }
      if ( off != this->yank.first && off != this->yank.max )
        LineSave::shrink_range( this->yank, off, this->yank.max );
    }
  }
  if ( this->yank.cnt > 0 )
    this->yank.idx = this->yank.cnt + 1;
  else
    this->yank.idx = 0;
}

void
State::add_yank( const char32_t *buf,  size_t size )
{
  if ( size == 0 )
    return;
  if ( ! this->realloc_lsb( this->yank, this->yank.max +
                             LineSave::size( size ) ) )
    return;
  /* check if yank is the same as the last yank */
  if ( LineSave::equals( this->yank, this->yank.off, buf, size ) )
    return;
  this->yank.idx = ++this->yank.cnt + 1;
  this->yank.off = LineSave::make( this->yank, buf, size, 0, this->yank.cnt );
  if ( this->show_mode == SHOW_YANK ) {
    this->show_yank();
    this->output_show();
  }
}

bool
State::get_yank_buf( char32_t *&buf,  size_t &size )
{
  size = 0;
  if ( this->yank.max > 0 ) {
    LineSave &ls = LineSave::line( this->yank, this->yank.max );
    size = ls.edited_len;
    buf  = &this->yank.buf[ ls.line_off ];
    this->yank.idx = ls.index;
    if ( this->show_mode == SHOW_YANK ) {
      this->show_yank();
      this->output_show();
    }
  }
  return size > 0;
}

bool
State::get_yank_pop( char32_t *&buf,  size_t &size )
{
  size_t off = 0;
  if ( this->yank.idx > 0 )
    off = LineSave::find_lt( this->yank, this->yank.max, this->yank.idx );
  if ( off == 0 ) {
    this->yank.idx = this->yank.cnt + 1;
    buf = NULL;
    size = 0;
    return false;
  }
  LineSave &ls = LineSave::line( this->yank, off );
  this->yank.off = off;
  this->yank.idx = ls.index;
  size = ls.edited_len;
  buf  = &this->yank.buf[ ls.line_off ];
  if ( this->show_mode == SHOW_YANK ) {
    this->show_yank();
    this->output_show();
  }
  return size > 0;
}

bool
State::show_yank( void )
{
  this->show_mode = SHOW_YANK;
  this->show_pg = this->pgcount( this->yank ) - 1;
  if ( this->show_save( this->yank.idx, 1 ) )
    return true;
  return false;
}

void
State::reset_marks( void )
{
  size_t j = 0;
  /* remove the marks which are not in history */
  for ( size_t i = 0; i < this->mark_cnt; i++ ) {
    if ( this->mark[ i ].line_idx != 0 ) {
      if ( j < i )
        this->mark[ j ] = this->mark[ i ];
      j++;
    }
  }
  this->mark_cnt = j;
  this->mark_upd = 0;
}

void
State::fix_marks( size_t mark_idx )
{
  size_t j = 0;
  for ( size_t i = this->mark_cnt; i > 0; ) {
    if ( this->mark[ --i ].line_idx == 0 ) {
      /* fix global marks with last line in history */
      if ( this->mark[ i ].mark_c >= 'A' && this->mark[ i ].mark_c <= 'Z' )
        this->mark[ i ].line_idx = mark_idx;
      if ( ++j == this->mark_upd )
        return;
    }
  }
}

void
State::add_mark( size_t mark_off,  size_t mark_idx,  char32_t c )
{
  for ( size_t i = 0; i < this->mark_cnt; i++ ) {
    if ( c == this->mark[ i ].mark_c ) {
      this->mark[ i ].cursor_off = mark_off;
      this->mark[ i ].line_idx   = mark_idx;
      return;
    }
  }
  if ( this->mark_cnt == this->mark_size ) {
    size_t len    = this->mark_size * sizeof( this->mark[ 0 ] ),
           newlen = len + sizeof( this->mark[ 0 ] );
    if ( ! this->do_realloc( &this->mark, len, newlen ) )
      return;
    this->mark_size = newlen / sizeof( this->mark[ 0 ] );
  }
  this->mark[ this->mark_cnt ].cursor_off = mark_off;
  this->mark[ this->mark_cnt ].line_idx   = mark_idx;
  this->mark[ this->mark_cnt ].mark_c     = c;
  this->mark_cnt++;
  this->mark_upd++;
}

bool
State::get_mark( size_t &mark_off,  size_t &mark_idx,  char32_t c )
{
  for ( size_t i = 0; i < this->mark_cnt; i++ ) {
    if ( c == this->mark[ i ].mark_c ) {
      mark_off = this->mark[ i ].cursor_off;
      mark_idx = this->mark[ i ].line_idx;
      return true;
    }
  }
  return false;
}

