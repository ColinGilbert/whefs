

See also: [Tweaking an EFS](WhefsTweakingEFS.md)


# The whefs Filesystem #

(Be forewarned - whefs represents my very first attempt at modelling a filesystem, and it certainly has aspects which seasoned FS implementors will find unattractive.)

# What is the filesystem? #

A whefs container file is essentially a database holding fixed-length records of three types:

  * <b>I-Nodes</b> are filesystem entries. Each file requires exactly one inode. whefs reserves one inode for internal use as the root node entry. Inodes hold information about a file entry, such as its  logical size (i.e. the number of bytes of data it holds), modification time, and a link to the first data block (if any) associated with the file.
  * <b>Inode names</b> are stored separately from the rest of the inode data, for very boring reasons.
  * <b>Data blocks</b> are where the contents of files are stored. Blocks may be of a near arbitrary size, but all blocks must be the same size. Each block contains the client data and a few bytes of metadata, like a link to the next block in the chain.

Wikipedia has [an informative article about inodes](http://en.wikipedia.org/wiki/Inode) (what they are and why they are named inodes). Be aware that our rather lax use of the term here does not strictly match with all common conceptions of what an inode is, but in the abstract we're all referring to the same basic concept.

In the whefs documentation inodes are often referred to as pseudofiles. In whefs, an inode is, for all intents an purposes, a file entry. We call them pseudofiles to avoid confusion with "real" files (the EFS container file itself, or files outside the EFS). When one opens a pseudofile, the associated file handle is really just a thin wrapper around an i/o device which acts on behalf of a specific inode, redirecting all i/o to and from the underlying data blocks in the EFS.

# File Names #

The EFS does not currently (and may never) support the concept of directories. Instead, it has a flat namespace. Files may have arbitrary names, e.g. `/my/file.foo`, as long as they do not exceed the maximum filename length specified by the EFS (see below for more information on that).

# The filesystem layout #

(Achtung: the filesystem layout is subject to change as the library evolves. This document may not always 100% reflect the current state.)

An EFS container file essentially looks like this:

```
(core magic bytes)       Helps us confirm that the file really is an EFS.
-> (client magic bytes)  Reserved for potential future client-side use.
-> (bookkeeping bytes)   Internal details about the EFS (e.g. sizes and ranges)
-> (inode names table)   Sequential list of inode names.
-> (inode list)          Sequential list of inode objects (all inode data except the names).
-> (data block list)     Sequential list of data blocks (including metadata)
-> (EOF)                 It had to come sometime, didn't it?
```

(Achtung again: that's subject to change from version to version until the library stablizes.)

As one can probably infer from this, the number of inodes and the size of each data block must be fixed when the EFS is created. The format currently allows for blocks to be added to the end of the EFS, but whether or not to add that as a feature is undecided, as there may be other (potentially more interesting) uses for the space after the data blocks (e.g. client-specified data).

Because records are of a fixed size, given the ID of any given inode or data block, we can calculate exactly where it is on disk and jump right to it. The "bookkeeping bytes" at the start of the EFS contain things like the size of each block, the length of inode names, etc., so that when we open the EFS we can use that info to find out where everything else is.

Each inode "owns" a number of blocks proportional to its logical size (i.e. the "size of the file") and the block size. For example, a pseudofile holding 500 bytes of information needs only 1 block if the block size is 500 bytes or less, and a file holding 33000 bytes needs 5 blocks if the block size is 8000 bytes.

# How data relationships are modelled #

Each inode has member which contains the ID of its first data block. An empty pseudofile has no data blocks (represented by the ID of 0, which is reserved for "not a block" and "not an inode" values). As data is written to a file, blocks may be added to it, and each data block holds the ID of the next block in the chain (or 0, meaning this block is the last one in the list).

Thus, given an inode we have (linear-time) access to its list of data blocks by traversing the next-block ID. While each block is of a fixed size, the inode itself stores the logical length (in bytes) of the pseuodofile, so that we will not consider unused bytes of the last block in the chain as part of the file.

Though the linear-time access penalty does come into play the first time an inode is read from or written to, subsequent i/o position calculations can be done in nearly amortized constant time. See [WhefsPerformance](WhefsPerformance.md) for more thoughts on that topic.

# How data is stored #

All FS-specific data is encoded in an endian-neutral format (pseudofile data is opaque to whefs and is not encoded). All FS objects are encoded using an initial tag byte (used for consistency checking) and followed by the actual data bytes. The underlying encoding routines supports the basic integral types plus c-style strings, and those routines are the basis for all data serialization. Each higher-level object, e.g. an inode, is encoded/decoded by iteratively en/decoding the member data.

An inode record, for example, currently (20090610) looks like this:

```
[uint8 TAG][uint16 ID][uint8 FLAGS][uint32 MODTIME][uint32 DATA_SIZE][uint16 FIRST_BLOCK]
```

The name is stored separately in a record which looks like:

```
[uint8 TAG][uint16 ID][uint16 LENGTH][... LENGTH char bytes ...]
```

Since all records in the FS have an on-disk size which is known at compile-time (or the maximum size, in the case of strings), and that size is fairly small, most objects are not written to disk one member at a time. They are first en/decoded into stack-allocated buffers and then written/read to/from disk in one operation.

There is one significant exception: data blocks. Data blocks are never automatically encoded or decoded, nor are they buffered in any way by the EFS engine (though the underlying storage might buffer). They are  are never read "in advance" - they are only read/written on demand. To the EFS, data blocks are opaque bytes, and each is passed as-is into or out of the underlying i/o device.


# Ranges and limits of the filesystem #

The filesystem currently requires that an EFS have at least two inodes. It reserves one for internal use, and a filesystem with no free inodes is effectively full. An EFS must also have at least as many blocks as inodes and the block size must be at least 32 bytes (arbitrarily chosen). In practice, a block size of 4k-32k is useful, with smaller block sizes being mainly only useful for testing whefs itself and larger sizes very wasteful except for some specific use cases (many large files).

Due to how inode names are stored, the library must unfortunately have a hard-coded maximum filename length, defined in the constant <tt>WHEFS_MAX_FILENAME_LENGTH</tt> (see [WhefsTweakingEFS](WhefsTweakingEFS.md) for details). Each in-memory inode requires this many bytes to store its name, and on-disk inodes each use a number of bytes defined by the EFS-specific options (which may never be greater than <tt>WHEFS_MAX_FILENAME_LENGTH</tt>). See [WhefsTweakingEFS](WhefsTweakingEFS.md) for more info on WHEFS\_MAX\_FILENAME\_LENGTH.

The maximum number of inodes and blocks is determined by <tt>WHEFS_ID_TYPE_BITS</tt>, which defaults to 16 bits (64k). See [WhefsTweakingEFS](WhefsTweakingEFS.md) for details on that. Since block <em>sizes</em> are of <tt>uin32_t</tt> (32 bits), a 64k limit on the <em>number</em> of blocks is not a real limitation when it comes to the amount of data we can store.

All of that said... even though whefs allows for fairly high limits (for an embedded filesystem, that is), and can be configured for higher limits (see [WhefsTweakingEFS](WhefsTweakingEFS.md)), the actual filesystem implementation will not scale well to large uses. It does only the most basic of caching (to conserve memory) and must refer to the underlying storage for many operations. Additionally, because it does only minimal caching, many lookups are linear in nature, meaning the time increases proportionally to the number of entries. Some options, e.g. searching for files by name, quickly get expensive (in terms of time and i/o) with this approach. Caching them makes i/o cheap but has the direct opposite effect on memory costs, so there's a balancing act to play here.

While the whefs filesystem <em>could</em> theoretically host tens or hundreds of thousands of pseudofiles in a container, that is considered to be well beyond the scalability of the current engine. If whefs ever becomes useful enough to potentially host that type of system, it may be adapted to do so.