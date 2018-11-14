/* Originally from: https://githup.com/BobSteagall/CppCon2018
// ==================================================================================================
//  File:       unicode_utils.h
//
//  Summary:    Header file for fast UTF-8 to UTF-32/UTF-16 conversion routines.
//
//  Copyright (c) 2018 Bob Steagall and KEWB Computing, All Rights Reserved
//==================================================================================================
*/
#ifndef __kewb_utf_h__
#define __kewb_utf_h__

#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

/* convert at most 4 utf8 chars to one utf32 char */
int ku_utf8_to_utf32( const char *src,  size_t len,  char32_t *cdpt );

/* return length of utf32 */
int ku_utf8_to_utf32_len( const char *src,  size_t len );

/* return length of utf32 copy */
int ku_utf8_to_utf32_copy( const char *src,  size_t len,  char32_t *dst,
                           size_t dst_len );
/* convert one utf32 char to at most 4 utf8 */
int ku_utf32_to_utf8( char32_t cdpt,  char *dst );

/* return length of utf8 */
int ku_utf32_to_utf8_len( const char32_t *src,  size_t len );

/* return length of utf8 copy */
int ku_utf32_to_utf8_copy( const char32_t *src,  size_t len,  char *dst,
                           size_t dst_len );

int ku_islocase_utf32( char32_t c );

int ku_isupcase_utf32( char32_t c );

char32_t ku_locase_utf32( char32_t c );

char32_t ku_upcase_utf32( char32_t c );
#ifdef __cplusplus
}

namespace kewb_uu {

enum CharClass
{
  Ill = 0,    //- C0..C1, F5..FF  ILLEGAL octets that should never appear in a UTF-8 sequence
              //
  Asc = 1,    //- 00..7F          ASCII leading byte range
              //
  Cr1 = 2,    //- 80..8F          Continuation range 1
  Cr2 = 3,    //- 90..9F          Continuation range 2
  Cr3 = 4,    //- A0..BF          Continuation range 3
              //
  L2a = 5,    //- C2..DF          Leading byte range A / 2-byte sequence
              //
  L3a = 6,    //- E0              Leading byte range A / 3-byte sequence
  L3b = 7,    //- E1..EC, EE..EF  Leading byte range B / 3-byte sequence
  L3c = 8,    //- ED              Leading byte range C / 3-byte sequence
              //
  L4a = 9,    //- F0              Leading byte range A / 4-byte sequence
  L4b = 10,   //- F1..F3          Leading byte range B / 4-byte sequence
  L4c = 11,   //- F4              Leading byte range C / 4-byte sequence
};

enum State
{
  Bgn = 0,    //- Start
  Err = 12,   //- Invalid sequence
              //
  Cs1 = 24,   //- Continuation state 1
  Cs2 = 36,   //- Continuation state 2
  Cs3 = 48,   //- Continuation state 3
              //
  P3a = 60,   //- Partial 3-byte sequence state A
  P3b = 72,   //- Partial 3-byte sequence state B
              //
  P4a = 84,   //- Partial 4-byte sequence state A
  P4b = 96,   //- Partial 4-byte sequence state B
              //
  End = Bgn   //- Start and End are the same state!
};

struct LookupTables
{
  CharClass maOctetCategory[ 256 ];
  State     maTransitions[ 108 ];
  uint8_t   maFirstOctetMask[ 16 ];
};

extern const LookupTables smTables;

} //- namespace kewb_uu

#endif // cplusplus

#endif
