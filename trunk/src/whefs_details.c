#if !defined(WANDERINGHORSE_NET_WHEFS_DETAILS_C_INCLUDED)
#define WANDERINGHORSE_NET_WHEFS_DETAILS_C_INCLUDED 1
#if ! defined __STDC_FORMAT_MACROS
#  define __STDC_FORMAT_MACROS 1
#endif

/*
  Author: Stephan Beal (http://wanderinghorse.net/home/stephan/

  License: Public Domain
*/

/** @file whefs_details.c

  This file contains private implementation details for whefs which
  are shared amongst implementation files but which are NOT part of
  the public API. It is intended to be included into the C files which
  need it, like a rather, rather than compiled by itself. The various extern
  objects/funcs declared here are likely implemented in whefs.c, but may be
  in other source files.
*/

#include <assert.h>
#include <wh/whefs/whefs.h>
#include "whefs_inode.h"
#include "whdbg.h"
#include <wh/whio/whio_devs.h>
#include "whbits.h"
#include "whefs_encode.h"
#include "whefs_hash.h"
#include "whefs_cache.h"

#if WHEFS_ENABLE_THREADS
#include <pthread.h>
#endif

/** @enum whefs_debug_flags
   Debugging flags for use with whdbg() and friends.
*/
enum whefs_debug_flags {
WHEFS_DBG_F_ERROR = WHDBG_ERROR,
WHEFS_DBG_F_WARNING = WHDBG_WARNING,
WHEFS_DBG_F_FIXME = WHDBG_FIXME,
WHEFS_DBG_F_NYI = WHDBG_NYI,
WHEFS_DBG_F_FYI = WHDBG_FYI,
WHEFS_DBG_F_ALWAYS = WHDBG_ALWAYS,
WHEFS_DBG_F_VERBOSE = WHDBG_VERBOSE,
WHEFS_DBG_F_mymask = 0x0f000000,
WHEFS_DBG_F_LOCK = WHEFS_DBG_F_mymask & 0x01000000,
WHEFS_DBG_F_CACHE = WHEFS_DBG_F_mymask & 0x02000000,
WHEFS_DBG_F_DEFAULTS_CLIENT = WHEFS_DBG_F_WARNING | WHEFS_DBG_F_ERROR | WHDBG_NYI,
WHEFS_DBG_F_DEFAULTS_HACKER = WHEFS_DBG_F_DEFAULTS_CLIENT | WHDBG_FIXME | WHEFS_DBG_F_FYI | WHDBG_VERBOSE, // | WHEFS_DBG_F_CACHE, // | WHEFS_DBG_F_LOCK,

#if defined(NDEBUG)
WHEFS_DBG_F_DEFAULT = 0 /* they'll be if(0)'d out in this case, anyway. */
#else
WHEFS_DBG_F_DEFAULT = WHEFS_DBG_F_DEFAULTS_CLIENT
//WHEFS_DBG_F_DEFAULTS_HACKER
#endif
};

#define WHEFS_DBG WHDBG(WHEFS_DBG_F_ALWAYS)
#define WHEFS_DBG_ERR WHDBG(WHEFS_DBG_F_ERROR)
#define WHEFS_DBG_WARN WHDBG(WHEFS_DBG_F_WARNING)
#define WHEFS_FIXME WHDBG(WHEFS_DBG_F_FIXME)
#define WHEFS_DBG_LOCK WHDBG(WHEFS_DBG_F_LOCK)
#define WHEFS_DBG_CACHE WHDBG(WHEFS_DBG_F_CACHE)
#define WHEFS_VERBOSE WHDBG(WHEFS_DBG_F_VERBOSE)
#define WHEFS_NYI WHDBG(WHEFS_DBG_F_NYI)("Not yet implemented!");
#define WHEFS_DBG_FYI WHDBG(WHEFS_DBG_F_FYI)

/** @enum whefs_flags

Flags used for various purposes within the VFS internals.

MAINTENANCE REMINDER: whefs_block::flags is uint8, so the flags
used with that type need to stay smal.
*/
typedef enum {
/**
   Mark unused inodes.
 */
WHEFS_FLAG_Unused = 0x00,
/**
   Mark used inodes/blocks.
 */
WHEFS_FLAG_Used = 0x01,
/**
   Mark an fs or inode object as readable.
*/
WHEFS_FLAG_Read = 0x02,
/**
   Mark an fs or inode object as read/write.
 */
WHEFS_FLAG_Write = 0x04,
WHEFS_FLAG_ReadWrite = WHEFS_FLAG_Write | WHEFS_FLAG_Read,
WHEFS_FLAG_Opened = 0x8,
//WHEFS_FLAG_FS_AutoExpand = 0x0010,
/**
   Mark error state for whefs_file objects.
*/
WHEFS_FLAG_FileError = 0x0100
} whefs_flags;

/**
   Array index symbols for whefs_fs::offsets.
*/
enum whefs_fs_offsets {
WHEFS_OFF_SIZE = 0,
WHEFS_OFF_CORE_MAGIC,
WHEFS_OFF_CLIENT_MAGIC,
WHEFS_OFF_OPTIONS,
WHEFS_OFF_INODE_NAMES,
WHEFS_OFF_INODES_NO_STR,
WHEFS_OFF_BLOCK_TABLE,
WHEFS_OFF_BLOCKS,
WHEFS_OFF_EOF,
WHEFS_OFF_COUNT /* must be the last entry! */
};
/**
   Array index symbols for whefs_fs::sizes.
*/
enum whefs_fs_sizes {
WHEFS_SZ_INODE_W_STR = 0,
WHEFS_SZ_INODE_NO_STR,
WHEFS_SZ_INODE_NAME,
WHEFS_SZ_BLOCK,
WHEFS_SZ_OPTIONS,
WHEFS_SZ_COUNT /* must be the last entry! */
};


/**
   Main filesystem structure.
*/
struct whefs_fs
{
    /**
       General flags.
    */
    unsigned int flags;
    /**
       Error code.
     */
    unsigned int err;
    /**
       Stores file position offsets for commonly-used
       reference points within a vfs.

       The array indexes should be from the whefs_fs_offsets enum.
    */
    uint32_t offsets[WHEFS_OFF_COUNT];
    /**
       Stores sizes of commonly used data structures.

       The array indexes should be from the whefs_fs_sizes enum.
    */
    uint32_t sizes[WHEFS_SZ_COUNT];
    /**
       Underlying i/o device for the backing store.
    */
    whio_dev * dev;
    /**
       If true, this object owns the dev member and will destroy
       it when the time comes.
    */
    bool ownsDev;
    /**
       The expected size of the container file. It must not change
       after initialization of the vfs state (i.e. after opening or
       mkfs).

       This really isn't needed, and is more of a sanity checking tool
       than anything. It has come in quite handy for that purpose.
    */
    uint32_t filesize;
    /**
       All "opened" inodes are store in this linked list.
    */
    whefs_inode_list * opened_nodes;
    /**
       IFF the fs thinks that it is using file-based storage it may
       try to enable some locking features. This is the file number of
       the underlying file. This relies on whio_dev::ioctl() for the
       underlying device supporting the whio_dev_ioctl_FILE_fd ioctl
       command.
    */
    int fileno;

    /**
       Bitsets for the minimal internal caching we do.
     */
    struct _bits
    {
	/**
	   bitset for marking in-use inodes.

	   Bits:
	     In-use: offset == inode.id
	*/
	whbits i;
	/**
	   bitset for marking in-use blocks.

	   Bits:
	     In-use: offset == block.id
	*/
	whbits b;
        /** Whether or not the inode bitset has been loaded. */
        bool i_loaded;
        /** Whether or not the block bitset has been loaded. */
        bool b_loaded;
    } bits;
    struct _hints
    {
	/**
	   This is used as a starting hint for whefs_pos_next_free().
	   Profiling showed that that function (due to its horribly
	   linear nature) took the single biggest time slot for some
	   use cases.  By using this hint we reduce the time (in one
	   test app) from 6% of the runtime to 0.5% of the runtime.

	   Each time whefs_block_next_free() finds a block, it sets
	   this variable to the offset of that block, plus 1. The next
	   time it is run, it starts at that offset. If a block is
	   wiped (via whefs_block_wipe()) and its ID is lower than
	   this value then this variable is set to that ID, such that
	   the next call to whefs_block_next_free() will start at that
	   block (and hit it on the first attempt). That transforms
	   whefs_block_next_free() into a linear operation only the
	   first time it is run (when this var is at its default
	   starting point and we may have used blocks we don't know
	   about).
	*/
	whefs_id_type unused_block_start;

	/**
	   This is the inode counterpart of unused_block_start, and is
	   used by whefs_inode_next_free() to speed up searching in
	   the same way that unused_block_start is used for blocks.
	*/
	whefs_id_type unused_inode_start;
    } hints;

    /**
       Client-configurable vfs options. Except in some very controlled
       circumstances, these must not change after initialization of
       the vfs state or the fs will become corrupted.
    */
    whefs_fs_options options;
    /**
       Objects used to partition the underlying storage device
       into more manageable units. Each one is responsible for
       a given table of fixed-length records.
    */
    struct fences
    {
	/** Fencer for the inode names table. */
	whio_blockdev s;
	/** Fencer for the inode metadata table. */
	whio_blockdev i;
	/** Fencer for the block metadata table. */
	//whio_blockdev b;
    } fences;
    /** Holder for threading-related data. */
    struct thread_info
    {
#if WHEFS_ENABLE_THREADS
        int placeholder;
#else
        int placeholder;
#endif
    } threads;
    struct _caches
    {
        whefs_hashid_list * hashes;
        whefs_hashval_type (*hashfunc)( char const * vstr);
        whefs_string_cache strings;
    } cache;
};

/** Empty initialization object. */
extern const whefs_fs whefs_fs_init;


/** inode/block in-use caching... */
#if !defined(WHEFS_FS_BITSET_CACHE_ENABLED)
/** @def WHEFS_FS_BITSET_CACHE_ENABLED

TODO: re-evaluate the real cost of this cache. Memory is very low, but
profiling has, in some cases, implied that it costs us more performance
than we lose when it's disabled.

If WHEFS_FS_BITSET_CACHE_ENABLED is true then the VFS caches (using a
bitset) whether or not any given inode or block is marked as used.
This speeds up some operations dramatically but costs malloced memory:
1 bit per inode plus 1 bit per block plus 1 byte.


This approach to caching is going to Cause Grief (or at least
Discomfort) when dealing with multi-app concurrency issues, as we
cannot keep it in sync across multiple applications.
*/
#  define WHEFS_FS_BITSET_CACHE_ENABLED (1)
#endif
#if WHEFS_FS_BITSET_CACHE_ENABLED
#define WHEFS_CACHE_ASSERT(NID) (assert(0 && ("bit #" # NID " out of range! Debug to here and look for fs->bits.{i,b}.sz_bits and friends")),0)
#define WHEFS_ICACHE_PRECHECK(FS,NID) (FS && FS->bits.i.bytes && ((FS->bits.i.sz_bits > NID)||WHEFS_CACHE_ASSERT(NID)))
#define WHEFS_ICACHE_SET_USED(FS,NID) (WHEFS_ICACHE_PRECHECK(FS,NID) && (WHBITS_SET(&FS->bits.i,NID)))
#define WHEFS_ICACHE_UNSET_USED(FS,NID) (WHEFS_ICACHE_PRECHECK(FS,NID) && (WHBITS_UNSET(&FS->bits.i,NID)))
#define WHEFS_ICACHE_IS_USED(FS,NID) (WHEFS_ICACHE_PRECHECK(FS,NID) && (WHBITS_GET(&FS->bits.i,NID)))
#else
#define WHEFS_ICACHE_SET_USED(FS,NID)
#define WHEFS_ICACHE_UNSET_USED(FS,NID)
#define WHEFS_ICACHE_IS_USED(FS,NID)
#endif

#if WHEFS_FS_BITSET_CACHE_ENABLED
#define WHEFS_BCACHE_PRECHECK(FS,NID) (FS &&  FS->bits.b.bytes && ((FS->bits.b.sz_bits > NID)||WHEFS_CACHE_ASSERT(NID)))
#define WHEFS_BCACHE_SET_USED(FS,NID) (WHEFS_BCACHE_PRECHECK(FS,NID) && WHBITS_SET(&FS->bits.b,NID))
#define WHEFS_BCACHE_UNSET_USED(FS,NID) (WHEFS_BCACHE_PRECHECK(FS,NID) && WHBITS_UNSET(&FS->bits.b,NID))
#define WHEFS_BCACHE_IS_USED(FS,NID) (WHEFS_BCACHE_PRECHECK(FS,NID) && WHBITS_GET(&FS->bits.b,NID))
#else
#define WHEFS_BCACHE_SET_USED(FS,NID)
#define WHEFS_BCACHE_UNSET_USED(FS,NID)
#define WHEFS_BCACHE_IS_USED(FS,NID) THIS_MUST_BE_IN_A_BLOCK_GUARDED_BY__WHEFS_FS_BITSET_CACHE_ENABLED
#endif


#define WHEFS_FS_ISRO(FS) ((FS) && ((FS)->flags & WHEFS_FLAG_Read))
#define WHEFS_FS_ISRW(FS) ((FS) && ((FS)->flags & WHEFS_FLAG_Write))
//#define WHEFS_FS_ISERR(F) ((F) && ((F)->err != WHEFS_ERR_None))

/**
   Equivalent to calling whio_dev::read() on fs's underlying i/o
   device.
*/
whio_size_t whefs_fs_read( whefs_fs * fs, void * dest, whio_size_t n );
/**
   Equivalent to calling whio_dev::write() on fs's underlying i/o
   device.
*/
whio_size_t whefs_fs_write( whefs_fs * fs, void const * src, whio_size_t n );
/**
   Equivalent to calling whio_dev::seek() on fs's underlying i/o
   device.
*/
whio_size_t whefs_fs_seek( whefs_fs * fs, off_t offset, int whence );
/**
   Equivalent to calling whio_dev::tell() on fs's underlying i/o
   device.
*/
whio_size_t whefs_fs_tell( whefs_fs * fs );

/**
   Tries to read the metadata associated with id, storing it in bl,
   which must not be 0. On success, bl is populated with the metadata
   and whefs_rc.OK is returned. On error, some other value is returned
   and bl may be in an undefined state.
*/
int whefs_block_read( whefs_fs * fs, whefs_id_type id, whefs_block * bl );

/**
   Returns the on-disk size of a data block, including the header, for
   the given options, or 0 if !opt.
*/
whio_size_t whefs_fs_sizeof_block( whefs_fs_options const * opt );

/**
   Reads the block following bl.  bl must be a valid block object
   (with a valid ID) and nextBlock must be valid memory (which will be
   overwritten). If bl has a next block, nextBlock is set to the
   contents of that block. bl and nextBlock may point to the same
   object.

   On successs, whefs_rc.OK is returned and nextBlock is populated.

   On error nextBlock is left in an undefined state and one of the
   following is returned:

   - !fs or !bl or !nextBlock or bl is not a valid block:
     whefs_rc.ArgError.

   - bl has no next block then whefs_rc.RangeError is returned.

   - If the read of the next block fails, the return value is that
   of whefs_block_read().
*/
int whefs_block_read_next( whefs_fs * fs, whefs_block const * bl, whefs_block * nextBlock );


/**
   fs is repositioned to the start of bl's on-disk position and bl's
   metadata is written to disk at that position.

   bl->id must be valid for fs.

   Returns whefs_rc.OK on success, else some other value.
*/
int whefs_block_flush( whefs_fs * fs, whefs_block const * bl );


/**
   Searches for the next free block and populates target with its
   metadata. On success, whefs_rc.OK is returned. If markUsed is true
   and a free block is found, the block is marked as being used,
   otherwise it is not marked (in which case a future call to this
   routine may return that same block).
*/
int whefs_block_next_free( whefs_fs * restrict fs, whefs_block * restrict tgt, bool markUsed );

/**
   Zeroes out parts of the given data block. Unlike most routines,
   which require only that bl->id is valid, bl must be fully populated
   for this to work.

   If data is true then the block's on-disk data bytes are zeroed out.

   If meta is true then the metadata associated with the block (block
   flags and next block ID) are reset to their default state.

   If deep is true then all linked blocks (starting at bl->next_block)
   are also cleaned with the same parameters.

   WARNING:

   If (meta && !deep), lost blocks will result if bl has child blocks! 
   Lost blocks are are marked used but are no longer reachable via a
   block chain, and thus cannot be directly recovered without manually
   fixing the block relationships.

   Returns whefs_rc.OK on success.
*/
int whefs_block_wipe( whefs_fs * fs, whefs_block * bl, bool data, bool meta, bool deep );

/**
   Fills all data bytes of the given block with 0, starting at the given starting
   position. If startPos is greater or equal to fs's block size, whefs_rc.RangeError
   is returned.

   On success, whefs_rc.OK is returned.
*/
int whefs_block_wipe_data( whefs_fs * fs, whefs_block const * bl, whio_size_t startPos );


/**
   Returns the on-disk position of the block with the given id,. fs
   must be opened and initialized. On error (!fs or !b, or b->id is
   out of range), 0 is returned.
*/
whio_size_t whefs_block_id_pos( whefs_fs const * fs, whefs_id_type id );

/**
   Returns the on-disk position of bl's data block (the point just
   past the block header), or 0 on error (!bl, !fs, or !bl->id).
*/
whio_size_t whefs_block_data_pos( whefs_fs const * fs, whefs_block const * bl );

/**
   Seeks to the given block's on-disk position. Returns whefs_rc.OK
   on success.
*/
int whefs_block_seek( whefs_fs const * fs, whefs_block const * bl );
/**
   Seeks to the on-disk position of the given block ID. Returns
   whefs_rc.OK on success.
*/
int whefs_block_id_seek( whefs_fs const * fs, whefs_id_type id );

/**
   Seeks to the on-disk position of the given block's data
   segment. On success it returns whefs_rc.OK and updates tgt
   (if tgt is not null).
*/
int whefs_block_seek_data( whefs_fs const * fs, whefs_block const * bl, whio_size_t * tgt );

/**
   Identical to whefs_block_seek_data() but takes a block ID instead
   of a whefs_block parameter.
*/
int whefs_block_id_seek_data( whefs_fs const * fs, whefs_id_type id, whio_size_t * tgt );


/** @def whefs_block_id_is_valid_m

   Internal implementation of the more public whefs_block_id_is_valid().

   Evaluates to true if block id BLID is valid for the given fs. That
   is, it has a non-zero id in a range legal for the given fs object.
 */
#define whefs_block_id_is_valid_m(FS,BLID) ((FS) && (BLID) && ((BLID) <= (FS)->options.block_count))
/** @def whefs_block_is_valid_m

   Internal implementation of the more public whefs_block_is_valid().

   Evaluates to true if block BL (a const pointer to a whefs_block) is
   valid for the given fs. That is, it has a non-zero id in a range
   legal for the given fs object.
 */
#define whefs_block_is_valid_m(FS,BL) ((BL) ? (whefs_block_id_is_valid_m((FS),(BL)->id)) : false)

#if WHEFS_MACROIZE_SMALL_CHECKS
#  define whefs_block_id_is_valid(FS,BL) whefs_block_id_is_valid_m(FS,BL)
#  define whefs_block_is_valid(FS,BL) whefs_block_is_valid_m(FS,BL)
#else
/**
   Returns true if blockID is "valid" for the given fs. That is, has a
   non-zero id in a range legal for the given fs object.
*/
bool whefs_block_id_is_valid( whefs_fs const * fs, whefs_id_type blockID );
/**
   Equivalent to whefs_block_id_is_valid(fs,bl->id). Returns false if
   !bl.
*/
bool whefs_block_is_valid( whefs_fs const * fs, whefs_block const * bl );
#endif


#if 0 // unused code
/**
   Appends a block to a chain of blocks.

   If bl is a valid block:

   - Find the last block in bl's chain. Call that B.
   - Append a new block after B. Call that B2.
   - Update B to point to B2.
   - Flush blocks to disk
   - Assign tgt to B2.

   If bl is not null and bl->next_block is not 0 then
   whefs_rc.ArgError is returned to avoid orphaning bl's current next
   block.

   If bl is not a valid block (is null or id is out of range)

   - Find next free block and mark it as used.
   - Flush block to disk.
   - Assign tgt to that block.

   On success returns whefs_rc.OK, else some other value is returned
   and tgt is left in an undefined state.

   It is theoretically OK for bl and tgt to be the same underlying
   object.
*/
int whefs_block_append( whefs_fs * fs, whefs_block const * bl, whefs_block * tgt );
#endif

/**
   This updates the internal used-blocks cache (if enabled) and search
   hints for the block with id bl->id.

   WARNING: as profiling shows that block validation check here is, over time,
   expensive, neither fs nor bl nor bl->id are verified. It is up to the caller
   to ensure that bl has a valid block ID for the given filesystem.
*/
void whefs_block_update_used( whefs_fs * fs, whefs_block const * bl );


/** @struct whefs_file

   Internal details for whefs_fs pseudofiles. They are created using
   whefs_fopen() and closed with whefs_fclose().
*/
struct whefs_file
{
    /**
       Underlying vfs.
    */
    whefs_fs * fs;
    /**
       Internal flags (RW/RO, etc)
    */
    uint32_t flags;
    /**
       i/o device specialized for whefs_inode access.
    */
    whio_dev * dev;
    /**
       inode ID.
    */
    whefs_id_type inode;
    /** Unfortunate. Should go away. */
    whefs_string name;
};
/** Empty initialization object. */
extern const whefs_file whefs_file_init;

/**
   Writes an object of type whefs_id_type to the given
   device. Returns the number of bytes written on succes, which
   will be whefs_sizeof_encoded_id_type.
 */
size_t whefs_dev_id_encode( whio_dev * dev, whefs_id_type v );

/**
   Identical to whefs_dev_id_encode(), but writes to a char array,
   which must be at least whefs_sizeof_encoded_id_type bytes long.

   Returns whefs_sizeof_encoded_id_type on success.
*/
size_t whefs_id_encode( unsigned char * dest, whefs_id_type v );

/**
   Tries to read a whefs_id_type object, which must have been encoded
   using whefs_id_encode(), from the given device. On success,
   whefs_rc.OK is returned and v (which may not be null) is updated to
   that value.
 */
int whefs_dev_id_decode( whio_dev * dev, whefs_id_type * v );

/**
   Identical to whefs_dev_id_decode(), but reads from a char array, which
   must be at least whefs_sizeof_encoded_id_type bytes long.
*/
int whefs_id_decode( unsigned char const * src, whefs_id_type * v );

/**
   If the library is not built with fcntl() support or fs's backing
   store does not report a file descriptor (via whio_dev_ioctl()) then
   this function does nothing and returns whefs_rc.UnsupportedError.
   Otherwise...

   It requests (and waits for) an advisory file lock on fs's
   underlying storage. If writeLock is true it requests an exclusive
   write lock, otherwise a read-only lock. The start, whence, and len
   parameters are described in the man page for fcntl() - their
   semantics are identical here.

   On success it returns 0 on success, on error some value defined by
   fcntl() (except as described above).

   @see whefs_fs_unlock()
   @see whefs_fs_lock_range()
   @see whefs_fs_unlock_range()
*/
int whefs_fs_lock( whefs_fs * fs, bool writeLock, off_t start, int whence, off_t len );

/**
   Unlocks a lock set via whefs_fs_lock(). If locking support is not compiled in
   or fs's data store does not report a file handle (see whefs_fs_lock()) then
   whefs_rc.UnsupportedError is returned, otherwise it returns 0 on success and
   some value defined by fcntl() on error.

   @see whefs_fs_lock()
   @see whefs_fs_lock_range()
   @see whefs_fs_unlock_range()
*/
int whefs_fs_unlock( whefs_fs * fs, off_t start, int whence, off_t len );

/** @struct whefs_fs_range_locker

   A type for holding parameters for a disk (un)locking request. See
   whefs_fs_lock_range() for details.

   @see whefs_fs_lock_range()
   @see whefs_fs_unlock_range()
*/
struct whefs_fs_range_locker
{
    /**
       Start position of lock/unlock request, relative
       to the whence member.
    */
    off_t start;
    /**
       Relative starting point of the lock/unlock request. Must be one
       of SEEK_SET, SEEK_CUR, or SEEK_END.
    */
    int whence;
    /**
       Length of the byte range for the lock/unlock request.
    */
    off_t len;
};
typedef struct whefs_fs_range_locker whefs_fs_range_locker;

/**
   Equivalent to whefs_fs_lock() but takes its arguments packages in an
   object (which may not be null). The reason for this overload is to simplify
   the implementation of matching lock/unlock calls by storing the lock
   arguments in an object which can be passed to both operations.

   @see whefs_fs_lock_range()
   @see whefs_fs_unlock()
   @see whefs_fs_unlock_range()
*/
int whefs_fs_lock_range( whefs_fs * fs, bool writeLock, whefs_fs_range_locker const * range );

/**
   Equivalent to whefs_fs_lock() but takes its arguments as an object (which
   may not be null). See whefs_fs_lock_range() for why.

   @see whefs_fs_lock()
   @see whefs_fs_unlock()
   @see whefs_fs_lock_range()
*/
int whefs_fs_unlock_range( whefs_fs * fs, whefs_fs_range_locker const * range );

/**
   Writes the name for the given inode ID in the names table.
   Only the first fs->options.filename_length bytes of name
   are used, or until the first null. The table's entry is
   padded with nulls. Returns whefs_rc.OK on success.

   The library uses uint16_t to store the string length, but doesn't
   allow strings longer thant WHEFS_MAX_FILENAME_LENGTH. If name is
   longer than fs->options.filename_length, whefs_rc.RangeError is
   returned.

   Returns whefs_rc.OK on success. Errors
   include: !name, id is not valid for fs, or an i/o error.
*/
int whefs_fs_name_write( whefs_fs * fs, whefs_id_type id, char const * name );

/**
   Generates a hashcode for the given name and identifies the inode
   with the given id with that hashcode. On success it returns whefs_rc.OK.

   Error cases:

   - If fs or name are null, or !*name: whefs_rc.ArgError

   - Allocation of the cache fails: whefs_rc.AllocError

   - An entry already exists for the same hash code: whefs_rc.InternalError. If
   this ever actually happens, i'll try other hashes or build a better hashtable.


   This routine is much faster if the cache is sorted before calling
   this.  If you're going to add lots of items at once, do so with the
   lower-level cache list API, then sort it, then mark is as unsorted (or sort it).
*/
int whefs_inode_hash_cache( whefs_fs * fs, whefs_id_type id, char const * name );

/**
   If a cached entry is found with the same hashcode as name, the id
   of that inode is returned, else 0.
*/
whefs_id_type whefs_inode_hash_cache_search_id(whefs_fs * fs, char const * name );
/**
   If a cached entry is found with the same hashcode as name, the index
   of that entry in the cache is returned, else whefs_id_type_end.
*/
whefs_id_type whefs_inode_hash_cache_search_ndx(whefs_fs * fs, char const * name );

/**
   Searches for a cache entry with the same hash as name. If it finds
   it, it removes it. The cache becomes unsorted by this.
*/
void whefs_inode_name_uncache(whefs_fs * fs, char const * name );

/**
   Iterates over all inodes and caches the name entries for all
   in-used inodes. This is normally automatically called when an FS is
   opened (unless an internal compile-time flag disables that).
*/
int whefs_inode_hash_cache_load( whefs_fs * fs );

/**
   Sorts the name cache, if it is loaded. It shouldn't strictly be
   in the public API, but it may need to be.
*/
void whefs_inode_hash_cache_sort(whefs_fs * fs );

int whefs_fs_cache_name( whefs_fs * fs, whefs_id_type id, char const * n );

#endif /* WANDERINGHORSE_NET_WHEFS_DETAILS_C_INCLUDED */
