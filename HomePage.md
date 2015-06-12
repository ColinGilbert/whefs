

# whefs: the WanderingHorse.net Embedded Filesystem Library for C #

whefs is a Free/Open Source C library implementing an embedded virtual filesystem.
It works by creating a "filesystem" inside a so-called container file (or in memory), called an EFS. This API can then treat that container similarly to a filesystem. In essence this is similar to conventional archives (e.g. zip files), except that this library provides random-access read/write support to the filesystem via an API similar to the conventional fopen(), fread(), fwrite() and friends.

<b>Author</b>: Stephan Beal (http://wanderinghorse.net/home/stephan/)

<b>License</b>: The code is released in the Public Domain except in jurisdictions which do not recognize Public Domain property, in which case it is released under the MIT license (which is about as close as Public Domain as a license can get). The MIT license is compatible for use with GPL'd and commercial software.

**Achtung:** there are no plans to make major changes/additions to whefs, aside from fixing significant bugs as they are reported. The "next generation" of whefs, [whio\_epfs](http://fossil.wanderinghorse.net/repos/whio/index.cgi/wiki/whio_epfs), is being developed. It currently (20110419) has all the features, plus some, of whefs, and gets rid of some of whefs' more notable limitations. Future coding efforts are going into that code base instead of this one (and they have been since early 2010). That said, at some point i hope to host whefs2 on this site, basically a thin wrapper around [whio\_epfs](http://fossil.wanderinghorse.net/repos/whio/index.cgi/wiki/whio_epfs). See [WhioEPFS](WhioEPFS.md) for more details.


# Features #

  * Provides features to create and open embedded/virtual filesystems, which is basically a filesystem which lives inside of a single file (or in memory), and to access "pseudofiles" within those filesystems.
  * i'm a documentation maniac - whefs comes with over 150k of API documentation and another 100k+ of docs in this site's wiki.
  * The i/o support is provided by [libwhio](http://fossil.wanderinghorse.net/repos/whio/), meaning it can in principal use a wide range of back-end storage devices. Implementations are provided for FILE handles, in-memory buffers, and mapping a user-supplied memory range as storage. See [Whefs2c](Whefs2c.md).
  * Provides two different approaches for I/O on "pseudofiles" inside an EFS. The lower-level API treats pseudofiles as random-access device objects. The higher-level API closely resembles the standard C file APIs (e.g. fopen(), fread()/read(), fwrite()/write(), etc.). The approaches are not mutually exclusive, and any given pseudofile can be accessed via either of the APIs.
  * The EFS [file format](WhefsFilesystem.md) is independent of the device or platform bitness/endianness.
  * Optimized for low memory consumption: small use cases can get away with less than 2kb of malloc()'d memory, and "normal" cases need less than 10k (as measured by Valgrind). If configured for absolute minimal memory consumption (see [WhefsTweakingEFS](WhefsTweakingEFS.md)) then small use cases (1 EFS and 2 opened pseudofiles) can get away with less than 1kb of `malloc()`d memory (448 bytes is the smallest i've seen, 352 of which came from `fopen()` and could not be avoided).
  * The source tree comes with [several tools for working with EFSes](WhefsTools.md), e.g. for creating EFSes, listing their contents, and copying files into and out of an EFS.
  * Supports read-only as well as read-write operation, at the EFS and pseudofile levels.
  * Has very liberal, non-viral licensing conditions, and is unhindered by licensing restrictions (or warranties, for that matter!). Googling has revealed little open-source work on embedded filesystems (but lots of commercial products), and i have been unable to find a comparable library released under non-restrictive (or non-viral) licensing terms.
  * Designed to be easy to copy directly into a client source tree. See [the amalgamation page](WhefsAmalgamation.md) for details.
  * Fairly rigorous consistency and bounds checking - it bails out of it finds the slightest hint of foul play. (Just remember to check the error codes! They're there for a reason!)
  * It gets run through [Valgrind](http://www.valgrind.org/) often to ensure that it doesn't leak memory.

# Misfeatures #

  * Some parts of the public API need fleshing out.
  * Very little support for concurrency - see [WhefsConcurrency](WhefsConcurrency.md) for details.
  * Optimized for ease of use/maintenance and memory consumption, not speed. Typical use requires only a few KB of dynamically allocated memory (for the minimal caching it does). Despite the minimalism, however, it is surprisingly performant.
  * Does not currently support directory hierarchies. See [issue #3](https://code.google.com/p/whefs/issues/detail?id=#3) for more thoughts on that.
  * Doesn't abstract enough away to support standard filesystems (e.g. VFAT) inside the container file. It uses its own custom filesystem implementation (my very first attempt at such) which may be suitable for small use cases but will certainly not scale well (performance-wise) into the thousands of files range.
  * A signal, crash, propagated C++ exception, or similar interruption while the FS is writing or holding unflushed file information can leave the pseudofile contents in an inconsistent state.
  * Until the software stabilizes, the file format may change from version to version. It is however always possible to export the data from an old EFS (using an older version of whefs) and re-import it using a newer version.

# Requirements and Platforms #

See [RequirementsAndPlatforms](RequirementsAndPlatforms.md).

# Current status #

"It works for me!"

Let's call it "Very Beta." It "seems to work", but the nature of the problem means there is lots of room for errors and bugs. Do not make the mistake of using it for data which you can't afford to lose.

That said,the basics are in place and are believed to work fairly well. There are no known data loss/corruption cases except potentially crashes or C signals during write operations.

**As of August 2011**, whefs is now officially superseded/obsoleted by [whio\_epfs](http://whiki.wanderinghorse.net/wikis/whio/?page=whio_epfs), which can do everything whefs can do (plus some), does it more efficiently (in terms of memory and speed), and includes a C++ wrapper which can simplify usage significantly. whefs _is still maintained_, and bug fixes which can fit within the current architecture will be made where feasible (and reported by users), but no significant additions/changes are planned. For a brief introduction to EPFS, see the JavaScript bindings: http://code.google.com/p/v8-juice/wiki/V8Convert_Whio

# Significant TODOs #

See [ToDos](ToDos.md) for the list of more pressing TODOs.

# News #

See [the news page](WhefsNews.md).

# Want to Help? #

whefs has some notable limitations and could be improved in some significant ways. Any feedback or assistance is always appreciated. You can reach me via http://wanderinghorse.net/home/stephan/. Anyone who shows an interest and submits a patch or two will gladly be given write access to the code repository.

Some areas of improvement which specifically come to mind (and in which i could definitely use a hand) are:

  * Concurrency. See [WhefsConcurrency](WhefsConcurrency.md) for details.
  * Caching/Performance. See [WhefsPerformance](WhefsPerformance.md) for details.
  * Implementing a B-tree or AVL tree on top of the whio\_dev interface for doing searches for pseudofiles by name. The current approach works but isn't very scalable.