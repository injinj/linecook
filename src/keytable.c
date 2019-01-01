#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <linecook/keycook.h>

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
  int nospace = 1;
  name[ 0 ] = 0;
  for ( size_t i = 0; i < sizeof( KeyCode ) && key[ i ] != 0; i += sz ) {
    sz = 1;
    if ( ! nospace )
      name[ k++ ] = ' ';
    nospace = 0;
    if ( key[ i ] == 27 ) { /* complex all start with ESC */
      for ( j = 0; j < code_name_size; j++ ) {
        for ( m = 0; key[ i + m ] != 0; m++ ) {
          if ( code_name[ j ].code[ m ] != key[ i + m ] )
            break;
        }
        if ( code_name[ j ].code[ m ] == key[ i + m ] ) {
          strcpy( &name[ k ], code_name[ j ].name );
          sz = strlen( code_name[ j ].code );
          k += strlen( &name[ k ] );
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
        strcpy( &name[ k ], "ctrl-" );
        k += 5;
        name[ k ] = 'a' + key[ i ] - 1;
        if ( name[ k ] > 'z' )
          name[ k ] -= 'a' - 'A';
        k++;
        break;
      case 27:
        if ( i + 1 < sizeof( KeyCode ) && key[ i + 1 ] != 0 ) {
          strcpy( &name[ k ], "meta-" );
          k += 5;
          nospace = 1;
        }
        else {
          strcpy( &name[ k ], "esc" );
          k += 3;
        }
        break;
      case 127:
        strcpy( &name[ k ], "backspace" );
        k += 9;
        break;
      case '\r':
        strcpy( &name[ k ], "enter" );
        k += 5;
        break;
      case '\t':
        strcpy( &name[ k ], "tab" );
        k += 3;
        break;
      case ' ':
        strcpy( &name[ k ], "space" );
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
    case ACTION_SHOW_TREE:           return "show_tree";
    case ACTION_SHOW_FZF:            return "show_fzf";
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
    case ACTION_INSERT:              return "insert";
    case ACTION_INSERT_BOL:          return "insert_bol";
    case ACTION_APPEND:              return "append";
    case ACTION_APPEND_EOL:          return "append_eol";
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
    case ACTION_DECR_SHOW:           return "decr_show";
    case ACTION_INCR_SHOW:           return "incr_show";
    case ACTION_MACRO:               return "macro";
    case ACTION_ACTION:              return "action";
    case ACTION_BELL:                return "bell";
    case ACTION_PUTBACK:             return "putback";
  }
  return "unknown";
}

const char *
lc_action_to_descr( KeyAction action )
{
  switch ( action ) {                    /* <- 35 chars wide ->               */
    case ACTION_PENDING:             return "Pending operation";
    case ACTION_INSERT_CHAR:         return "Insert character";
    case ACTION_FINISH_LINE:         return "Execute line and reset state";
    case ACTION_OPER_AND_NEXT:       return "Execute completion for line";
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
    case ACTION_SHOW_TREE:           return "Search dir tree using substring";
    case ACTION_SHOW_FZF:            return "Search dir tree using fzf";
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
    case ACTION_INSERT:              return "Go to insert mode";
    case ACTION_INSERT_BOL:          return "Go to insert at line start";
    case ACTION_APPEND:              return "Go to insert after cursor";
    case ACTION_APPEND_EOL:          return "Go to insert at line end";
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
    case ACTION_DECR_SHOW:           return "Decrement show size";
    case ACTION_INCR_SHOW:           return "Increment show size";
    case ACTION_MACRO:               return "General purpose macro";
    case ACTION_ACTION:              return "General purpose action";
    case ACTION_BELL:                return "Ring bell (show bell)";
    case ACTION_PUTBACK:             return "Putback char";
  }
  return "unknown";
}

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
           KEY_CX_ACTION     = {  24, 1   }, /* action: ctrl-x ctrl-a <c> */
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
           META_SLASH        = {  27, '/' },
           META_LT           = {  27, '<' },
           META_GT           = {  27, '>' },
           META_LPAREN       = {  27, '(' },
           META_RPAREN       = {  27, ')' },
           META__            = {  27, '_' },
           META_CTRL_A       = {  27,  1  }, /* unused */
           META_CTRL_B       = {  27,  2  }, /* unused */
           META_CTRL_C       = {  27,  3  }, /* unused */
           META_CTRL_D       = {  27,  4  },
           META_CTRL_E       = {  27,  5  },
           META_CTRL_F       = {  27,  6  },
           META_CTRL_H       = {  27,  8  },
           META_TAB          = {  27,  9  }, /* window system uses */
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
{ KEY_ESC         , ACTION_VI_ESC          , MOVE_MODE       },
{ KEY_CTRL_A      , ACTION_GOTO_BOL        , MOVE_MODE       },
{ KEY_CTRL_B      , ACTION_GO_LEFT         , MOVE_MODE       },
{ KEY_CTRL_C      , ACTION_INTERRUPT       , MOVE_MODE       },
{ KEY_CTRL_D      , ACTION_DELETE_CHAR     , VI_EMACS_MODE   },
{ KEY_CTRL_E      , ACTION_GOTO_EOL        , MOVE_MODE       },
{ KEY_CTRL_F      , ACTION_GO_RIGHT        , MOVE_MODE       },
{ KEY_CTRL_G      , ACTION_SHOW_CLEAR      , MOVE_MODE       },
{ KEY_CTRL_H      , ACTION_BACKSPACE       , VI_EMACS_MODE   },
{ KEY_BACKSPACE   , ACTION_BACKSPACE       , VI_EMACS_MODE   },
{ KEY_TAB         , ACTION_TAB_COMPLETE    , EVIL_MODE       },
{ SHIFT_TAB       , ACTION_TAB_REVERSE     , EVIL_MODE       },
{ KEY_CTRL_J      , ACTION_FINISH_LINE     , MOVE_MODE       },
{ KEY_CTRL_K      , ACTION_ERASE_EOL       , VI_EMACS_MODE   },
{ KEY_CTRL_L      , ACTION_REDRAW_LINE     , MOVE_MODE       },
{ KEY_ENTER       , ACTION_FINISH_LINE     , MOVE_MODE       },
{ KEY_CTRL_N      , ACTION_NEXT_ITEM       , EVIL_MODE       },
{ KEY_CTRL_O      , ACTION_OPER_AND_NEXT   , MOVE_MODE       },
{ KEY_CTRL_P      , ACTION_PREV_ITEM       , EVIL_MODE       },
{ KEY_CTRL_Q      , ACTION_QUOTED_INSERT   , EVILS_MODE      },
{ KEY_CTRL_R      , ACTION_SEARCH_HISTORY  , VI_EMACS_MODE   },
{ KEY_CTRL_R      , ACTION_REDO            , VI_COMMAND_MODE },
{ KEY_CTRL_S      , ACTION_SEARCH_REVERSE  , VI_EMACS_MODE   },
{ KEY_CTRL_T      , ACTION_TRANSPOSE       , VI_EMACS_MODE   },
{ KEY_CTRL_U      , ACTION_ERASE_BOL       , VI_EMACS_MODE   },
{ KEY_CTRL_V      , ACTION_VISUAL_MARK     , MOVE_MODE       },
{ KEY_CTRL_W      , ACTION_ERASE_PREV_WORD , VI_EMACS_MODE   },
{ KEY_CX_BACKSPACE, ACTION_ERASE_BOL       , VI_EMACS_MODE   },
{ KEY_CX_CTRL_R   , ACTION_REDO            , VI_EMACS_MODE   },
{ KEY_CX_CTRL_U   , ACTION_UNDO            , VI_EMACS_MODE   },
{ KEY_CTRL_Y      , ACTION_PASTE_INSERT    , VI_EMACS_MODE   },
{ KEY_CTRL_Z      , ACTION_SUSPEND         , MOVE_MODE       },
{ KEY_CTRL_RBR    , ACTION_FIND_NEXT_CHAR  , MOVE_MODE       },
{ META_CTRL_RBR   , ACTION_FIND_PREV_CHAR  , MOVE_MODE       },
{ KEY_CTRL__      , ACTION_UNDO            , VI_EMACS_MODE   },
{ 0               , ACTION_INSERT_CHAR     , VI_EMACS_MODE   }, /* default */

/* The meta functions could mess up vi insert processing, since escape goes to
 * command mode and meta + char == <esc> + char. If a network desides to delay
 * keystrokes, this will cause insert -> command mode + char instead of meta +
 * char. */
{ META_A          , ACTION_GOTO_TOP        , EVILS_MODE      },
{ META_B          , ACTION_GOTO_PREV_WORD  , MOVE_MODE       },
{ META_C          , ACTION_CAPITALIZE_WORD , EVILS_MODE      },
{ META_D          , ACTION_ERASE_NEXT_WORD , EVILS_MODE      },
{ META_F          , ACTION_GOTO_NEXT_WORD  , MOVE_MODE       },
{ META_H          , ACTION_ERASE_PREV_WORD , EVILS_MODE      },
{ META_BACKSPACE  , ACTION_ERASE_PREV_WORD , EVILS_MODE      },
{ META_J          , ACTION_SHOW_NEXT_PAGE  , EVILS_MODE      },
{ META_K          , ACTION_SHOW_PREV_PAGE  , EVILS_MODE      },
{ META_L          , ACTION_LOWERCASE_WORD  , EVILS_MODE      },
{ META_M          , ACTION_VI_MODE         , EVIL_MODE       },
{ META_P          , ACTION_HISTORY_COMPLETE, EVIL_MODE       },
{ META_S          , ACTION_SEARCH_INLINE   , EVIL_MODE       },
{ META_R          , ACTION_REPEAT_LAST     , EVILS_MODE      },
{ META_T          , ACTION_TRANSPOSE_WORDS , EVILS_MODE      },
{ META_U          , ACTION_UPPERCASE_WORD  , EVILS_MODE      },
{ META_Y          , ACTION_YANK_POP        , EVILS_MODE      },
{ META_Z          , ACTION_GOTO_BOTTOM     , EVILS_MODE      },
{ META_DOT        , ACTION_YANK_LAST_ARG   , EVILS_MODE      },
{ META__          , ACTION_YANK_LAST_ARG   , EVILS_MODE      },
{ META_CTRL_Y     , ACTION_YANK_NTH_ARG    , EVILS_MODE      },
{ META_SLASH      , ACTION_SHOW_TREE       , EVIL_MODE       },
{ ARROW_LEFT      , ACTION_GO_LEFT         , MOVE_MODE       },
{ ARROW_RIGHT     , ACTION_GO_RIGHT        , MOVE_MODE       },
{ ARROW_LEFT2     , ACTION_GO_LEFT         , MOVE_MODE       },
{ ARROW_RIGHT2    , ACTION_GO_RIGHT        , MOVE_MODE       },
{ CTRL_ARROW_LEFT , ACTION_GOTO_BOL        , MOVE_MODE       },
{ CTRL_ARROW_RIGHT, ACTION_GOTO_EOL        , MOVE_MODE       },
{ META_ARROW_LEFT , ACTION_GOTO_PREV_WORD  , MOVE_MODE       },
{ META_ARROW_RIGHT, ACTION_GOTO_NEXT_WORD  , MOVE_MODE       },
{ ARROW_UP        , ACTION_PREV_ITEM       , EVIL_MODE       },
{ ARROW_DOWN      , ACTION_NEXT_ITEM       , EVIL_MODE       },
{ ARROW_UP2       , ACTION_PREV_ITEM       , EVIL_MODE       },
{ ARROW_DOWN2     , ACTION_NEXT_ITEM       , EVIL_MODE       },
{ INSERT_FUNCTION , ACTION_REPLACE_MODE    , VI_EMACS_MODE   },
{ DELETE_FUNCTION , ACTION_DELETE_CHAR     , EVILS_MODE      },
{ HOME_FUNCTION   , ACTION_GOTO_BOL        , MOVE_MODE       },
{ END_FUNCTION    , ACTION_GOTO_EOL        , MOVE_MODE       },
{ META_LT         , ACTION_GOTO_FIRST_ENTRY, EVILS_MODE      },
{ META_GT         , ACTION_GOTO_LAST_ENTRY , EVILS_MODE      },
{ PGUP_FUNCTION   , ACTION_SHOW_PREV_PAGE  , EVILS_MODE      },
{ PGDN_FUNCTION   , ACTION_SHOW_NEXT_PAGE  , EVILS_MODE      },
{ META_LPAREN     , ACTION_DECR_SHOW       , MOVE_MODE       },
{ META_RPAREN     , ACTION_INCR_SHOW       , MOVE_MODE       },
{ "a"             , ACTION_VI_APPEND       , VI_COMMAND_MODE },
{ "A"             , ACTION_VI_APPEND_EOL   , VI_COMMAND_MODE },
{ "b"             , ACTION_VI_GOTO_PREV_WORD,VI_MOVE_MODE    },
{ "c0"            , ACTION_VI_CHANGE_BOL   , VI_COMMAND_MODE },
{ "cc"            , ACTION_VI_CHANGE_LINE  , VI_COMMAND_MODE },
{ "cw"            , ACTION_VI_CHANGE_WORD  , VI_COMMAND_MODE },
{ "c$"            , ACTION_VI_CHANGE_EOL   , VI_COMMAND_MODE },
{ "C"             , ACTION_VI_CHANGE_LINE  , VI_COMMAND_MODE },
{ "d0"            , ACTION_ERASE_BOL       , VI_COMMAND_MODE },
{ "dd"            , ACTION_ERASE_LINE      , VI_COMMAND_MODE },
{ "dw"            , ACTION_VI_ERASE_NEXT_WORD,VI_COMMAND_MODE},
{ "d$"            , ACTION_ERASE_EOL       , VI_COMMAND_MODE },
{ "D"             , ACTION_ERASE_EOL       , VI_COMMAND_MODE },
{ "e"             , ACTION_GOTO_ENDOF_WORD , VI_MOVE_MODE    },
{ "f"             , ACTION_FIND_NEXT_CHAR  , VI_MOVE_MODE    },
{ "F"             , ACTION_FIND_PREV_CHAR  , VI_MOVE_MODE    },
{ "G"             , ACTION_GOTO_ENTRY      , VI_COMMAND_MODE },
{ "h"             , ACTION_GO_LEFT         , VI_MOVE_MODE    },
{ "H"             , ACTION_GOTO_TOP        , VI_COMMAND_MODE },
{ "i"             , ACTION_VI_INSERT       , VI_COMMAND_MODE },
{ "I"             , ACTION_VI_INSERT_BOL   , VI_COMMAND_MODE },
{ "j"             , ACTION_NEXT_ITEM       , VI_COMMAND_MODE },
{ "k"             , ACTION_PREV_ITEM       , VI_COMMAND_MODE },
{ " "             , ACTION_GO_RIGHT        , VI_MOVE_MODE    },
{ "l"             , ACTION_GO_RIGHT        , VI_MOVE_MODE    },
{ "L"             , ACTION_GOTO_BOTTOM     , VI_COMMAND_MODE },
{ "m"             , ACTION_VI_MARK         , VI_COMMAND_MODE },
{ "M"             , ACTION_GOTO_MIDDLE     , VI_COMMAND_MODE },
{ "n"             , ACTION_SEARCH_NEXT     , VI_COMMAND_MODE },
{ "N"             , ACTION_SEARCH_NEXT_REV , VI_COMMAND_MODE },
{ "p"             , ACTION_PASTE_APPEND    , VI_COMMAND_MODE },
{ "P"             , ACTION_PASTE_INSERT    , VI_COMMAND_MODE },
{ "R"             , ACTION_VI_REPLACE      , VI_COMMAND_MODE },
{ "r"             , ACTION_VI_REPLACE_CHAR , VI_COMMAND_MODE },
{ "s"             , ACTION_VI_CHANGE_CHAR  , VI_COMMAND_MODE },
{ "S"             , ACTION_VI_CHANGE_LINE  , VI_COMMAND_MODE },
{ "t"             , ACTION_TILL_NEXT_CHAR  , VI_MOVE_MODE    },
{ "T"             , ACTION_TILL_PREV_CHAR  , VI_MOVE_MODE    },
{ "u"             , ACTION_UNDO            , VI_COMMAND_MODE },
{ "U"             , ACTION_REVERT          , VI_COMMAND_MODE },
{ "v"             , ACTION_VISUAL_MARK     , VI_MOVE_MODE    },
{ "w"             , ACTION_VI_GOTO_NEXT_WORD,VI_MOVE_MODE    },
{ "x"             , ACTION_DELETE_CHAR     , VI_COMMAND_MODE },
{ "X"             , ACTION_BACKSPACE       , VI_COMMAND_MODE },
{ "y0"            , ACTION_YANK_BOL        , VI_COMMAND_MODE },
{ "yy"            , ACTION_YANK_LINE       , VI_COMMAND_MODE },
{ "yw"            , ACTION_YANK_WORD       , VI_COMMAND_MODE },
{ "y$"            , ACTION_YANK_EOL        , VI_COMMAND_MODE },
{ "Y"             , ACTION_YANK_EOL        , VI_COMMAND_MODE },
{ "$"             , ACTION_GOTO_EOL        , VI_MOVE_MODE    },
{ "0"             , ACTION_GOTO_BOL        , VI_MOVE_MODE    },
{ "^"             , ACTION_GOTO_BOL        , VI_MOVE_MODE    },
{ "~"             , ACTION_FLIP_CASE       , VI_COMMAND_MODE },
{ "."             , ACTION_REPEAT_LAST     , VI_COMMAND_MODE },
{ ";"             , ACTION_REPEAT_FIND_NEXT, VI_MOVE_MODE    },
{ ","             , ACTION_REPEAT_FIND_PREV, VI_MOVE_MODE    },
{ "%"             , ACTION_MATCH_PAREN     , VI_MOVE_MODE    },
{ "/"             , ACTION_SEARCH_HISTORY  , VI_COMMAND_MODE },
{ "?"             , ACTION_SEARCH_REVERSE  , VI_COMMAND_MODE },
{ "`"             , ACTION_VI_GOTO_MARK    , VI_COMMAND_MODE },
{ "'"             , ACTION_VI_GOTO_MARK    , VI_COMMAND_MODE },
{ "="             , ACTION_TAB_COMPLETE    , VI_COMMAND_MODE },
{ "+"             , ACTION_INCR_NUMBER     , VI_COMMAND_MODE },
{ KEY_CTRL_H      , ACTION_GO_LEFT         , VI_MOVE_MODE    },
{ "-"             , ACTION_DECR_NUMBER     , VI_COMMAND_MODE },
{ KEY_BACKSPACE   , ACTION_GO_LEFT         , VI_MOVE_MODE    },
{ INSERT_FUNCTION , ACTION_VI_INSERT       , VI_COMMAND_MODE },
{ "1"             , ACTION_VI_REPEAT_DIGIT , VI_COMMAND_MODE },
{ "2"             , ACTION_VI_REPEAT_DIGIT , VI_COMMAND_MODE },
{ "3"             , ACTION_VI_REPEAT_DIGIT , VI_COMMAND_MODE },
{ "4"             , ACTION_VI_REPEAT_DIGIT , VI_COMMAND_MODE },
{ "5"             , ACTION_VI_REPEAT_DIGIT , VI_COMMAND_MODE },
{ "6"             , ACTION_VI_REPEAT_DIGIT , VI_COMMAND_MODE },
{ "7"             , ACTION_VI_REPEAT_DIGIT , VI_COMMAND_MODE },
{ "8"             , ACTION_VI_REPEAT_DIGIT , VI_COMMAND_MODE },
{ "9"             , ACTION_VI_REPEAT_DIGIT , VI_COMMAND_MODE },
{ META_0          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       },
{ META_1          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       },
{ META_2          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       },
{ META_3          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       },
{ META_4          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       },
{ META_5          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       },
{ META_6          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       },
{ META_7          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       },
{ META_8          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       },
{ META_9          , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       },
{ META_MINUS      , ACTION_EMACS_ARG_DIGIT , MOVE_MODE       },
{ 0               , ACTION_BELL            , VI_COMMAND_MODE }, /* default */
/* visual mode */
{ "c"             , ACTION_VISUAL_CHANGE   , VISUAL_MODE     },
{ "s"             , ACTION_VISUAL_CHANGE   , VISUAL_MODE     },
{ KEY_CTRL_D      , ACTION_VISUAL_DELETE   , VISUAL_MODE     },
{ "d"             , ACTION_VISUAL_DELETE   , VISUAL_MODE     },
{ "x"             , ACTION_VISUAL_DELETE   , VISUAL_MODE     },
{ "y"             , ACTION_VISUAL_YANK     , VISUAL_MODE     },
{ 0               , ACTION_VISUAL_MARK     , VISUAL_MODE     }, /* default */
/* show commands */
{ F1_FUNCTION     , ACTION_SHOW_KEYS       , MOVE_MODE       },
{ F2_FUNCTION     , ACTION_SHOW_HISTORY    , EVILS_MODE      },
{ F3_FUNCTION     , ACTION_SHOW_FILES      , EVIL_MODE       },
{ F4_FUNCTION     , ACTION_SHOW_TREE       , EVIL_MODE       },
{ F5_FUNCTION     , ACTION_SHOW_EXES       , EVIL_MODE       },
{ F6_FUNCTION     , ACTION_SHOW_DIRS       , EVIL_MODE       },
{ F7_FUNCTION     , ACTION_SHOW_UNDO       , MOVE_MODE       },
{ F8_FUNCTION     , ACTION_SHOW_YANK       , MOVE_MODE       },
{ F9_FUNCTION     , ACTION_SHOW_FZF        , EVIL_MODE       },
{ F10_FUNCTION    , ACTION_SHOW_VARS       , EVIL_MODE       },
{ F11_FUNCTION    , ACTION_REDRAW_LINE     , MOVE_MODE       },
{ F12_FUNCTION    , ACTION_SHOW_CLEAR      , MOVE_MODE       },

{ META_CTRL_D     , ACTION_SHOW_DIRS       , EVIL_MODE       },
{ META_CTRL_E     , ACTION_SHOW_EXES       , EVIL_MODE       },
{ META_CTRL_F     , ACTION_SHOW_FILES      , EVIL_MODE       },
{ META_CTRL_H     , ACTION_SHOW_HISTORY    , EVILS_MODE      },
{ META_CTRL_K     , ACTION_SHOW_KEYS       , MOVE_MODE       },
{ META_CTRL_L     , ACTION_SHOW_CLEAR      , MOVE_MODE       },
{ META_CTRL_P     , ACTION_SHOW_YANK       , MOVE_MODE       },
{ META_CTRL_U     , ACTION_SHOW_UNDO       , MOVE_MODE       },
{ META_CTRL_V     , ACTION_SHOW_VARS       , EVIL_MODE       },
{ KEY_CX_ACTION   , ACTION_ACTION          , EVIL_MODE       }
};

size_t lc_default_key_recipe_size = sizeof( lc_default_key_recipe ) /
                                    sizeof( lc_default_key_recipe[ 0 ] );

int
lc_action_options( KeyAction action )
{
  switch ( action ) {
    case ACTION_PENDING:            return 0;
    case ACTION_INSERT_CHAR:        return 0; /* char */
    case ACTION_FINISH_LINE:        return 0; /* <enter> */
    case ACTION_OPER_AND_NEXT:      return 0; /* ctrl-o */
    case ACTION_GOTO_BOL:           return 0; /* ctrl-a */
    case ACTION_INTERRUPT:          return 0; /* ctrl-c */
    case ACTION_SUSPEND:            return 0; /* ctrl-z */
    case ACTION_DELETE_CHAR:        return OPT_UNDO | OPT_REPEAT; /* ctrl-d */
    case ACTION_BACKSPACE:          return OPT_UNDO | OPT_REPEAT; /* <bs> */
    case ACTION_GOTO_EOL:           return 0; /* ctrl-e, $ */
    case ACTION_GO_LEFT:            return 0; /* ctrl-b, <- */
    case ACTION_GO_RIGHT:           return 0; /* ctrl-f, -> */
    case ACTION_GOTO_PREV_WORD:     return 0; /* meta-b */
    case ACTION_VI_GOTO_PREV_WORD:  return 0; /* b */
    case ACTION_GOTO_NEXT_WORD:     return 0; /* meta-f */
    case ACTION_VI_GOTO_NEXT_WORD:  return 0; /* w */
    case ACTION_GOTO_ENDOF_WORD:    return 0; /* e */
    case ACTION_FIND_NEXT_CHAR:     return OPT_VI_CHAR_ARG; /* f (char) */
    case ACTION_FIND_PREV_CHAR:     return OPT_VI_CHAR_ARG; /* F (char) */
    case ACTION_TILL_NEXT_CHAR:     return OPT_VI_CHAR_ARG; /* t (char) */
    case ACTION_TILL_PREV_CHAR:     return OPT_VI_CHAR_ARG; /* T (char) */
    case ACTION_REPEAT_FIND_NEXT:   return 0; /* ; */
    case ACTION_REPEAT_FIND_PREV:   return 0; /* , */
    case ACTION_TAB_COMPLETE:       return 0; /* tab */
    case ACTION_TAB_REVERSE:        return 0; /* shift-tab */
    case ACTION_PREV_ITEM:          return 0; /* ctrl-p */
    case ACTION_NEXT_ITEM:          return 0; /* ctrl-n */
    case ACTION_SEARCH_HISTORY:     return 0; /* / */
    case ACTION_SEARCH_NEXT:        return 0; /* n */
    case ACTION_SEARCH_NEXT_REV:    return 0; /* N */
    case ACTION_SEARCH_REVERSE:     return 0; /* ? */
    case ACTION_SEARCH_INLINE:      return 0; /* meta-s */
    case ACTION_HISTORY_COMPLETE:   return 0; /* meta-p */
    case ACTION_SHOW_TREE:          return 0; /* meta-/ */
    case ACTION_SHOW_FZF:           return 0;
    case ACTION_CANCEL_SEARCH:      return 0; /* esc */
    case ACTION_TRANSPOSE:          return OPT_UNDO | OPT_REPEAT; /* ctrl-t */
    case ACTION_TRANSPOSE_WORDS:    return OPT_UNDO | OPT_REPEAT; /* meta-t */
    case ACTION_CAPITALIZE_WORD:    return OPT_UNDO | OPT_REPEAT; /* meta-c */
    case ACTION_LOWERCASE_WORD:     return OPT_UNDO | OPT_REPEAT; /* meta-l */
    case ACTION_UPPERCASE_WORD:     return OPT_UNDO | OPT_REPEAT; /* meta-u */
    case ACTION_ERASE_BOL:          return OPT_UNDO | OPT_REPEAT; /* ctrl-u */
    case ACTION_ERASE_EOL:          return OPT_UNDO | OPT_REPEAT; /* D, ctrl-k*/
    case ACTION_ERASE_LINE:         return OPT_UNDO | OPT_REPEAT; /* dd */
    case ACTION_ERASE_PREV_WORD:    return OPT_UNDO | OPT_REPEAT; /* ctrl-w */
    case ACTION_ERASE_NEXT_WORD:    return OPT_UNDO | OPT_REPEAT; /* meta-d */
    case ACTION_VI_ERASE_NEXT_WORD: return OPT_UNDO | OPT_REPEAT; /* dw */
    case ACTION_FLIP_CASE:          return OPT_UNDO | OPT_REPEAT; /* ~ */
    case ACTION_MATCH_PAREN:        return 0; /* % */
    case ACTION_INCR_NUMBER:        return OPT_UNDO | OPT_REPEAT; /* + */
    case ACTION_DECR_NUMBER:        return OPT_UNDO | OPT_REPEAT; /* - */
    case ACTION_REDRAW_LINE:        return 0; /* ctrl-l */
    case ACTION_REPLACE_MODE:       return 0; /* R, <insert> */
    case ACTION_UNDO:               return 0; /* u */
    case ACTION_REVERT:             return 0; /* U */
    case ACTION_REDO:               return 0; /* ctrl-r */
    case ACTION_PASTE_APPEND:       return OPT_UNDO | OPT_REPEAT; /* p */
    case ACTION_PASTE_INSERT:       return OPT_UNDO | OPT_REPEAT; /* P */
    case ACTION_QUOTED_INSERT:      return 0; /* ctrl-q */
    case ACTION_YANK_BOL:           return 0; /* y0 */
    case ACTION_YANK_LINE:          return 0; /* yy */
    case ACTION_YANK_WORD:          return 0; /* yw */
    case ACTION_YANK_EOL:           return 0; /* Y */
    case ACTION_YANK_LAST_ARG:      return 0; /* meta-., meta-_ */
    case ACTION_YANK_NTH_ARG:       return 0; /* meta-ctrl-y */
    case ACTION_YANK_POP:           return OPT_UNDO | OPT_REPEAT; /* meta-y */
    case ACTION_VI_MODE:            return 0; /* meta-m */
    case ACTION_VI_ESC:             return 0; /* esc */
    case ACTION_VI_INSERT:          return OPT_UNDO; /* i */
    case ACTION_VI_INSERT_BOL:      return OPT_UNDO; /* I */
    case ACTION_VI_REPLACE:         return OPT_UNDO; /* R */
    case ACTION_VI_REPLACE_CHAR:    return OPT_UNDO | OPT_REPEAT |
                                           OPT_VI_CHAR_ARG; /*r(char)*/
    case ACTION_VI_APPEND:          return OPT_UNDO; /* a */
    case ACTION_VI_APPEND_EOL:      return OPT_UNDO; /* A */
    case ACTION_VI_CHANGE_BOL:      return OPT_UNDO; /* c0 */
    case ACTION_VI_CHANGE_LINE:     return OPT_UNDO; /* C, cc */
    case ACTION_VI_CHANGE_WORD:     return OPT_UNDO; /* cw */
    case ACTION_VI_CHANGE_CHAR:     return OPT_UNDO | OPT_REPEAT; /* s */
    case ACTION_VI_CHANGE_EOL:      return OPT_UNDO; /* c$ */
    case ACTION_INSERT:             return OPT_UNDO;
    case ACTION_INSERT_BOL:         return OPT_UNDO;
    case ACTION_APPEND:             return OPT_UNDO;
    case ACTION_APPEND_EOL:         return OPT_UNDO;
    case ACTION_REPEAT_LAST:        return 0;        /* . */
    case ACTION_VI_MARK:            return OPT_VI_CHAR_ARG; /* m (char) */
    case ACTION_VI_GOTO_MARK:       return OPT_VI_CHAR_ARG; /* ` (char) */
    case ACTION_VISUAL_MARK:        return 0;        /* v */
    case ACTION_VISUAL_CHANGE:      return OPT_UNDO; /* v (move) c */
    case ACTION_VISUAL_DELETE:      return OPT_UNDO; /* v (move) d */
    case ACTION_VISUAL_YANK:        return 0;        /* v (move) y */
    case ACTION_VI_REPEAT_DIGIT:    return 0;  /* 1-9 */
    case ACTION_EMACS_ARG_DIGIT:    return 0;  /* meta-0 .. meta-9, meta-'-' */
    case ACTION_SHOW_DIRS:          return 0;  /* meta-ctrl-d */
    case ACTION_SHOW_EXES:          return 0;  /* meta-ctrl-e */
    case ACTION_SHOW_FILES:         return 0;  /* meta-ctrl-f */
    case ACTION_SHOW_HISTORY:       return 0;  /* meta-ctrl-h */
    case ACTION_SHOW_KEYS:          return 0;  /* meta-ctrl-k */
    case ACTION_SHOW_CLEAR:         return 0;  /* meta-ctrl-c */
    case ACTION_SHOW_UNDO:          return 0;  /* meta-ctrl-u */
    case ACTION_SHOW_VARS:          return 0;  /* meta-ctrl-v */
    case ACTION_SHOW_YANK:          return 0;  /* meta-ctrl-y */
    case ACTION_SHOW_PREV_PAGE:     return 0;  /* pgup */
    case ACTION_SHOW_NEXT_PAGE:     return 0;  /* pgdn */
    case ACTION_GOTO_FIRST_ENTRY:   return 0;  /* meta-< */
    case ACTION_GOTO_LAST_ENTRY:    return 0;  /* meta-> */
    case ACTION_GOTO_ENTRY:         return 0;  /* G */
    case ACTION_GOTO_TOP:           return 0;  /* H */
    case ACTION_GOTO_MIDDLE:        return 0;  /* M */
    case ACTION_GOTO_BOTTOM:        return 0;  /* L */
    case ACTION_DECR_SHOW:          return 0;  /* meta-( */
    case ACTION_INCR_SHOW:          return 0;  /* meta-) */
    case ACTION_MACRO:              return 0;
    case ACTION_ACTION:             return OPT_VI_CHAR_ARG;
    case ACTION_BELL:               return 0;
    case ACTION_PUTBACK:            return 0;
  }
  return 0;
}

