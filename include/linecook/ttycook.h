#ifndef __linecook__ttycook_h__
#define __linecook__ttycook_h__
/* va_list */
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LineCook_s LineCook; /* in linecook.h */

typedef enum TTYPrompt_e {
  TTYP_PROMPT1 = 0,      /* primary prompt on the left */
  TTYP_PROMPT2 = 1,      /* alternate prompt for line continuation */
  TTYP_R_INS   = 2,      /* vi insert mode right prompt */
  TTYP_R_CMD   = 3,      /* vi command mode right prompt */
  TTYP_R_EMACS = 4,      /* emacs mode right prompt */
  TTYP_R_SRCH  = 5,      /* history search right prompt */
  TTYP_R_COMP  = 6,      /* tab complete right prompt */
  TTYP_R_VISU  = 7,      /* visual select right prompt */
  TTYP_R_OUCH  = 8,      /* ouch right prompt (bell visual) */
  TTYP_R_SEL1  = 9,      /* left select tick */
  TTYP_R_SEL2  = 10,     /* right select tick */
  TTYP_MAX     = 11
} TTYPrompt;

extern const char *ttyp_def_prompt[]; /* index by TTYPrompt */

typedef enum TTYState_s {
  TTYS_IS_RAW         = 1,  /* when terminal in raw mode */
  TTYS_IS_NONBLOCK    = 2,  /* when fds are async w/O_NONBLOCK */
  TTYS_POLL_OUT       = 4,  /* when write() returns EAGAIN */
  TTYS_PROMPT_UPDATE  = 8,  /* when a prompt is changed and needs updating */
  TTYS_GEOM_UPDATE    = 16, /* when screen geom changed */
  TTYS_HIST_UPDATE    = 32, /* when external history added */
  TTYS_CONTINUE_LINE  = 64, /* when continuing to current line */
  TTYS_ROTATE_HISTORY = 128,/* when hitory needs rotation */
  TTYS_COMPLETE       = 256 /* when completion requested */
} TTYState;

/*
 * History is shared between shells:
 *   after 5 seconds of idle, the history file is checked for updates;
 *   after 60 seconds of idle, the history is checked for rotations.
 *
 * If a shell is active, then hist updates are checked when a new line
 * is logged.
 *
 * When line_cnt >= rotate_line_cnt, then a history file is rotated
 * after a 5 second idle period in order to give other shells a chance
 * to catch up with current history.
 *
 * Rotation moves the filename -> filename.1, filename.2 -> filename.3, etc.
 * On startup, the history files are read in reverse order:
 * filename.3, then filename.2, filename.1, and filename
 *
 * There currently exists no mechanism to remove old history files, they
 * must be removed manually.
 *
 * There is a filename.lck file that protects multiple shells from rotating
 * the history files at the same time.  If that is not cleaned up, it will
 * causes a 10 second timeout, then failure to open the history file.
 */
typedef struct TTYHistory_s {
  char * filename;   /* ex: /home/user/.xyz_history */
  size_t fsize,      /* size of last read, excluding partially written lines */
         line_cnt,   /* history line count in file name */
         rotate_line_cnt; /* when history file should be rotated */
  time_t last_check, /* time since last looked at the file for new lines */
         last_rotation_check; /* time since last hist rotation check */
  int    fd;         /* fd for appending with O_APPEND, which should be atomic*/
  char * hist_buf;   /* save history buffer for continuations */
  size_t hist_len,
         hist_buflen;
} TTYHistory;

typedef struct TTYCook_s {
  LineCook * lc;            /* the line cook, lc->closure == this */
  void     * closure;       /* can be used externally */
  int        in_fd,         /* input read here */
             out_fd,        /* output sent here */
             in_mode,       /* original fcntl() of in_fd */
             out_mode,      /* original fcntl() of out_fd */
             cols,          /* new cols & lines */
             lines,
             approx_ticks;  /* approximate time passed since hist check */
  TTYHistory hist;          /* history file data */
#ifndef _MSC_VER
  void     * raw,           /* termios in raw mode */
           * orig;          /* original termios */
#else
  void     * win_vt;
#endif
  size_t     off[ TTYP_MAX ];  /* offsets into prompt_buf[] for prompts */
  char     * prompt_buf;    /* buffer for prompts above */
  size_t     prompt_len,    /* length of buffer used */
             prompt_buflen, /* prompt buf size */
             prompt_cnt;    /* Number of prompts set */
  char     * line;          /* Current line */
  size_t     line_len,      /* Line length (null terminated) */
             line_buflen;   /* Alloc size of line */
  char     * push_buf;      /* Line(s) saved in push back for continue */
  size_t     push_len,      /* Length of push back line */
             push_buflen;   /* Alloc push len */
  int        state,         /* tracks whether in raw mode or not */
             lc_status;     /* status of current lc_get_line() */
} TTYCook;

/* setlocale( LC_ALL, "" ) */
void lc_tty_set_locale( void );
/* Create tty struct and set it as the closure for lc */
TTYCook *lc_tty_create( LineCook *lc );
/* Free it */
void lc_tty_release( TTYCook *tty );
/* Set simple default prompt (nocolor) */
int lc_tty_set_default_prompt( TTYCook *tty,  TTYPrompt pnum );
/* Set all simple default prompts (nocolor) */
int lc_tty_set_default_prompts( TTYCook *tty );
/* Set simple default prompt w/color */
int lc_tty_set_color_prompt( TTYCook *tty,  TTYPrompt pnum );
/* Set all simple default prompts w/color */
int lc_tty_set_color_prompts( TTYCook *tty );
/* Set one of the prompts, this buffers the prompts to prevent unnecessary
 * prompt updates */
int lc_tty_set_prompt( TTYCook *tty,  TTYPrompt pnum,  const char *p );
/* Set the fds and the callbacks for read/write, necessary for the next funcs */
int lc_tty_init_fd( TTYCook *tty,  int in,  int out );
/* Register the signal for SIGWINCH */
int lc_tty_init_sigwinch( TTYCook *tty );
/* Init history file, load, create, and log to it */
int lc_tty_open_history( TTYCook *tty,  const char *fn );
/* Log the current line to the history file */
int lc_tty_log_history( TTYCook *tty );
/* Rotate the history files */
int lc_tty_rotate_history( TTYCook *tty );
/* Push the line into the history buffer */
int lc_tty_push_history( TTYCook *tty,  const char *line,  size_t len );
/* Flush the history buffer to the log file, if it exists */
int lc_tty_flush_history( TTYCook *tty );
/* Toss any history buffer pushed before */
void lc_tty_break_history( TTYCook *tty );
/* Prepare for terminal raw mode i/o */
int lc_tty_raw_mode( TTYCook *tty );
/* Prepare for terminal non block i/o */
int lc_tty_non_block( TTYCook *tty );
/* Restore terminal original mode */
int lc_tty_reset_raw( TTYCook *tty );
/* Restore terminal fcntl */
int lc_tty_reset_non_block( TTYCook *tty );
/* Reset the terminal to normal mode */
int lc_tty_normal_mode( TTYCook *tty );
/* Continue get_line mode set or clear */
void lc_tty_set_continue( TTYCook *tty,  int on );
/* Concat this line with the next line input */
int lc_tty_push_line( TTYCook *tty,  const char *line,  size_t len );
/* Try to read a line, returns 1 when have a line, 0 when not, -1 on error */
int lc_tty_get_line( TTYCook *tty );
/* Fetch the term that is being completed from the line edit */
int lc_tty_get_completion_cmd( TTYCook *tty,  char *cmd,  size_t len,
                               int *arg_num,  int *arg_count,  int *arg_off,
                               int *arg_len,  size_t arg_size );
/* Fetch the term that is being completed from the line edit */
int lc_tty_get_completion_term( TTYCook *tty,  char *term,  size_t len );
/* Wait for I/O, returns 1 when ready, 0 when not, -1 when error */
int lc_tty_poll_wait( TTYCook *tty,  int time_ms );
/* Get geom and use it */
int lc_tty_init_geom( TTYCook *tty );
/* Try to get terminal geometry */
void lc_tty_get_terminal_geom( int fd,  int *cols,  int *lines );
/* Clear line and refresh when get_line() called again */
void lc_tty_clear_line( TTYCook *tty );
void lc_tty_erase_prompt( TTYCook *tty );
void lc_tty_refresh_line( TTYCook *tty );
/* printf to output */
int lc_tty_print( TTYCook *tty,  const char *fmt,  ... )
#ifndef _MSC_VER
    __attribute__((format(printf,2,3)));
#else
   ;
#endif
int lc_tty_printv( TTYCook *tty,  const char *fmt,  va_list args );
int lc_tty_write( TTYCook *tty,  const void *buf,  int buflen );
/* flush prompt to output */
int lc_tty_show_prompt( TTYCook *tty );
/* Do file completeion */
int lc_tty_file_completion( LineCook *lc,  const char *buf,  size_t off,
                            size_t len,  void *arg );
#ifdef __cplusplus
}

namespace linecook {

struct TTY : public TTYCook_s {
  void * operator new( size_t, void *ptr ) { return ptr; }
  void operator delete( void *ptr ) { ::free( ptr ); }
  TTY( LineCook *lc ) noexcept;
  ~TTY() noexcept;
  void release( void ) noexcept;

  int test( int fl ) const { return this->state & fl; }
  void set( int fl )       { this->state |= fl; }
  void clear( int fl )     { this->state &= ~fl; }
  /* update a prompt */
  int set_prompt( TTYPrompt pnum,  const char *p ) noexcept;
  int init_fd( int in,  int out ) noexcept;

  /* history, history, so much history */
  int open_history( const char *fn,  bool reinitialize ) noexcept;
  size_t load_history_buffer( const char *buf,  size_t n,
                              size_t &line_cnt ) noexcept;
  size_t read_history( int rfd,  size_t max_len,  size_t &line_cnt ) noexcept;
  int check_history( void ) noexcept;
  int log_history( void ) noexcept;
  int log_hist( char *line,  size_t len ) noexcept;
  int push_history( const char *line,  size_t len ) noexcept;
  int flush_history( void ) noexcept;
  void break_history( void ) noexcept;
  int close_history( void ) noexcept;
  int acquire_history_lck( const char *filename,  char *lckpath ) noexcept;
  int rotate_history( void ) noexcept;
  /* terminal / tty fd mode change */
  int raw_mode( void ) noexcept;
  int non_block( void ) noexcept;
  int reset_raw( void ) noexcept;
  int reset_non_block( void ) noexcept;
  int normal_mode( void ) noexcept;
  void clear_line( void ) noexcept;
  void erase_prompt( void ) noexcept;
  void refresh_line( void ) noexcept;
  /* completion */
  int get_completion_cmd( char *cmd,  size_t len,  int *arg_num,
                          int *arg_count,  int *arg_off,
                          int *arg_len,  size_t args_size ) noexcept;
  int get_completion_term( char *term,  size_t len ) noexcept;
  /* line edit */
  int push_line( const char *line,  size_t len ) noexcept; /* push line back */
  int init_get_line( void ) noexcept;
  int get_line( void ) noexcept;         /* same as lc_tty_get_line() */
  /* wait for i/o */
  int poll_wait( int time_ms ) noexcept; /* same as lc_tty_poll_wait() */

  char * ptr( TTYPrompt pnum ) noexcept; /* get the prompt associated w/enum */
  size_t len( TTYPrompt pnum ) const noexcept;
};

}

#endif

#endif

