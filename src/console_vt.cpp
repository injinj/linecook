#if defined( _MSC_VER ) || defined( __MINGW32__ )

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>
#include <io.h>
#include <linecook/console_vt.h>

using namespace linecook;
/* derived from: */
/* https://android.googlesource.com/platform/system/adb/sysdeps_win32.cpp */
/**************************************************************************/
/**************************************************************************/
/*****                                                                *****/
/*****      Console Window Terminal Emulation                         *****/
/*****                                                                *****/
/**************************************************************************/
/**************************************************************************/
// This reads input from a Win32 console window and translates it into Unix
// terminal-style sequences. This emulates mostly Gnome Terminal (in Normal
// mode, not Application mode), which itself emulates xterm. Gnome Terminal
// is emulated instead of xterm because it is probably more popular than xterm:
// Ubuntu's default Ctrl-Alt-T shortcut opens Gnome Terminal, Gnome Terminal
// supports modern fonts, etc. It seems best to emulate the terminal that most
// Android developers use because they'll fix apps (the shell, etc.) to keep
// working with that terminal's emulation.
//
// The point of this emulation is not to be perfect or to solve all issues with
// console windows on Windows, but to be better than the original code which
// just called read() (which called ReadFile(), which called ReadConsoleA())
// which did not support Ctrl-C, tab completion, shell input line editing
// keys, server echo, and more.

bool
ConsoleVT::init_vt( int in_fd,  int out_fd ) noexcept
{
  if ( ! _isatty( in_fd ) || ! _isatty( out_fd ) ) {
    fprintf( stderr, "Not a tty\n" );
    return false;
  }

  const intptr_t inptr_handle  = _get_osfhandle( in_fd ),
                 outptr_handle = _get_osfhandle( out_fd );
  if ( inptr_handle == -1 || outptr_handle == -1 ) {
    fprintf( stderr, "_get_osfhandle() failed: %ld\n", GetLastError() );
    return false;
  }

  this->in_h  = reinterpret_cast<HANDLE>( inptr_handle );
  this->out_h = reinterpret_cast<HANDLE>( outptr_handle );

  if ( ! GetConsoleMode( this->in_h, &this->orig_in_mode ) ||
       ! GetConsoleMode( this->out_h, &this->orig_out_mode ) ) {
    fprintf( stderr, "GetConsoleMode failed: %ld\n", GetLastError() );
    return false;
  }
  return true;
}

bool
ConsoleVT::enable_raw_mode( void ) noexcept
{
  DWORD new_console_mode =
    ( this->orig_in_mode &
      ~( ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT ) ) |
       ( ENABLE_WINDOW_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT );

  if ( ! SetConsoleMode( this->in_h, new_console_mode ) ) {
    // This really should not fail.
    fprintf( stderr, "set_raw_mode: SetConsoleMode( in_h ) failed: %ld",
             GetLastError() );
    return false;
  }
  new_console_mode =
    ( this->orig_out_mode &
      ~( ENABLE_WRAP_AT_EOL_OUTPUT ) ) |
       ( DISABLE_NEWLINE_AUTO_RETURN | ENABLE_PROCESSED_OUTPUT |
         ENABLE_VIRTUAL_TERMINAL_PROCESSING );

  if ( ! SetConsoleMode( this->out_h, new_console_mode ) ) {
    // This really should not fail.
    fprintf( stderr, "set_raw_mode: SetConsoleMode( out_h ) failed: %ld",
             GetLastError() );
    return false;
  }
  return true;
}

bool
ConsoleVT::disable_raw_mode( void ) noexcept
{
  if ( ! SetConsoleMode( this->in_h, this->orig_in_mode ) ||
       ! SetConsoleMode( this->out_h, this->orig_out_mode ) ) {
    fprintf( stderr, "disable_raw_mode failed: %ld\n", GetLastError() );
    return false;
  }
  return true;
}

bool
ConsoleVT::poll_events( int time_ms ) noexcept
{
  DWORD nevents = 0;
  this->events_avail = 0;

  if ( WaitForSingleObject( this->in_h, time_ms ) != WAIT_OBJECT_0 )
    return false;

  if ( ! GetNumberOfConsoleInputEvents( this->in_h, &nevents ) )
    return false;

  if ( nevents > 0 ) {
    this->events_avail = nevents;
    return true;
  }
  return false;
}

static inline bool
_is_shift_pressed( const DWORD control_key_state )
{
  return ( control_key_state & SHIFT_PRESSED ) != 0;
}
static inline bool
_is_ctrl_pressed( const DWORD control_key_state )
{
  return ( control_key_state & ( LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED ) ) !=
         0;
}
static inline bool
_is_alt_pressed( const DWORD control_key_state )
{
  return ( control_key_state & ( LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED ) ) != 0;
}
static inline bool
_is_numlock_on( const DWORD control_key_state )
{
  return ( control_key_state & NUMLOCK_ON ) != 0;
}
static inline bool
_is_capslock_on( const DWORD control_key_state )
{
  return ( control_key_state & CAPSLOCK_ON ) != 0;
}
static inline bool
_is_enhanced_key( const DWORD control_key_state )
{
  return ( control_key_state & ENHANCED_KEY ) != 0;
}
// Constants from MSDN for ToAscii().
static const BYTE TOASCII_KEY_OFF        = 0x00;
static const BYTE TOASCII_KEY_DOWN       = 0x80;
static const BYTE TOASCII_KEY_TOGGLED_ON = 0x01; // for CapsLock
// Given a key event, ignore a modifier key and return the character that was
// entered without the modifier. Writes to *ch and returns the number of bytes
// written.
static size_t
_get_char_ignoring_modifier( char* const              ch,
                             const KEY_EVENT_RECORD & key_event,
                             const DWORD              control_key_state,
                             const WORD               modifier )
{
  // If there is no character from Windows, try ignoring the specified
  // modifier and look for a character. Note that if AltGr is being used,
  // there will be a character from Windows.
  if ( key_event.uChar.AsciiChar == '\0' ) {
    // Note that we read the control key state from the passed in argument
    // instead of from key_event since the argument has been normalized.
    if ( ( ( modifier == VK_SHIFT ) &&
           _is_shift_pressed( control_key_state ) ) ||
         ( ( modifier == VK_CONTROL ) &&
           _is_ctrl_pressed( control_key_state ) ) ||
         ( ( modifier == VK_MENU ) && _is_alt_pressed( control_key_state ) ) ) {
      BYTE key_state[ 256 ] = { 0 };
      key_state[ VK_SHIFT ] = _is_shift_pressed( control_key_state )
                                ? TOASCII_KEY_DOWN
                                : TOASCII_KEY_OFF;
      key_state[ VK_CONTROL ] = _is_ctrl_pressed( control_key_state )
                                  ? TOASCII_KEY_DOWN
                                  : TOASCII_KEY_OFF;
      key_state[ VK_MENU ] = _is_alt_pressed( control_key_state )
                               ? TOASCII_KEY_DOWN
                               : TOASCII_KEY_OFF;
      key_state[ VK_CAPITAL ] = _is_capslock_on( control_key_state )
                                  ? TOASCII_KEY_TOGGLED_ON
                                  : TOASCII_KEY_OFF;
      // cause this modifier to be ignored
      key_state[ modifier ] = TOASCII_KEY_OFF;
      WORD translated       = 0;
      if ( ToAscii( key_event.wVirtualKeyCode, key_event.wVirtualScanCode,
                    key_state, &translated, 0 ) == 1 ) {
        // Ignoring the modifier, we found a character.
        *ch = (CHAR) translated;
        return 1;
      }
    }
  }
  // Just use whatever Windows told us originally.
  *ch = key_event.uChar.AsciiChar;
  // If the character from Windows is NULL, return a size of zero.
  return ( *ch == '\0' ) ? 0 : 1;
}
// If a Ctrl key is pressed, lookup the character, ignoring the Ctrl key,
// but taking into account the shift key. This is because for a sequence like
// Ctrl-Alt-0, we want to find the character '0' and for Ctrl-Alt-Shift-0,
// we want to find the character ')'.
//
// Note that Windows doesn't seem to pass bKeyDown for Ctrl-Shift-NoAlt-0
// because it is the default key-sequence to switch the input language.
// This is configurable in the Region and Language control panel.
static inline size_t
_get_non_control_char( char* const ch, const KEY_EVENT_RECORD &key_event,
                       const DWORD control_key_state )
{
  return _get_char_ignoring_modifier( ch, key_event, control_key_state,
                                      VK_CONTROL );
}
// Get without Alt.
static inline size_t
_get_non_alt_char( char* const ch, const KEY_EVENT_RECORD &key_event,
                   const DWORD control_key_state )
{
  return _get_char_ignoring_modifier( ch, key_event, control_key_state,
                                      VK_MENU );
}
// Ignore the control key, find the character from Windows, and apply any
// Control key mappings (for example, Ctrl-2 is a NULL character). Writes to
// *pch and returns number of bytes written.
static size_t
_get_control_character( char* const             pch,
                        const KEY_EVENT_RECORD &key_event,
                        const DWORD             control_key_state )
{
  const size_t len = _get_non_control_char( pch, key_event, control_key_state );
  if ( ( len == 1 ) && _is_ctrl_pressed( control_key_state ) ) {
    char ch = *pch;
    switch ( ch ) {
      case '2':
      case '@':
      case '`': ch = '\0'; break;
      case '3':
      case '[':
      case '{': ch = '\x1b'; break;
      case '4':
      case '\\':
      case '|': ch = '\x1c'; break;
      case '5':
      case ']':
      case '}': ch = '\x1d'; break;
      case '6':
      case '^':
      case '~': ch = '\x1e'; break;
      case '7':
      case '-':
      case '_': ch = '\x1f'; break;
      case '8': ch = '\x7f'; break;
      case '/':
        if ( !_is_alt_pressed( control_key_state ) ) {
          ch = '\x1f';
        }
        break;
      case '?':
        if ( !_is_alt_pressed( control_key_state ) ) {
          ch = '\x7f';
        }
        break;
    }
    *pch = ch;
  }
  return len;
}
static DWORD
_normalize_altgr_control_key_state( const KEY_EVENT_RECORD &key_event )
{
  DWORD control_key_state = key_event.dwControlKeyState;
  // If we're in an AltGr situation where the AltGr key is down (depending on
  // the keyboard layout, that might be the physical right alt key which
  // produces a control_key_state where Right-Alt and Left-Ctrl are down) or
  // AltGr-equivalent keys are down (any Ctrl key + any Alt key), and we have
  // a character (which indicates that there was an AltGr mapping), then act
  // as if alt and control are not really down for the purposes of modifiers.
  // This makes it so that if the user with, say, a German keyboard layout
  // presses AltGr-] (which we see as Right-Alt + Left-Ctrl + key), we just
  // output the key and we don't see the Alt and Ctrl keys.
  if ( _is_ctrl_pressed( control_key_state ) &&
       _is_alt_pressed( control_key_state ) &&
       ( key_event.uChar.AsciiChar != '\0' ) ) {
    // Try to remove as few bits as possible to improve our chances of
    // detecting combinations like Left-Alt + AltGr, Right-Ctrl + AltGr, or
    // Left-Alt + Right-Ctrl + AltGr.
    if ( ( control_key_state & RIGHT_ALT_PRESSED ) != 0 ) {
      // Remove Right-Alt.
      control_key_state &= ~RIGHT_ALT_PRESSED;
      // If uChar is set, a Ctrl key is pressed, and Right-Alt is
      // pressed, Left-Ctrl is almost always set, except if the user
      // presses Right-Ctrl, then AltGr (in that specific order) for
      // whatever reason. At any rate, make sure the bit is not set.
      control_key_state &= ~LEFT_CTRL_PRESSED;
    }
    else if ( ( control_key_state & LEFT_ALT_PRESSED ) != 0 ) {
      // Remove Left-Alt.
      control_key_state &= ~LEFT_ALT_PRESSED;
      // Whichever Ctrl key is down, remove it from the state. We only
      // remove one key, to improve our chances of detecting the
      // corner-case of Left-Ctrl + Left-Alt + Right-Ctrl.
      if ( ( control_key_state & LEFT_CTRL_PRESSED ) != 0 ) {
        // Remove Left-Ctrl.
        control_key_state &= ~LEFT_CTRL_PRESSED;
      }
      else if ( ( control_key_state & RIGHT_CTRL_PRESSED ) != 0 ) {
        // Remove Right-Ctrl.
        control_key_state &= ~RIGHT_CTRL_PRESSED;
      }
    }
    // Note that this logic isn't 100% perfect because Windows doesn't
    // allow us to detect all combinations because a physical AltGr key
    // press shows up as two bits, plus some combinations are ambiguous
    // about what is actually physically pressed.
  }
  return control_key_state;
}
// If NumLock is on and Shift is pressed, SHIFT_PRESSED is not set in
// dwControlKeyState for the following keypad keys: period, 0-9. If we detect
// this scenario, set the SHIFT_PRESSED bit so we can add modifiers
// appropriately.
static DWORD
_normalize_keypad_control_key_state( const WORD  vk,
                                     const DWORD control_key_state )
{
  if ( !_is_numlock_on( control_key_state ) ) {
    return control_key_state;
  }
  if ( !_is_enhanced_key( control_key_state ) ) {
    switch ( vk ) {
      case VK_INSERT: // 0
      case VK_DELETE: // .
      case VK_END:    // 1
      case VK_DOWN:   // 2
      case VK_NEXT:   // 3
      case VK_LEFT:   // 4
      case VK_CLEAR:  // 5
      case VK_RIGHT:  // 6
      case VK_HOME:   // 7
      case VK_UP:     // 8
      case VK_PRIOR:  // 9
        return control_key_state | SHIFT_PRESSED;
    }
  }
  return control_key_state;
}
static const char*
_get_keypad_sequence( const DWORD control_key_state, const char* const normal,
                      const char* const shifted )
{
  if ( _is_shift_pressed( control_key_state ) ) {
    // Shift is pressed and NumLock is off
    return shifted;
  }
  else {
    // Shift is not pressed and NumLock is off, or,
    // Shift is pressed and NumLock is on, in which case we want the
    // NumLock and Shift to neutralize each other, thus, we want the normal
    // sequence.
    return normal;
  }
  // If Shift is not pressed and NumLock is on, a different virtual key code
  // is returned by Windows, which can be taken care of by a different case
  // statement in _console_read().
}
// Write sequence to buf and return the number of bytes written.
static size_t
_get_modifier_sequence( char* const buf, const WORD vk, DWORD control_key_state,
                        const char* const normal )
{
  // Copy the base sequence into buf.
  const size_t len = strlen( normal );
  memcpy( buf, normal, len );
  int code = 0;
  control_key_state =
    _normalize_keypad_control_key_state( vk, control_key_state );
  if ( _is_shift_pressed( control_key_state ) ) {
    code |= 0x1;
  }
  if ( _is_alt_pressed( control_key_state ) ) { // any alt key pressed
    code |= 0x2;
  }
  if ( _is_ctrl_pressed( control_key_state ) ) { // any control key pressed
    code |= 0x4;
  }
  // If some modifier was held down, then we need to insert the modifier code
  if ( code != 0 ) {
    if ( len == 0 ) {
      // Should be impossible because caller should pass a string of
      // non-zero length.
      return 0;
    }
    size_t     index    = len - 1;
    const char lastChar = buf[ index ];
    if ( lastChar != '~' ) {
      buf[ index++ ] = '1';
    }
    buf[ index++ ] = ';'; // modifier separator
    // 2 = shift, 3 = alt, 4 = shift & alt, 5 = control,
    // 6 = shift & control, 7 = alt & control, 8 = shift & alt & control
    buf[ index++ ] = '1' + code;
    buf[ index++ ] = lastChar; // move ~ (or other last char) to the end
    return index;
  }
  return len;
}
// Write sequence to buf and return the number of bytes written.
static size_t
_get_modifier_keypad_sequence( char* const buf, const WORD vk,
                               const DWORD       control_key_state,
                               const char* const normal, const char shifted )
{
  if ( _is_shift_pressed( control_key_state ) ) {
    // Shift is pressed and NumLock is off
    if ( shifted != '\0' ) {
      buf[ 0 ] = shifted;
      return sizeof( buf[ 0 ] );
    }
    else {
      return 0;
    }
  }
  else {
    // Shift is not pressed and NumLock is off, or,
    // Shift is pressed and NumLock is on, in which case we want the
    // NumLock and Shift to neutralize each other, thus, we want the normal
    // sequence.
    return _get_modifier_sequence( buf, vk, control_key_state, normal );
  }
  // If Shift is not pressed and NumLock is on, a different virtual key code
  // is returned by Windows, which can be taken care of by a different case
  // statement in _console_read().
}
// The decimal key on the keypad produces a '.' for U.S. English and a ',' for
// Standard German. Figure this out at runtime so we know what to output for
// Shift-VK_DELETE.
static char
_get_decimal_char()
{
  return (char) MapVirtualKeyA( VK_DECIMAL, MAPVK_VK_TO_CHAR );
}
// Prefix the len bytes in buf with the escape character, and then return the
// new buffer length.
size_t
_escape_prefix( char* const buf, const size_t len )
{
  // If nothing to prefix, don't do anything. We might be called with
  // len == 0, if alt was held down with a dead key which produced nothing.
  if ( len == 0 ) {
    return 0;
  }
  memmove( &buf[ 1 ], buf, len );
  buf[ 0 ] = '\x1b';
  return len + 1;
}

bool
ConsoleVT::read_input( void ) noexcept
{
  size_t input_sz;
  if ( this->events_avail == 0 ) {
    DWORD nevents = 0;
    if ( ! GetNumberOfConsoleInputEvents( this->in_h, &nevents ) ||
         nevents == 0 )
      return false;
    this->events_avail = nevents;
  }
  input_sz = sizeof( INPUT_RECORD ) * this->events_avail;
  DWORD read_count = 0;
  if ( this->events_avail > this->events_len ) {
    this->input_events = (INPUT_RECORD *)
      ::realloc( this->input_events, input_sz );
    this->events_len   = this->events_avail;
  }
  ::memset( this->input_events, 0, input_sz );

  this->input_avail = 0;
  this->input_off   = 0;
  DWORD avail = (DWORD) this->events_avail;
  if ( avail == 0 )
    return false;

  this->events_avail = 0;
  if ( ! ReadConsoleInput( this->in_h, this->input_events, avail,
                           &read_count ) )
    return false;

  for ( DWORD ev_num = 0; ev_num < read_count; ev_num++ ) {
    INPUT_RECORD &input_record = this->input_events[ ev_num ];
    if ( input_record.EventType == WINDOW_BUFFER_SIZE_EVENT ) {
      this->sigwinch = true;
      continue;
    }
    if ( input_record.EventType != KEY_EVENT ||
         ! input_record.Event.KeyEvent.bKeyDown ||
         input_record.Event.KeyEvent.wRepeatCount == 0 )
      continue;

    const KEY_EVENT_RECORD & key_event = input_record.Event.KeyEvent;
    const WORD               vk        = key_event.wVirtualKeyCode;
    const CHAR               ch        = key_event.uChar.AsciiChar;
    const DWORD              control_key_state =
      _normalize_altgr_control_key_state( key_event );
    // The following emulation code should write the output sequence to
    // either seqstr or to seqbuf and seqbuflen.
    const char* seqstr = NULL; // NULL terminated C-string
    // Enough space for max sequence string below, plus modifiers and/or
    // escape prefix.
    char   seqbuf[ 16 ];
    size_t seqbuflen = 0; // Space used in seqbuf.
#define MATCH( vk, normal )                                                    \
  case ( vk ): {                                                               \
    seqstr = ( normal );                                                       \
  } break;
    // Modifier keys should affect the output sequence.
#define MATCH_MODIFIER( vk, normal )                                           \
  case ( vk ): {                                                               \
    seqbuflen =                                                                \
      _get_modifier_sequence( seqbuf, ( vk ), control_key_state, ( normal ) ); \
  } break;
    // The shift key should affect the output sequence.
#define MATCH_KEYPAD( vk, normal, shifted )                                    \
  case ( vk ): {                                                               \
    seqstr =                                                                   \
      _get_keypad_sequence( control_key_state, ( normal ), ( shifted ) );      \
  } break;
    // The shift key and other modifier keys should affect the output
    // sequence.
#define MATCH_MODIFIER_KEYPAD( vk, normal, shifted )                           \
  case ( vk ): {                                                               \
    seqbuflen = _get_modifier_keypad_sequence(                                 \
      seqbuf, ( vk ), control_key_state, ( normal ), ( shifted ) );            \
  } break;
#define ESC "\x1b"
#define CSI ESC "["
#define SS3 ESC "O"
    // Only support normal mode, not application mode.
    // Enhanced keys:
    // * 6-pack: insert, delete, home, end, page up, page down
    // * cursor keys: up, down, right, left
    // * keypad: divide, enter
    // * Undocumented: VK_PAUSE (Ctrl-NumLock), VK_SNAPSHOT,
    //   VK_CANCEL (Ctrl-Pause/Break), VK_NUMLOCK
    if ( _is_enhanced_key( control_key_state ) ) {
      switch ( vk ) {
        case VK_RETURN: // Enter key on keypad
          if ( _is_ctrl_pressed( control_key_state ) ) {
            seqstr = "\n";
          }
          else {
            seqstr = "\r";
          }
          break;
          MATCH_MODIFIER( VK_PRIOR, CSI "5~" ); // Page Up
          MATCH_MODIFIER( VK_NEXT, CSI "6~" );  // Page Down
          // gnome-terminal currently sends SS3 "F" and SS3 "H", but that
          // will be fixed soon to match xterm which sends CSI "F" and
          // CSI "H". https://bugzilla.redhat.com/show_bug.cgi?id=1119764
          MATCH( VK_END, CSI "F" );
          MATCH( VK_HOME, CSI "H" );
          MATCH_MODIFIER( VK_LEFT, CSI "D" );
          MATCH_MODIFIER( VK_UP, CSI "A" );
          MATCH_MODIFIER( VK_RIGHT, CSI "C" );
          MATCH_MODIFIER( VK_DOWN, CSI "B" );
          MATCH_MODIFIER( VK_INSERT, CSI "2~" );
          MATCH_MODIFIER( VK_DELETE, CSI "3~" );
          MATCH( VK_DIVIDE, "/" );
      }
    }
    else { // Non-enhanced keys:
      switch ( vk ) {
        case VK_BACK: // backspace
          if ( _is_alt_pressed( control_key_state ) ) {
            seqstr = ESC "\x7f";
          }
          else {
            seqstr = "\x7f";
          }
          break;
        case VK_TAB:
          if ( _is_shift_pressed( control_key_state ) ) {
            seqstr = CSI "Z";
          }
          else {
            seqstr = "\t";
          }
          break;
          // Number 5 key in keypad when NumLock is off, or if NumLock is
          // on and Shift is down.
          MATCH_KEYPAD( VK_CLEAR, CSI "E", "5" );
        case VK_RETURN: // Enter key on main keyboard
          if ( _is_alt_pressed( control_key_state ) ) {
            seqstr = ESC "\n";
          }
          else if ( _is_ctrl_pressed( control_key_state ) ) {
            seqstr = "\n";
          }
          else {
            seqstr = "\r";
          }
          break;
        // VK_ESCAPE: Don't do any special handling. The OS uses many
        // of the sequences with Escape and many of the remaining
        // sequences don't produce bKeyDown messages, only !bKeyDown
        // for whatever reason.
        case VK_SPACE:
          if ( _is_alt_pressed( control_key_state ) ) {
            seqstr = ESC " ";
          }
          else if ( _is_ctrl_pressed( control_key_state ) ) {
            seqbuf[ 0 ] = '\0'; // NULL char
            seqbuflen   = 1;
          }
          else {
            seqstr = " ";
          }
          break;
          MATCH_MODIFIER_KEYPAD( VK_PRIOR, CSI "5~", '9' ); // Page Up
          MATCH_MODIFIER_KEYPAD( VK_NEXT, CSI "6~", '3' );  // Page Down
          MATCH_KEYPAD( VK_END, CSI "4~", "1" );
          MATCH_KEYPAD( VK_HOME, CSI "1~", "7" );
          MATCH_MODIFIER_KEYPAD( VK_LEFT, CSI "D", '4' );
          MATCH_MODIFIER_KEYPAD( VK_UP, CSI "A", '8' );
          MATCH_MODIFIER_KEYPAD( VK_RIGHT, CSI "C", '6' );
          MATCH_MODIFIER_KEYPAD( VK_DOWN, CSI "B", '2' );
          MATCH_MODIFIER_KEYPAD( VK_INSERT, CSI "2~", '0' );
          MATCH_MODIFIER_KEYPAD( VK_DELETE, CSI "3~", _get_decimal_char() );
        case 0x30:          // 0
        case 0x31:          // 1
        case 0x39:          // 9
        case VK_OEM_1:      // ;:
        case VK_OEM_PLUS:   // =+
        case VK_OEM_COMMA:  // ,<
        case VK_OEM_PERIOD: // .>
        case VK_OEM_7:      // '"
        case VK_OEM_102:    // depends on keyboard, could be <> or \|
        case VK_OEM_2:      // /?
        case VK_OEM_3:      // `~
        case VK_OEM_4:      // [{
        case VK_OEM_5:      // \|
        case VK_OEM_6:      // ]}
        {
          seqbuflen =
            _get_control_character( seqbuf, key_event, control_key_state );
          if ( _is_alt_pressed( control_key_state ) ) {
            seqbuflen = _escape_prefix( seqbuf, seqbuflen );
          }
        } break;
        case 0x32:         // 2
        case 0x33:         // 3
        case 0x34:         // 4
        case 0x35:         // 5
        case 0x36:         // 6
        case 0x37:         // 7
        case 0x38:         // 8
        case VK_OEM_MINUS: // -_
        {
          seqbuflen =
            _get_control_character( seqbuf, key_event, control_key_state );
          // If Alt is pressed and it isn't Ctrl-Alt-ShiftUp, then
          // prefix with escape.
          if ( _is_alt_pressed( control_key_state ) &&
               !( _is_ctrl_pressed( control_key_state ) &&
                  !_is_shift_pressed( control_key_state ) ) ) {
            seqbuflen = _escape_prefix( seqbuf, seqbuflen );
          }
        } break; /* a -> z */
        case 0x41: case 0x42: case 0x43: case 0x44:
        case 0x45: case 0x46: case 0x47: case 0x48:
        case 0x49: case 0x4a: case 0x4b: case 0x4c:
        case 0x4d: case 0x4e: case 0x4f: case 0x50:
        case 0x51: case 0x52: case 0x53: case 0x54:
        case 0x55: case 0x56: case 0x57: case 0x58:
        case 0x59: case 0x5a:
        {
          seqbuflen = _get_non_alt_char( seqbuf, key_event, control_key_state );
          // If Alt is pressed, then prefix with escape.
          if ( _is_alt_pressed( control_key_state ) ) {
            seqbuflen = _escape_prefix( seqbuf, seqbuflen );
          }
        } break;
          // These virtual key codes are generated by the keys on the
          // keypad *when NumLock is on* and *Shift is up*.
          MATCH( VK_NUMPAD0, "0" );
          MATCH( VK_NUMPAD1, "1" );
          MATCH( VK_NUMPAD2, "2" );
          MATCH( VK_NUMPAD3, "3" );
          MATCH( VK_NUMPAD4, "4" );
          MATCH( VK_NUMPAD5, "5" );
          MATCH( VK_NUMPAD6, "6" );
          MATCH( VK_NUMPAD7, "7" );
          MATCH( VK_NUMPAD8, "8" );
          MATCH( VK_NUMPAD9, "9" );
          MATCH( VK_MULTIPLY, "*" );
          MATCH( VK_ADD, "+" );
          MATCH( VK_SUBTRACT, "-" );
        // VK_DECIMAL is generated by the . key on the keypad *when
        // NumLock is on* and *Shift is up* and the sequence is not
        // Ctrl-Alt-NoShift-. (which causes Ctrl-Alt-Del and the
        // Windows Security screen to come up).
        case VK_DECIMAL:
          // U.S. English uses '.', Germany German uses ','.
          seqbuflen =
            _get_non_control_char( seqbuf, key_event, control_key_state );
          break;
          MATCH_MODIFIER( VK_F1, SS3 "P" );
          MATCH_MODIFIER( VK_F2, SS3 "Q" );
          MATCH_MODIFIER( VK_F3, SS3 "R" );
          MATCH_MODIFIER( VK_F4, SS3 "S" );
          MATCH_MODIFIER( VK_F5, CSI "15~" );
          MATCH_MODIFIER( VK_F6, CSI "17~" );
          MATCH_MODIFIER( VK_F7, CSI "18~" );
          MATCH_MODIFIER( VK_F8, CSI "19~" );
          MATCH_MODIFIER( VK_F9, CSI "20~" );
          MATCH_MODIFIER( VK_F10, CSI "21~" );
          MATCH_MODIFIER( VK_F11, CSI "23~" );
          MATCH_MODIFIER( VK_F12, CSI "24~" );
          MATCH_MODIFIER( VK_F13, CSI "25~" );
          MATCH_MODIFIER( VK_F14, CSI "26~" );
          MATCH_MODIFIER( VK_F15, CSI "28~" );
          MATCH_MODIFIER( VK_F16, CSI "29~" );
          MATCH_MODIFIER( VK_F17, CSI "31~" );
          MATCH_MODIFIER( VK_F18, CSI "32~" );
          MATCH_MODIFIER( VK_F19, CSI "33~" );
          MATCH_MODIFIER( VK_F20, CSI "34~" );
          // MATCH_MODIFIER(VK_F21, ???);
          // MATCH_MODIFIER(VK_F22, ???);
          // MATCH_MODIFIER(VK_F23, ???);
          // MATCH_MODIFIER(VK_F24, ???);
      }
    }
#undef MATCH
#undef MATCH_MODIFIER
#undef MATCH_KEYPAD
#undef MATCH_MODIFIER_KEYPAD
#undef ESC
#undef CSI
#undef SS3
    const char* out;
    size_t      outlen;
    // Check for output in any of:
    // * seqstr is set (and strlen can be used to determine the length).
    // * seqbuf and seqbuflen are set
    // Fallback to ch from Windows.
    if ( seqstr != NULL ) {
      out    = seqstr;
      outlen = strlen( seqstr );
    }
    else if ( seqbuflen > 0 ) {
      out    = seqbuf;
      outlen = seqbuflen;
    }
    else if ( ch != '\0' ) {
      // Use whatever Windows told us it is.
      seqbuf[ 0 ] = ch;
      seqbuflen   = 1;
      out         = seqbuf;
      outlen      = seqbuflen;
    }
    else {
      // This is a alt or shift keypress, consume the input and 'continue' to
      // cause us to get a new key event.
      continue;
    }
    // put output wRepeatCount times into g_console_buf
    for ( DWORD r = 0; r < key_event.wRepeatCount; r++ ) {
      size_t new_len = this->input_avail + outlen;
      if ( new_len > this->input_len ) {
        this->input_buf =
          (char *) ::realloc( this->input_buf, new_len );
        this->input_len = new_len;
      }
      ::memcpy( &this->input_buf[ this->input_avail ], out, outlen );
      this->input_avail += outlen;
    }
  }
  return this->input_avail > 0;
}

#endif /* _MSC_VER */
