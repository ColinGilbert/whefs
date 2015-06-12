whefs-ls is a tool similar to the conventional <tt>ls</tt> command. It lists the contents of a VFS file.

Usage: whefs-ls `[flags]` VFS\_file `[filter_pattern]`

Aside from the [common command flags](WhefsTools.md), whefs-ls supports these flags:

  * -i = Show the inode (file) list. This is the default operation.
  * -b = Show blocks owned by each inode. Use with -v to show more information.
  * -B = Show used blocks in an alternate (longer) format.
  * -s = Show some basic VFS stats.

Any arguments after the VFS filename are considered to be shell-style globs (be sure to quote them!). If any patterns (which may also be file names) are provided, only file entries matching one of those patterns are shown in the file listings.

For example:

```
stephan@jareth:~/cvs/fossil/whefs$ ./whefs-ls my.whefs 
List of used inodes:
Node ID:        First block:    Size:       Timestamp:      Name:           
2               1               4478        1231009247      cp.c
3               4               3570        1231009247      foo.c
... snip...
36              228             273         1231009247      whio.h
37              229             5774        1231009247      whprintf.h
                         Total: 434053 bytes
36 of 37 inodes in use
stephan@jareth:~/cvs/fossil/whefs$ ./whefs-ls my.whefs 'whio*.c'
List of used inodes:
Node ID:        First block:    Size:       Timestamp:      Name:           
20              92              683         1231009247      whio.c
21              93              11376       1231009247      whio_dev.c
22              99              7293        1231009247      whio_dev_FILE.c
23              103             20670       1231009247      whio_dev_mem.c
24              114             9957        1231009247      whio_dev_subdev.c
                         Total: 49979 bytes
5 of 37 inodes in use
```

Not all common glob patterns are supported. Most notably missing are sets in the form <tt>{a,b,c}</tt>. For example, the pattern `*.{c,h}` will not behave as it does in the shell (specifically, it will only match some really weird file names). As a workaround, break it into two patterns: `*.c` and `*.h`.