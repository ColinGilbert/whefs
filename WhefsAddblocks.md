whefs-addblocks appends data blocks to an existing EFS container file. It is not currently possible to expand the number of inodes or change the maximum filename length without recreating the FS and re-importing the data.

Usage: whefs-addblocks `[flags]` EFS\_file

Aside from the [common command flags](WhefsTools.md), whefs-addblocks supports these flags:

  * -a# = Appends # blocks to the EFS.

The total number of blocks is limited by the value of [WHEFS\_ID\_TYPE\_BITS](WhefsTweakingEFS.md). The default maximum number of inodes or blocks is 2^16 (64k).

Achtung: always make a backup of your EFS first, just in case lingering bugs in whefs (or an untimely Ctrl-C) corrupts the EFS.