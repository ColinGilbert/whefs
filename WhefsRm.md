whefs-rm is a tool similar to the conventional <tt>rm</tt> command. It can remove files living inside a VFS.

Usage: whefs-rm `[flags]` VFS\_file `[file names or quoted glob patterns]`

Aside from the [common command flags](WhefsTools.md), whefs-cp supports these flags:

  * -n = Dry-run mode (don't actually delete anything).

Example:

```
stephan@jareth:~/cvs/fossil/whefs$ ./whefs-ls my.whefs 'whio*.h'
List of file entries:

Node ID:        First block:    Size:       Timestamp:      Name:           
33              194             4858        1231243700      whio_common.h
34              197             30593       1231243700      whio_dev.h
35              212             16626       1231243700      whio_devs.h
36              221             273         1231243700      whio.h
                         Total: 52350 bytes
4 of 37 inodes in use

stephan@jareth:~/cvs/fossil/whefs$ ./whefs-rm -v my.whefs 'whio*.h'
./whefs-rm [no VFS]: Verbose mode activated.
./whefs-rm [my.whefs]: Opening VFS [my.whefs]
./whefs-rm [my.whefs]: Deleting VFS file [whio_common.h]...
./whefs-rm [my.whefs]: Deleting VFS file [whio_dev.h]...
./whefs-rm [my.whefs]: Deleting VFS file [whio_devs.h]...
./whefs-rm [my.whefs]: Deleting VFS file [whio.h]...

stephan@jareth:~/cvs/fossil/whefs$ ./whefs-ls my.whefs 'whio*.h'
List of file entries:

Node ID:        First block:    Size:       Timestamp:      Name:           
                         Total: 0 bytes
0 of 37 inodes in use
```