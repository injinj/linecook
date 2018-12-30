#ifndef __linecook__keycook_h__
#define __linecook__keycook_h__

#ifdef __cplusplus
extern "C" {
#endif

typedef char KeyCode[ 8 ]; /* these should be null terminated */

typedef enum KeyAction_e { /* current storage of this is uint8_t, 255 max */
  ACTION_PENDING         = 0, /* arrow/function key pending or initial state */
  ACTION_INSERT_CHAR        , /* normal insert char */
  ACTION_FINISH_LINE        , /* enter */
  ACTION_OPER_AND_NEXT      , /* ctrl-o (emacs) */
  ACTION_GOTO_BOL           , /* ctrl-a (emacs) '0', '^' (vi), HOME */
  ACTION_INTERRUPT          , /* ctrl-c */
  ACTION_SUSPEND            , /* ctrl-z */
  ACTION_DELETE_CHAR        , /* ctrl-d (emacs), 'x' (vi), DELETE */
  ACTION_BACKSPACE          , /* ctrl-h, backspace */
  ACTION_GOTO_EOL           , /* ctrl-e (emacs), '$' (vi), END */
  ACTION_GO_LEFT            , /* ctrl-b, arrow left */
  ACTION_GO_RIGHT           , /* ctrl-f, arrow right */
  ACTION_GOTO_PREV_WORD     , /* meta-b, (emacs) */
  ACTION_VI_GOTO_PREV_WORD  , /* 'b' (vi) */
  ACTION_GOTO_NEXT_WORD     , /* meta-f (emacs) */
  ACTION_VI_GOTO_NEXT_WORD  , /* 'w' (vi) */
  ACTION_GOTO_ENDOF_WORD    , /* 'e' (vi) */
  ACTION_FIND_NEXT_CHAR     , /* 'f' (vi) find next char entered */
  ACTION_FIND_PREV_CHAR     , /* 'F' (vi) find prev char entered */
  ACTION_TILL_NEXT_CHAR     , /* 't' (vi) find pos before next char entered */
  ACTION_TILL_PREV_CHAR     , /* 'T' (vi) find pos before prev char entered */
  ACTION_REPEAT_FIND_NEXT   , /* ';' (vi) repeat find next */
  ACTION_REPEAT_FIND_PREV   , /* ',' (vi) repeat find prev */
  ACTION_TAB_COMPLETE       , /* tab */
  ACTION_TAB_REVERSE        , /* shift-tab */
  ACTION_PREV_ITEM          , /* ctrl-p (emacs), 'k' (vi) */
  ACTION_NEXT_ITEM          , /* ctrl-n (emacs), 'j' (vi)  */
  ACTION_SEARCH_HISTORY     , /* '/' (vi)  */
  ACTION_SEARCH_NEXT        , /* 'n' (vi)  */
  ACTION_SEARCH_NEXT_REV    , /* 'N' (vi)  */
  ACTION_SEARCH_REVERSE     , /* '?' (vi)  */
  ACTION_SEARCH_INLINE      , /* mata-s */
  ACTION_HISTORY_COMPLETE   , /* meta-p */
  ACTION_SEARCH_COMPLETE    , /* meta-/ */
  ACTION_CANCEL_SEARCH      , /* esc */
  ACTION_TRANSPOSE          , /* ctrl-t */
  ACTION_TRANSPOSE_WORDS    , /* meta-t */
  ACTION_CAPITALIZE_WORD    , /* meta-c (emacs) */
  ACTION_LOWERCASE_WORD     , /* meta-l (emacs) */
  ACTION_UPPERCASE_WORD     , /* meta-u (emacs) */
  ACTION_ERASE_BOL          , /* ctrl-u */
  ACTION_ERASE_EOL          , /* ctrl-k (emacs), 'D' (vi) */
  ACTION_ERASE_LINE         , /* 'dd' (vi) */
  ACTION_ERASE_PREV_WORD    , /* ctrl-w */
  ACTION_ERASE_NEXT_WORD    , /* meta-d (emacs) word without space */
  ACTION_VI_ERASE_NEXT_WORD , /* 'dw' (vi) word with space */
  ACTION_FLIP_CASE          , /* '~' (vi) */
  ACTION_MATCH_PAREN        , /* '%' (vi) */
  ACTION_INCR_NUMBER        , /* ctrl-a (vi) */
  ACTION_DECR_NUMBER        , /* ctrl-x (vi) */
  ACTION_REDRAW_LINE        , /* ctrl-l */
  ACTION_REPLACE_MODE       , /* toggle insert mode / replace mode */
  ACTION_UNDO               , /* ctrl-_ (emacs), 'u' (vi) */
  ACTION_REVERT             , /* 'U' (vi) */
  ACTION_REDO               , /* ctrl-r (vi) (is reverse-i search in bash) */
  ACTION_PASTE_APPEND       , /* 'p' (vi) */
  ACTION_PASTE_INSERT       , /* 'P' (vi) */
  ACTION_QUOTED_INSERT      , /* ctrl-q (emacs) */
  ACTION_YANK_BOL           , /* 'y0' (vi) */
  ACTION_YANK_LINE          , /* 'yy' (vi) */
  ACTION_YANK_WORD          , /* 'yw' (vi) */
  ACTION_YANK_EOL           , /* 'Y' (vi) */
  ACTION_YANK_LAST_ARG      , /* meta-., meta-_ (emacs) */
  ACTION_YANK_NTH_ARG       , /* meta-ctrl-y (emacs) */
  ACTION_YANK_POP           , /* meta-y (emacs) */
  ACTION_VI_MODE            , /* esc ctrl-m (emacs) */
  ACTION_VI_ESC             , /* esc (vi) */
  ACTION_VI_INSERT          , /* 'i' (vi) */
  ACTION_VI_INSERT_BOL      , /* 'I' (vi) */
  ACTION_VI_REPLACE         , /* 'R' (vi) */
  ACTION_VI_REPLACE_CHAR    , /* 'r' (vi) */
  ACTION_VI_APPEND          , /* 'a' (vi) */
  ACTION_VI_APPEND_EOL      , /* 'A' (vi) */
  ACTION_VI_CHANGE_BOL      , /* 'c0' (vi) */
  ACTION_VI_CHANGE_LINE     , /* 'C', 'cc' (vi) */
  ACTION_VI_CHANGE_WORD     , /* 'cw' (vi) */
  ACTION_VI_CHANGE_CHAR     , /* 's' (vi) */
  ACTION_VI_CHANGE_EOL      , /* 'c$' (vi) */
  ACTION_INSERT             , /* generic actions emacs or vi work */
  ACTION_INSERT_BOL         ,
  ACTION_APPEND             ,
  ACTION_APPEND_EOL         ,
  ACTION_REPEAT_LAST        , /* '.' (vi) */
  ACTION_VI_MARK            , /* 'm' (vi) */
  ACTION_VI_GOTO_MARK       , /* '`' (vi) */
  ACTION_VISUAL_MARK        , /* 'v' (vi) */
  ACTION_VISUAL_CHANGE      , /* 'vc' (vi) */
  ACTION_VISUAL_DELETE      , /* 'vd' (vi) */
  ACTION_VISUAL_YANK        , /* 'vy' (vi) */
  ACTION_VI_REPEAT_DIGIT    , /* '1' -> '9' (vi) */
  ACTION_EMACS_ARG_DIGIT    , /* meta '-' and '0' -> '9' (emacs) */
  ACTION_SHOW_DIRS          , /* meta-ctrl-d completion dirs */
  ACTION_SHOW_EXES          , /* meta-ctrl-e completion exes */
  ACTION_SHOW_FILES         , /* meta-ctrl-f completion files */
  ACTION_SHOW_HISTORY       , /* meta-ctrl-h show history */
  ACTION_SHOW_KEYS          , /* meta-ctrl-k keys */
  ACTION_SHOW_CLEAR         , /* meta-ctrl-c close show */
  ACTION_SHOW_UNDO          , /* meta-ctrl-u show undo buffers */
  ACTION_SHOW_VARS          , /* meta-ctrl-v completion vars */
  ACTION_SHOW_YANK          , /* meta-ctrl-y show yank buffer */
  ACTION_SHOW_PREV_PAGE     , /* pgup */
  ACTION_SHOW_NEXT_PAGE     , /* pgdn */
  ACTION_GOTO_FIRST_ENTRY   , /* meta-< */
  ACTION_GOTO_LAST_ENTRY    , /* meta-> */
  ACTION_GOTO_ENTRY         , /* 'G' */
  ACTION_GOTO_TOP           , /* 'H' */
  ACTION_GOTO_MIDDLE        , /* 'M' */
  ACTION_GOTO_BOTTOM        , /* 'L' */
  ACTION_DECR_SHOW          , /* meta-( */
  ACTION_INCR_SHOW          , /* meta-) */
  ACTION_MACRO              , /* generic macro */
  ACTION_ACTION             , /* ctrl-x ctrl-a <a> */
  ACTION_BELL               , /* function not assigned */
  ACTION_PUTBACK              /* function not matched, interpet char */
} KeyAction;

typedef enum LineMode_e {
  REPLACE_MODE    = 1, /* Toggle with insert function key */
  VI_INSERT_MODE  = 2, /* Vi insert mode */
  VI_COMMAND_MODE = 4, /* Vi command, from emacs: esc ctrl-m / meta-ctrl-m */
  EMACS_MODE      = 8, /* None of that modal vi stuff */
  SEARCH_MODE     = 16, /* Entering search term when in insert/emacs mode */
  VISUAL_MODE     = 32, /* Visually highlight selection */
  EVIL_MODE       = 2 | 4 | 8,  /* Emacs or Vi mode, but not search, ! visual */
  EVILS_MODE      = 2 | 4 | 8 | 16, /* EVIL mode and search, not visual */
  VI_EMACS_MODE   = 2 | 8 | 16, /* Emacs + Vi insert mode */
  MOVE_MODE       = 2 | 4 | 8 | 16 | 32, /* Navigation modes */
  VI_MOVE_MODE    = 4 | 32  /* Vi Navigation in command mode */
} LineMode;

typedef enum LineOption_e {
  OPT_UNDO        = 1,
  OPT_REPEAT      = 2,
  OPT_VI_CHAR_ARG = 4  /* vi options that have a char arg: r, f, t */
} LineOption;

typedef struct KeyRecipe_s {
  const char * char_sequence; /* if null, is default action for valid_mode */
  KeyAction    action;        /* what to do with key sequence */
  uint8_t      valid_mode;    /* EMACS / VI_INSERT / VI_COMMAND */
} KeyRecipe;

int lc_key_to_name( const char *key,  char *name );
const char *lc_action_to_name( KeyAction action );
const char *lc_action_to_descr( KeyAction action );
int lc_action_options( KeyAction action );
unsigned int lc_hash_action_name( const char *s );
KeyAction lc_string_to_action( const char *s );
KeyAction lc_hash_to_action( unsigned int h );

extern KeyRecipe lc_default_key_recipe[];
extern size_t    lc_default_key_recipe_size;

extern KeyCode    KEY_CTRL_A        , /* goto beginning of line */
                  KEY_CTRL_B        , /* go back one char */
                  KEY_CTRL_C        , /* interrupt */
                  KEY_CTRL_D        , /* delete char under cursor */
                  KEY_CTRL_E        , /* goto end of line */
                  KEY_CTRL_F        , /* go fwd one char */
                  KEY_CTRL_G        , /* unused (bell) */
                  KEY_CTRL_H        , /* delete char to left */
                  KEY_TAB           , /* completion */
                  KEY_CTRL_J        , /* enter key */
                  KEY_CTRL_K        , /* erase to end of line */
                  KEY_CTRL_L        , /* redraw line */
                  KEY_ENTER         , /* finish line */
                  KEY_CTRL_N        , /* next history item */
                  KEY_CTRL_O        , /* operate and next */
                  KEY_CTRL_P        , /* previous history item */
                  KEY_CTRL_Q        , /* quoted insert */
                  KEY_CTRL_R        , /* history search reverse, redo undo */
                  KEY_CTRL_S        , /* history search forward */
                  KEY_CTRL_T        , /* transpose chars */
                  KEY_CTRL_U        , /* erase to beginning of line */
                  KEY_CTRL_V        , /* visual mark */
                  KEY_CTRL_W        , /* erase previous word */
                  KEY_CTRL_X        , /* emacs CX below -- don't use */
                  KEY_CTRL_Y        , /* paste yank buffer */
                  KEY_CTRL_Z        , /* suspend */
                  KEY_ESC           , /* go to vi command mode */
                  KEY_CTRL_RBR      , /* find char  */
                  KEY_CTRL__        , /* undo previous edit */
                  KEY_CX_CTRL_R     , /* ctrl-x ctrl-r */
                  KEY_CX_CTRL_U     , /* ctrl-x ctrl-u */
                  KEY_CX_ACTION     , /* ctrl-x ctrl-a */
                  KEY_CX_BACKSPACE  , /* ctrl-x backspace */
                  KEY_BACKSPACE     , /* erase previous char */
                  META_0            , /* emacs argument digits */
                  META_1            ,
                  META_2            ,
                  META_3            ,
                  META_4            ,
                  META_5            ,
                  META_6            ,
                  META_7            ,
                  META_8            ,
                  META_9            ,
                  META_MINUS        ,
                  META_A            , /* goto top show entry */
                  META_B            , /* goto previous word */
                  META_C            , /* capitalize word */
                  META_D            , /* erase next word */
                  META_E            , /* unused */
                  META_F            , /* goto next word */
                  META_G            , /* unused */
                  META_H            , /* erase prev word */
                  META_J            , /* show next page (pgdn) */
                  META_K            , /* show prev page (pgup) */
                  META_L            , /* lowercase word */
                  META_N            , /* unused */
                  META_M            , /* flip emacs/vi mode */
                  META_P            , /* complete using history */
                  META_O            , /* unused */
                  META_R            , /* repeat last action */
                  META_S            , /* search inline */
                  META_T            , /* transpose words */
                  META_U            , /* uppercase word */
                  META_V            , /* unused */
                  META_W            , /* unused */
                  META_X            , /* unused */
                  META_Y            , /* redo */
                  META_Z            , /* goto bottom show entry */
                  META_DOT          , /* yank last arg in history */
                  META_SLASH        , /* search complete */
                  META_LT           , /* goto first show entry */
                  META_GT           , /* goto last show entry */
                  META_LPAREN       , /* increase show size */
                  META_RPAREN       , /* decrease show size */
                  META__            , /* yank last arg */
                  META_CTRL_A       , /* unused */
                  META_CTRL_B       , /* unused */
                  META_CTRL_C       , /* unused */
                  META_CTRL_D       , /* complete dirs */
                  META_CTRL_E       , /* complete exes */
                  META_CTRL_F       , /* complete files */
                  META_CTRL_H       , /* show history */
                  META_TAB          , /* window system uses */
                  META_CTRL_J       , /* unused */
                  META_CTRL_K       , /* show keybindings */
                  META_CTRL_L       , /* clear show */
                  META_CTRL_M       , /* unsused */
                  META_CTRL_N       , /* unused */
                  META_CTRL_O       , /* unused */
                  META_CTRL_P       , /* show yank kill ring */
                  META_CTRL_Q       , /* unused */
                  META_CTRL_R       , /* unused */
                  META_CTRL_S       , /* unused */
                  META_CTRL_T       , /* unused */
                  META_CTRL_U       , /* show undo lines */
                  META_CTRL_V       , /* complete env vars */
                  META_CTRL_W       , /* unused */
                  META_CTRL_X       , /* unused */
                  META_CTRL_Y       , /* yank nth arg */
                  META_CTRL_Z       , /* unused */
                  META_CTRL_RBR     , /* find prev char */
                  META_BACKSPACE    , /* erase previous word */
                  ARROW_LEFT        , /* go prev char */
                  ARROW_RIGHT       , /* go next char */
  /* alt */       ARROW_LEFT2       , /* go prev char */
                  ARROW_RIGHT2      , /* go next char */
                  CTRL_ARROW_LEFT   , /* go prev word */
                  CTRL_ARROW_RIGHT  , /* go next word */
                  META_ARROW_LEFT   , /* go prev word */
                  META_ARROW_RIGHT  , /* go next word */
                  ARROW_UP          , /* prev history item */
                  ARROW_DOWN        , /* next history item */
                  ARROW_UP2         , /* prev history item */
                  ARROW_DOWN2       , /* next history item */
                  INSERT_FUNCTION   , /* toggle insert/replace mode */
                  DELETE_FUNCTION   , /* delete char under cursor */
                  HOME_FUNCTION     , /* goto beginning of line */
                  END_FUNCTION      , /* goto end of line */
                  SHIFT_TAB         , /* complete previous */
                  PGUP_FUNCTION     , /* show prev page */
                  PGDN_FUNCTION     , /* show next page */
                  F1_FUNCTION       , /* show keys */
                  F2_FUNCTION       ,
                  F3_FUNCTION       ,
                  F4_FUNCTION       ,
                  F5_FUNCTION       ,
                  F6_FUNCTION       ,
                  F7_FUNCTION       ,
                  F8_FUNCTION       ,
                  F9_FUNCTION       ,
                  F10_FUNCTION      ,
                  F11_FUNCTION      ,
                  F12_FUNCTION      ; /* show history */

#ifdef __cplusplus
}
#endif

#endif
