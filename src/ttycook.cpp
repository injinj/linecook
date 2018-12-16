#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <signal.h>
#include <termios.h>
#include <dirent.h>
#include <pwd.h>
#include <linecook/linecook.h>
#include <linecook/ttycook.h>


extern "C" {

const char *ttyp_def_prompt[] = {
  "[\\#]:\\w\\$ ", /* TTYP_PROMPT1  show term history wd */
  "> ",     /* TTYP_PROMPT2  no colors, text prompts */
  "<-i",     /* TTYP_R_INS    shows vi insert mode */
  "|=c",     /* TTYP_R_CMD    shows vi command mode */
  "<-e",     /* TTYP_R_EMACS  shows emacs mode */
  "/_?",     /* TTYP_R_SRCH   shows history search */
  "TAB",     /* TTYP_R_COMP   shows TAB completion */
  "[-]",     /* TTYP_R_VISU   shows visual select */
  "!@!1&",   /* TTYP_R_OUCH   shows bell */
  "[",       /* TTYP_R_SEL1   enclose selection */
  "]",       /* TTYP_R_SEL2 */
};

void
lc_tty_set_locale( void )
{
  setlocale( LC_ALL, "" );
}

TTYCook *
lc_tty_create( LineCook *lc )
{
  void *p = ::malloc( sizeof( linecook::TTY ) );
  if ( p == NULL )
    return NULL;
  return new ( p ) linecook::TTY( lc );
}

void
lc_tty_release( TTYCook *tty )
{
  static_cast<linecook::TTY *>( tty )->release();
  delete static_cast<linecook::TTY *>( tty );
}

int
lc_tty_set_default_prompt( TTYCook *tty,  TTYPrompt pnum )
{
  return lc_tty_set_prompt( tty, pnum, ttyp_def_prompt[ pnum ] );
}

int
lc_tty_set_default_prompts( TTYCook *tty )
{
  int i;
  for ( i = 0; i < TTYP_MAX; i++ )
    lc_tty_set_prompt( tty, (TTYPrompt) i, ttyp_def_prompt[ i ] );
  return 0;
}

int
lc_tty_set_prompt( TTYCook *tty,  TTYPrompt pnum,  const char *p )
{
  return static_cast<linecook::TTY *>( tty )->set_prompt( pnum, p );
}

int
lc_tty_init_fd( TTYCook *tty,  int in,  int out )
{
  return static_cast<linecook::TTY *>( tty )->init_fd( in, out );
}

static sig_atomic_t my_geom_changed;

static void
geom_changed_on_signal( int )
{
  my_geom_changed = 1;
}

int
lc_tty_init_sigwinch( TTYCook * /*tty*/ )
{
  ::signal( SIGWINCH, geom_changed_on_signal );
  return 0;
}

int
lc_tty_open_history( TTYCook *tty,  const char *fn )
{
  return static_cast<linecook::TTY *>( tty )->open_history( fn, true );
}

int
lc_tty_log_history( TTYCook *tty )
{
  return static_cast<linecook::TTY *>( tty )->log_history();
}

int
lc_tty_rotate_history( TTYCook *tty )
{
  return static_cast<linecook::TTY *>( tty )->rotate_history();
}

int
lc_tty_push_history( TTYCook *tty,  const char *line,  size_t len )
{
  return static_cast<linecook::TTY *>( tty )->push_history( line, len );
}

int
lc_tty_flush_history( TTYCook *tty )
{
  return static_cast<linecook::TTY *>( tty )->flush_history();
}

void
lc_tty_break_history( TTYCook *tty )
{
  return static_cast<linecook::TTY *>( tty )->break_history();
}

int
lc_tty_raw_mode( TTYCook *tty )
{
  return static_cast<linecook::TTY *>( tty )->raw_mode();
}

int
lc_tty_non_block( TTYCook *tty )
{
  return static_cast<linecook::TTY *>( tty )->non_block();
}

int
lc_tty_normal_mode( TTYCook *tty )
{
  return static_cast<linecook::TTY *>( tty )->normal_mode();
}

void
lc_tty_set_continue( TTYCook *tty,  int on )
{
  if ( on ) tty->state |= TTYS_CONTINUE_LINE;
  else      tty->state &= ~TTYS_CONTINUE_LINE;
}

int
lc_tty_push_line( TTYCook *tty,  const char *line,  size_t len )
{
  return static_cast<linecook::TTY *>( tty )->push_line( line, len );
}

int
lc_tty_get_line( TTYCook *tty )
{
  return static_cast<linecook::TTY *>( tty )->get_line();
}

int
lc_tty_poll_wait( TTYCook *tty,  int time_ms )
{
  return static_cast<linecook::TTY *>( tty )->poll_wait( time_ms );
}

int
lc_tty_init_geom( TTYCook *tty )
{
  int cols, lines;
  lc_tty_get_terminal_geom( tty->out_fd, &cols, &lines );
  if ( cols != tty->cols || lines != tty->lines ) {
    tty->state |= TTYS_GEOM_UPDATE;
    tty->cols  = cols;
    tty->lines = lines;
  }
  return 0;
}

void
lc_tty_get_terminal_geom( int fd,  int *cols,  int *lines )
{
  struct winsize ws;
  const char *l, *c;

  *lines = *cols = 0;
  if ( ::ioctl( fd, TIOCGWINSZ, &ws ) != -1 ) {
    *cols  = ws.ws_col;
    *lines = ws.ws_row;
  }
  if ( *lines == 0 || *cols == 0 ) {
    if ( (l = getenv( "LINES" )) != NULL &&
         (c = getenv( "COLUMNS" )) != NULL ) {
      *lines = atoi( l );
      *cols  = atoi( c );
    }
    if ( *lines == 0 )
      *lines = 25;
    if ( *cols == 0 )
      *cols = 80;
  }
}

void
lc_tty_clear_line( TTYCook *tty )
{
  return static_cast<linecook::TTY *>( tty )->clear_line();
}

static int
do_read( LineCook *state,  void *buf,  size_t buflen )
{
  linecook::TTY *tty = static_cast<linecook::TTY *>( state->closure );
  ssize_t n = ::read( tty->in_fd, buf, buflen );
  if ( n < 0 ) {
    if ( errno == EAGAIN || errno == EINTR )
      return 0;
    return -1;
  }
  return n;
}

static int
do_write( LineCook *state,  const void *buf,  size_t buflen )
{
  linecook::TTY *tty = static_cast<linecook::TTY *>( state->closure );
  ssize_t n = ::write( tty->out_fd, buf, buflen );
  if ( n < 0 ) {
    if ( errno == EAGAIN || errno == EINTR )
      return 0;
    return -1;
  }
  return n;
}

static size_t
catp( char *p,  const char *q,  const char *r,  const char *s = NULL )
{
  size_t i = 0;
  while ( *q != '\0' && i < 1023 )
    p[ i++ ] = *q++;
  while ( *r != '\0' && i < 1023 )
    p[ i++ ] = *r++;
  if ( s != NULL ) {
    while ( *s != '\0' && i < 1023 )
      p[ i++ ] = *s++;
  }
  p[ i ] = '\0';
  return i;
}

struct CompletePathStack {
  struct Path {
    Path * next;
    size_t base_sz;
    char   path[ 8 ];
  };
  Path * hd, * tl, * idx;
  CompletePathStack() : hd( 0 ), tl( 0 ), idx( 0 ) {}
  ~CompletePathStack() {
    Path * next;
    for ( Path *p = this->hd; p != NULL; p = next ) {
      next = p->next;
      ::free( p );
    }
  }
  void push( const char *path,  size_t len,  size_t sz ) {
    Path *p = (Path *) ::malloc( len + sizeof( Path ) );
    if ( p == NULL )
      return;
    p->next = NULL;
    p->base_sz = sz;
    ::memcpy( p->path, path, len );
    p->path[ len ] = '\0';
    if ( this->tl != NULL )
      this->tl->next = p;
    else
      this->hd = p;
    if ( this->idx == NULL )
      this->idx = p;
    this->tl = p;
  }
  const char *pop( size_t &sz ) {
    if ( this->idx == NULL )
      return NULL;
    const char * path = this->idx->path;
    sz = this->idx->base_sz;
    if ( this->hd != this->idx ) { /* idx in use, but el before is free */
      Path * next = this->hd->next;
      ::free( this->hd );
      this->hd = next;
    }
    this->idx = this->idx->next;
    return path;
  }
};

static bool
is_dotdir( const char *p,  size_t plen )
{
  if ( plen <= 2 ) {
    if ( plen == 1 && p[ 0 ] == '.' ) return true;
    if ( plen == 2 && p[ 0 ] == '.' && p[ 1 ] == '.' ) return true;
  }
  return false;
}

extern char ** environ;
int
lc_tty_file_completion( LineCook *state,  const char *buf,  size_t off,
                        size_t len,  int comp_type )
{
  DIR           * dirp;
  struct dirent * dp;
  int             cnt = 0;
  bool            is_dot = ( len > 0 && buf[ off ] == '.' );

  if ( comp_type == 'v' || ( len > 0 && buf[ off ] == '$' ) ) {
    if ( environ != NULL ) { /* env var complete */
      size_t i;
      char * p;
      for ( i = 0; environ[ i ] != NULL; i++ ) {
        if ( (p = strchr( environ[ i ], '=' )) != NULL ) {
          char var[ 1024 ];
          size_t sz = ( p - environ[ i ] );
          if ( sz + 1 <= sizeof( var ) ) {
            var[ 0 ] = '$';
            ::memcpy( &var[ 1 ], environ[ i ], sz );
            lc_add_completion( state, 'v', var, sz + 1 );
            cnt++;
          }
        }
      }
    }
  }
  else {
    CompletePathStack path_stack;
    const char *path_search = NULL, * e;
    const char *ptr = &buf[ off ];
    char path[ 1024 ], path2[ 1024 ], path3[ 1024 ];
    size_t path_sz, base_sz = 0;
    bool no_directory = true, is_base_path = true;
    if ( len > 0 ) {
      if ( len >= 1024 )
        return 0;
      for ( path_sz = len; ; ) { /* find if a directory prefix exists */
        if ( ptr[ path_sz - 1 ] == '/' ) {
          if ( path_sz + 1 < sizeof( path ) ) {
            if ( path_sz < len && ptr[ path_sz ] == '.' )
              is_dot = true;
            ::memcpy( path, ptr, path_sz );
            path[ path_sz ] = '\0';
            path_search = path;
            no_directory = false;
          }
          break;
        }
        if ( --path_sz == 0 )
          break;
      }
      if ( path_search == NULL ) {
        if ( ptr[ 0 ] == '~' ) {
          ::memcpy( path, ptr, len );
          path[ len ] = '\0';
          path_search = path;
          no_directory = false;
        }
      }
    }
    if ( path_search == NULL ) {
      if ( comp_type == 'e' ) { /* use PATH if no prefix */
        path_search = getenv( "PATH" );
        no_directory = true; /* don't include directory */
      }
      else if ( comp_type == 'd' ) { /* use CDPATH if no prefix */
        path_search = getenv( "CDPATH" );
        no_directory = false; /* include directory */
      }
      if ( path_search == NULL ) { /* if no path, use pwd */
        path_search = ".";
        no_directory = true;
      }
    }
    for (;;) {
      bool bad_path = false;
      if ( path_search == NULL ) {
        e = path_stack.pop( base_sz );
        if ( e == NULL )
          break;
        path_sz = ::strlen( e );
        is_base_path = false;
        if ( path_sz + 2 < sizeof( path2 ) ) {
          ::memcpy( path2, e, path_sz );
          path2[ path_sz ] = '\0';
        }
        else {
          bad_path = true;
        }
      }
      else {
        const char *next;
        e = (const char *) ::strchr( path_search, ':' );
        if ( e == NULL ) {
          e = &path_search[ ::strlen( path_search ) ];
          next = NULL;
        }
        else
          next = &e[ 1 ];

        path_sz = (size_t) ( e - path_search );
        if ( path_sz > 0 && path_sz + 2 < sizeof( path2 ) ) {
          ::memcpy( path2, path_search, path_sz );
          path2[ path_sz ] = '\0';
        }
        else {
          bad_path = true;
        }
        path_search = next;
      }
      if ( bad_path ) /* path too large, print error? */
        continue;
      char *dirpath = path2;
      size_t j = 0;
      /* check if home directory expansion (~u/) */
      if ( path2[ 0 ] == '~' ) {
        char user[ 1024 ], pw_dir[ 1024 ];
        const char *dstart = ::strchr( path2, '/' );
        const char *p = NULL;
        size_t len;
        if ( dstart == NULL )
          dstart = &path2[ ::strlen( path2 ) ];
        len = (size_t) ( dstart - &path2[ 1 ] );
        pw_dir[ 0 ] = '\0';
        if ( len > 0 ) {
          ::strncpy( user, &path2[ 1 ], len );
          user[ len ] = '\0';
        }
        else {
          user[ 0 ] = '\0';
          p = getenv( "HOME" ); /* my home */
          if ( p != NULL && ::strlen( p ) < sizeof( pw_dir ) ) {
            ::strcpy( pw_dir, p );
            p = pw_dir;
          }
        }
        if ( user[ 0 ] != '\0' ) { /* some user other home */
          bool found_pw = false;
          for (;;) {
            struct passwd *pw = getpwent();
            if ( pw == NULL )
              break;
            if ( ::strncmp( user, pw->pw_name, len ) == 0 ) {
              if ( dstart[ 0 ] == '\0' ) { /* if no '/', complete users */
                j = catp( pw_dir, "~", pw->pw_name, "/" );
                lc_add_completion( state, comp_type, pw_dir, j );
                cnt++;
              }
              /* dstart[ 0 ] == '/' */
              else if ( pw->pw_name[ len ] == '\0' ) {
                if ( ::strlen( pw->pw_dir ) < sizeof( pw_dir ) ) {
                  ::strcpy( pw_dir, pw->pw_dir );
                  found_pw = true;
                }
                break;
              }
            }
          }
          endpwent();
          if ( ! found_pw )
            continue;
          p = pw_dir;
        }
        /* concat home with path3 */
        if ( p != NULL ) {
          if ( dstart[ 0 ] == '\0' )
            j = catp( path3, p, "/" );
          else
            j = catp( path3, p, dstart );
          dirpath = path3;
        }
      }
      if ( path2[ path_sz - 1 ] != '/' ) {
        path2[ path_sz++ ] = '/';
        path2[ path_sz ] = '\0';
      }
      dirp = ::opendir( dirpath );
      if ( dirp != NULL ) {
        while ( (dp = ::readdir( dirp )) != NULL ) {
          struct stat s;
          unsigned char d_type = dp->d_type;
          bool      is_link    = ( d_type == DT_LNK );
          mode_t    st_mode    = 0;
          char    * completion;
          size_t    dlen       = ::strlen( dp->d_name ),
                    comp_sz;
          if ( no_directory ) {
            if ( is_base_path ) {
              completion = &path2[ path_sz ];
              comp_sz    = dlen;
            }
            else {
              completion = &path2[ base_sz ];
              comp_sz    = ( path_sz - base_sz ) + dlen;
            }
          }
          else {
            completion = path2;
            comp_sz    = path_sz + dlen;
          }
          if ( path_sz + dlen + 2 < sizeof( path2 ) &&
               j + dlen + 2 < sizeof( path3 ) ) {
            ::strcpy( &path2[ path_sz ], dp->d_name );
            /* resolve sym links and/or get st_mode */
            if ( is_link || comp_type == 'e' || d_type == 0 ) {
              if ( dirpath == path3 )
                ::strcpy( &path3[ j ], dp->d_name );
              if ( ::stat( dirpath, &s ) == 0 ) {
                switch ( s.st_mode & S_IFMT ) {
                  case S_IFDIR: d_type = DT_DIR; break;
                  case S_IFREG: d_type = DT_REG; break;
                  default: break;
                }
                st_mode = s.st_mode;
              }
            }
            /* any file complete */
            if ( comp_type == 'f' || comp_type == 's' || comp_type == 0 ) {
              bool is_dd = is_dotdir( dp->d_name, dlen ); /* . or .. */
              if ( ! is_dd || ( dlen > 1 && is_dot ) /* allow .. */) {
                if ( d_type == DT_DIR ) {
                  if ( comp_type == 's' && ! is_link && ! is_dd ) {
                    size_t sz = 0;
                    if ( no_directory ) {
                      if ( is_base_path )
                        sz = path_sz;
                      else
                        sz = base_sz;
                    }
                    path_stack.push( path2, path_sz + dlen, sz );
                  }
                  completion[ comp_sz++ ] = '/';
                }
                lc_add_completion( state, comp_type, completion, comp_sz );
                cnt++;
              }
            }
            else if ( comp_type == 'e' ) { /* exe complete */
              if ( ( d_type == DT_REG && ( st_mode & 0111 ) != 0 ) ||
                   d_type == DT_DIR ) {
                if ( d_type == DT_DIR )
                  completion[ comp_sz++ ] = '/';
                lc_add_completion( state, comp_type, completion, comp_sz );
                cnt++;
              }
            }
            else if ( comp_type == 'd' ) { /* dir complete */
              if ( d_type == DT_DIR ) {
                completion[ comp_sz++ ] = '/';
                lc_add_completion( state, comp_type, completion, comp_sz );
                cnt++;
              }
            }
          }
        }
        closedir( dirp );
      }
    }
  }
  return cnt;
}
} /* extern "C" */

using namespace linecook;


TTY::TTY( LineCook *lc )
{
  ::memset( this, 0, sizeof( *this ) );
  this->lc     = lc;
  this->in_fd  = -1;
  this->out_fd = -1;
  /* only use closure when _cb() are used */
  if ( lc->read_cb == NULL ) {
    lc->read_cb = do_read;
    lc->closure  = this;
  }
  if ( lc->write_cb == NULL ) {
    lc->write_cb = do_write;
    lc->closure  = this;
  }
  if ( lc->complete_cb == NULL )
    lc->complete_cb = lc_tty_file_completion;
}

TTY::~TTY()
{
  this->lc     = NULL;
  this->in_fd  = -1;
  this->out_fd = -1;
  if ( this->raw != NULL )        ::free( this->raw );
  if ( this->orig != NULL )       ::free( this->orig );
  if ( this->prompt_buf != NULL ) ::free( this->prompt_buf );
  if ( this->push_buf != NULL )   ::free( this->push_buf );
  if ( this->line != NULL )       ::free( this->line );
}

void
TTY::release( void )
{
  this->close_history();
}

int
TTY::set_prompt( TTYPrompt pnum,  const char *p )
{
  size_t slen = ( p != NULL ? ::strlen( p ) : 0 );
  if ( slen != this->len( pnum ) ||
       ::memcmp( p, this->ptr( pnum ), slen ) != 0 ) {
    size_t len, sz = 0;
    if ( slen > 0 ) {
      len  = align<size_t>( slen + 1 + sizeof( size_t ), 8 ),
      sz   = len + this->prompt_len;
      State *state = static_cast<linecook::State *>( this->lc );
      if ( ! state->realloc_buf8( &this->prompt_buf, this->prompt_buflen, sz ) )
        return -1;
      sz = this->prompt_len;
      this->prompt_len += len;
      ::memcpy( &this->prompt_buf[ sz ], &slen, sizeof( size_t ) );
      sz += sizeof( size_t );
      ::memcpy( &this->prompt_buf[ sz ], p, slen );
      this->prompt_buf[ sz + slen ] = '\0';
    }
    this->off[ pnum ] = sz;
    this->state |= TTYS_PROMPT_UPDATE;
    if ( ++this->prompt_cnt > 25 ) {
      size_t off2[ TTYP_MAX ];
      char * buf2 = this->prompt_buf;
      ::memcpy( off2, this->off, sizeof( this->off ) );
      ::memset( &this->off, 0, sizeof( this->off ) );
      this->prompt_buf    = NULL;
      this->prompt_len    = 0;
      this->prompt_buflen = 0;
      this->prompt_cnt    = 0;
      for ( size_t k = 0; k < TTYP_MAX; k++ ) {
        size_t i = off2[ k ];
        if ( i != 0 )
          if ( this->set_prompt( (TTYPrompt) k, &buf2[ i ] ) != 0 )
            return -1;
      }
      ::free( buf2 );
    }
  }
  return 0;
}

size_t TTY::len( TTYPrompt pnum ) const {
  size_t i = this->off[ pnum ];
  if ( i < sizeof( size_t ) ) return 0;
  return *((size_t *) (void *) &this->prompt_buf[ i - sizeof( size_t ) ]);
}
char * TTY::ptr( TTYPrompt pnum ) {
  size_t i = this->off[ pnum ];
  return ( i != 0 ? &this->prompt_buf[ i ] : NULL );
}

int
TTY::init_fd( int in,  int out )
{
  this->in_fd  = in;
  this->out_fd = out;
  return 0;
}

size_t
TTY::load_history_buffer( const char *buf,  size_t n,  size_t &line_cnt )
{
  const char *eol;
  size_t sz;
  /* find lines and add them */
  for ( size_t off = 0; ; off += sz ) {
    sz = n - off;
    if ( sz == 0 ) /* all consumed */
      return n;
    eol = (const char *) ::memchr( &buf[ off ], '\n', sz );
    if ( eol == NULL )
      return off; /* last line not terminated */
    sz = &eol[ 1 ] - &buf[ off ];
    if ( sz > 1 ) {
      while ( eol > &buf[ off ] && isspace( *--eol ) )
        ;
      if ( ! isspace( buf[ off ] ) || eol > &buf[ off ] ) {
        lc_add_history( this->lc, &buf[ off ], &eol[ 1 ] - &buf[ off ] );
        line_cnt++;
      }
    }
  }
}

size_t
TTY::read_history( int rfd,  size_t max_len,  size_t &line_cnt )
{
  char buf[ 64 * 1024 ]; /* max hist line size */
  size_t amount_consumed = 0, file_off = 0;
  ssize_t n;

  for ( size_t off = 0; ; ) {
    size_t sz = max_len;
    if ( max_len == 0 || max_len - file_off > sizeof( buf ) )
      sz = sizeof( buf );
    if ( sz == 0 )
      return amount_consumed;
    n = ::read( rfd, &buf[ off ], sizeof( buf ) - off );
    if ( n <= 0 )
      return amount_consumed;
    file_off += n;
    off = this->load_history_buffer( buf, off + n, line_cnt );
    if ( off != 0 ) {
      sz = (size_t) n - off; /* leftover, move to front */
      if ( sz > 0 )
        ::memmove( buf, &buf[ off ], sz );
      amount_consumed += off;
      off = sz;
    }
    else {
      amount_consumed += (size_t) n; /* discard, line is larger than buf */
    }
  }
}

#define ROTATEDBG(x)
int
TTY::open_history( const char *fn,  bool reinitialize )
{
  char   lckpath[ 1024 ],
         path[ 1024 ];
  size_t len;
  char * fncpy;
  size_t i, j;
  int    rfd, afd = -1;
  
  ROTATEDBG(( stderr, "\r\nopening %s\r\n", fn ));
  if ( fn == NULL )
    return 0; /* close history ?? */
  len = ::strlen( fn );
  /* lcoks and rotations append digits to the filename, need 1024 chars */
  if ( len + 24 > 1024 ) {
    fprintf( stderr, "Filename \"%s\" too large\n", fn );
    return -1;
  }
  fncpy = (char *) ::malloc( len + 1 );
  if ( fncpy == NULL )
    return -1;
  ::strcpy( fncpy, fn );

  if ( this->acquire_history_lck( fn, lckpath ) != 0 )
    goto failed;

  afd = ::open( fn, O_WRONLY | O_APPEND | O_CLOEXEC, 0666 );
  if ( afd < 0 ) { /* doesn't exist, create it atomically */
    afd = ::open( fn, O_WRONLY | O_CREAT | O_EXCL | O_APPEND | O_CLOEXEC,
                  0666 );
  }
  else if ( reinitialize ) { /* exists, look for hist rotations */
    /* find the last file.N */
    ::strcpy( path, fn );
    path[ len ] = '.';
    for ( i = 1; ; i++ ) {
      uint_to_string( i, &path[ len + 1 ] );
      if ( ::access( path, R_OK | W_OK ) != 0 )
        break;
    }
    /* load files from N to 1 */
    for ( j = i - 1; j > 0; j-- ) {
      uint_to_string( j, &path[ len + 1 ] );
      rfd = ::open( path, O_RDONLY | O_CLOEXEC );
      if ( rfd >= 0 ) {
        size_t line_cnt = 0;
        this->read_history( rfd, 0, line_cnt );
        ::close( rfd );
      }
    }
  }
  ::unlink( lckpath );
  if ( afd < 0 )
    goto failed;

  this->hist.filename            = fncpy;
  this->hist.fsize               = 0;
  this->hist.line_cnt            = 0;
  this->hist.rotate_line_cnt     = 1000;
  this->hist.last_check          = ::time( NULL );
  this->hist.last_rotation_check = this->hist.last_check;
  this->hist.fd                  = afd;
  if ( this->check_history() != 0 ) {
    this->close_history();
    return -1;
  }
  if ( reinitialize )
    return lc_compress_history( this->lc );
  return 0;

failed:;
  ::perror( fn );
  ::free( fncpy );
  if ( afd >= 0 )
    ::close( afd );
  return -1;
}

int
TTY::check_history( void )
{
  TTYHistory &h = this->hist;
  struct stat x, y;
  int rfd, afd;
  bool is_rotated = false;

  if ( h.filename == NULL )
    return 0;
  if ( ::fstat( h.fd, &x ) != 0 )
    return -1;
  /* check rotation and load lines logged since the last read */
  if ( h.fsize < (size_t) x.st_size ||
       ( this->state & TTYS_ROTATE_HISTORY ) != 0 ||
       h.last_rotation_check + 60 < h.last_check ) {
    h.last_rotation_check = h.last_check;
    rfd = ::open( h.filename, O_RDONLY | O_CLOEXEC );
    if ( rfd >= 0 ) {
      /* check that the files refer to the same ino */
      if ( ::fstat( rfd, &y ) == 0 && x.st_ino != y.st_ino ) {
        char path[ 1024 ];
        ::close( rfd );
        rfd = -1;
        is_rotated = true;
        /* try to find the old history to read the rest of that */
        ::strcpy( path, this->hist.filename );
        ::strcat( path, ".1" );
        rfd = ::open( path, O_RDONLY | O_CLOEXEC );
        if ( rfd >= 0 ) {
          /* if ino matches, this is the old one */
          if ( ::fstat( rfd, &y ) != 0 || x.st_ino != y.st_ino ) {
            ::close( rfd );
            rfd = -1;
          }
        }
      }
      /* the files match at this point, read the rest, if any */
      if ( rfd >= 0 ) {
        if ( (size_t) ::lseek( rfd, h.fsize, SEEK_SET ) == h.fsize ) {
          size_t sz = this->read_history( rfd, 0, h.line_cnt );
          h.fsize += sz;
          if ( sz > 0 )
            this->state |= TTYS_HIST_UPDATE;
        }
        ::close( rfd );
      }
    }
    /* open the new history file if rotated */
    if ( is_rotated ) {
      ROTATEDBG(( stderr, "\r\ndetected rotated %s\r\n", h.filename ));
      afd = ::open( h.filename, O_WRONLY | O_APPEND | O_CLOEXEC, 0666 );
      if ( afd >= 0 ) {
        ::close( this->hist.fd );
        this->hist.fsize      = 0;
        this->hist.line_cnt   = 0;
        this->hist.last_check = 0; /* cause check_history to be called again */
        this->hist.fd         = afd;
        this->state &= ~TTYS_ROTATE_HISTORY;
      }
    }
  }
  return 0;
}

int
TTY::acquire_history_lck( const char *filename,  char *lckpath )
{
  TTYHistory &h = this->hist;
  size_t len;
  int    tmpfd = -1;

  if ( filename == NULL )
    filename = h.filename;
  len = ::strlen( filename );
  ::memcpy( lckpath, filename, len );
  ::strcpy( &lckpath[ len ], ".lck" );
  /* try a maximum of 10 seconds */
  for ( int i = 0; i < 100; i++ ) {
    tmpfd = ::open( lckpath, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0666 );
    if ( tmpfd >= 0 )
      break;
    if ( errno != EEXIST )
      break;
    ::poll( NULL, 0, 100 );
  }
  if ( tmpfd < 0 ) {
    ::perror( lckpath );
    return -1;
  }
  ::close( tmpfd );
  return 0;
}

int
TTY::rotate_history( void )
{
  TTYHistory &h = this->hist;
  char        newpath[ 1024 ],
              oldpath[ 1024 ],
              lckpath[ 1024 ];
  struct stat x, y;
  size_t      i, j, len;
  int         status;

  ROTATEDBG(( stderr, "\r\nrotating %s\r\n", h.filename ));
  this->state &= ~TTYS_ROTATE_HISTORY;
  if ( h.filename == NULL )
    return 0;
  /* check size of file before and after lock acquire */
  ::strcpy( newpath, h.filename );
  if ( ::stat( newpath, &x ) != 0 ) {
    this->close_history();
    return this->open_history( newpath, false );
  }
  if ( this->acquire_history_lck( NULL, lckpath ) != 0 )
    return -1;

  if ( ::stat( newpath, &y ) != 0 ) {
    ::perror( newpath );
    ::unlink( lckpath );
    return -1;
  }
  /* if file was already rotated, size will be less than old history */
  if ( x.st_size > y.st_size ) {
    ::unlink( lckpath );
    this->close_history();
    return this->open_history( newpath, false );
  }

  /* find the last file.N */
  len = ::strlen( newpath );
  newpath[ len ] = '.';
  for ( i = 1; ; i++ ) {
    uint_to_string( i, &newpath[ len + 1 ] );
    if ( ::access( newpath, R_OK | W_OK ) != 0 )
      break;
  }
  /* rename files from N to N + 1 */
  ::memcpy( oldpath, h.filename, len );
  oldpath[ len ] = '.';
  for ( j = i - 1; j > 0; j-- ) {
    uint_to_string( j, &oldpath[ len + 1 ] );
    if ( (status = ::rename( oldpath, newpath )) != 0 ) {
      ::perror( oldpath );
      goto failed;
    }
    ::strcpy( &newpath[ len + 1 ], &oldpath[ len + 1 ] );
  }
  /* rename the current history to history.1 */
  if ( (status = ::rename( h.filename, newpath )) == 0 ) {
    ::strcpy( newpath, h.filename );
    this->close_history();
    ::unlink( lckpath );
    status = this->open_history( newpath, false );
  }
failed:;
  if ( status != 0 ) {
    ::perror( newpath );
    ::unlink( lckpath );
  }

  return status;
}

int
TTY::log_hist( char *line,  size_t len )
{
  TTYHistory &h = this->hist;
  struct stat x;
  size_t  i;
  ssize_t n;

  /* load any history that occurred in another shell */
  this->check_history();
  for ( i = 0; i < len; i++ ) {
    if ( ! isspace( line[ i ] ) )
      break;
  }
  if ( i == len )
    return 0;
  lc_add_history( this->lc, line, len );

  if ( h.filename != NULL ) {
    line[ len ] = '\n'; /* replace null with nl temporarily */
    /* anything written between check_history and fstat will not be loaded into
     * the history buffer */
    n = ::write( h.fd, line, len + 1 );
    if ( ::fstat( h.fd, &x) == 0 )
      h.fsize = x.st_size;
    line[ len ] = '\0';
    h.last_check = x.st_mtime;
    h.line_cnt++;

    if ( (size_t) n != len + 1 ) /* might be full */
      return -1;

    if ( h.line_cnt >= h.rotate_line_cnt ) /* rotate after an idle period */
      this->state |= TTYS_ROTATE_HISTORY;
  }
  return 0;
}

int
TTY::log_history( void )
{
  return this->log_hist( this->line, this->line_len );
}

int
TTY::push_history( const char *line,  size_t len )
{
  if ( len > 0 ) {
    if ( line[ len - 1 ] == '\\' )
      if ( len == 1 || line[ len - 2 ] != '\\' )
        len--;
  }
  if ( len > 0 ) {
    TTYHistory &h = this->hist;
    State *state = static_cast<linecook::State *>( this->lc );
    size_t sz = len + h.hist_len;
    if ( ! state->realloc_buf8( &h.hist_buf, h.hist_buflen, sz + 1 ) )
      return -1;
    ::memcpy( &h.hist_buf[ h.hist_len ], line, len );
    h.hist_len = sz;
    h.hist_buf[ sz ] = '\0';
  }
  return 0;
}

int
TTY::flush_history( void )
{
  TTYHistory &h = this->hist;
  int status = this->log_hist( h.hist_buf, h.hist_len );
  h.hist_len = 0;
  return status;
}

void
TTY::break_history( void )
{
  TTYHistory &h = this->hist;
  h.hist_len = 0;
  this->push_len = 0;
}

int
TTY::close_history( void )
{
  TTYHistory &h = this->hist;
  int fd = h.fd;

  if ( h.filename != NULL ) {
    ::free( h.filename );
    if ( h.hist_buf ) ::free( h.hist_buf );
    ::memset( &h, 0, sizeof( TTYHistory ) );
    return ::close( fd );
  }
  return 0;
}

int
TTY::raw_mode( void )
{
  if ( this->in_fd == -1 ) { /* if no in_fd */
    this->set( TTYS_IS_RAW );
    return 0;
  }
  if ( this->orig == NULL ) {
    if ( (this->orig = ::malloc( sizeof( struct termios ) )) == NULL )
      return -1;
    ::memset( this->orig, 0, sizeof( struct termios ) );
  }
  if ( this->raw == NULL ) {
    if ( (this->raw = ::malloc( sizeof( struct termios ) )) == NULL )
      return -1;
    ::memset( this->raw, 0, sizeof( struct termios ) );
  }
  if ( this->test( TTYS_IS_RAW ) == 0 )  {
    struct termios &o = *(struct termios *) this->orig;
    struct termios &r = *(struct termios *) this->raw;

    if ( ::tcgetattr( this->in_fd, &o ) == -1 ) {
      ::perror( "tcgetattr (terminal raw mode failed)" );
      return -1;
    }

    ::memcpy( &r, &o, sizeof( struct termios ) );
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    r.c_iflag &= ~( BRKINT | ICRNL | INPCK | ISTRIP | IXON );
    r.c_oflag &= ~( OPOST ); /* output modes - disable post processing */
    r.c_cflag |= ( CS8 );    /* control modes - set 8 bit chars */
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    r.c_lflag &= ~( ECHO | ICANON | IEXTEN | ISIG );
    /* control chars - set return condition: min number of bytes and timer.
     * We want read to return every single byte, without timeout. */
    r.c_cc[ VMIN ]  = 1;
    r.c_cc[ VTIME ] = 0; /* 1 byte, no timer */

    /* put terminal in raw mode after flushing */
    if ( ::tcsetattr( this->in_fd, TCSAFLUSH, &r ) < 0 ) {
      ::perror( "tcsetattr (terminal raw mode failed)" );
      return -1;
    }
    this->set( TTYS_IS_RAW );
    lc_tty_init_geom( this );
  }
  return 0;
}

int
TTY::non_block( void )
{
  if ( this->test( TTYS_IS_NONBLOCK ) == 0 ) {
    if ( this->in_fd != -1 ) {
      this->in_mode  = ::fcntl( this->in_fd, F_GETFL );
      this->in_mode &= ~O_NONBLOCK;
    }
    if ( this->out_fd != -1 ) {
      this->out_mode  = ::fcntl( this->out_fd, F_GETFL );
      this->out_mode &= ~O_NONBLOCK; /* must be off in normal mode */
    }
    if ( this->in_fd != -1 ) {
      ::fcntl( this->in_fd, F_SETFL, this->in_mode | O_NONBLOCK );
    }
    if ( this->out_fd != -1 ) {
      ::fcntl( this->out_fd, F_SETFL, this->out_mode | O_NONBLOCK );
    }
    this->set( TTYS_IS_NONBLOCK );
  }
  return 0;
}

int
TTY::reset_raw( void )
{
  if ( this->test( TTYS_IS_RAW ) != 0 ) {
    if ( this->in_fd != -1 ) {
      if ( this->orig == NULL )
        return -1;
      struct termios &o = *(struct termios *) this->orig;
      ::tcsetattr( this->in_fd, TCSAFLUSH, &o );
    }
    this->clear( TTYS_IS_RAW );
  }
  return 0;
}

int
TTY::reset_non_block( void )
{
  if ( this->test( TTYS_IS_NONBLOCK ) != 0 ) {
    if ( this->in_fd != -1 )
      ::fcntl( this->in_fd, F_SETFL, this->in_mode );
    if ( this->out_fd != -1 )
      ::fcntl( this->out_fd, F_SETFL, this->out_mode );
    this->clear( TTYS_IS_NONBLOCK );
  }
  return 0;
}

int
TTY::normal_mode( void )
{
  bool b;
  b  = ( this->reset_raw() == 0 );
  b &= ( this->reset_non_block() == 0 );
  return b ? 0 : -1;
}

void
TTY::clear_line( void )
{
  lc_clear_line( this->lc );
}

int
TTY::push_line( const char *line,  size_t len )
{
  State *state = static_cast<linecook::State *>( this->lc );
  size_t sz = len + this->push_len;
  if ( ! state->realloc_buf8( &this->push_buf, this->push_buflen, sz ) )
    return -1;
  ::memcpy( &this->push_buf[ this->push_len ], line, len );
  this->push_len = sz;
  return 0;
}

int
TTY::get_line( void )
{
  static const int fl = TTYS_IS_NONBLOCK | TTYS_IS_RAW;
  int n;
  /* check the raw modes are set */
  if ( this->test( fl ) != fl &&
       ( this->raw_mode() != 0 || this->non_block() != 0 ) ) {
    this->lc_status = LINE_STATUS_RD_FAIL;
    return -1;
  }
  /* must set prompts before entering lc_get_line(), it will dereference them */
  if ( this->test( TTYS_PROMPT_UPDATE ) ) {
    n = lc_set_prompt( this->lc, this->ptr( TTYP_PROMPT1 ),
                                 this->len( TTYP_PROMPT1 ),
                                 this->ptr( TTYP_PROMPT2 ),
                                 this->len( TTYP_PROMPT2 ) );
    if ( n == 0 )
      n = lc_set_right_prompt( this->lc,
                           this->ptr( TTYP_R_INS ),  this->len( TTYP_R_INS ),
                           this->ptr( TTYP_R_CMD ),  this->len( TTYP_R_CMD ),
                           this->ptr( TTYP_R_EMACS ),this->len( TTYP_R_EMACS ),
                           this->ptr( TTYP_R_SRCH ), this->len( TTYP_R_SRCH ),
                           this->ptr( TTYP_R_COMP ), this->len( TTYP_R_COMP ),
                           this->ptr( TTYP_R_VISU ), this->len( TTYP_R_VISU ),
                           this->ptr( TTYP_R_OUCH ), this->len( TTYP_R_OUCH ),
                           this->ptr( TTYP_R_SEL1 ), this->len( TTYP_R_SEL1 ),
                           this->ptr( TTYP_R_SEL2 ), this->len( TTYP_R_SEL2 ) );
    if ( n == 0 )
      this->clear( TTYS_PROMPT_UPDATE | TTYS_HIST_UPDATE );
    else {
      this->lc_status = LINE_STATUS_BAD_PROMPT;
      return -1;
    }
  }
  /* if not using poll_wait approx_ticks will be zero */
  if ( this->approx_ticks == 0 || this->approx_ticks >= 5000 ) {
    if ( this->hist.filename != NULL ) {
      time_t t = ::time( NULL );
      if ( this->hist.last_check + 5 < t ) {
        this->approx_ticks = 0;
        this->hist.last_check = t;
        this->check_history();
        if ( ( this->state & TTYS_ROTATE_HISTORY ) != 0 )
          this->rotate_history();
      }
    }
  }
  /* change the term geom if changed */
  if ( my_geom_changed ) {
    my_geom_changed = 0;
    lc_tty_init_geom( this );
  }
  if ( this->test( TTYS_GEOM_UPDATE | TTYS_HIST_UPDATE ) ) {
    lc_set_geom( this->lc, this->cols, this->lines );
    this->clear( TTYS_GEOM_UPDATE | TTYS_HIST_UPDATE );
  }
  /* fetch the line, if prompt has a time in it, that will update  */
  if ( this->test( TTYS_CONTINUE_LINE ) )
    this->lc_status = lc_continue_get_line( this->lc );
  else
    this->lc_status = lc_get_line( this->lc );
  /* push line back into buffer */
  if ( this->lc_status == LINE_STATUS_EXEC ) {
    int    n   = lc_line_length( this->lc );
    size_t off = ( n > 0 ? n : 0 ) + this->push_len;
    if ( off + 1 > this->line_buflen ) {
      State *state = static_cast<linecook::State *>( this->lc );
      if ( ! state->realloc_buf8( &this->line, this->line_buflen, off + 1 ) ) {
        this->lc_status = LINE_STATUS_ALLOC_FAIL;
        return -1;
      }
    }
    this->line_len = off;
    off = 0;
    if ( this->push_len > 0 ) {
      ::memcpy( this->line, this->push_buf, this->push_len );
      off = this->push_len;
      this->push_len = 0;
    }
    if ( n > 0 ) {
      lc_line_copy( this->lc, &this->line[ off ] );
      off += n;
    }
    this->line[ off ] = '\0';
  }
  switch ( this->lc_status ) {
    case LINE_STATUS_WR_EAGAIN: this->set( TTYS_POLL_OUT ); /* FALLTHRU */
    case LINE_STATUS_RD_EAGAIN:            /* need poll() */
    case LINE_STATUS_INTERRUPT:            /* ctrl-c typed */
    case LINE_STATUS_SUSPEND:              /* ctrl-z typed */
    case LINE_STATUS_OK:        return 0;  /* no change */
    case LINE_STATUS_EXEC:      return 1;  /* line ready */
    default:                    return -1; /* other fail status */
  }
}

int
TTY::poll_wait( int time_ms )
{
  struct pollfd fdset;
  int n;
  /* only poll when on these conditions */
  if ( this->lc_status == LINE_STATUS_WR_EAGAIN ||
       this->lc_status == LINE_STATUS_RD_EAGAIN ) {
    if ( time_ms > 0 ) {
      time_ms = lc_max_timeout( this->lc, time_ms );
      this->approx_ticks += time_ms;
    }
    if ( this->test( TTYS_POLL_OUT ) ) {
      fdset.fd      = this->out_fd;
      fdset.events  = POLLOUT | POLLERR;
      fdset.revents = 0;
    }
    else {
      fdset.fd      = this->in_fd;
      fdset.events  = POLLIN | POLLHUP;
      fdset.revents = 0;
    }
    n = ::poll( &fdset, 1, time_ms );
    if ( n == 0 ) /* no change */
      return 0;
    if ( n < 0 ) {
      if ( errno == EINTR ) /* if got a signal */
        return 0;
      /* some other error */
      this->lc_status = LINE_STATUS_RD_FAIL;
      return -1;
    }
    /* fd ready, if writing, clear poll out */
    if ( this->test( TTYS_POLL_OUT ) )
      this->clear( TTYS_POLL_OUT );
    this->lc_status = LINE_STATUS_OK; /* ready for i/o */
  }
  return 1; /* caller should call get_line() on 1 */
}

