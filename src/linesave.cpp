#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <linecook/linecook.h>

using namespace linecook;

void
State::restore_save( const LineSaveBuf &lsb,  const LineSave &ls )
{
  if ( ! this->realloc_line( ls.edited_len ) )
    return;
  this->move_cursor( this->prompt.cols );
  copy<char32_t>( this->line, &lsb.buf[ ls.line_off ], ls.edited_len );
  this->edited_len = ls.edited_len;
  this->cursor_output( this->line, this->edited_len );
  this->cursor_erase_eol();
  this->move_cursor( ls.cursor_off + this->prompt.cols );
}

/* insert ls into ls2 at cursor pos of ls2 */
void
State::restore_insert( const LineSaveBuf &lsb,  const LineSave &ls,
                       const LineSaveBuf &lsb2,  const LineSave &ls2 )
{
  size_t off, off2;
  if ( ! this->realloc_line( ls.edited_len + ls2.edited_len ) )
    return;
  this->move_cursor( this->prompt.cols );
  /* copy prefix of ls2 into line[] */
  off = off2 = ls2.cursor_off;
  copy<char32_t>( this->line, &lsb2.buf[ ls2.line_off ], off );
  /* copy all of ls into line[] after ls2 prefix */
  copy<char32_t>( &this->line[ off ], &lsb.buf[ ls.line_off ], ls.edited_len );
  off += ls.edited_len;
  /* copy suffix of ls2 into line[] */
  copy<char32_t>( &this->line[ off ], &lsb2.buf[ ls2.line_off + off2 ],
                  ls2.edited_len - off2 );
  this->edited_len = ls.edited_len + ls2.edited_len;
  this->cursor_output( this->line, this->edited_len );
  this->cursor_erase_eol();
  this->move_cursor( off2 + ls.edited_len + this->prompt.cols );
}

void
LineSave::reset( LineSaveBuf &lsb )
{
  lsb.off   = 0;         
  lsb.max   = 0;
  lsb.idx   = 0;
  lsb.cnt   = 0;
  lsb.first = 0;
}

static const size_t LINE_SAVE_CHARS = sizeof( LineSave ) / sizeof( char32_t );

bool
LineSave::equals( const LineSaveBuf &lsb,  size_t off,  const char32_t *buf,
                  size_t len )
{
  if ( off > len ) { /* off must contain both LineSave and line buf len */
    const void * n = (const void *) &lsb.buf[ off - LINE_SAVE_CHARS ];
    const LineSave & ls = *(const LineSave *) n;
    return
      cmp<char32_t>( &lsb.buf[ ls.line_off ], buf, ls.edited_len, len ) == 0;
  }
  return false;
}

int
LineSave::compare( const LineSaveBuf &lsb,  size_t off,  size_t off2 )
{
  if ( off == 0 || off2 == 0 )
    return ( off < off2 ? 1 : ( off > off2 ? -1 : 0 ) );
  const void * n = (const void *) &lsb.buf[ off - LINE_SAVE_CHARS ],
             * m = (const void *) &lsb.buf[ off2 - LINE_SAVE_CHARS ];
  const LineSave & lsn = *(const LineSave *) n,
                 & lsm = *(const LineSave *) m;
  return cmp<char32_t>( &lsb.buf[ lsn.line_off ], &lsb.buf[ lsm.line_off ],
                        lsn.edited_len, lsm.edited_len );
}

size_t
LineSave::size( size_t len )
{
  return align<size_t>( len, 8 ) + LINE_SAVE_CHARS;
}

size_t
LineSave::make( LineSaveBuf &lsb,  const char32_t *buf,  size_t len,
                size_t cursor_off,  size_t idx )
{
  char32_t * p = &lsb.buf[ lsb.max ];
  size_t next;

  copy<char32_t>( p, buf, len );
  if ( cursor_off > len )
    cursor_off = 0;

  size_t off = lsb.max + align<size_t>( len, 8 );
  void * n = (void *) &lsb.buf[ off ];
  new ( n ) LineSave( lsb.max, len, cursor_off, idx );

  next = off + LINE_SAVE_CHARS;
  if ( lsb.max > 0 )
    LineSave::line( lsb, lsb.max ).next_off = next;
  else
    lsb.first = next;
  return lsb.max = next;
}

LineSave &
LineSave::line( LineSaveBuf &lsb,  size_t off )
{
  void * n = (void *) &lsb.buf[ off - LINE_SAVE_CHARS ];
  return *(LineSave *) n;
}

const LineSave &
LineSave::line_const( const LineSaveBuf &lsb,  size_t off )
{
  const void * n = (const void *) &lsb.buf[ off - LINE_SAVE_CHARS ];
  return *(const LineSave *) n;
}

size_t
LineSave::card( const LineSaveBuf &lsb )
{
  size_t off = lsb.max;
  size_t cnt = 0;
  while ( off > 0 ) {
    const LineSave &ls = LineSave::line_const( lsb, off );
    off = ls.line_off;
    cnt++;
  }
  return cnt;
}

size_t
LineSave::find( const LineSaveBuf &lsb,  size_t off,  size_t i )
{
  while ( off > 0 ) {
    const LineSave &ls = LineSave::line_const( lsb, off );
    if ( i == ls.index )
      return off;
    if ( i > ls.index ) {
      if ( ls.next_off == 0 )
        return 0;
      off = ls.next_off;
    }
    else {
      off = ls.line_off;
    }
  }
  return 0;
}

size_t
LineSave::find_gteq( const LineSaveBuf &lsb,  size_t off,  size_t i )
{
  size_t hi = 0;
  while ( off > 0 ) {
    const LineSave &ls = LineSave::line_const( lsb, off );
    if ( i == ls.index )
      return off;
    if ( i > ls.index ) {
      if ( hi != 0 )
        return hi;
      off = ls.next_off;
    }
    else {
      hi = off;
      off = ls.line_off;
    }
  }
  return hi;
}

size_t
LineSave::find_lt( const LineSaveBuf &lsb,  size_t off,  size_t i )
{
  size_t lo = 0;
  while ( off > 0 ) {
    const LineSave &ls = LineSave::line_const( lsb, off );
    if ( i > ls.index ) {
      lo = off;
      off = ls.next_off;
    }
    else {
      if ( lo != 0 )
        return lo;
      off = ls.line_off;
    }
  }
  return lo;
}

size_t
LineSave::find_substr( const LineSaveBuf &lsb,  size_t off,
                       const char32_t *str,  size_t len,  int dir )
{
  while ( off > 0 ) {
    const LineSave & ls = LineSave::line_const( lsb, off );
    const char32_t * line = &lsb.buf[ ls.line_off ];
    size_t           edited_len = ls.edited_len;
    for ( size_t i = len - 1; i < edited_len; ) { 
      const char32_t *p; /* find last str char in line */
      size_t o;
      for ( p = &line[ i ]; p < &line[ edited_len ]; p++ )
        if ( casecmp<char32_t>( *p, str[ len - 1 ] ) == 0 )
          break;
      if ( p == &line[ edited_len ] ) /* not found */
        break;
      o  = (size_t) ( p - &line[ i ] );
      p -= len - 1;
      if ( casecmp<char32_t>( p, str, len - 1 ) == 0 )
        return off;
      i += o + 1;
    }
    if ( dir < 0 )
      off = ls.next_off;
    else
      off = ls.line_off;
  }
  return 0;
}

size_t
LineSave::find_prefix( const LineSaveBuf &lsb,  size_t off, const char32_t *str,
                       size_t len,  size_t &prefix_len,  size_t &match_cnt )
{
  size_t           match_off  = 0;
  const char32_t * match_line = NULL;
  match_cnt = 0;
  for ( prefix_len = 0; off > 0; ) {
    const LineSave & ls = LineSave::line_const( lsb, off );
    const char32_t * line = &lsb.buf[ ls.line_off ];
    /* if prefix matches str */
    for ( size_t i = 0; i < len; i++ ) {
      if ( i == ls.edited_len ||
           casecmp<char32_t>( str[ i ], line[ i ] ) != 0 ) {
        if ( i > prefix_len ) {
          prefix_len = i; /* partial match of str, but not all */
          match_off  = off;
        }
        goto not_matched;
      }
    }
    match_cnt++; /* all of str matches line */
    if ( match_line == NULL ) { /* first match */
      prefix_len = ls.edited_len;
      match_line = line;
      match_off  = off;
    }
    else { /* another match, find shortest prefix of the two matches */
      for ( size_t j = len; ; j++ ) {
        if ( j == prefix_len || j == ls.edited_len ||
             casecmp<char32_t>( match_line[ j ], line[ j ] ) != 0 ) {
          prefix_len = j;
          break;
        }
      }
    }
  not_matched:;
    off = ls.next_off; /* next line */
  }
  return match_off;
}

bool
LineSave::filter_substr( LineSaveBuf &lsb,  const char32_t *str,  size_t len )
{
  LineSaveBuf      lsb2;
  const char32_t * line;
  size_t           off,
                   next_off,
                   sz = 0;

  for ( off = lsb.first; off > 0; ) {
    next_off = LineSave::find_substr( lsb, off, str, len, -1 );
    if ( next_off == 0 )
      break;
    const LineSave & ls = LineSave::line_const( lsb, next_off );
    sz += LineSave::size( ls.edited_len );
    off = ls.next_off;
  }

  ::memset( &lsb2, 0, sizeof( lsb2 ) );
  lsb2.buflen = sz;
  lsb2.buf = (char32_t *) ::malloc( sz * sizeof( char32_t ) );
  if ( lsb2.buf == NULL )
    return false;

  for ( off = lsb.first; off > 0; ) {
    next_off = LineSave::find_substr( lsb, off, str, len, -1 );
    if ( next_off == 0 )
      break;
    const LineSave & ls = LineSave::line_const( lsb, next_off );
    line = &lsb.buf[ ls.line_off ];
    LineSave::make( lsb2, line, ls.edited_len, ls.cursor_off, ++lsb2.cnt );
    off = ls.next_off;
  }
  ::free( lsb.buf );
  ::memcpy( &lsb, &lsb2, sizeof( lsb2 ) );
  lsb.off = lsb.max;
  return true;
}

size_t
LineSave::scan( const LineSaveBuf &lsb,  size_t i )
{
  size_t off = lsb.max;
  while ( off > 0 ) {
    const LineSave &ls = LineSave::line_const( lsb, off );
    if ( i == ls.index )
      return off;
    off = ls.line_off;
  }
  return 0;
}

size_t
LineSave::check_links( LineSaveBuf &lsb,  size_t first,  size_t max_off,
                       size_t cnt )
{
  size_t i, fwd_cnt = 0, bck_cnt = 0, last;
  if ( first == 0 ) {
    if ( max_off != 0 )
      printf( "max_off wrong\n" );
    return 0;
  }
  if ( max_off == 0 ) {
    if ( first != 0 )
      printf( "first wrong\n" );
    return 0;
  }

  last = 0;
  for ( i = max_off; i != 0; ) {
    LineSave &ls = LineSave::line( lsb, i );
    bck_cnt++;
    if ( ls.next_off != last ) {
      printf( "next_off != last @%lu\n", i );
      return 0;
    }
    last = i;
    i = ls.line_off;
    if ( i > max_off ) {
      printf( "line_off > max_off @%lu\n", i );
      return 0;
    }
  }
  last = 0;
  for ( i = first; i != 0; ) {
    LineSave &ls = LineSave::line( lsb, i );
    fwd_cnt++;
    if ( ls.line_off != last ) {
      printf( "line_off != last @%lu\n", i );
      return 0;
    }
    last = i;
    i = ls.next_off;
    if ( i > max_off ) {
      printf( "next_off > max_off @%lu\n", i );
      return 0;
    }
  }
  if ( bck_cnt != fwd_cnt ) {
    printf( "bck %lu != fwd_cnt %lu\n", bck_cnt, fwd_cnt );
  }
  if ( cnt != 0 && cnt != fwd_cnt ) {
    printf( "cnt %lu != fwd_cnt %lu\n", cnt, fwd_cnt );
  }
  return bck_cnt;
}

size_t
LineSave::resize( LineSaveBuf &lsb,  size_t &off,  size_t newsz )
{
  size_t     asize   = align<size_t>( newsz, 8 );
  LineSave & ls      = LineSave::line( lsb, off );
  size_t     osize   = align<size_t>( ls.edited_len, 8 );
  size_t     max_off = lsb.max;
  if ( osize == asize )
    return max_off;
  size_t  node_off = off - LINE_SAVE_CHARS;
  ssize_t diff     = asize - osize;
  if ( newsz == 0 ) { /* remove node */
    node_off += LINE_SAVE_CHARS;
    diff     -= LINE_SAVE_CHARS;
  }
  size_t cnt = LineSave::check_links( lsb, lsb.first, max_off, 0 );
  size_t i = max_off;
  for (;;) {
    LineSave &ls = LineSave::line( lsb, i );
    bool at_offset = ( i == off );
    i = ls.line_off;
    if ( at_offset )
      break;
    ls.line_off += diff;
  }
  move<char32_t>( &lsb.buf[ node_off + diff ], &lsb.buf[ node_off ],
                  max_off - node_off );
  off += diff;
  max_off += diff;

  size_t last = 0;
  for ( i = max_off; i != 0; ) {
    LineSave &ls = LineSave::line( lsb, i );
    ls.next_off = last;
    last = i;
    i = ls.line_off;
  }
  lsb.first = last;
  lsb.max   = max_off;
  LineSave::check_links( lsb, lsb.first, max_off,
                         cnt - ( ( newsz == 0 ) ? 1 : 0) );
  return max_off;
}

static int
cmpls( const void *ls1,  const void *ls2 )
{
  const LineSave *lsx = *(LineSave **) ls1,
                 *lsy = *(LineSave **) ls2;
  size_t lenx = lsx->edited_len, leny = lsy->edited_len;
  const char32_t * bufx = &((char32_t *) lsx)[ -align<size_t>( lenx, 8 ) ],
                 * bufy = &((char32_t *) lsy)[ -align<size_t>( leny, 8 ) ];

  return casecmp<char32_t>( bufx, bufy, lenx, leny );
}

bool
LineSave::sort( LineSaveBuf &lsb )
{
  LineSaveBuf lsb2;
  LineSave *elbuf[ 1024 ], **el = elbuf;
  size_t off, i = 0, j, last_len = 0;
  const char32_t *last_line = NULL;

  if ( lsb.first == 0 || lsb.cnt == 1 )
    return true;
  if ( lsb.cnt > 1024 ) {
    el = (LineSave **) ::malloc( lsb.cnt * sizeof( el[ 0 ] ) );
    if ( el == NULL )
      return false;
  }
  for ( off = lsb.first; off != 0; ) {
    LineSave &ls = LineSave::line( lsb, off );
    el[ i++ ] = &ls;
    off = ls.next_off;
  }
  ::qsort( el, i, sizeof( el[ 0 ] ), cmpls );

  ::memset( &lsb2, 0, sizeof( lsb2 ) );
  lsb2.buf = (char32_t *) ::malloc( lsb.buflen * sizeof( char32_t ) );
  if ( lsb2.buf == NULL ) {
    if ( el != elbuf )
      ::free( el );
    return false;
  }
  lsb2.buflen = lsb.buflen;
  off = 0;
  for ( j = 0; j < i; j++ ) {
    const char32_t * line = &lsb.buf[ el[ j ]->line_off ];
    size_t           len  = el[ j ]->edited_len;
    if ( cmp<char32_t>( last_line, line, last_len, len ) != 0 ) {
      LineSave::make( lsb2, line, len, el[ j ]->cursor_off, ++lsb2.cnt );
      last_len  = len;
      last_line = line;
    }
  }
  ::free( lsb.buf );
  ::memcpy( &lsb, &lsb2, sizeof( lsb2 ) );
  lsb.off = lsb.max;
  if ( el != elbuf )
    ::free( el );
  return true;
}

bool
LineSave::shrink_range( LineSaveBuf &lsb,  size_t off,  size_t to_off )
{
  LineSaveBuf      lsb2;
  const char32_t * line;

  ::memset( &lsb2, 0, sizeof( lsb2 ) );
  if ( to_off >= off ) {
    LineSave &first = LineSave::line( lsb, off );
    lsb2.buflen = to_off - off + LineSave::size( first.edited_len );
    lsb2.buf = (char32_t *) ::malloc( lsb2.buflen * sizeof( char32_t ) );
    if ( lsb2.buf == NULL )
      return false;
    for (;;) {
      LineSave &ls = LineSave::line( lsb, off );
      line = &lsb.buf[ ls.line_off ];
      LineSave::make( lsb2, line, ls.edited_len, ls.cursor_off, ++lsb2.cnt );
      if ( off == to_off )
        break;
      off = ls.next_off;
    }
  }
  ::free( lsb.buf );
  ::memcpy( &lsb, &lsb2, sizeof( lsb2 ) );
  lsb.off = lsb.max;
  return true;
}

static inline uint64_t /* murmur */
mmhash64( const void * key, int len, uint64_t seed )
{
  static const uint64_t m = ( (uint64_t) 0xc6a4a793U << 32 ) | 0x5bd1e995;
  static const int r = 47;

  uint64_t h = seed ^ ( len * m );

  const uint64_t * data  = (const uint64_t *) key;
  const uint64_t * end   = data + ( len / 8 );
  const uint8_t  * data2 = (const uint8_t *) end;

  while ( data != end ) {
    uint64_t k = *data++;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  switch ( len & 7 ) {
    case 7: h ^= ((uint64_t) data2[ 6 ] ) << 48; /* FALLTHRU */
    case 6: h ^= ((uint64_t) data2[ 5 ] ) << 40; /* FALLTHRU */
    case 5: h ^= ((uint64_t) data2[ 4 ] ) << 32; /* FALLTHRU */
    case 4: h ^= ((uint64_t) data2[ 3 ] ) << 24; /* FALLTHRU */
    case 3: h ^= ((uint64_t) data2[ 2 ] ) << 16; /* FALLTHRU */
    case 2: h ^= ((uint64_t) data2[ 1 ] ) << 8; /* FALLTHRU */
    case 1: h ^= ((uint64_t) data2[ 0 ] );
        h *= m;
  };

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}

bool
LineSave::shrink_unique( LineSaveBuf &lsb )
{
  LineSaveBuf      lsb2;
  uint64_t       * ht;
  uint32_t       * htcnt;
  void           * p;
  size_t           i, off, htsz, sz;
  uint64_t         h;
  const char32_t * line;

  ::memset( &lsb2, 0, sizeof( lsb2 ) );
  htsz = lsb.cnt * 2;
  if ( htsz == 0 )
    return true;
  p = ::malloc( ( sizeof( uint64_t ) + sizeof( uint32_t ) ) * htsz );
  if ( p == NULL )
    return false;
  ::memset( p, 0, ( sizeof( uint64_t ) + sizeof( uint32_t ) ) * htsz );
  ht = (uint64_t *) p;
  htcnt = (uint32_t *) (void *) &ht[ htsz ];
  sz = 0;
  /* maintain the order of the entries at the end of the list --
   * this useful for history */
  for ( off = lsb.first; off != 0; ) {
    LineSave &ls = LineSave::line( lsb, off );
    line = &lsb.buf[ ls.line_off ];
    h = mmhash64( line, ls.edited_len * sizeof( line[ 0 ] ),
                  ls.edited_len + htsz );
    for ( i = h % htsz; ; i = ( i + 1 ) % htsz ) {
      if ( ht[ i ] == 0 || ht[ i ] == h ) {
        ht[ i ] = h;
        if ( htcnt[ i ] == 0 )
          sz += LineSave::size( ls.edited_len );
        htcnt[ i ]++; /* first pass, increment counter of items */
        break;
      }
    }
    off = ls.next_off;
  }
  lsb2.buflen = sz;
  lsb2.buf = (char32_t *) ::malloc( sz * sizeof( char32_t ) );
  if ( lsb2.buf != NULL ) {
    for ( off = lsb.first; off != 0; ) {
      LineSave &ls = LineSave::line( lsb, off );
      line = &lsb.buf[ ls.line_off ];
      h = mmhash64( line, ls.edited_len * sizeof( line[ 0 ] ),
                    ls.edited_len + htsz );
      for ( i = h % htsz; ; i = ( i + 1 ) % htsz ) {
        if ( ht[ i ] == h )
          break;
      }
      if ( --htcnt[ i ] == 0 ) /* second pass, add when counter is zero */
        LineSave::make( lsb2, line, ls.edited_len, ls.cursor_off, ++lsb2.cnt );
      off = ls.next_off;
    }
    ::free( p );
    ::free( lsb.buf );
    ::memcpy( &lsb, &lsb2, sizeof( lsb2 ) );
    lsb.off = lsb.max;
    return true;
  }
  ::free( p );
  return false;
}

