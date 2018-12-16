# Readme for Linecook

[![copr status](https://copr.fedorainfracloud.org/coprs/injinj/gold/package/linecook/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/injinj/gold/package/linecook/)

1. [What is Linecook?](#what-is-linecook-)
2. [Why does Linecook exist?](#why-does-linecook-exist-)
3. [How do you build/install Linecook?](#how-do-you-build-install-linecook-)
4. [What editing commands exist in Linecook?](#what-editing-commands-exist-in-linecook-)

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

2. Bob Steagall for [UTF-32](https://githup.com/BobSteagall/CppCon2018).  Easy
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
$ sudo apt-get install make g++ gcc chrpath libpcre2-dev devscripts
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

$ sudp dpkg -i dpkgbuild/linecook_1.1.0-14_amd64.deb
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

