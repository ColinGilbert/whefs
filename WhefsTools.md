

# whefs Tools Overview #

The source tree comes with several tools (under `<top_srcdir>/app`) for working with whefs container files:

  * [whefs-mkfs](WhefsMkfs.md) creates EFS container files.
  * [whefs-ls](WhefsLs.md) lists the contents of a EFS container.
  * [whefs-cp](WhefsCp.md) imports files into or exports them from an EFS.
  * [whefs-rm](WhefsRm.md) deletes files from a EFS.
  * [whefs-addblocks](WhefsAddblocks.md) adds blocks to an existing EFS.
  * [whefs-cat](WhefsCat.md) exports files from an EFS to standard output.
  * [whefs2c](Whefs2c.md) converts an EFS file to C code and creates a statically-linked in-memory EFS for it.

Some of those tools rely on private API routines which most client code won't have access to. Over time the public API will be fleshed out, such that client-side tools will have the level of access they would need to implement features like those these tools provide.

The source code for all of these tools id in the [app directory of the source tree](http://code.google.com/p/whefs/source/browse/trunk/app/).

# Common command arguments #

Most of the whefs tools support the following common arguments:

  * <tt>-?</tt> or <tt>--help</tt> = Show app help and quit.
  * <tt>-v</tt> or <tt>--verbose</tt>= Enable verbose mode
  * <tt>-V</tt> or <tt>--version</tt> = Show version information and quit.
  * `-D<string>` or `--debug-flags=<string>` = Set to a string value describing the whefs debug categories which should be used. See below for more detail.

Most of these tools also require that an EFS file be given as the first non-flag argument on the command line. "Non-flag" means an argument which does not start with '-' or '--'.

## Debugging flags ##

If whefs is built in debug mode (i.e. with the optional debuggering information turned on - see [BuildAndInstall](BuildAndInstall.md) for more details), the apps can use the <tt>-D</tt> or <tt>--debug-flags</tt> parameter to enable certain debugging messages. The arguments are used like <tt>-Dabc</tt> or <tt>--debug-flags=abc</tt>, where <tt>abc</tt> is one or more of the following letters (case-sensitive), each of which turns on a specific category of message:

  * <tt>'a'</tt> = All messages.
  * <tt>'c'</tt> = Caching messages.
  * <tt>'d'</tt> = Default log level.
  * <tt>'e'</tt> = Error messages.
  * <tt>'f'</tt> = FIXME messages.
  * <tt>'h'</tt> = Hacker-level messages (turns on several other options).
  * <tt>'l'</tt> = Locking messages.
  * <tt>'n'</tt> = NYI (Not Yet Implemented) messages.
  * <tt>'w'</tt> = Warning messages.

If whefs was built without debugging information then the debug flags are ignored.