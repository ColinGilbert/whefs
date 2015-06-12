
See also: [BuildAndInstall](BuildAndInstall.md)

# Requirements/Prerequisites for compiling whefs #

The library is standalone C code conforming to the ISO C99 standard (don't even think about asking me to back-port to C89 - not gonna happen). The storage handler for on-disk EFSes (as opposed to in-memory VFSes) requires certain functions defined in the POSIX-1.2001 standard (e.g. <tt>ftruncate()</tt> and <tt>fileno()</tt>). Most or all Unix-like systems will have the few required POSIX routines. Windows... i don't know. Without those storage handlers it can only be used for in-memory filesystems.

No third-party libraries are needed except the system's standard C libraries and the above-mentioned POSIX-defined routines.

The i/o layer has some optional support for compression using zlib, but zlib has become a core system-level component and is available preinstalled on any sane system. The feature can be activated with a C macro (when building whefs, not the client) - see [whio\_zlib.h](http://code.google.com/p/whefs/source/browse/trunk/include/wh/whio/whio_zlib.h) for details.

Most compilers explicitly require enabling C99 compatibility mode (in gcc this is the <tt>-std=c99</tt> flag, on SunCC it's <tt>-xc99=all</tt>). Note that compilers which don't support C99 variable-sized arrays (e.g. tcc) will need to `malloc()` in some places where other compilers do not, so the overall memory costs may go up.

# Platforms witnessed to work with whefs #

The whefs code has been shown to build, run, and pass basic sanity checks on:

  * [gcc](http://www.gnu.org/software/gcc/) on Linux (2.4.x and 2.6.x) ix86/32: gcc 3.2.3, 4.0.2, 4.3.x
  * [gcc](http://www.gnu.org/software/gcc/) on Linux ix86/64: gcc 4.2.x
  * [tcc](http://bellard.org/tcc/) on ix86/32 Linux: tcc 0.9.24/25 (and tcc is FAST!)
  * gcc on [Nexenta](http://www.nexentaos.org/os) OpenSolaris 2.x in an x86/32 virtual machine.
  * gcc on a Sun V240 (sparcv9) under Solaris 10: gcc 3.4
  * Sun Studio 12 on a Sun V240 (sparcv9) under Solaris 10

That is not to say the code is _supported_ on all of those platforms - my access to non-Linux boxes is very limited, and i cannot test all new code on all of those platforms (especially Sun Solaris). It might just be that a platform which worked yesterday might not work tomorrow (but that's _not_ a project goal!).

Last i checked, the code compiles cleanly with gcc's `-pedantic` mode enabled, which gives me a warm fuzzy feeling regarding its portability.

Reports of success/failure for other platforms are always appreciated. (Simply email me or add a comment to this wiki page.)