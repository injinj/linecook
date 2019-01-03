#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <linecook/linecook.h>
#include <linecook/ttycook.h>

static int
complete( LineCook *lc,  const char *buf,  size_t off,  size_t len )
{
  CompleteType complete_type = lc_get_complete_type( lc );
  if ( complete_type == COMPLETE_ANY ) {
    if ( off == 0 )
      lc_set_complete_type( lc, COMPLETE_EXES ); /* change to exe mode */
    else if ( off + len >= 2 && buf[ 0 ] == 'c' && buf[ 1 ] == 'd' ) {
      if ( off + len == 2 || buf[ 2 ] == ' ' )
        lc_set_complete_type( lc, COMPLETE_DIRS ); /* change to dir mode */
    }
  }
  return lc_tty_file_completion( lc, buf, off, len );
}

static size_t
split_args( const char *line,  size_t line_len,  char *args[],
            size_t max_args )
{
  size_t i, argc = 0;
  const char *start = NULL, *end = NULL;
  for ( i = 0; ; ) {
    while ( i < line_len && isspace( line[ i ] ) )
      i++;
    if ( start != NULL && end != NULL && end > start ) {
      size_t len = end - start;
      args[ argc ] = (char *) malloc( len + 1 );
      memcpy( args[ argc ], start, len );
      args[ argc++ ][ len ] = '\0';
      if ( argc == max_args )
        return argc;
    }
    if ( i >= line_len )
      return argc;
    start = &line[ i ];
    if ( *start == '"' ) {
      start++;
      end = start;
      while ( ++i < line_len && *end != '\"' )
        end++;
      i++;
    }
    else {
      while ( i < line_len && ! isspace( line[ i ] ) )
        i++;
      end = &line[ i ];
    }
  }
}

static void
free_args( char *args[],  size_t argc )
{
  size_t i = 0;
  while ( i < argc )
    free( args[ i++ ] );
}

static int
init_tty( TTYCook *tty )
{
  /* this prompt setup looks like this:
   *  \u     \h       \w          \R            \4      \l       \d      \@
   * chris@deedee:~/linecook/                10.4.4.21 pts/0 Sat Nov 03 11:49pm
   *  \# \$
   * [13]$
   */
  static const char * prompt = "\\U0001f436 " /* doge */
                             //ANSI_BLUE    "\\u250c\\u2510 " ANSI_NORMAL
                               ANSI_CYAN    "\\u"   ANSI_NORMAL "@"
                               ANSI_MAGENTA "\\h"   ANSI_NORMAL ":"
                               ANSI_GREEN   "\\w"   ANSI_NORMAL "\\R"
                               ANSI_DEVOLVE "\\4 \\l" ANSI_NORMAL " "
                               ANSI_BLUE    "\\d \\@" ANSI_NORMAL "\\r\\n"
                             //ANSI_BLUE    "\\u2514\\u2518 " ANSI_NORMAL
                               ANSI_BLUE    "["     ANSI_NORMAL 
                               ANSI_RED     "\\#"   ANSI_NORMAL
                               ANSI_BLUE    "]"     ANSI_NORMAL "\\$ ",
                    * promp2 = ANSI_BLUE    "> "    ANSI_NORMAL,
                    * ins    = ANSI_GREEN   "<-"    ANSI_NORMAL,
                    * cmd    = ANSI_MAGENTA "|="    ANSI_NORMAL,
                    * emacs  = ANSI_GREEN   "<e"    ANSI_NORMAL,
                    * srch   = ANSI_CYAN    "/_"    ANSI_NORMAL,
                    * comp   = ANSI_MAGENTA "TAB"   ANSI_NORMAL,
                    * visu   = ANSI_CYAN    "[-]"   ANSI_NORMAL,
/* doge no smoking* * ouch   =           "\\U0001f436 "   * 🐶 * 
                                         "\\U0001f6ad "   * 🚭 * 
                                         "\\U0001f354",   * 🍔 */
/* XxXxX       */   * ouch   = ANSI_RED   "\\u274cx" 
                                          "\\u274cx" 
                                          "\\u274c" ANSI_NORMAL,
/* red skulls */ /* * ouch   = ANSI_RED  "\\xE2\\x98\\xA0 " 
                                         "\\xE2\\x98\\xA0 " 
                                         "\\xE2\\x98\\xA0" ANSI_NORMAL, */
                    /** ouch   = ANSI_RED     "?1!#?" ANSI_NORMAL,*/
                    * sel1   = ANSI_RED     "["     ANSI_NORMAL,
                    * sel2   = ANSI_RED     "]"     ANSI_NORMAL,
                    * brk    = " \t\n\\'`><=;|&{()}",
                    * qc     = " \t\n\\\"'@<>=;|&()#$`?*[!:{";

  lc_tty_set_locale(); /* setlocale( LC_ALL, "" ) */
  lc_set_completion_break( tty->lc, brk, strlen( brk ) );
  lc_set_quotables( tty->lc, qc, strlen( qc ), '\'' ); /* quoting uses \' */
  lc_tty_open_history( tty, ".example_history" );
  tty->lc->complete_cb = complete;

  /* init i/o fd, prompt vars, geometry, SIGWINCH */
  if ( lc_tty_init_fd( tty, STDIN_FILENO, STDOUT_FILENO ) != 0 ||
       lc_tty_set_prompt( tty, TTYP_PROMPT1, prompt )     != 0 ||
       lc_tty_set_prompt( tty, TTYP_PROMPT2, promp2 )     != 0 ||
       lc_tty_set_prompt( tty, TTYP_R_INS,   ins )        != 0 ||
       lc_tty_set_prompt( tty, TTYP_R_CMD,   cmd )        != 0 ||
       lc_tty_set_prompt( tty, TTYP_R_EMACS, emacs )      != 0 ||
       lc_tty_set_prompt( tty, TTYP_R_SRCH,  srch )       != 0 ||
       lc_tty_set_prompt( tty, TTYP_R_COMP,  comp )       != 0 ||
       lc_tty_set_prompt( tty, TTYP_R_VISU,  visu )       != 0 ||
       lc_tty_set_prompt( tty, TTYP_R_OUCH,  ouch )       != 0 ||
       lc_tty_set_prompt( tty, TTYP_R_SEL1,  sel1 )       != 0 ||
       lc_tty_set_prompt( tty, TTYP_R_SEL2,  sel2 )       != 0 ||
       lc_tty_init_geom( tty )                            != 0 ||
       lc_tty_init_sigwinch( tty )                        != 0 )
    return -1;
  return 0;
}

int
main( void )
{
  LineCook * lc  = lc_create_state( 80, 25 );
  TTYCook  * tty = lc_tty_create( lc );

  if ( lc == NULL || tty == NULL || init_tty( tty ) != 0 )
    return 1;
  printf( "Welcome to dumb shell, lines entered will use system(3)\r\n" );
  printf( "Type exit/quit to exit\r\n" );
  for (;;) {
    int n = lc_tty_get_line( tty ); /* retry line and run timed events */
    if ( n == 0 )
      n = lc_tty_poll_wait( tty, 250 ); /* wait on terminal for 250ms */
    if ( n < 0 ) /* if error in one of the above */
      break;
    if ( tty->lc_status == LINE_STATUS_INTERRUPT ) {
      lc_tty_set_continue( tty, 0 ); /* cancel continue */
      lc_tty_break_history( tty );   /* cancel buffered line */
    }
    else if ( tty->lc_status == LINE_STATUS_SUSPEND ) {
      lc_tty_normal_mode( tty ); /* set terminal to normal */
      raise( SIGSTOP ); /* suspend */
    }
    else if ( tty->lc_status == LINE_STATUS_COMPLETE ) {
      CompleteType ctype = lc_get_complete_type( lc );
      if ( ctype != COMPLETE_HIST ) {
        size_t i;
        int  arg_num,   /* which arg is completed, 0 = first */
             arg_count, /* how many args */
             arg_off[ 32 ],  /* offset of args */
             arg_len[ 32 ];  /* length of args */
        char buf[ 1024 ];
        int n = lc_tty_get_completion_cmd( tty, buf, sizeof( buf ),
                                           &arg_num, &arg_count, arg_off,
                                           arg_len, 32 );
        #define MATCH_CMD( CMD ) \
          ( sizeof( CMD ) == arg_len[ 0 ] + 1 && \
            strncmp( CMD, &buf[ arg_off[ 0 ] ], arg_len[ 0 ] ) == 0 )
        /* check if git completion */
        if ( MATCH_CMD( "git" ) && arg_num == 1 ) {
          static const char *g[] = {
            "init", "add", "mv", "reset", "rm", "bisect",
            "grep", "log", "show", "status", "branch",
            "checkout", "commit", "diff", "merge", "rebase",
            "tag", "fetch", "pull", "push"
          };
          for ( i = 0; i < sizeof( g ) / sizeof( g[ 0 ] ); i++ ) {
            lc_add_completion( lc, g[ i ], strlen( g[ i ] ) );
          }
        }
        /* for vi(m) use fzf completion */
        else if ( ( MATCH_CMD( "vi" ) || MATCH_CMD( "vim" ) ) &&
                  arg_num == 1 ) {
          #define FZFCMD "find . -print | fzf --layout=reverse --height=50%"
          const char *cmd = FZFCMD;
          FILE *fp;
          char query[ 128 ];
          n = lc_tty_get_completion_term( tty, query, sizeof( query ) );
          if ( n > 0 ) { /* append query to fzf command above */
            snprintf( buf, sizeof( buf ), FZFCMD "% --query=\"%s\"", query );
            cmd = buf;
          }
          fp = popen( cmd, "r" );
          if ( fp != NULL ) {
            while ( fgets( buf, sizeof( buf ), fp ) != NULL ) {
              size_t len = strlen( buf );
              while ( len > 0 && buf[ len - 1 ] <= ' ' )
                buf[ --len ] = '\0';
              if ( len > 0 )
                lc_add_completion( lc, buf, len );
            }
            pclose( fp );
          }
          else {
            perror( "popen(fzf)" );
          }
        }
      }
#if 0
      else { /* test history completion */
        lc_add_completion( lc, "a test line example", 19 );
      }
#endif
    }
    else if ( tty->lc_status == LINE_STATUS_EXEC ) { /* if a line available */
      int is_continue = 0;
      if ( strcmp( tty->line, "exit" ) == 0 ||
           strcmp( tty->line, "quit" ) == 0 ||
           strcmp( tty->line, "q" ) == 0 ) {
        goto break_loop;
      }
      if ( strcmp( tty->line, "rotate" ) == 0 ) {
        lc_tty_rotate_history( tty ); /* test history rotation */
        continue;
      }
      /* if line has a \\n trailer, continue */
      if ( tty->line_len > 0 && tty->line[ tty->line_len - 1 ] == '\\' ) {
        lc_tty_set_continue( tty, 1 );
        /* push it back, it will be prepended to the next line */
        lc_tty_push_line( tty, tty->line, tty->line_len - 1 );
        is_continue = 1;
      }
      else {
        lc_tty_set_continue( tty, 0 );
        /* log the line to history file (.example_history) */
        lc_tty_log_history( tty );
      }
      lc_tty_normal_mode( tty ); /* set terminal to normal */
      if ( ! is_continue ) {
        if ( tty->line_len > 8 && strncmp( tty->line, "bindkey ", 8 ) == 0 ) {
          char *args[ 16 ];
          size_t argc;
          argc = split_args( &tty->line[ 8 ], tty->line_len-8, args, 16 );
          if ( argc > 0 ) {
            lc_bindkey( lc, args, argc );
            free_args( args, argc );
          }
          else {
            fprintf( stderr, "usage: bindkey <key> val\n" );
          }
          continue;
        }
        /* builtin cd (no ~ or $var expansion) */
        if ( strncmp( tty->line, "cd", 2 ) == 0 &&
             ( tty->line[ 2 ] == ' ' || tty->line[ 2 ] == 0 ) ) {
          const char *s;
          int i = 2;
          while ( tty->line[ i ] == ' ' ) i++;
          /* if cd has args, use those, otherwise go home */
          s = ( tty->line[ i ] == 0 ? getenv( "HOME" ) : &tty->line[ i ] );
          if ( s == NULL ) s = "/";
          if ( chdir( s ) != 0 )
            perror( s );
        }
        /* otherwise, let sh do the work */
        else {
          if ( system( tty->line ) != 0 )
            fprintf( stderr, "system() errno = %d (%s)\n", errno, strerror( errno ) );
        }
      }
    }
  }
break_loop:;
  if ( lc->error != 0 )
    printf( "error: %s\r\n", LINE_ERR_STR( lc->error ) );
  printf( "bye\r\n" );
  lc_tty_normal_mode( tty );
  lc_tty_release( tty );
  lc_release_state( lc );
  return 0;
}
