# Readme for Linecook

[![Copr build status](https://copr.fedorainfracloud.org/coprs/injinj/rel/package/linecook/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/injinj/rel/package/linecook/)

1. [What is Linecook?](#what-is-linecook)
2. [Why does Linecook exist?](#why-does-linecook-exist)
3. [How do you build/install Linecook?](#how-do-you-build-install-linecook)
4. [What editing commands exist in Linecook?](#what-editing-commands-exist-in-linecook)

## What is Linecook?

Linecook is a C library for editing the command line, much like the
[readline](https://tiswww.cwru.edu/php/chet/readline/readline.html) library
used in <b>bash</b> and <b>gdb</b>.

An example of a Read Eval Print Loop (REPL) using Linecook looks like
[this](test/simple.c):

```c
printf( "simple echo test, type 'q' to quit\n" );
for (;;) {
  int n = lc_tty_get_line( tty ); /* retry line and run timed events */
  if ( n == 0 )
    n = lc_tty_poll_wait( tty, 60000 ); /* wait at most 60 seconds */
  if ( n < 0 ) /* if error in one of the above */
    break;
  if ( tty->lc_status == LINE_STATUS_EXEC ) { /* if there is one available */
    lc_tty_normal_mode( tty );     /* set terminal to normal mode */
    printf( "[%s]\n", tty->line ); /* which translates \n to \r\n */
    if ( tty->line_len == 1 && tty->line[ 0 ] == 'q' ) /* type 'q' to quit */
      break;
    lc_tty_log_history( tty );
  }
}
```

### Credits, inspiration

Some people and projects that inspired this library:

1. Salvatore Sanfilippo for [linenoise](https://github.com/antirez/linenoise).
   I started with this library.  I believe it is not possible, 20,000 lines are
   necessary :).

2. Bob Steagall for [UTF-32](https://github.com/BobSteagall/CppCon2018).  Easy
   to follow, problem solving with SSE (even though I didn't use that part).

3. Joshua Rubin for [wcwidth9](https://github.com/joshuarubin/wcwidth9).  He
   pointed me in the correct direction, double width chars was not something I
   thought about.

4. The awesome [Fish shell](https://github.com/fish-shell).  I think it sets
   the bar for command line shell integration.

5. The ubiquitous [Bash/readline](https://www.gnu.org/software/bash/).  It is
   the reason that we have command line editing in everything.

6. The [es-shell](https://github.com/wryun/es-shell).  I am using this shell at
   the moment, with Linecook integration, in
   [desh](https://github.com/injinj/desh).  This still needs some work and
   baking.

## Why does Linecook exist?

  There are many programs that come with command line editing and some
  libraries which implement these features, but I could not find one that fit
  my needs.

1.  Asynchronous.  Network ready.

  Linecook does not presume input comes from a terminal.  My plan is to use it
as a backend for a web based console using [xterm.js](https://xtermjs.org/).
The only example of something similar is [DomTerm](http://domterm.org/) (AFAIK).

2.  Vi Mode input.

  Most of the libraries I found were oriented towards completion and hinting
with a <b>Emacs</b> style editing base.  Examples of these are
[Linenoise](https://github.com/antirez/linenoise),
[Linenoise NG](https://github.com/arangodb/linenoise-ng),
[replxx](https://github.com/AmokHuginnsson/replxx).
These are all fine libraries with nice features.  I also tried the line
editing within these shells
[zsh](http://www.zsh.org/),
[fish](https://github.com/fish-shell),
[elvish](https://github.com/elves/elvish).
They all have very nice features as well as Vi mode (in Zsh and Fish cases),
although the line editing within shells tends to be integrated tightly with the
shell language... completions call out to shell scripts, etc.

3.  BSD License.

  Readline is GPL licensed rather than LGPL.  This means anything linked with
must also be released with a GPL license.  This is the reason that
[rlwrap](https://github.com/hanslub42/rlwrap) exists.

## What platforms does Linecook support?

Linux.  With <b>C11</b> for Unicode <b>char32_t</b> support (from 2012 --
[glibc 2.16](https://sourceware.org/ml/libc-alpha/2012-06/msg00807.html))

## What features does Linecook have?

### Linecook aims to be a <i>readline</i> replacement.

I am a lifelong user of the <b>bash</b> shell mainly because of
<b>readline</b>.  I have also used <b>readline</b> in previous projects. It has
a large feature set and it is highly configurable.  Most of the <b>readline</b>
editing commands are supported in Linecook, but it is not API compatible.
Whenever I was adding an feature I would always try it in <b>bash</b> first, to
see how it works there.  The history operations is slightly different, but most
other things operate as the would in <b>readline</b>.  Most of my experience
within <b>bash</b> is in <b>Vi</b> mode using <b>set -o vi</b>. Many of the
<b>Emacs</b> commands I learned as I coded them.  I have only used <b>emacs</b>
from time to time, and use <b>vim</b> nearly daily.

### Visual Select editing

The Visual Select method of editing text in <b>vim</b> exists in Linecook as
'ctrl-v' in <b>Emacs</b> or <b>Vi Insert</b> mode, and 'v' in <b>Vi Command</b>
mode.  This is the <b>vim</b> feature that I miss most within <b>readline</b>.
The <b>fish</b> shell also has this now.

### Combined Vi and Emacs modes (EVIL mode)

Unlike <b>readline</b> and like <b>Emacs EVIL</b> mode (I presume), most all of
<b>Emacs</b> mode is available within <b>Vi Insert</b> mode.  The only
difference between <b>Emacs</b> and <b>Vi</b> mode is that the 'esc' key can be
used as a 'meta-' (or 'alt-') substitute, whereas in <b>Vi</b> mode, that
toggles between <b>Vi Insert</b> and <b>Vi Command</b>.  Still, <b>Emacs</b>
mode can be useful when the window manager or terminal multiplexer steals
'alt-' keycodes for it's own use.  The default mode in Linecook is <b>Vi
Insert</b> mode and 'meta-m' switches between <b>Vi</b> mode and <b>Emacs</b>
mode (and 'meta-' is really emacs notation for 'alt-', there was a time
keyboards actually had a [meta key](https://en.wikipedia.org/wiki/Meta_key)).

### History Searching

The history is searched as in <b>readline</b> <b>Vi</b> mode, with '/' and '?'
in <b>Vi Command</b> mode.  The 'ctrl-r' and 'ctrl-s' which normally map to
<b>Emacs</b> mode "reverse-i-search" and "i-search", map to '/' and '?' in
Linecook instead.  The "reverse-i-search" method of history search does not
currently exist.  That may change in the future, since the "i-search" does have
completion properties that '/' does not.  There is no builtin handling of
history expansion, ex:  '!!', '!-1', '!cp:^'.  This is also on the todo list.

### TAB Completions

The method for TAB completions is also different than <b>readline</b>.  Unlike
<b>readline</b> Linecook allows completion selection like <b>fish</b> and
<b>zsh</b>.  Completion selection uses the TAB key to cycle through
alternatives and movement (ctrl-n, ctrl-p, arrow keys) to move a selection
cursor over the desired completion string.

### UTF-32 Unicode

Internally, Unicode is supported as UTF-32, so each character in memory is a 32
bit character, using the type <b>char32_t</b>.  Externally, the characters are
converted to UTF-8.  Any I/O that occurs causes UTF-32 to UTF-8 translation.
Double-wide characters, such as Han or Kanji characters, and Emoji symbols are
supported.  The code that determines whether a character is double wide is
commonly derived by Linecook and many other applications, from
[wcwidth.c](https://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c) by Markus Kuhn.

### ANSI Terminal Management

All screen movements that manage the layout of screen elements are using this
minimal set of [ANSI](https://en.wikipedia.org/wiki/ANSI_escape_code) control
codes:

1. EL (Erase Line)
   <b>ESC [ n K</b> -- erase to EOL (n=0), erase to BOL (n=1), erase line (n=2)

2. CUF (Cursor Foward)
   <b>ESC [ n C</b> -- move forward 'n' characters.

3. CUB (Cursor Backward)
   <b>ESC [ n D</b> -- move backward 'n' characters.

4. CUU (Cursor Up)
   <b>ESC [ n A</b> -- move cursor up 'n' characters.

5. CUD (Cursor Down)
   <b>ESC [ n B</b> -- move cursor down 'n' characters.

Color control sequences are parsed and passed through to the terminal.  Nearly
every terminal shares these control codes, going back to VT100.  Most terminals
within a modern Linux distribution support Color control sequences these days
and newer Font libraries such as [Nerd
Fonts](https://github.com/ryanoasis/nerd-fonts) also support Emoji codes.

## How do you build/install Linecook?

Install these and type <b>make</b>.

```console
$ sudo dnf install make gcc-c++ chrpath pcre2-devel rpm-build
```

Or

```console
$ sudo apt-get install make g++ gcc chrpath libpcre2-dev devscripts debhelper
```

The build directory is created and populated by the build in
OS_rel_cpu/{bin,lib64,obj,dep}.  If you wish to install it, there is an
'dist_rpm' target for RedHat like systems.  This requires the
<b>rpm-build</b> package to be installed.

```console
$ make dist_rpm

# Install

$ sudo rpm -i rpmbuild/RPMS/x86_64/linecook-1.1.0-14.fc27.x86_64.rpm
```

For Debian style, do this instead.

```console
$ make dist_dpkg

$ sudo dpkg -i dpkgbuild/linecook_1.1.0-14_amd64.deb
```

For rpms, the prefix option works, as in --prefix=/opt/rai

```console
$ sudo rpm --prefix=/opt/rai -i rpmbuild/RPMS/x86_64/linecook-1.0.0-1.fc27.x86_64.rpm
$ rpm -ql linecook
/opt/rai/bin/lc_example
/opt/rai/include/linecook
/opt/rai/include/linecook/kewb_utf.h
/opt/rai/include/linecook/keycook.h
/opt/rai/include/linecook/linecook.h
/opt/rai/include/linecook/ttycook.h
/opt/rai/include/linecook/xwcwidth9.h
/opt/rai/lib/.build-id
/opt/rai/lib/.build-id/4f
/opt/rai/lib/.build-id/4f/1519b3809a3afdfbdeceb9badceb3e3620afcb
/opt/rai/lib/.build-id/62
/opt/rai/lib/.build-id/62/20397653a6db5da601b7920be7b395ba8860d7
/opt/rai/lib64/liblinecook.a
/opt/rai/lib64/liblinecook.so
/opt/rai/lib64/liblinecook.so.1.1
/opt/rai/lib64/liblinecook.so.1.1.0-14
```

Optionally install debuginfo and debugsource for gdb.

```console
$ sudo rpm -i rpmbuild/RPMS/x86_64/linecook-debugsource-1.0.0-1.fc27.x86_64.rpm
$ sudo rpm -i rpmbuild/RPMS/x86_64/linecook-debuginfo-1.0.0-1.fc27.x86_64.rpm
```

Removing the packages from the system.

```console
$ sudo rpm -e linecook linecook-debugsource linecook-debuginfo
```

## What editing commands exist in Linecook?

This list is generated by 'print_keys' in the build.  It shows the builtin
mapping of keystroke sequence to commands that is compiled into the library.
This is a table, but it is not currently file loadable as would be necessary
for runtime configuration (todo).

There are 5 modal states which have keybindings.  These are:

1. <b>E</b> = <b>Emacs</b> mode.

  This mode is the same as <b>Vi Insert</b> mode, except that 'esc' is a 'meta-'
prefix rather than a switch between <b>Vi Insert</b> and <b>Vi Command</b> mode.

2. <b>I</b> = <b>Vi Insert</b> mode.

  Text is inserted or replaced as chars are entered.  Most all of the control
keys and meta keys have functions that map to (mostly) <b>Emacs</b> commands.

3. <b>C</b> = <b>Vi Command</b> mode.

  All keys are <b>Vi</b> commands.  Movement, Editing, Yanking, Pasting, etc.

4. <b>S</b> = <b>History Search</b> mode.

  The search text is manipulated.

5. <b>V</b> = <b>Visual Select</b> mode.

  The movement keys are used to control the area that surrounds the text which
  is yanked or deleted.

This is table that is loaded at startup in [keycook.cpp](src/keycook.cpp).

| Key | Action | Mode | Description |
| --- | ------ | ---- | ----------- |
| esc | vi_esc | EICSV | Switch vi ins mode to cmd mode |
| ctrl-a | goto_bol | EICSV | Goto beginning of line |
| ctrl-b | go_left | EICSV | Goto char before cursor |
| ctrl-c | interrupt | EICSV | Cancel line and reset state |
| ctrl-d | delete_char | EIS | Delete char under cursor |
| ctrl-e | goto_eol | EICSV | Goto end of line |
| ctrl-f | go_right | EICSV | Goto char after cursor |
| ctrl-g | show_clear | EICSV | Clear the show buffer |
| ctrl-h | backspace | EIS | Delete char before cursor |
| backspace | backspace | EIS | Delete char before cursor |
| tab | tab_complete | EIC | Use TAB completion at cursor |
| shift-tab | tab_reverse | EIC | Use TAB for previous completion |
| ctrl-j | finish_line | EICSV | Execute line and reset state |
| ctrl-k | erase_eol | EIS | Erase from cursor to line end |
| ctrl-l | redraw_line | EICSV | Refresh prompt and line |
| enter | finish_line | EICSV | Execute line and reset state |
| ctrl-n | next_item | EIC | Goto next item in history |
| ctrl-o | oper_and_next | EICSV | Execute completion for line |
| ctrl-p | prev_item | EIC | Goto previous item in history |
| ctrl-q | quoted_insert | EICS | Insert key typed (eg. ctrl char) |
| ctrl-r | search_history | EIS | Search history from end |
| ctrl-r | redo | C | Redo edit that was undone |
| ctrl-s | search_reverse | EIS | Search history from start |
| ctrl-t | transpose | EIS | Transpose two chars at cursor |
| ctrl-u | erase_bol | EIS | Erase from cursor to line start |
| ctrl-v | visual_mark | EICSV | Toggle visual select mode |
| ctrl-w | erase_prev_word | EIS | Erase word before cursor |
| ctrl-x backspace | erase_bol | EIS | Erase from cursor to line start |
| ctrl-x ctrl-r | redo | EIS | Redo edit that was undone |
| ctrl-x ctrl-u | undo | EIS | Undo last edit |
| ctrl-y | paste_insert | EIS | Paste insert the yank buffer |
| ctrl-z | suspend | EICSV | Stop editing are refresh |
| ctrl-] (k) | find_next_char | EICSV | Scan fwd in line for char match |
| meta-ctrl-] (k) | find_prev_char | EICSV | Scan back in line for char match |
| ctrl-_ | undo | EIS | Undo last edit |
| (other key) | insert_char | EIS | Insert character |
| meta-a | goto_top | EICS | Goto top of current page |
| meta-b | goto_prev_word | EICSV | Goto word before cursor (emacs) |
| meta-c | capitalize_word | EICS | Capitalize word at cursor |
| meta-d | erase_next_word | EICS | Erase word after cursor (emacs) |
| meta-f | goto_next_word | EICSV | Goto word after cursor (emacs) |
| meta-h | erase_prev_word | EICS | Erase word before cursor |
| meta-backspace | erase_prev_word | EICS | Erase word before cursor |
| meta-j | show_next_page | EICS | Page down, show buf or history |
| meta-k | show_prev_page | EICS | Page up, show buf or history |
| meta-l | lowercase_word | EICS | Lowercase word at cursor |
| meta-m | vi_mode | EIC | Toggle vi and emacs mode |
| meta-p | history_complete | EIC | Search hist using word at cursor |
| meta-s | search_inline | EIC | Insert history search at cursor |
| meta-r | repeat_last | EICS | Repeat last operation |
| meta-t | transpose_words | EICS | Transpose two words at cursor |
| meta-u | uppercase_word | EICS | Uppercase word at cursor |
| meta-y | yank_pop | EICS | Yank next item in yank stack |
| meta-z | goto_bottom | EICS | Goto bottom of current page |
| meta-. | yank_last_arg | EICS | Yank last argument |
| meta-_ | yank_last_arg | EICS | Yank last argument |
| meta-ctrl-y | yank_nth_arg | EICS | Yank Nth argument |
| meta-/ | show_tree | EIC | Search dir tree using substring |
| arrow-left | go_left | EICSV | Goto char before cursor |
| arrow-right | go_right | EICSV | Goto char after cursor |
| arrow-left-vt | go_left | EICSV | Goto char before cursor |
| arrow-right-vt | go_right | EICSV | Goto char after cursor |
| ctrl-left | goto_bol | EICSV | Goto beginning of line |
| ctrl-right | goto_eol | EICSV | Goto end of line |
| meta-left | goto_prev_word | EICSV | Goto word before cursor (emacs) |
| meta-right | goto_next_word | EICSV | Goto word after cursor (emacs) |
| arrow-up | prev_item | EIC | Goto previous item in history |
| arrow-down | next_item | EIC | Goto next item in history |
| arrow-up-vt | prev_item | EIC | Goto previous item in history |
| arrow-down-vt | next_item | EIC | Goto next item in history |
| insert | replace_mode | EIS | Toggle insert and replace mode |
| delete | delete_char | EICS | Delete char under cursor |
| home | goto_bol | EICSV | Goto beginning of line |
| end | goto_eol | EICSV | Goto end of line |
| meta-< | goto_first_entry | EICS | Goto 1st entry of show / history |
| meta-> | goto_last_entry | EICS | Goto last entry of show / history |
| page-up | show_prev_page | EICS | Page up, show buf or history |
| page-down | show_next_page | EICS | Page down, show buf or history |
| meta-( | decr_show | EICSV | Decrement show size |
| meta-) | incr_show | EICSV | Increment show size |
| a | vi_append | C | Go to vi insert mode after cursor |
| A | vi_append_eol | C | Go to vi insert mode at line end |
| b | vi_goto_prev_word | CV | Goto word before cursor (vi) |
| c 0 | vi_change_bol | C | Change from cursor to line start |
| c c | vi_change_line | C | Change all of line |
| c w | vi_change_word | C | Change word under cursor |
| c $ | vi_change_eol | C | Change from cursor to line end |
| C | vi_change_line | C | Change all of line |
| d 0 | erase_bol | C | Erase from cursor to line start |
| d d | erase_line | C | Erase all of line |
| d w | vi_erase_next_word | C | Erase word after cursor (vi) |
| d $ | erase_eol | C | Erase from cursor to line end |
| D | erase_eol | C | Erase from cursor to line end |
| e | goto_endof_word | CV | Goto the end of word at cursor |
| f (k) | find_next_char | CV | Scan fwd in line for char match |
| F (k) | find_prev_char | CV | Scan back in line for char match |
| G | goto_entry | C | Goto entry of number entered |
| h | go_left | CV | Goto char before cursor |
| H | goto_top | C | Goto top of current page |
| i | vi_insert | C | Go to vi insert mode at cursor |
| I | vi_insert_bol | C | Go to vi ins mode at line start |
| j | next_item | C | Goto next item in history |
| k | prev_item | C | Goto previous item in history |
| space | go_right | CV | Goto char after cursor |
| l | go_right | CV | Goto char after cursor |
| L | goto_bottom | C | Goto bottom of current page |
| m (k) | vi_mark | C | Mark the cursor position |
| M | goto_middle | C | Goto middle of current page |
| n | search_next | C | Goto next history search match |
| N | search_next_rev | C | Goto prev history search match |
| p | paste_append | C | Paste append the yank buffer |
| P | paste_insert | C | Paste insert the yank buffer |
| R | vi_replace | C | Go to vi ins mode with replace |
| r (k) | vi_replace_char | C | Replace char under cursor |
| s | vi_change_char | C | Change char under cursor |
| S | vi_change_line | C | Change all of line |
| t (k) | till_next_char | CV | Scan fwd in line for char match |
| T (k) | till_prev_char | CV | Scan back in line for char match |
| u | undo | C | Undo last edit |
| U | revert | C | Revert line to original state |
| v | visual_mark | CV | Toggle visual select mode |
| w | vi_goto_next_word | CV | Goto word after cursor (vi) |
| x | delete_char | C | Delete char under cursor |
| X | backspace | C | Delete char before cursor |
| y 0 | yank_bol | C | Yank from cursor to line start |
| y y | yank_line | C | Yank all of line |
| y w | yank_word | C | Yank word at cursor |
| y $ | yank_eol | C | Yank from cursor to line end |
| Y | yank_eol | C | Yank from cursor to line end |
| $ | goto_eol | CV | Goto end of line |
| 0 | goto_bol | CV | Goto beginning of line |
| ^ | goto_bol | CV | Goto beginning of line |
| ~ | flip_case | C | Toggle case of letter at cursor |
| . | repeat_last | C | Repeat last operation |
| ; | repeat_find_next | CV | Repeat scan forward char match |
| , | repeat_find_prev | CV | Repeat scan backward char match |
| % | match_paren | CV | Match the paren at cursor |
| / | search_history | C | Search history from end |
| ? | search_reverse | C | Search history from start |
| ` (k) | vi_goto_mark | C | Goto a marked position |
| ' (k) | vi_goto_mark | C | Goto a marked position |
| = | tab_complete | C | Use TAB completion at cursor |
| + | incr_number | C | Increment number at cursor |
| ctrl-h | go_left | CV | Goto char before cursor |
| - | decr_number | C | Decrement number at cursor |
| backspace | go_left | CV | Goto char before cursor |
| insert | vi_insert | C | Go to vi insert mode at cursor |
| 1 | vi_repeat_digit | C | Add to repeat counter |
| 2 | vi_repeat_digit | C | Add to repeat counter |
| 3 | vi_repeat_digit | C | Add to repeat counter |
| 4 | vi_repeat_digit | C | Add to repeat counter |
| 5 | vi_repeat_digit | C | Add to repeat counter |
| 6 | vi_repeat_digit | C | Add to repeat counter |
| 7 | vi_repeat_digit | C | Add to repeat counter |
| 8 | vi_repeat_digit | C | Add to repeat counter |
| 9 | vi_repeat_digit | C | Add to repeat counter |
| meta-0 | emacs_arg_digit | EICSV | Add to emacs arg counter |
| meta-1 | emacs_arg_digit | EICSV | Add to emacs arg counter |
| meta-2 | emacs_arg_digit | EICSV | Add to emacs arg counter |
| meta-3 | emacs_arg_digit | EICSV | Add to emacs arg counter |
| meta-4 | emacs_arg_digit | EICSV | Add to emacs arg counter |
| meta-5 | emacs_arg_digit | EICSV | Add to emacs arg counter |
| meta-6 | emacs_arg_digit | EICSV | Add to emacs arg counter |
| meta-7 | emacs_arg_digit | EICSV | Add to emacs arg counter |
| meta-8 | emacs_arg_digit | EICSV | Add to emacs arg counter |
| meta-9 | emacs_arg_digit | EICSV | Add to emacs arg counter |
| meta-- | emacs_arg_digit | EICSV | Add to emacs arg counter |
| (other key) | bell | C | Ring bell (show bell) |
| c | visual_change | V | Change visual selection |
| s | visual_change | V | Change visual selection |
| ctrl-d | visual_delete | V | Delete visual selection |
| d | visual_delete | V | Delete visual selection |
| x | visual_delete | V | Delete visual selection |
| y | visual_yank | V | Yank visual selection |
| (other key) | visual_mark | V | Toggle visual select mode |
| f1 | show_keys | EICSV | Show the key bindings |
| f2 | show_history | EICS | Show the history lines |
| f3 | show_files | EIC | Use TAB completion w/files |
| f4 | show_tree | EIC | Search dir tree using substring |
| f5 | show_exes | EIC | Use TAB completion w/exes |
| f6 | show_dirs | EIC | Use TAB completion w/dirs |
| f7 | show_undo | EICSV | Show the undo lines |
| f8 | show_yank | EICSV | Show the yank lines |
| f9 | show_fzf | EIC | Search dir tree using fzf |
| f10 | show_help | EICSV | Show help option on cmd |
| f11 | show_man | EICSV | Show man page of cmd |
| f12 | show_clear | EICSV | Clear the show buffer |
| meta-ctrl-d | show_dirs | EIC | Use TAB completion w/dirs |
| meta-ctrl-e | show_exes | EIC | Use TAB completion w/exes |
| meta-ctrl-f | show_files | EIC | Use TAB completion w/files |
| meta-ctrl-h | show_history | EICS | Show the history lines |
| meta-ctrl-k | show_keys | EICSV | Show the key bindings |
| meta-ctrl-l | show_clear | EICSV | Clear the show buffer |
| meta-enter | show_man | EICSV | Show man page of cmd |
| meta-ctrl-n | show_help | EICSV | Show help option on cmd |
| meta-ctrl-p | show_yank | EICSV | Show the yank lines |
| meta-ctrl-u | show_undo | EICSV | Show the undo lines |
| meta-ctrl-v | show_vars | EIC | Use TAB completion w/env vars |
| ctrl-x ctrl-a (k) | action | EIC | General purpose action |
