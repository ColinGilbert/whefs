# whefs News (most recent items at the top) #

**2 Nov 2011:**
  * [Issue #30](https://code.google.com/p/whefs/issues/detail?id=#30) revealed an encoding/decoding problem for one of the EFS' internal metadata bits. The fix is unfortunately incompatible with prior FS containers created with `WHEFS_ID_TYPE_BITS=16`. It "might" be compatible with those created with `WHEFS_ID_TYPE_BITS=32`, or i "might" have had a cross-wiring of two numbers which would also cause that to be incompatible.

**12 Oct 2011:**
  * Two more bugfixes (one minor, one significant). See the [ChangeLog](ChangeLog.md) for details.

**1 Oct 2011:**
  * Fixed a long-standing file handle list mis-linking bug which was unlikely to show up until 3 handles were opened and then closed in a particular order. All users "should" upgrade, as the previous code effectively had a memory corruption case. It would never appear if only one pseudofile was opened and was highly unlikely to appear if only 2 were opened, but was bound to happen once 3 or more handles were opened. See [issue #27](https://code.google.com/p/whefs/issues/detail?id=#27) for more details.

**15 Aug 2011:**
  * Whefs is now officially superseded/obsoleted by [whio\_epfs](http://whiki.wanderinghorse.net/wikis/whio/?page=whio_epfs), which can do everything whefs can do (plus some), does most of it faster, and includes a C++ wrapper which can simplify usage significantly. whefs _is still maintained_, and bug fixes which can fit within the current architecture will be made where feasible (and reported by users), but no significant additions/changes are planned.
  * We now have a JavaScript binding for EPFS: http://code.google.com/p/v8-juice/wiki/V8Convert_Whio

**18 Apr 2011:**
  * Fixed [issue #26](https://code.google.com/p/whefs/issues/detail?id=#26), which could cause an "inode ghost" to come back and haunt us.
  * Learned from user Dan Schmitt than whefs compiles as-is on Android "ndk" and "AOPS" kernels :-D.
  * Fixed the last known major bug in the [whio\_epfs\_namer](http://fossil.wanderinghorse.net/repos/whio/index.cgi/wiki/whio_epfs_namer) implementation, which means that [whio\_epfs](http://fossil.wanderinghorse.net/repos/whio/index.cgi/wiki/whio_epfs) now has all the features of whefs, plus some. The writing of whefs2 can finally begin. (But it won't begin tonight!)

**18 Dec 2010:**
  * Officially added the [whio\_epfs\_namer](http://fossil.wanderinghorse.net/repos/whio/index.cgi/wiki/whio_epfs_namer) interface to the  [whio\_epfs](http://fossil.wanderinghorse.net/repos/whio/index.cgi/wiki/whio_epfs) API, along with a useful implementation which uses [a hashtable](http://fossil.wanderinghorse.net/repos/whio/index.cgi/wiki/whio_ht) stored as a pseudofile in the EFS. With that in place, EPFS can now do almost everything whefs can (but is more performant all-around), and whefs2 is one step closer to becoming something other than a vaporware.

**13 Oct 2010:**
  * After a half-year development pause, over in the [libwhio source tree](http://fossil.wanderinghorse.net/repos/whio/) we've added a "namer" interface into the `whio_epfs` API, which allows software like whefs to flexibly add its own inode-to-name mappings to the `whio_epfs` core filesystem. For example, we can use this in conjunction with [whio\_udb](http://fossil.wanderinghorse.net/repos/whio/index.cgi/wiki?name=whio_udb) to provide a hashtable back-end which stores the inode/name mappings. The plan is to implement whefs2 as a thin wrapper around the (`whio_epfs`+`whio_udb`) combination.


**16 March 2010:**
  * Over in the [libwhio source tree](http://fossil.wanderinghorse.net/repos/whio/), we've finally got all of the components which we need in order to start refactoring whefs. It won't be so much a refactoring as a rewrite. _All_ of the core features of whefs have been refactored into lower-level components, which now simply need to be plugged in together to re-implement whefs. i expect that 75-80% of the whefs code will literally disappear through the refactoring. Because of the drastic level of change, i've decided to leave whefs as it is and call the rewrite whefs2. whefs2 will have all the main features of whefs, none of the speed bottlenecks (e.g. finding the next free inode/block is now O(1)), plus the new components allow a few other features which whefs doesn't have. e.g. it will be able to grow on demand and the new inode-to-name mapping parts perform better while using a constant amount of memory (independent of the number of inodes in the EFS). The new code will likely have a slightly larger memory footprint, but i expect to see it cap out at well under 10kb of RAM (my current estimate is 5-6kb).
  * All that having been said... it will still probably be some months before i get around to the refactoring. (i'm having far too much fun hacking on the lower-level bits!)


**12 March 2010:**
  * Woohoo! Early this morning i got a new [whio\_dev-based "micro database"](http://fossil.wanderinghorse.net/repos/whio/index.cgi/wiki/whio_udb) (UDB) API working. It is a storage-based hashtable with amortized O(1) insert/search/remove, uses arbitrary whio\_dev storage, has near-constant memory requirements, and will make a _perfect_ replacement for the current inode-name management bits. :-D

**10 March 2010:**
  * Over in the [whio\_epfs](http://fossil.wanderinghorse.net/repos/whio/index.cgi/wiki/whio_epfs) source tree (which will eventually become the basis of a refactored whefs), the two most glaring performance bottlenecks have been removed: the find-the-next-free-record operations for inodes and data blocks are now O(1) plus a small amount i/o to update the free-list links (zero, one, or two linked records). These ops have so far been O(1) for the average case but degrade to O(N) worst case. With that out of the way, the internal filesystem engine can scale to much larger sizes than before without a notable runtime performance hit.

**4 March 2010:**
  * The new bits which will eventually replace much of whefs internals (mentioned below) are functionally complete and seem to work well, providing all of the core features of whefs except the ability to map names to inodes (which will be provided by the whefs layer). At some point work will begin on rewriting whefs on top of [whio\_epfs](http://fossil.wanderinghorse.net/repos/whio/index.cgi/wiki/whio_epfs), but it is likely to be some months before i start on it (for lack of energy).

**16 Feb 2010:**
  * Work has been underway the past couple of months to prepare whefs for a major refactoring, where it will be split into two parts: [whio\_epfs](http://fossil.wanderinghorse.net/repos/whio/index.cgi/wiki/whio_epfs), now functional and largely feature-complete, will provide the basic i/o features, e.g. the inode-to-block mappings and the inode-as-whio\_dev translation. whefs will then add the inode-to-filename mapping support on top of that. Since whefs will have to be completely gutted and rewritten for this, it's likely to be a good while before it gets done. i will probably go ahead and fork off whefs2 for that purpose (it'd be simpler, overall).

**29 Nov 2009:**
  * whefs turns one year old this week. The first commits i could trace go back to one year ago today. The library wasn't really functional until the end of December, 2008, though.

**22 June 2009:**
  * Added optional `mmap()` support for file-based storage, which speeds up access considerably and drastically cuts the overall amount of seek operations which actually have to refer to disk. (For read-intensive uses `mmap()` actually slows whefs down.)

**16/17 June 2009:**
  * Ported the project over to Google Code. i miss the ability to download a snapshot zip of the source repo, but the wiki/issues/downloads maintenance is much simpler on Google Code.

<b>13 June 2009:</b>
  * Added a heavier-weight name caching supplement, which allows us to search for an inode by name without search-related i/o once it has been traversed. Its memory cost becomes arguably too high for EFSs with lots of files, especially those with high maximum filename length limits. It is currently always on but a toggle will eventually provided, as the memory cost becomes prohibitive on very small systems

<b>10/11 June 2009:</b>
  * Added a basic filename lookup cache. It's quite primitive, and isn't yet fully optimized (it must be sorted more often than it should be) but it's fairly memory-light and it's a starting point. For cases where a file is sought by name, this provides a dramatic reduction in the search time.
  * Shown to compile and run cleanly on the [Nexenta flavour of OpenSolaris](http://www.nexenta.org/os).

<b>8 June 2009:</b> (Happy birthday to my brother Toby!)
  * Now compiles with gcc's <tt>-pedantic</tt> and <tt>-fstrict-aliasing</tt> flags. Seems to work, too.
  * It is now possible to add blocks to an EFS at runtime (via <tt>whefs_fs_append_blocks()</tt>). It is not possible to add inodes, and will not be without first significantly refactoring of how inodes <em>and</em> blocks are stored (and subsequently addressed).

<b>22 March 2009:</b>
  * Got the most basic of concurrency options - locking the whole VFS - working. See [WhefsConcurrency](WhefsConcurrency.md) for the current state.
  * It is now possible to use whefs from JavaScript: http://code.google.com/p/v8-juice/wiki/PluginWhefs. As far as i know, this is the world's first embedded filesystem which is JavaScriptable.

<b>15 Jan 2009:</b>
  * Some restructuring of the inodes data (not yet completed) has led to an overall reduction in the number of calls to read() and write() on the underlying storage by approximately 40%. The amount of data is the same, but we're read/writing certain objects as an encoded blob then decoding/encoding them in memory, as opposed to making one read()/write() call per member field (as was done before).

<b>11 Jan 2009:</b>
  * whefs now runs on 64-bit platforms. Previously it wouldn't work on platforms where size\_t was not 32 bits. Thanks to http://www.hostmonster.com for unwittingly providing a 64-bit box to test on. This also proved that the file format is independent of the bitness (it is theoretically also endian-neutral, but that hasn't yet been proven).

<b>9 Jan 2009:</b>
  * <em>More speed!</em> The routine which figures out which virtual block is associated with a given pseudofile read/write position is now somewhere close to amortized constant-time (it used to be linear before, based on the number of blocks owned by the inode). It is O(N) (linear) the first time it is called for a given inode (to load the block list: N=(number of blocks owned by the inode)) and when appending multiple blocks at a time (N=(new\_block\_count-old\_block\_count)), but for operations within existing block boundaries (that is, the vast majority of the time) it is constant time after the one-time load of the block list.

<b>8 Jan 2009:</b>
  * i added a cache of blocks used by opened inodes. This <em>drastically</em> speeds up the majority of pseudofile read/write operations, cutting the number of underlying i/o operations by more than <em>20 times</em> in some places. But... this also means that it's no longer possible to configure whefs to use <em>no</em> calls to malloc(). That said, the memory cost is still low - currently about 8 bytes per block belonging to the opened inode (they get cleaned up when the inode is closed).
  * There have been other dramatic performance improvements today. After running many tests through valgrind/callgrind i was able (with the help of KCacheGrind) to identify the horrible hotspots and eliminate the most glaring performance problems. As of right now, i haven't got any complaints about performance for the average use case (though some ops are still glaringly linear).

<b>7 Jan 2009:</b>
  * So you want a <em>fully embedded</em> filesystem? An experiment a few hours ago showed that we could: A) create a VFS file, B) convert that VFS file to C code (as a big char array), C) link that C code in with a standalone program, and D) use the VFS in memory from there (with read/write access). Since it is trivial to dump any VFS to any other output device, the in-memory VFS can be dumped to a file (and re-imported into static memory, as long as the VFS file doesn't grow). There's got to be some weird uses for that, e.g. embedding config files (or default versions of them) directly in the application. Or even app temp files (and no more worries about other applications reading them).

<b>2 Jan 2009:</b>
  * Experimentation with alternatives to malloc() has shown that we can, with the proper combination of compile-time options, configure whefs to not require a single call to malloc() (by using shared pools of statically-initialized objects, dropping back to malloc() if the pool fills up). That said, calling fopen() will indeed malloc() (352 bytes on my machine), so unless one is using a custom i/o device (or a read/write static memory buffer) for the back end storage, at least 1 malloc() is required.

<b>27 Dec 2008:</b>
  * The read() API appears to work, so now the most significant bits are in place for full-featured pseudofiles. (Now we just need to test the hell out of it and flesh out the API.)
  * It is indeed possible to embed a vfs within a vfs (apparently to an arbitrary depth). This is a side-effect of the i/o model, and requires no special case handling in the vfs or i/o parts.