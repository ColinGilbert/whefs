This is the informal change list for whefs releases. Newest items are at the top. Only an overview is provided here - the exact list of changes is tracked only via the source repository.


---


**2 Nov 2011:**
  * Fixed two `whio_size_t` encoding/decoding calls which were incorrectly using `uint32_t` (as opposed to `whio_size_t`) en/decoding. See [issue #30](https://code.google.com/p/whefs/issues/detail?id=#30) for details. Warning: this fix required a minor file format incompatibility for builds where `WHEFS_SIZE_T_BITS=16` (it "should" be identical to/compatible with old FS containers built with `WHEFS_SIZE_T_BITS=32`).

**12 Oct 2011:**
  * Fixed [issue #28](https://code.google.com/p/whefs/issues/detail?id=#28), a misbehaviour the p-file truncation operation which caused it to zero-out data before the truncation point. Thanks once again to mikimotoh for the detailed bug report and test code.
  * Fixed [issue #29](https://code.google.com/p/whefs/issues/detail?id=#29), where an inode whose size was exactly at a block boundary had one too many blocks allocated to it (functionally harmless but wasted a block until the file's size changed).

**1 Oct 2011:**
  * Fixed [issue #27](https://code.google.com/p/whefs/issues/detail?id=#27), broken re-linking of pseudofile handles when closing handles in certain orders. This caused memory corruption, which showed up as a crash on some platforms but was silently unnoticed on mine (until that use case was put through valgrind). Many thanks to "mikimotoh" for the details bug report and test code (that made finding the problem really easy!).
  * Removed an extraneous flush() call which was cutting write-intensive performance by as much as 50%.
  * Made a new release with the above changes.

**18 Apr 2011:**
  * Fixed [issue #26](https://code.google.com/p/whefs/issues/detail?id=#26), which could cause "the ghost of an unlinked inode" to come back and haunt us. Thanks to Dan Schmitt for the bug report and assistance.

**26 Jan 2010:**
  * Apparently the compile fix from the last release didn't really get into the release tarball. Releasing again to fix that.

**18 Dec 2009:**
  * New release release due to a compilation bug i overlooked in the last one.

**1 Dec 2009:**
  * A slight performance hack required a small change in the EFS format. That invalidates older EFS containers.

**1 July 2009**
  * Added C++ `std::istream` and `std::ostream` wrappers for `whio_stream`, so it's possible to import/export data via the C++ STL stream API.

**26 June 2009**
  * Removed the full-string cache altogether. It _might_ be reintroduced in another form, but it's not on the immediate to-do list. The hash cache gives us 95% of what we need and requires much less memory.
  * Fixed a memleak of an inode name in `whefs_fclose()` when (ironically enough) the static allocators were enabled.
  * It is now possible to toggle the inode hash cache on/off at runtime on a per-EFS basis. Leaving it on is highly recommended, but a large EFS (lots of entries) will require relatively large amounts of memory for the cache (about 8 bytes/entry).

**23 June 2009**
  * All of the EFS caching elements can now be disabled at compile time. Work is underway to make them runtime-togglable.
  * Read-only EFSes are no longer mmap()ed because profiling shows read-heavy uses to be slowed down by this.
  * Added `whio_dev_api::iomode()` so downstream clients can ask the device about its read/write mode.

**22 June 2009**
  * Added optional support for `mmap()`ing file-based storage, which can speed up access. In some simple benchmarks `mmap()`ing cut the number of disk seek operations by about 75%.
  * Other (only incidentally related) optimizations cut the overall number of seeks considerably compared to the `mmap()`-reduced number (approximately 50% reduction in one test), but these changes also affect non-`mmap()`ed files.
  * Prettied up the output from [whefs-ls](WhefsLs.md).

**21 June 2009**
  * Added the [whefs-cat](WhefsCat.md) tool.

**20 June 2009**
  * Added `whefs_stream_open()` to provide sequential-only access to a pseudofile. This simplifies using pseudofiles together with `whio_stream_gzip()`, `whio_stream_copy()`, etc.
  * Fixed [Issue #6](https://code.google.com/p/whefs/issues/detail?id=#6), where closing an EFS while there are open pseudofiles would leak a file/device handle. They are now closed automatically during `whefs_fs_finalize()` if the client fails to close them beforehand.

**19 June 2009**
  * Added public `whefs_fs_entry` type and `whefs_fs_entry_foreach()` to iterate over all in-use EFS entries.
  * First packaged release.

**16 June 2009**
  * Moved the project over to Google code. This loses about 8 months of code history, but that's okay. The first commits were on 29 Nov 2008, and about 400 commits were made before moving to Google Code.