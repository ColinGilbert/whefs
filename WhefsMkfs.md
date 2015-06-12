

# whefs-mkfs #

whefs-mkfs is a tool similar to the conventional <tt>mkfs</tt> command. It creates EFS container files.

Usage: whefs-mkfs `[flags]` VFS\_file

Aside from the [common command flags](WhefsTools.md), whefs-mkfs supports these flags:

| **Parameter** | **Description** |
|:--------------|:----------------|
| `-i###`       | Sets the number of inodes. Valid range is 2 to (2^`WHEFS_ID_TYPE_BITS`-1). |
| `-b###`       | Sets the size of each block, in bytes. Range is 32 to (theoretically) (2^32-2), but in practice 4k-32k is realistic and extremely large values (e.g. 100M) may exceed other limits of the EFS. |
| `-c###`       | Set the number of blocks. Must be at least the the same value as `-i`, but may be higher (theoretically up to (2^`WHEFS_ID_TYPE_BITS`-1)). |
|`-s###`        | Set the maximum filename length. Maximum value is `WHEFS_MAX_FILENAME_LENGTH`.|
| `-n`          | "Dry run" mode - show what would be done, but don't do it. In dry run mode not all input ranges are fully checked because they happen at a higher level of the API which does not know what the valid ranges are. |

To understand what some of these values mean it may be helpful to read [WhefsFilesystem](WhefsFilesystem.md) and/or [WhefsTweakingEFS](WhefsTweakingEFS.md).

The library doesn't currently explicitly support "large files" (>2GB), so the combination of parameters should be low enough that they not exceed this limit. The `-n` parameter can be used to check this before creating the container. For example:

```
stephan@jareth:~/cvs/whefs/trunk/app$  ./whefs-mkfs -i700 -b8192 -n -c700 my.whefs
Dry-run mode! (Not creating/changing EFS container file!)
EFS container file: my.whefs
	Block size: 8192
	Block count: 700
	inode count: 700
	Free data bytes: 5734400
	Max filename length: 64
	Required container size (bytes): 5804480
	Metadata-to-data ratio: 1.2073%
```

## Creating an EFS for a given set of files... ##

The source tree comes with a script called <tt>whefs-mkfs-for-files.sh</tt> which creates a new EFS for a set of files, sizing the EFS to fit (well, it gets close, anyway). It is used like this:

```
stephan@jareth:~/cvs/whefs/trunk/app$ ./whefs-mkfs-for-files.sh my.whefs *.c *.h
Using these whefs tools:  whefs-ls whefs-cp whefs-mkfs
Size:        Blocks:    Name:
2088         1          addblocks.c
11524        2          avl.c
1347         1          bin2c.c
2383         1          cat.c
4555         1          cp.c
11769        2          ls.c
4850         1          mkfs.c
2163         1          rm.c
2430         1          staticfs.c
2033         1          subfs.c
18668        3          test.c
3558         1          whargv.c
4092         1          whefs2c.c
19901        3          WHEFSApp.c
18428        3          whio-test.c
101          1          x.c
15586        2          avl_tree.h
7286         1          bar.h
54           1          foo.h
6182         1          whargv.h

20 file(s) totaling 138998 bytes across 29 blocks.

File sizes:
    smallest: 54
    average:  6950
    middle:   9977
    largest:  19901
Max filename length: 11

Using these parameters:
    inode count: 21
    block count: 29
    block size: 8192

Really continue creating (or overwriting) [my.whefs]?
Tap Ctrl-C to abort to ENTER to continue...

Creating EFS...
+ ./whefs-mkfs -s11 -i21 -b8192 -c29 my.whefs
EFS container file: my.whefs
	Block size: 8192
	Block count: 29
	inode count: 21
	Free data bytes: 237568
	Max filename length: 11
	Required container size (bytes): 238715
	Metadata-to-data ratio: 0.4805%
./whefs-mkfs [my.whefs]: Done! Error code=0=[You win :)].
Importing files...
Import complete. Contents look like:
List of file entries:

Node ID:  Size:   Timestamp: (YMD)     Name:
2          2088   2009.07.05 12:54:34  addblocks.c
3         11524   2009.07.05 12:54:34  avl.c
4          1347   2009.07.05 12:54:34  bin2c.c
5          2383   2009.07.05 12:54:34  cat.c
6          4555   2009.07.05 12:54:34  cp.c
7         11769   2009.07.05 12:54:34  ls.c
8          4850   2009.07.05 12:54:34  mkfs.c
9          2163   2009.07.05 12:54:34  rm.c
10         2430   2009.07.05 12:54:34  staticfs.c
11         2033   2009.07.05 12:54:34  subfs.c
12        18668   2009.07.05 12:54:34  test.c
13         3558   2009.07.05 12:54:34  whargv.c
14         4092   2009.07.05 12:54:34  whefs2c.c
15        19901   2009.07.05 12:54:34  WHEFSApp.c
16        18428   2009.07.05 12:54:34  whio-test.c
17          101   2009.07.05 12:54:34  x.c
18        15586   2009.07.05 12:54:34  avl_tree.h
19         7286   2009.07.05 12:54:34  bar.h
20           54   2009.07.05 12:54:34  foo.h
21         6182   2009.07.05 12:54:34  whargv.h
                         Total: 138998 bytes
20 of 21 total inodes listed.
Done processing EFS container file [my.whefs]:
-rw-r--r-- 1 stephan stephan 238715 2009-07-05 12:54 my.whefs
```

The script may need to be tweaked for certain data sets to avoid too much wasted space. For example, if packaging only very small files (under 2k) or only large files, you may want to change the default block size (4k). If someone can come up with a best-fit algorithm for the script i'd be more than happy to include it!