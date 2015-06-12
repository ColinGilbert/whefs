whefs2c converts a whefs EFS container file to C code and creates a linkable in-memory EFS for it.

Usage: `whefs2c ObjectName < input.whefs > outfile.c`

The input should be a whefs EFS, but this program does not check that because the generated output could potentially still be used as a target for whefs\_mkfs().

This is best shown with an example:

Assume we have an EFS file named my.whefs, which we want to statically embed into an application. To do so we first use whefs2c:

```
~> whefs2c MyWHEFS < my.whefs > MyWHEFS.c
```

The name MyWHEFS is unimportant, but must be a legal C identifier name, as it will be used to name a static global object and a pair of functions.

Now we can use that in a client program like this:

```
#include "MyWHEFS.c"// this is the whole EFS + support code

int main(int argc, char const ** argv )
{
  whefs_fs * fs = MyWHEFS_whefs_open( true ); /* true=read/write, false=read-only */

  ... use the whefs API ...

  MyWHEFS_whefs_finalize(); /* closes the VFS and frees any VFS resources */
  /* It is legal to call MyWHEFS_whefs_open() to re-open the VFS. */
  return 0;
}
```

With this in place, we no longer need the my.whefs file, as its contents are completely embedded in statically-initialized memory in MyWHEFS.c.

Embedding an EFS this way of course has a huge memory cost (the size of the EFS, plus a few bytes for the added functionality), but it provides <em>really</em> fast access to the EFS and requires only a few kb of dynamically allocated memory. Also, by using the i/o device API it is easy to dump the in-memory EFS to disk and restore it from disk later (provided its size does not change), so this can be used as a "fast swap space" for an EFS which is used extensively. It could also theoretically be used to implement a basic form of transactional system, where committing a transaction flushes the in-memory EFS to disk.