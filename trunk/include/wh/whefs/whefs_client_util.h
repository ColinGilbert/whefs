#if !defined(WANDERINGHORSE_NET_WHEFS_CLIENT_UTIL_INCLUDED)
#define WANDERINGHORSE_NET_WHEFS_CLIENT_UTIL_INCLUDED 1

#include "whefs.h"
#include "whefs_string.h"

/**
   Like the 'ls' Unix program, this function fetches a list of
   filenames matching the given pattern. If no matches are found, null
   is returned, otherwise a string list is returned and count (if it
   is not null) is set to the number of items in the list.

   A pattern of null or an empty string is equivalent to a pattern of
   "*" (but much faster, since it doesn't need to be compared).

   The returned list must be freed by calling whefs_string_finalize()
   and passing true as the second parameter.

   Example usage:

   @code
    whefs_string * ls = whefs_ls( myFS, "*.c", 0 );
    whefs_string * head = ls;
    while( ls )
    {
        printf("%s\n", ls->string );
        ls = ls->next;
    }
    whefs_string_finalize( head, true );
   @endcode

   BUG:

   This routine has no way of reporting an error. If some sort of I/O
   error happens after an entry has been fetched, the entries matched
   so far are returned but there is no way of knowing how many entries
   would have been returned if all could have been read. That said, if
   the first entry is read successfully, it is "reasonable to assume"
   that the remaining entries could be read as well. In fact, since
   only "used" entries are considered for matching, any entries
   returned here must have been read previously (to build the
   used-items cache), and only a true I/O error (or corruption of the
   VFS container) should cause a failure here.
*/
whefs_string * whefs_ls( struct whefs_fs * fs, char const * pattern, whefs_id_type * count );


/**
   Imports all contents from src into the VFS pseudofile named fname.
   If overwrite is true, any existing entry will be overwritten,
   otherwise an existing file with that name will cause this function
   to return whefs_rc.AccessError.

   Ownership of src is not changed. This routine will re-set src's
   cursor to its pre-call position before returning.

   On success, whefs_rc.OK is returned.

   On error, any number of different codes might be returned, both
   from whefs_rc and whio_rc (some of which overlap or conflict).
   If the import fails and the entry did not exist before the import
   started, it is removed from the filesystem. If it did exist then
   exactly what happens next depends on a few factors:

   - If the import failed because the existing file could not be
   expanded to its new size then it is kept intact, with its old
   size. This step happens first, before any writing is done.

   - If the import failed during the copy process then the destination
   file is left in an undefined state and it should probably be
   unlinked.

*/
int whefs_import( whefs_fs * fs, whio_dev * src, char const * fname, bool overwrite );


/**
   A type for reporting certain vfs metrics.
*/
typedef struct whefs_fs_stats
{
    /**
       Size of the vfs container.
    */
    size_t size;
    /**
       Number of uses inodes.
    */
    size_t used_inodes;
    /**
       Number of used blocks.
    */
    size_t used_blocks;
    /**
       Number of used bytes (not necessarily whole blocks).
    */
    size_t used_bytes;
} whefs_fs_stats;

/**
   Not yet implemented.

   Calculates some statistics for fs and returns them via
   the st parameter.

   Returns whefs_rc.OK on success. On error some other value
   is returned and st's contents are in an undefined state.
*/
int whefs_fs_stats_get( whefs_fs * fs, whefs_fs_stats * st );

/**
   Test/debug routine which inserts some entries into the inode table.
*/
int whefs_test_insert_dummy_files( whefs_fs * fs );

#endif // WANDERINGHORSE_NET_WHEFS_CLIENT_UTIL_INCLUDED
