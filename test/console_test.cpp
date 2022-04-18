#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include <io.h>
#include <linecook/console_vt.h>

using namespace linecook;

int
main( void )
{
  ConsoleVT term;

  if ( ! term.init_vt( _fileno( stdin ), _fileno( stdout ) ) ) {
    fprintf( stderr, "unable to init term\n" );
    return 1;
  }
  if ( ! term.enable_raw_mode() ) {
    fprintf( stderr, "unable to set raw mode\n" );
    return 1;
  }

  for (;;) {
    if ( term.poll_events( 1000 ) ) {
      if ( term.read_input() ) {
        term.disable_raw_mode();
        for ( size_t i = 0; i < term.input_avail; i++ ) {
          char c = term.input_buf[ i ];
          if ( c > ' ' && c <= '~' )
            printf( "%c ", c );
          else
            printf( "%d ", (uint8_t) c );
        }
        printf( "\n" );
        term.enable_raw_mode();
        if ( term.input_avail > 0 && term.input_buf[ 0 ] == 'q' )
          break;
      }
    }
  }
  term.disable_raw_mode();
  return 0;
}

