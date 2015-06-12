
# Tweaking an EFS #

(See also: [WhefsPerformance](WhefsPerformance.md) [WhefsFilesystem](WhefsFilesystem.md))

whefs offers several different ways to tweak a filesystem's characteristics. Some are compile-time options (for whefs, not the client) and some are configurable by client code.

# Client-configurable Options #

The options which can be configured by the client are described by the struct <tt>whefs_fs_options</tt>, which is shown in its entirety below. An options object is used when creating a filesystem. The <tt>whefs_id_type</tt> type is a configurable unsigned integer type explained later on.

The options described below correspond to the various command-line options for [WhefsMkfs](WhefsMkfs.md).

```
/** @struct whefs_fs_options

   whefs_fs_options defines the major parameters of a vfs. Once they
   are set, the must not be changed for the life of the vfs, as doing
   so invalidates the vfs and will lead to data corruption.

   Normally this type is used by clients only when creating a new vfs
   container file (e.g. via whefs_mkfs()). When opening an existing
   vfs container, the options will be read in from the container.
*/
struct whefs_fs_options
{
    /**
       The magic cookie of the vfs. This is used to verify that the file
       is an EFS.        
    */
    whefs_magic magic;
    /**
       The size of each data block in the vfs.
    */
    uint32_t block_size;
    /**
       The number of blocks in the VFS. The implementation
       requires that this number be at least as larger as
       inode_count. Future versions may allow this value to
       grow during the operation of the VFS.
    */
    whefs_id_type block_count;
    /**
       Number of "inodes" in the vfs. That is, the maximum number
       of files or directories. Each non-0-byte file/directory also
       takes up at least one data block.
    */
    whefs_id_type inode_count;
    /**
       Must be greater than 0 and less than or equal to
       WHEFS_MAX_FILENAME_LENGTH.
    */
    uint16_t filename_length;
};
```

# Compile-time options #

whefs has a few compile-time options which can be tweaked. Some can be passed to the compiler via <tt>-Dxxxx=yyy</tt>, but others require editing a line of the whefs source code (probably in <tt><a href='http://code.google.com/p/whefs/source/browse/trunk/include/wh/whefs/whefs_config.h'>whefs_config.h</a></tt>). The options include...

## Debugging Mode ##

If compiled without `NDEBUG` set to any value then the library builds in support for reporting certain debugging messages, to assist in tracking down errors. If `NDEBUG` is defined when the library is compiled, that support is compiled out. Having it enabled is a speed sink, and it should be disabled if it is not particularly needed.

Be aware that building with debugging extensions enabled might also enable some `assert()` calls which would otherwise trigger error codes to be returned.

Also be aware that this has _nothing_ to do with the compiler's debugging mode flags. This affects only whether certain code gets compiled in or not, and not the level of debugging information available to an external debugger.

Enabling or disabling the debugging support does not change binary compatibility for clients dynamically linked to libwhefs, with the _possible_ exception of some of the [whefs-\* tools](WhefsTools.md) which blithely use certain inner secrets of the library.

## <tt>whefs_id_type</tt> and <tt>WHEFS_ID_TYPE_BITS</tt> ##

In order to squeeze out a few more bytes of memory, the maximum number of inodes and blocks in a VFS can be restricted to an unsigned integer type with 8, 16, 32, or 64 bits. For reasons explained fully in the API docs, the default is 16. Rather than repeat those docs, they are pasted in below (the most current version is in [whefs\_config.h](http://code.google.com/p/whefs/source/browse/trunk/include/wh/whefs/whefs_config.h)):

```

/** @def WHEFS_ID_TYPE_BITS

WHEFS_ID_TYPE_BITS tells us how many bits to use for inode and
data block IDs. The supported values are 8, 16, 32, or 64, and libraries
compiled with different values will not be compatible (nor can
they read the others' format, though i'm looking into ways to
be able to enable that).

The number of bits here is significant because:

- We are limited to (2^WHEFS_ID_TYPE_BITS-2) inodes and blocks
(but for all practical purposes, this isn't a limitation even with
16 bits).

- The layout of the filesystem is dependent on the number of bits in
this type, as the number of bytes used for certain data fields changes.

- When/if inode or block caching is added, the cache size will incrase
by (WHEFS_ID_TYPE_BITS/8) for each ID field stored in the cache
(that's a minimum of 2 IDs per object, and maybe up to 6 per inode
once directory support is added). The number grows quickly.

In practice, a 16-bit ID type is completely sufficient, but the
library was originally created with size_t as the identifier type, so
it supports 32-bit IDs as well. The switch to 16 as the default was
made to help save memory when/if inode caching gets added to the
library. In any case, the filesystem's minimalistic implementation is
not scalable to tens of thousands of files, and so 16 bits is a very
realistic limit.

In theory, 64-bit IDs are also okay, but (A) for this particular use
case that is way overkill, (B) it's a huge waste of memory (8 bytes
where we realistically don't need more than 2), and (C) the filesystem
model itself is not scalable to that level of use (e.g. millions of
files). inode/block IDs are always sequential, starting at 1, and by
the time we hit that number of blocks or inodes, the computer's memory
would almost certainly be filled.

i would strongly prefer to have WHEFS_ID_TYPE_BITS as an enum constant
instead of a macro value, but we unfortunately need some conditional
compilation based on the bit count.

If this constant is changed, all whefs client code which depends on
it must be recompiled and all filesystems written using the old value
will not be readable. That is the reason it is not set up with an
ifndef guard, so clients cannot blithely change it. If you are copying
whefs directly into another project, feel free to change the value
all you want, but be aware of the compatibility ramifications. Doing so
may also screw up any printf() (or similar) commands which have a hard-coded
format specifier which no longer matches after changing this value.
*/
#define WHEFS_ID_TYPE_BITS 16
```

Depending on the value of WHEFS\_ID\_TYPE\_BITS, the type whefs\_id\_type is defined differently. Here's a shortened version of what can be found in [whefs.h](http://code.google.com/p/whefs/source/browse/trunk/include/wh/whefs/whefs.h):

```
#if WHEFS_ID_TYPE_BITS == 8
    typedef uint8_t whefs_id_type;
#elif WHEFS_ID_TYPE_BITS == 16
    typedef uint16_t whefs_id_type;
#elif WHEFS_ID_TYPE_BITS == 32
    typedef uint32_t whefs_id_type;
#elif WHEFS_ID_TYPE_BITS == 64
    typedef uint64_t whefs_id_type;
#else
#  error "WHEFS_ID_TYPE_BITS must be one of: 8, 16, 32, 64"
#endif
```

We use integer types with fixed standard sizes, instead of types like size\_t (which can differ in size). Not doing so has been known to Cause Grief in unexpected places when the size of a type changes (e.g. size\_t can be 32 or 64 bits).

## <tt>WHEFS_MAX_FILENAME_LENGTH</tt> ##

WHEFS\_MAX\_FILENAME\_LENGTH is a constant number (an enum value) which is explained fully in the API docs:

```
/**
   WHEFS_MAX_FILENAME_LENGTH defines the hard maximum string length
   for filenames in a vfs, not including the null terminator. This
   currently includes the names of any parent directories.

   whefs_fs_options::filename_length must not be greater than this
   number.

   This value plays a big part in the in-memory size of a vfs inode,
   and should be kept to a reasonable length. While 1024 sounds
   reasonable, setting it that high will drastically increase the
   memory costs of certain operations.
 */
WHEFS_MAX_FILENAME_LENGTH = 128,
```

The relatively low value of WHEFS\_MAX\_FILENAME\_LENGTH is largely historic in nature. A value of 512 or 1024 is being considered, and may be necessary as long as whefs does not support directories.

This value is used quite often in the library for allocating string buffers on the stack (which can be used to avoid a `malloc()` in many cases), so it should not be set to a huge value.

## WHIO\_CONFIG\_ENABLE\_STATIC\_MALLOC and WHEFS\_CONFIG\_ENABLE\_STATIC\_MALLOC ##

These macros are defined in [whio\_config.h](http://code.google.com/p/whefs/source/browse/trunk/include/wh/whio/whio_config.h) and [whefs\_config.h](http://code.google.com/p/whefs/source/browse/trunk/include/wh/whefs/whefs_config.h), respectively, but can be set by the client at library compile time.

If set to a true value they enable "static allocators" for many of the library types. The static allocators are routines which "allocate" objects from a pre-initialized list in static memory, falling back to malloc() if the list fills up. These allocators are not strictly thread safe (the deallocators are, curiously enough), but neither is whefs, so unless you plan on using the whio code in other threads of your app for purposes other than whefs (or want to run multiple EFSes in multiple threads), there's little reason not to turn it on. It will increase the size of the overall binary by a relatively small amount, but with this option turned on the vast majority of the calls to malloc() go away.

The allocators are believed to perform fairly well. Allocation is O(1) until the list overflows, at which point it will fall back to malloc() and inherit malloc()'s performance characteristics. As items in the list are freed, which may fragment the list, the allocators run between O(1) (best case) and O(N) (worst case), where N is one less than the number of items in the list for that specific allocator (typically 3-10 items). The deallocation routines are always O(1) unless the object they are freeing came from malloc() (because the pool filled up), in which case they have the same performance as free().

Here are some results of dynamic memory allocation made by the library
under a couple different test cases, using different values for these
configuration options:

| **`WHIO_CONFIG_ENABLE_STATIC_MALLOC`** | **`WHEFS_CONFIG_ENABLE_STATIC_MALLOC`** | **`malloc()` calls** | **Total bytes `malloc()`ed** |
|:---------------------------------------|:----------------------------------------|:---------------------|:-----------------------------|
<tr><td><b><a href='WhefsLs.md'>whefs-ls</a> on a small EFS:</b></td></tr>
| 0                                      | 0                                       | 18                   | 1055                         |
| 1                                      | 1                                       | 7                    | 651                          |
| 0                                      | 1                                       | 17                   | 843                          |
| 1                                      | 0                                       | 8                    | 863                          |
<tr><td><b>Opening and closing a total of 100 pseudofile objects, in groups of 10:</b></td></tr>
| 0                                      | 0                                       | 616                  | 18120                        |
| 1                                      | 1                                       | 165                  | 6276                         |
| 0                                      | 1                                       | 275                  | 8468                         |
| 1                                      | 0                                       | 506                  | 15928                        |

(Stats provided by [Valgrind](http://www.valgrind.org).)

The bytes of memory reported allocated is the _total_ number which was allocated by the library over the life of the test application, and not the maximum amount which was allocated at any given time (that value is much lower, but unfortunately [Valgrind](http://www.valgrind.org) cannot seem to report that value).