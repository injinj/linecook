#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <linecook/keycook.h>

unsigned int
hash_action( const char *s )
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

int
main( int argc, char *argv[] )
{
  char         name[ 40 ],
               mode[ 40 ];
  KeyRecipe  * r;
  const char * action,
             * descr;
  size_t       i, k;

  if ( argc == 1 ) {
    printf(
    "| Key | Action | Mode | Description |\n"
    "| --- | ------ | ---- | ----------- |\n" );
    for ( i = 0; i < lc_default_key_recipe_size; i++ ) {
      r = &lc_default_key_recipe[ i ];
      if ( r->char_sequence == NULL )
        strcpy( name, "(other key)" );
      else {
        lc_key_to_name( r->char_sequence, name );
        if ( ( lc_action_options( r->action ) & OPT_VI_CHAR_ARG ) != 0 )
          strcat( name, " (k)" );
      }
      action = lc_action_to_name( r->action );
      descr  = lc_action_to_descr( r->action );
      k = 0;
      mode[ 0 ] = '\0';
      if ( ( r->valid_mode & EMACS_MODE ) != 0 ) {
        strcpy( &mode[ k ], "E" ); k++;
      }
      if ( ( r->valid_mode & VI_INSERT_MODE ) != 0 ) {
        strcpy( &mode[ k ], "I" ); k++;
      }
      if ( ( r->valid_mode & VI_COMMAND_MODE ) != 0 ) {
        strcpy( &mode[ k ], "C" ); k++;
      }
      if ( ( r->valid_mode & SEARCH_MODE ) != 0 ) {
        strcpy( &mode[ k ], "S" ); k++;
      }
      if ( ( r->valid_mode & VISUAL_MODE ) != 0 ) {
        strcpy( &mode[ k ], "V" ); k++;
      }
      printf( "| %s | %s | %s | %s |\n", name, action, mode, descr );
    }
  }
  else if ( argc == 2 && strcmp( argv[ 1 ], "hash" ) == 0 ) {
    printf(
"#include <stdlib.h>\n"
"#include <stdint.h>\n"
"#include <linecook/keycook.h>\n"
"\n"
"unsigned int\n"
"lc_hash_action_name( const char *s )\n"
"{\n"
"  unsigned int key = 5381;\n"
"  while ( *s != '\\0' ) {\n"
"    int c = *s++;\n"
"    if ( c != '_' && c != '-' ) { /* skip '-', go-left == goleft */\n"
"      c &= ~0x20; /* upcase */\n"
"      key = ( (unsigned int) (unsigned char) c ) ^ ( ( key << 5 ) + key );\n"
"    }\n"
"  }\n"
"  return key;\n"
"}\n"
"\n"
"KeyAction\n"
"lc_string_to_action( const char *s )\n"
"{\n"
"  return lc_hash_to_action( lc_hash_action_name( s ) );\n"
"}\n"
"\n"
"KeyAction\n"
"lc_hash_to_action( unsigned int h )\n"
"{\n"
"  int a;\n"
"  switch ( h ) {\n" );
    for ( i = ACTION_PENDING + 1; i < ACTION_MACRO; i++ ) {
      printf( "    case 0x%x: a = %ld; break;\n",
              hash_action( lc_action_to_name( (KeyAction) i ) ), i );
    }
    printf(
"    default: a = 0; break;\n"
"  }\n"
"  return (KeyAction) a;\n"
"}\n" );
  }
  return 0;
}

