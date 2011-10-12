/**
   Test code for issue #27, contributed by mikimotoh:

   http://code.google.com/p/whefs/issues/detail?id=27
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
    int errCode = whefs_mkfs(":memory:", &options, &fs);
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
    assert(memcmp(buf, "abcd", 4) == 0);//This point fails!!
    
    whefs_fclose(rs1);
    whefs_fs_finalize(fs);
}

int main( int argc, char const ** argv )
{
    test_issue_28();
    puts("Done!");
    return 0;
}
