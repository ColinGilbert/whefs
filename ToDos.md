Despite the (out-dated) lists below, as of mid-2011 there are no current TODOs because most of them have been implemented in whefs' successor, [whio\_epfs](http://whiki.wanderinghorse.net/wikis/whio/?page=whio_epfs). Bugfixes will be made (if reported!), insofar as they can be made within the current architecture, but no new features are planned.

# whefs TODO #

This is my personal list of "immediate" TODOs (in no particular order):

  * Several parts of the public API need to be rounded out.
  * Directories. See [Issue #3](https://code.google.com/p/whefs/issues/detail?id=#3) for thoughts on that.
  * Re-model the inode string table as an on-disk b-tree or AVL tree (or similar). With this we could drop the hash cache altogether. It would cost i/o (not terribly much) but not much memory.
  * Concurrency via advisory locks (POSIX `fcntl()`). See [WhefsConcurrency](WhefsConcurrency.md).
  * Concurrency via threads. See [WhefsConcurrency](WhefsConcurrency.md).
  * Scripting language bindings via [SWIG](http://www.swig.org). i've tinkered with scripting the i/o layer a bit, and i don't think making whio/whefs play nicely with SWIG will be all that difficult to do. We've already got hand-implemented [JavaScript bindings for whefs](http://code.google.com/p/v8-juice/wiki/PluginWhefs).
  * Add storage for Unix access rights on the inodes (not user/group IDs), or at least +x bit. We have no intention of using them to control access within the VFS, but for when we export the data to a file (so we can keep the original permissions intact).
  * Add client-supplied metadata to inodes, in the form of (for example) 2 64-bit numeric slots. These could be used to store client-specific stuff like user/group IDs, file type information, and such.
  * Add ability to "mount" a VFS as a subdirectory of another VFS. This has a big fat can of worms attached to it, though, and is dependent on directory support (which is in turn dependent on path parsing).
  * 20090111: (maybe) change file format to put block metadata all together and block data all together. This will simplify certain operations. The format originally kept them together (on disk) because it made it easy to visually check if the layout was doing what it was supposed to (i used 'less' as a test tool before i got pseudofiles working).

## Currently Underway ##

**2009 Dec:**

  * The core EFS bits are being refactored into a lower-level interface in the whio API. Those bits deal with inodes and blocks, but only with inode IDs, not names. This will allow those base bits to be implemented very memory-efficiently, and the client (in this case whefs) will be able to add whatever filename-to-inode mapping features they need on top of that.
  * A side-effect of the above is that the EFS will no longer be required to have a fixed number of blocks - it will be able to expand as needed. The inode count, however, will still be fixed when an EFS is created.
  * The newer split of the API may allow us to add thread-level concurrency in the higher-level API, at least to some degree.