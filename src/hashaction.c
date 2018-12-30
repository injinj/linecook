#include <stdlib.h>
#include <stdint.h>
#include <linecook/keycook.h>

unsigned int
lc_hash_action_name( const char *s )
{
  unsigned int key = 5381;
  while ( *s != '\0' ) {
    int c = *s++;
    if ( c != '_' && c != '-' ) { /* skip '-', go-left == goleft */
      c &= ~0x20; /* upcase */
      key = ( (unsigned int) (unsigned char) c ) ^ ( ( key << 5 ) + key );
    }
  }
  return key;
}

KeyAction
lc_string_to_action( const char *s )
{
  return lc_hash_to_action( lc_hash_action_name( s ) );
}

KeyAction
lc_hash_to_action( unsigned int h )
{
  int a;
  switch ( h ) {
    case 0x928c074a: a = 1; break;
    case 0x27079438: a = 2; break;
    case 0x19c0e7a1: a = 3; break;
    case 0xf8c7797: a = 4; break;
    case 0x217456c2: a = 5; break;
    case 0xc463790f: a = 6; break;
    case 0x87477d04: a = 7; break;
    case 0x21e89e8a: a = 8; break;
    case 0xf8c7390: a = 9; break;
    case 0x9b926976: a = 10; break;
    case 0x103e04ad: a = 11; break;
    case 0x802b53a9: a = 12; break;
    case 0xb0692d76: a = 13; break;
    case 0xac52b81f: a = 14; break;
    case 0x75746980: a = 15; break;
    case 0xf18f7c3e: a = 16; break;
    case 0xeb08fddf: a = 17; break;
    case 0x1711af69: a = 18; break;
    case 0xb7e57c7: a = 19; break;
    case 0x337dbd71: a = 20; break;
    case 0x280808f0: a = 21; break;
    case 0x27fc3ac6: a = 22; break;
    case 0xebd16c5b: a = 23; break;
    case 0xd0e86532: a = 24; break;
    case 0xe8725ea1: a = 25; break;
    case 0x35633bd7: a = 26; break;
    case 0xcc3cf329: a = 27; break;
    case 0x3b094eec: a = 28; break;
    case 0x75af2b4d: a = 29; break;
    case 0x88b259ab: a = 30; break;
    case 0x13cd2282: a = 31; break;
    case 0xd36a01ce: a = 32; break;
    case 0x2011e022: a = 33; break;
    case 0x9d801ced: a = 34; break;
    case 0x47092b56: a = 35; break;
    case 0x2e050a2b: a = 36; break;
    case 0xb8b0109f: a = 37; break;
    case 0xafee105c: a = 38; break;
    case 0x1a0f10bd: a = 39; break;
    case 0x8a0274c4: a = 40; break;
    case 0x8a0258c3: a = 41; break;
    case 0xca540d8b: a = 42; break;
    case 0x6437fb1a: a = 43; break;
    case 0xb13be72c: a = 44; break;
    case 0x39d45bd3: a = 45; break;
    case 0x62fea0e2: a = 46; break;
    case 0x103334be: a = 47; break;
    case 0x531d3bd0: a = 48; break;
    case 0xc3452b76: a = 49; break;
    case 0xa7d751bc: a = 50; break;
    case 0x9e90cc4a: a = 51; break;
    case 0x7c855235: a = 52; break;
    case 0xc21ba5c7: a = 53; break;
    case 0x7c88d919: a = 54; break;
    case 0x50953978: a = 55; break;
    case 0x639a3c41: a = 56; break;
    case 0x5f032b6c: a = 57; break;
    case 0x947af459: a = 58; break;
    case 0x23d43db6: a = 59; break;
    case 0x23d12e56: a = 60; break;
    case 0x947b005e: a = 61; break;
    case 0x922b0306: a = 62; break;
    case 0x75d66f7e: a = 63; break;
    case 0x947a97d7: a = 64; break;
    case 0xb9a43039: a = 65; break;
    case 0xd62306f: a = 66; break;
    case 0xaa6eba0d: a = 67; break;
    case 0x21b4a9cc: a = 68; break;
    case 0xca9dcfd6: a = 69; break;
    case 0x822f604e: a = 70; break;
    case 0xbb9af934: a = 71; break;
    case 0xd601ea52: a = 72; break;
    case 0x3a52a61d: a = 73; break;
    case 0x84a24972: a = 74; break;
    case 0x849f3912: a = 75; break;
    case 0x84aa3ae4: a = 76; break;
    case 0x3a52921a: a = 77; break;
    case 0xa82658d2: a = 78; break;
    case 0xaf1b80b3: a = 79; break;
    case 0x97889aab: a = 80; break;
    case 0x1f59422d: a = 81; break;
    case 0xeddd8a98: a = 82; break;
    case 0xb9a414ef: a = 83; break;
    case 0xa377cb9c: a = 84; break;
    case 0x1d5209a4: a = 85; break;
    case 0xac80dff7: a = 86; break;
    case 0xb2570e08: a = 87; break;
    case 0x1d4713ac: a = 88; break;
    case 0x656408da: a = 89; break;
    case 0xf9a5233f: a = 90; break;
    case 0x5dc10dca: a = 91; break;
    case 0x5dc1c78d: a = 92; break;
    case 0x15781a13: a = 93; break;
    case 0x8fbf4364: a = 94; break;
    case 0x5dc4b942: a = 95; break;
    case 0x15cdd4bf: a = 96; break;
    case 0x5dca61b6: a = 97; break;
    case 0x5dc67490: a = 98; break;
    case 0x5dd0c63b: a = 99; break;
    case 0x56bba784: a = 100; break;
    case 0x19d5c0f2: a = 101; break;
    case 0xc86b2a78: a = 102; break;
    case 0xead705c8: a = 103; break;
    case 0x2477fba2: a = 104; break;
    case 0xf8c1b1d: a = 105; break;
    case 0xc51e155b: a = 106; break;
    case 0xb59aab19: a = 107; break;
    case 0x6e9f53f6: a = 108; break;
    case 0x2693b350: a = 109; break;
    default: a = 0; break;
  }
  return (KeyAction) a;
}
