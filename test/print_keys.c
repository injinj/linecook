#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <linecook/keycook.h>

int
main( void )
{
  char         name[ 40 ],
               mode[ 40 ];
  KeyRecipe  * r;
  const char * action,
             * descr;
  size_t       i, k;

  printf(
  "| Key | Action | Mode | Description |\n"
  "| --- | ------ | ---- | ----------- |\n" );
  for ( i = 0; i < lc_default_key_recipe_size; i++ ) {
    r = &lc_default_key_recipe[ i ];
    if ( r->char_sequence == NULL )
      strcpy( name, "(other key)" );
    else {
      lc_key_to_name( r->char_sequence, name );
      if ( ( r->options & OPT_VI_CHAR_OP ) != 0 )
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
  return 0;
}

