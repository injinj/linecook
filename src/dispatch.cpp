#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <linecook/linecook.h>
#include <linecook/xwcwidth9.h>

using namespace linecook;

int
State::dispatch( void )
{
  LineSave    * ls;
  LineSaveBuf * lsb;
  size_t        off = this->cursor_pos - this->prompt.cols; /* off of line[] */
  char32_t      c   = this->in.cur_char;
  int           w, x;
  bool          is_interrupt = false,
                is_search,
                need_undo;
  /* process tab completions, it should be it's own mode */
  if ( this->show_mode == SHOW_COMPLETION ) {
    switch ( this->action ) {
      case ACTION_BELL:
      case ACTION_PENDING:
      case ACTION_PUTBACK:
      case ACTION_INTERRUPT:
      case ACTION_SUSPEND:
      case ACTION_OPER_AND_NEXT: /* ctrl-o XXX ?? */
      case ACTION_FINISH_LINE:
      case ACTION_TAB_COMPLETE:
      case ACTION_TAB_REVERSE:
      case ACTION_VI_REPEAT_DIGIT:
      case ACTION_EMACS_ARG_DIGIT:
      case ACTION_INSERT_CHAR:
        break; /* continue action */

      case ACTION_PREV_ITEM:     /* arrow up key cause prev selection */
        this->completion_prev();
        return LINE_STATUS_OK;

      case ACTION_NEXT_ITEM:     /* arrow down key */
        this->completion_next();
        return LINE_STATUS_OK;

      case ACTION_SHOW_NEXT_PAGE:/* pgdn function causes show window to page*/
        this->show_completion_next_page();
        this->output_show();
        return LINE_STATUS_OK;

      case ACTION_SHOW_PREV_PAGE:/* pgup function */
        this->show_completion_prev_page();
        this->output_show();
        return LINE_STATUS_OK;

      case ACTION_GOTO_FIRST_ENTRY:
        this->completion_start();
        return LINE_STATUS_OK;

      case ACTION_GOTO_LAST_ENTRY:
        this->completion_end();
        return LINE_STATUS_OK;

      case ACTION_GOTO_TOP:
        this->completion_top();
        return LINE_STATUS_OK;

      case ACTION_GOTO_BOTTOM:
        this->completion_bottom();
        return LINE_STATUS_OK;
        
      case ACTION_GOTO_ENTRY:
      case ACTION_GOTO_MIDDLE:
        return LINE_STATUS_OK;

      default:
        this->show_clear();
        break; /* continue action */

      case ACTION_GO_RIGHT: /* cursor forward to end of completion */
        this->completion_accept();
        return LINE_STATUS_OK;
    }
  }
  if ( this->last_action == ACTION_VI_REPEAT_DIGIT && c == '0' )
    this->action = ACTION_VI_REPEAT_DIGIT;
  /* switch whether action needs undo push, when starting a search an undo
   * is pushed, but while a search is being entered, no undos are pushed */
  switch ( this->action ) {
    case ACTION_INSERT_CHAR:
    case ACTION_BELL:
    case ACTION_PENDING:
    case ACTION_PUTBACK:
      break;

    default: { /* may need undo */
      is_search = false;
      if ( this->in.cur_recipe != NULL ) {
        need_undo = ( lc_action_options( this->action ) & OPT_UNDO ) != 0;
        if ( ! need_undo &&
             ( this->is_emacs_mode() || this->is_vi_insert_mode() ) ) {
          if ( this->action != ACTION_INSERT_CHAR &&
               this->last_action == ACTION_INSERT_CHAR )
            need_undo = true;
        }
        if ( need_undo )
          need_undo = ! this->is_search_mode();
        if ( need_undo ) {
          if ( 0 ) {
    case ACTION_SEARCH_HISTORY:  /* '/'    start a search always does undo */
    case ACTION_SEARCH_NEXT:     /* 'n' */
    case ACTION_SEARCH_NEXT_REV: /* 'N' */
    case ACTION_SEARCH_REVERSE:  /* '?' */
    case ACTION_SEARCH_INLINE:   /* ctrl-s */
    case ACTION_HISTORY_COMPLETE:/* alt-p */
            if ( ( this->last_action == ACTION_HISTORY_COMPLETE ||
                   this->last_action == ACTION_SEARCH_NEXT ) &&
                 this->action == ACTION_HISTORY_COMPLETE )
               this->action = ACTION_SEARCH_NEXT;
            is_search = true;
          }
          /* skip undo if in the middle of a search */
          if ( ! is_search || this->hist.idx == 0 ) {
            this->push_undo();
            if ( this->show_mode == SHOW_UNDO ) {
              this->show_undo();
              this->output_show();
            }
          }
          /* erase the existing line, start insert mode with '/' */
          if ( is_search ) {
            if ( this->edited_len > 0 ) {
              if ( this->action != ACTION_HISTORY_COMPLETE ) {
                this->edited_len = 0;
                this->move_cursor_back( off );
                off = 0;
                this->cursor_erase_eol();
              }
            }
            /* if use search history twice, go to next */
            if ( this->action == ACTION_SEARCH_HISTORY ||
                 this->action == ACTION_SEARCH_REVERSE ) {
              if ( this->is_search_mode() ) {
                if ( this->action == ACTION_SEARCH_HISTORY )
                  this->action = ACTION_SEARCH_NEXT;
                else
                  this->action = ACTION_SEARCH_NEXT_REV;
              }
            }
            if ( this->action != ACTION_SEARCH_NEXT &&
                 this->action != ACTION_SEARCH_NEXT_REV ) {
              if ( this->action != ACTION_HISTORY_COMPLETE ) {
                if ( this->action == ACTION_SEARCH_HISTORY ||
                     this->action == ACTION_SEARCH_INLINE )
                  c = '/';
                else if ( this->action == ACTION_SEARCH_REVERSE )
                  c = '?';
                /* after search term is entered, search is performed */
                this->save_action = this->action;
                this->action = ACTION_INSERT_CHAR;
                this->search_direction = ( c == '?' ? -1 : 1 );
              }
              else {
                this->search_direction = 1;
              }
              this->set_search_mode();
            }
            /* otherwise, continue searching next match */
          }
        }
      }
      break; /* continue into main switch action */
    }
  }
  switch ( this->action ) {
    case ACTION_INSERT_CHAR: /* insert char at cursor */
    case ACTION_VI_REPLACE_CHAR: /* 'r' in vi mode */
      if ( (w = wcwidth9( c )) > 0 ) {
        size_t size;
        if ( ! this->realloc_line( this->edited_len + w ) )
          return LINE_STATUS_ALLOC_FAIL;
        /* replace or append */
        if ( this->is_replace_mode() ||
             this->action == ACTION_VI_REPLACE_CHAR ) {
          x = this->char_width_next( off );
          if ( w == x ) { /* if same width */
            this->line[ off ] = c;
            this->cursor_output( &this->line[ off ], w );
            x = ( this->action == ACTION_VI_REPLACE_CHAR ? w : 0 );
          }
          else { /* replace char larger or smaller than current char */
            size = this->edited_len - ( off + x );
            move<char32_t>( &this->line[ off + w ], &this->line[ off + x ],
                            size );
            this->line[ off ] = c;
            if ( w == 2 )
              this->line[ off + 1 ] = 0;
            this->extend( this->edited_len + w - x );
            size += w;
            this->cursor_output( &this->line[ off ], size );
            if ( w < x )
              this->cursor_erase_eol();
            x = size - ( this->action == ACTION_VI_REPLACE_CHAR ? 0 : w );
          }
          /* replace char moves back to char, replace mode after char */
          if ( x != 0 )
            this->move_cursor_back( x );
        }
        /* if at the end */
        else if ( off >= this->edited_len ) {
          this->extend( off + w );
          this->line[ off ] = c;
          if ( w > 1 )
            this->line[ off + 1 ] = 0;
          this->cursor_output( &this->line[ off ], w );
        }
        else { /* in the middle */
          size = this->edited_len - off;
          move<char32_t>( &this->line[ off + w ], &this->line[ off ], size );
          this->line[ off ] = c;
          if ( w > 1 )
            this->line[ off + 1 ] = 0;
          this->extend( this->edited_len + w );
          size += w;
          this->cursor_output( &this->line[ off ], size );
          this->move_cursor_back( size - w );
        }
        if ( this->show_mode == SHOW_COMPLETION )
          this->completion_update( w );
      }
      break;

    case ACTION_SUSPEND: /* ctrl-z */
    case ACTION_OPER_AND_NEXT: /* ctrl-o */
      if ( this->show_mode != SHOW_NONE ) /* clear show buffer */
        this->show_clear();
      if ( this->is_visual_mode() ) {
        this->toggle_visual_mode();
        this->refresh_visual_line();
      }
      if ( this->action == ACTION_SUSPEND ) {
        /* send cursor to the next line, which may be multiple rows */
        this->output_right_prompt( true ); /* clear right prompt */
        this->output_newline( ( this->edited_len - off ) / this->cols + 1 );
        this->refresh_needed = true; /* refresh line after called again */
        return LINE_STATUS_SUSPEND;
      }
      if ( this->is_vi_command_mode() )
        this->set_vi_insert_mode();
      this->reset_completions();
      this->init_completion_term();
      this->refresh_needed = true; /* refresh line after called again */
      return LINE_STATUS_COMPLETE;

    case ACTION_INTERRUPT: /* ctrl-c */
      is_interrupt = true; /* same as finish, reset state */
      /* FALLTHRU */
    case ACTION_FINISH_LINE: /* enter key pressed */
      if ( ! is_interrupt && this->is_search_mode() ) {
    case ACTION_SEARCH_HISTORY: /* enter causes search to execute */
    case ACTION_SEARCH_NEXT:
    case ACTION_SEARCH_NEXT_REV:
    case ACTION_SEARCH_REVERSE:
    case ACTION_SEARCH_INLINE:
    case ACTION_HISTORY_COMPLETE:
        this->do_search();
        break;
      }
      if ( this->show_mode != SHOW_NONE ) /* clear show buffer */
        this->show_clear();
      if ( this->is_visual_mode() ) {
        this->toggle_visual_mode();
        this->refresh_visual_line();
      }
      if ( ! this->realloc_line( this->edited_len + 1 ) ) /* need null byte */
        return LINE_STATUS_ALLOC_FAIL;
      this->line[ this->edited_len ] = '\0'; /* the result of finish */
      this->line_len = this->edited_len;
      this->output_right_prompt( true ); /* clear right prompt */
      if ( is_interrupt )
        this->output_str( "^C", 2 );  /* show the interrupt */
      this->reset_state();               /* reset state */
      /* send cursor to the next line, which may be multiple rows */
      this->output_newline( ( this->line_len - off ) / this->cols + 1 );
      if ( is_interrupt )
        return LINE_STATUS_INTERRUPT;
      return LINE_STATUS_EXEC;

    case ACTION_CANCEL_SEARCH:
      this->cancel_search();
      break;

    case ACTION_GOTO_BOL: /* go to beginning of line */
      if ( off != 0 )
        this->move_cursor( this->prompt.cols );
      break;

    case ACTION_BACKSPACE: /* erase previous char, if any */
      if ( off > 0 ) {
        w = this->char_width_back( off );
        off -= w;
        this->move_cursor_back( w );
        /* if backing out of a search */
        if ( off == 0 && this->is_search_mode() ) {
          this->cancel_search();
          break;
        }
        /* FALLTHRU */
    case ACTION_VI_CHANGE_CHAR:
    case ACTION_DELETE_CHAR:
        if ( off < this->edited_len ) { /* erase next char, if present */
          /* if delete char from middle */
          if ( off + 1 < this->edited_len ) {
            w = this->char_width_next( off );
            size_t size = this->edited_len - ( off + w );
            /*if ( this->action != ACTION_BACKSPACE )
              this->add_yank( &this->line[ off ], w );*/
            move<char32_t>( &this->line[ off ], &this->line[ off + w ], size );
            this->edited_len -= w;
            this->cursor_output( &this->line[ off ], size );
            this->erase_eol_with_right_prompt();
            this->move_cursor_back( size );
          }
          else { /* at the last char, erase it */
            w = this->char_width_back( this->edited_len );
            this->edited_len -= w;
            /*this->add_yank( &this->line[ this->edited_len ], w );*/
            this->erase_eol_with_right_prompt();
          }
        }
      }
      if ( this->action == ACTION_VI_CHANGE_CHAR )
        this->set_vi_insert_mode();
      break;

    case ACTION_GOTO_EOL: { /* cursor to eol */
      size_t add = 0;
      if ( ( this->in.mode & VI_COMMAND_MODE ) == 0 ) /* to last char */
        add = this->edited_len - off;
      else if ( off + 1 < this->edited_len ) /* to insert pos */
        add = this->edited_len - ( off + 1 );
      if ( add > 0 )
        this->move_cursor_fwd( add );
      break;
    }
    case ACTION_GO_LEFT: /* cursor back one */
      if ( this->cursor_pos > this->prompt.cols ) {
        w = this->char_width_back( off );
        this->move_cursor_back( w );
      }
      break;
    case ACTION_GO_RIGHT: /* cursor forward one */
      if ( off < this->edited_len ) {
        w = this->char_width_next( off );
        this->move_cursor_fwd( w );
      }
      break;
    case ACTION_GOTO_PREV_WORD: /* postion cursor at previous word (emacs) */
      this->move_cursor_back( off - this->prev_word( off ) );
      break;
    case ACTION_VI_GOTO_PREV_WORD: /* postion cursor at previous word (vi) */
      this->move_cursor_back( off - this->prev_word_start( off ) );
      break;
    case ACTION_GOTO_NEXT_WORD: /* pos cursor at space after next word (emacs)*/
      this->move_cursor_fwd( this->next_word( off ) - off );
      break;
    case ACTION_VI_GOTO_NEXT_WORD: /* position cursor at next word (vi) */
      this->move_cursor_fwd( this->next_word_start( off ) - off );
      break;
    case ACTION_GOTO_ENDOF_WORD: /* postion cursor at end of cur word */
      this->move_cursor_fwd( this->next_word_end( off ) - off );
      break;

    case ACTION_LOWERCASE_WORD: /* emacs meta-l, meta-u, meta-c */
    case ACTION_UPPERCASE_WORD:
    case ACTION_CAPITALIZE_WORD: {
      size_t word = this->skip_next_space( off ),
             spc  = this->skip_next_bword( word );
      if ( word < this->edited_len ) {
        if ( this->action == ACTION_CAPITALIZE_WORD ||
             this->action == ACTION_UPPERCASE_WORD ) {
          this->line[ word ] = upcase<char32_t>( this->line[ word ] );
          if ( this->action == ACTION_UPPERCASE_WORD ) {
            while ( ++word < spc )
              this->line[ word ] = upcase<char32_t>( this->line[ word ] );
          }
        }
        else {
          this->line[ word ] = locase<char32_t>( this->line[ word ] );
          while ( ++word < spc )
            this->line[ word ] = locase<char32_t>( this->line[ word ] );
        }
        this->cursor_output( &this->line[ off ], spc - off );
      }
      else
        this->bell();
      break;
    }

    case ACTION_TRANSPOSE: /* flip 2 chars, ctrl-t in emacs mode */
    case ACTION_TRANSPOSE_WORDS: { /* flip 2 words, meta-t in emacs */
      size_t b_start, a_start, a_end, b_end;
      b_start = off;
      if ( this->action == ACTION_TRANSPOSE_WORDS ) {
        if ( b_start > 0 && ! this->is_spc_char( this->line[ b_start - 1 ] ) )
          b_start = this->prev_word( off );
        a_start = this->prev_word( b_start );
        a_end   = this->next_word( a_start );
        b_end   = this->next_word( b_start );
      }
      else {
        b_end = b_start;
        if ( b_start > 0 )
          b_start -= this->char_width_back( off );
        a_end = b_start;
        a_start = a_end;
        if ( a_start > 0 )
          a_start -= this->char_width_back( a_end );
      }
      if ( a_start < a_end && b_start < b_end && a_start < b_start &&
           a_end <= b_start ) {
        size_t len   = b_end - a_start,
               a_len = a_end - a_start,
               b_len = b_end - b_start;
        this->move_cursor( a_start + this->prompt.cols );
        /* rotate until b is in front of a */
        rotate( &this->line[ a_start ], len, len - a_len );
        rotate( &this->line[ a_start ], len - a_len, b_len );
        this->cursor_output( &this->line[ a_start ], b_end - a_start );
      }
      else
        this->bell();
      break;
    }
    case ACTION_ERASE_BOL: /* erase from cursor to start */
      if ( off > 0 ) {
        this->add_yank( this->line, off );
        this->edited_len -= off;
        if ( this->edited_len > 0 ) {
          move<char32_t>( this->line, &this->line[ off ], this->edited_len );
          this->move_cursor( this->prompt.cols );
          this->cursor_output( this->line, this->edited_len );
          this->cursor_erase_eol();
          this->move_cursor( this->prompt.cols );
        }
        else {
          this->move_cursor( this->prompt.cols );
          this->cursor_erase_eol();
        }
      }
      break;

    case ACTION_ERASE_EOL: /* erase from cursor to end */
      if ( off != this->edited_len ) {
        this->add_yank( &this->line[ off ], this->edited_len - off );
        this->edited_len = off;
        this->cursor_erase_eol();
        if ( this->is_vi_command_mode() && off > 0 )
          this->move_cursor_back( this->char_width_back( off ) );
      }
      break;

    case ACTION_VI_CHANGE_LINE: /* 'C' or 'cc' in vi mode */
    case ACTION_ERASE_LINE: /* erase all of line */
      if ( this->edited_len != 0 ) {
        this->add_yank( this->line, this->edited_len );
        this->edited_len = 0;
        this->move_cursor( this->prompt.cols );
        this->cursor_erase_eol();
      }
      if ( this->action == ACTION_VI_CHANGE_LINE )
        this->set_vi_insert_mode();
      break;

    case ACTION_ERASE_PREV_WORD: { /* kill previous space and word */
      size_t start = this->prev_word( off );
      if ( start != off ) {
        this->add_yank( &this->line[ start ], off - start );
        move<char32_t>( &this->line[ start ], &this->line[ off ],
                        this->edited_len - off );
        this->edited_len -= ( off - start );
        this->move_cursor_back( off - start );
        size_t save = this->cursor_pos;
        this->cursor_output( &this->line[ start ], this->edited_len - start );
        this->cursor_erase_eol();
        this->move_cursor( save );
      }
      else
        this->bell();
      break;
    }
    case ACTION_VISUAL_CHANGE:  /* visual select + change */
    case ACTION_VISUAL_DELETE:  /* visual select + delete */
    case ACTION_VI_CHANGE_WORD: /* 'cw', change next word */
    case ACTION_VI_CHANGE_BOL:  /* 'c0', change to bol */
    case ACTION_VI_CHANGE_EOL:  /* 'c$', change to eol */
    case ACTION_ERASE_NEXT_WORD:   /* meta-d, kill word */
    case ACTION_VI_ERASE_NEXT_WORD: { /* 'dw', kill word and space */
      size_t end = 0, start;
      if ( this->action == ACTION_VISUAL_CHANGE ||
           this->action == ACTION_VISUAL_DELETE ) {
        this->visual_bounds( off, start, end );
      }
      else {
        start = off;
        if ( this->action == ACTION_ERASE_NEXT_WORD )
          end = this->next_word( off );
        else if ( this->action == ACTION_VI_ERASE_NEXT_WORD )
          end = this->next_word_start( off );
        else if ( this->action == ACTION_VI_CHANGE_EOL )
          end = this->edited_len;
        else if ( this->action == ACTION_VI_CHANGE_BOL ) {
          end   = off;
          start = 0;
        }
      }
      if ( end != start ) {
        this->add_yank( &this->line[ start ], end - start );
        move<char32_t>( &this->line[ start ], &this->line[ end ],
                        this->edited_len - end );
        this->edited_len -= ( end - start );
        if ( start != off )
          this->move_cursor( this->prompt.cols + start );
        this->cursor_output( &this->line[ start ], this->edited_len - start );
        this->cursor_erase_eol();
        this->move_cursor( this->prompt.cols + start );
      }
      if ( this->action == ACTION_VI_CHANGE_WORD ||
           this->action == ACTION_VISUAL_CHANGE ||
           this->action == ACTION_VI_CHANGE_BOL ||
           this->action == ACTION_VI_CHANGE_EOL )
        this->set_vi_insert_mode();
      else if ( this->action == ACTION_VISUAL_DELETE )
        this->toggle_visual_mode();
      break;
    }
    case ACTION_FLIP_CASE: /* '~' in vi cmd mode */
      if ( off < this->edited_len ) {
        if ( this->char_width_next( off ) == 1 ) {
          char32_t c = this->line[ off ];
          if ( isupcase<char32_t>( c ) )
            c = locase<char32_t>( c );
          else if ( islocase<char32_t>( c ) )
            c = upcase<char32_t>( c );
          this->line[ off ] = c;
          this->cursor_output( c );
        }
        else { /* no case with double wide chars */
          this->move_cursor_fwd( 2 );
        }
      }
      break;

    case ACTION_MATCH_PAREN: /* '%' in vi cmd mode */
      if ( off < this->edited_len ) {
        size_t pos = this->match_paren( off );
        if ( pos != off ) {
          this->move_cursor( this->prompt.cols + pos );
          break;
        }
      }
      this->bell();
      break;
    case ACTION_INCR_NUMBER: /* ctrl-a and ctrl-x in vi cmd mode */
    case ACTION_DECR_NUMBER: {
      this->incr_decr( this->action == ACTION_INCR_NUMBER ? 1 : -1 );
      break;
    }
    case ACTION_REPEAT_FIND_NEXT: /* ';', ',' in vi, repeat find */
      if ( (this->action = this->last_rep_next_act) == 0 )
        break;
      c = this->last_rep_next_char;
      if ( 0 ) {
        /* FALLTHRU */
    case ACTION_REPEAT_FIND_PREV:
        if ( (this->action = this->last_rep_prev_act) == 0 )
          break;
        c = this->last_rep_prev_char;
      }
      /* FALLTHRU */
    case ACTION_FIND_NEXT_CHAR: /* 'f' in vi, find char entered */
    case ACTION_FIND_PREV_CHAR:
    case ACTION_TILL_NEXT_CHAR: /* 't' in vi, find pos before char entered */
    case ACTION_TILL_PREV_CHAR: {
      size_t i = 0;
      bool   found = false;
      if ( this->action == ACTION_FIND_NEXT_CHAR ||
           this->action == ACTION_TILL_NEXT_CHAR ) {
        this->last_rep_next_act  = this->action;
        this->last_rep_next_char = c;
        for ( i = off + 1; i < this->edited_len; i++ ) {
          if ( this->line[ i ] == c ) {
            found = true;
            if ( this->action == ACTION_TILL_NEXT_CHAR )
              i -= this->char_width_back( i );
            break;
          }
        }
      }
      else {
        this->last_rep_prev_act  = this->action;
        this->last_rep_prev_char = c;
        if ( off > 0 ) { /* find prev char */
          for ( i = off; ; ) {
            if ( this->line[ --i ] == c ) {
              found = true;
              if ( this->action == ACTION_TILL_PREV_CHAR )
                i += this->char_width_next( i );
              break;
            }
            if ( i == 0 )
              break;
          }
        }
      }
      if ( found )
        this->move_cursor( this->prompt.cols + i );
      else
        this->bell();
      break;
    }
    case ACTION_REPLACE_MODE: /* replace mode causes chars to overwrite */
      this->toggle_replace_mode();
      break;

    case ACTION_REVERT: /* 'U', revert all edits */
    case ACTION_UNDO: /* 'u' (vi), pop the last undo pushed */
    case ACTION_REDO: /* ctrl-r, push the last undo popped (redo it) */
      if ( this->action == ACTION_UNDO )
        ls = this->pop_undo();
      else if ( this->action == ACTION_REDO )
        ls = this->push_redo();
      else
        ls = this->revert_undo();
      if ( this->show_mode == SHOW_UNDO ) {
        this->show_undo();
        this->output_show();
      }
      if ( ls != NULL ) {
        this->extend( ls->edited_len );
        this->restore_save( this->undo, *ls );
      }
      else if ( this->undo.idx == 0 && this->edited_len > 0 ) {
        this->edited_len = 0;
        this->move_cursor( this->prompt.cols );
        this->cursor_erase_eol();
      }
      else
        this->bell();
      break;

    case ACTION_VI_MODE: /* switch from emacs to vi or back to emacs */
      if ( this->is_emacs_mode() )
        this->set_vi_insert_mode();
      else
        this->set_emacs_mode();
      break;

    case ACTION_VI_ESC: /* escape char in vi mode */
      if ( this->is_visual_mode() ) { /* if visual selection, stop that */
        this->toggle_visual_mode();
        this->refresh_visual_line();
      }
      else if ( this->is_search_mode() ) { /* if searching, stop that */
        this->cancel_search();       /* repaints line without search */
      }
#if 0
      else if ( this->show_mode != SHOW_NONE &&
                this->show_mode != SHOW_HISTORY ) {
        this->show_clear();          /* hides show window */
      }
#endif
      else if ( this->is_vi_mode() ) {
        if ( this->is_vi_insert_mode() ) {
          if ( off > 0 ) {
            w = this->char_width_back( off );
            this->move_cursor_back( w );
          }
        }
#if 0
        else if ( this->show_mode == SHOW_HISTORY ) /* esc * 2 clears hist */
          this->show_clear();
#endif
        this->set_vi_command_mode(); /* clears replace mode */
      }
      else if ( this->is_emacs_mode() ) {
        this->set_emacs_mode();      /* clears replace mode */
      }
      break;

    case ACTION_QUOTED_INSERT: /* ctrl-q XXX */
      break;
    /* generic insert/append, either vi or emacs */
    case ACTION_INSERT:
      this->set_any_insert_mode();
      break;
    case ACTION_INSERT_BOL:
      this->set_any_insert_mode();
      if ( off > 0 )
        this->move_cursor_back( off );
      break;
    case ACTION_APPEND:
      this->set_any_insert_mode();
      if ( off < this->edited_len )
        this->move_cursor_fwd( 1 );
      break;
    case ACTION_APPEND_EOL:
      this->set_any_insert_mode();
      if ( off < this->edited_len )
        this->move_cursor_fwd( this->edited_len - off );
      break;
    /* vi insert/append */
    case ACTION_VI_INSERT: /* 'i' in vi mode */
      this->set_vi_insert_mode();
      break;
    case ACTION_VI_REPLACE: /* 'R' in vi mode */
      this->set_vi_replace_mode();
      break;
    case ACTION_VI_APPEND: /* 'a' in vi mode */
      this->set_vi_insert_mode();
      if ( off < this->edited_len )
        this->move_cursor_fwd( 1 );
      break;
    case ACTION_VI_APPEND_EOL: /* 'A' in vi mode */
      this->set_vi_insert_mode();
      if ( off < this->edited_len )
        this->move_cursor_fwd( this->edited_len - off );
      break;
    case ACTION_VI_INSERT_BOL: /* 'I' in vi mode */
      this->set_vi_insert_mode();
      if ( off > 0 )
        this->move_cursor_back( off );
      break;

    case ACTION_VI_MARK:      /* 'm' in vi mode */
      this->add_mark( off, this->hist.idx, c );
      break;

    case ACTION_VI_GOTO_MARK: { /* '`' in vi mode XXX */
      size_t mark_off, mark_idx, curs_off, ho = 0;
      if ( this->get_mark( mark_off, mark_idx, c ) &&
           ( mark_idx == 0 ||
             (ho = LineSave::find( this->hist, this->hist.max,
                                   mark_idx )) != 0 ) ) {
        if ( ho != 0 ) {
          this->hist.idx = mark_idx;
          ls = this->find_edit( mark_idx );
          if ( ls != NULL ) {
            lsb = &this->edit;
          }
          else {
            ls  = &LineSave::line( this->hist, ho );
            lsb = &this->hist;
          }
          this->extend( ls->edited_len );
          this->restore_save( *lsb, *ls );
          curs_off = this->cursor_pos - this->prompt.cols;
        }
        else {
          curs_off = off;
        }
        if ( mark_off <= curs_off )
          this->move_cursor_back( curs_off - mark_off );
        else {
          if ( mark_off > this->edited_len )
            mark_off = this->edited_len;
          this->move_cursor_fwd( mark_off - curs_off );
        }
      }
      else {
        this->bell();
      }
      break;
    }
    case ACTION_VISUAL_MARK:  /* 'v' in vi mode */
      if ( this->is_visual_mode() ) {
        this->toggle_visual_mode();
        this->refresh_visual_line();
      }
      else {
        this->visual_off = off;
        this->toggle_visual_mode();
      }
      break;

    case ACTION_VI_REPEAT_DIGIT: /* XXX : display repeat counter? */
      if ( c >= '0' && c <= '9' )
        this->vi_repeat_cnt = this->vi_repeat_cnt * 10 + ( c - '0' );
      break; /* this is handled in lineio.cpp, since '0' causes '^' action */
    case ACTION_EMACS_ARG_DIGIT:
      if ( c >= '0' && c <= '9' )
        this->emacs_arg_cnt = this->emacs_arg_cnt * 10 + ( c - '0' );
      else if ( c == '-' )
        this->emacs_arg_neg = -1;
      break;

    case ACTION_YANK_LAST_ARG: /* paste contents of history arg */
    case ACTION_YANK_NTH_ARG:
    case ACTION_YANK_POP:
    case ACTION_PASTE_APPEND:  /* paste contents of yank buffer */
    case ACTION_PASTE_INSERT: {
      char32_t * buf  = NULL;
      size_t     size = 0;
      bool       b    = false;
      if ( this->action == ACTION_PASTE_APPEND ||
           this->action == ACTION_PASTE_INSERT )
        b = this->get_yank_buf( buf, size );
      else {
        size_t subs_start = 0,
               subs_size  = 0,
               trail      = 0;
        /* repeating the yank options will substitute previous yank/paste ops */
        if ( this->last_action == ACTION_YANK_LAST_ARG ||
             this->last_action == ACTION_YANK_NTH_ARG ||
             this->last_action == ACTION_YANK_POP ||
             this->last_action == ACTION_PASTE_APPEND ||
             this->last_action == ACTION_PASTE_INSERT ) {
          subs_start = this->last_yank_start;
          subs_size  = this->last_yank_size;
          this->last_yank_start = 0;
          this->last_yank_size  = 0;
          if ( subs_size > 0 &&
               subs_start + subs_size <= this->edited_len &&
               off == subs_start + subs_size ) {
            trail = this->edited_len - ( subs_start + subs_size );
            this->move_cursor_back( subs_size );
            move<char32_t>( &this->line[ subs_start ],
                            &this->line[ subs_start + subs_size ], trail );
            this->extend( subs_start + trail );
            off -= subs_size;
          }
          else {
            subs_start = 0;
            subs_size  = 0;
          }
        }
        size = subs_size;
        if ( this->action == ACTION_YANK_POP )
          b = this->get_yank_pop( buf, size );
        else 
          b = this->get_hist_arg( buf, size,
                                  this->action == ACTION_YANK_NTH_ARG );
        if ( ! b && subs_size > 0 ) {
          this->cursor_output( &this->line[ subs_start ], trail );
          this->cursor_erase_eol();
          this->move_cursor_back( trail );
        }
      }
      if ( b ) {
        size_t start = off;
        if ( ! this->realloc_line( size + this->edited_len ) )
          return LINE_STATUS_ALLOC_FAIL;
        if ( this->action == ACTION_PASTE_APPEND &&
             start < this->edited_len ) {
          w = this->char_width_next( start );
          start += w;
          this->move_cursor_fwd( w );
        }
        if ( start < this->edited_len )
          move<char32_t>( &this->line[ start + size ], &this->line[ start ],
                          this->edited_len - start );
        copy<char32_t>( &this->line[ start ], buf, size );
        this->last_yank_start = start;
        this->last_yank_size  = size;
        this->extend( this->edited_len + size );
        size_t save = this->cursor_pos;
        this->cursor_output( &this->line[ start ], this->edited_len - start );
        this->cursor_erase_eol();
        this->move_cursor( save + size );
      }
      else
        this->bell();
      break;
    }
    case ACTION_VISUAL_YANK:
    case ACTION_YANK_BOL:
    case ACTION_YANK_EOL:
    case ACTION_YANK_LINE:
    case ACTION_YANK_WORD: {
      size_t start, size;
      if ( this->action == ACTION_VISUAL_YANK ) {
        size_t end;
        this->visual_bounds( off, start, end );
        size = end - start;
      }
      else {
        start = off,
        size  = this->edited_len;
        if ( this->action == ACTION_YANK_EOL )
          size -= off;
        else if ( this->action == ACTION_YANK_WORD )
          size = this->next_word( off ) - off;
        else if ( this->action == ACTION_YANK_LINE )
          start = 0;
        else if ( this->action == ACTION_YANK_BOL ) {
          start = 0;
          size  = off;
        }
      }
      this->add_yank( &this->line[ start ], size );
      if ( this->action == ACTION_VISUAL_YANK ) {
        this->move_cursor( start + this->prompt.cols );
        this->toggle_visual_mode();
        this->refresh_visual_line();
      }
      break;
    }

    case ACTION_TAB_COMPLETE:
    case ACTION_TAB_REVERSE:
      if ( this->show_mode == SHOW_HISTORY ) { /* tab accept history */
        size_t add = 0;
        if ( off < this->edited_len ) /* to insert pos */
          add = this->edited_len - off;
        if ( add > 0 )
          this->move_cursor_fwd( add );
        this->show_clear();
        if ( this->is_vi_command_mode() )
          this->set_vi_insert_mode();
        break;
      }
      /* FALLTHRU */
    case ACTION_SHOW_DIRS:
    case ACTION_SHOW_EXES:
    case ACTION_SHOW_FILES:
    case ACTION_SHOW_VARS:
    case ACTION_SHOW_TREE:      
    case ACTION_SHOW_FZF:       

      if ( this->show_mode != SHOW_COMPLETION ) {
        this->reset_completions();
        switch ( this->action ) {
          default:
          case ACTION_TAB_COMPLETE: this->complete_type = COMPLETE_ANY;   break;
          case ACTION_TAB_REVERSE:  this->complete_type = COMPLETE_SCAN;  break;
          case ACTION_SHOW_DIRS:    this->complete_type = COMPLETE_DIRS;  break;
          case ACTION_SHOW_EXES:    this->complete_type = COMPLETE_EXES;  break;
          case ACTION_SHOW_FILES:   this->complete_type = COMPLETE_FILES; break;
          case ACTION_SHOW_VARS:    this->complete_type = COMPLETE_ENV;   break;
          case ACTION_SHOW_TREE:    this->complete_type = COMPLETE_SCAN;  break;
          case ACTION_SHOW_FZF:     this->complete_type = COMPLETE_FZF;   break;
        }
        /* complete a term under the cursor */
        if ( this->is_vi_command_mode() )
          this->set_vi_insert_mode();
        this->init_completion_term();
        this->refresh_needed = true; /* refresh line after called again */
        return LINE_STATUS_COMPLETE;
      }
      if ( this->tab_complete( this->action == ACTION_TAB_REVERSE ) ) {
        if ( this->show_mode == SHOW_COMPLETION )
          this->show_clear();
      }
      else /*if ( this->last_action == ACTION_TAB_COMPLETE )*/ {
        if ( this->show_mode != SHOW_COMPLETION ) {
          if ( this->comp.cnt > 0 ) {
            if ( this->show_mode != SHOW_NONE )
              this->show_clear();
            this->show_completion_index();
            this->output_show();
          }
          else
            this->bell();
        }
      }
      break;

    case ACTION_GOTO_TOP:
    case ACTION_GOTO_MIDDLE:
    case ACTION_GOTO_BOTTOM:
    case ACTION_PREV_ITEM:       /* arrow up key cause history selection */
    case ACTION_NEXT_ITEM:       /* arrow down key */
      switch ( this->action ) {
        case ACTION_PREV_ITEM:   ls  = this->history_older();  break;
        case ACTION_NEXT_ITEM:   ls  = this->history_newer();  break;
        case ACTION_GOTO_TOP:    ls  = this->history_top();    break;
        case ACTION_GOTO_MIDDLE: ls  = this->history_middle(); break;
        case ACTION_GOTO_BOTTOM: ls  = this->history_bottom(); break;
        default: ls = NULL; break;
      }
      this->display_history_line( ls );
      break;

    case ACTION_SHOW_UNDO:   /* display show window with undo buffer */
      if ( this->show_mode != SHOW_NONE )
        this->show_clear();
      this->show_undo();
      this->output_show();
      break;
    case ACTION_SHOW_YANK:   /* display show window with yank buffer */
      if ( this->show_mode != SHOW_NONE )
        this->show_clear();
      this->show_yank();
      this->output_show();
      break;
    case ACTION_SHOW_HISTORY:/* display show window with history buffer */
      if ( this->show_mode != SHOW_NONE )
        this->show_clear();
      this->show_history_index();
      this->output_show();
      break;
    case ACTION_SHOW_KEYS:   /* display show window with key events */
      if ( this->show_mode != SHOW_NONE )
        this->show_clear();
      this->show_keys();
      this->output_show();
      break;
    case ACTION_SHOW_CLEAR:  /* hide show window */
      this->show_clear();
      break;
    case ACTION_GOTO_ENTRY:
      if ( this->vi_repeat_cnt != 0 ) {
        if ( this->display_history_index( this->vi_repeat_cnt ) ) {
          if ( this->show_mode != SHOW_NONE )
            this->show_clear();
          this->show_history_index();
          this->output_show();
        }
        else
          this->bell();
        this->vi_repeat_cnt = 0;
        break;
      }
      /* FALLTHRU */
    case ACTION_SHOW_NEXT_PAGE:/* pgdn function causes show window to page */
    case ACTION_SHOW_PREV_PAGE:/* pgup function */
    case ACTION_GOTO_FIRST_ENTRY:
    case ACTION_GOTO_LAST_ENTRY:
      if ( this->show_mode == SHOW_KEYS ) {
        switch ( this->action ) {
          case ACTION_GOTO_FIRST_ENTRY:
            this->show_keys_start();
            break;
          case ACTION_SHOW_NEXT_PAGE:
            this->show_keys_next_page();
            break;
          case ACTION_SHOW_PREV_PAGE:
            this->show_keys_prev_page();
            break;
          case ACTION_GOTO_ENTRY:
          case ACTION_GOTO_LAST_ENTRY:
            this->show_keys_end();
            break;
          default: break;
        }
      }
      else {
        if ( this->show_mode != SHOW_HISTORY ) {
          size_t index = 0;
          if ( this->show_mode != SHOW_NONE )
            this->show_clear();
          switch ( this->action ) {
            case ACTION_GOTO_FIRST_ENTRY:
            case ACTION_SHOW_NEXT_PAGE:
              this->show_history_start();
              index = this->show_start;
              break;
            case ACTION_GOTO_ENTRY:
            case ACTION_GOTO_LAST_ENTRY:
            case ACTION_SHOW_PREV_PAGE:
              this->show_history_end();
              index = this->show_end;
              break;
            default: break;
          }
          if ( this->hist.idx == 0 && this->edited_len == 0 ) {
            this->display_history_index( index );
            this->show_history_index();
          }
        }
        else {
          switch ( this->action ) {
            case ACTION_SHOW_NEXT_PAGE:   this->show_history_next_page(); break;
            case ACTION_SHOW_PREV_PAGE:   this->show_history_prev_page(); break;
            case ACTION_GOTO_FIRST_ENTRY: this->show_history_start();     break;
            case ACTION_GOTO_ENTRY:
            case ACTION_GOTO_LAST_ENTRY:  this->show_history_end();       break;
            default: break;
          }
        }
      }
      this->output_show();
      break;
    case ACTION_REDRAW_LINE: /* ctrl-l redraws */
      this->refresh_line();
      break;
    case ACTION_BELL:        /* an unbound key event or one bound to bell */
      this->bell();
      break;
    case ACTION_PENDING:     /* when needs input, but nothing available */
    case ACTION_PUTBACK:     /* doesn't occur here, only in input */
    case ACTION_REPEAT_LAST: /* handled before switch */
      break;
    case ACTION_DECR_SHOW:   /* handled outside dispatch */
    case ACTION_INCR_SHOW:
    case ACTION_MACRO:
    case ACTION_ACTION:
      break;
  }
  return LINE_STATUS_OK;
}

