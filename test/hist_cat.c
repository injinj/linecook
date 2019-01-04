#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <linecook/linecook.h>
#include <linecook/ttycook.h>

static const char *
get_arg( int argc, char *argv[], int b, const char *f, const char *def )
{
  int i;
  for ( i = 1; i < argc - b; i++ )
    if ( strcmp( f, argv[ i ] ) == 0 ) /* -p port */
      return argv[ i + b ];
  return def; /* default value */
}

int
main( int argc, char *argv[] )
{
  const char * history = ".example_history";
  LineCook   * lc  = lc_create_state( 80, 25 );
  TTYCook    * tty = lc_tty_create( lc );
  size_t       buflen,
               count,
               i;
  char       * buf;
  int          n;
  const char * rev = get_arg( argc, argv, 0, "-r", NULL ),
             * fn  = get_arg( argc, argv, 1, "-f", getenv( "history" ) ),
             * num = get_arg( argc, argv, 0, "-n", NULL ),
             * h   = get_arg( argc, argv, 0, "-h", NULL );

  if ( fn == NULL )
    fn = history;
  if ( h != NULL ) {
    printf( "%s [-f file] [-r] [-n]\n", argv[ 0 ] );
    printf( " -f file : file = history file name (default: %s)\n", fn );
    printf( " -r : reverse order, last to first\n" );
    printf( " -n : add numbers 1 -> N to each line\n" );
    printf(
"This program deduplicates lines, concatenates, and reverses the order\n"
"of history files\n" );
    return 0;
  }
  if ( lc == NULL || tty == NULL )
    return 1;
  if ( lc_tty_open_history( tty, fn ) == 0 ) {
    buflen = 1024;
    buf    = (char *) malloc( buflen );
    count  = lc_history_count( lc );
    i      = (rev != NULL) ? count : 1;
    while ( 1 <= i && i <= count ) {
      n = lc_history_line_length( lc, i );
      if ( n > (int) buflen ) {
        buflen = (size_t) ( n + 1 );
        buf = (char *) realloc( buf, buflen );
      }
      lc_history_line_copy( lc, i, buf );
      if ( num != NULL )
        printf( "%ld ", i );
      printf( "%.*s\n", (int) n, buf );
      i = (rev != NULL) ? ( i - 1 ) : ( i + 1 );
    }
  }
  lc_tty_release( tty );
  lc_release_state( lc );
  return 0;
}
