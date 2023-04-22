#ifndef __linecook__console_vt_h__
#define __linecook__console_vt_h__

#if defined( __cplusplus ) && ( defined( _MSC_VER ) || defined( __MINGW32__ ) )
namespace linecook {

struct ConsoleVT {
  HANDLE         in_h,
                 out_h;
  INPUT_RECORD * input_events;
  DWORD          orig_in_mode,
                 orig_out_mode;
  size_t         events_len,
                 events_avail;
  char         * input_buf;
  size_t         input_off,
                 input_len,
                 input_avail;
  bool           sigwinch;

  ConsoleVT() : in_h( 0 ), out_h( 0 ), input_events( 0 ),
    orig_in_mode( 0 ), orig_out_mode( 0 ), events_len( 0 ), events_avail( 0 ),
    input_buf( 0 ), input_off( 0 ), input_len( 0 ), input_avail( 0 ),
    sigwinch( false ) {}
  /* set vt processing mode */
  bool init_vt( int in_fd,  int out_fd ) noexcept;
  /* set input and output virtual terminal mode */
  bool enable_raw_mode( void ) noexcept;
  /* disable input and output virtual terminal mode */
  bool disable_raw_mode( void ) noexcept;
  /* wait for input, return true if some events avail */
  bool poll_events( int time_ms ) noexcept;
  /* translate the events into vt key sequences */
  bool read_input( void ) noexcept;
};

}
#endif

#endif
