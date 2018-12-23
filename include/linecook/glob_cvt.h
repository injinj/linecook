#ifndef __linecook__glob_cvt_h__
#define __linecook__glob_cvt_h__

#ifdef __cplusplus
namespace linecook {

/* convert a file glob wildcard to a pcre wildcard
 * (see pcre2_pattern_convert(), not available on centos 7)
 *
 * this does allow '*' to match '/' dirs chars, which is different than file
 * globbing
 */

template <class CHAR>
struct GlobCvt {
  CHAR * out; /* utf8 or utf32 char classes */
  size_t off, /* off will be > maxlen on buf space failure */
         maxlen;

  GlobCvt( CHAR *o, size_t len ) : out( o ), off( 0 ), maxlen( len ) {}

  void char_out( CHAR c ) {
    if ( ++this->off <= this->maxlen )
      this->out[ off - 1 ] = c;
  }

  void str_out( const char *s,  size_t len ) {
    size_t i = this->off;
    if ( (this->off += len) <= this->maxlen ) {
      for (;;) {
        this->out[ i++ ] = (CHAR) (uint8_t) *s++;
        if ( --len == 0 )
          break;
      }
    }
  }
  /* return 0 on success or -1 on failure */
  int convert_glob( const CHAR *pattern,  size_t patlen,  bool anchor_start ) {
    size_t k;
    bool   inside_bracket,
           anchor_end = true;

    this->str_out( "(?s)", 4 ); /* match newlines */
    if ( patlen > 0 ) {
      if ( pattern[ 0 ] != '*' ) {
        if ( anchor_start )
          this->str_out( "\\A", 2 ); /* anchor at start */
        k = 0;
      }
      else {
        k = 1; /* skip anchor, starts with '*' */
      }
      if ( patlen > k && pattern[ patlen - 1 ] == '*' ) {
        if ( patlen == 1 || pattern[ patlen - 2 ] != '\\' ) {
          patlen -= 1; /* no need to match trail */
          anchor_end = false;
        }
      }
      inside_bracket = false;
      for ( ; k < patlen; k++ ) {
        if ( pattern[ k ] == '\\' ) {
          if ( k + 1 < patlen ) {
            switch ( pattern[ ++k ] ) {
              case '\\': case '?': case '*':
                this->char_out( '\\' );
                /* FALLTHRU */
              default:
                this->char_out( pattern[ k ] );
                break;
            }
          }
        }
        else if ( ! inside_bracket ) {
          if ( pattern[ k ] == '*' ) {
            if ( k > 0 && pattern[ k - 1 ] == '*' )
              k++; /* skip duplicate '*' */
            else
              this->str_out( "(*COMMIT).*?", 12 ); /* commit and star */
          }
          else if ( pattern[ k ] == '?' ) {
            this->char_out( '.' );
          }
          else if ( pattern[ k ] == '.' ) {
            this->char_out( '\\' );
            this->char_out( '.' );
          }
          else {
            if ( pattern[ k ] == '[' )
              inside_bracket = true;
            this->char_out( pattern[ k ] );
          }
        }
        else {
          if ( pattern[ k ] == ']' )
            inside_bracket = false;
          this->char_out( pattern[ k ] );
        }
      }
      if ( anchor_end )
        this->str_out( "\\z", 2 ); /* anchor at end */
    }
    if ( this->off > this->maxlen )
      return -1;
    return 0;
  }
};

}
#endif
#endif
