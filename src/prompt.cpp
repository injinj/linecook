#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <pwd.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <linecook/linecook.h>
#include <linecook/keycook.h>
#include <linecook/xwcwidth9.h>

extern "C"
int
lc_set_prompt( LineCook *state,  const char *prompt,  size_t prompt_len,
               const char *prompt2,  size_t prompt2_len )
{
  return static_cast<linecook::State *>( state )->
    set_prompt( prompt, prompt_len, prompt2, prompt2_len );
}

extern "C"
int
lc_set_right_prompt( LineCook *state,
                     const char *ins,    size_t ins_len,
                     const char *cmd,    size_t cmd_len,
                     const char *emacs,  size_t emacs_len,
                     const char *srch,   size_t srch_len,
                     const char *comp,   size_t comp_len,
                     const char *visu,   size_t visu_len,
                     const char *ouch,   size_t ouch_len,
                     const char *sel1,   size_t sel1_len,
                     const char *sel2,   size_t sel2_len )
{
  return static_cast<linecook::State *>( state )->
    set_right_prompt( ins,   ins_len,
                      cmd,   cmd_len,
                      emacs, emacs_len,
                      srch,  srch_len,
                      comp,  comp_len,
                      visu,  visu_len,
                      ouch,  ouch_len,
                      sel1,  sel1_len,
                      sel2,  sel2_len );
}

using namespace linecook;

static const char32_t *cur_fmt( LeftPrompt &pr ) {
  return ( ! pr.is_continue ) ? pr.fmt : pr.fmt2;
}

static size_t cur_fmt_len( LeftPrompt &pr ) {
  return ( ! pr.is_continue ) ? pr.fmt_len : pr.fmt2_len;
}

ScreenClass
State::escape_class( const char32_t *code,  size_t &sz )
{
  size_t i;
  /* title bar escape sequence: <ESC> ] 0 ; title-description <BEL> */
  if ( code[ 0 ] == ']' ) {
    for ( i = 1; i < sz && code[ i ] != '\0'; i++ ) {
      if ( code[ i ] == 7 || ( code[ i ] == '\\' && code[ i - 1 ] == 27 ) ) {
        sz = i + 1;
        return SCR_TERMINAL;
      }
    }
  }
  /* vt100 sequence: <ESC> [ @.._ */
  else if ( code[ 0 ] == '[' ) {
    if ( sz > 1 && code[ 1 ] >= '@' && code[ 1 ] <= '_' ) {
      sz = 2;
      return SCR_VT100_2;
    }
    /* colors, bold, underline: <ESC> [ N ; N m */
    for ( i = 1; i < sz && code[ i ] != '\0'; i++ ) {
      if ( code[ i ] >= '@' && code[ i ] <= '~' ) {
        sz = i + 1;
        return SCR_COLOR;
      }
    }
  }
  /* vt100 sequence: <ESC> @.._ */
  else if ( code[ 0 ] >= '@' && code[ 0 ] <= '_' ) {
    sz = 1;
    return SCR_VT100_1;
  }
  sz = 0;
  return SCR_ESC;
}

ScreenClass
State::screen_class( const char32_t *code,  size_t &sz )
{
  /* sz must be >= 1 */
  switch ( code[ 0 ] ) {
    default:
      sz = 1;
      if ( code[ 0 ] < ' ' )
        return SCR_CTRL;
      return SCR_CHAR;
    case 27:
      if ( sz > 1 ) {
        size_t xsz = sz - 1;
        ScreenClass cl = State::escape_class( &code[ 1 ], xsz );
        sz = xsz + 1;
        return cl;
      }
      sz = 1;
      return SCR_ESC;
    case 127:
      sz = 1;
      return SCR_DEL;
  }
}

bool
State::format_prompt( void )
{
  static const size_t OUT_SZ = 1024;
  char32_t         out[ OUT_SZ ];
  size_t           x, sz, o, n,
                   j    = 0,
                   col  = 0,
                   rj   = 0,
                   lj   = 0,
                   left = 0,
                   right, spc, start;
  LeftPrompt     & pr    = this->prompt;
  const char32_t * p     = cur_fmt( pr );
  size_t           p_len = cur_fmt_len( pr );
  uint32_t         fl    = pr.flags & ~pr.flags_mask;
  ScreenClass      code;
  bool             nl            = false,
                   right_justify = false,
                   saw_escape    = false;

  pr.time_off = 0;
  pr.date_off = 0;
  pr.hist_off = 0;
  for ( size_t i = 0; i < p_len; i += sz ) {
    if ( (x = OUT_SZ - j) == 0 )
      break;

    sz = p_len - i;
    if ( ! saw_escape )
      code = State::screen_class( &p[ i ], sz );
    else {
      saw_escape = false;
      code = State::escape_class( &p[ i ], sz );
    }
    if ( code == SCR_TERMINAL || code == SCR_COLOR ) {
      if ( sz < x ) {
        for ( size_t k = 0; k < sz; k++ )
          out[ j + k ] = p[ i + k ];
        j += sz;
      }
    }
    else if ( code == SCR_CTRL ) {
      if ( p[ i ] == '\n' ) {
        out[ j++ ] = p[ i ];
        if ( right_justify )
          goto do_justify;
      }
      if ( p[ i ] == '\r' ) {
        out[ j++ ] = p[ i ];
      }
    }
    else if ( code == SCR_CHAR ) {
      start = j;
      if ( p[ i ] == '\\' ) {
        const char     * q     = NULL;
        size_t           q_len = 0;
        const char32_t * q32   = NULL;
        size_t           q32ln = 0;
        switch ( p[ ++i ] ) {
          case '[': /* bash escapes */
          case ']':
            break;
          case 'j': /* jobs */
            break;
          case '!': /* hist number */
          case '#': /* command number */
            if ( ( fl & P_HAS_HIST ) != 0 ) {
              q     = pr.hist;
              q_len = pr.hist_len;
              pr.hist_off = j;
            }
            break;
          case '$': /* prompt $ or # */
            out[ j++ ] = ( pr.euid == 0 ) ? '#' : '$';
            break;
          case 'v': /* version string */
          case 'V':
            q     = pr.vers;
            q_len = pr.vers_len;
            break;
          case 'h': /* host */
          case 'H': /* host.domain */
            if ( ( fl & P_HAS_HOST ) != 0 ) {
              q32   = pr.host;
              q32ln = ( p[ i ] == 'H' ) ? pr.host_len : pr.host_off;
            }
            break;
          case 's': /* sh name */
            if ( ( fl & P_HAS_SHNAME ) != 0 ) {
              q     = pr.shname;
              q_len = pr.shname_len;
            }
            break;
          case 'l': /* ttyXX */
            if ( ( fl & P_HAS_TTYNAME ) != 0 ) {
              q     = pr.ttyname;
              q_len = pr.ttyname_len;
            }
            break;
          case 'd': /* Mon Nov 5 */
            if ( ( fl & P_HAS_DATE ) != 0 ) {
              q32   = pr.date;
              q32ln = pr.date_len;
              pr.date_off = j;
            }
            break;
          case 'A': /* 24 hr:mi */
          case 't': /* 24 hr:mi:se */
          case 'T': /* 12 hr:mi:se time */
          case '@': /* 12 hr:mi ampm time */
            if ( ( fl & P_HAS_ANY_TIME ) != 0 ) {
              q     = pr.time;
              q_len = pr.time_len;
              pr.time_off = j;
            }
            break;
          case 'u': /* user */
            if ( ( fl & P_HAS_USER ) != 0 ) {
              q32   = pr.user;
              q32ln = pr.user_len;
            }
            break;
          case '4': /* ipaddr */
            if ( ( fl & P_HAS_IPADDR ) != 0 ) {
              q     = pr.ipaddr;
              q_len = pr.ipaddr_len;
            }
            break;
          case 'w': /* pwd */
            if ( ( fl & P_HAS_CWD ) != 0 ) {
              q32   = pr.cwd;
              q32ln = pr.cwd_len;
              if ( pr.home_len > 0 &&
                   pr.home_len <= q32ln &&
                   cmp<char32_t>( pr.home, q32, pr.home_len ) == 0 ) {
                q32 = &q32[ pr.home_len ];
                q32ln -= pr.home_len;
                out[ j++ ] = '~';
              }
            }
            break;
          case 'W': /* basename pwd */
            if ( ( fl & P_HAS_CWD ) != 0 ) {
              q32   = &pr.cwd[ pr.dir_off ];
              q32ln = pr.cwd_len - pr.dir_off;
            }
            break;
          case 'r': /* ignore this */
            break;
          case 'R': /* right justify, needs newline */
            right_justify = true;
            rj = j;
            left = col;
            break;
          case 'a': /* alert */
            out[ j++ ] = '\a';
            break;
          case 'e': /* escape */
            out[ j++ ] = '\033';
            start++; /* not a printable, dont't add to col */
            saw_escape = true;
            break;
          case '0': case '1': case '2': case '3':
            /* octal, ignore it unless esc, nl or ascii  */
            o = p[ i ] - '0';
            n = 1;
            while ( p[ i + 1 ] >= '0' && p[ i + 1 ] <= '9' ) {
              o = ( o * 8 ) + ( p[ ++i ] - '0' );
              n++;
            }
            start += n;
            if ( o == 27 ) {
              out[ j++ ] = '\033';
              saw_escape = true;
            }
            else if ( o == '\n' ) {
          case 'n': /* lf */
              if ( x >= 2 ) {
                out[ j++ ] = '\r';
                out[ j++ ] = '\n';
                nl = true;
              }
            }
            else if ( o >= ' ' ) {
              out[ j++ ] = (char32_t) o;
              start--;
            }
            break;
          case '\\':
            out[ j++ ] = '\\';
            break;
          default:
            if ( x >= 2 ) {
              out[ j++ ] = '\\';
              out[ j++ ] = p[ i ];
            }
            break;
        }
        if ( q_len > 0 && x >= q_len ) {
          for ( size_t k = 0; k < q_len; k++ )
            out[ j + k ] = q[ k ];
          j += q_len;
        }
        if ( q32ln > 0 && x >= q32ln ) {
          for ( size_t k = 0; k < q32ln; k++ )
            out[ j + k ] = q32[ k ];
          j += q32ln;
        }
      }
      else {
        out[ j++ ] = p[ i ];
      }
      if ( nl ) {
        if ( right_justify ) {
        do_justify:;
          right = col - left; /* insert spaces between left and right cols */
          if ( this->cols > right + left ) { /* XXX may need to squish */
            spc = this->cols - ( right + left );
            if ( spc + j <= x ) {
              move<char32_t>( &out[ rj + spc ], &out[ rj ], j - rj );
              set<char32_t>( &out[ rj ], ' ', spc );
              j += spc;
              if ( pr.time_off >= lj && pr.time_off > rj )
                pr.time_off += spc;
              if ( pr.date_off >= lj && pr.date_off > rj )
                pr.date_off += spc;
              if ( pr.hist_off >= lj && pr.hist_off > rj )
                pr.hist_off += spc;
              lj = j; /* don't right justify time again */
            }
          }
        }
        col = 0;
        nl  = false;
      }
      else {
        for ( size_t k = start; k < j; k++ ) {
          int w = wcwidth9( out[ k ] );
          if ( w > 0 )
            col += w;
        //col += j - start;
        }
      }
    }
  }
  if ( j > pr.buflen )
    if ( ! this->realloc_buf32( &pr.str, pr.buflen, j ) )
      return false;
  copy<char32_t>( pr.str, out, j );
  pr.len = j;
  return true;
}

bool
State::get_prompt_vars( void )
{
  char              tmp[ 1024 ];
  struct addrinfo * res = NULL;
  struct tm         val;
  size_t            l, sz;
  ssize_t           n;
  LeftPrompt      & pr    = this->prompt;
  char            * s;
  const char32_t  * p     = cur_fmt( pr );
  size_t            p_len = cur_fmt_len( pr );

  pr.cur_time = 0;
  pr.flags = 0;
  for ( size_t i = 0; i < p_len; i += sz ) {
    sz = p_len - i;
    if ( State::screen_class( &p[ i ], sz ) == SCR_CHAR ) {
      if ( p[ i ] == '\\' ) {
        switch ( p[ ++i ] ) {
          case '[': /* bash escapes */
          case ']':
            break;
          case 'j': /* jobs XXX */
            break;
          case '!': /* hist number */
          case '#': /* command number */
            pr.flags |= P_HAS_HIST;
            pr.cur_hist = this->hist.cnt;
            l = uint_digits( pr.cur_hist );
            uint_to_str( pr.cur_hist, pr.hist, l );
            pr.hist_len = l;
            break;
          case '$': /* prompt $ or # */
            pr.flags |= P_HAS_EUID;
            pr.euid = ::geteuid();
            break;
          case 'v': /* version string */
          case 'V':
            ::strcpy( pr.vers, "1" ); /* XXX */
            pr.vers_len = 1;
            break;
          case 'h': /* host */
          case 'H': /* host.domain */
          case '4': /* ipaddr */
            if ( ::gethostname( tmp, sizeof( tmp ) ) == 0 ) {
              if ( ! this->make_utf32( tmp, ::strlen( tmp ), pr.host,
                                       pr.host_len ) )
                return false;
              pr.flags |= P_HAS_HOST;
              for ( pr.host_off = 0; pr.host_off < pr.host_len; pr.host_off++ )
                if ( pr.host[ pr.host_off ] == '.' )
                  break;
            }
            if ( p[ i ] == '4' && ( pr.flags & P_HAS_HOST ) != 0 &&
                 ::getaddrinfo( tmp, NULL, NULL, &res ) == 0 ) {
              for ( struct addrinfo *p = res; p != NULL; p = p->ai_next ) {
                if ( p->ai_family == AF_INET && p->ai_addr != NULL ) {
                  char *ip = ::inet_ntoa( ((struct sockaddr_in *) p->ai_addr)->
                                          sin_addr );
                  l = ::strlen( ip );
                  if ( l <= sizeof( pr.ipaddr ) ) {
                    pr.flags |= P_HAS_IPADDR;
                    ::memcpy( pr.ipaddr, ip, l );
                    pr.ipaddr_len = l;
                  }
                  break;
                }
              }
              ::freeaddrinfo( res );
            }
            break;
          case 'l': /* ttyXX */
          case 's': /* sh name */
            n = ::readlink( 
              ( p[ i ] == 'l' ) ? "/proc/self/fd/0" : "/proc/self/exe",
              tmp, sizeof( tmp ) );
            if ( n > 0 && (size_t) n + 1 < sizeof( tmp ) ) {
              tmp[ n ] = '\0';
              if ( p[ i ] == 'l' )
                s = ::strchr( &tmp[ 1 ], '/' );
              else
                s = ::strrchr( tmp, '/' );
              if ( s == NULL )
                s = tmp;
              else
                s++;
              l = &tmp[ n ] - s;
              if ( p[ i ] == 'l' ) {
                if ( ! this->realloc_buf8( &pr.ttyname, pr.ttyname_len, l + 1 ))
                  return false;
                pr.flags |= P_HAS_TTYNAME;
                ::strcpy( pr.ttyname, s );
                pr.ttyname_len = l;
              }
              else {
                if ( ! this->realloc_buf8( &pr.shname, pr.shname_len, l + 1 ) )
                  return false;
                pr.flags |= P_HAS_SHNAME;
                ::strcpy( pr.shname, s );
                pr.shname_len = l;
              }
            }
            break;
          case 'd': /* Mon Nov 5 */
            pr.flags |= P_HAS_DATE;
            if ( pr.cur_time == 0 )
              ::time( &pr.cur_time );
            if ( ! this->update_date( pr.cur_time ) )
              return false;
            break;
          case 'A': /* 24 hr:mi */
          case 't': /* 24 hr:mi:se */
          case 'T': /* 12 hr:mi:se time */
          case '@': /* 12 hr:mi ampm time */
            if ( pr.cur_time == 0 )
              ::time( &pr.cur_time );
            ::localtime_r( &pr.cur_time, &val );
            switch ( p[ i ] ) {
              case 'A': pr.flags |= P_HAS_TIME24; break;
              case 't': pr.flags |= P_HAS_TIME24s; break;
              case 'T': pr.flags |= P_HAS_TIME12s; break;
              case '@': pr.flags |= P_HAS_TIME12ampm; break;
            }
            l = val.tm_hour;
            if ( p[ i ] == 'T' || p[ i ] == '@' )
              l %= 12;
            uint_to_str( l, &pr.time[ 0 ], 2 );
            pr.time[ 2 ] = ':';
            uint_to_str( val.tm_min, &pr.time[ 3 ], 2 );
            pr.time_len = 5;
            if ( p[ i ] == 't' || p[ i ] == 'T' ) {
              pr.time[ 5 ] = ':';
              uint_to_str( val.tm_sec, &pr.time[ 6 ], 2 );
              pr.time_len += 3;
            }
            else if ( p[ i ] == '@' ) {
              /*pr.time[ 5 ] = ' ';*/
              pr.time[ 5 ] = ( val.tm_hour > 11 ) ? 'p' : 'a';
              pr.time[ 6 ] = 'm';
              pr.time_len += 2;
            }
            break;
          case 'u': /* user */
            pr.euid = ::geteuid();
            if ( ! this->update_user() )
              return false;
            pr.flags |= P_HAS_EUID;
            pr.flags |= P_HAS_USER;
            break;
          case 'w': /* pwd */
          case 'W': /* basename pwd */
            if ( ! this->update_cwd() )
              return false;
            pr.flags |= P_HAS_CWD;
            break;
          default:
            break;
        }
      }
    }
  }
  return true;
}

bool
State::update_date( time_t t )
{
  LeftPrompt & pr = this->prompt;
  struct tm    val;
  char         tmp[ 1024 ];
  size_t       l;

  ::localtime_r( &t, &val );
  l = ::strftime( tmp, sizeof( tmp ), "%a %b %d", &val );
  if ( l > 0 ) {
    if ( ! this->make_utf32( tmp, l, pr.date, pr.date_len ) )
      return false;
  }
  return true;
}

bool
State::update_cwd( void )
{
  LeftPrompt & pr = this->prompt;
  char         tmp[ 1024 ];
  const char * h;
  size_t       l;

  if ( ::getcwd( tmp, sizeof( tmp ) ) != NULL ) {
    if ( ! this->make_utf32( tmp, ::strlen( tmp ), pr.cwd, pr.cwd_len ) )
      return false;
    for ( l = pr.cwd_len; l > 0; ) {
      if ( pr.cwd[ --l ] == '/' )
        break;
    }
    pr.dir_off = l;
    if ( pr.home_len == 0 ) {
      if ( (h = ::getenv( "HOME" )) != NULL ) {
        if ( ! this->make_utf32( h, ::strlen( h ), pr.home, pr.home_len ) )
          return false;
      }
    }
  }
  return true;
}

bool
State::update_user( void )
{
  LeftPrompt    & pr = this->prompt;
  struct passwd * pw, pwbuf;
  char            tmp[ 1024 ];
  if ( ::getpwuid_r( pr.euid, &pwbuf, tmp, sizeof( tmp ), &pw ) == 0){
    if ( ! this->make_utf32( pw->pw_name, ::strlen( pw->pw_name ),
                             pr.user, pr.user_len ) )
      return false;
  }
  return true;
}

bool
State::get_prompt_geom( void )
{
  LeftPrompt     & pr    = this->prompt;
  const char32_t * p     = pr.str;
  size_t           p_len = pr.len,
                   i, j = 0, c = 0, l = 0, sz;
  int              w;
  ::memset( pr.line_cols, 0, sizeof( pr.line_cols ) );
  pr.cols  = 0;
  pr.lines = 0;
  for ( i = 0; i < p_len; i += sz ) {
    sz = p_len - i;
    switch ( State::screen_class( &p[ i ], sz ) ) {
      case SCR_CTRL:
        if ( i + 1 < p_len && p[ i ] == '\r' && p[ i + 1 ] == '\n' ) {
          j += 2;
          sz = 2;
          if ( l >= sizeof( pr.line_cols ) / sizeof( pr.line_cols[ 0 ] ) )
            return false;
          pr.line_cols[ l ] = c;
          l++;
          c = 0;
        }
        else if ( p[ i ] == 0 ) {
          j++;
        }
        break;
      case SCR_CHAR:     /* normal chars */
        j++;
        w = wcwidth9( p[ i ] );
        if ( w > 0 ) {
          c += w;
          if ( w == 2 && i + 1 < p_len && p[ i + 1 ] == 0 ) {
            j++;
            sz = 2;
          }
        }
        break;
      case SCR_TERMINAL: /* change title */
      case SCR_COLOR:    /* change appearance */
        j += sz;
        break;
      default: /* skip ctrl chars, vt100 sequences, 8bit codes */
        break;
    }
  }
  if ( j != p_len )
    return false;
  pr.cols  = c;
  pr.lines = l;
  return true;
}

static void
u32u8cpy( char32_t *str,  const char *s,  size_t n )
{
  for ( size_t i = 0; i < n; i++ )
    str[ i ] = ((uint8_t *) s)[ i ] & 0x7f;
}

bool
State::update_prompt_time( void )
{
  LeftPrompt & pr = this->prompt;
  uint32_t     fl = pr.flags & ~pr.flags_mask;
  time_t       new_time;
  bool         b  = false;

  if ( ( fl & P_HAS_HIST ) != 0 ) {
    /* only quietly updates if it has the same number of digits */
    if ( pr.cur_hist != this->hist.cnt ) {
      size_t l = uint_digits( this->hist.cnt );
      if ( l == pr.hist_len ) {
        pr.cur_hist = this->hist.cnt;
        uint_to_str( pr.cur_hist, pr.hist, l );
        u32u8cpy( &pr.str[ pr.hist_off ], pr.hist, l );
        b = true;
      }
    }
  }
  if ( ( fl & ( P_HAS_ANY_TIME | P_HAS_DATE ) ) == 0 )
    return b;
  /* if a new second elapsed */
  if ( pr.cur_time != ::time( &new_time ) ) {
    struct tm val;
    ::localtime_r( &new_time, &val );
    if ( ( fl & ( P_HAS_DATE | P_HAS_TIME12ampm ) ) != 0 ) {
      /* if a new hour elapsed */
      if ( new_time / 3600 != pr.cur_time / 3600 ) {
        if ( ( fl & P_HAS_DATE ) != 0 ) {
          this->update_date( new_time );
          b = true;
        }
        if ( ( fl & P_HAS_TIME12ampm ) != 0 ) {
          pr.time[ 5 ] = ( val.tm_hour > 11 ) ? 'p' : 'a';
        }
      }
    }
    if ( ( fl & P_HAS_ANY_TIME ) != 0 ) {
      /* if a new minute elapsed or has seconds */
      if ( ( fl & ( P_HAS_TIME12s | P_HAS_TIME24s ) ) != 0 ||
           new_time / 60 != pr.cur_time / 60 ) {
        uint64_t l = val.tm_hour;
        if ( ( fl & ( P_HAS_TIME12s | P_HAS_TIME12ampm ) ) != 0 )
          l %= 12;
        uint_to_str( l, &pr.time[ 0 ], 2 );
        uint_to_str( val.tm_min, &pr.time[ 3 ], 2 );
        if ( ( fl & ( P_HAS_TIME12s | P_HAS_TIME24s ) ) != 0 )
          uint_to_str( val.tm_sec, &pr.time[ 6 ], 2 );
        u32u8cpy( &pr.str[ pr.time_off ], pr.time, pr.time_len );
        b = true;
      }
    }
    pr.cur_time = new_time;
  }
  return b;
}

bool
State::update_prompt( bool force )
{
  LeftPrompt & pr = this->prompt;
  bool         b = false;

  if ( pr.flags_mask != 0 ) {
    pr.flags_mask = 0;
    force = true;
  }
  if ( ( pr.flags & P_HAS_CWD ) != 0 ) {
    if ( ! this->update_cwd() )
      return false;
    b = true;
  }
  if ( ( pr.flags & P_HAS_EUID ) != 0 ) {
    uint32_t euid = ::geteuid();
    if ( euid != pr.euid ) {
      pr.euid = euid;
      if ( ( pr.flags & P_HAS_USER ) != 0 ) {
        if ( ! this->update_user() )
          return false;
      }
      b = true;
    }
  }
  if ( ( pr.flags & P_HAS_HIST ) != 0 ) {
    /* only quietly updates if it has the same number of digits */
    if ( pr.cur_hist != this->hist.cnt ) {
      size_t l = uint_digits( this->hist.cnt );
      pr.cur_hist = this->hist.cnt;
      uint_to_str( pr.cur_hist, pr.hist, l );
      pr.hist_len = l;
      b = true;
    }
  }
  b |= this->update_prompt_time();
  if ( b || force ) {
    for (;;) {
      bool is_too_big = false;
      this->format_prompt();
      this->get_prompt_geom();
      for ( size_t i = 0; i < pr.lines; i++ ) {
        if ( pr.line_cols[ i ] > this->cols ) {
          is_too_big = true;
          break;
        }
      }
      if ( ! is_too_big )
        break;
      /* remove stuff until prompt fits, highest priority last */
      if ( ( ( pr.flags & ~pr.flags_mask ) & P_HAS_IPADDR ) != 0 )
        pr.flags_mask |= P_HAS_IPADDR;
      else if ( ( ( pr.flags & ~pr.flags_mask ) & P_HAS_ANY_TIME ) != 0 )
        pr.flags_mask |= P_HAS_ANY_TIME;
      else if ( ( ( pr.flags & ~pr.flags_mask ) & P_HAS_SHNAME ) != 0 )
        pr.flags_mask |= P_HAS_SHNAME;
      else if ( ( ( pr.flags & ~pr.flags_mask ) & P_HAS_TTYNAME ) != 0 )
        pr.flags_mask |= P_HAS_TTYNAME;
      else if ( ( ( pr.flags & ~pr.flags_mask ) & P_HAS_DATE ) != 0 )
        pr.flags_mask |= P_HAS_DATE;
      else if ( ( ( pr.flags & ~pr.flags_mask ) & P_HAS_HOST ) != 0 )
        pr.flags_mask |= P_HAS_HOST;
      else if ( ( ( pr.flags & ~pr.flags_mask ) & P_HAS_USER ) != 0 )
        pr.flags_mask |= P_HAS_USER;
      else if ( ( ( pr.flags & ~pr.flags_mask ) & P_HAS_CWD ) != 0 )
        pr.flags_mask |= P_HAS_CWD;
      else if ( ( ( pr.flags & ~pr.flags_mask ) & P_HAS_HIST ) != 0 )
        pr.flags_mask |= P_HAS_HIST;
      else
        break;
    }
  }
  return b;
}

bool
State::make_utf32( const char *buf,  size_t len,  char32_t *&x,  size_t &xlen )
{
  char32_t tmp[ 1024 ], * p = tmp;
  size_t i = 0, j = 0;
  this->error = 0;
  if ( len > 128 ) {
    if ( (p = (char32_t *) ::malloc( len * 8 * sizeof( char32_t ) )) == NULL )
      this->error = LINE_STATUS_ALLOC_FAIL;
  }
  if ( this->error == 0 ) {
    while ( j < len ) {
      int n = ku_utf8_to_utf32( &buf[ j ], len - j, &p[ i ] );
      if ( n <= 0 ) {
        this->error = LINE_STATUS_BAD_INPUT;
        break;
      }
      /* replace tab with space */
      else if ( p[ i ] == '\t' ) {
        do {
          p[ i++ ] = ' ';
        } while ( ( i % 8 ) != 0 );
      }
      /* double width chars need padding */
      else if ( wcwidth9( p[ i ] ) > 1 ) {
        i++;
        p[ i++ ] = 0;
      }
      else {
        i++;
      }
      j += n;
    }
    if ( this->error == 0 && xlen < i ) {
      void *nx = ::realloc( x, i * sizeof( char32_t ) );
      if ( nx == NULL )
        this->error = LINE_STATUS_ALLOC_FAIL;
      else
        x = (char32_t *) nx;
    }
    if ( this->error == 0 ) {
      copy<char32_t>( x, p, i );
      xlen = i;
    }
  }
  if ( p != NULL && p != tmp )
    ::free( p );
  return this->error == 0;
}

static bool
isxnum( const char *buf,  size_t len,  uint32_t &val )
{
  uint8_t bval;
  val = 0;
  for ( size_t i = 0; i < len; i++ ) {
    if ( buf[ i ] >= '0' && buf[ i ] <= '9' )
      bval = (uint8_t) buf[ i ] - '0';
    else if ( buf[ i ] >= 'a' && buf[ i ] <= 'f' )
      bval = (uint8_t) buf[ i ] - 'a' + 10;
    else if ( buf[ i ] >= 'A' && buf[ i ] <= 'F' )
      bval = (uint8_t) buf[ i ] - 'A' + 10;
    else 
      return false;
    val = ( val << 4 ) | bval;
  }
  return true;
}

static bool
isonum( const char *buf,  size_t len,  uint32_t &val )
{
  uint8_t oval;
  val = 0;
  for ( size_t i = 0; i < len; i++ ) {
    if ( buf[ i ] >= '0' && buf[ i ] <= '7' )
      oval = (uint8_t) buf[ i ] - '0';
    else 
      return false;
    val = ( val << 3 ) | oval;
  }
  return true;
}

bool
State::make_prompt_utf32( const char *buf,  size_t len,  char32_t *&x,
                          size_t &xlen )
{
  char tmp[ 1024 ], * p = tmp;
  size_t i = 0, j = 0;
  uint32_t val;
  this->error = 0;
  if ( len > 1024 ) {
    if ( (p = (char *) ::malloc( len )) == NULL )
      this->error = LINE_STATUS_ALLOC_FAIL;
  }
  if ( this->error == 0 ) {
    while ( i < len ) {
      if ( buf[ i ] == '\\' && i + 1 < len ) {
        bool matched = false;
        switch ( buf[ i + 1 ] ) {
          case 'u': /* 4 hex u16 */
            if ( i + 1 + 4 < len && isxnum( &buf[ i + 2 ], 4, val ) ) {
              int n = ku_utf32_to_utf8( val, &p[ j ] );
              if ( n > 0 ) {
                j += n;
                i += 6;
                matched = true;
              }
            }
            break;
          case 'U': /* 8 hex u32 */
            if ( i + 1 + 8 < len && isxnum( &buf[ i + 2 ], 8, val ) ) {
              int n = ku_utf32_to_utf8( val, &p[ j ] );
              if ( n > 0 ) {
                j += n;
                i += 10;
                matched = true;
              }
            }
            break;
          case 'x': /* 2 hex u8 */
            if ( i + 1 + 2 < len && isxnum( &buf[ i + 2 ], 2, val ) ) {
              p[ j++ ] = (char) ( val & 0xff );
              i += 4;
              matched = true;
            }
            break;
          case '0': /* 3 octal */
            if ( i + 1 + 3 < len && isonum( &buf[ i + 2 ], 3, val ) ) {
              p[ j++ ] = (char) ( val & 0xff );
              i += 5;
              matched = true;
            }
            break;
          default:
            break;
        }
        if ( matched )
          continue;
      }
      p[ j++ ] = buf[ i++ ];
    }
    this->make_utf32( p, j, x, xlen );
  }
  if ( p != NULL && p != tmp )
    ::free( p );
  return this->error == 0;
}

void
State::set_default_prompt( void )
{
  LeftPrompt & pr = this->prompt;
  this->make_utf32( "; ", 2, pr.fmt, pr.fmt_len );
  this->make_utf32( "> ", 2, pr.fmt2, pr.fmt2_len );
  this->make_utf32( "; ", 2, pr.str, pr.len );
  this->prompt.cols       = 2;
  this->prompt.lines      = 0;
  this->prompt.flags      = 0;
  this->prompt.flags_mask = 0;
}

int
State::set_prompt( const char *p,  size_t p_len,  const char *p2,
                   size_t p2_len )
{
  LeftPrompt & pr = this->prompt;
  if ( ! this->make_prompt_utf32( p, p_len, pr.fmt, pr.fmt_len ) ||
       ! this->make_prompt_utf32( p2, p2_len, pr.fmt2, pr.fmt2_len ) )
    return this->error;
  this->prompt.flags_mask = 0;

  return this->init_lprompt();
}

int
State::init_lprompt( void )
{
  if ( ! this->get_prompt_vars() || ! this->format_prompt() ||
       ! this->get_prompt_geom() ) {
    this->set_default_prompt();
    return -1;
  }
  return 0;
}
#if 0
  size_t save_cols  = this->prompt_cols,
         save_lines = this->prompt_lines;
  /* if not currently editing line */
  if ( ! this->left_prompt_needed )
    return 0;
  size_t new_pos = this->cursor_pos - save_cols + this->prompt_cols;
  this->error = 0;
  if ( save_lines < this->prompt_lines )
    this->output_fmt( ANSI_CURSOR_UP_FMT, this->prompt_lines - save_lines  );
  else if ( save_lines > this->prompt_lines )
    this->output_fmt( ANSI_CURSOR_DOWN_FMT, save_lines - this->prompt_lines );
  this->refresh_line();
  this->move_cursor( new_pos );
  return this->error;
#endif

bool
State::init_rprompt( LinePrompt &p,  const char *buf,  size_t len )
{
  if ( ! this->make_prompt_utf32( buf, len, p.prompt, p.prompt_len ) ) {
  bad_prompt:;
    p.prompt_cols = 0;
    p.prompt_len  = 0;
    return false;
  }
  size_t i, j = 0, c = 0, sz;
  int    w;
  for ( i = 0; i < p.prompt_len; i += sz ) {
    sz = p.prompt_len - i;
    switch ( State::screen_class( &p.prompt[ i ], sz ) ) {
      case SCR_CTRL:
      case SCR_CHAR:    /* normal chars */
        j++;
        w = wcwidth9( p.prompt[ i ] );
        if ( w > 0 )
          c += w;
        break;
      case SCR_TERMINAL: /* change title */
      case SCR_COLOR:    /* change appearance */
        j += sz;
        break;
      default: /* skip ctrl chars, vt100 sequences, 8bit codes */
        break;
    }
  }
  if ( j != p.prompt_len ) {
    this->error = LINE_STATUS_BAD_PROMPT;
    goto bad_prompt;
  }
  p.prompt_cols = c;
  return true;
}

int
State::set_right_prompt( const char *ins,    size_t ins_len,
                         const char *cmd,    size_t cmd_len,
                         const char *emacs,  size_t emacs_len,
                         const char *srch,   size_t srch_len,
                         const char *comp,   size_t comp_len,
                         const char *visu,   size_t visu_len,
                         const char *ouch,   size_t ouch_len,
                         const char *sel1,   size_t sel1_len,
                         const char *sel2,   size_t sel2_len )
{
  if ( ! init_rprompt( this->r_ins,     ins,   ins_len  ) ||
       ! init_rprompt( this->r_cmd,     cmd,   cmd_len  ) ||
       ! init_rprompt( this->r_emacs,   emacs, emacs_len ) ||
       ! init_rprompt( this->r_srch,    srch,  srch_len ) ||
       ! init_rprompt( this->r_comp,    comp,  comp_len ) ||
       ! init_rprompt( this->r_visu,    visu,  visu_len ) ||
       ! init_rprompt( this->r_ouch,    ouch,  ouch_len ) ||
       ! init_rprompt( this->sel_left,  sel1,  sel1_len ) ||
       ! init_rprompt( this->sel_right, sel2,  sel2_len ) )
    return -1;
  return 0;
}

void
State::output_prompt( void )
{
  if ( this->prompt.len > 0 )
    this->output( this->prompt.str, this->prompt.len );
  if ( this->prompt.pad_cols > 0 ) {
    for ( uint8_t i = 0; i < this->prompt.pad_cols; i++ )
      this->output( ' ' );
  }
}

void
State::output_right_prompt( bool clear )
{
  LinePrompt * p;
  if ( this->bell_cnt == 1 ) {
    this->bell_cnt = 0;
    this->clear_right_prompt( this->r_ouch );
    this->output_right_prompt( clear );
    return;
  }
  if ( this->bell_cnt != 0 )
    p = &this->r_ouch;
  else if ( this->show_mode == SHOW_COMPLETION )
    p = &this->r_comp;
  else if ( this->test( this->in.mode, VISUAL_MODE ) != 0 )
    p = &this->r_visu;
  else if ( this->test( this->in.mode, SEARCH_MODE ) != 0 )
    p = &this->r_srch;
  else if ( this->test( this->in.mode, EMACS_MODE ) != 0 )
    p = &this->r_emacs;
  else if ( this->test( this->in.mode, VI_INSERT_MODE ) != 0 )
    p = &this->r_ins;
  else
    p = &this->r_cmd;
  if ( this->r_prompt != NULL && this->r_prompt != p ) {
    LinePrompt * x = this->r_prompt;
    if ( clear ) {
      if ( p->prompt_cols > x->prompt_cols )
        x = p;
    }
    this->clear_right_prompt( *x );
    this->r_prompt = NULL;
    if ( clear )
      return;
  }
  if ( clear ) {
    this->clear_right_prompt( *p );
    this->r_prompt = NULL;
  }
  else {
    this->show_right_prompt( *p );
    this->r_prompt = p;
  }
}

void
State::clear_right_prompt( LinePrompt &p )
{
  size_t rprompt_cols = p.prompt_cols;
  if ( rprompt_cols == 0 )
    return;
  size_t save = this->cursor_pos,
         rpos = ( ( ( this->edited_len + this->prompt.cols ) /
                  this->cols ) + 1 ) * this->cols - ( rprompt_cols + 1 );
  this->move_cursor( rpos );
  size_t i = rpos, j = rpos - this->prompt.cols;
  for ( ; i < rpos + rprompt_cols; i++ ) {
    if ( j < this->edited_len )
      this->output( this->line[ j++ ] );
    else
      this->output( ' ' );
  }
  this->cursor_pos += rprompt_cols;
  this->move_cursor( save );
}

void
State::show_right_prompt( LinePrompt &p )
{
  size_t rprompt_cols = p.prompt_cols;
  if ( rprompt_cols == 0 )
    return;
  size_t save = this->cursor_pos,
         rpos = ( ( ( this->edited_len + this->prompt.cols ) /
                  this->cols ) + 1 ) * this->cols - ( rprompt_cols + 1 );
  if ( save >= rpos ) {
    this->clear_right_prompt( p );
    return;
  }
  this->move_cursor( rpos );
  this->output( p.prompt, p.prompt_len );
  this->cursor_pos += rprompt_cols;
  this->move_cursor( save );
}

uint64_t
State::time_ms( void )
{
  struct timeval tv;
  ::gettimeofday( &tv, NULL );
  return (long) tv.tv_sec * 1000 + (long) tv.tv_usec / 1000;
}

void
State::erase_eol_with_right_prompt( void )
{
  this->output_str( ANSI_ERASE_EOL, ANSI_ERASE_EOL_SIZE );
  this->right_prompt_needed = true;
}

void
State::bell( void )
{
  this->bell_cnt = 2;
  this->bell_time = State::time_ms();
  this->right_prompt_needed = true;
}

