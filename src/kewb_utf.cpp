/* Originally from: https://githup.com/BobSteagall/CppCon2018
//==================================================================================================
//  File:       unicode_utils.cpp
//
//  Copyright (c) 2018 Bob Steagall and KEWB Computing, All Rights Reserved
//==================================================================================================
*/
#include <stdio.h>
#include <wctype.h>
#include <linecook/kewb_utf.h>

using namespace kewb_uu;

extern "C" {

int
ku_utf8_to_utf32( const char *src,  size_t len,  char32_t *cdpt )
{
  if ( len > 0 ) {
    const uint8_t * u = (const uint8_t *) (const void *) src;
    size_t    i    = 0;
    char32_t  unit = u[ i++ ];
    CharClass type = kewb_uu::smTables.maOctetCategory[ unit ];
    State     curr = kewb_uu::smTables.maTransitions[ type ];

    *cdpt = kewb_uu::smTables.maFirstOctetMask[ type ] & unit;

    for (;;) {
      if ( curr == End )
        return i;
      if ( curr == Err )
        return -1;
      if ( i == len )
        return 0;
      unit  = u[ i++ ];
      *cdpt = ( *cdpt << 6 ) | ( unit & 0x3F );
      type  = kewb_uu::smTables.maOctetCategory[ unit ];
      curr  = kewb_uu::smTables.maTransitions[ curr + type ];
    }
  }
  return 0;
}

int
ku_utf8_to_utf32_len( const char *src,  size_t len )
{
  const uint8_t * u = (const uint8_t *) (const void *) src;
  size_t i = 0, j;

  for ( j = 0; i < len; j++ ) {
    CharClass type = kewb_uu::smTables.maOctetCategory[ u[ i++ ] ];
    State     curr = kewb_uu::smTables.maTransitions[ type ];

    for (;;) {
      if ( curr == End )
        break;
      if ( curr == Err )
        return -1;
      if ( i == len )
        return 0;
      type  = kewb_uu::smTables.maOctetCategory[ u[ i++ ] ];
      curr  = kewb_uu::smTables.maTransitions[ curr + type ];
    }
  }
  return (int) j;
}

int
ku_utf8_to_utf32_copy( const char *src,  size_t len,  char32_t *dst,
                       size_t dst_len )
{
  size_t i = 0, j;

  for ( j = 0; i < len && j < dst_len; j++ ) {
    int n = ku_utf8_to_utf32( &src[ i ], len - i, &dst[ j ] );
    if ( n <= 0 )
      return n;
    i += (size_t) n;
  }
  return (int) j;
}

int
ku_utf32_to_utf8( char32_t cdpt,  char *dst )
{
  uint8_t * u = (uint8_t *) (void *) dst;

  if ( cdpt <= 0x7F ) {
    u[ 0 ] = (uint8_t) cdpt;
    return 1;
  }
  if ( cdpt <= 0x7FF ) {
    u[ 0 ] = (uint8_t) ( 0xC0 | ((cdpt >> 6) & 0x1F) );
    u[ 1 ] = (uint8_t) ( 0x80 | (cdpt        & 0x3F) );
    return 2;
  }
  if ( cdpt <= 0xFFFF ) {
    u[ 0 ] = (uint8_t) ( 0xE0 | ((cdpt >> 12) & 0x0F) );
    u[ 1 ] = (uint8_t) ( 0x80 | ((cdpt >> 6)  & 0x3F) );
    u[ 2 ] = (uint8_t) ( 0x80 | (cdpt         & 0x3F) );
    return 3;
  }
  if ( cdpt <= 0x10FFFF ) {
    u[ 0 ] = (uint8_t) ( 0xF0 | ((cdpt >> 18) & 0x07) );
    u[ 1 ] = (uint8_t) ( 0x80 | ((cdpt >> 12) & 0x3F) );
    u[ 2 ] = (uint8_t) ( 0x80 | ((cdpt >> 6)  & 0x3F) );
    u[ 3 ] = (uint8_t) ( 0x80 | (cdpt         & 0x3F) );
    return 4;
  }
  return -1;
}

int
ku_utf32_to_utf8_len( const char32_t *src,  size_t len )
{
  size_t i = 0, j;

  for ( j = 0; j < len; j++ ) {
    int n = ( src[ j ] <= 0x7F ) ? 1 :
            ( src[ j ] <= 0x7FF ) ? 2 :
            ( src[ j ] <= 0xFFFF ) ? 3 :
            ( src[ j ] <= 0x10FFFF ) ? 4 : -1;
    if ( n < 0 )
      return n;
    i += (size_t) n;
  }
  return (int) i;
}

int
ku_utf32_to_utf8_copy( const char32_t *src,  size_t len,  char *dst,
                       size_t dst_len )
{
  size_t i = 0, j = 0;
  int n;

  while ( i + 4 <= dst_len && j < len ) {
    if ( (n = ku_utf32_to_utf8( src[ j++ ], &dst[ i ] )) <= 0 )
      return n;
    i += n;
  }
  while ( j < len ) {
    char tmp[ 4 ];
    tmp[ 0 ] = tmp[ 1 ] = tmp[ 2 ] = tmp[ 3 ] = 0;
    if ( (n = ku_utf32_to_utf8( src[ j++ ], tmp )) <= 0 )
      return n;
    if ( i + n > dst_len )
      break;
    dst[ i++ ] = tmp[ 0 ];
    for ( int k = 1; k < n; k++ )
      dst[ i++ ] = tmp[ k ];
  }
  return (int) i;
}

int
ku_islocase_utf32( char32_t c )
{
  return iswlower( c );
}

int
ku_isupcase_utf32( char32_t c )
{
  return iswupper( c );
}

char32_t
ku_locase_utf32( char32_t c )
{
  return towlower( c );
}

char32_t
ku_upcase_utf32( char32_t c )
{
  return towupper( c );
}

}

//
//- Static member data init.
//
const LookupTables kewb_uu::smTables =
{
    //- Initialize the maOctetCategory member array.  This array implements a lookup table
    //  that maps an input octet to a corresponding octet category.
    //
    //   0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
    //============================================================================================
    {
        Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, //- 00..0F
        Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, //- 10..1F
        Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, //- 20..2F
        Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, //- 30..3F
                                                                                        //
        Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, //- 40..4F
        Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, //- 50..5F
        Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, //- 60..6F
        Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, Asc, //- 70..7F
                                                                                        //
        Cr1, Cr1, Cr1, Cr1, Cr1, Cr1, Cr1, Cr1, Cr1, Cr1, Cr1, Cr1, Cr1, Cr1, Cr1, Cr1, //- 80..8F
        Cr2, Cr2, Cr2, Cr2, Cr2, Cr2, Cr2, Cr2, Cr2, Cr2, Cr2, Cr2, Cr2, Cr2, Cr2, Cr2, //- 90..9F
        Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, //- A0..AF
        Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, Cr3, //- B0..BF
                                                                                        //
        Ill, Ill, L2a, L2a, L2a, L2a, L2a, L2a, L2a, L2a, L2a, L2a, L2a, L2a, L2a, L2a, //- C0..CF
        L2a, L2a, L2a, L2a, L2a, L2a, L2a, L2a, L2a, L2a, L2a, L2a, L2a, L2a, L2a, L2a, //- D0..DF
        L3a, L3b, L3b, L3b, L3b, L3b, L3b, L3b, L3b, L3b, L3b, L3b, L3b, L3c, L3b, L3b, //- E0..EF
        L4a, L4b, L4b, L4b, L4c, Ill, Ill, Ill, Ill, Ill, Ill, Ill, Ill, Ill, Ill, Ill, //- F0..FF
    },

    //- Initialize the maTransitions member array.  This array implements a lookup table that,
    //  given the current DFA state and an input code unit, indicates the next DFA state. 
    //
    //  ILL  ASC  CR1  CR2  CR3  L2A  L3A  L3B  L3C  L4A  L4B  L4C  CLASS/STATE
    //=========================================================================
    {
        Err, End, Err, Err, Err, Cs1, P3a, Cs2, P3b, P4a, Cs3, P4b,   //- BGN|END
        Err, Err, Err, Err, Err, Err, Err, Err, Err, Err, Err, Err,   //- ERR
                                                                      //
        Err, Err, End, End, End, Err, Err, Err, Err, Err, Err, Err,   //- CS1
        Err, Err, Cs1, Cs1, Cs1, Err, Err, Err, Err, Err, Err, Err,   //- CS2
        Err, Err, Cs2, Cs2, Cs2, Err, Err, Err, Err, Err, Err, Err,   //- CS3
                                                                      //
        Err, Err, Err, Err, Cs1, Err, Err, Err, Err, Err, Err, Err,   //- P3A
        Err, Err, Cs1, Cs1, Err, Err, Err, Err, Err, Err, Err, Err,   //- P3B
                                                                      //
        Err, Err, Err, Cs2, Cs2, Err, Err, Err, Err, Err, Err, Err,   //- P4A
        Err, Err, Cs2, Err, Err, Err, Err, Err, Err, Err, Err, Err,   //- P4B
    },

    //- Initialize the maFirstOctetMask member array.  This array implements a lookup table that
    //  maps a character class to a mask that is applied to the first code unit in a sequence.
    //
    {
        0xFF,   //- ILL - C0..C1, F5..FF    Illegal code unit
                //
        0x7F,   //- ASC - 00..7F            ASCII byte range
                //
        0x3F,   //- CR1 - 80..8F            Continuation range 1
        0x3F,   //- CR2 - 90..9F            Continuation range 2
        0x3F,   //- CR3 - A0..BF            Continuation range 3
                //
        0x1F,   //- L2A - C2..DF            Leading byte range 2A / 2-byte sequence
                //
        0x0F,   //- L3A - E0                Leading byte range 3A / 3-byte sequence
        0x0F,   //- L3B - E1..EC, EE..EF    Leading byte range 3B / 3-byte sequence
        0x0F,   //- L3C - ED                Leading byte range 3C / 3-byte sequence
                //
        0x07,   //- L4A - F0                Leading byte range 4A / 4-byte sequence
        0x07,   //- L4B - F1..F3            Leading byte range 4B / 4-byte sequence
        0x07,   //- L4C - F4                Leading byte range 4C / 4-byte sequence
    },
};

