#include <assert.h>
#include <stdio.h>
#include "wh/whefs/whefs.h"

// make the amalgamation then 
// gcc -std=c99 -o failtest mytest.c whefs_amalgamation.h


int main()
{
    int rc;
    struct whefs_fs_options fs_options = whefs_fs_options_default;
    fs_options.block_size = 128;
    fs_options.block_count = 10865;
    fs_options.inode_count = 505;
    fs_options.filename_length = 12;
    whefs_fs *fsys = 0;
    whefs_mkfs("test.img", &fs_options, &fsys);
    whefs_file *fileptr = whefs_fopen(fsys, "testf", "r+");
    assert( NULL != fileptr );
#if 1
    rc = whefs_ftrunc(fileptr, 13);
    assert( 0 == rc );
#endif

    whefs_fclose(fileptr);

    /* 

    If you flush/finalize/stop here it is as expected

    ./whefs-ls test.img 
    List of file entries:

    Node ID:  Size:   Timestamp: (YMD)     Name:
    2             0   2011.04.17 16:37:54  testf
    Total: 0 bytes
    1 of 505 total inodes listed.

    */
    rc = whefs_unlink_filename(fsys,"testf");
    assert( 0 == rc );
    //    goto close_fs;

    fileptr = whefs_fopen(fsys, "testf", "r+");
    assert( NULL != fileptr );

    size_t wrc = whefs_fwrite( fileptr, 3, 1, "hi!" );
    assert( 1 == wrc );
#if 0
    rc = whefs_ftrunc(fileptr, 1);
    assert( 0 == rc );
#endif
    rc = whefs_fclose(fileptr);
    assert( 0 == rc );

    { /* test issue #25 while we're at it ... */
        fileptr = whefs_fopen( fsys, "non-existent", "r" );
        assert( NULL == fileptr );

        rc = whefs_unlink_filename(fsys,"testf");
        assert( 0 == rc );
    
        fileptr = whefs_fopen( fsys, "testf", "r" );
        assert( NULL == fileptr );
    }
    
    goto close_fs;

    close_fs:
    rc = whefs_fs_flush(fsys);
    assert( 0 == rc );
    whefs_fs_finalize(fsys);
    /*

    at this point test.img is wrong:

    ./whefs-ls test.img 
    List of file entries:

    Node ID:  Size:   Timestamp: (YMD)     Name:
    Total: 0 bytes
    0 of 505 total inodes listed.

    */

    return 0;
}
