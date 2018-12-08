#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <linecook/linecook.h>

extern "C"
int
lc_add_completion( LineCook *state,  int ctype,  const char *line,  size_t len )
{
  return static_cast<linecook::State *>( state )->
    add_completion( ctype, line, len );
}

using namespace linecook;

size_t
State::quote_line_length( const char32_t *buf,  size_t len )
{
  if ( ! this->complete_has_quote ) {
    size_t extra = 2;
    bool   needs_quotes = false;
    if ( this->complete_type != 'v' ) {
      for ( size_t i = 0; i < len; i++ ) {
        if ( this->is_quote_char( buf[ i ] ) ) {
          needs_quotes = true;
          if ( char32_eq( this->quote_char, buf[ i ] ) || buf[ i ] == '\\' )
            extra++;
        }
      }
    }
    if ( needs_quotes )
      len += extra;
  }
  return len;
}

void
State::quote_line_copy( char32_t *out,  const char32_t *buf,  size_t len )
{
  size_t i, j = 0;
  out[ j++ ] = this->quote_char;
  for ( i = 0; i < len; i++ ) {
    if ( char32_eq( this->quote_char, buf[ i ] ) || buf[ i ] == '\\' )
      out[ j++ ] = '\\';
    out[ j++ ] = buf[ i ];
  }
  out[ j++ ] = this->quote_char;
}

bool
State::tab_complete( int ctype,  bool reverse )
{
  if ( this->show_mode != SHOW_COMPLETION ) {
    if ( reverse && ctype == 0 )
      ctype = 's';
    return this->tab_first_completion( ctype );
  }
  return this->tab_next_completion( ctype, reverse );
}

void
State::completion8( size_t off,  size_t len,  int ctype )
{
  char buf[ 4 * 1024 ], * p = buf;
  size_t i = 0, j = 0, off8 = 0, len8 = 0;
  if ( this->edited_len > 1024 ) {
    if ( (p = (char *) ::malloc( this->edited_len * 4 )) == NULL )
      return;
  }
  for (;;) {
    if ( i >= off ) {
      if ( i == off )
        off8 = j;
      else if ( i == off + len )
        len8 = j - off8;
    }
    if ( i == this->edited_len )
      break;
    int n = ku_utf32_to_utf8( this->line[ i++ ], &p[ j ] );
    if ( n <= 0 )
      goto failed;
    j += (size_t) n;
  }
  this->completion( p, off8, len8, ctype );

failed:
  if ( p != buf )
    ::free( p );
}

void
State::reset_complete( void )
{
  this->complete_off = 0;
  this->complete_len = 0;
  this->complete_tab = 0;
  this->complete_type = 0;
  this->complete_has_quote = false;
}

bool
State::tab_first_completion( int ctype )
{
  const char32_t * pattern;
  size_t patlen,
         off  = this->cursor_pos - this->prompt.cols, /* offset into line[] */
         coff,
         cend,
         caft,
         replace_len,
         quote_len,
         start_off   = 0,
         match_len   = 0,
         match_cnt   = 0;
  bool   found_match = false;

  this->reset_complete();
  if ( off < this->edited_len &&
       this->line[ off ] == (uint8_t) this->quote_char ) {
    for ( coff = off; ; coff-- ) {
      if ( coff == 0 )
        break;
      if ( this->line[ coff - 1 ] == (uint8_t) this->quote_char ) {
        cend = off;
        goto matched_quotes_fwd;
      }
    }
  }
  if ( off > 1 && this->line[ off - 1 ] == (uint8_t) this->quote_char ) {
    for ( coff = off - 1; ; coff-- ) {
      if ( coff == 0 )
        break;
      if ( this->line[ coff - 1 ] == (uint8_t) this->quote_char )
        goto matched_quotes_back;
    }
  }
  coff = this->skip_prev_word( off, this->plete_brk );
  cend = this->skip_next_word( off, this->plete_brk );
  if ( 0 ) {
matched_quotes_back:;
    this->move_cursor_back( 1 );
    cend = --off;
matched_quotes_fwd:;
    this->complete_has_quote = true;
  }
  caft = this->edited_len - cend;
  replace_len = cend - coff;

  LineSave::reset( this->comp ); /* reset any completions */
  this->completion8( coff, replace_len, ctype );
  if ( ctype == 's' ) {
    if ( replace_len > 0 ) {
      size_t i = coff + replace_len;
      for (;;) {
        if ( this->line[ i - 1 ] == '/' )
          break;
        if ( --i == coff )
          break;
      }
      pattern = &this->line[ i ];
      patlen  = replace_len - ( i - coff );
      for ( i = 0; i < patlen; i++ ) {
        if ( pattern[ i ] == '*' || pattern[ i ] == '?' || pattern[ i ] == '[' )
          break;
      }
      if ( i < patlen )
        LineSave::filter_glob( this->comp, pattern, patlen );
      else
        LineSave::filter_substr( this->comp, pattern, patlen );
    }
    LineSave::sort( this->comp );
    this->comp.off = this->comp.first;
    if ( this->comp.cnt == 1 ) {
      const LineSave &ls = LineSave::line_const( this->comp, this->comp.first );
      start_off = this->comp.first;
      match_len = ls.edited_len;
      match_cnt = 1;
    }
  }
  else {
    size_t i;
    pattern = &this->line[ coff ];
    patlen  = replace_len;
    for ( i = 0; i < patlen; i++ ) {
      if ( pattern[ i ] == '*' || pattern[ i ] == '?' || pattern[ i ] == '[' )
        break;
    }
    if ( i < patlen )
      LineSave::filter_glob( this->comp, pattern, patlen );
    LineSave::sort( this->comp );
    if ( i < patlen ) {
      this->comp.off = this->comp.first;
      if ( this->comp.cnt == 1 ) {
        const LineSave &ls = LineSave::line_const( this->comp,
                                                   this->comp.first );
        start_off = this->comp.first;
        match_len = ls.edited_len;
        match_cnt = 1;
      }
    }
    else {
      /* pull out the lines that match the prefix */
      start_off = LineSave::find_prefix( this->comp, this->comp.first,
                                         &this->line[ coff ], replace_len,
                                         match_len, match_cnt );
      if ( start_off != 0 ) {
        if ( match_cnt > 1 ) {
          LineSave &first = LineSave::line( this->comp, start_off );
          /* set the bounds of the prefix match */
          size_t to_off = LineSave::find( this->comp, start_off,
                                          first.index + match_cnt - 1 );
          LineSave::shrink_range( this->comp, start_off, to_off );
          this->comp.off = this->comp.first;
        }
        else {
          this->comp.off = start_off;
        }
      }
    }
  }
  /* start_off is the completion index that the first prefix matches */
  if ( start_off != 0 ) {
    LineSave &ls = LineSave::line( this->comp, this->comp.off );
    /* if a match is longer than the prefix string being replaced */
    if ( match_len >= replace_len ) {
      const char32_t *substitute = &this->comp.buf[ ls.line_off ];
      this->move_cursor_back( off - coff );
      if ( match_len == ls.edited_len && match_cnt == 1 ) {
        found_match = true;
      }
      /* if line needs quoting, quote_len will be longer than match_len */
      quote_len = this->quote_line_length( substitute, ls.edited_len );
      if ( replace_len < quote_len ) {
        if ( ! this->realloc_line( this->edited_len + quote_len - replace_len ) )
          return false;
      }
      /* update the edit line with the match */
      if ( found_match ) {
        move<char32_t>( &this->line[ coff + quote_len ], &this->line[ cend ],
                        caft );
        if ( quote_len > match_len )
          this->quote_line_copy( &this->line[ coff ], substitute,
                                 ls.edited_len );
        else
          copy<char32_t>( &this->line[ coff ], substitute, match_len );
        match_len = quote_len;
      }
      else {
        move<char32_t>( &this->line[ coff + match_len ], &this->line[ cend ],
                        caft );
        copy<char32_t>( &this->line[ coff ], substitute, match_len );
      }
      this->extend( coff + match_len + caft );
      /* update the screen */
      this->cursor_output( &this->line[ coff ], match_len + caft );
      if ( replace_len > match_len )
        this->erase_eol_with_right_prompt();
      this->move_cursor_back( caft );
    }
  }
  /* if not found, a list of items can be displayed, remember the position
   * of the completion prefix */
  if ( ! found_match ) {
    this->complete_off = coff;
    if ( (this->complete_len = match_len) < replace_len )
      this->complete_len = replace_len;
  }
  else { /* otherwise no completion match */
    this->reset_complete();
  }
  return found_match;
}

bool
State::tab_next_completion( int /*ctype*/,  bool reverse )
{
  size_t old_idx = this->comp.idx;
  size_t off;
  /* cycle through the matches */
  if ( this->complete_tab == 0 ) {
    if ( (this->complete_tab = this->comp.idx) == 0 ) {
      this->complete_tab = this->comp.idx = 1;
    }
  }
  else {
    if ( reverse ) {
      if ( this->comp.idx-- == this->complete_tab )
        this->comp.idx = this->complete_tab + this->comp.cnt - 1;
    }
    else {
      if ( ++this->comp.idx >= this->complete_tab + this->comp.cnt )
        this->comp.idx = this->complete_tab;
    }
  }
  do {
    off = LineSave::find( this->comp, this->comp.off, this->comp.idx );
    if ( off == 0 ) {
      if ( this->comp.idx == this->complete_tab )
        return false;
      this->comp.idx = this->complete_tab;
    }
  } while ( off == 0 );
  /* update the line displayed with the current replacement */
  LineSave       & ls         = LineSave::line( this->comp, off );
  const char32_t * substitute = &this->comp.buf[ ls.line_off ];
  size_t           match_len  = ls.edited_len,
                   coff       = this->complete_off,
                   cend       = coff + this->complete_len,
                   caft       = this->edited_len - cend;

  if ( ! this->realloc_line( this->edited_len + match_len -
                             this->complete_len ) )
    return false;

  move<char32_t>( &this->line[ coff + match_len ], &this->line[ cend ], caft );
  copy<char32_t>( &this->line[ coff ], substitute, match_len );
  this->extend( coff + match_len + caft );
  this->move_cursor( coff + this->prompt.cols );
  this->cursor_output( &this->line[ coff ], match_len + caft );
  if ( this->complete_len > match_len )
    this->erase_eol_with_right_prompt();
  this->move_cursor_back( caft );
  this->complete_len = match_len;
  if ( ! this->show_update( old_idx, ls.index ) ) {
    this->show_completion_index();
    this->output_show();
  }
  return false;
}

void
State::completion_accept( void )
{
  /* update line and quote it if necessary, then clear the matches */
  size_t off = LineSave::find( this->comp, this->comp.off, this->comp.idx );
  if ( off != 0 ) {
    LineSave       & ls         = LineSave::line( this->comp, off );
    const char32_t * substitute = &this->comp.buf[ ls.line_off ];
    size_t           match_len  = ls.edited_len,
                     quote_len  = this->quote_line_length(substitute,match_len),
                     coff       = this->complete_off,
                     cend       = coff + this->complete_len,
                     caft       = this->edited_len - cend;

    this->move_cursor( coff + this->prompt.cols );
    if ( this->complete_len < quote_len ) {
      if ( ! this->realloc_line( this->edited_len + quote_len -
                                 this->complete_len ) )
        return;
    }
    move<char32_t>( &this->line[ coff + quote_len ], &this->line[ cend ], caft);
    if ( quote_len > match_len )
      this->quote_line_copy( &this->line[ coff ], substitute, ls.edited_len );
    else
      copy<char32_t>( &this->line[ coff ], substitute, ls.edited_len );
    this->extend( coff + quote_len + caft );
    this->cursor_output( &this->line[ coff ], quote_len + caft );
    this->move_cursor_back( caft ); /* cursor at end of replacement */
    if ( this->complete_len > quote_len )
      this->erase_eol_with_right_prompt();
  }
  this->show_clear();
  this->reset_complete();
}

void
State::completion_update( int delta )
{
  /* a new char was added to the prefix, match it */
  if ( this->complete_off + this->complete_len + delta <= this->edited_len ) {
    char32_t * buf = &this->line[ this->complete_off ];
    size_t len = this->complete_len + delta;
    size_t match_len = 0,
           match_cnt = 0,
           off;
    off = LineSave::find_prefix( this->comp, this->comp.off, buf, len,
                                 match_len, match_cnt );
    if ( off == 0 || match_len < len ) {
      off = LineSave::find( this->comp, this->comp.off, this->comp.idx );
      if ( off != 0 ) {
        LineSave &ls = LineSave::line( this->comp, off );
        if ( ls.edited_len == this->complete_len ) {
          this->completion_accept();
          if ( match_len < len )
            this->move_cursor_fwd( len - match_len );
          return;
        }
      }
    }
    else if ( match_len >= len ) {
      LineSave &ls = LineSave::line( this->comp, off );
      size_t old_idx = this->comp.idx;
      this->comp.idx = ls.index;
      this->comp.off = off;
      this->complete_len = len;
      if ( match_cnt == 1 ) {
        this->completion_accept();
        return;
      }
      else if ( match_len > len ) {
        const char32_t * substitute = &this->comp.buf[ ls.line_off ];
        size_t add_len              = match_len - len,
               coff                 = this->complete_off,
               cend                 = coff + this->complete_len,
               caft                 = this->edited_len - cend;
        if ( ! this->realloc_line( this->edited_len + add_len ) )
          return;
        this->complete_len = match_len;
        this->extend( cend + add_len + caft );
        move<char32_t>( &this->line[ cend + add_len ], &this->line[ cend ],
                        caft );
        copy<char32_t>( &this->line[ cend ], &substitute[ len ], add_len );
        this->cursor_output( &this->line[ cend ], add_len + caft );
        this->move_cursor_back( caft );
      }
      if ( old_idx != ls.index && this->show_mode == SHOW_COMPLETION ) {
        if ( ! this->show_update( old_idx, ls.index ) ) {
          this->show_completion_index();
          this->output_show();
        }
      }
      return;
    }
  }
  this->show_clear();
  this->complete_off = 0;
  this->complete_len = 0;
  this->complete_tab = 0;
  this->complete_type = 0;
}

bool
State::completion_prev( void )
{
  size_t old_idx = this->comp.idx;
  bool   found;
  if ( old_idx > this->show_end )
    this->comp.idx = this->show_end;
  else if ( old_idx > 1 )
    this->comp.idx = old_idx - 1;
  found = this->show_update( old_idx, this->comp.idx );
  if ( ! found && this->comp.idx != 0 ) {
    this->show_completion_prev_page();
    this->output_show();
  }
  return true;
}

bool
State::completion_next( void )
{
  size_t old_idx = this->comp.idx;
  bool   found;
  if ( old_idx < this->show_start )
    this->comp.idx = this->show_start;
  else if ( old_idx < this->comp.cnt )
    this->comp.idx = old_idx + 1;
  found = this->show_update( old_idx, this->comp.idx );
  if ( ! found && this->comp.idx != 0 ) {
    this->show_completion_next_page();
    this->output_show();
  }
  return true;
}

int
State::add_completion( int ctype,  const char *buf,  size_t len )
{
  if ( ! this->make_utf32( buf, len, this->cvt, this->cvt_len ) )
    return this->error;
  this->push_completion( this->cvt, this->cvt_len );
  this->comp.off = this->comp.max;
  this->complete_type = ctype;
  return 0;
}

void
State::push_completion( const char32_t *buf,  size_t len )
{
  if ( ! this->realloc_lsb( this->comp, this->comp.max + LineSave::size( len )))
    return;
  LineSave::make( this->comp, buf, len, 0, ++this->comp.cnt );
}
#if 0
LineSave *
State::match_completion( const char *buf,  size_t len,  size_t &match_len,
                         size_t &match_cnt,  CompletionType t )
{
  size_t off, old_idx;

  if ( this->show_mode != SHOW_COMPLETION ) {
    LineSave::reset( this->comp ); /* reset any completions */
    this->completion( buf, len, t );
    LineSave::sort( this->comp );
  }
  off = LineSave::find_prefix( this->comp, this->comp.first, buf, len,
                               match_len, match_cnt, true );
  if ( off == 0 )
    return NULL;

  LineSave &ls = LineSave::line( this->comp, off );
  old_idx = this->comp.idx;
  this->comp.idx = ls.index;
  this->comp.off = off;
  if ( old_idx != ls.index && this->show_mode == SHOW_COMPLETION ) {
    if ( ! this->show_update( old_idx, ls.index ) ) {
      this->show_completion_index();
      this->output_show();
    }
  }
  return &ls;
}
#endif
bool
State::show_completion_index( void )
{
  this->show_mode = SHOW_COMPLETION;
  this->show_pg = this->pgnum( this->comp );
  if ( this->show_save( this->comp.idx, this->comp.idx ) )
    return true;
  this->show_mode = SHOW_NONE;
  return false;
}

bool
State::show_completion_prev_page( void )
{
  if ( this->show_pg < this->pgcount( this->comp ) - 1 )
    this->show_pg++;
  return this->show_lsb( SHOW_COMPLETION, this->comp );
}

bool
State::show_completion_next_page( void )
{
  if ( this->show_pg > 0 )
    this->show_pg--;
  return this->show_lsb( SHOW_COMPLETION, this->comp );
}
