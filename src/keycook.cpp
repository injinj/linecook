#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <linecook/linecook.h>

using namespace linecook;

void
State::free_recipe( void ) noexcept
{
  RecipeNode *n, *next = NULL;
  /* free macros */
  for ( n = this->recipe_list.hd; n != NULL; n = next ) {
    next = n->next;
    ::free( n );
  }
  /* free recipes */
  if ( this->recipe != NULL )
    ::free( this->recipe );
}

int
State::set_recipe( KeyRecipe *key_rec,  size_t key_rec_count ) noexcept
{
  KeyRecipe  * new_rec;
  size_t       cur_size,
               start     = 0,
               macro_cnt = 0;
  RecipeNode * n;
  /* count macros */
  for ( n = this->recipe_list.hd; n != NULL; n = n->next )
    macro_cnt++;
  /* new recipe, not reusing existing */
  if ( key_rec_count != 0 )
    cur_size = key_rec_count;
  else {
    /* macros are always at the head */
    for ( start = 0; start < this->recipe_size; start++ )
      if ( this->recipe[ start ].action != ACTION_MACRO )
        break;
    cur_size = this->recipe_size - start;
  }
  /* allocate new space for recipes + macros */
  void *p = ::malloc( ( cur_size + macro_cnt ) * sizeof( new_rec[ 0 ] ) );
  if ( p == NULL )
    return LINE_STATUS_ALLOC_FAIL;
  /* state vars which index into this->recipe[] */
  this->reset_input( this->in );
  this->last_repeat_recipe = NULL;
  /* copy recipes and put macros at head */
  new_rec = (KeyRecipe *) p;
  if ( key_rec_count != 0 )
    ::memcpy( &new_rec[ macro_cnt ], key_rec,
              sizeof( new_rec[ 0 ] ) * key_rec_count );
  else
    ::memcpy( &new_rec[ macro_cnt ], &this->recipe[ start ],
              sizeof( new_rec[ 0 ] ) * cur_size );
  macro_cnt = 0;
  for ( n = this->recipe_list.hd; n != NULL; n = n->next )
    new_rec[ macro_cnt++ ] = n->r;
  /* initialize the recipe for key sequence translations */
  if ( this->recipe != NULL )
    ::free( this->recipe );
  this->recipe      = new_rec;
  this->recipe_size = cur_size + macro_cnt;

  this->init_single_key_transitions( this->emacs, EMACS_MODE );
  this->init_single_key_transitions( this->vi_insert, VI_INSERT_MODE );
  this->init_single_key_transitions( this->vi_command, VI_COMMAND_MODE );
  this->init_single_key_transitions( this->visual, VISUAL_MODE );
  this->init_single_key_transitions( this->search, SEARCH_MODE );

  /* set up multi char sequences */
  KeyRecipe ** mc = this->multichar;
  size_t       sz = 0;

  sz += this->emacs.mc_size;
  sz += this->vi_insert.mc_size;
  sz += this->vi_command.mc_size;
  sz += this->visual.mc_size;
  sz += this->search.mc_size;

  mc = (KeyRecipe **) ::realloc( mc, sizeof( mc[ 0 ] ) * sz );
  if ( mc == NULL )
    return -1;

  this->multichar      = mc;
  this->multichar_size = sz;

  this->emacs.mc      = mc; mc += this->emacs.mc_size;
  this->vi_insert.mc  = mc; mc += this->vi_insert.mc_size;
  this->vi_command.mc = mc; mc += this->vi_command.mc_size;
  this->visual.mc     = mc; mc += this->visual.mc_size;
  this->search.mc     = mc; mc += this->search.mc_size;

  this->init_multi_key_transitions( this->emacs, EMACS_MODE );
  this->init_multi_key_transitions( this->vi_insert, VI_INSERT_MODE );
  this->init_multi_key_transitions( this->vi_command, VI_COMMAND_MODE );
  this->init_multi_key_transitions( this->visual, VISUAL_MODE );
  this->init_multi_key_transitions( this->search, SEARCH_MODE );

  for ( size_t i = 0; i < macro_cnt; i++ ) {
    this->filter_macro( this->emacs, EMACS_MODE, this->recipe[ i ] );
    this->filter_macro( this->vi_insert, VI_INSERT_MODE, this->recipe[ i ] );
    this->filter_macro( this->vi_command, VI_COMMAND_MODE, this->recipe[ i ] );
    this->filter_macro( this->visual, VISUAL_MODE, this->recipe[ i ] );
    this->filter_macro( this->search, SEARCH_MODE, this->recipe[ i ] );
  }

  LineSave::reset( this->keys );
  this->keys_pg = 0;
  if ( this->show_mode == SHOW_KEYS )
    this->show_keys();
  return 0;
}

extern "C"
int
lc_bindkey( LineCook *state,  char *args[],  size_t argc )
{
  return static_cast<linecook::State *>( state )->bindkey( args, argc );
}

static char
hex_code( char c1,  char c2 )
{
  c1 = ( c1 >= '0' && c1 <= '9' ) ? c1 - '0' :
       ( c1 >= 'a' && c1 <= 'f' ) ? ( c1 - 'a' ) + 10 :
       ( c1 >= 'A' && c1 <= 'F' ) ? ( c1 - 'A' ) + 10 : 16;
  c2 = ( c2 >= '0' && c2 <= '9' ) ? c2 - '0' :
       ( c2 >= 'a' && c2 <= 'f' ) ? ( c2 - 'a' ) + 10 :
       ( c2 >= 'A' && c2 <= 'F' ) ? ( c2 - 'A' ) + 10 : 16;
  if ( (unsigned char) ( c1 | c2 ) < 16 )
    return (char) ( ( (unsigned char) c1 << 4 ) | (unsigned char) c2 );
  return 0;
}

static char
octal_code( char c1,  char c2,  char c3 )
{
  c1 = ( c1 >= '0' && c1 <= '7' ) ? c1 - '0' : 8;
  c2 = ( c2 >= '0' && c2 <= '7' ) ? c2 - '0' : 8;
  c3 = ( c3 >= '0' && c3 <= '7' ) ? c3 - '0' : 8;
  if ( (unsigned char) ( c1 | c2 | c3 ) < 8 )
    return (char) ( ( (unsigned char) c1 << 6 ) |
                    ( (unsigned char) c2 << 3 ) |
                      (unsigned char) c3 );
  return 0;
}

static int
parse_key( char *key,  size_t maxsize,  const char *keyseq )
{
  size_t n = 0;
  const char * x;
  while ( *keyseq != '\0' && n + 1 < maxsize ) {
    int sz = 1;
    switch ( keyseq[ 0 ] ) {
      case '\\':
        x = NULL;
        sz = 2;
        switch ( keyseq[ 1 ] ) {
          case 'c': /* ctrl key */
          case 'C':
            if ( keyseq[ 2 ] == '-' ) { /* ctrl key */
              if ( keyseq[ 3 ] >= 'A' &&
                   (uint8_t) keyseq[ 3 ] <= (uint8_t) ( 'A' + 30 ) )
                key[ n++ ] = keyseq[ 3 ] - 'A' + 1;
              else if ( keyseq[ 3 ] >= 'a' &&
                        (uint8_t) keyseq[ 3 ] <= (uint8_t) ( 'a' + 30 ) )
                key[ n++ ] = keyseq[ 3 ] - 'a' + 1;
              else
                return -1;
              sz = 4;
              break;
            }
            return -2;
          case 'm': /* meta key */
          case 'M':
            if ( keyseq[ 2 ] == '-' ) { /* <esc> key */
              if ( keyseq[ 3 ] == 0 )
                return -3;
              key[ n++ ] = KEY_ESC[ 0 ];
              key[ n++ ] = keyseq[ 3 ];
              sz = 4;
              break;
            }
            return -4;
          case 'a': x = KEY_CTRL_A;     break; /* \a, \b, \c ... */
          case 'b': x = KEY_BACKSPACE;  break;
          case 'd': x = KEY_CTRL_D;     break;
          case 'e': x = KEY_ESC;        break;
          case 'f': x = KEY_CTRL_F;     break;
          case 'n': x = KEY_ENTER;      break;
          case 'r': x = KEY_CTRL_R;     break;
          case 't': x = KEY_TAB;        break;
          case 'v': x = KEY_CTRL_K;     break;
          case 'x': /* \xXX */
            if ( (key[ n++ ] = hex_code( keyseq[ 2 ], keyseq[ 3 ] )) == 0 )
              return -5; /* bad hex */
            sz = 4;
            break;
          case '"':  key[ n++ ] = '"';  break;
          case '\\': key[ n++ ] = '\\'; break;
          default: /* \nnn */
            if ( keyseq[ 1 ] >= '0' && keyseq[ 2 ] <= '9' ) {
              if ( (key[ n++ ] = octal_code( keyseq[ 1 ], keyseq[ 2 ],
                                             keyseq[ 3 ] )) == 0 )
                return -6; /* bad octal */
              sz = 4;
              break;
            }
            return -7; /* unknown escape */
        }
        if ( x != NULL )
          do { key[ n++ ] = *x++; } while ( *x != '\0' );
        break;
      default:
        key[ n++ ] = keyseq[ 0 ];
        break;
    }
    keyseq = &keyseq[ sz ];
  }
  if ( *keyseq != '\0' ) /* overflow */
    return -8;
  return (int) n;
}

int
State::bindkey( char *args[],  size_t argc ) noexcept
{
  uint8_t valid_mode = VI_EMACS_MODE; /* default: vi insert & emacs */
  uint8_t parse_mode = 0;
  char key[ 16 ]/*, name[ 40 ]*/;
  int  n, status = 0;

  while ( argc > 0 && args[ 0 ][ 0 ] == '-' ) {
    const char *s = &args[ 0 ][ 1 ];
    for ( ; *s != '\0'; s++ ) {
      switch ( *s ) {
        case 'i': case 'I': parse_mode |= VI_INSERT_MODE;  break;
        case 'e': case 'E': parse_mode |= EMACS_MODE;      break;
        case 'c': case 'C': parse_mode |= VI_COMMAND_MODE; break;
        case 's': case 'S': parse_mode |= SEARCH_MODE;     break;
        case 'v': case 'V': parse_mode |= VISUAL_MODE;     break;
        default: break;
      }
    }
    argc--;
    args++;
    valid_mode = parse_mode;
  }
  if ( argc > 0 ) {
    n = parse_key( key, sizeof( key ), args[ 0 ] );
    if ( n < 0 )
      return -1;
    if ( argc == 1 )
      status = this->remove_bindkey_recipe( key, n );
    else
      status = this->add_bindkey_recipe( key, n, &args[ 1 ], argc - 1,
                                         valid_mode );
    if ( status == 0 )
      status = this->set_recipe( NULL, 0 );
  }
  return status;
}

void
State::push_bindkey_recipe( void ) noexcept
{
  size_t len = ::strlen( this->in.cur_recipe->char_sequence );
  RecipeNode *n;
  KeyAction a;
  size_t i, j;
  ::memcpy( &n, &this->in.cur_recipe->char_sequence[ len + 1 ],
            sizeof( RecipeNode * ) );
  for ( i = 0; i < n->argc; i++ ) {
    const char *s = n->args[ i ];
    len = ::strlen( s );
    if ( ! this->realloc_input( this->in.input_len + len + 3 ) )
      return;
    char * buf = &this->in.input_buf[ this->in.input_len ];
    if ( len > 1 && s[ 0 ] == '&' &&
         (a = lc_string_to_action( &s[ 1 ] )) > 0 ) {
      for ( j = 0; KEY_CX_ACTION[ j ] != 0; j++ )
        buf[ j ] = KEY_CX_ACTION[ j ];
      buf[ j ] = (char) a;
      len = j + 1;
    }
    else {
      ::memcpy( buf, s, len );
    }
    this->in.input_len += len;
  }
}

int
State::add_bindkey_recipe( const char *key,  size_t keylen,  char **args,
                           size_t argc,  uint8_t valid_mode ) noexcept
{
  RecipeNode *n;
  size_t i, len = sizeof( RecipeNode ) + keylen + 1 + sizeof( RecipeNode * );
  char *s;
  void *p;

  for ( i = 0; i < argc; i++ )
    len += ::strlen( args[ i ] ) + 1;
  len += sizeof( args[ 0 ] ) * argc;
  this->remove_bindkey_recipe( key, keylen );
  p = ::malloc( len );
  if ( p == NULL )
    return LINE_STATUS_ALLOC_FAIL;
  ::memset( p, 0, len );

  n       = (RecipeNode *) p;
  n->args = (char **) &n[ 1 ];
  n->argc = argc;
  s       = (char *) &n->args[ argc ];
  ::memcpy( s, key, keylen );
  s[ keylen ] = '\0';
  n->r.char_sequence = (const char *) s;
  s = &s[ keylen + 1 ];
  ::memcpy( s, &n, sizeof( RecipeNode * ) );
  s = &s[ sizeof( RecipeNode * ) ];
  for ( i = 0; i < argc; i++ ) {
    n->args[ i ] = s;
    ::strcpy( s, args[ i ] );
    s = &s[ ::strlen( s ) + 1 ];
  }
  n->r.action     = ACTION_MACRO;
  n->r.valid_mode = valid_mode;

  if ( this->recipe_list.tl == NULL )
    this->recipe_list.hd = n;
  else
    this->recipe_list.tl->next = n;
  n->back = this->recipe_list.tl;
  this->recipe_list.tl = n;
  return 0;
}

int
State::remove_bindkey_recipe( const char *key,  size_t keylen ) noexcept
{
  RecipeNode *n;
  for ( n = this->recipe_list.hd; n != NULL; n = n->next ) {
    if ( ::memcmp( key, n->r.char_sequence, keylen ) == 0 &&
         n->r.char_sequence[ keylen ] == '\0' ) {
      if ( n->back == NULL )
        this->recipe_list.hd = n->next;
      else
        n->back->next = n->next;
      if ( n->next == NULL )
        this->recipe_list.tl = n->back;
      else
        n->next->back = n->back;
      n->next = n->back = NULL;
      ::free( n );
      break;
    }
  }
  return 0;
}

void
State::init_single_key_transitions( LineKeyMode &km,  uint8_t mode ) noexcept
{
  size_t i;
  /* find the default transition */
  km.def = 0;
  km.mode = mode;
  for ( i = 0; i < this->recipe_size; i++ ) {
    if ( this->recipe[ i ].char_sequence == NULL ) {
      if ( ( this->recipe[ i ].valid_mode & mode ) != 0 ) {
        km.def = i;
        break;
      }
    }
  }
  /* clear all transitions to the default recipe */
  for ( i = 0; i < sizeof( km.recipe ); i++ )
    km.recipe[ i ] = km.def;

  /* set up single char sequences */
  km.mc_size = 0;
  for ( i = 0; i < this->recipe_size; i++ ) {
    if ( this->recipe[ i ].char_sequence != NULL ) {
      uint8_t  c = this->recipe[ i ].char_sequence[ 0 ],
              c2 = this->recipe[ i ].char_sequence[ 1 ];

      if ( ( this->recipe[ i ].valid_mode & mode ) != 0 ) {
        if ( km.recipe[ c ] == km.def ) {
          km.recipe[ c ] = i; /* transition for char -> this->recipe[ i ] */
        }
        else if ( c2 == 0 ) {/* the single char sequence wins */
          if ( this->recipe[ km.recipe[ c ] ].char_sequence[ 1 ] != 0 )
            km.recipe[ c ] = i;
        }
        if ( c2 != 0 ) /* if is a multichar sequence */
          km.mc_size++;
      }
    }
  }
}

void
State::init_multi_key_transitions( LineKeyMode &km,  uint8_t mode ) noexcept
{
  size_t i, j = 0;
  for ( i = 0; i < this->recipe_size; i++ )
    if ( this->recipe[ i ].char_sequence != NULL )
      if ( this->recipe[ i ].char_sequence[ 1 ] != 0 )
        if ( ( this->recipe[ i ].valid_mode & mode ) != 0 )
          km.mc[ j++ ] = &this->recipe[ i ];
}

void
State::filter_macro( LineKeyMode &km,  uint8_t mode,  KeyRecipe &r ) noexcept
{
  size_t i;
  if ( ( r.valid_mode & mode ) == 0 )
    return;
  for ( i = 0; i < km.mc_size; i++ ) {
    if ( km.mc[ i ] != &r &&
         ::strcmp( km.mc[ i ]->char_sequence, r.char_sequence ) == 0 )
      break;
  }
  if ( i == km.mc_size )
    return;
  ::memmove( &km.mc[ i ], &km.mc[ i + 1 ],
             sizeof( km.mc[ 0 ] ) * ( km.mc_size - ( i + 1 ) ) );
  km.mc_size -= 1;
}

void
State::filter_mode( LineKeyMode &km,  uint8_t &mode,  KeyRecipe &r ) noexcept
{
  if ( r.char_sequence[ 1 ] == '\0' ) {
    size_t off = &r - this->recipe;
    if ( km.recipe[ (uint8_t) r.char_sequence[ 0 ] ] != (uint8_t) off ) {
      mode &= ~km.mode;
    }
  }
  else {
    size_t i;
    for ( i = 0; i < km.mc_size; i++ ) {
      if ( km.mc[ i ] == &r )
        break;
    }
    if ( i == km.mc_size )
      mode &= ~km.mode;
  }
}

void
State::layout_keys( const char *key,  const char *action,  const char *mode,
                    const char *descr ) noexcept
{
  /* col
   * 0              16                 35     42
   * Key            Action             Mode
   * ---            ------             ----
   * ctrl-a         goto_bol           ICSV   Description
   */
  char32_t line[ 80 ];
  size_t   i, len;
  for ( i = 0; i < 80; i++ )
    line[ i ] = ' ';
  len = ::strlen( key );
  for ( i = 0; i < len; i++ )
    line[ i ] = key[ i ];
  len = ::strlen( action );
  for ( i = 0; i < len; i++ )
    line[ 17 + i ] = action[ i ];
  len = ::strlen( mode );
  for ( i = 0; i < len; i++ )
    line[ 36 + i ] = mode[ i ];
  if ( descr == NULL )
    len += 36;
  else {
    len = ::strlen( descr );
    for ( i = 0; i < len; i++ )
      line[ 42 + i ] = descr[ i ];
    len += 42;
  }
  LineSave::make( this->keys, line, len, 0, ++this->keys.cnt );
}

bool
State::show_keys( void ) noexcept
{
  if ( this->keys.cnt == 0 ) {
    char         name[ 40 ],
                 mode[ 40 ];
    const char * action,
               * descr;
    size_t       k;

    if ( this->keys.first == 0 ) {
      if ( ! this->realloc_lsb( this->keys, LineSave::size( 80 ) *
                                ( this->recipe_size + 2 ) ) )
        return false;
      this->layout_keys( "Key", "Action",
                         "Mode: E:emacs I:vi C:cmd S:Srch V:Visu", NULL );
      this->layout_keys( "---", "------", "----", NULL );
      for ( size_t i = 0; i < this->recipe_size; i++ ) {
        KeyRecipe &r = this->recipe[ i ];
        uint8_t valid_mode = r.valid_mode;
        if ( r.char_sequence == NULL )
          ::strcpy( name, "(other key)" );
        else {
          lc_key_to_name( r.char_sequence, name );
          if ( this->recipe_list.hd != NULL ) {
            if ( ( valid_mode & EMACS_MODE ) != 0 )
              this->filter_mode( this->emacs, valid_mode, r );
            if ( ( valid_mode & VI_INSERT_MODE ) != 0 )
              this->filter_mode( this->vi_insert, valid_mode, r );
            if ( ( valid_mode & VI_COMMAND_MODE ) != 0 )
              this->filter_mode( this->vi_command, valid_mode, r );
            if ( ( valid_mode & SEARCH_MODE ) != 0 )
              this->filter_mode( this->search, valid_mode, r );
            if ( ( valid_mode & VISUAL_MODE ) != 0 )
              this->filter_mode( this->visual, valid_mode, r );
          }
        }
        if ( valid_mode == 0 )
          continue;
        action = lc_action_to_name( r.action );
        descr  = lc_action_to_descr( r.action );
        k = 0;
        mode[ 0 ] = '\0';
        if ( ( valid_mode & EMACS_MODE ) != 0 )
          ::strcpy( &mode[ k++ ], "E" );
        if ( ( valid_mode & VI_INSERT_MODE ) != 0 )
          ::strcpy( &mode[ k++ ], "I" );
        if ( ( valid_mode & VI_COMMAND_MODE ) != 0 )
          ::strcpy( &mode[ k++ ], "C" );
        if ( ( valid_mode & SEARCH_MODE ) != 0 )
          ::strcpy( &mode[ k++ ], "S" );
        if ( ( valid_mode & VISUAL_MODE ) != 0 )
          ::strcpy( &mode[ k++ ], "V" );
        this->layout_keys( name, action, mode, descr );
      }
    }
    this->keys.off = this->keys.first;
    this->show_mode = SHOW_KEYS;
    if ( this->show_keys_start() )
      return true;
  }
  else {
    this->show_pg   = this->keys_pg;
    this->show_mode = SHOW_KEYS;
    return this->show_lsb( SHOW_KEYS, this->keys );
  }
  this->show_mode = SHOW_NONE;
  return false;
}

bool
State::show_keys_start( void ) noexcept
{
  this->show_pg  = this->pgcount( this->keys ) - 1;
  this->keys.off = this->keys.first;
  this->keys_pg  = this->show_pg;
  return this->show_save( 0, 0 );
}

bool
State::show_keys_end( void ) noexcept
{
  this->show_pg = 0;
  this->keys_pg = 0;
  return this->show_lsb( SHOW_KEYS, this->keys );
}

bool
State::show_keys_next_page( void ) noexcept
{
  if ( this->show_pg > 0 )
    this->show_pg--;
  this->keys_pg = this->show_pg;
  return this->show_lsb( SHOW_KEYS, this->keys );
}

bool
State::show_keys_prev_page( void ) noexcept
{
  if ( this->show_pg < this->pgcount( this->keys ) - 1 )
    this->show_pg++;
  this->keys_pg = this->show_pg;
  return this->show_lsb( SHOW_KEYS, this->keys );
}

bool
State::copy_help( LineSaveBuf &lsb ) noexcept
{
  if ( this->help.buf != NULL )
    ::free( this->help.buf );
  ::memcpy( &this->help, &lsb, sizeof( this->help ) );
  lsb.buf    = NULL;
  lsb.buflen = 0;
  LineSave::reset( lsb );
  this->show_mode = SHOW_HELP;
  this->show_pg   = this->pgcount( this->help ) - 1;
  return this->show_lsb( SHOW_HELP, this->help );
}

bool
State::show_help( void ) noexcept
{
  char32_t help_str[ 4 ];
  help_str[ 0 ] = 'h';
  help_str[ 1 ] = '3';
  help_str[ 2 ] = 'l';
  help_str[ 3 ] = 'P';
  if ( this->help.cnt == 0 ) {
    if ( ! this->realloc_lsb( this->help, 1024 ) )
      return false;
    LineSave::make( this->help, help_str, 4, 0, ++this->help.cnt );
  }
  this->show_mode = SHOW_HELP;
  return this->show_lsb( SHOW_HELP, this->help );
}

bool
State::show_help_start( void ) noexcept
{
  this->show_pg  = this->pgcount( this->help ) - 1;
  this->help.off = this->help.first;
  return this->show_save( 0, 0 );
}

bool
State::show_help_end( void ) noexcept
{
  this->show_pg = 0;
  return this->show_lsb( SHOW_HELP, this->help );
}

bool
State::show_help_next_page( void ) noexcept
{
  if ( this->show_pg > 0 )
    this->show_pg--;
  return this->show_lsb( SHOW_HELP, this->help );
}

bool
State::show_help_prev_page( void ) noexcept
{
  if ( this->show_pg < this->pgcount( this->help ) - 1 )
    this->show_pg++;
  return this->show_lsb( SHOW_HELP, this->help );
}

