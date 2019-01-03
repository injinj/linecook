#ifndef __linecook__linecook_h__
#define __linecook__linecook_h__

#include <linecook/kewb_utf.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LineCook_s LineCook;
typedef struct LineCookInput_s LineCookInput;

/* Status codes, set in line->error, returned from get_line() */
typedef enum LineStatus_e {
  LINE_STATUS_BAD_INPUT  = -9, /* Invalid input (not utf8) */
  LINE_STATUS_BAD_PROMPT = -8, /* Prompt is invalid */
  LINE_STATUS_BAD_CURSOR = -7, /* Cursor position is invalid */
  LINE_STATUS_WR_FAIL    = -6, /* Write returned < 0 */
  LINE_STATUS_RD_FAIL    = -5, /* Read returned < 0 */
  LINE_STATUS_ALLOC_FAIL = -4, /* A realloc() returned null */
  LINE_STATUS_SUSPEND    = -3, /* Ctrl-z read */
  LINE_STATUS_INTERRUPT  = -2, /* Ctrl-c read */
  LINE_STATUS_WR_EAGAIN  = -1, /* Write didn't eat available output */
  LINE_STATUS_RD_EAGAIN  = 0,  /* No new input available, read blocked */
  LINE_STATUS_EXEC       = 1,  /* A new line is ready */
  LINE_STATUS_OK         = 2,  /* Still processing */
  LINE_STATUS_COMPLETE   = 3   /* A completion is requested */
} LineStatus;

#define LINE_ERR_STR( e ) \
  ( e == LINE_STATUS_BAD_INPUT  ? "Bad input" : \
    e == LINE_STATUS_BAD_PROMPT ? "Bad prompt" : \
    e == LINE_STATUS_BAD_CURSOR ? "Bad cursor" : \
    e == LINE_STATUS_WR_FAIL    ? "Write failed" : \
    e == LINE_STATUS_RD_FAIL    ? "Read failed" : \
    e == LINE_STATUS_ALLOC_FAIL ? "Allocation failed" : \
    e == LINE_STATUS_SUSPEND    ? "Suspend" : \
    e == LINE_STATUS_INTERRUPT  ? "Interrupt" : \
    e == LINE_STATUS_WR_EAGAIN  ? "Write blocked" : \
    e == LINE_STATUS_RD_EAGAIN  ? "Read blocked" : \
    e == LINE_STATUS_EXEC       ? "Exec" : \
    e == LINE_STATUS_OK         ? "OK" : \
    e == LINE_STATUS_COMPLETE   ? "Complete" : "Unknown status" )

typedef enum CompleteType_e {
  COMPLETE_ANY   = 0,   /* type not specified */
  COMPLETE_FILES = 'f', /* file or dir */
  COMPLETE_DIRS  = 'd', /* dir only */
  COMPLETE_EXES  = 'e', /* dir or exe, uses $PATH */
  COMPLETE_SCAN  = 's', /* directory tree scan */
  COMPLETE_ENV   = 'v', /* variable */
  COMPLETE_FZF   = 'z'  /* fzf */
} CompleteType;

/* Allocate the state */
LineCook *lc_create_state( int cols,  int lines );
/* Free it */
void lc_release_state( LineCook *state );
/* Set or change the current prompt and secondary prompt (line continuation) */
int lc_set_prompt( LineCook *state,  const char *prompt,  size_t prompt_len,
                   const char *prompt2,  size_t prompt2_len );
/* Set the insert mode cursor */
int lc_set_right_prompt( LineCook *state,
                         const char *ins,    size_t ins_len,
                         const char *cmd,    size_t cmd_len,
                         const char *emacs,  size_t emacs_len,
                         const char *srch,   size_t srch_len,
                         const char *comp,   size_t comp_len,
                         const char *visu,   size_t visu_len,
                         const char *ouch,   size_t ouch_len,
                         const char *sel1,   size_t sel1_len,
                         const char *sel2,   size_t sel2_len );
/* Change the geometry of terminal (on SIGWINCH usually) */
int lc_set_geom( LineCook *state,  int cols,  int lines );
/* Set the word break chars */
void lc_set_word_break( LineCook *state,  const char *brk,  size_t brk_len );
/* Set the completion word break chars */
void lc_set_completion_break( LineCook *state,  const char *brk,
                              size_t brk_len );
/* Set the file name quote chars and quote */
void lc_set_quotables( LineCook *state,  const char *qc,  size_t qc_len,
                       char quote );
int lc_bindkey( LineCook *state,  char *args[],  size_t argc );
/* Read and edit a line, returns Line_STATUS above, if LINE_STATUS_OK, then
 * the line is null terminated in state->line with size state->line_len bytes */
int lc_get_line( LineCook *state );
/* Continue completion after LINE_STATUS_COMPLETE is returned */
int lc_completion_get_line( LineCook *state );
/* Utf8 length of line in this->line, not including null char */
int lc_line_length( LineCook *state );
/* Utf8 copy line in this->line, return length copied, not null terminated */
int lc_line_copy( LineCook *state,  char *out );
/* Utf8 length of line in this->line, currently editing */
int lc_edit_length( LineCook *state );
/* Utf8 copy line in this->line, return length copied, currently editing */
int lc_edit_copy( LineCook *state,  char *out );
/* Utf8 length of completion in this->line[ complete_off ], complete_len */
int lc_complete_term_length( LineCook *state );
/* Utf8 copy completion in this->line[ complete_off ], complete_len */
int lc_complete_term_copy( LineCook *state,  char *out );
/* Get the current completion type */
CompleteType lc_get_complete_type( LineCook *state );
/* Set the current completion type */
void lc_set_complete_type( LineCook *state,  CompleteType ctype );
/* Read a line continuation */
int lc_continue_get_line( LineCook *state );
/* Add line to history */
int lc_add_history( LineCook *state,  const char *line,  size_t len );
/* Compress history database to unique items */
int lc_compress_history( LineCook *state );
/* Add line to completion list */
int lc_add_completion( LineCook *state,  const char *line,  size_t len );
/* If any timers are active, calc timeout */
int lc_max_timeout( LineCook *state,  int time_ms );
/* Clear line and refresh when get_line called again */
void lc_clear_line( LineCook *state );
/* Parse the edited line into arg offsets and lengths for completion */
int lc_get_complete_geom( LineCook *state, int *arg_num,  int *arg_count,
                          int *arg_off,  int *arg_len,  size_t args_size );
/* Callbacks for complete, hints, read/write terminal */
typedef int (* LineCompleteCB )( LineCook *state,  const char *buf,
                                 size_t off,  size_t len );
/*typedef char *(* LineHintsCB )( LineCook *state,  const char *buf,
                                int *color,  int *bold );*/
/* These return -1 on error, 0 on EAGAIN, > 0 when part or all fullfilled */
typedef int (* LineReadCB )( LineCook *state,  void *buf,  size_t len );
typedef int (* LineWriteCB )( LineCook *state,  const void *buf,  size_t len );

/* Source:  https://en.wikipedia.org/wiki/ANSI_escape_code */
/* Defs ripped from linenoise */
/* Code to color terminal -- These are only used in example.c */
#define ANSI_NORMAL               "\033[0m"
#define ANSI_NORMAL_SIZE          sizeof( ANSI_NORMAL ) - 1
#define ANSI_RED                  "\033[91m"
#define ANSI_RED_SIZE             sizeof( ANSI_RED ) - 1
#define ANSI_GREEN                "\033[92m"
#define ANSI_GREEN_SIZE           sizeof( ANSI_GREEN ) - 1
#define ANSI_YELLOW               "\033[93m"
#define ANSI_YELLOW_SIZE          sizeof( ANSI_YELLOW ) - 1
#define ANSI_BLUE                 "\033[94m"
#define ANSI_BLUE_SIZE            sizeof( ANSI_BLUE ) - 1
#define ANSI_MAGENTA              "\033[95m"
#define ANSI_MAGENTA_SIZE         sizeof( ANSI_MAGENTA ) - 1
#define ANSI_CYAN                 "\033[96m"
#define ANSI_CYAN_SIZE            sizeof( ANSI_CYAN ) - 1
/* faint, green */
#define ANSI_DEVOLVE              "\033[2;32m"
#define ANSI_DEVOLVE_SIZE         sizeof( ANSI_DEVOLVE ) - 1
/* bold, reverse, blue */
#define ANSI_VISUAL_SELECT        "\033[1;34;7m"
#define ANSI_VISUAL_SELECT_SIZE   sizeof( ANSI_VISUAL_SELECT ) - 1
/* bold, green */
#define ANSI_HILITE_SELECT        "\033[1;92m"
#define ANSI_HILITE_SELECT_SIZE   sizeof( ANSI_HILITE_SELECT ) - 1

/* ES (Erase Screen)
 *    Sequence: ESC [ n J
 *    Effect: if n is 0 or missing, clear from cursor to end of screen
 *    Effect: if n is 1, clear from cursor up to beginning of screen
 *    Effect: if n is 2, clear entire screen */
#define ANSI_ERASE_END_SCR        "\033[0J"
#define ANSI_ERASE_END_SCR_SIZE   sizeof( ANSI_ERASE_END_SCR ) - 1
#define ANSI_ERASE_BEG_SCR        "\033[1J"
#define ANSI_ERASE_BEG_SCR_SIZE   sizeof( ANSI_ERASE_BEG_SCR ) - 1
#define ANSI_ERASE_SCREEN         "\033[2J"
#define ANSI_ERASE_SCREEN_SIZE    sizeof( ANSI_ERASE_SCREEN ) - 1
/* EL (Erase Line)
 *    Sequence: ESC [ n K
 *    Effect: if n is 0 or missing, clear from cursor to end of line
 *    Effect: if n is 1, clear from beginning of line to cursor
 *    Effect: if n is 2, clear entire line */
#define ANSI_ERASE_EOL            "\033[0K"
#define ANSI_ERASE_EOL_SIZE       sizeof( ANSI_ERASE_EOL ) - 1
#define ANSI_ERASE_BOL            "\033[1K"
#define ANSI_ERASE_BOL_SIZE       sizeof( ANSI_ERASE_BOL ) - 1
#define ANSI_ERASE_LINE           "\033[2K"
#define ANSI_ERASE_LINE_SIZE      sizeof( ANSI_ERASE_LINE ) - 1
/* CUF (CUrsor Forward)
 *    Sequence: ESC [ n C
 *    Effect: moves cursor forward n chars */
#define ANSI_CURSOR_FWD_FMT       "\033[%dC"
#define ANSI_CURSOR_FWD_ONE       "\033[1C"
#define ANSI_CURSOR_FWD_ONE_SIZE  sizeof( ANSI_CURSOR_FWD_ONE ) - 1
/* CUB (CUrsor Backward)
 *    Sequence: ESC [ n D
 *    Effect: moves cursor backward n chars */
#define ANSI_CURSOR_BACK_FMT      "\033[%dD"
#define ANSI_CURSOR_BACK_ONE      "\033[1D"
#define ANSI_CURSOR_BACK_ONE_SIZE sizeof( ANSI_CURSOR_BACK_ONE ) - 1
/* CUU (Cursor Up)
 *    Sequence: ESC [ n A
 *    Effect: moves cursor up of n chars. */
#define ANSI_CURSOR_UP_FMT        "\033[%dA"
#define ANSI_CURSOR_UP_ONE        "\033[1A"
#define ANSI_CURSOR_UP_ONE_SIZE   sizeof( ANSI_CURSOR_UP_ONE ) - 1
/* CUD (Cursor Down)
 *    Sequence: ESC [ n B
 *    Effect: moves cursor down of n chars. */
#define ANSI_CURSOR_DOWN_FMT      "\033[%dB"
#define ANSI_CURSOR_DOWN_ONE      "\033[1B"
#define ANSI_CURSOR_DOWN_ONE_SIZE sizeof( ANSI_CURSOR_DOWN_ONE ) - 1
#if 0
/* Not used at the moment ... */
/* CUP (Cursor position)
 *    Sequence: ESC [ H
 *    Effect: moves the cursor to upper left corner */
#define ANSI_SCREEN_HOME          "\033[H"
#define ANSI_SCREEN_HOME_SIZE     sizeof( ANSI_SCREEN_HOME ) - 1
/* ED (Erase display)
 *    Sequence: ESC [ 2 J
 *    Effect: clear the whole screen */
#define ANSI_SCREEN_ERASE         "\033[J"
#define ANSI_SCREEN_ERASE_SIZE    sizeof( ANSI_SCREEN_ERASE ) - 1
#endif
typedef struct KeyRecipe_s KeyRecipe;

typedef enum ShowMode_e { /* what is in the show window */
  SHOW_NONE    = 0,
  SHOW_UNDO       ,
  SHOW_YANK       ,
  SHOW_HISTORY    ,
  SHOW_COMPLETION ,
  SHOW_KEYS
} ShowMode;

typedef struct LineSaveBuf_s {
  char32_t   * buf;        /* Line stack: <line><trail> ... <line><trail> */
  size_t       off,        /* Line top of stack */
               max,        /* Line max extent */
               idx,        /* Current index, 1 <= idx <= cnt */
               cnt,        /* Line counter, idx <= cnt  */
               buflen,     /* Buffer allocation len */
               first;      /* First node trail index */
} LineSaveBuf;

typedef struct LineMark_s {
  size_t   cursor_off,       /* Cursor offset of mark */
           line_idx;         /* Which line, 0 for current line */
  char32_t mark_c;           /* Vi mark char */
} LineMark;

typedef struct LineKeyMode_s { /* Key transitions for mode: ins, cmd, emacs..*/
  KeyRecipe ** mc;         /* Multikey recipes:  arrows, meta-X, etc */
  size_t       mc_size;    /* Size of mc[] */
  uint8_t      recipe[ 128 ], /* Char to action: <ctrl-a> -> ACTION_GOTO_BOL */
               def,        /* Default recipe, likely bell or insert char */
               mode;       /* Which mode is it */
} LineKeyMode;

typedef struct LinePrompt_s {
  char32_t * prompt;     /* Prompt string */
  size_t     prompt_len, /* Prompt length in bytes */
             prompt_cols;/* Prompt length in number of cols */
} LinePrompt;

enum LeftPrompt_fl {
  P_HAS_HIST       = 1<<0,  /* \# or \! */
  P_HAS_CWD        = 1<<1,  /* \w or \W */
  P_HAS_USER       = 1<<2,  /* \u geteuid() pwent */
  P_HAS_HOST       = 1<<3,  /* \h or \H gethostname() */
  P_HAS_TIME24     = 1<<4,  /* \A localtime() */
  P_HAS_TIME12s    = 1<<5,  /* \T */
  P_HAS_TIME24s    = 1<<6,  /* \t */
  P_HAS_TIME12ampm = 1<<7,  /* \@ */
  P_HAS_ANY_TIME   = (1<<4)|(1<<5)|(1<<6)|(1<<7),
  P_HAS_EUID       = 1<<8,  /* used for \\u and \\$ */
  P_HAS_IPADDR     = 1<<9,  /* \4 ipv4 address derived from hostname */
  P_HAS_DATE       = 1<<10, /* \d Mon Nov 5 */
  P_HAS_SHNAME     = 1<<11, /* \s /proc/self/exe */
  P_HAS_TTYNAME    = 1<<12  /* \l /proc/self/fd/0 */
};

typedef struct LeftPrompt_s {
  size_t           len,        /* Length in bytes of str */
                   buflen,     /* Length of str allocation */
                   cols,       /* Number of cols in the last line of the prompt */
                   lines,      /* Number of <cr><lf> in prompt */
                   line_cols[ 8 ]; /* Number of cols in lines if more than one*/
  char32_t       * str,        /* Formatted prompt, this is written to output */
                 * fmt,        /* Unformatted prompt string */
                 * fmt2,       /* Secondary prompt */
                 * host,       /* Hostname cache        \h or \H */
                 * user,       /* Username cache        \u */
                 * home,       /* Home directory cache */
                 * cwd,        /* Workding directory    \w or \W */
                 * date;       /* Date formatted        \d */
  char           * shname,     /* Shell name            \s */
                 * ttyname;    /* TTY name              \l */
  time_t           cur_time;   /* Current time, controls update frequency */
  char             time[ 16 ]; /* Time formatted        \t, \T, \@, \A */
  size_t           cur_hist;   /* Current history # */
  char             hist[ 8 ],  /* Hist formatted        \# or \! */
                   vers[ 8 ],  /* Version of program    \v or \V */
                   ipaddr[ 16 ]; /* Ipaddr of hostname  \4 */
  uint32_t         euid,       /* Euid from geteuid() */
                   flags,      /* Fmt string requires (LeftPrompt_fl bits) */
                   flags_mask; /* Mask out some formats when term geom is small */
  uint8_t          is_continue,/* Prompt is using fmt2 */
                   pad_cols;   /* Prompt has padding to align dbl char edge */
  size_t           fmt_len,    /* Length of fmt string */
                   fmt2_len,   /* length of fmt2 string */
                   host_off,   /* Off of first '.' in host */
                   host_len,   /* Len of host */
                   user_len,   /* Len of user */
                   home_len,   /* Len of home */
                   cwd_len,    /* Len of cwd */
                   shname_len, /* Len of shname */
                   ttyname_len,/* Len of ttyname */
                   dir_off,    /* Offset into cwd where cur directory starts */
                   time_len,   /* Len of time */
                   date_len,   /* Len of date */
                   hist_len,   /* Len of hist */
                   vers_len,   /* Len of vers */
                   ipaddr_len, /* Len of ipaddr */
                   time_off,   /* Offset of time for ticking */
                   date_off,   /* Offset of date for ticking */
                   hist_off;   /* Offset of hist for ticking */
} LeftPrompt;

typedef enum ScreenClass_e {
  SCR_CHAR     = 0,
  SCR_CTRL     = 1,
  SCR_ESC      = 2,
  SCR_DEL      = 3,
  SCR_VT100_1  = 4,
  SCR_VT100_2  = 5,
  SCR_TERMINAL = 6,
  SCR_COLOR    = 7
} ScreenClass;

#ifndef __linecook__keycook_h__
#include <linecook/keycook.h>
#endif

struct LineCookInput_s {
  /* Mode is Vi Insert, Emacs, or Vi Command + with other flags */
  int           mode,            /* Bit mask of modes (LineMode) */
                input_mode;      /* Input mode state, tracks changes to mode */
  char32_t      cur_char;        /* Current input char */
  /* These are derived from the recipe array above */
  LineKeyMode * cur_input;       /* Current input mode */
  KeyRecipe   * cur_recipe;      /* Current action recipe matched */
  /* Input buffer of key code string, filled on entry to get_line() */
  char        * input_buf;       /* Unprocessed input */
  size_t        input_off,       /* Offset of input, next char to process */
                input_len,       /* Length of input from read() */
                input_buflen;    /* Input buffer allocation len */
  /* Input putback */
  uint8_t       pcnt,            /* Number of chars in pending */
                putb;            /* Pending offset of putback */
  char32_t      pending[ 14 ];   /* Pending function key / multi char input */
};

typedef struct RecipeNode_s RecipeNode;
struct RecipeNode_s {
  RecipeNode * next, * back;
  KeyRecipe    r;
  char      ** args;
  size_t       argc;
};
typedef struct RecipeList_s {
  RecipeNode *hd, *tl;
} RecipeList;

/* Current state:  Line contains the current edit, edited_len is the extent
 * until enter pressed, then line_len contains the length */
struct LineCook_s {
  /* Callbacks / closure */
  void         * closure;      /* Whatever */
  LineReadCB     read_cb;      /* Read from terminal */
  LineWriteCB    write_cb;     /* Write to terminal */
  LineCompleteCB complete_cb;  /* Modify completion db based on input */
  /*LineHintsCB    hints_cb;     * Hint for current input */
  LineCookInput  in;

  /* The current state of the line edit */
  char32_t   * line;            /* Edited line buffer. */
  size_t       line_len,        /* When line is ready, this is the length */
               edited_len,      /* Extent of current line edit */
               erase_len,       /* Max extent of line text on the terminal */
               buflen;          /* Current line allocation len of line */
  int          save_mode,       /* Mode saved for hist searches */
               error;           /* If error, non-zero */

  /* Actions for repeat operations, for example '.' in vi mode */
  KeyAction    action,          /* Current action (KeyAction) */
               last_action,     /* Previous action (for TAB-TAB complete) */
               save_action,     /* Action saved from history search */
               last_repeat_action, /* Last action that can be repeated */
               last_rep_next_act, /* Last find next action that can be rep */
               last_rep_prev_act; /* Last find prev action that can be rep */
  int          emacs_arg_cnt,   /* Meta-0 through Meta-9 emacs/vi ins mode */
               emacs_arg_neg,   /* Meta-minus when < 0 */
               vi_repeat_cnt;   /* Digits entered in vi cmd mode */

  char32_t     last_repeat_char,/* Last character of above action (4 replace) */
               last_rep_next_char, /* Char arg of repeat find next */
               last_rep_prev_char; /* Char arg of repeat find prev */

  ShowMode     show_mode;       /* IF show is active, what show is showing */
  char         search_direction,/* If '/' used, dir = 1, if '?' used dir = -1 */
               quote_char;      /* \" */
  uint8_t      left_prompt_needed,  /* If left prompt should be output */
               right_prompt_needed, /* If line wrap moved right prompt down */
               refresh_needed,   /* If need a refresh line */
               bell_cnt;         /* If ouch bell is displayed */
  uint64_t     bell_time;        /* Time bell is triggered */

  /* Output buffer, flushed after each input buffer is emptied */
  char       * output_buf;      /* Unprocessed output */
  size_t       output_off,      /* Offset of output ready to write() */
               output_buflen;   /* Output buffer allocation len */

  /* Multiple lines in a buffer containing offsets for a double linked list */
  LineSaveBuf  undo,            /* Undo stack, for undo / redo of line edit */
               hist,            /* History stack, previous line edits */
               comp,            /* Completion stack, filled by tab */
               edit,            /* History edits, one entry for every edit */
               keys,            /* Key code translations */
               yank;            /* Yank kill ring */

  size_t       last_yank_start, /* Last command yank start offset */
               last_yank_size;  /* Last command yank size */

  /* Saved search, for repeating the last one */
  char32_t   * search_buf;      /* Search history string */
  size_t       search_len,      /* Length of search string */
               search_buflen;   /* Search buffer allocation len */

  char32_t   * comp_buf;        /* Completion string */
  size_t       comp_len,        /* Length of complete string */
               comp_buflen;     /* Complete buffer allocation len */

  /* Show window data, display history, completions, or undo */
  char32_t   * show_buf;        /* Show buffer */
  size_t       show_len,        /* Amount of text in show buf */
               show_buflen,     /* Show buf allocation len */
               show_rows,       /* Number of lines in show buffer */
               show_cols,       /* NUmber of cols in show buffer */
               show_start,      /* Index of first line in show */
               show_end,        /* Index of last line in show */
               show_pg,         /* Page number of show */
               show_row_start,  /* Row offset of first show line */
               keys_pg;         /* Current page of keys */

  /* Prompts */
  LeftPrompt   prompt;          /* The main prompt */
  LinePrompt   r_ins,           /* Right prompts: vi insert mode */
               r_cmd,           /*                vi command mode */
               r_emacs,         /*                emacs mode */
               r_srch,          /*                search mode */
               r_comp,          /*                tab complete mode */
               r_visu,          /*                visual select mode */
               r_ouch,          /*                ouch bell */
               sel_left,        /* Select indicator left */
               sel_right,       /* Select indicator right */
             * r_prompt;        /* Currently displayed r_prompt above */

  /* Screen geometry / pos */
  size_t       cursor_pos,      /* Current cursor position */
               cols,            /* Number of columns in terminal */
               lines,           /* Number of lines in terminal */
               refresh_pos,     /* Pos of cursor reset until refresh */
               show_lines;

  /* Complete off / len */
  size_t       complete_off,    /* Offset into line where completion starts */
               complete_len;    /* Length of completion phrase */
  CompleteType complete_type;   /* Type of conpletion in operation */
  uint8_t      complete_has_quote, /* Completing a quoted phrase */
               complete_has_glob,  /* Completing a glob wildcard */
               complete_is_prefix; /* Completing a prefix */

  /* Visual mode highlighting */
  size_t       visual_off;      /* Offset into line where visual starts */

  /* Vi mark positions */
  LineMark   * mark;            /* Mark storage */
  size_t       mark_cnt,        /* Count of marks */
               mark_upd,        /* Number of mark updates, cleared on reset */
               mark_size;       /* Size of mark[] buf */

  /* Bit mask of characters */
#define CHAR_BITS_SZ ( 128 / 32 )
  uint32_t     word_brk[ CHAR_BITS_SZ ], /* Chars that separate words */
               plete_brk[ CHAR_BITS_SZ ],/* Chars that break completions */
               quotable[ CHAR_BITS_SZ ]; /* Chars that need quotes */

  char32_t   * cvt;     /* general utf8 to utf32 convert temp */
  size_t       cvt_len; /* size of cvt[] */

  /* This is the table in keycook.h */
  KeyRecipe ** multichar;       /* Multi character transitions */
  size_t       multichar_size;  /* Size of multichar[] */
  KeyRecipe  * recipe;          /* Index of key sequences to actions */
  size_t       recipe_size;     /* Size of recipe[] */

  /* These are derived from the recipe array above */
  LineKeyMode  emacs,              /* Emacs mode transitions */
               vi_insert,          /* Vi insert mode */
               vi_command,         /* Vi command mode */
               visual,             /* Visual select mode */
               search;             /* Search term mode */
  KeyRecipe  * last_repeat_recipe; /* Last repeatable recipe */
  RecipeList   recipe_list;
};

#ifdef __cplusplus
}

namespace linecook {

static const size_t LINE_BUF_LEN_INCR = 128; /* expands as needed */
static const size_t LINE_BUF_BIG_INCR = 64 * 1024;

/* Saved state for hist / undo / redo commands
 *                 
 * first_off --+               
 *             |   +------+ <-+
 *             |   | line |   |
 *             |   | data |   |
 *             |   +------+   |
 *             |   | line | --+ line_off
 *  next_off +-+-- | save |
 *           | +-> +------+ <-+
 *           |     | line |   |
 *           |     | data |   |
 *           |     +------+   |
 *           |     | line | --+ line_off
 *           | +-- | save |
 *           +-|-> +------+ <-- max_off
 *             |
 */
struct LineSave {
  void * operator new( size_t, void *ptr ) { return ptr; }
  size_t line_off,   /* Index into buffer where line starts */
         next_off,   /* Offset of next line in buffer */
         edited_len, /* Length of the line at line_off */
         cursor_off, /* Where the cursor position was in the line */
         index;      /* Used to identify a line */
  LineSave( size_t off,  size_t edit,  size_t curs,  size_t idx )
    : line_off( off ), next_off( 0 ), edited_len( edit ), cursor_off( curs ),
      index( idx ) {}
  /* Compare buf, len to line at off */
  static bool equals( const LineSaveBuf &lsb,  size_t off,  const char32_t *buf,
                      size_t len );
  static int compare( const LineSaveBuf &lsb,  size_t off,  size_t off2 );

  static size_t size( size_t len ); /* Amount of space needed for line len */
  /* Make a new line by copying data into save buffer and adding a LineSave */
  static size_t make( LineSaveBuf &lsb,  const char32_t *buf,  size_t len, 
                      size_t cursor_off,  size_t idx );
  /* Return the LineSave node at off */
  static LineSave &line( LineSaveBuf &lsb,  size_t off );
  /* Return the LineSave node at off */
  static const LineSave &line_const( const LineSaveBuf &lsb,  size_t off );
  /* Count how man nodes a save buf contains, starting at off */
  static size_t card( const LineSaveBuf &lsb );
  /* Return the offset of i when in sorted order */
  static size_t find( const LineSaveBuf &lsb,  size_t off,  size_t i );
  /* Return the offset of the index >= of i */
  static size_t find_gteq( const LineSaveBuf &lsb,  size_t off,  size_t i );
  /* Return the offset of the index >= of i */
  static size_t find_lt( const LineSaveBuf &lsb,  size_t off,  size_t i );
  /* Find a subscring in a line */
  static size_t find_substr( const LineSaveBuf &lsb,  size_t off,
                             const char32_t *str,  size_t len,  int dir );
  /* Find the shortest prefix matching str + len */
  static size_t find_prefix( const LineSaveBuf &lsb,  size_t off,
                             const char32_t *str,  size_t len,
                             size_t &prefix_len,  size_t &match_cnt,
                             size_t &prefix_cnt );
  /* Find longest prefix in set */
  static size_t find_longest_prefix( const LineSaveBuf &lsb,  size_t off,
                                     size_t &prefix_len,  size_t &match_cnt );
  /* Remove entries which do not contain substr */
  static bool filter_substr( LineSaveBuf &lsb,  const char32_t *str,
                             size_t len );
  /* Remove entries which do not match glob */
  static bool filter_glob( LineSaveBuf &lsb,  const char32_t *str,
                           size_t len,  bool implicit_anchor );
  /* Return the offset of idx when not in sorted order */
  static size_t scan( const LineSaveBuf &lsb,  size_t i );
  /* Debug check the fwd & bck links */
  static size_t check_links( LineSaveBuf &lsb,  size_t first,  size_t max_off,
                             size_t cnt );
  /* Resize an element by adjusting the line_off, next_off */
  static size_t resize( LineSaveBuf &lsb,  size_t &off,  size_t newsz );
  /* Reset a save buffer, zero indexes */
  static void reset( LineSaveBuf &lsb );
  /* Sort lines */
  static bool sort( LineSaveBuf &lsb );
  /* Shrink line buffer to items between range */
  static bool shrink_range( LineSaveBuf &lsb,  size_t off,  size_t to_off );
  /* Shrink line buffer to unique items (history compression) */
  static bool shrink_unique( LineSaveBuf &lsb );
};

static const size_t SHOW_PAD = 3;
struct State;
struct ShowState {
  LineSaveBuf * lsb;
  size_t        off,
                cnt;
  bool          has_local_edit, /* line may exist in local edit lines */
                left_overflow,  /* show end of line if overflow */
                show_index;     /* show index number of line */
  ShowState( State &state );
  void zero( void ) {
    this->lsb = NULL;
    this->off = this->cnt = 0;
    this->has_local_edit = this->left_overflow = this->show_index = false;
  }
};

/* The linenoiseState structure represents the state during line editing.
 * We pass this state to functions implementing specific editing
 * functionalities. */
struct State : public LineCook_s {
  void * operator new( size_t, void *ptr ) { return ptr; }
  void operator delete( void *ptr ) { ::free( ptr ); }
  State( int num_cols,  int num_lines );
  ~State();

  /* Initialize key sequence translations */
  int set_recipe( KeyRecipe *key_rec,  size_t key_rec_count );
  void free_recipe( void );
  void init_single_key_transitions( LineKeyMode &km,  uint8_t mode );
  void init_multi_key_transitions( LineKeyMode &km,  uint8_t mode );
  void filter_macro( LineKeyMode &km,  uint8_t mode,  KeyRecipe &r );
  void filter_mode( LineKeyMode &km,  uint8_t &mode,  KeyRecipe &r );
  /* Bind key sequence */
  int bindkey( char *args[],  size_t argc );
  void push_bindkey_recipe( void );
  int add_bindkey_recipe( const char *key,  size_t n,  char **args,
                          size_t argc,  uint8_t valid_mode );
  int remove_bindkey_recipe( const char *key,  size_t n );

  /* Callbacks */
  int read( void *buf,  size_t len ) { /* do the C callbacks */
    return this->read_cb( this, buf, len );
  }
  int write( const void *buf,  size_t len ) {
    return this->write_cb( this, buf, len );
  }
  int completion( const char *buf,  size_t off,  size_t len ) {
    return this->complete_cb( this, buf, off, len );
  }
  /*char *hints( const char *buf,  int &color,  int &bold ) {
    return this->hints_cb( this, buf, &color, &bold );
  }*/
  /* Prompt */
  static ScreenClass screen_class( const char32_t *code,  size_t &sz );
  static ScreenClass escape_class( const char32_t *code,  size_t &sz );
  bool format_prompt( void );
  bool get_prompt_vars( void );
  bool get_prompt_geom( void );
  bool update_date( time_t t );
  bool update_cwd( void );
  bool update_user( void );
  bool update_prompt_time( void );
  bool update_prompt( bool force );
  void set_default_prompt( void );
  bool make_utf32( const char *s,  size_t len,  char32_t *&x,  size_t &xlen );
  bool make_prompt_utf32( const char *s,  size_t len,  char32_t *&x,
                          size_t &xlen );
  int set_prompt( const char *p,  size_t p_len,
                  const char *p2,  size_t p2_len );
  int init_lprompt( void );
  bool init_rprompt( LinePrompt &p,  const char *buf,  size_t len );
  /* Right prompt / select cursor */
  int set_right_prompt( const char *ins,    size_t ins_len,
                        const char *cmd,    size_t cmd_len,
                        const char *emacs,  size_t emacs_len,
                        const char *srch,   size_t srch_len,
                        const char *comp,   size_t comp_len,
                        const char *visu,   size_t visu_len,
                        const char *ouch,   size_t ouch_len,
                        const char *sel1,   size_t sel1_len,
                        const char *sel2,   size_t sel2_len );
  static uint64_t time_ms( void ); /* used for bell timeout */
  void output_right_prompt( bool clear = false );
  void clear_right_prompt( LinePrompt &p );
  void show_right_prompt( LinePrompt &p );
  void erase_eol_with_right_prompt( void );
  /* Clear line and refresh */
  void clear_line( void );
  /* Geom */
  int set_geom( int num_cols,  int num_lines );
  /* Word break chars */
  void set_word_break( const char *brk,  size_t brk_len );
  void set_completion_break( const char *brk,  size_t brk_len );
  void set_quotables( const char *qc,  size_t qc_len,  char quote );
  static void set_char_bit( uint32_t *bits,  char c ) {
    uint32_t off  = (uint8_t) c / 32,
             shft = (uint8_t) c % 32;
    if ( off < CHAR_BITS_SZ )
      bits[ off ] |= ( 1U << shft );
  }
  static bool test_char_bit( const uint32_t *bits,  char32_t c ) {
    uint32_t off  = c / 32,
             shft = c % 32;
    return ( off < CHAR_BITS_SZ && bits[ off ] & ( 1U << shft ) ) != 0;
  }
  bool is_brk_char( char32_t c ) const {
    return test_char_bit( this->word_brk, c );
  }
  bool is_plete_brk( char32_t c ) const {
    return test_char_bit( this->plete_brk, c );
  }
  bool is_quote_char( char32_t c ) const {
    return test_char_bit( this->quotable, c );
  }
  static bool is_spc_char( char32_t c ) {
    return c == ' ';
  }
  /* Main loop */
  int get_line( void ); /* Process input chars and return LINE_STATUS_EXEC when
                           available, < 0 on error, 0 when more input needed  */
  int completion_get_line( void ); /* Get line after completion */
  int continue_get_line( void ); /* Get line after continuation */
  int do_get_line( void );
  int dispatch( void ); /* Process a single command sequence */
  int max_timeout( int time_ms ); /* Get max timer */
  /* Input key to action matching */
  void reset_state( void );     /* Clears line modal vars */
  void reset_input( LineCookInput &input );
  char32_t next_input_char( LineCookInput &input ); /* Multichar to action */
  KeyAction eat_multichar( LineCookInput &input );  /* Char input to action */
  KeyAction eat_input( LineCookInput &input );
  bool input_available( LineCookInput &input );
  int fill_input( void ); /* Read bytes */
  void bell( void );    /* You can ring my bell */
  int char_width_next( size_t off ) const {
    return ( off + 1 < this->edited_len && this->line[ off + 1 ] == 0 ) ? 2 : 1;
  }
  int char_width_back( size_t off ) const {
    return ( off > 1 && this->line[ off - 1 ] == 0 ) ? 2 : 1;
  }

  /* Mode testing and setting */
  static int test( int mode,  int fl ) { return mode & fl; }

  bool is_emacs_mode( void );      /* this->in.mode bit test functions */
  static bool is_emacs_mode( int mode );
  bool is_vi_mode( void );
  bool is_vi_insert_mode( void );
  bool is_vi_command_mode( void );
  static bool is_vi_command_mode( int mode );
  bool is_search_mode( void );
  bool is_replace_mode( void );
  bool is_visual_mode( void );

  void reset_vi_insert_mode( void );
  void set_vi_insert_mode( void ); /* sets this->in.mode bits */
  void set_vi_replace_mode( void );
  void set_emacs_replace_mode( void );
  void set_vi_command_mode( void );
  void set_any_insert_mode( void );  /* either vi or emacs */
  void set_any_replace_mode( void ); /* either vi or emacs */
  void reset_emacs_mode( void );
  void set_emacs_mode( void );

  void set_search_mode( void );
  void clear_search_mode( void );
  void toggle_replace_mode( void );
  void toggle_visual_mode( void );

  void do_search( void );     /* Set history index to search in line buffer */
  void show_search( size_t off );/* Show the line text of the history entry */
  bool display_history_index( size_t idx );
  void display_history_line( LineSave *ls );
  void cancel_search( void ); /* Clear line buffer and cancel search mode */

  /* Convenience functions for word boundaries */
  size_t skip_next_space( size_t off ) { /* find space */
    while ( off < this->edited_len &&
            this->is_spc_char( this->line[ off ] ) ) off++;
    return off;
  }
  size_t skip_next_break( size_t off ) { /* find non-break */
    while ( off < this->edited_len &&
            this->is_brk_char( this->line[ off ] ) ) off++;
    return off;
  }
  size_t skip_next_word( size_t off ) { /* find not space*/
    while ( off < this->edited_len &&
            ! this->is_brk_char( this->line[ off ] ) ) off++;
    return off;
  }
  size_t skip_next_word( size_t off,  uint32_t *b ) {
    while ( off < this->edited_len &&
            ! this->test_char_bit( b, this->line[ off ] ) ) off++;
    return off;
  }
  size_t skip_next_bword( size_t off ) { /* find not space or not break */
    if ( this->is_brk_char( this->line[ off ] ) )
      return this->skip_next_break( off );
    return this->skip_next_word( off );
  }
  size_t skip_prev_space( size_t off ) { /* find space */
    while ( off > 0 && this->is_spc_char( this->line[ off - 1 ] ) ) off--;
    return off;
  }
  size_t skip_prev_break( size_t off ) { /* find non-break */
    while ( off > 0 && this->is_brk_char( this->line[ off - 1 ] ) ) off--;
    return off;
  }
  size_t skip_prev_word( size_t off ) {
    while ( off > 0 && ! this->is_brk_char( this->line[ off - 1 ] ) ) off--;
    return off;
  }
  size_t skip_prev_word( size_t off,  uint32_t *b ) {
    while ( off > 0 && ! this->test_char_bit( b, this->line[ off - 1 ] ) )
      off--;
    return off;
  }
  size_t skip_prev_bword( size_t off ) { /* find not space */
    if ( off > 0 && this->is_brk_char( this->line[ off - 1 ] ) )
      return this->skip_prev_break( off );
    return this->skip_prev_word( off );
  }
  size_t prev_word( size_t off ) { /* skip space, then skip word (backwards) */
    if ( off > 0 && this->is_brk_char( this->line[ off - 1 ] ) )
      return this->skip_prev_word( this->skip_prev_break( off ) );
    return this->skip_prev_bword( this->skip_prev_space( off ) );
  }
  size_t prev_word_start( size_t off ) { /* skip word, then skip space */
    return this->skip_prev_bword( this->skip_prev_space( off ) );
  }
  size_t next_word( size_t off ) { /* skip space, then skip word */
    if ( this->is_brk_char( this->line[ off ] ) )
      return this->skip_next_word( this->skip_next_break( off ) );
    return this->skip_next_bword( this->skip_next_space( off ) );
  }
  size_t next_word_start( size_t off ) { /* skip word, then skip space */
    return this->skip_next_space( this->skip_next_bword( off ) );
  }
  size_t next_word_end( size_t off ) { /* skip word, then skip space */
    if ( off < this->edited_len )
      return this->skip_next_word( this->skip_next_space( off + 1 ) ) - 1;
    return off;
  }
  size_t match_paren( size_t off ); /* Find matching paren */
  void incr_decr( int64_t delta );

  /* Cursor movement */
  void move_cursor_back( size_t amt ); /* sub amt to cursor pos */
  void move_cursor_fwd( size_t amt ); /* add amt to cursor pos */
  void move_cursor( size_t new_pos ); /* move cursor to new pos */

  void output_prompt( void );
  void refresh_line( void ); /* Output line and reposition cursor */
  void visual_bounds( size_t off,  size_t &start,  size_t &end );
  void refresh_visual_line( void ); /* Output visual line mark */

  void extend( size_t extent ) { /* Chars appended to line after edited_len */
    this->edited_len = extent;
    if ( extent > this->erase_len ) /* track max col/row for erasing */
      this->erase_len = extent;
  }
  size_t cursor_row( void ) {
    return this->cursor_pos / this->cols;
  }
  size_t num_rows( void ) {
    return ( ( this->prompt.cols + this->edited_len ) / this->cols );
  }
  size_t erase_rows( void ) {
    return ( ( this->prompt.cols + this->erase_len ) / this->cols );
  }
  size_t next_row( void ) {
    return this->num_rows() + 1;
  }
  /* Display text and increment cursor */
  void cursor_output( char32_t c ); /* output one char, increment cursor */
  void cursor_output( const char32_t *str,  size_t len ); /* output, incr cursor */
  void cursor_erase_eol( void ); /* erase lines after cursor */
  void output_show_string( const char32_t *str,  size_t off,  size_t len );
  size_t output_show_line( const char32_t *show_line,  size_t len );
  void output_show( void ); /* Write show buffer */
  void show_clear( void );  /* Clear show screen area */
  void show_clear_lines( size_t from_row,  size_t to_row ); /* Clear show rows*/
  void incr_show_size( int amt );

  /* Output functions */
  void output( char32_t c ); /* Output functions buffer output until return */
  void output( const char32_t *str,  size_t len );
  void output_str( const char *str,  size_t len ); /* A ANSI seq */
  void output_fmt( const char *fmt,  size_t pos ); /* A single arg ANSI seq */
  void output_newline( size_t count );  /* Put \r\n*count and flush */
  void output_flush( void );  /* Write buffer to terminal */
  int line_length( void );    /* Utf8 length of this->line, line_len */
  int line_copy( char *out ); /* Utf8 copy line, not null terminated */
  int edit_length( void );    /* Utf8 length of this->line, edited_len */
  int edit_copy( char *out ); /* Utf8 copy line, not null terminated */
  int complete_term_length( void );    /* Utf8 length of line[ complete_off ] */
  int complete_term_copy( char *out ); /* Utf8 copy line, not null terminated */
  int line_length( size_t from,  size_t to ); /* Utf8 length of this->line */
  int line_copy( char *out,  size_t from,  size_t to ); /* Utf8 copy line */
  int get_complete_geom( int &arg_num,  int &arg_count,  int *arg_off,
                         int *arg_len,  size_t args_size );
  /* Add to yank buffer */
  void reset_yank( void );
  void add_yank( const char32_t *buf,  size_t size ); /* copy to yank buffer */
  bool get_yank_buf( char32_t *&buf,  size_t &size );
  bool get_yank_pop( char32_t *&buf,  size_t &size );

  /* Undo/Redo */
  void push_undo( void );       /* Push line state, happens before a change */
  LineSave *pop_undo( void );   /* Pop line undo state */
  LineSave *peek_undo( void );  /* Return current undo state */
  LineSave *revert_undo( void );/* Goto top of line undo state */
  LineSave *push_redo( void );  /* Push the last state popped */
  void restore_save( const LineSaveBuf &lsb,  const LineSave &ls );
  void restore_insert( const LineSaveBuf &lsb,  const LineSave &ls,
                       const LineSaveBuf &lsb2,  const LineSave &ls2 );

  /* History push, iter */
  int add_history( const char *buf,  size_t len );
  int compress_history( void );
  void push_history( const char32_t *buf,  size_t len ); /* Push hist stack */
  LineSave *history_older( void );    /* Find older history entry */
  LineSave *history_newer( void );    /* Find newer history entry */
  LineSave *history_top( void );      /* Find top history entry */
  LineSave *history_middle( void );   /* Find middle history entry */
  LineSave *history_bottom( void );   /* Find bottom history entry */
  LineSave *history_move( size_t old_idx );
  void save_hist_edit( size_t idx );  /* Check if line edited then save it */
  LineSave *find_edit( size_t idx );  /* Check if a line edit exists */
  bool get_hist_arg( char32_t *&buf,  size_t &size,  bool nth );
  bool show_history_next_page( void );/* Page down the hist stack */
  bool show_history_prev_page( void );/* Page up the hist stack */
  bool show_history_start( void );    /* First page the hist stack */
  bool show_history_end( void );      /* Last page of the hist stack */
  bool show_history_index( void );    /* Show the page of the hist.idx ptr */
  bool show_lsb( ShowMode m,  LineSaveBuf &lsb ); /* Init and show a page */

  /* Completion */
  CompleteType get_complete_type( void ) { return this->complete_type; }
  void set_complete_type( CompleteType ctype ) { this->complete_type = ctype; }
  size_t quote_line_length( const char32_t *buf,  size_t len );
  void quote_line_copy( char32_t *out,  const char32_t *buf,  size_t len );
  bool tab_complete( bool reverse );
  void copy_complete_string( const char32_t *str,  size_t len );
  void fill_completions( void );
  void reset_completions( void );
  bool tab_first_completion( void );
  void init_completion_term( void );
  bool tab_next_completion( bool reverse );
  int add_completion( const char *buf,  size_t len );
  void push_completion( const char32_t *buf,  size_t len );
#if 0
  LineSave *match_completion( const char *buf,  size_t len,  size_t &match_len,
                              size_t &match_cnt,  CompletionType t );
#endif
  void completion_accept( void );
  void completion_update( int delta );
  void completion_next( void );
  void completion_prev( void );
  void show_completion_index( void );
  void show_completion_prev_page( void );
  void show_completion_next_page( void );
  void completion_start( void );
  void completion_end( void );
  void completion_top( void );
  void completion_bottom( void );

  /* Show commands */
  size_t max_show_lines( void ) const { return this->show_lines; }
  size_t pgcount( LineSaveBuf &lsb ); /* How many pages in the stack */
  size_t pgnum( LineSaveBuf &lsb );   /* What page stack cnt is on */
  bool show_update( size_t old_idx,  size_t new_idx );  /* Update index ptr */
  bool show_save( size_t cur_idx,  size_t start_idx );  /* Show lines in save */
  bool show_line( ShowState &state,  char32_t *buf,  size_t cur_idx,
                  LineSave **lsptr ); /* Copy a line into the save buf */

  /* Other show windows */
  bool show_undo( void );             /* Show the contents of the undo stack */
  bool show_yank( void );             /* Show the contents of the yank buf */

  void layout_keys( const char *key,  const char *action,  const char *mode,
                    const char *descr );
  bool show_keys( void );             /* Show the keys */
  bool show_keys_start( void );
  bool show_keys_end( void );
  bool show_keys_next_page( void );
  bool show_keys_prev_page( void );

  void reset_marks( void );
  void fix_marks( size_t mark_idx );
  void add_mark( size_t mark_off,  size_t mark_idx,  char32_t c );
  bool get_mark( size_t &mark_off,  size_t &mark_idx,  char32_t c );

  /* Buffer reallocs (buf is a ptr to a ptr) */
  bool do_realloc( void *buf,  size_t &len,  size_t newlen ); /* Expand */

  bool realloc_buf32( char32_t **buf,  size_t &len,  size_t newlen ) {
    size_t len32;
    bool b = this->do_realloc( buf, len32, newlen * sizeof( char32_t ) );
    if ( b ) len = len32 / sizeof( char32_t );
    return b;
  }

  bool realloc_buf8( char **buf,  size_t &len,  size_t newlen ) {
    return this->do_realloc( buf, len, newlen );
  }

  bool realloc_line( size_t needed ) {
    return this->buflen >= needed ||
           this->realloc_buf32( &this->line, this->buflen, needed );
  }
  bool realloc_input( size_t needed ) {
    return this->in.input_buflen >= needed ||
           this->realloc_buf8( &this->in.input_buf, this->in.input_buflen,
                               needed );
  }
  bool realloc_output( size_t needed ) {
    return this->output_buflen >= needed ||
           this->realloc_buf8( &this->output_buf, this->output_buflen, needed );
  }
  bool realloc_lsb( LineSaveBuf &lsb,  size_t needed ) {
    return lsb.buflen >= needed ||
           this->realloc_buf32( &lsb.buf, lsb.buflen, needed );
  }
  bool realloc_show( size_t needed ) {
    return this->show_buflen >= needed ||
           this->realloc_buf32( &this->show_buf, this->show_buflen, needed );
  }
  bool realloc_search( size_t needed ) {
    return this->search_buflen >= needed ||
           this->realloc_buf32( &this->search_buf, this->search_buflen, needed);
  }
  bool realloc_complete( size_t needed ) {
    return this->comp_buflen >= needed ||
           this->realloc_buf32( &this->comp_buf, this->comp_buflen, needed );
  }
};

inline bool State::is_emacs_mode( void )
  { return State::test( this->in.mode, EMACS_MODE ) != 0; }
inline bool State::is_emacs_mode( int mode )
  { return State::test( mode, EMACS_MODE ) != 0; }
inline bool State::is_vi_mode( void )
  { return State::test( this->in.mode,
                         ( VI_INSERT_MODE | VI_COMMAND_MODE ) ) != 0; }
inline bool State::is_vi_insert_mode( void )
  { return State::test( this->in.mode, VI_INSERT_MODE ) != 0; }
inline bool State::is_vi_command_mode( void )
  { return State::test( this->in.mode, VI_COMMAND_MODE ) != 0; }
inline bool State::is_vi_command_mode( int mode )
  { return State::test( mode, VI_COMMAND_MODE ) != 0; }
inline bool State::is_search_mode( void )
  { return State::test( this->in.mode, SEARCH_MODE ) != 0; }
inline bool State::is_replace_mode( void )
  { return State::test( this->in.mode, REPLACE_MODE ) != 0; }
inline bool State::is_visual_mode( void )
  { return State::test( this->in.mode, VISUAL_MODE ) != 0; }

/* these can only be in one state: emacs, vi-ins, vi-cmd */
inline void State::reset_vi_insert_mode( void ) {
  this->in.mode |= VI_INSERT_MODE;
  this->in.mode &= ~( VI_COMMAND_MODE | EMACS_MODE |
                      REPLACE_MODE | SEARCH_MODE | VISUAL_MODE );
  this->right_prompt_needed = true;
}
inline void State::set_vi_insert_mode( void ) {
  this->reset_vi_insert_mode();
}
inline void State::set_vi_replace_mode( void ) {
  this->in.mode |= VI_INSERT_MODE | REPLACE_MODE;
  this->in.mode &= ~( VI_COMMAND_MODE | EMACS_MODE | SEARCH_MODE |
                      VISUAL_MODE );
  this->right_prompt_needed = true;
}
inline void State::set_emacs_replace_mode( void ) {
  this->in.mode |= EMACS_MODE | REPLACE_MODE;
  this->in.mode &= ~( VI_INSERT_MODE | VI_COMMAND_MODE | SEARCH_MODE |
                      VISUAL_MODE );
  this->right_prompt_needed = true;
}
inline void State::set_vi_command_mode( void ) {
  this->in.mode |= VI_COMMAND_MODE;
  this->in.mode &= ~( VI_INSERT_MODE | EMACS_MODE |
                      REPLACE_MODE | SEARCH_MODE | VISUAL_MODE );
  this->right_prompt_needed = true;
}
/* reset emacs insert mode */
inline void State::reset_emacs_mode( void ) {
  this->in.mode |= EMACS_MODE;
  this->in.mode &= ~( VI_INSERT_MODE | VI_COMMAND_MODE |
                      REPLACE_MODE | SEARCH_MODE | VISUAL_MODE );
  this->right_prompt_needed = true;
}
inline void State::set_emacs_mode( void ) {
  this->reset_emacs_mode();
}
inline void State::set_any_insert_mode( void ) {
  if ( ( this->in.mode & EMACS_MODE ) == 0 )
    this->reset_vi_insert_mode();
  else
    this->reset_emacs_mode();
}
inline void State::set_any_replace_mode( void ) {
  if ( ( this->in.mode & EMACS_MODE ) == 0 )
    this->set_vi_replace_mode();
  else
    this->set_emacs_replace_mode();
}
/* search mode goes into insert mode, out of search goes to command */
inline void State::set_search_mode( void ) {
  if ( ! this->is_search_mode() ) {
    this->save_mode = this->in.mode;

    this->in.mode |= SEARCH_MODE | VI_INSERT_MODE;
    this->in.mode &= ~( VI_COMMAND_MODE | EMACS_MODE );
    this->right_prompt_needed = true;
  }
}
inline void State::clear_search_mode( void ) {
  if ( this->is_search_mode() ) {
    this->in.mode = this->save_mode;
    this->right_prompt_needed = true;
  }
}
inline void State::toggle_replace_mode( void ) {
  if ( State::test( this->in.mode, VI_COMMAND_MODE ) == 0 )
    this->in.mode ^= REPLACE_MODE;
  this->right_prompt_needed = true;
}
inline void State::toggle_visual_mode( void ) {
  this->in.mode ^= VISUAL_MODE;
  this->right_prompt_needed = true;
}

template <class I> static inline I align( I sz,  I a ) {
  return ( sz + ( a - 1 ) ) & ~( a - 1 );
}
template <class I> static inline I min( I a,  I b ) {
  return ( a < b ? a : b );
}
template <class I> static inline I max( I a,  I b ) {
  return ( a > b ? a : b );
}

static inline size_t
uint_digits( uint64_t v ) {
  for ( size_t n = 1; ; n += 4 ) {
    if ( v < 10 )    return n;
    if ( v < 100 )   return n + 1;
    if ( v < 1000 )  return n + 2;
    if ( v < 10000 ) return n + 3;
    v /= 10000;
  }
}

static inline size_t
uint_to_str( uint64_t v,  char *buf,  size_t digits ) {
  for ( size_t pos = digits; pos > 1; ) {
    const uint64_t q = v / 10, r = v % 10;
    buf[ --pos ] = '0' + r;
    v = q;
  }
  buf[ 0 ] = '0' + v;
  return digits;
}

static inline size_t
uint_to_string( uint64_t v,  char *buf ) {
  size_t d = uint_digits( v );
  uint_to_str( v, buf, d );
  buf[ d ] = '\0';
  return d;
}

static inline size_t
int_digits( int64_t v ) {
  if ( v < 0 ) return uint_digits( (uint64_t) -v ) + 1;
  return uint_digits( (uint64_t) v );
}

static inline size_t
int_to_str( int64_t v,  char *buf,  size_t digits ) {
  if ( v < 0 ) {
    buf[ 0 ] = '-';
    return uint_to_str( (uint64_t) -v, &buf[ 1 ], digits - 1 ) + 1;
  }
  return uint_to_str( (uint64_t) v, buf, digits );
}

static inline size_t
uint_to_str( int64_t v,  char32_t *buf,  size_t digits ) {
  char buf8[ 32 ];
  uint_to_str( v, buf8, digits );
  for ( size_t i = 0; i < digits; i++ )
    buf[ i ] = (char32_t) (uint8_t) buf8[ i ];
  return digits;
}

static inline size_t
int_to_str( int64_t v,  char32_t *buf,  size_t digits ) {
  char buf8[ 32 ];
  int_to_str( v, buf8, digits );
  for ( size_t i = 0; i < digits; i++ )
    buf[ i ] = (char32_t) (uint8_t) buf8[ i ];
  return digits;
}

template <class T> static inline bool
isdig( T c ) { return c >= '0' && c <= '9'; }

static inline int64_t
str_to_int( const char32_t *str,  size_t len )
{
  int64_t ival = 0;
  size_t  i    = 0;
  bool    neg  = false;
  if ( i < len && str[ i ] == '-' ) {
    neg = true;
    i++;
  }
  for ( ; i < len && isdig<char32_t>( str[ i ] ); i++ )
    ival = ival * 10 + ( str[ i ] - '0' );
  return neg ? -ival : ival;
}

static inline bool
char32_eq( char c,  char32_t c32 ) {
  return (uint8_t) c == (uint8_t) c32 && c32 < 128;
}

template <class T> static inline void
move( T *dest,  const T *src,  size_t len ) {
  ::memmove( dest, src, len * sizeof( T ) );
}

template <class T> static inline void
copy( T *dest,  const T *src,  size_t len ) {
  ::memcpy( dest, src, len * sizeof( T ) );
}

template <class T> static inline void
set( T *dest,  T c,  size_t len ) {
  for ( size_t i = 0; i < len; i++ )
    dest[ i ] = c;
}

static inline void
rotate( char32_t *p, size_t len,  size_t cnt ) {
  for (;;) {
    char32_t c = p[ len - 1 ];
    move<char32_t>( &p[ 1 ], p, len - 1 );
    p[ 0 ] = c;
    if ( --cnt == 0 )
      return;
  }
}

template <class T> static inline bool
islocase( T a ) {
  if ( a >= 0 && a < 128 )
    return ( a >= 'a' && a <= 'z' );
  return ku_islocase_utf32( a );
}

template <class T> static inline bool
isupcase( T a ) {
  if ( a >= 0 && a < 128 )
    return ( a >= 'A' && a <= 'Z' );
  return ku_isupcase_utf32( a );
}

template <class T> static inline T
locase( T a ) {
  if ( a >= 0 && a < 128 ) {
    if ( a >= 'A' && a <= 'Z' )
      a |= 0x20;
    return a;
  }
  return ku_locase_utf32( a );
}

template <class T> static inline T
upcase( T a ) {
  if ( a >= 0 && a < 128 ) {
    if ( a >= 'a' && a <= 'z' )
      a &= ~0x20;
    return a;
  }
  return ku_upcase_utf32( a );
}

template <class T> static inline int
casecmp( T a,  T b ) {
  if ( a != b ) {
    a = locase<T>( a );
    b = locase<T>( b );
    if ( a != b )
      return ( a > b ? 1 : -1 );
  }
  return 0;
}

template <class T> static inline int
cmp( const T *a,  const T *b,  size_t len ) {
  for ( size_t i = 0; i < len; i++ )
    if ( a[ i ] != b[ i ] )
      return ( a[ i ] > b[ i ] ? 1 : -1 );
  return 0;
}

template <class T> static inline int
casecmp( const T *a,  const T *b,  size_t len ) {
  for ( size_t i = 0; i < len; i++ ) {
    T x = a[ i ], y = b[ i ];
    if ( x != y ) {
      x = locase<T>( x );
      y = locase<T>( y );
      if ( x != y )
        return ( x > y ? 1 : -1 );
    }
  }
  return 0;
}

template <class T> static inline int
cmp( const T *a,  const T *b,  size_t alen,  size_t blen ) {
  size_t len = min<size_t>( alen, blen );
  for ( size_t i = 0; i < len; i++ )
    if ( a[ i ] != b[ i ] )
      return ( a[ i ] > b[ i ] ? 1 : -1 );
  return alen == blen ? 0 : ( alen > blen ? 1 : -1 );
}

template <class T> static inline int
casecmp( const T *a,  const T *b,  size_t alen,  size_t blen ) {
  size_t len = min<size_t>( alen, blen );
  for ( size_t i = 0; i < len; i++ ) {
    T x = a[ i ], y = b[ i ];
    if ( x != y ) {
      x = locase<T>( x );
      y = locase<T>( y );
      if ( x != y )
        return ( x > y ? 1 : -1 );
    }
  }
  return alen == blen ? 0 : ( alen > blen ? 1 : -1 );
}

}
#endif

#endif
