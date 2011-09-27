/**
   Test code for issue #27, contributed by mikimotoh:

   http://code.google.com/p/whefs/issues/detail?id=27
*/
#ifdef NDEBUG
#  undef NDEBUG
#endif

#include <assert.h>
#include <stdlib.h>
#include <wh/whefs/whefs.h>
#include <wh/whio/whio_devs.h>

void test_rw3files_2(){
    whefs_fs_options options={ WHEFS_MAGIC_DEFAULT, 
        4*1024, // blockSize
        4*1024, // blockCount
        4*1024, // inodeCount
        WHEFS_MAX_FILENAME_LENGTH,
    };
    whio_size_t imageSize = whefs_fs_calculate_size(&options);
    char* buf = (char*)malloc(imageSize);

    whio_dev* dev = whio_dev_for_memmap_rw(buf, imageSize);
    whefs_fs *fs=NULL;
    int errCode = whefs_mkfs_dev(dev, &options, &fs, true);
    assert(0 == errCode);
    whefs_file* rs1;
    whefs_file* rs2;
    whefs_file* rs3;

    rs1 = rs2 = rs3 = NULL;

#define CL(F) errCode = whefs_fclose(F); assert(0==errCode)
    /* valgrind is revealing lib-level memory mis-use
       in some whefs_fclose() ordering combinations...

       These closing orderings seem to work:

       (2,1,3)
       (1,2,3)
       (1,3,2)
       (3,2,1)
       (3,1,2)

       These break:

       (2,3,1)

       weird!
    */
    rs1 = whefs_fopen(fs, "rs1", "r+");
    assert(NULL != rs1);
    rs2 = whefs_fopen(fs, "rs2", "r+");
    assert(NULL != rs2);
    rs3 = whefs_fopen(fs, "rs3", "r+");
    assert(NULL != rs3);
    CL(rs2);
    CL(rs3);
    CL(rs1);
    whefs_fs_finalize(fs);
    free(buf);
}

int main( int argc, char const ** argv )
{
    test_rw3files_2();
    puts("If you got this far without crashing, then could not reproduce problem.");
    puts("BUT BE SURE TO RUN IT THROUGH VALGRIND to check for invalid mem access.");
    return 0;
}
