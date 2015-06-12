# whio\_epfs #

whio\_epfs (hereafter called EPFS) is a component of the whio i/o library. libwhio provides the generic back-end storage API off of which whefs is built. (Indeed, whio was written because i wanted a generic storage API on top of which i could write whefs!) EPFS was written to take over the core-most component of whefs: the management of inodes and data blocks. i.e. the core filesystem itself. EPFS is basically whefs stripped of inode names support. Leaving out the naming support removed a surprising amount of complexity from the core code and gives higher-level client code more freedom/options in how they handle embedded filenames. (i was never 100% happy with any of the 3 or 4 solutions i tried in whefs, for reasons of performance or memory consumption.) A whio\_dev-based hashtable was also written, and which will fit the inode-naming role perfectly. It can, in fact, itself be stored as a pseudofile within the EFS is serves. (whefs has historically reserved inode #1 for its own use, and the eventual embedding of filesystem metadata in the first pseudofile was in fact the reason for that. Now i've finally got something to store there!)

As of March 2010, EPFS is functional and feature-complete, and all of the components we need in order to rewrite whefs are in place. They simply have to be glued together now. whefs2, as it will be called, will be written on top of EPFS and other components which were written as part of the overall restructuring (e.g.. the whio\_dev-based hashtable).

But... bringing whefs2 to life will be a large undertaking (a complete rewrite, but not half as much work as has gone into EPFS), and i do not expect to get around to it until possibly late 2011.

EPFS is described in more detail in the whio source repo:

  * http://fossil.wanderinghorse.net/repos/whio/index.cgi/wiki/whio_epfs

And the new storage-based hashtable is described here:

  * http://fossil.wanderinghorse.net/repos/whio/index.cgi/wiki/whio_ht

# A Very Brief Demonstration #

It looks a little bit like this:

```
$ ./whio-epfs-mkfs my.epfs --namer=ht --block-size=8192 \
    --inode-count=128 --label='hi, world'
./whio-epfs-mkfs [my.epfs]: EFS container created (12288 bytes).
$ ./whio-epfs-cp my.epfs *.[ch]
$ ./whio-epfs-ls my.epfs 
whio_efps container file [my.epfs]:
Label:	[hi, world]
Inode #	   Size	  Mod. Time (Local)	Name
     1     5276   2011-04-19 22:05:12	<([whio_epfs_namer_ht.whio_ht])>
     2    32755   2010-12-20 07:35:56	EPFSApp.c
     3    13014   2010-12-26 17:49:58	cp.c
...
    26     6783   2011-04-19 20:38:08	whio_epfs_namer_array.c
    27    24956   2011-04-19 22:43:33	whio_epfs_namer_ht.c
    28    21915   2010-12-20 07:35:56	whio_epfs_namer_vlbm.c

Totals: 28 of 128 inodes take up 396439 bytes.

$ ls -la my.epfs 
-rw-r--r-- 1 stephan stephan 529140 Apr 19 23:05 my.epfs
```

That weird-looking name for inode #1 is the pseudofile used by this particular "namer' implementation. The "namer" interface is responsible for mapping inodes to names (and optionally also names to inodes). The "ht" namer impl uses an on-storage (whio\_dev-based) hashtable and maps both inode-to-name and name-to-inode. This namer impl uses the first available inode for its storage and then gives it a funny name so that clients don't try to use it. Since the hashtable supports any whio\_dev storage, we can dump it to a file and look at the hashtable like this:

```
$ ./whio-epfs-cp my.epfs -x 1=x.ht # <-- extract inode #1 to 'x.ht'
$ ./whio-ht-tool x.ht ls
[1970]=[http.c]=[<([4])>]
[3748]=[whio_epfs_block.c]=[<([18])>]
...
[1906]=[<([3])>]=[<([1848])>]
[3361]=[whargv.c]=[<([15])>]
[3489]=[whargv.h]=[<([16])>]
[3296]=[<([14])>]=[<([3234])>]
```

That rubbish is the internal listing of the hashtable, which maps names to inodes and inodes to names. "I/O device layering" makes this type of copying into and out of an EFS so easy. EFS pseudofiles are full-fledged whio\_dev i/o devices, and they live on top of a parent whio\_dev device (which need not be a file, but can be yet another "subdevice").