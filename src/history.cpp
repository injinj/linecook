#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <linecook/linecook.h>

extern "C"
int
lc_add_history( LineCook *state,  const char *line,  size_t len )
{
  return static_cast<linecook::State *>( state )->add_history( line, len );
}

extern "C"
int
lc_compress_history( LineCook *state )
{
  return static_cast<linecook::State *>( state )->compress_history();
}

extern "C"
size_t
lc_history_count( LineCook *state )
{
  return static_cast<linecook::State *>( state )->history_count();
}

extern "C"
int
lc_history_line_length( LineCook *state,  size_t index )
{
  return static_cast<linecook::State *>( state )->history_line_length( index );
}

extern "C"
int
lc_history_line_copy( LineCook *state,  size_t index,  char *out )
{
  return static_cast<linecook::State *>( state )->
    history_line_copy( index, out );
}

using namespace linecook;

size_t
State::history_count( void ) noexcept
{
  return this->hist.cnt;
}

int
State::history_line_length( size_t index ) noexcept
{
  size_t off = LineSave::find( this->hist, this->hist.off, index );
  if ( off == 0 )
    return -1;
  this->hist.off = off;
  LineSave &ls = LineSave::line( this->hist, off );
  char32_t *buf = &this->hist.buf[ ls.line_off ];
  int size = 0;
  for ( size_t i = 0; i < ls.edited_len; i++ ) {
    if ( buf[ i ] != 0 ) {
      int n = ku_utf32_to_utf8_len( &buf[ i ], 1 );
      if ( n > 0 )
        size += n;
    }
  }
  return size;
}

int
State::history_line_copy( size_t index,  char *out ) noexcept
{
  size_t off = LineSave::find( this->hist, this->hist.off, index );
  if ( off == 0 )
    return -1;
  this->hist.off = off;
  LineSave &ls = LineSave::line( this->hist, off );
  char32_t *buf = &this->hist.buf[ ls.line_off ];
  int size = 0;
  for ( size_t i = 0; i < ls.edited_len; i++ ) {
    if ( buf[ i ] != 0 ) {
      int n = ku_utf32_to_utf8( buf[ i ], &out[ size ] );
      if ( n > 0 )
        size += n;
    }
  }
  return size;
}

void
State::do_search( void ) noexcept
{
  size_t off = 0,
         len = this->edited_len,
         i   = 0;
  int    search_dir = this->search_direction;
  bool   do_next;

  if ( this->action == ACTION_SEARCH_NEXT_REV )
    search_dir *= -1;
  if ( this->action != ACTION_HISTORY_COMPLETE )
    i = 1;
  if ( len > i ) {
    if ( ! this->realloc_search( len - i ) )
      return;
    this->search_len = len - i;
    copy<char32_t>( this->search_buf, &this->line[ i ], this->search_len );
    this->hist.idx = 0; /* restart the search, new term */
  }
  if ( this->hist.idx == 0 || this->hist.idx > this->hist.cnt ) {
    if ( search_dir >= 0 )
      this->hist.off = this->hist.max;
    else
      this->hist.off = this->hist.first;
    do_next = false;
  }
  else {
    do_next = true;
  }
  for (;;) {
    size_t old_idx = this->hist.idx,
           old_off = this->hist.off;
    if ( do_next )
      this->hist.off = LineSave::find( this->hist, this->hist.off,
                                       this->hist.idx - search_dir );
    if ( this->search_len > 0 )
      off = LineSave::find_substr( this->hist, this->hist.off,
                                   this->search_buf, this->search_len,
                                   search_dir );
    if ( off == 0 ) {
      this->cancel_search();
      this->bell();
      return;
    }
    else {
      if ( old_idx != 0 && old_off != 0 &&
           LineSave::compare( this->hist, old_off, off ) == 0 ) {
        this->hist.off = off;
        this->hist.idx = LineSave::line( this->hist, off ).index;
        do_next = true;
      }
      else {
        this->show_search( off );
        return;
      }
    }
  }
}

void
State::show_search( size_t off ) noexcept
{
  LineSave *lsu;
  LineSave &ls = LineSave::line( this->hist, off );
  this->hist.idx = ls.index;
  this->hist.off = off;
  if ( this->save_action == ACTION_SEARCH_INLINE &&
       (lsu = this->peek_undo()) != NULL ) {
    this->extend( ls.edited_len + lsu->edited_len );
    this->restore_insert( this->hist, ls, this->undo, *lsu );
  }
  else {
    this->display_history_line( &ls );
  }
  if ( this->show_mode == SHOW_HISTORY ) {
    this->show_history_index();
    this->output_show();
  }
  this->clear_search_mode();
}

bool
State::display_history_index( size_t idx ) noexcept
{
  size_t off = LineSave::find( this->hist, this->hist.max, idx );
  if ( off == 0 )
    return false;
  LineSave &ls = LineSave::line( this->hist, off );
  this->display_history_line( &ls );
  this->hist.off = off;
  this->hist.idx = ls.index;
  return true;
}

void
State::display_history_line( LineSave *ls ) noexcept
{
  LineSaveBuf *lsb = &this->hist;
  if ( ls != NULL ) {        /* check if hist line was edited */
    LineSave *local = this->find_edit( ls->index );
    if ( local != NULL ) {
      ls  = local;
      lsb = &this->edit;
    }
  }
  else {                     /* check if this->line was edited */
    ls  = this->find_edit( 0 );
    lsb = &this->edit;
  }
  if ( ls != NULL ) {        /* output the history entry */
    this->extend( ls->edited_len );
    this->restore_save( *lsb, *ls );
  }
  else { /* clear the line, no history */
    if ( this->edited_len != 0 ) {
      this->edited_len = 0;
      this->move_cursor( this->prompt.cols );
      this->cursor_erase_eol();
    }
  }
}

void
State::cancel_search( void ) noexcept
{
  LineSave *ls;

  this->edited_len = 0;
  this->move_cursor( this->prompt.cols );
  this->cursor_erase_eol();
  if ( (ls = this->pop_undo()) != NULL ) {
    this->extend( ls->edited_len );
    this->restore_save( this->undo, *ls );
  }
  this->clear_search_mode();
}

bool
State::show_history_next_page( void ) noexcept
{
  if ( this->show_pg > 0 )
    this->show_pg--;
  return this->show_lsb( SHOW_HISTORY, this->hist );
}

bool
State::show_history_prev_page( void ) noexcept
{
  if ( this->show_pg < this->pgcount( this->hist ) - 1 )
    this->show_pg++;
  return this->show_lsb( SHOW_HISTORY, this->hist );
}

bool
State::show_history_start( void ) noexcept
{
  this->show_pg = this->pgcount( this->hist ) - 1;
  return this->show_lsb( SHOW_HISTORY, this->hist );
}

bool
State::show_history_end( void ) noexcept
{
  this->show_pg = 0;
  return this->show_lsb( SHOW_HISTORY, this->hist );
}

bool
State::show_history_index( void ) noexcept
{
  if ( this->hist.idx == 0 || this->hist.idx >= this->hist.cnt )
    return this->show_history_end();
  this->show_pg = this->pgnum( this->hist );
  return this->show_lsb( SHOW_HISTORY, this->hist );
}

void
State::save_hist_edit( size_t idx ) noexcept
{
  size_t eoff, hoff;
  /* check if edit exists and is the same as current edit */
  eoff = LineSave::scan( this->edit, idx );
  /* check if edit is the same as the hist entry */
  if ( idx != 0 ) {
    hoff = LineSave::find( this->hist, this->hist.off, idx );
    if ( hoff != 0 ) {
      if ( LineSave::equals( this->hist, hoff, this->line,
                             this->edited_len ) ) {
        if ( eoff != 0 ) /* remove */
          LineSave::resize( this->edit, eoff, 0 );
        return;
      }
    }
  }
  if ( eoff != 0 ) {
    if ( LineSave::equals( this->edit, eoff, this->line, this->edited_len ) ) {
      LineSave &ls = LineSave::line( this->edit, eoff );
      ls.cursor_off = this->cursor_pos - this->prompt.cols; /* save cursor */
      return;
    }
  }
  if ( ! this->realloc_lsb( this->edit, this->edit.max +
                             LineSave::size( this->edited_len ) ) )
    return;
  if ( eoff != 0 ) {
  /* resize edit buffer to contain new line, if it exsists */
    LineSave::resize( this->edit, eoff, this->edited_len );
    LineSave &ls = LineSave::line( this->edit, eoff );
    copy<char32_t>( &this->edit.buf[ ls.line_off ], this->line,
                    this->edited_len );
    ls.edited_len = this->edited_len;
    ls.cursor_off = this->cursor_pos - this->prompt.cols; /* save cursor */
  }
  /* create a new edit buffer */
  else {
    LineSave::make( this->edit, this->line, this->edited_len,
                    this->cursor_pos - this->prompt.cols, idx );
  }
}

LineSave *
State::find_edit( size_t idx ) noexcept
{
  size_t off = LineSave::scan( this->edit, idx );
  if ( off != 0 )
    return &LineSave::line( this->edit, off );
  return NULL;
}

int
State::add_history( const char *buf,  size_t len ) noexcept
{
  if ( ! this->make_utf32( buf, len, this->cvt, this->cvt_len ) )
    return this->error;
  this->push_history( this->cvt, this->cvt_len );
  this->hist.off = this->hist.max;
  return 0;
}

int
State::compress_history( void ) noexcept
{
  this->error = 0;
  if ( ! LineSave::shrink_unique( this->hist ) )
    this->error = LINE_STATUS_ALLOC_FAIL;
  return this->error;
}

bool
State::get_hist_arg( char32_t *&buf,  size_t &size,  bool nth ) noexcept
{
  size_t old_idx = this->hist.idx;
  if ( this->hist.max == 0 )
    return false;
  if ( this->hist.idx == 0 ) {
    if ( this->hist.cnt != 0 )
      this->hist.idx = this->hist.cnt;
  }
  else if ( this->hist.idx > 0 ) {
    if ( ! nth )
      this->hist.idx -= 1;
  }
  this->hist.off = LineSave::find( this->hist, this->hist.off,
                                   this->hist.idx );
  if ( this->hist.off == 0 )
    return false;
  if ( ! nth )
    size = 0;

  LineSave & ls    = LineSave::line( this->hist, this->hist.off );
  size_t     off   = ( size == 0 || size >= ls.edited_len ? ls.edited_len :
                       ls.edited_len - ( size + 1 ) );
  char32_t * start = &this->hist.buf[ ls.line_off ],
           * p     = &start[ off ],
           * end   = &start[ ls.edited_len ];
  bool       b     = false;
  for ( ; ; p-- ) {
    if ( p == start )
      b = true;
    else {
      if ( p[ -1 ] == 0 )
        p--;
      else if ( isspace( p[ -1 ] ) )
        b = true;
    }
    if ( b ) {
      if ( p == end )
        b = false;
      else {
        buf  = p;
        size = end - p;
        b    = true;
      }
      break;
    }
  }
  /* show the page that the cursor is on */
  if ( this->show_mode == SHOW_HISTORY ) {
    if ( old_idx != this->hist.idx ) {
      if ( ! this->show_update( old_idx, this->hist.idx ) ) {
        if ( this->hist.idx != 0 ) {
          this->show_history_index();
          this->output_show();
        }
      }
    }
  }
  return b;
}

void
State::push_history( const char32_t *buf,  size_t len ) noexcept
{
  while ( len > 0 && isspace( buf[ len - 1 ] ) )
    --len;
  if ( ! this->realloc_lsb( this->hist, this->hist.max + LineSave::size( len )))
    return;
  /* check if line is the same as the last history entry */
  if ( LineSave::equals( this->hist, this->hist.max, buf, len ) )
    return;
  this->hist.cnt++;
  LineSave::make( this->hist, buf, len, 0, this->hist.cnt );
  if ( this->mark_upd != 0 )
    this->fix_marks( this->hist.cnt );
}

LineSave *
State::history_older( void ) noexcept
{
  size_t old_idx = this->hist.idx;

  if ( this->show_mode == SHOW_HISTORY ) {
    /* if hist.idx is off screen or not set, move it onto the screen */
    if ( old_idx < this->show_start || old_idx > this->show_end )
      this->hist.idx = this->show_end + 1;
  }
  if ( this->hist.idx == 0 ) {
    if ( this->hist.cnt != 0 )
      this->hist.idx = this->hist.cnt;
  }
  else {
    this->hist.idx -= 1;
  }
  return this->history_move( old_idx );
}

LineSave *
State::history_newer( void ) noexcept
{
  size_t old_idx = this->hist.idx;

  if ( this->show_mode == SHOW_HISTORY ) {
    /* if hist.idx is off screen or not set, move it onto the screen */
    if ( old_idx < this->show_start || old_idx > this->show_end )
      this->hist.idx = this->show_start - 1;
  }
  if ( ++this->hist.idx > this->hist.cnt ) {
    this->hist.idx = 0; /* clears the input line */
  }
  else {
    if ( this->hist.idx == 1 ) /* offset is the first */
      this->hist.off = this->hist.first;
  }
  return this->history_move( old_idx );
}

LineSave *
State::history_top( void ) noexcept
{
  if ( this->show_mode == SHOW_HISTORY ) {
    size_t old_idx = this->hist.idx;
    this->hist.idx = this->show_start;
    return this->history_move( old_idx );
  }
  return this->history_newer();
}

LineSave *
State::history_middle( void ) noexcept
{
  size_t old_idx = this->hist.idx;
  if ( this->show_mode == SHOW_HISTORY )
    this->hist.idx = ( this->show_start + this->show_end ) / 2;
  else
    this->hist.idx = this->hist.cnt / 2;
  return this->history_move( old_idx );
}

LineSave *
State::history_bottom( void ) noexcept
{
  if ( this->show_mode == SHOW_HISTORY ) {
    size_t old_idx = this->hist.idx;
    this->hist.idx = this->show_end;
    return this->history_move( old_idx );
  }
  return this->history_older();
}

LineSave *
State::history_move( size_t old_idx ) noexcept
{
  LineSave * hist = NULL;

  if ( this->hist.idx != 0 ) {
    this->hist.off = LineSave::find( this->hist, this->hist.off,
                                     this->hist.idx );
    hist = &LineSave::line( this->hist, this->hist.off );
  }
  if ( hist != NULL || old_idx != 0 )
    this->save_hist_edit( old_idx );
  /* show the page that the cursor is on */
  if ( this->show_mode == SHOW_HISTORY ) {
    if ( old_idx != this->hist.idx ) {
      if ( ! this->show_update( old_idx, this->hist.idx ) ) {
        if ( this->hist.idx != 0 ) {
          this->show_history_index();
          this->output_show();
        }
      }
    }
  }
  return hist;
}

