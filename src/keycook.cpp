#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <linecook/linecook.h>

using namespace linecook;

int
State::set_recipe( KeyRecipe *key_rec,  size_t key_rec_count )
{
  /* state vars which index into this->recipe[] */
  this->cur_recipe         = NULL;
  this->last_repeat_recipe = NULL;
  this->vi_repeat_default  = NULL;
  this->cur_input          = NULL;

  /* initialize the recipe for key sequence translations */
  this->recipe      = key_rec;
  this->recipe_size = key_rec_count;

  this->init_single_key_transitions( this->emacs, EMACS_MODE );
  this->init_single_key_transitions( this->vi_insert, VI_INSERT_MODE );
  this->init_single_key_transitions( this->vi_command, VI_COMMAND_MODE );
  this->init_single_key_transitions( this->visual, VISUAL_MODE );
  this->init_single_key_transitions( this->search, SEARCH_MODE );

  /* vi repeat action are the digits 1 - 9, [ 0 -> 9 ] */
  for ( size_t i = 0; i < this->recipe_size; i++ ) {
    if ( this->recipe[ i ].action == ACTION_VI_REPEAT_DIGIT ) {
      this->vi_repeat_default = &this->recipe[ i ];
      break;
    }
  }
  /* set up multi char sequences */
  KeyRecipe ** mc = this->multichar;
  size_t       sz = 0;

  sz += this->emacs.mc_size;
  sz += this->vi_insert.mc_size;
  sz += this->vi_command.mc_size;
  sz += this->visual.mc_size;
  sz += this->search.mc_size;

  mc = (KeyRecipe **) ::realloc( mc, sizeof( mc[ 0 ] ) * sz );
  if ( mc == NULL )
    return -1;

  this->multichar      = mc;
  this->multichar_size = sz;

  this->emacs.mc      = mc; mc += this->emacs.mc_size;
  this->vi_insert.mc  = mc; mc += this->vi_insert.mc_size;
  this->vi_command.mc = mc; mc += this->vi_command.mc_size;
  this->visual.mc     = mc; mc += this->visual.mc_size;
  this->search.mc     = mc; mc += this->search.mc_size;

  this->init_multi_key_transitions( this->emacs, EMACS_MODE );
  this->init_multi_key_transitions( this->vi_insert, VI_INSERT_MODE );
  this->init_multi_key_transitions( this->vi_command, VI_COMMAND_MODE );
  this->init_multi_key_transitions( this->visual, VISUAL_MODE );
  this->init_multi_key_transitions( this->search, SEARCH_MODE );

  return 0;
}

void
State::init_single_key_transitions( LineKeyMode &km,  uint8_t mode )
{
  size_t i;
  /* find the default transition */
  km.def = 0;
  for ( i = 0; i < this->recipe_size; i++ ) {
    if ( this->recipe[ i ].char_sequence == NULL ) {
      if ( ( this->recipe[ i ].valid_mode & mode ) != 0 ) {
        km.def = i;
        break;
      }
    }
  }
  /* clear all transitions to the default recipe */
  for ( i = 0; i < sizeof( km.recipe ); i++ )
    km.recipe[ i ] = km.def;

  /* set up single char sequences */
  km.mc_size = 0;
  for ( i = 0; i < this->recipe_size; i++ ) {
    if ( this->recipe[ i ].char_sequence != NULL ) {
      uint8_t  c = this->recipe[ i ].char_sequence[ 0 ],
              c2 = this->recipe[ i ].char_sequence[ 1 ];

      if ( ( this->recipe[ i ].valid_mode & mode ) != 0 ) {
        if ( km.recipe[ c ] == km.def )
          km.recipe[ c ] = i; /* transition for char -> this->recipe[ i ] */
        if ( c2 != 0 ) /* if is a multichar sequence */
          km.mc_size++;
      }
    }
  }
}

void
State::init_multi_key_transitions( LineKeyMode &km,  uint8_t mode )
{
  size_t i, j = 0;
  for ( i = 0; i < this->recipe_size; i++ )
    if ( this->recipe[ i ].char_sequence != NULL )
      if ( this->recipe[ i ].char_sequence[ 1 ] != 0 )
        if ( ( this->recipe[ i ].valid_mode & mode ) != 0 )
          km.mc[ j++ ] = &this->recipe[ i ];
}

void
State::layout_keys( const char *key,  const char *action,  const char *mode,
                    const char *descr )
{
  /* col
   * 0              16                 35     42
   * Key            Action             Mode
   * ---            ------             ----
   * ctrl-a         goto_bol           ICSV   Description
   */
  char32_t line[ 80 ];
  size_t   i, len;
  for ( i = 0; i < 80; i++ )
    line[ i ] = ' ';
  len = ::strlen( key );
  for ( i = 0; i < len; i++ )
    line[ i ] = key[ i ];
  len = ::strlen( action );
  for ( i = 0; i < len; i++ )
    line[ 17 + i ] = action[ i ];
  len = ::strlen( mode );
  for ( i = 0; i < len; i++ )
    line[ 36 + i ] = mode[ i ];
  if ( descr == NULL )
    len += 36;
  else {
    len = ::strlen( descr );
    for ( i = 0; i < len; i++ )
      line[ 42 + i ] = descr[ i ];
    len += 42;
  }
  LineSave::make( this->keys, line, len, 0, ++this->keys.cnt );
}

bool
State::show_keys( void )
{
  if ( this->keys.cnt == 0 ) {
    char         name[ 40 ],
                 mode[ 40 ];
    const char * action,
               * descr;
    size_t       k;

    if ( this->keys.first == 0 ) {
      if ( ! this->realloc_lsb( this->keys, LineSave::size( 80 ) *
                                ( this->recipe_size + 2 ) ) )
        return false;
      this->layout_keys( "Key", "Action",
                         "Mode: E:emacs I:vi C:cmd S:Srch V:Visu", NULL );
      this->layout_keys( "---", "------", "----", NULL );
      for ( size_t i = 0; i < this->recipe_size; i++ ) {
        KeyRecipe &r = this->recipe[ i ];
        if ( r.char_sequence == NULL )
          ::strcpy( name, "(other key)" );
        else
          lc_key_to_name( r.char_sequence, name );
        action = lc_action_to_name( r.action );
        descr  = lc_action_to_descr( r.action );
        k = 0;
        if ( ( r.valid_mode & EMACS_MODE ) != 0 ) {
          ::strcpy( &mode[ k ], "E" ); k++;
        }
        if ( ( r.valid_mode & VI_INSERT_MODE ) != 0 ) {
          ::strcpy( &mode[ k ], "I" ); k++;
        }
        if ( ( r.valid_mode & VI_COMMAND_MODE ) != 0 ) {
          ::strcpy( &mode[ k ], "C" ); k++;
        }
        if ( ( r.valid_mode & SEARCH_MODE ) != 0 ) {
          ::strcpy( &mode[ k ], "S" ); k++;
        }
        if ( ( r.valid_mode & VISUAL_MODE ) != 0 ) {
          ::strcpy( &mode[ k ], "V" ); k++;
        }
        this->layout_keys( name, action, mode, descr );
      }
    }
    this->keys.off = this->keys.first;
    this->show_mode = SHOW_KEYS;
    if ( this->show_keys_start() )
      return true;
  }
  else {
    this->show_pg   = this->keys_pg;
    this->show_mode = SHOW_KEYS;
    return this->show_lsb( SHOW_KEYS, this->keys );
  }
  this->show_mode = SHOW_NONE;
  return false;
}

bool
State::show_keys_start( void )
{
  this->show_pg  = this->pgcount( this->keys ) - 1;
  this->keys.off = this->keys.first;
  this->keys_pg  = this->show_pg;
  return this->show_save( 0, 0 );
}

bool
State::show_keys_end( void )
{
  this->show_pg = 0;
  this->keys_pg = 0;
  return this->show_lsb( SHOW_KEYS, this->keys );
}

bool
State::show_keys_next_page( void )
{
  if ( this->show_pg > 0 )
    this->show_pg--;
  this->keys_pg = this->show_pg;
  return this->show_lsb( SHOW_KEYS, this->keys );
}

bool
State::show_keys_prev_page( void )
{
  if ( this->show_pg < this->pgcount( this->keys ) - 1 )
    this->show_pg++;
  this->keys_pg = this->show_pg;
  return this->show_lsb( SHOW_KEYS, this->keys );
}

extern "C"
int
lc_key_to_name( const char *key,  char *name )
{
  static struct {
    const char *name;
    const char *code;
  } code_name[] = {
     { "arrow-left",    ARROW_LEFT },   /* complex codes */
     { "arrow-right",   ARROW_RIGHT },
     { "arrow-up",      ARROW_UP },
     { "arrow-down",    ARROW_DOWN },
     { "arrow-left-vt", ARROW_LEFT2 },
     { "arrow-right-vt",ARROW_RIGHT2 },
     { "arrow-up-vt",   ARROW_UP2 },
     { "arrow-down-vt", ARROW_DOWN2 },
     { "home",          HOME_FUNCTION },
     { "end",           END_FUNCTION },
     { "shift-tab",     SHIFT_TAB },
     { "ctrl-left",     CTRL_ARROW_LEFT },
     { "ctrl-right",    CTRL_ARROW_RIGHT },
     { "meta-left",     META_ARROW_LEFT },
     { "meta-right",    META_ARROW_RIGHT },
     { "insert",        INSERT_FUNCTION },
     { "delete",        DELETE_FUNCTION },
     { "page-up",       PGUP_FUNCTION },
     { "page-down",     PGDN_FUNCTION },
     { "f1",            F1_FUNCTION },
     { "f2",            F2_FUNCTION },
     { "f3",            F3_FUNCTION },
     { "f4",            F4_FUNCTION },
     { "f5",            F5_FUNCTION },
     { "f6",            F6_FUNCTION },
     { "f7",            F7_FUNCTION },
     { "f8",            F8_FUNCTION },
     { "f9",            F9_FUNCTION },
     { "f10",           F10_FUNCTION },
     { "f11",           F11_FUNCTION },
     { "f12",           F12_FUNCTION } };
  size_t code_name_size = sizeof( code_name ) / sizeof( code_name[ 0 ] );
  size_t sz, j, k = 0, m;
  bool nospace = true;
  name[ 0 ] = 0;
  for ( size_t i = 0; i < sizeof( KeyCode ) && key[ i ] != 0; i += sz ) {
    sz = 1;
    if ( ! nospace )
      name[ k++ ] = ' ';
    nospace = false;
    if ( key[ i ] == 27 ) { /* complex all start with ESC */
      for ( j = 0; j < code_name_size; j++ ) {
        for ( m = 0; key[ i + m ] != 0; m++ ) {
          if ( code_name[ j ].code[ m ] != key[ i + m ] )
            break;
        }
        if ( code_name[ j ].code[ m ] == key[ i + m ] ) {
          ::strcpy( &name[ k ], code_name[ j ].name );
          sz = ::strlen( code_name[ j ].code );
          k += ::strlen( &name[ k ] );
          break;
        }
      }
      if ( j < code_name_size )
        continue;
    }
    switch ( (uint8_t) key[ i ] ) {
      case 1: case 2: case 3: case 4: case 5:
      case 6: case 7: case 8: /*tab*/ case 10:
      case 11: case 12: /*enter*/ case 14: case 15:
      case 16: case 17: case 18: case 19: case 20:
      case 21: case 22: case 23: case 24: case 25:
      case 26: /*esc*/ case 28: case 29: case 30: case 31: /*space*/
        ::strcpy( &name[ k ], "ctrl-" );
        k += 5;
        name[ k ] = 'a' + key[ i ] - 1;
        if ( name[ k ] > 'z' )
          name[ k ] -= 'a' - 'A';
        k++;
        break;
      case 27:
        if ( i + 1 < sizeof( KeyCode ) && key[ i + 1 ] != 0 ) {
          ::strcpy( &name[ k ], "meta-" );
          k += 5;
          nospace = true;
        }
        else {
          ::strcpy( &name[ k ], "esc" );
          k += 3;
        }
        break;
      case 127:
        ::strcpy( &name[ k ], "backspace" );
        k += 9;
        break;
      case '\r':
        ::strcpy( &name[ k ], "enter" );
        k += 5;
        break;
      case '\t':
        ::strcpy( &name[ k ], "tab" );
        k += 3;
        break;
      case ' ':
        ::strcpy( &name[ k ], "space" );
        k += 5;
        break;
      default:
        name[ k++ ] = key[ i ];
        break;
    }
  }
  name[ k ] = 0;
  return (int) k;
}

extern "C"
const char *
lc_action_to_name( KeyAction action )
{
  switch ( action ) {
    case ACTION_PENDING:             return "pending";
    case ACTION_INSERT_CHAR:         return "insert_char";
    case ACTION_FINISH_LINE:         return "finish_line";
    case ACTION_OPER_AND_NEXT:       return "oper_and_next";
    case ACTION_GOTO_BOL:            return "goto_bol";
    case ACTION_INTERRUPT:           return "interrupt";
    case ACTION_SUSPEND:             return "suspend";
    case ACTION_DELETE_CHAR:         return "delete_char";
    case ACTION_BACKSPACE:           return "backspace";
    case ACTION_GOTO_EOL:            return "goto_eol";
    case ACTION_GO_LEFT:             return "go_left";
    case ACTION_GO_RIGHT:            return "go_right";
    case ACTION_GOTO_PREV_WORD:      return "goto_prev_word";
    case ACTION_VI_GOTO_PREV_WORD:   return "vi_goto_prev_word";
    case ACTION_GOTO_NEXT_WORD:      return "goto_next_word";
    case ACTION_VI_GOTO_NEXT_WORD:   return "vi_goto_next_word";
    case ACTION_GOTO_ENDOF_WORD:     return "goto_endof_word";
    case ACTION_FIND_NEXT_CHAR:      return "find_next_char";
    case ACTION_FIND_PREV_CHAR:      return "find_prev_char";
    case ACTION_TILL_NEXT_CHAR:      return "till_next_char";
    case ACTION_TILL_PREV_CHAR:      return "till_prev_char";
    case ACTION_REPEAT_FIND_NEXT:    return "repeat_find_next";
    case ACTION_REPEAT_FIND_PREV:    return "repeat_find_prev";
    case ACTION_TAB_COMPLETE:        return "tab_complete";
    case ACTION_TAB_REVERSE:         return "tab_reverse";
    case ACTION_PREV_ITEM:           return "prev_item";
    case ACTION_NEXT_ITEM:           return "next_item";
    case ACTION_SEARCH_HISTORY:      return "search_history";
    case ACTION_SEARCH_NEXT:         return "search_next";
    case ACTION_SEARCH_NEXT_REV:     return "search_next_rev";
    case ACTION_SEARCH_REVERSE:      return "search_reverse";
    case ACTION_SEARCH_INLINE:       return "search_inline";
    case ACTION_HISTORY_COMPLETE:    return "history_complete";
    case ACTION_CANCEL_SEARCH:       return "cancel_search";
    case ACTION_TRANSPOSE:           return "transpose";
    case ACTION_TRANSPOSE_WORDS:     return "transpose_words";
    case ACTION_CAPITALIZE_WORD:     return "capitalize_word";
    case ACTION_LOWERCASE_WORD:      return "lowercase_word";
    case ACTION_UPPERCASE_WORD:      return "uppercase_word";
    case ACTION_ERASE_BOL:           return "erase_bol";
    case ACTION_ERASE_EOL:           return "erase_eol";
    case ACTION_ERASE_LINE:          return "erase_line";
    case ACTION_ERASE_PREV_WORD:     return "erase_prev_word";
    case ACTION_ERASE_NEXT_WORD:     return "erase_next_word";
    case ACTION_VI_ERASE_NEXT_WORD:  return "vi_erase_next_word";
    case ACTION_FLIP_CASE:           return "flip_case";
    case ACTION_MATCH_PAREN:         return "match_paren";
    case ACTION_INCR_NUMBER:         return "incr_number";
    case ACTION_DECR_NUMBER:         return "decr_number";
    case ACTION_REDRAW_LINE:         return "redraw_line";
    case ACTION_REPLACE_MODE:        return "replace_mode";
    case ACTION_UNDO:                return "undo";
    case ACTION_REVERT:              return "revert";
    case ACTION_REDO:                return "redo";
    case ACTION_PASTE_APPEND:        return "paste_append";
    case ACTION_PASTE_INSERT:        return "paste_insert";
    case ACTION_QUOTED_INSERT:       return "quoted_insert";
    case ACTION_YANK_BOL:            return "yank_bol";
    case ACTION_YANK_LINE:           return "yank_line";
    case ACTION_YANK_WORD:           return "yank_word";
    case ACTION_YANK_EOL:            return "yank_eol";
    case ACTION_YANK_LAST_ARG:       return "yank_last_arg";
    case ACTION_YANK_NTH_ARG:        return "yank_nth_arg";
    case ACTION_YANK_POP:            return "yank_pop";
    case ACTION_VI_MODE:             return "vi_mode";
    case ACTION_VI_ESC:              return "vi_esc";
    case ACTION_VI_INSERT:           return "vi_insert";
    case ACTION_VI_INSERT_BOL:       return "vi_insert_bol";
    case ACTION_VI_REPLACE:          return "vi_replace";
    case ACTION_VI_REPLACE_CHAR:     return "vi_replace_char";
    case ACTION_VI_APPEND:           return "vi_append";
    case ACTION_VI_APPEND_EOL:       return "vi_append_eol";
    case ACTION_VI_CHANGE_BOL:       return "vi_change_bol";
    case ACTION_VI_CHANGE_LINE:      return "vi_change_line";
    case ACTION_VI_CHANGE_WORD:      return "vi_change_word";
    case ACTION_VI_CHANGE_CHAR:      return "vi_change_char";
    case ACTION_VI_CHANGE_EOL:       return "vi_change_eol";
    case ACTION_REPEAT_LAST:         return "repeat_last";
    case ACTION_VI_MARK:             return "vi_mark";
    case ACTION_VI_GOTO_MARK:        return "vi_goto_mark";
    case ACTION_VISUAL_MARK:         return "visual_mark";
    case ACTION_VISUAL_CHANGE:       return "visual_change";
    case ACTION_VISUAL_DELETE:       return "visual_delete";
    case ACTION_VISUAL_YANK:         return "visual_yank";
    case ACTION_VI_REPEAT_DIGIT:     return "vi_repeat_digit";
    case ACTION_EMACS_ARG_DIGIT:     return "emacs_arg_digit";
    case ACTION_SHOW_DIRS:           return "show_dirs";
    case ACTION_SHOW_EXES:           return "show_exes";
    case ACTION_SHOW_FILES:          return "show_files";
    case ACTION_SHOW_HISTORY:        return "show_history";
    case ACTION_SHOW_KEYS:           return "show_keys";
    case ACTION_SHOW_CLEAR:          return "show_clear";
    case ACTION_SHOW_UNDO:           return "show_undo";
    case ACTION_SHOW_VARS:           return "show_vars";
    case ACTION_SHOW_YANK:           return "show_yank";
    case ACTION_SHOW_PREV_PAGE:      return "show_prev_page";
    case ACTION_SHOW_NEXT_PAGE:      return "show_next_page";
    case ACTION_GOTO_FIRST_ENTRY:    return "goto_first_entry";
    case ACTION_GOTO_LAST_ENTRY:     return "goto_last_entry";
    case ACTION_GOTO_ENTRY:          return "goto_entry";
    case ACTION_GOTO_TOP:            return "goto_top";
    case ACTION_GOTO_MIDDLE:         return "goto_middle";
    case ACTION_GOTO_BOTTOM:         return "goto_bottom";
    case ACTION_BELL:                return "bell";
    case ACTION_PUTBACK:             return "putback";
  }
  return "unknown";
}

extern "C"
const char *
lc_action_to_descr( KeyAction action )
{
  switch ( action ) {                    /* <- 35 chars wide ->               */
    case ACTION_PENDING:             return "Pending operation";
    case ACTION_INSERT_CHAR:         return "Insert character";
    case ACTION_FINISH_LINE:         
    case ACTION_OPER_AND_NEXT:       return "Execute line and reset state";
    case ACTION_GOTO_BOL:            return "Goto beginning of line";
    case ACTION_INTERRUPT:           return "Cancel line and reset state";
    case ACTION_SUSPEND:             return "Stop editing are refresh";
    case ACTION_DELETE_CHAR:         return "Delete char under cursor";
    case ACTION_BACKSPACE:           return "Delete char before cursor";
    case ACTION_GOTO_EOL:            return "Goto end of line";
    case ACTION_GO_LEFT:             return "Goto char before cursor";
    case ACTION_GO_RIGHT:            return "Goto char after cursor";
    case ACTION_GOTO_PREV_WORD:      return "Goto word before cursor (emacs)";
    case ACTION_VI_GOTO_PREV_WORD:   return "Goto word before cursor (vi)";
    case ACTION_GOTO_NEXT_WORD:      return "Goto word after cursor (emacs)";
    case ACTION_VI_GOTO_NEXT_WORD:   return "Goto word after cursor (vi)";
    case ACTION_GOTO_ENDOF_WORD:     return "Goto the end of word at cursor";
    case ACTION_FIND_NEXT_CHAR:      return "Scan fwd in line for char match";
    case ACTION_FIND_PREV_CHAR:      return "Scan back in line for char match";
    case ACTION_TILL_NEXT_CHAR:      return "Scan fwd in line for char match";
    case ACTION_TILL_PREV_CHAR:      return "Scan back in line for char match";
    case ACTION_REPEAT_FIND_NEXT:    return "Repeat scan forward char match";
    case ACTION_REPEAT_FIND_PREV:    return "Repeat scan backward char match";
    case ACTION_TAB_COMPLETE:        return "Use TAB completion at cursor";
    case ACTION_TAB_REVERSE:         return "Use TAB for previous completion";
    case ACTION_PREV_ITEM:           return "Goto previous item in history";
    case ACTION_NEXT_ITEM:           return "Goto next item in history";
    case ACTION_SEARCH_HISTORY:      return "Search history from end";
    case ACTION_SEARCH_NEXT:         return "Goto next history search match";
    case ACTION_SEARCH_NEXT_REV:     return "Goto prev history search match";
    case ACTION_SEARCH_REVERSE:      return "Search history from start";
    case ACTION_SEARCH_INLINE:       return "Insert history search at cursor";
    case ACTION_HISTORY_COMPLETE:    return "Search hist using word at cursor";
    case ACTION_CANCEL_SEARCH:       return "Cancel history search";
    case ACTION_TRANSPOSE:           return "Transpose two chars at cursor";
    case ACTION_TRANSPOSE_WORDS:     return "Transpose two words at cursor";
    case ACTION_CAPITALIZE_WORD:     return "Capitalize word at cursor";
    case ACTION_LOWERCASE_WORD:      return "Lowercase word at cursor";
    case ACTION_UPPERCASE_WORD:      return "Uppercase word at cursor";
    case ACTION_ERASE_BOL:           return "Erase from cursor to line start";
    case ACTION_ERASE_EOL:           return "Erase from cursor to line end";
    case ACTION_ERASE_LINE:          return "Erase all of line";
    case ACTION_ERASE_PREV_WORD:     return "Erase word before cursor";
    case ACTION_ERASE_NEXT_WORD:     return "Erase word after cursor (emacs)";
    case ACTION_VI_ERASE_NEXT_WORD:  return "Erase word after cursor (vi)";
    case ACTION_FLIP_CASE:           return "Toggle case of letter at cursor";
    case ACTION_MATCH_PAREN:         return "Match the paren at cursor";
    case ACTION_INCR_NUMBER:         return "Increment number at cursor";
    case ACTION_DECR_NUMBER:         return "Decrement number at cursor";
    case ACTION_REDRAW_LINE:         return "Refresh prompt and line";
    case ACTION_REPLACE_MODE:        return "Toggle insert and replace mode";
    case ACTION_UNDO:                return "Undo last edit";
    case ACTION_REVERT:              return "Revert line to original state";
    case ACTION_REDO:                return "Redo edit that was undone";
    case ACTION_PASTE_APPEND:        return "Paste append the yank buffer";
    case ACTION_PASTE_INSERT:        return "Paste insert the yank buffer";
    case ACTION_QUOTED_INSERT:       return "Insert key typed (eg. ctrl char)";
    case ACTION_YANK_BOL:            return "Yank from cursor to line start";
    case ACTION_YANK_LINE:           return "Yank all of line";
    case ACTION_YANK_WORD:           return "Yank word at cursor";
    case ACTION_YANK_EOL:            return "Yank from cursor to line end";
    case ACTION_YANK_LAST_ARG:       return "Yank last argument";
    case ACTION_YANK_NTH_ARG:        return "Yank Nth argument";
    case ACTION_YANK_POP:            return "Yank next item in yank stack";
    case ACTION_VI_MODE:             return "Toggle vi and emacs mode";
    case ACTION_VI_ESC:              return "Switch vi ins mode to cmd mode";
    case ACTION_VI_INSERT:           return "Go to vi insert mode at cursor";
    case ACTION_VI_INSERT_BOL:       return "Go to vi ins mode at line start";
    case ACTION_VI_REPLACE:          return "Go to vi ins mode with replace";
    case ACTION_VI_REPLACE_CHAR:     return "Replace char under cursor";
    case ACTION_VI_APPEND:           return "Go to vi insert mode after cursor";
    case ACTION_VI_APPEND_EOL:       return "Go to vi insert mode at line end";
    case ACTION_VI_CHANGE_BOL:       return "Change from cursor to line start";
    case ACTION_VI_CHANGE_LINE:      return "Change all of line";
    case ACTION_VI_CHANGE_WORD:      return "Change word under cursor";
    case ACTION_VI_CHANGE_CHAR:      return "Change char under cursor";
    case ACTION_VI_CHANGE_EOL:       return "Change from cursor to line end";
    case ACTION_REPEAT_LAST:         return "Repeat last operation";
    case ACTION_VI_MARK:             return "Mark the cursor position";
    case ACTION_VI_GOTO_MARK:        return "Goto a marked position";
    case ACTION_VISUAL_MARK:         return "Toggle visual select mode";
    case ACTION_VISUAL_CHANGE:       return "Change visual selection";
    case ACTION_VISUAL_DELETE:       return "Delete visual selection";
    case ACTION_VISUAL_YANK:         return "Yank visual selection";
    case ACTION_VI_REPEAT_DIGIT:     return "Add to repeat counter";
    case ACTION_EMACS_ARG_DIGIT:     return "Add to emacs arg counter";
    case ACTION_SHOW_DIRS:           return "Use TAB completion w/dirs";
    case ACTION_SHOW_EXES:           return "Use TAB completion w/exes";
    case ACTION_SHOW_FILES:          return "Use TAB completion w/files";
    case ACTION_SHOW_HISTORY:        return "Show the history lines";
    case ACTION_SHOW_KEYS:           return "Show the key bindings";
    case ACTION_SHOW_CLEAR:          return "Clear the show buffer";
    case ACTION_SHOW_UNDO:           return "Show the undo lines";
    case ACTION_SHOW_VARS:           return "Use TAB completion w/env vars";
    case ACTION_SHOW_YANK:           return "Show the yank lines";
    case ACTION_SHOW_PREV_PAGE:      return "Page up, show buf or history";
    case ACTION_SHOW_NEXT_PAGE:      return "Page down, show buf or history";
    case ACTION_GOTO_FIRST_ENTRY:    return "Goto 1st entry of show / history";
    case ACTION_GOTO_LAST_ENTRY:     return "Goto last entry of show / history";
    case ACTION_GOTO_ENTRY:          return "Goto entry of number entered";
    case ACTION_GOTO_TOP:            return "Goto top of current page";
    case ACTION_GOTO_MIDDLE:         return "Goto middle of current page";
    case ACTION_GOTO_BOTTOM:         return "Goto bottom of current page";
    case ACTION_BELL:                return "Ring bell (show bell)";
    case ACTION_PUTBACK:             return "Putback char";
  }
  return "unknown";
}

extern "C" {
KeyCode    KEY_CTRL_A        = {   1 }, /* unused are commented */
           KEY_CTRL_B        = {   2 },
           KEY_CTRL_C        = {   3 },
           KEY_CTRL_D        = {   4 },
           KEY_CTRL_E        = {   5 },
           KEY_CTRL_F        = {   6 },
           KEY_CTRL_G        = {   7 },
           KEY_CTRL_H        = {   8 },
           KEY_TAB           = {   9 },
           KEY_CTRL_J        = {  10 },
           KEY_CTRL_K        = {  11 },
           KEY_CTRL_L        = {  12 },
           KEY_ENTER         = {  13 },
           KEY_CTRL_N        = {  14 },
           KEY_CTRL_O        = {  15 },
           KEY_CTRL_P        = {  16 },
           KEY_CTRL_Q        = {  17 },
           KEY_CTRL_R        = {  18 },
           KEY_CTRL_S        = {  19 },
           KEY_CTRL_T        = {  20 },
           KEY_CTRL_U        = {  21 },
           KEY_CTRL_V        = {  22 },
           KEY_CTRL_W        = {  23 },
           KEY_CTRL_X        = {  24 }, /* emacs CX below -- don't use */
           KEY_CTRL_Y        = {  25 },
           KEY_CTRL_Z        = {  26 },
           KEY_ESC           = {  27 },
           KEY_CTRL_RBR      = {  29 },
           KEY_CTRL__        = {  31 },
           KEY_CX_CTRL_R     = {  24, 18  }, /* emacs: ctrl-x ctrl-r */
           KEY_CX_CTRL_U     = {  24, 21  }, /* emacs: ctrl-x ctrl-u */
           KEY_CX_BACKSPACE  = {  24, 127 },
           KEY_BACKSPACE     = { 127 },
           META_0            = {  27, '0' }, /* emacs argument digits */
           META_1            = {  27, '1' },
           META_2            = {  27, '2' },
           META_3            = {  27, '3' },
           META_4            = {  27, '4' },
           META_5            = {  27, '5' },
           META_6            = {  27, '6' },
           META_7            = {  27, '7' },
           META_8            = {  27, '8' },
           META_9            = {  27, '9' },
           META_MINUS        = {  27, '-' },
           META_A            = {  27, 'a' }, /* unused */
           META_B            = {  27, 'b' },
           META_C            = {  27, 'c' },
           META_D            = {  27, 'd' },
           META_E            = {  27, 'e' }, /* unused */
           META_F            = {  27, 'f' },
           META_G            = {  27, 'g' }, /* unused */
           META_H            = {  27, 'h' },
           META_J            = {  27, 'j' },
           META_K            = {  27, 'k' },
           META_L            = {  27, 'l' },
           META_N            = {  27, 'n' }, /* unused */
           META_M            = {  27, 'm' },
           META_P            = {  27, 'p' },
           META_O            = {  27, 'o' }, /* unused */
           META_R            = {  27, 'r' },
           META_S            = {  27, 's' },
           META_T            = {  27, 't' },
           META_U            = {  27, 'u' },
           META_V            = {  27, 'v' }, /* unused */
           META_W            = {  27, 'w' }, /* unused */
           META_X            = {  27, 'x' }, /* unused */
           META_Y            = {  27, 'y' },
           META_Z            = {  27, 'z' }, /* unused */
           META_DOT          = {  27, '.' },
           META_LT           = {  27, '<' },
           META_GT           = {  27, '>' },
           META__            = {  27, '_' },
           META_CTRL_A       = {  27,  1  }, /* unused */
           META_CTRL_B       = {  27,  2  }, /* unused */
           META_CTRL_C       = {  27,  3  }, /* unused */
           META_CTRL_D       = {  27,  4  },
           META_CTRL_E       = {  27,  5  },
           META_CTRL_F       = {  27,  6  },
           META_CTRL_H       = {  27,  8  },
           META_CTRL_I       = {  27,  9  }, /* unused */
           META_CTRL_J       = {  27, 10  },
           META_CTRL_K       = {  27, 11  },
           META_CTRL_L       = {  27, 12  },
           META_CTRL_M       = {  27, 13  },
           META_CTRL_N       = {  27, 14  }, /* unused */
           META_CTRL_O       = {  27, 15  }, /* unused */
           META_CTRL_P       = {  27, 16  },
           META_CTRL_Q       = {  27, 17  }, /* unused */
           META_CTRL_R       = {  27, 18  }, /* unused */
           META_CTRL_S       = {  27, 19  }, /* unused */
           META_CTRL_T       = {  27, 20  }, /* unused */
           META_CTRL_U       = {  27, 21  },
           META_CTRL_V       = {  27, 22  },
           META_CTRL_W       = {  27, 23  }, /* unused */
           META_CTRL_X       = {  27, 24  }, /* unused */
           META_CTRL_Y       = {  27, 25  },
           META_CTRL_Z       = {  27, 26  }, /* unused */
           META_CTRL_RBR     = {  27, 29  },
           META_BACKSPACE    = {  27, 127 },
           ARROW_LEFT        = {  27, '[', 'D' },
           ARROW_RIGHT       = {  27, '[', 'C' },
/* alt */  ARROW_LEFT2       = {  27, 'O', 'D' },
           ARROW_RIGHT2      = {  27, 'O', 'C' },
           CTRL_ARROW_LEFT   = {  27, '[', '1', ';', '5', 'D' },
           CTRL_ARROW_RIGHT  = {  27, '[', '1', ';', '5', 'C' },
           META_ARROW_LEFT   = {  27, '[', '1', ';', '3', 'D' },
           META_ARROW_RIGHT  = {  27, '[', '1', ';', '3', 'C' },
           ARROW_UP          = {  27, '[', 'A' },
           ARROW_DOWN        = {  27, '[', 'B' },
/* alt */  ARROW_UP2         = {  27, 'O', 'A' },
           ARROW_DOWN2       = {  27, 'O', 'B' },
           INSERT_FUNCTION   = {  27, '[', '2', '~' },
           DELETE_FUNCTION   = {  27, '[', '3', '~' },
           HOME_FUNCTION     = {  27, '[', 'H' },
           END_FUNCTION      = {  27, '[', 'F' },
           SHIFT_TAB         = {  27, '[', 'Z' },
           PGUP_FUNCTION     = {  27, '[', '5', '~' },
           PGDN_FUNCTION     = {  27, '[', '6', '~' },
           F1_FUNCTION       = {  27, 'O', 'P' },
           F2_FUNCTION       = {  27, 'O', 'Q' },
           F3_FUNCTION       = {  27, 'O', 'R' },
           F4_FUNCTION       = {  27, 'O', 'S' },
           F5_FUNCTION       = {  27, '[', '1', '5', '~' },
           F6_FUNCTION       = {  27, '[', '1', '7', '~' },
           F7_FUNCTION       = {  27, '[', '1', '8', '~' },
           F8_FUNCTION       = {  27, '[', '1', '9', '~' },
           F9_FUNCTION       = {  27, '[', '2', '0', '~' },
           F10_FUNCTION      = {  27, '[', '2', '1', '~' },
           F11_FUNCTION      = {  27, '[', '2', '3', '~' },
           F12_FUNCTION      = {  27, '[', '2', '4', '~' };

/* enhancement:  could load this table from somewhere, or modify w/termcap */
KeyRecipe lc_default_key_recipe[] = {
{ KEY_ESC         , ACTION_VI_ESC          , MOVE_MODE       , 0 },
{ KEY_CTRL_A      , ACTION_GOTO_BOL        , MOVE_MODE       , 0 },
{ KEY_CTRL_B      , ACTION_GO_LEFT         , MOVE_MODE       , 0 },
{ KEY_CTRL_C      , ACTION_INTERRUPT       , MOVE_MODE       , 0 },
{ KEY_CTRL_D      , ACTION_DELETE_CHAR     , VI_EMACS_MODE   , OPT_YA_UN_REP },
{ KEY_CTRL_E      , ACTION_GOTO_EOL        , MOVE_MODE       , 0 },
{ KEY_CTRL_F      , ACTION_GO_RIGHT        , MOVE_MODE       , 0 },
{ KEY_CTRL_G      , ACTION_SHOW_CLEAR      , MOVE_MODE       , 0 },
{ KEY_CTRL_H      , ACTION_BACKSPACE       , VI_EMACS_MODE   , OPT_YA_UN_REP },
{ KEY_BACKSPACE   , ACTION_BACKSPACE       , VI_EMACS_MODE   , OPT_YA_UN_REP },
{ KEY_TAB         , ACTION_TAB_COMPLETE    , EVIL_MODE       , 0 },
{ SHIFT_TAB       , ACTION_TAB_REVERSE     , EVIL_MODE       , 0 },
{ KEY_CTRL_J      , ACTION_FINISH_LINE     , MOVE_MODE       , 0 },
{ KEY_CTRL_K      , ACTION_ERASE_EOL       , VI_EMACS_MODE   , OPT_YA_UN_REP },
{ KEY_CTRL_L      , ACTION_REDRAW_LINE     , MOVE_MODE       , 0 },
{ KEY_ENTER       , ACTION_FINISH_LINE     , MOVE_MODE       , 0 },
{ KEY_CTRL_N      , ACTION_NEXT_ITEM       , EVIL_MODE       , 0 },
{ KEY_CTRL_O      , ACTION_OPER_AND_NEXT   , MOVE_MODE       , 0 },
{ KEY_CTRL_P      , ACTION_PREV_ITEM       , EVIL_MODE       , 0 },
{ KEY_CTRL_Q      , ACTION_QUOTED_INSERT   , EVILS_MODE      , 0 },
{ KEY_CTRL_R      , ACTION_SEARCH_HISTORY  , VI_EMACS_MODE   , 0 },
{ KEY_CTRL_R      , ACTION_REDO            , VI_COMMAND_MODE , 0 },
{ KEY_CTRL_S      , ACTION_SEARCH_REVERSE  , VI_EMACS_MODE   , 0 },
{ KEY_CTRL_T      , ACTION_TRANSPOSE       , VI_EMACS_MODE   , OPT_UNDO_REP },
{ KEY_CTRL_U      , ACTION_ERASE_BOL       , VI_EMACS_MODE   , OPT_YA_UN_REP },
{ KEY_CTRL_V      , ACTION_VISUAL_MARK     , MOVE_MODE       , 0 }, /* XXX */
{ KEY_CTRL_W      , ACTION_ERASE_PREV_WORD , VI_EMACS_MODE   , OPT_YA_UN_REP },
{ KEY_CX_BACKSPACE, ACTION_ERASE_BOL       , VI_EMACS_MODE   , OPT_YA_UN_REP },
{ KEY_CX_CTRL_R   , ACTION_REDO            , VI_EMACS_MODE   , 0 },
{ KEY_CX_CTRL_U   , ACTION_UNDO            , VI_EMACS_MODE   , 0 },
{ KEY_CTRL_Y      , ACTION_PASTE_INSERT    , VI_EMACS_MODE   , OPT_UNDO_REP },
{ KEY_CTRL_Z      , ACTION_SUSPEND         , MOVE_MODE       , 0 },
{ KEY_CTRL_RBR    , ACTION_FIND_NEXT_CHAR  , MOVE_MODE       , OPT_VI_CHAR_OP },
{ META_CTRL_RBR   , ACTION_FIND_PREV_CHAR  , MOVE_MODE       , OPT_VI_CHAR_OP },
{ KEY_CTRL__      , ACTION_UNDO            , VI_EMACS_MODE   , 0 },
{ 0               , ACTION_INSERT_CHAR     , VI_EMACS_MODE   , 0 }, /* default */

/* The meta functions could mess up vi insert processing, since escape goes to
 * command mode and meta + char == <esc> + char. If a network desides to delay
 * keystrokes, this will cause insert -> command mode + char instead of meta +
 * char. */
{ META_B          , ACTION_GOTO_PREV_WORD  , MOVE_MODE       , 0 },
{ META_C          , ACTION_CAPITALIZE_WORD , EVILS_MODE      , OPT_UNDO_REP },
{ META_D          , ACTION_ERASE_NEXT_WORD , EVILS_MODE      , OPT_YA_UN_REP },
{ META_F          , ACTION_GOTO_NEXT_WORD  , MOVE_MODE       , 0 },
{ META_H          , ACTION_ERASE_PREV_WORD , EVILS_MODE      , OPT_YA_UN_REP },
{ META_BACKSPACE  , ACTION_ERASE_PREV_WORD , EVILS_MODE      , OPT_YA_UN_REP },
{ META_J          , ACTION_SHOW_NEXT_PAGE  , EVILS_MODE      , 0 },
{ META_K          , ACTION_SHOW_PREV_PAGE  , EVILS_MODE      , 0 },
{ META_L          , ACTION_LOWERCASE_WORD  , EVILS_MODE      , OPT_UNDO_REP },
{ META_M          , ACTION_VI_MODE         , EVIL_MODE       , 0 },
{ META_P          , ACTION_HISTORY_COMPLETE, EVIL_MODE       , 0 },
{ META_S          , ACTION_SEARCH_INLINE   , EVIL_MODE       , 0 },
{ META_R          , ACTION_REPEAT_LAST     , EVILS_MODE      , 0 },
{ META_T          , ACTION_TRANSPOSE_WORDS , EVILS_MODE      , OPT_UNDO_REP },
{ META_U          , ACTION_UPPERCASE_WORD  , EVILS_MODE      , OPT_UNDO_REP },
{ META_Y          , ACTION_YANK_POP        , EVILS_MODE      , 0 },
{ META_DOT        , ACTION_YANK_LAST_ARG   , EVILS_MODE      , 0 },
{ META__          , ACTION_YANK_LAST_ARG   , EVILS_MODE      , 0 },
{ META_CTRL_Y     , ACTION_YANK_NTH_ARG    , EVILS_MODE      , 0 },
{ ARROW_LEFT      , ACTION_GO_LEFT         , MOVE_MODE       , 0 },
{ ARROW_RIGHT     , ACTION_GO_RIGHT        , MOVE_MODE       , 0 },
{ ARROW_LEFT2     , ACTION_GO_LEFT         , MOVE_MODE       , 0 },
{ ARROW_RIGHT2    , ACTION_GO_RIGHT        , MOVE_MODE       , 0 },
{ CTRL_ARROW_LEFT , ACTION_GOTO_BOL        , MOVE_MODE       , 0 },
{ CTRL_ARROW_RIGHT, ACTION_GOTO_EOL        , MOVE_MODE       , 0 },
{ META_ARROW_LEFT , ACTION_GOTO_PREV_WORD  , MOVE_MODE       , 0 },
{ META_ARROW_RIGHT, ACTION_GOTO_NEXT_WORD  , MOVE_MODE       , 0 },
{ ARROW_UP        , ACTION_PREV_ITEM       , EVIL_MODE       , 0 },
{ ARROW_DOWN      , ACTION_NEXT_ITEM       , EVIL_MODE       , 0 },
{ ARROW_UP2       , ACTION_PREV_ITEM       , EVIL_MODE       , 0 },
{ ARROW_DOWN2     , ACTION_NEXT_ITEM       , EVIL_MODE       , 0 },
{ INSERT_FUNCTION , ACTION_REPLACE_MODE    , VI_EMACS_MODE   , 0 },
{ DELETE_FUNCTION , ACTION_DELETE_CHAR     , EVILS_MODE      , OPT_YA_UN_REP },
{ HOME_FUNCTION   , ACTION_GOTO_BOL        , MOVE_MODE       , 0 },
{ END_FUNCTION    , ACTION_GOTO_EOL        , MOVE_MODE       , 0 },
{ META_LT         , ACTION_GOTO_FIRST_ENTRY, EVILS_MODE      , 0 },
{ META_GT         , ACTION_GOTO_LAST_ENTRY , EVILS_MODE      , 0 },
{ PGUP_FUNCTION   , ACTION_SHOW_PREV_PAGE  , EVILS_MODE      , 0 },
{ PGDN_FUNCTION   , ACTION_SHOW_NEXT_PAGE  , EVILS_MODE      , 0 },
{ "a"             , ACTION_VI_APPEND       , VI_COMMAND_MODE , OPT_UNDO },
{ "A"             , ACTION_VI_APPEND_EOL   , VI_COMMAND_MODE , OPT_UNDO },
{ "b"             , ACTION_VI_GOTO_PREV_WORD,VI_MOVE_MODE    , 0 },
{ "c0"            , ACTION_VI_CHANGE_BOL   , VI_COMMAND_MODE , OPT_UNDO },
{ "cc"            , ACTION_VI_CHANGE_LINE  , VI_COMMAND_MODE , OPT_UNDO },
{ "cw"            , ACTION_VI_CHANGE_WORD  , VI_COMMAND_MODE , OPT_UNDO },
{ "c$"            , ACTION_VI_CHANGE_EOL   , VI_COMMAND_MODE , OPT_UNDO },
{ "C"             , ACTION_VI_CHANGE_LINE  , VI_COMMAND_MODE , OPT_UNDO },
{ "d0"            , ACTION_ERASE_BOL       , VI_COMMAND_MODE , OPT_UNDO_REP },
{ "dd"            , ACTION_ERASE_LINE      , VI_COMMAND_MODE , OPT_UNDO_REP },
{ "dw"            , ACTION_VI_ERASE_NEXT_WORD,VI_COMMAND_MODE, OPT_UNDO_REP },
{ "d$"            , ACTION_ERASE_EOL       , VI_COMMAND_MODE , OPT_UNDO_REP },
{ "D"             , ACTION_ERASE_EOL       , VI_COMMAND_MODE , OPT_UNDO_REP },
{ "e"             , ACTION_GOTO_ENDOF_WORD , VI_MOVE_MODE    , 0 },
{ "f"             , ACTION_FIND_NEXT_CHAR  , VI_MOVE_MODE    , OPT_VI_CHAR_OP },
{ "F"             , ACTION_FIND_PREV_CHAR  , VI_MOVE_MODE    , OPT_VI_CHAR_OP },
{ "G"             , ACTION_GOTO_ENTRY      , VI_COMMAND_MODE , 0 },
{ "h"             , ACTION_GO_LEFT         , VI_MOVE_MODE    , 0 },
{ "H"             , ACTION_GOTO_TOP        , VI_COMMAND_MODE , 0 },
{ "i"             , ACTION_VI_INSERT       , VI_COMMAND_MODE , OPT_UNDO },
{ "I"             , ACTION_VI_INSERT_BOL   , VI_COMMAND_MODE , OPT_UNDO },
{ "j"             , ACTION_NEXT_ITEM       , VI_COMMAND_MODE , 0 },
{ "k"             , ACTION_PREV_ITEM       , VI_COMMAND_MODE , 0 },
{ " "             , ACTION_GO_RIGHT        , VI_MOVE_MODE    , 0 },
{ "l"             , ACTION_GO_RIGHT        , VI_MOVE_MODE    , 0 },
{ "L"             , ACTION_GOTO_BOTTOM     , VI_COMMAND_MODE , 0 },
{ "m"             , ACTION_VI_MARK         , VI_COMMAND_MODE, OPT_VI_CHAR_OP },
{ "M"             , ACTION_GOTO_MIDDLE     , VI_COMMAND_MODE , 0 },
{ "n"             , ACTION_SEARCH_NEXT     , VI_COMMAND_MODE , 0 },
{ "N"             , ACTION_SEARCH_NEXT_REV , VI_COMMAND_MODE , 0 },
{ "p"             , ACTION_PASTE_APPEND    , VI_COMMAND_MODE , OPT_UNDO_REP },
{ "P"             , ACTION_PASTE_INSERT    , VI_COMMAND_MODE , OPT_UNDO_REP },
{ "R"             , ACTION_VI_REPLACE      , VI_COMMAND_MODE , OPT_UNDO },
{ "r"             , ACTION_VI_REPLACE_CHAR , VI_COMMAND_MODE , OPT_UNDO_REP_OP},
{ "s"             , ACTION_VI_CHANGE_CHAR  , VI_COMMAND_MODE , OPT_UNDO },
{ "S"             , ACTION_VI_CHANGE_LINE  , VI_COMMAND_MODE , OPT_UNDO },
{ "t"             , ACTION_TILL_NEXT_CHAR  , VI_MOVE_MODE    , OPT_VI_CHAR_OP },
{ "T"             , ACTION_TILL_PREV_CHAR  , VI_MOVE_MODE    , OPT_VI_CHAR_OP },
{ "u"             , ACTION_UNDO            , VI_COMMAND_MODE , 0 },
{ "U"             , ACTION_REVERT          , VI_COMMAND_MODE , 0 },
{ "v"             , ACTION_VISUAL_MARK     , VI_MOVE_MODE    , 0 },
{ "w"             , ACTION_VI_GOTO_NEXT_WORD,VI_MOVE_MODE    , 0 },
{ "x"             , ACTION_DELETE_CHAR     , VI_COMMAND_MODE , OPT_YA_UN_REP },
{ "X"             , ACTION_BACKSPACE       , VI_COMMAND_MODE , OPT_YA_UN_REP },
{ "y0"            , ACTION_YANK_BOL        , VI_COMMAND_MODE , 0 },
{ "yy"            , ACTION_YANK_LINE       , VI_COMMAND_MODE , 0 },
{ "yw"            , ACTION_YANK_WORD       , VI_COMMAND_MODE , 0 },
{ "y$"            , ACTION_YANK_EOL        , VI_COMMAND_MODE , 0 },
{ "Y"             , ACTION_YANK_EOL        , VI_COMMAND_MODE , 0 },
{ "$"             , ACTION_GOTO_EOL        , VI_MOVE_MODE    , 0 },
{ "0"             , ACTION_GOTO_BOL        , VI_MOVE_MODE    , OPT_VI_REPC_OP },
{ "^"             , ACTION_GOTO_BOL        , VI_MOVE_MODE    , 0 },
{ "~"             , ACTION_FLIP_CASE       , VI_COMMAND_MODE , OPT_UNDO_REP },
{ "."             , ACTION_REPEAT_LAST     , VI_COMMAND_MODE , 0 },
{ ";"             , ACTION_REPEAT_FIND_NEXT, VI_MOVE_MODE    , 0 },
{ ","             , ACTION_REPEAT_FIND_PREV, VI_MOVE_MODE    , 0 },
{ "%"             , ACTION_MATCH_PAREN     , VI_MOVE_MODE    , 0 },
{ "/"             , ACTION_SEARCH_HISTORY  , VI_COMMAND_MODE , 0 },
{ "?"             , ACTION_SEARCH_REVERSE  , VI_COMMAND_MODE , 0 },
{ "`"             , ACTION_VI_GOTO_MARK    , VI_COMMAND_MODE , OPT_VI_CHAR_OP },
{ "'"             , ACTION_VI_GOTO_MARK    , VI_COMMAND_MODE , OPT_VI_CHAR_OP },
{ "="             , ACTION_TAB_COMPLETE    , VI_COMMAND_MODE , 0 },
{ "+"             , ACTION_INCR_NUMBER     , VI_COMMAND_MODE , OPT_UNDO_REP },
{ KEY_CTRL_H      , ACTION_GO_LEFT         , VI_MOVE_MODE    , 0 },
{ "-"             , ACTION_DECR_NUMBER     , VI_COMMAND_MODE , OPT_UNDO_REP },
{ KEY_BACKSPACE   , ACTION_GO_LEFT         , VI_MOVE_MODE    , 0 },
{ INSERT_FUNCTION , ACTION_VI_INSERT       , VI_COMMAND_MODE , OPT_UNDO },
{ "1"             , ACTION_VI_REPEAT_DIGIT , VI_COMMAND_MODE , OPT_VI_REPC_OP },
{ "2"             , ACTION_VI_REPEAT_DIGIT , VI_COMMAND_MODE , OPT_VI_REPC_OP },
{ "3"             , ACTION_VI_REPEAT_DIGIT , VI_COMMAND_MODE , OPT_VI_REPC_OP },
{ "4"             , ACTION_VI_REPEAT_DIGIT , VI_COMMAND_MODE , OPT_VI_REPC_OP },
{ "5"             , ACTION_VI_REPEAT_DIGIT , VI_COMMAND_MODE , OPT_VI_REPC_OP },
{ "6"             , ACTION_VI_REPEAT_DIGIT , VI_COMMAND_MODE , OPT_VI_REPC_OP },
{ "7"             , ACTION_VI_REPEAT_DIGIT , VI_COMMAND_MODE , OPT_VI_REPC_OP },
{ "8"             , ACTION_VI_REPEAT_DIGIT , VI_COMMAND_MODE , OPT_VI_REPC_OP },
{ "9"             , ACTION_VI_REPEAT_DIGIT , VI_COMMAND_MODE , OPT_VI_REPC_OP },
{ META_0          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       , 0 },
{ META_1          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       , 0 },
{ META_2          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       , 0 },
{ META_3          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       , 0 },
{ META_4          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       , 0 },
{ META_5          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       , 0 },
{ META_6          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       , 0 },
{ META_7          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       , 0 },
{ META_8          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       , 0 },
{ META_9          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       , 0 },
{ META_MINUS      , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       , 0 },
{ 0               , ACTION_BELL            , VI_COMMAND_MODE , 0 }, /* default */
/* visual mode */
{ "c"             , ACTION_VISUAL_CHANGE   , VISUAL_MODE     , OPT_UNDO },
{ "s"             , ACTION_VISUAL_CHANGE   , VISUAL_MODE     , OPT_UNDO },
{ KEY_CTRL_D      , ACTION_VISUAL_DELETE   , VISUAL_MODE     , OPT_UNDO },
{ "d"             , ACTION_VISUAL_DELETE   , VISUAL_MODE     , OPT_UNDO },
{ "x"             , ACTION_VISUAL_DELETE   , VISUAL_MODE     , OPT_UNDO },
{ "y"             , ACTION_VISUAL_YANK     , VISUAL_MODE     , 0 },
{ 0               , ACTION_VISUAL_MARK     , VISUAL_MODE     , 0 }, /* default */
/* show commands */
{ F1_FUNCTION     , ACTION_SHOW_KEYS       , MOVE_MODE       , 0 },
{ F2_FUNCTION     , ACTION_SHOW_FILES      , EVIL_MODE       , 0 },
{ F3_FUNCTION     , ACTION_SHOW_EXES       , EVIL_MODE       , 0 },
{ F4_FUNCTION     , ACTION_SHOW_CLEAR      , MOVE_MODE       , 0 },
{ F5_FUNCTION     , ACTION_SHOW_DIRS       , EVIL_MODE       , 0 },
{ F6_FUNCTION     , ACTION_SHOW_UNDO       , MOVE_MODE       , 0 },
{ F7_FUNCTION     , ACTION_SHOW_YANK       , MOVE_MODE       , 0 },
{ F8_FUNCTION     , ACTION_SHOW_HISTORY    , EVILS_MODE      , 0 },
{ F9_FUNCTION     , ACTION_REDRAW_LINE     , MOVE_MODE       , 0 },
{ F10_FUNCTION    , ACTION_SHOW_VARS       , EVIL_MODE       , 0 },

{ META_CTRL_D     , ACTION_SHOW_DIRS       , EVIL_MODE       , 0 },
{ META_CTRL_E     , ACTION_SHOW_EXES       , EVIL_MODE       , 0 },
{ META_CTRL_F     , ACTION_SHOW_FILES      , EVIL_MODE       , 0 },
{ META_CTRL_H     , ACTION_SHOW_HISTORY    , EVILS_MODE      , 0 },
{ META_CTRL_K     , ACTION_SHOW_KEYS       , MOVE_MODE       , 0 },
{ META_CTRL_L     , ACTION_SHOW_CLEAR      , MOVE_MODE       , 0 },
{ META_CTRL_P     , ACTION_SHOW_YANK       , MOVE_MODE       , 0 },
{ META_CTRL_U     , ACTION_SHOW_UNDO       , MOVE_MODE       , 0 },
{ META_CTRL_V     , ACTION_SHOW_VARS       , EVIL_MODE       , 0 }
};

size_t lc_default_key_recipe_size = sizeof( lc_default_key_recipe ) /
                                    sizeof( lc_default_key_recipe[ 0 ] );
}
