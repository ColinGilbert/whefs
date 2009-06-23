#if !defined(WANDERINGHORSE_NET_WHEFS_CACHE_H_INCLUDED)
#define WANDERINGHORSE_NET_WHEFS_CACHE_H_INCLUDED 1
/*
  Author: Stephan Beal (http://wanderinghorse.net/home/stephan/

  License: Public Domain
*/

/**
   This file contains declarations for some of the whefs
   private/internal caching API.

*/
#include <wh/whefs/whefs.h>
#include <wh/whefs/whefs_string.h>
#include <wh/whio/whio_devs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   An internal type for caching strings via a whio_dev memory
   device. This optimizes the storage by using a contiguous
   array for all strings. Unfortunately, each entry in the cache
   must have a set length, however.
*/
struct whefs_string_cache
{
    /**
       Block device manager for writing strings.
    */
    whio_blockdev devBlock;
    /**
       The storage for the strings.
    */
    whio_dev * devMem;
    /**
       Internal buffer used for encoding/decoding.
    */
    //unsigned char buf[WHEFS_MAX_FILENAME_LENGTH+1];
};
typedef struct whefs_string_cache whefs_string_cache;
/** Empty initialization whefs_string_cache object. */
#define whefs_string_cache_init_m { \
        whio_blockdev_init_m/*devBlock*/,                      \
        0/*devMem*/ \
    }

/** Empty initialization whefs_string_cache object. */
extern const whefs_string_cache whefs_string_cache_init;

/**
   Calls whefs_string_cache_cleanup() then deallocates db.
*/
void whefs_string_cache_free( whefs_string_cache * db );

/**
   Allocates and empty-initializes a new whefs_string_cache object,
   the passes ownership of it to the caller. However, it might be
   configured to not use malloc(), so the object must only be
   destroyed by passing it to whefs_string_cache_free().
*/
whefs_string_cache * whefs_string_cache_alloc();
/**
   Deallocates all resources associated with db, but does not free db
   itself. It can be re-used in another call to
   whefs_string_cache_setup() or freed using whefs_string_cache_free().

   Returns whefs_rc.OK on success.
*/
int whefs_string_cache_cleanup( whefs_string_cache * db );

/**
   Truncates the internal string table of db to 0 bytes, potentially freeing
   up memory. db is still a valid/usable object after calling this.

   Returns whefs_rc.OK on success.
*/
int whefs_string_cache_clear_contents( whefs_string_cache * db );

/**
   Sets up db, which must be initialized memory (use
   whefs_string_cache_init or whefs_string_cache_init_m to initialize it).
   db will be able to hold a number (blockCount) of fixed-size blocks of
   blockSize bytes each.

   On success whefs_rc.OK is returned.
*/
int whefs_string_cache_setup( whefs_string_cache * db, whefs_id_type blockCount, whio_size_t blockSize );

/**
   Creates a new whefs_string_cache with the given capacity. On error
   returns 0, else a new object which must be eventually deleted by
   passing it to whefs_string_cache_free().
*/
whefs_string_cache * whefs_string_cache_create( whefs_id_type blockCount, whio_size_t blockSize );
/**
   Sets the given record to the given str value. str may be null or empty but
   may not be longer than db's block size. It is copied, and need not live
   longer than this call.

   Error cases:

   whefs_rc.ArgError = !db or id is out of range for db.

   whefs_rc.RangeError = str is longer than db's block size, as passed
   to whefs_string_cache_create() or whefs_string_cache_setup().

   whio_rc.AllocError = underying cache memory could not be allocated.

   In theory, another whio_rc error may be propagated, but i can't
   think of a reason why it would other than an internal bug.

   This is an O(N) operation, where N is the length of db's blocks, but it involves
   only simple math, memcpy() and memset(), so it's pretty past. If
   the underlying cache must reallocate to grow then it also takes on
   the performance characteristics of realloc().

   @see whefs_string_cache_get()
*/
int whefs_string_cache_set( whefs_string_cache * db, whefs_id_type id, char const * str );

/**
   Gets the string at the given block number.

   On success (i.e. a cache entry was found) it returns a pointer to
   the cached string bytes (null-terminated). They are owned by db and
   may be reallocated or destroyed by any whefs_string_cache_set() calls,
   so they should be copied if they are to be held long-term.

   This is an O(1) operation.

   This function returns 0 (NULL) if the name cannot be read (i.e. it
   has not be cached yet, or caching has been disabled), if the id is
   out of bounds, or a few other theoretical but not likely cases.

   @see whefs_string_cache_set().
*/
char const * whefs_string_cache_get( whefs_string_cache const * db, whefs_id_type id );
/**
   Returns an approximate cost of the memory associated with db, which
   must be a fully-setup object. The real cost is slightly higher than
   the returned value, as there are internal details which this code
   does not have access to, so it cannot measure them.
*/
whio_size_t whefs_string_cache_memcost( whefs_string_cache const * db );

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* WANDERINGHORSE_NET_WHEFS_CACHE_H_INCLUDED */
