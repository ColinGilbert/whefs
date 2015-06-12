
See also: [WhefsTools](WhefsTools.md)


# Hello, world! #

Here's the "Hello, world!" of whefs...

```
// Open an existing EFS:
whefs_fs * fs = 0;
int rc = whefs_openfs( "my.whefs", &fs, true );
if( whefs_rc.OK != rc ) { ... error ... }

// Open or create a file:
whefs_file * fi = whefs_fopen( fs, "hello.txt", "r+" );
if( ! fi ) { ... error ... }

// Set new content:
whefs_ftrunc( fi, 0 );
whefs_fwrite( fi, "Hello, world!\n", 14 );

// Read it back:
whefs_frewind( fi );
enum { bufSize = 1024 * 2 };
char buf[bufSize];
memset(buf,0,bufSize);
whefs_fread( fi, buf, bufSize );
printf( "Read back: %s", buf );

// Clean up:
whefs_fclose( fi );
whefs_fs_finalize( fs );
```

# Iterating over all used inodes #

We can use `whefs_fs_entry_foreach()` to loop over in-use inodes. The public API does
not provide access to unused inodes (though this may be added someday, to allow clients to write lower-level routines).


Here's an example which implements something like [whefs-ls](WhefsLs.md):

```
#include <wh/whefs/whefs_client_util.h> // the whefs_fs_entry type

/** A private type to hold data we want to keep across iterations
of whefs_fs_entry_foreach().
*/
typedef struct
{
    size_t pos;
} ForEachData;
static const ForEachData ForEachDataInit = {0};

/** Callback for use with whefs_fs_entry_foreach(). */
int my_foreach( whefs_fs * fs, whefs_fs_entry const * ent, void * data )
{
    ForEachData * fd = (ForEachData *)data;
    if( 1 == ++fd->pos )
    {
        printf("%-16s%-16s%-12s%-16s%-16s\n",
               "Node ID:","First block:", "Size:", "Timestamp:","Name:" );
    }
    printf("%-16"WHEFS_ID_TYPE_PFMT"%-16"WHEFS_ID_TYPE_PFMT"%-12u%-16u%s\n",
           ent->inode_id,
           ent->block_id,
           ent->size,
           ent->mtime,
           ent->name.string );

    return whefs_rc.OK;
}

/** Example of how to use whefs_fs_entry_foreach(). */
void do_foreach( whefs_fs * fs )
{
    ForEachData d = ForEachDataInit;
    whefs_fs_entry_foreach( fs, my_foreach, &d );
}
```

Now calling `do_foreach(myFS)` will cause `my_foreach()` to be called for each entry. From there we simply output the info associated with the entry, which might look something like this:

```
Node ID:        First block:    Size:       Timestamp:      Name:           
2               1               40960       1245435778      fileA
3               2               40960       1245435778      fileB
4               3               40960       1245435778      fileC
5               4               40960       1245435778      fileD
6               5               40960       1245435778      fileE
7               6               40960       1245435778      fileF
8               7               40960       1245435778      fileG
9               8               40960       1245435778      fileH
10              9               40960       1245435778      fileI
11              10              40960       1245435778      fileJ
```

The foreach code carefully manages the memory for the name string of each entry, and does not need to allocate any dynamic memory (except possibly as a side-effect of caching, if it's enabled). Thus looping this way has a constant memory cost regardless of the number of inodes. This is in contrast to `whefs_ls()`, which costs memory relative to the number of entries.

# A `cat`-like Program #

Here is the entire source code for [whefs-cat](WhefsCat.md) (or some older version of it, anyway):
```
#include "WHEFSApp.c" /* common code for the whefs-* tools. */
#include <wh/whio/whio_streams.h> /* whio_stream_for_FILE() */

/** App-specific data. */
struct
{
    /** Destination stream for 'catted' data. */
    whio_stream * stdout;
} WhefsCatApp = {
0
};

/** Opens the given file in fs and copies it to dest */
static int cat_file( whefs_fs * fs, char const * name, whio_stream * dest )
{
    whio_stream * str = whefs_stream_open( fs, name, false, false );
    if( ! str )
    {
        APPERR("Could not open file [%s]!\n", name );
        return whefs_rc.IOError; // we have no info as to what went wrong
    }
    whio_stream_copy( str, dest );
    str->api->finalize(str);
    return whefs_rc.OK;
}

/**
   Loops over the WHEFSApp.fe list and sends each named file to
   WhefsCatApp.stdout.
*/
static int cat_doit()
{
    whefs_fs * fs = WHEFSApp.fs;
    int rc = whefs_rc.OK;
    WHEFSApp_fe const * arg = WHEFSApp.fe;
    for( ; arg; arg = arg->next )
    {
        rc = cat_file( fs, arg->name, WhefsCatApp.stdout );
        if( whefs_rc.OK != rc ) break;
    }
    return rc;
}

int main( int argc, char const ** argv )
{
    WHEFSApp.usageText = "vfs_file name(s)-of-in-EFS-files";
    WHEFSApp.helpText =
	"Copies files from an EFS to standard output."
	;
    bool gotHelp = false;
    int rc = WHEFSApp_init( argc, argv, WHEFSApp_OpenRO, &gotHelp, 0 );
    if( (0 != rc) )
    {
	APPMSG("Initialization failed with error code #%d\n", rc);
	return rc;
    }
    if( gotHelp ) return 0;
    if( ! WHEFSApp.fe )
    {
        APPERR("At least one filename argument is required.\n");
        return whefs_rc.ArgError;
    }
    WhefsCatApp.stdout = whio_stream_for_FILE( stdout, true );
    if( ! WhefsCatApp.stdout )
    {
        APPERR("Could not open stdout for writing!\n");
        return whefs_rc.IOError;
    }
    rc = cat_doit();
    WhefsCatApp.stdout->api->finalize( WhefsCatApp.stdout );
    return rc;
}
```

# Sending data to or from `popen()` #

Here's an example of how we can use the whio interface to port data from `popen()` into into an EFS.

```
/** x must be-a (FILE*) opened by popen(). */
static void test_popen_dtor( void * x )
{
    pclose( (FILE*)x );
}
int test_popen( whio_stream * dest )
{
    char const * cmd = "/bin/ls -ltd *.h *.c; echo rc=$?";
    FILE * p = popen(cmd, "r" );
    assert( p && "popen() failed!");
    whio_stream * str = whio_stream_for_FILE( p, false );
    str->client.data = p;
    str->client.dtor = test_popen_dtor;
    int rc = whio_stream_copy( str, dest );
    str->api->finalize(str); // will call test_popen_dtor()
    return rc;
}
```

We can use `whefs_stream_open()` to get a stream handle to a pseudofile, then pass it to `test_popen()` to send the `popen()`-produced data to that stream. Conversely, if we create a writable `FILE` with `popen()` we could export EFS data to it.