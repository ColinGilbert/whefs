

See also: [BuildAndInstall](BuildAndInstall.md)

# whefs Amalgamation Builds #

To simplify integration with other software, whefs's makefile provides an "amalgamation build." This copies all header files needed by whefs into a single mega-header, and all source files into a single mega-source file. Those two files can then be dropped into arbitrary client trees.

To build the amalgamation, simply do the following from the whefs source tree:

```
~> make amal
# Or:
~> ./createAmalgamation.sh
```

That will create the files <tt>whefs_amalgamation.h</tt> and <tt>whefs_amalgamation.c</tt>, which can be compiled into library form or embedded directly in a client source tree. The [sqlite project](http://sqlite.org) uses this technique as well, and they claim that via this approach the overall performance of the library can be improved by 5-10% because the compiler can perform certain extra optimizations when all definitions are in the same source file.

Creating the amalgamation requires some (automated) tweaks to the source and header files (e.g. removing the #includes for the related headers). To see how it's done, look at the the script [createAmalgamation.sh](http://code.google.com/p/whefs/source/browse/trunk/createAmalgamation.sh).

# Using the amalgamation from client code #

The amalgamation is intended to be compiled and linked as part of a client project. To use it from the client code, the user must `#include "whefs_amalgamation.h"`. If `whefs_amalgamation.c` is not compiled directly in with the client code then the client code will of course need to link to either `whefs_amalgamation.o` or a library containing the code from `whefs_amalgamation.c`.

# Compiling the amalgamation #

Compilation of the library is <em>much</em> faster when compiling just whefs\_amalgamation.c, as opposed to the individual source files:

```
# whefs as of 20090609:
# with gcc:
stephan@jareth:~/cvs/fossil/whefs$ time gcc -c -std=c99 whefs_amalgamation.c
real	0m0.563s
user	0m0.516s
sys	0m0.048s
# with tcc:
stephan@jareth:~/cvs/fossil/whefs$ time tcc -c whefs_amalgamation.c  
real	0m0.035s
user	0m0.020s
sys	0m0.012s
```

(Man, [tcc](http://bellard.org/tcc/) is <em>fast</em>!)

Building the library via the individual files (via Make), minus the utility apps, takes significantly longer:

```
# with gcc:
stephan@jareth:~/cvs/fossil/whefs$ time make libs &>/dev/null

real	0m2.316s
user	0m1.836s
sys	0m0.472s

# with tcc:
stephan@jareth:~/cvs/fossil/whefs$ time make libs TCC=1 &>/dev/null

real	0m0.960s
user	0m0.724s
sys	0m0.216s
```

If the size of the generated amalgamation header file bothers you, you can shrink it considerably by stripping out the API documentation. Here is a Perl script which can be used to do that:

```
#!/usr/bin/env perl
# Strips C-style comments.
# Source: http://www.perl.com/doc/FAQs/FAQ/oldfaq-html/Q4.27.html
$/ = undef;
$_ = <>; 

s#/\*[^*]*\*+([^/*][^*]*\*+)*/|([^/"']*("[^"\\]*(\\[\d\D][^"\\]*)*"[^/"']*|'[^'\\]*(\\[\d\D][^'\\]*)*'[^/"']*|/+[^*/][^/"']*)*)#$2#g;
print;
```

That said, stripping the API docs, even though they are large, does not have a consistent measurable effect on the compilation speed, and they should probably only be stripped when space is at a premium. _It also incidentally strips the license text, which might make some lawyers nervous._

By default the library builds with support for internal debugging messages. This slows down the library a bit but can be very useful in tracking down errors. To deactivate the debugging code you must simply define the standard (or pseudostandard) `NDEBUG` macro (e.g. pass `-DNDEBUG=1` to your preprocessor).


# Using tcc with the amalgamation #

[TinyCC](http://bellard.org/tcc/) is a <em>really</em> fast C compiler with some interesting features. Namely, it's so fast that it can be used to run C code as if it were a shell-style script. With a copy of the whefs amalgamation it is trivial to write "scripts" in C which use whefs, without ever actually having to have libwhefs installed. tcc will compile it on the fly (and will do so so quickly that you won't even have time to time it). Here's an example, which uses [Whefs2c](Whefs2c.md) input for an in-memory EFS:

```
#!/usr/bin/tcc -run
#include "whefs_amalgamation.c" // .c, not .h!

#define MARKER if(1) printf("MARKER: %s:%d:%s():\n",__FILE__,__LINE__,__func__); if(1) printf

/* StaticWHEFS.c is created using whefs2c. */
#include "StaticWHEFS.c"

int main( int argc, char const ** argv )
{
    // Open the EFS:
    whefs_fs * fs = StaticWHEFS_whefs_open( true );
    assert( fs && "openfs failed!" );

    // Do an "ls":
    int rc = whefs_rc.OK;
    whefs_id_type lscount = 0;
    char const * lspat = "*";
    whefs_string * ls = whefs_ls( fs, lspat, &lscount );
    whefs_string * head = ls;
    MARKER("VFS File list matching '%s':\n",lspat);
    char const * openMe = 0;
    while( ls )
    {
	printf("\t%s\n", ls->string );
	if( ! openMe ) openMe = ls->string;
	ls = ls->next;
    }
    // Open the first file we found:
    whefs_file * fi =  whefs_fopen( fs, openMe, "r+" );
    whefs_string_finalize( head, true );
    assert( fi && "fopen failed!" );

    // Manipulate that file using the whio_dev API:
    whio_dev * fdev = whefs_fdev( fi );
    fdev->api->seek( fdev, 30, SEEK_SET );
    char const * toWrite = "[fdev was here]";
    fdev->api->write( fdev, toWrite, strlen(toWrite) );
    rc = whefs_file_set_name( fi, "renamed!" );
    printf("Attempted to rename file. RC=%d\n", rc );
    whefs_fclose( fi );

    // Re-do our "ls" to see if the rename worked:
    head = ls = whefs_ls( fs, lspat, &lscount );
    MARKER("VFS File list matching '%s':\n",lspat);
    while( ls )
    {
	printf("\t%s\n", ls->string );
	ls = ls->next;
    }
    whefs_string_finalize( head, true );

    // Dump the in-memory EFS to a file:
    toWrite = "StaticWHEFS-export.whefs";
    rc = whefs_fs_dump_to_filename( fs, toWrite );
    printf("Dumping VFS to file [%s]. RC=%d\n", toWrite, rc );

    // Free up whefs-internal memory associated with StaticEFS
    // but not the EFS storage itself (which is static memory):
    StaticWHEFS_whefs_finalize();
    puts("Done!");
    return 0;
}
```

Now let's time it:
```
stephan@jareth:~/cvs/fossil/whefs$ time ./X.c
MARKER: ./X.c:47:main():
VFS File list matching '*':
	whefs.h
	whefs.c
	whio_dev_subdev.c
Attempted to rename file. RC=0
MARKER: ./X.c:68:main():
VFS File list matching '*':
	renamed!
	whefs.c
	whio_dev_subdev.c
Dumping VFS to file [StaticWHEFS-export.whefs]. RC=0
Done!

real	0m0.107s
user	0m0.088s
sys	0m0.000s
```

The vast majority of that time was spent processing StaticWHEFS.c and exporting the VFS to a file (see the call to whefs\_fs\_dump\_to\_filename()). In this case the EFS is 119kb and the static EFS C code is 725kb. If we replace that .c file with a smaller one (406k generated from a 66k VFS), the time changes noticeably:

```
stephan@jareth:~/cvs/fossil/whefs$ time ./X.c
...
Done!

real	0m0.078s
user	0m0.056s
sys	0m0.008s
```

That's the time it takes for tcc to compile, link, <em>and</em> execute that code, totaling about 840kb of .c and .h files in this case, on a standard 2.1GHz PC. With that kind of speed, who needs pre-compiled libraries?