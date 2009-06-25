/**
  Author: Stephan Beal (http://wanderinghorse.net/home/stephan/

  License: Public Domain

  This file contiains most of the name-cache-related functionality.
*/

#include "whefs_details.c"
#include <stdlib.h> // free()
#include <string.h> // memset()
#include <assert.h>
#include "whefs_cache.h"

const whefs_string_cache whefs_string_cache_init = whefs_string_cache_init_m;


#if WHEFS_CONFIG_ENABLE_STATIC_MALLOC
enum {
/**
   The number of elements to statically allocate
   in the whefs_string_cache_alloc_slots object.

   We only need one per live whefs_fs object.
*/
whefs_string_cache_alloc_count = 3
};
static struct
{
    whefs_string_cache objs[whefs_string_cache_alloc_count];
    char used[whefs_string_cache_alloc_count];
    size_t next;
    const size_t count;
} whefs_string_cache_alloc_slots = { { whefs_string_cache_init_m }, {0}, 0, whefs_string_cache_alloc_count };
#endif

whefs_string_cache * whefs_string_cache_alloc()
{
    whefs_string_cache * obj = 0;
#if WHEFS_CONFIG_ENABLE_STATIC_MALLOC
    size_t i = whefs_string_cache_alloc_slots.next;
    for( ; i < whefs_string_cache_alloc_slots.count; ++i )
    {
	if( whefs_string_cache_alloc_slots.used[i] ) continue;
	whefs_string_cache_alloc_slots.used[i] = 1;
	whefs_string_cache_alloc_slots.next = i+1;
	obj = &whefs_string_cache_alloc_slots.objs[i];
	break;
    }
#endif /* WHEFS_CONFIG_ENABLE_STATIC_MALLOC */
    if( ! obj ) obj = (whefs_string_cache *) malloc( sizeof(whefs_string_cache) );
    if( obj ) *obj = whefs_string_cache_init;
    return obj;
}

void whefs_string_cache_free( whefs_string_cache * obj )
{
    if( ! obj ) return;
    whefs_string_cache_cleanup( obj );
#if WHEFS_CONFIG_ENABLE_STATIC_MALLOC
    if( (obj < &whefs_string_cache_alloc_slots.objs[0]) ||
	(obj > &whefs_string_cache_alloc_slots.objs[whefs_string_cache_alloc_slots.count-1]) )
    { /* it does not belong to us */
        *obj = whefs_string_cache_init;
	free(obj);
	return;
    }
    else
    {
	const size_t ndx = (obj - &whefs_string_cache_alloc_slots.objs[0]);
	whefs_string_cache_alloc_slots.used[ndx] = 0;
	if( ndx < whefs_string_cache_alloc_slots.next ) whefs_string_cache_alloc_slots.next = ndx;
	return;
    }
#else
    free(obj);
#endif /* WHEFS_CONFIG_ENABLE_STATIC_MALLOC */
}


int whefs_string_cache_cleanup( whefs_string_cache * db )
{
    if( ! db ) return whefs_rc.ArgError;
    else
    {
        whio_blockdev_cleanup( &db->devBlock );
        if( db->devMem ) db->devMem->api->finalize( db->devMem );
        *db = whefs_string_cache_init;
        return whefs_rc.OK;
    }

}

int whefs_string_cache_clear_contents( whefs_string_cache * db )
{
    if( ! db ) return whefs_rc.ArgError;
    else
    {
        if( db->devMem ) db->devMem->api->truncate( db->devMem, 0 );
        return whefs_rc.OK;
    }

}

int whefs_string_cache_setup( whefs_string_cache * db, whefs_id_type blockCount, whio_size_t blockSize )
{
    if( ! db ) return whefs_rc.ArgError;
    whefs_string_cache_cleanup( db );
    db->devMem = whio_dev_for_membuf( 0, 1.0 );
    static unsigned char wipebuf[WHEFS_MAX_FILENAME_LENGTH+1] = {'*'};
    if( '*' == wipebuf[0] )
    {
        memset( wipebuf, 0, WHEFS_MAX_FILENAME_LENGTH+1 );
    }
    int rc = whio_blockdev_setup( &db->devBlock, db->devMem, 0U,
                                  blockSize+1/*trailing null!*/, blockCount, wipebuf );
    if( whefs_rc.OK != rc )
    {
        whefs_string_cache_cleanup(db);
    }
    return rc;
}

whefs_string_cache * whefs_string_cache_create( whefs_id_type blockCount, whio_size_t blockSize )
{
    whefs_string_cache * db = whefs_string_cache_alloc();
    if( ! db ) return 0;
    int rc = whefs_string_cache_setup( db, blockCount, blockSize );
    if( whefs_rc.OK != rc )
    {
        whefs_string_cache_free(db);
        db = 0;
    }
    return db;
}

whio_size_t whefs_string_cache_memcost( whefs_string_cache const * db )
{
    if( ! db ) return 0;
    whio_size_t mbase = sizeof(whefs_string_cache)
        + (db->devMem ? sizeof(*(db->devMem)) : 0)
        ;
    whio_size_t msize = (whio_size_t)-1;
    whio_dev_ioctl( db->devMem, whio_dev_ioctl_BUFFER_size, &msize );
    if( (whio_size_t)-1 == msize )
    {
        if( 0 == whio_dev_size(db->devMem) ) return mbase;
        assert( 0 && "whio_dev_ioctl_BUFFER_size not behaving as documented!" );
        return 0;
    }
    msize += mbase;
        // we don't know the underlying internal costs of db->devMem, other than msize.
        ;
    return msize;
}

int whefs_string_cache_set( whefs_string_cache * db, whefs_id_type id, char const * str )
{
    if(!WHEFS_CONFIG_ENABLE_STRINGS_CACHE)
    {
        return whefs_rc.OK;
    }
    if( ! whio_blockdev_in_range( db ? &db->devBlock : 0, id ) ) return whefs_rc.ArgError;
    size_t slen = (str && *str) ? strlen(str) : 0;
    if( db->devBlock.blocks.size <= slen ) return whefs_rc.RangeError;
    enum { bufSize = WHEFS_MAX_FILENAME_LENGTH+1 };
    unsigned char buf[bufSize] = {0};
#if 1
    whio_blockdev_wipe( &db->devBlock, id );
#endif
    memcpy( buf, str, slen );
    memset( buf + slen, 0, db->devBlock.blocks.size - slen );
    //WHEFS_DBG("writing %u bytes for name of inode #%"WHEFS_ID_TYPE_PFMT" as [%s]",slen,id+1,buf);
    const int rc = whio_blockdev_write( &db->devBlock, id, buf );
#if 0 // truncate to a known maximum size if we've allocated above that.
    whio_size_t msize = (whio_size_t)-1;
    whio_dev_ioctl( db->devMem, whio_dev_ioctl_BUFFER_size, &msize );
    if( (whio_size_t)-1 == msize )
    {
        assert( 0 && "whio_dev_ioctl_GENERAL_size not behaving as documented!" );
        rc = whefs_rc.InternalError;
    }
    else if( msize > (db->devBlock.blocks.size * db->devBlock.blocks.count) )
    {
        db->devMem->api->truncate( db->devMem, (db->devBlock.blocks.size * db->devBlock.blocks.count) + 1 );
    }
#endif
    return rc;
}

char const * whefs_string_cache_get( whefs_string_cache const * db, whefs_id_type id )
{
    if( ! WHEFS_CONFIG_ENABLE_STRINGS_CACHE ) return 0;
#if 0
    static char empty[2] = {0,0};
#elseif 0
    static char const * empty = "";
#else
    static char const * empty = 0;
    /**
       Reminder to self: we need to be able to differentiate between
       "found but empty" and "not yet loaded".
    */
#endif
    if( ! db ) return 0;
    if( ! whio_blockdev_in_range( db ? &db->devBlock : 0, id ) ) return 0;


    whio_size_t msize = (whio_size_t)-1;
    whio_dev_ioctl( db->devMem, whio_dev_ioctl_GENERAL_size, &msize );
    if( (whio_size_t)-1 == msize )
    {
        assert( 0 && "whio_dev_ioctl_GENERAL_size not behaving as documented!");
        (void)0;
        return 0;
    }
    if( ! msize ) return empty;

    unsigned char const * mem = 0;
    whio_dev_ioctl( db->devMem, whio_dev_ioctl_BUFFER_uchar_ptr, &mem );
    assert( mem && "whio_dev_ioctl_BUFFER_uchar_ptr not behaving as documented!");
    if( ! mem ) return empty;

    //WHEFS_DBG("id=%"WHEFS_ID_TYPE_PFMT", mem=%p [%s]",id,(void const *)mem,(mem?(char const*)mem:"<empty>") );
    const whio_size_t off = (id * db->devBlock.blocks.size);
    if( off > msize ) return 0;
    if(id)
    {
        mem += off;
        //WHEFS_DBG("id=%"WHEFS_ID_TYPE_PFMT", mem=%p [%s] [-1=%d]",id,(void const *)mem,(mem?(char const*)mem:"<empty>"), *(mem-1) );
        assert( ('\0' == *(mem-1)) && "Logic or memory management error here!");
    }
    return (mem && *mem) ? (char const *)mem : (char const *)0;
}

int whefs_inode_hash_cache_load( whefs_fs * fs )
{
    if( ! WHEFS_FS_HASH_CACHE_IS_ENABLED(fs) ) return whefs_rc.OK;
    if( ! fs->cache.hashes )
    {
        whefs_id_type toAlloc = 16 /* arbitrary */;
        if( toAlloc > fs->options.inode_count ) toAlloc = fs->options.inode_count;
        whefs_hashid_list_alloc( &fs->cache.hashes, toAlloc );
    }
    if( ! fs->cache.hashes ) return whefs_rc.AllocError;
    else if( !fs->cache.hashes->maxAlloc )
    {
        fs->cache.hashes->maxAlloc = fs->options.inode_count;
    }

    whefs_hashid_list * li = fs->cache.hashes;
    //whefs_hashid h = whefs_hashid_init;
    whefs_string name = whefs_string_init;
    enum { bufSize = WHEFS_MAX_FILENAME_LENGTH+1 };
    unsigned char buf[bufSize];
    memset( buf, 0, bufSize );
    // ensure that whefs_inode_name_get() won't malloc():
    name.string = (char *)buf;
    name.alloced = bufSize;
    int rc = 0;
    whefs_id_type i;
    whefs_id_type count = 0;
    fs->cache.hashes->skipAutoSort = true; // optimization to avoid extra sorting via whefs_inode_name_get()
    for( i = fs->options.inode_count; i >=1 ; --i )
    {
        /**
           Maintenance reminder:

           We do this loop in reverse order as an efficiency hack for
           whefs_string_cache. The trick is: the whefs_string_cache's
           internal buffer grows only as the number of used inodes
           does (it's size is a function of the highest used inode
           ID). If we insert from low to high it may realloc many
           times. If we insert from high to low, we're guaranteed to
           need only one malloc/realloc on it.
        */
#if WHEFS_CONFIG_ENABLE_BITSET_CACHE
        if( fs->bits.i_loaded )
	{
            if( ! WHEFS_ICACHE_IS_USED(fs,i) )
            {
                continue;
            }
	}
#endif // WHEFS_CONFIG_ENABLE_BITSET_CACHE
        rc = whefs_inode_name_get( fs, i, &name ); // this caches the name hash
        if( whefs_rc.OK != rc ) break;
        ++count;
    }
    assert( (name.string == (char *)buf) && "Internal memory management foo-foo." );
    fs->cache.hashes->skipAutoSort = false;
    whefs_hashid_list_sort(li);
    WHEFS_DBG_CACHE("Loaded all used inodes into cache with %"WHEFS_ID_TYPE_PFMT" name(s).",count);
    return rc;
}
int whefs_inode_hash_cache_chomp_lv( whefs_fs * fs )
{
    if( ! fs ) return whefs_rc.ArgError;
    if( ! fs->cache.hashes || !fs->cache.hashes->count ) return whefs_rc.OK;
    return whefs_hashid_list_chomp_lv( fs->cache.hashes );
}

whefs_id_type whefs_inode_hash_cache_search_ndx(whefs_fs * fs, char const * name )
{
    if( ! WHEFS_FS_HASH_CACHE_IS_ENABLED(fs) ) return whefs_rc.IDTypeEnd;
    if( fs->cache.hashes && !fs->cache.hashes->isSorted )
    {
        WHEFS_DBG_CACHE("Achtung: auto-sorting dirty name cache before search starts.");
        whefs_inode_hash_cache_sort(fs);
    }
    return ( ! fs->cache.hashes )
        ? whefs_rc.IDTypeEnd
        : whefs_hashid_list_index_of( fs->cache.hashes, fs->cache.hashfunc(name) );
}

whefs_id_type whefs_inode_hash_cache_search_id(whefs_fs * fs, char const * name )
{
    if( ! WHEFS_FS_HASH_CACHE_IS_ENABLED(fs) ) return 0;
    if( ! fs->cache.hashes ) return 0;
    whefs_id_type n = whefs_inode_hash_cache_search_ndx( fs, name );
    return ( n == whefs_rc.IDTypeEnd )
        ? 0
        : fs->cache.hashes->list[n].id;
}



void whefs_inode_hash_cache_sort(whefs_fs * fs )
{
    if( fs->cache.hashes && ! fs->cache.hashes->isSorted )
    {
        whefs_hashid_list_sort( fs->cache.hashes );
    }
}


void whefs_inode_name_uncache(whefs_fs * fs, char const * name )
{
    if( !fs->cache.hashes || ! name || !*name  ) return;
    const whefs_id_type ndx = whefs_inode_hash_cache_search_ndx( fs, name );
    if( whefs_rc.IDTypeEnd != ndx )
    {
        fs->cache.hashes->list[ndx] = whefs_hashid_init;
        fs->cache.hashes->isSorted = false;
    }
}

int whefs_inode_hash_cache( whefs_fs * fs, whefs_id_type id, char const * name )
{
    if( ! WHEFS_FS_HASH_CACHE_IS_ENABLED(fs) )
    {
        return whefs_rc.OK;
    }
    if( ! fs || !name || !*name ) return whefs_rc.ArgError;
    int rc = whefs_string_cache_set( &fs->cache.strings, id-1, name );
    if( whefs_rc.OK != rc ) return rc;
    if( ! fs->cache.hashes )
    {
        const whefs_id_type max = fs->options.inode_count;
        whefs_id_type dflt = 100/sizeof(whefs_hashid) /*for lack of a better default value.*/;
        if( dflt > max ) dflt = max;
        rc = whefs_hashid_list_alloc( &fs->cache.hashes, dflt );
        if( fs->cache.hashes )
        {
            fs->cache.hashes->maxAlloc = max;
        }
    }
    if( whefs_rc.OK != rc ) return rc;
    whefs_hashval_type h = fs->cache.hashfunc( name );
#if 1
    const whefs_id_type ndx = whefs_hashid_list_index_of( fs->cache.hashes, h );
    if( whefs_rc.IDTypeEnd != ndx )
    {
        if(0) WHEFS_DBG("CHECKING: name cache count[%"WHEFS_ID_TYPE_PFMT"], alloced=[%"WHEFS_ID_TYPE_PFMT"], hash [%"WHEFS_HASHVAL_TYPE_PFMT"] for name [%s], ndx=[%"WHEFS_ID_TYPE_PFMT"]",
                        fs->cache.hashes->count, fs->cache.hashes->alloced, h, name, ndx );
        if( fs->cache.hashes->list[ndx].id == id ) return whefs_rc.OK;
        WHEFS_DBG_ERR("ERROR: name cache hash collision on hash code "
                      "%"WHEFS_HASHVAL_TYPE_PFMT" "
                      "between inodes #"
                      "%"WHEFS_ID_TYPE_PFMT
                      " and #%"WHEFS_ID_TYPE_PFMT"!",
                      h, id, ndx );
        return whefs_rc.InternalError;
    }
    WHEFS_DBG_CACHE("ADDING: name cache count[%"WHEFS_ID_TYPE_PFMT"], alloced=[%"WHEFS_ID_TYPE_PFMT"], hash [%"WHEFS_HASHVAL_TYPE_PFMT"] for name [%s], ndx=[%"WHEFS_ID_TYPE_PFMT"]",
                          fs->cache.hashes->count, fs->cache.hashes->alloced, h, name, ndx );
#endif
    whefs_hashid H = whefs_hashid_init;
    H.hash = h;
    H.id = id;
    rc = whefs_hashid_list_add( fs->cache.hashes, &H );
    WHEFS_DBG_CACHE("Added to name cache: hash[%"WHEFS_HASHVAL_TYPE_PFMT"]=id[%"WHEFS_ID_TYPE_PFMT"], name=[%s], rc=%d", H.hash, H.id, name, rc );
    return whefs_rc.OK;
}


