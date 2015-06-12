

See also: [RequirementsAndPlatforms](RequirementsAndPlatforms.md), [WhefsAmalgamation](WhefsAmalgamation.md)

# Building and Installing #

## Unixish Platforms ##

The build tree currently requires GNU Make 3.81 or higher. It also assumes a compiler which supports GCC's arguments (with the special exception that it also supports tcc). That said, see [WhefsAmalgamation](WhefsAmalgamation.md) for details on how to create a single standalone source file which can be compiled using an arbitrary build tool and C99 compiler.

To build the tree on Unixish platforms:

```
~> make
# Or, if you want to build using the TinyC compiler:
# ~> make CC=tcc CXX=tcc
```

It takes only 2-3 seconds to build the library and supporting tools using gcc on a reasonably fast machine, or about 12 seconds on a 1.6GHz netbook with a slow hard drive.

**To install it:**

  * Copy `src/libwhefs.*` to a directory in your library search path.
  * Copy the directory `include/wh` to a directory in your INCLUDES search path.
  * Copy `app/whefs-*` to a directory in your executable search path.

Some day i'll add `make install` rules, but the makefile code is intentionally kept overly simple.

## Non-Unixish Platforms ##

This code is 100% untested on non-Unix platforms. In theory, [the amalgamation build](WhefsAmalgamation.md) should compile on any system which provides C99 support and two or three non-C99 (but POSIX-1.2001) functions used for dealing with file-based EFSes.

# Enabling or Disabling Internal Debug Info #

By default the library builds with debug messaging information enabled. This slows down the library a small amount, but is useful for tracking down errors. To build the library without debug message support, run `make DEBUG=0` (for versions newer than 20090620). When building the library using custom tools, simply add `-UNDEBUG` to the preprocessor flags to enable the debug code, or `-DNDEBUG=1` to compile it out.

# Using whefs from client code #

To use libwhefs from client code:

  * Include the appropriate header file(s) from `<wh/whefs/*.h>` and/or `<wh/whio/*.h>`. If you're using the [amalgamation build](WhefsAmalgamation.md) you'll need to include `whefs_amalgamation.h` _instead of_ the other headers.
  * Link against `-lwhefs`.

# Building the API Documentation #

The public API documentation is in the header files under `include/wh/...`. The [doxygen tool](http://www.stack.nl/~dimitri/doxygen/) can generate HTML versions of the docs. If you have doxygen, then try running `make doc` from the `doc` subdirectory of the source tree. If you have built the [WhefsAmalgamation](WhefsAmalgamation.md) then two versions of the API docs will be generated - one with the public API and one with the complete API, including internal/private details.

# Source Tree Overview #

The source tree is intentionally kept very simple in structure, to simplify supporting arbitrary build platforms. It looks a bit like this:

| `<top>/include` |All PUBLIC header files go under here. PRIVATE headers MAY be in here, but are more likely to be in `<top>/src`|
|:----------------|:--------------------------------------------------------------------------------------------------------------|
| `<top>/include/wh` | Headers belonging to the "whXXX" family of code go here.  ("wh" = "WanderingHorse.net", my home domain.)      |
| `<top>/include/wh/whio` | Public headers for the `whio` API (the i/o API on which whefs is built).                                      |
| `<top>/include/wh/whefs` | libwhefs PUBLIC API headers. The private ones are under `<top>/src`.                                          |
| `<top>/src`     | All sources for libwhefs, including the whio layer. whio actually has [its own source repo](http://fossil.wanderinghorse.net/repos/whio), and changes are synched now and then between the whefs and whio. |
| `<top>/app`     | The various [whefs-\* tools](WhefsTools.md) plus some other test code.                                        |
| `<top>/doc`     | Documentation for the library.                                                                                |

## Configuration files ##

The project has several compile-time configurable options in `<wh/whio/whio_config.h>` and `<wh/whefs/whefs_config.h>`. However, in the interest of build tree simplicity those files are manually edited and not automatically generated. Some of the options can be overridden by passing `-Dxxxx` to the compiler, but some of them need to be set directly in the header file, to ensure that the core library and all clients have the same values.

The client-configurable files include:

| `<top>/include/wh/whio/whio_config.h` | Configuration options specific to the i/o layer. |
|:--------------------------------------|:-------------------------------------------------|
| `<top>/include/wh/whefs/whefs_config.h` | Configuration options specific to the EFS layer. |
| `<top>/config.make`                   | Build-time configuration options.                |