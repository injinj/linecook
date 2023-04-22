#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#if defined( _MSC_VER ) || defined( __MINGW32__ )
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include <linecook/linecook.h>
#include <linecook/ttycook.h>

using namespace linecook;

#if defined( _MSC_VER ) || defined( __MINGW32__ )
typedef SOCKET socket_t;
typedef HANDLE poll_event_t;
typedef int addrlen_t;
static bool is_invalid( socket_t s ) { return s == INVALID_SOCKET; }
static poll_event_t open_listener_event( socket_t s ) {
  u_long mode = 1;
  ioctlsocket( s, FIONBIO, &mode );
  WSAEVENT event = WSACreateEvent();
  WSAEventSelect( s, event, FD_ACCEPT );
  return event;
}
static poll_event_t open_connection_event( socket_t s ) {
  u_long mode = 1;
  ioctlsocket( s, FIONBIO, &mode );
  WSAEVENT event = WSACreateEvent();
  WSAEventSelect( s, event, FD_READ | FD_CLOSE );
  return event;
}
static void close_listener_event( poll_event_t event, socket_t s ) {
  WSACloseEvent( event );
  closesocket( s );
}
static void close_connection_event( poll_event_t event, socket_t s ) {
  WSACloseEvent( event );
  closesocket( s );
}
static void reset_listener_event( poll_event_t event ) {
  WSAResetEvent( event );
}
static void reset_connection_event( poll_event_t event ) {
  WSAResetEvent( event );
}
static void reset_console_event( poll_event_t ) {}
#ifdef _MSC_VER
#define STDIN_FILENO _fileno( stdin )
#define STDOUT_FILENO _fileno( stdout )
#endif
#else
typedef int socket_t;
typedef struct pollfd poll_event_t;
typedef socklen_t addrlen_t;
static bool is_invalid( socket_t s ) { return s < 0; }
static poll_event_t open_listener_event( socket_t s ) {
  ::fcntl( s, F_SETFL, O_NONBLOCK | ::fcntl( s, F_GETFL ) );
  poll_event_t ev;
  ev.fd      = s;
  ev.events  = POLLIN;
  ev.revents = 0;
  return ev;
}
static poll_event_t open_connection_event( socket_t s ) {
  ::fcntl( s, F_SETFL, O_NONBLOCK | ::fcntl( s, F_GETFL ) );
  poll_event_t ev;
  ev.fd      = s;
  ev.events  = POLLIN | POLLHUP;
  ev.revents = 0;
  return ev;
}
static void close_listener_event( poll_event_t, socket_t s ) { ::close( s ); }
static void close_connection_event( poll_event_t, socket_t s ) { ::close( s ); }
static void reset_listener_event( poll_event_t &ev ) { ev.revents = 0; }
static void reset_connection_event( poll_event_t &ev ) { ev.revents = 0; }
static void reset_console_event( poll_event_t &ev ) { ev.revents = 0; }
#endif
struct Poller;

static poll_event_t null_event;
struct EventDispatch {
  Poller & poll;
  int      index;
  EventDispatch( Poller &p ) : poll( p ), index( -1 ) {}
  virtual bool         dispatch( void )    { return true; }
  virtual poll_event_t get_event( void )   { return null_event; }
  virtual void         close( void ) {}
};

#ifdef _MSC_VER
#define __attribute__(x)
#endif
struct Console : public EventDispatch {
  LineCook & lc;
  TTYCook  & tty;
  int        msg_count;

  Console( Poller &p, LineCook &l,  TTYCook &t ) 
    : EventDispatch( p ), lc( l ), tty( t ), msg_count( 0 ) {}

  virtual bool         dispatch( void );
  virtual poll_event_t get_event( void );
  int                  print( const char *fmt, ... )
    __attribute__((format(printf,2,3)));
};

struct Listener : public EventDispatch {
  socket_t  sock;
  Console & console;

  void * operator new( size_t, void *ptr ) { return ptr; }
  Listener( Poller &p, socket_t s, Console &c )
    : EventDispatch( p ), sock( s ), console( c ) {}
  virtual bool         dispatch( void );
  virtual poll_event_t get_event( void );
  virtual void         close( void );
};

struct Connection : public EventDispatch {
  socket_t  sock;
  Console & console;

  void * operator new( size_t, void *ptr ) { return ptr; }
  Connection( Poller &p, socket_t s, Console &c )
    : EventDispatch( p ), sock( s ), console( c ) {}

  virtual bool         dispatch( void );
  virtual poll_event_t get_event( void );
  virtual void         close( void );
};

struct Poller {
  EventDispatch * events[ 64 ];
  poll_event_t    poll_set[ 64 ];
  int             nevents,
                  dispatch_cnt;
  bool            quit;

  Poller() : nevents( 0 ), dispatch_cnt( 0 ), quit( false ) {}

  void add( EventDispatch *ev );
  void remove( EventDispatch *ev );
  bool next_event( EventDispatch *&ev, int time_ms );
};


void
Poller::add( EventDispatch *ev )
{
  ev->index                       = this->nevents;
  this->events[ this->nevents ]   = ev;
  this->poll_set[ this->nevents ] = ev->get_event();
  this->nevents++;
}

void
Poller::remove( EventDispatch *ev )
{
  if ( ev->index < this->nevents - 1 ) {
    ::memmove( &this->events[ ev->index ], &this->events[ ev->index + 1 ],
               sizeof( this->events[ 0 ] ) *
                 ( this->nevents - ( ev->index + 1 ) ) );
    ::memmove( &this->poll_set[ ev->index ], &this->poll_set[ ev->index + 1 ],
               sizeof( this->poll_set[ 0 ] ) *
                 ( this->nevents - ( ev->index + 1 ) ) );
  }
  this->nevents--;
}

bool
Poller::next_event( EventDispatch *&ev, int time_ms )
{
#if defined( _MSC_VER ) || defined( __MINGW32__ )
  DWORD cnt;
  cnt = WaitForMultipleObjects( this->nevents, this->poll_set, FALSE, time_ms );
  if ( cnt >= WAIT_OBJECT_0 && cnt < WAIT_OBJECT_0 + this->nevents ) {
    cnt -= WAIT_OBJECT_0;
    ev = this->events[ cnt ];
    this->dispatch_cnt++;
    return true;
  }
  return false;
#else
  for (;;) {
    int n;
    for ( n = 0; n < this->nevents; n++ ) {
      if ( this->poll_set[ n ].revents != 0 ) {
        ev = this->events[ n ];
        this->dispatch_cnt++;
        return true;
      }
    }
    n = ::poll( this->poll_set, this->nevents, time_ms );
    if ( n == 0 )
      return false;
  }
#endif
}

poll_event_t
Console::get_event( void )
{
#if defined( _MSC_VER ) || defined( __MINGW32__ )
  return GetStdHandle( STD_INPUT_HANDLE );
#else
  poll_event_t ev;
  ev.fd      = STDIN_FILENO;
  ev.events  = POLLIN | POLLRDHUP;
  ev.revents = 0;
  return ev;
#endif
}

bool
Console::dispatch( void )
{
  int n = lc_tty_get_line( &this->tty );
  if ( n < 0 )
    return false;
  if ( this->tty.lc_status == LINE_STATUS_EXEC ) {
    lc_tty_normal_mode( &this->tty );
    this->print( "d=%d,n=%d,m=%d[%s]\n",
                 this->poll.dispatch_cnt, this->poll.nevents, this->msg_count,
                 this->tty.line );
    if ( this->tty.line_len == 1 && this->tty.line[ 0 ] == 'q' ) {
      this->poll.quit = true;
      return false;
    }
    lc_tty_log_history( &this->tty );
  }
  if ( this->tty.lc_status > LINE_STATUS_RD_EAGAIN )
    return true;
  reset_console_event( this->poll.poll_set[ this->index ] );
  return false;
}

int
Console::print( const char *fmt, ... )
{
  this->msg_count++;

  va_list args;
  va_start( args, fmt );
  int n = lc_tty_printv( &this->tty, fmt, args );
  va_end( args );
  return n;
}

poll_event_t
Listener::get_event( void )
{
  return open_listener_event( this->sock );
}

void
Listener::close( void )
{
  close_listener_event( this->poll.poll_set[ this->index ], this->sock );
  this->poll.remove( this );
}

poll_event_t
Connection::get_event( void )
{
  return open_connection_event( this->sock );
}

void
Connection::close( void )
{
  close_connection_event( this->poll.poll_set[ this->index ], this->sock );
  this->poll.remove( this );
}

bool
Listener::dispatch( void )
{
  struct sockaddr_in addr;
  addrlen_t          addrlen = sizeof( addr );

  socket_t s = ::accept( this->sock, (struct sockaddr *) &addr, &addrlen );
  if ( is_invalid( s ) ) {
    reset_listener_event( this->poll.poll_set[ this->index ] );
    return false;
  }
  char   buf[ 64 ];
  ::snprintf( buf, sizeof( buf ), "%u.%u.%u.%u:%u",
              ((uint8_t *) &addr.sin_addr.s_addr)[ 0 ],
              ((uint8_t *) &addr.sin_addr.s_addr)[ 1 ],
              ((uint8_t *) &addr.sin_addr.s_addr)[ 2 ],
              ((uint8_t *) &addr.sin_addr.s_addr)[ 3 ],
              ntohs( addr.sin_port ) );

  this->console.print( "accept from %s\n", buf );
  this->poll.add( new ( ::malloc( sizeof( Connection ) ) )
    Connection( this->poll, s, this->console ) );
  return true;
}

bool
Connection::dispatch( void )
{
  char buf[ 128 ];
  int  buflen = sizeof( buf ), n;
  n           = recv( this->sock, (char *) buf, (int) buflen, 0 );
  if ( n <= 0 ) {
    if ( n == 0 ) {
      this->console.print( "Close %u\n", this->index );
      this->close();
    }
    else {
      reset_connection_event( this->poll.poll_set[ this->index ] );
    }
    return false;
  }
  const char *nl = ( buf[ n - 1 ] != '\n' ? "\n" : "" );
  this->console.print( "%.*s%s", n, buf, nl );
  return true;
}

int
main( void )
{
  socket_t           sock;
  struct sockaddr_in sockaddr;
  int                status;
  Poller             poll;
  LineCook         * lc  = lc_create_state( 80, 25 );
  TTYCook          * tty = lc_tty_create( lc );
  Console            console( poll, *lc, *tty );

#if defined( _MSC_VER ) || defined( __MINGW32__ )
  WORD               ver;
  WSADATA            wdata;
  ver = MAKEWORD( 2, 2 );
  WSAStartup( ver, &wdata );
#endif

  sock                     = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
  sockaddr.sin_family      = AF_INET;
  sockaddr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
  sockaddr.sin_port        = htons( 9000 );
  status = bind( sock, (struct sockaddr *) &sockaddr, sizeof( sockaddr ) );

  if ( status == 0 )
    status = listen( sock, 5 );
  if ( status != 0 ) {
    fprintf( stderr, "fail status %d\n", status );
    return 1;
  }

  lc_tty_set_locale();
  lc_tty_init_fd( tty, STDIN_FILENO, STDOUT_FILENO );
  lc_tty_set_color_prompts( tty );
  /*lc_tty_set_prompt( tty, TTYP_PROMPT1, prompt );*/
  lc_tty_init_geom( tty );
  lc_tty_init_sigwinch( tty );
  lc_tty_show_prompt( tty );

  poll.add( &console );
  poll.add( new ( ::malloc( sizeof( Listener ) ) )
    Listener( poll, sock, console ) );
  while ( ! poll.quit ) {
    EventDispatch *ev;
    if ( poll.next_event( ev, 1000 ) ) {
      while ( ev->dispatch() )
        ;
    }
  }
  lc_tty_normal_mode( tty );
  printf( "\nbye\n" );
  fflush( stdout );
  return 0;
}
