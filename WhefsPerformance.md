

See also: [Tweaking an EFS](WhefsTweakingEFS.md)

# Notes and ramblings about whefs's performance #


It goes without saying that a filesystem should be fast. That said, whefs is not blazingly fast. <em>That</em> said, it is actually quite performant for the range of use cases it was initially envisioned for.

whefs represents my first attempt at modeling a filesystem. Most of it is fairly conventional, but one of the early goals was keeping a very lean memory profile, so in some areas it must refer to disk more often than necessary (in particular is must `seek()` a lot). Versions of whefs before 20090108 had particularly poor performance due to a huge i/o overhead (but had almost no dynamic memory costs, and was fast enough for small use cases), but the speed and amount of i/o overhead has been improved _significantly_ since then. Right now (June 2009) i'm fairly satisfied with its performance, at least when the basic caching options are turned on (discussed in more detail below).

# Caching... #

whefs has no unified internal caching mechanism. The primary reason is to save memory. Instead, i've opted for adding specialized caches for specific operations where they help significantly or are required for the sake of data consistency. The engine currently uses the caches described below...

## Opened inodes cache ##

When an inode is "opened" (e.g. via `whefs_fopen()`), its record is stored in memory. If the file is opened multiple times, each independent file handle shares that same inode copy. This is required for proper synchronization of the inode data (think of it as a detailed file descriptor) across file handles. The library currently enforces that an inode may only be opened for writing by one caller, but allows any number of readers. Whether or not to allow multiple writers is under consideration. If a use case comes up which can't be done without it, i'll consider adding it, otherwise i'm prone to side-step the potential problems involved.

Anyway...

The opened-inodes cache is used for several things:

  * As mentioned above, each opened file handle for the same inode references a shared copy of the inode.
  * When inodes are searched for in some contexts, the opened inodes cache is first checked before referring to disk. The reason is so that the request will pick up any changes made to the inode since the last time the owning writer flushed it (which normally only happens when the file is closed, unless the client explicitly flushes).

The cache is currently a linked list of entries, so these operations are O(N), but N is normally quite small (1-2 is typical, more than 4 is unusual), being only the number of currently opened inodes. Since N is typically so small in this case, it is expected that a linked list can perform (on average) as well or better than a heavier-weight search mechanism for all but the weirdest use cases (e.g. someone has 10 file handles opened at once). A binary search mechanism has shown to be a poor choice for this case for reasons explained in the next section.

## In-use inode and block cache ##

The engine stores a bitset in which the "is in use" state of each inode and block is stored. "In use" for an inode means it is associated with a file. An "in use" block is owned by an inode (i.e. it contains pseudofile data).

This bitset is optional (it can be disabled at compile time) and requires ((inode\_count+block\_count)+8) <em>bits</em> (not bytes) of space. It affects only the following operations:

  * Searching for the next free inode or block (we can skip entries marked in-use).
  * Searching for an inode by name (we can skip the unused entries).

Any items we cannot skip (due to their marking in the appropriate bitset) must be read from disk before we can find out if it matches our search criteria.

These operations are unfortunately O(N), but using the bitset cache allows us to avoid i/o when we know an items cannot meet our search requirements, so the relative cost of searching is reduced.

Even though using these caches only require addition operations and dereferencing pointers, profiling has shown that these caches cost a disproportionately high amount of time compared doing a full search if the underlying storage is particularly fast. e.g. for in-memory EFSes or extremely fast disks this cache can be a slight speed sink. For storage where i/o is relatively costly, however, these caches serves their purpose.

## Inode Names Cache ##

As of 20090611, whefs has a search cache mechanism which allows much faster searches for files by their exact name. Before this change, it was an O(N) search, where N was <em>approximately</em> the number of inodes in the filesystem. (N is somewhat smaller, actually, due to the effects of the in-use inode cache.) This name cache has a much higher cost than the inode/block bitmap caches (about 8 bytes per cached entry), but provides a huge performance boost when searching for files by name.

It works by using a sorted array of entries which map the name's hash value to the inode's id. Once this cache is built (it is automatically built during whefs\_fopen()), searches by name can be done in average O(log N) time instead of O(N) time, by doing a binary search with the hash value of the name as the search key.

The cache is, by default, built when the fs is opened. If it is not initialized when the EFS is opened then it will be built up incrementally as files are searched from and added to the FS, and when an inode is renamed (which requires special handling of the cached key). It grows incrementally, but won't allocate more than it can possibly use (that is, never more than the inode count of the FS). In a worst case, this costs `(inode_count*sizeof(whefs_hashid)[=currently 8])` bytes of memory, plus another 20 bytes or so for the management data structure. So, for 1000 _used_ inodes, the cache would cost up to about 8k. That is its peak, however - it will only get that high if it reads enough _in-use_ inodes to cause it to grow (non-in-use inodes don't take up memory in the cache).

Though this cache can cut the search-by-name time by a factor of hundreds over previous code, 1000+ inodes is still considered to be fairly large for whefs' target audience. Then again, the primary reason so many inodes were considered "too many" was because of the time associated with searching for them.

The API will eventually evolve to allow clients to monitor and tune some aspects of this cache. At the moment it has an embarrassingly low-level interface and needs several important optimizations (like optimizing the number of sorts which are performed on the cache).

As of 20090629, experimentation is underway with replacing this so-called "hash cache" with an in-disk AVL tree. This approach costs much more i/o than the hash cache but also has a memory cost which is independent of the number of inodes. (An AVL tree was chosen over a B-Tree because the B-Tree code i've looked at so far is way over my head, whereas AVL code is still grokkable.)

## Block Chain Cache ##

whefs does not, by default, cache any block-related info other than the (optional) bitset cache mentioned above. However...

When an inode is opened for i/o, the most common operation involved is mapping the read/write position of a psdeudofile into the coordinate space of the VFS data blocks. That operation essentially follows this order:

  * Caller requests a translation of position P.
  * Find first data block of the inode. If it has none and this is a read operation, the operation fails. If it is a write request, we add an initial block here. Call this block B1.
  * Jump (or walk) to block number X in B1's block chain, where X is (P / BLOCK\_SIZE + 1). Again, a write operation will add blocks as needed.
  * If we can reach a block, the operation succeeds and we return the block information to the caller.

These operations are performed <em>every</em> time we read or write from or to a pseudofile, no matter how big or small the read/write request is (with the exception of 0-byte read/write requests, which are optimized away).

The expensive part here is primarily walking the block chain. The earliest versions of whefs had to literally walk the disk for each block in the chain on each request. It was simple to code but terribly inefficient, requiring an absurd amount of seeking and reading. However, it also costs us no memory at all (only the stack and there is no recursion involved).

Versions starting with 20090108 instead walk the block chain only a single time, then store the chain as an array of block objects. The `sizeof(whefs_block)` is, by default, only 8 bytes per block (slightly higher if `WHEFS_ID_TYPE_BITS` is set to 32 or 64), so the array costs us very little. Building that array is a linear operation which walks the disk as mentioned above (O(N), where N=number of blocks owned by the inode), but we always know one step in advance where the next seek/read must be, so there are no extra searches required.

The savings come in subsequent position-to-block translations, which become  O(1) if the requested position is inside the current bounds of the file's size. It is a simple calculation and a jump to the corresponding offset in the block cache array. The array is kept in logical block order so that we don't need to second-guess the offset of the array. If blocks must be added to the chain due to a write causing the file to grow, the block chain cache is expanded (using realloc()) and the operation becomes O(N), where N is (new\_block\_count-old\_block\_count). For the common case (a file growing at a rate smaller than the block size) N is 1. So the amortized cost of position-to-block translations becomes effectively (as i understand it) O(1) or close to it.

We delay loading the block cache until the first position translation is requested, which adds a one-time linear factor to the initial position translation request. In addition, allocating a new block is O(N) (N=block size) because the allocator explicitly zeroes out any older contents of the block, for data integrity reasons.

The building of the cache is delayed until i/o because we don't ever need it before that. Some operations on a file (e.g. renaming them) do not require doing i/o on the data blocks, and it would be wasteful to load the cache immediately after the inode is opened (which is actually the logical place to do it) because many operations won't ever use that cache.

Adding the byte chain cache improved the speed of the whefs test applications by 20-30x in some places, and cut the overall amount of time spent on i/o by 2/3rds (as measured by Valgrind/Callgrind). Those numbers became even more favourable as the number of read and write operations on a pseudofile  went up.

## A final word on caching... ##

We will eventually add more specialized caches to speed these operations up, but there are some logistical problems involved:

  * Any caching which requires a significant amount of memory must be either optional or so compelling in and of itself that it becomes a must-have. That is a design requirement of the library.
  * Who says when to cache something and when not?
  * Who says how much we're allowed to cache? And how can we control that?

# Device-specific Hacks #

Though the underlying i/o interface is generic, hiding the details of the underlying storage, we can sometimes make use of device-specific features for performance gains...

## Speeding up file access with `mmap()` ##

Versions starting with [r184](https://code.google.com/p/whefs/source/detail?r=184) (20090622) have an option to enable `mmap()`, a POSIX-1.2001 function which gives access to files via direct memory addressing. If the EFS kernel thinks that the device can support it, it will try to `mmap()` the storage and access it via an i/o proxy device which acts upon the `mmap()`ed memory range instead of the file (if `mmap()` fails it falls back to normal access). This speeds up access drastically and in some simple tests it cut the number of `seek()` operations which actually go to disk by 75%. Seeking is the most common i/o operation in the library, so using `mmap()` gives a significant payoff here.

To enable this, see the `WHEFS_CONFIG_ENABLE_MMAP` macro in [whefs\_config.h](http://code.google.com/p/whefs/source/browse/trunk/include/wh/whefs/whefs_config.h).

For those interested in detail: this hack works by probing the storage using ioctl calls. If it has a file descriptor number then we call `mmap()`. If that works we create a whio\_dev device which proxies that memory range and directs `flush()` operations to `msync()` (the `mmap()` equivalent of `fsync()`). We then replace the kernel's device with that one, so it directs all i/o through the memory mapped device. The implementation is _mostly_ non-intrusive on the EFS kernel, but due to some seemingly unrelated internal details it's not 100% unintrusive (e.g. if the EFS size changes during its lifetime we have to re-`mmap()` it).

Profiling has shown that using this approach for read-heavy uses actually costs more time than using the file device directly (most of it in `memcpy()`). Thus, if an EFS is opened in read-only mode it is not `mmap()`ed.

Also, on some older platforms the `mmap()` code has been seen to segfault (see [Issue #15](https://code.google.com/p/whefs/issues/detail?id=#15)), and so this option should be tested on any given platform before it is compiled in as the default behaviour.