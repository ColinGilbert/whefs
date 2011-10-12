/**
   Test code for issue #28, contributed by mikimotoh:

   http://code.google.com/p/whefs/issues/detail?id=28
*/
#ifdef NDEBUG
#  undef NDEBUG
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h> /* memcmp() */
#include <wh/whefs/whefs.h>

void test_issue_28()
{
    whefs_fs_options options= whefs_fs_options_default;
    whefs_fs *fs=NULL;
    whefs_file* rs1 = NULL;
    char buf[4]={0};
    int errCode;

    options.block_size = 512;
    options.inode_count = 2;
    options.block_count = 3;
    errCode = whefs_mkfs("28.whefs", &options, &fs);
    
    assert(0 == errCode);

    rs1 = whefs_fopen(fs, "rs1", "r+");
    assert(rs1);

    whefs_fwrite(rs1, 4, 1, "abcd");
    whefs_fwrite(rs1, 4, 1, "efgh");

    whefs_fseek(rs1, 0, SEEK_SET);
    whefs_fread(rs1, 4, 1, buf);
    assert(memcmp(buf, "abcd", 4) == 0);//This point succeeds.

    whefs_ftrunc(rs1, 4);

    whefs_fseek(rs1, 0, SEEK_SET);
    whefs_fread(rs1, 4, 1, buf);
    assert(memcmp(buf, "abcd", 4) == 0)
        /* failing here due to bug in whefs_block_wipe_data()
           not honoring final argument.
        */;

    whefs_ftrunc(rs1, options.block_size);
    assert(options.block_size == whefs_fsize(rs1));
    whefs_fseek(rs1, -4, SEEK_END);
    whefs_fwrite(rs1, 4, 1, "wxyz");
    whefs_fseek(rs1, -4, SEEK_END);
    whefs_fread(rs1, 4, 1, buf);
    assert(0==memcmp(buf,"wxyz",4));
    assert(options.block_size == whefs_fsize(rs1));
    
    whefs_fclose(rs1);
    whefs_fs_finalize(fs);
}

int main( int argc, char const ** argv )
{
    test_issue_28();
    puts("Done!");
    return 0;
}
