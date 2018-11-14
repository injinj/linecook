#include <stdio.h>
#include <unistd.h>
#include <linecook/linecook.h>
#include <linecook/ttycook.h>

int
main( void )
{
  LineCook * lc  = lc_create_state( 80, 25 );
  TTYCook  * tty = lc_tty_create( lc );

  lc_tty_set_locale(); /* setlocale( LC_ALL, "" ) */
  lc_tty_init_fd( tty, STDIN_FILENO, STDOUT_FILENO ); /* use stdin/stdout */
  lc_tty_set_default_prompts( tty );
  lc_tty_init_geom( tty );     /* try to determine lines/cols */
  lc_tty_init_sigwinch( tty ); /* install sigwinch handler */

  printf( "simple echo test, type 'q' to quit\n" );
  for (;;) {
    int n = lc_tty_get_line( tty ); /* retry line and run timed events */
    if ( n == 0 )
      n = lc_tty_poll_wait( tty, 60000 ); /* wait at most 60 seconds */
    if ( n < 0 ) /* if error in one of the above */
      break;
    if ( tty->lc_status == LINE_STATUS_EXEC ) { /* if there is one available */
      lc_tty_normal_mode( tty );     /* set terminal to normal mode */
      printf( "[%s]\n", tty->line ); /* which translates \n to \r\n */
      if ( tty->line_len == 1 && tty->line[ 0 ] == 'q' ) /* type 'q' to quit */
        break;
      lc_tty_log_history( tty );
    }
  }
  lc_tty_normal_mode( tty ); /* restore normal terminal handling */
  if ( tty->lc_status < 0 )
    printf( "status: %s\n", LINE_ERR_STR( tty->lc_status ) );
  lc_tty_release( tty );     /* free tty */
  lc_release_state( lc );    /* free lc */
  return 0;
}

