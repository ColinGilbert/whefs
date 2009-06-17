/*
  Author: Stephan Beal (http://wanderinghorse.net/home/stephan/)

  License: Public Domain
*/

#if defined(__cplusplus) && !defined(__STDC_LIMIT_MACROS)
#  define __STDC_LIMIT_MACROS
/* A comment from the Linux stdint.h:

   The ISO C99 standard specifies that in C++ implementations these
   macros should only be defined if explicitly requested.
*/
#endif
#include <stdint.h>



#include <string.h> /* memset() */
#include <stdlib.h> /* calloc() and friends */
#include <inttypes.h> /* PRIuXX */
#include <wh/whio/whio_common.h>
#include <wh/whio/whio_dev.h>
#include <wh/whio/whio_devs.h>
#include <wh/whprintf.h>

#define WHIO_DEV_EMPTY_INIT {0/*api*/, whio_impl_data_init_m, whio_client_data_init_m }

static const whio_dev whio_dev_empty_init = WHIO_DEV_EMPTY_INIT;

#if WHIO_USE_STATIC_MALLOC
enum {
/**
   The number of elements to statically allocate
   in the whio_dev_alloc_slots object.
*/
whio_dev_alloc_count = 15
};
struct
{
    whio_dev devs[whio_dev_alloc_count]; /* Flawfinder: ignore  this is intentional. */
    char used[whio_dev_alloc_count]; /* Flawfinder: ignore  this is intentional. */
    whio_size_t next;
} whio_dev_alloc_slots = { {WHIO_DEV_EMPTY_INIT}, {0}, 0 };
#endif

whio_dev * whio_dev_alloc()
{
    whio_dev * dev = 0;
#if WHIO_USE_STATIC_MALLOC
    size_t i = whio_dev_alloc_slots.next;
    for( ; i < whio_dev_alloc_count; ++i )
    {
	if( whio_dev_alloc_slots.used[i] ) continue;
	whio_dev_alloc_slots.next = i+1;
	whio_dev_alloc_slots.used[i] = 1;
	dev = &whio_dev_alloc_slots.devs[i];
	//WHIO_DEBUG("Allocated device #%u @0x%p\n", i, (void const *)dev );
	break;
    }
#endif /* WHIO_USE_STATIC_MALLOC */
    if( ! dev ) dev = (whio_dev *) malloc( sizeof(whio_dev) );
    if( dev ) *dev = whio_dev_empty_init;
    return dev;
}

void whio_dev_free( whio_dev * dev )
{
    if( dev ) *dev = whio_dev_empty_init;
    else return;	
#if WHIO_USE_STATIC_MALLOC
    if( (dev < &whio_dev_alloc_slots.devs[0]) ||
	(dev > &whio_dev_alloc_slots.devs[whio_dev_alloc_count-1]) )
    { /* doesn't belong to us. */
	free(dev);
	return;
    }
    else
    {
	size_t ndx = dev - &whio_dev_alloc_slots.devs[0];
	if( 0 )
	{
	    WHIO_DEBUG("Address range = 0x%p to 0x%p, ndx=%u\n",
		       (void const *)&whio_dev_alloc_slots.devs[0],
		       (void const *)&whio_dev_alloc_slots.devs[whio_dev_alloc_count-1],
		       ndx
		       );
	    WHIO_DEBUG("Freeing object @0x%p from static pool index %u (@0x%p)\n",
		       (void const *)dev,
		       ndx,
		       (void const *)&whio_dev_alloc_slots.devs[ndx] );
	}
	whio_dev_alloc_slots.used[ndx] = 0;
	if( ndx < whio_dev_alloc_slots.next ) whio_dev_alloc_slots.next = ndx;
	return;
    }
#else
    free(dev);
#endif /* WHIO_USE_STATIC_MALLOC */
}


int whio_dev_ioctl( whio_dev * dev, int operation, ... )
{
    if( ! dev ) return whio_rc.ArgError;
    va_list vargs;
    va_start( vargs, operation );
    int rc = dev->api->ioctl( dev, operation, vargs );
    va_end(vargs);
    return rc;
}

whio_size_t whio_dev_write( whio_dev * dev, void const * data, whio_size_t n )
{
    return dev->api->write( dev, data, n );
}

whio_size_t whio_dev_writeat( whio_dev * dev, whio_size_t pos, void const * data, whio_size_t n )
{
    if( ! dev || ! data || !n ) return 0;
    //WHIO_DEBUG("Writing %u bytes at pos %u\n", n, pos );
    whio_size_t rc = dev->api->seek( dev, pos, SEEK_SET );
    return (whio_rc.SizeTError == rc)
	? rc
	: whio_dev_write( dev, data, n );
}

whio_size_t whio_dev_size( whio_dev * dev )
{
    if( ! dev ) return whio_rc.SizeTError;
    whio_size_t pos = dev->api->tell( dev );
    if( whio_rc.SizeTError == pos ) return pos;
    whio_size_t rc = dev->api->seek( dev, 0L, SEEK_END );
    dev->api->seek( dev, pos, SEEK_SET );
    return rc;
}

int whio_dev_rewind( whio_dev * dev )
{
    if( ! dev ) return whio_rc.ArgError;
    return (whio_rc.SizeTError != dev->api->seek( dev, 0, SEEK_SET ))
	? whio_rc.OK
	: whio_rc.IOError;
}

int whio_dev_copy( whio_dev * src, whio_dev * dest )
{
    if( ! src || ! dest ) return whio_rc.ArgError;
    int rc = whio_rc.OK;
    enum { bufSize = (1024 * 4) };
    unsigned char buf[bufSize];  /* Flawfinder: ignore This is intentional and used correctly in the loop below. */
    whio_size_t rlen = 0;
    if( whio_rc.SizeTError == src->api->seek( src, 0L, SEEK_SET ) )
    {
	return whio_rc.RangeError;
    }
    while( (rlen = src->api->read( src, buf /*Flawfinder: ignore  This is safe in conjunction with bufSize*/, bufSize ) ) )
    {
	if( rlen != dest->api->write( dest, buf, rlen ) )
	{
	    rc = whio_rc.IOError;
	    break;
	}
    }
    return rc;
}


static const unsigned char whio_dev_uint64_tag_char = 0x80 | 64;
size_t whio_dev_uint64_encode( whio_dev * fs, uint64_t i )
{
    unsigned char buf[whio_dev_sizeof_uint64]; /* Flawfinder: ignore This is intentional and safe as long as whio_dev_sizeof_uint64 is the correct size. */
    const uint64_t mask = UINT64_C(0x00ff);
    size_t x = 0;
    buf[x++] = whio_dev_uint64_tag_char;
    buf[x++] = (unsigned char)((i>>56) & mask);
    buf[x++] = (unsigned char)((i>>48) & mask);
    buf[x++] = (unsigned char)((i>>40) & mask);
    buf[x++] = (unsigned char)((i>>32) & mask);
    buf[x++] = (unsigned char)((i>>24) & mask);
    buf[x++] = (unsigned char)((i>>16) & mask);
    buf[x++] = (unsigned char)((i>>8) & mask);
    buf[x++] = (unsigned char)(i & mask);
    return whio_dev_write( fs, buf, whio_dev_sizeof_uint64 );
}

int whio_dev_uint64_decode( whio_dev * dev, uint64_t * tgt )
{
    if( ! dev || ! tgt ) return whio_rc.ArgError;
    unsigned char buf[whio_dev_sizeof_uint64]; /* Flawfinder: ignore This is intentional and safe as long as whio_dev_sizeof_uint64 is the correct size. */
    memset( buf, 0, whio_dev_sizeof_uint64 );
    size_t rc = dev->api->read( dev, buf  /*Flawfinder: ignore  This is safe in conjunction with whio_dev_sizeof_uint64*/, whio_dev_sizeof_uint64 );
    if( whio_dev_sizeof_uint64 != rc )
    {
	return whio_rc.IOError;
    }
    if( whio_dev_uint64_tag_char != buf[0] )
    {
	return whio_rc.ConsistencyError;
    }
#define SHIFT(X) ((uint64_t)buf[X])
    uint64_t val = (SHIFT(1) << UINT64_C(56))
	+ (SHIFT(2) << UINT64_C(48))
	+ (SHIFT(3) << UINT64_C(40))
	+ (SHIFT(4) << UINT64_C(32))
	+ (SHIFT(5) << UINT64_C(24))
	+ (SHIFT(6) << UINT64_C(16))
	+ (SHIFT(7) << UINT64_C(8))
	+ (SHIFT(8));
#undef SHIFT
    *tgt = val;
    return whio_rc.OK;
}


static const unsigned char whio_dev_uint32_tag_char = 0x80 | 32;
size_t whio_dev_uint32_encode( whio_dev * fs, uint32_t i )
{
    unsigned char buf[whio_dev_sizeof_uint32];  /* Flawfinder: ignore This is intentional and safe as long as whio_dev_sizeof_uint32 is the correct size. */
    const unsigned short mask = 0x00ff;
    size_t x = 0;
    /** We tag all entries with a prefix mainly to simplify debugging
	of the vfs files (it's easy to spot them in a file viewer),
	but it incidentally also gives us a sanity-checker at
	read-time (we simply confirm that the first byte is this
	prefix).
    */
    buf[x++] = whio_dev_uint32_tag_char;
    buf[x++] = (unsigned char)(i>>24) & mask;
    buf[x++] = (unsigned char)(i>>16) & mask;
    buf[x++] = (unsigned char)(i>>8) & mask;
    buf[x++] = (unsigned char)(i & mask);
    //WHIO_DEBUG("wrote int=%u to [%02x %02x %02x %02x]\n",i,buf[1],buf[2],buf[3],buf[4]);
    return whio_dev_write( fs, buf, whio_dev_sizeof_uint32 );
}

int whio_dev_uint32_decode( whio_dev * dev, uint32_t * tgt )
{
    if( ! dev || ! tgt ) return whio_rc.ArgError;
    unsigned char buf[whio_dev_sizeof_uint32];  /* Flawfinder: ignore This is intentional and safe as long as whio_dev_sizeof_uint32 is the correct size. */
    memset( buf, 0, whio_dev_sizeof_uint32 );
    size_t rc = dev->api->read( dev, buf, whio_dev_sizeof_uint32 ); /*Flawfinder: ignore  This is safe in conjunction with whio_dev_sizeof_uint32*/
    if( whio_dev_sizeof_uint32 != rc )
    {
	//WHIO_DEBUG("read of integer failed! rc=%u\n",rc);
	return whio_rc.IOError;
    }
    //WHIO_DEBUG("read int(?) from [%c %02x %02x %02x %02x]\n",buf[0],buf[1],buf[2],buf[3],buf[4]);
    if( whio_dev_uint32_tag_char != buf[0] )
    {
	//WHIO_DEBUG("read bytes are not an encoded integer value!\n");
	return whio_rc.ConsistencyError;
    }
    uint32_t val = (buf[1] << 24)
	+ (buf[2] << 16)
	+ (buf[3] << 8)
	+ (buf[4]);
    *tgt = val;
    //WHIO_DEBUG("read int=%u from [%c %02x %02x %02x %02x]\n",val,buf[0],buf[1],buf[2],buf[3],buf[4]);
    return whio_rc.OK;
}


static const unsigned char whio_dev_uint16_tag_char = 0x80 | 16;
size_t whio_dev_uint16_encode( whio_dev * fs, uint16_t i )
{
    unsigned char buf[whio_dev_sizeof_uint16]; /* Flawfinder: ignore This is intentional and safe as long as whio_dev_sizeof_uint16 is the correct size. */
    const size_t mask = 0x00ff;
    size_t x = 0;
    buf[x++] = whio_dev_uint16_tag_char;
    buf[x++] = (unsigned char)(i>>8) & mask;
    buf[x++] = (unsigned char)(i & mask);
    return whio_dev_write( fs, buf, whio_dev_sizeof_uint16 );
}

int whio_dev_uint16_decode( whio_dev * dev, uint16_t * tgt )
{
    if( ! dev || ! tgt ) return whio_rc.ArgError;
    unsigned char buf[whio_dev_sizeof_uint16]; /* Flawfinder: ignore This is intentional and safe as long as whio_dev_sizeof_uint16 is the correct size. */
    memset( buf, 0, whio_dev_sizeof_uint16 );
    size_t rc = dev->api->read( dev, buf, whio_dev_sizeof_uint16 ); /*Flawfinder: ignore  This is safe in conjunction with whio_dev_sizeof_uint16*/
    if( whio_dev_sizeof_uint16 != rc )
    {
	//WHIO_DEBUG("read of uint16 failed! rc=%u\n",rc);
	return whio_rc.IOError;
    }
    if( whio_dev_uint16_tag_char != buf[0] )
    {
	//WHIO_DEBUG("read bytes are not an encoded uint16 value!\n");
	return whio_rc.ConsistencyError;
    }
    uint16_t val = + (buf[1] << 8)
	+ (buf[2]);
    *tgt = val;
    return whio_rc.OK;
}

size_t whio_dev_uint32_array_encode( whio_dev * dev, size_t n, uint32_t const * list )
{
    size_t i = (dev && n && list) ? 0 : n;
    size_t rc = 0;
    for( ; i < n; ++i, ++rc )
    {
	if( whio_dev_sizeof_uint32 != whio_dev_uint32_encode( dev, *(list++) ) )
	{
	    break;
	}
    }
    return rc;
}

size_t whio_dev_uint32_array_decode( whio_dev * dev, size_t n, uint32_t * list )
{
    size_t i = (dev && n && list) ? 0 : n;
    size_t rc = 0;
    for( ; i < n; ++i, ++rc )
    {
	if( whio_rc.OK != whio_dev_uint32_decode( dev, &list[i] ) )
	{
	    break;
	}
    }
    return rc;
}

static const unsigned char whio_dev_cstring_tag_char = 0x80 | '"';
whio_size_t whio_dev_cstring_encode( whio_dev * dev, char const * s, uint32_t n )
{
    if( ! dev || !s ) return 0;
    if( ! n )
    {
	char const * x = s;
	loop: if( x && *x ) { ++x; ++n; goto loop; }
	//for( ; x && *x ; ++x, ++n ){}
    }
    whio_size_t rc = dev->api->write( dev, &whio_dev_cstring_tag_char, 1 );
    if( 1 != rc ) return rc;
    rc += whio_dev_uint32_encode( dev, n );
    if( (1 + whio_dev_sizeof_uint32) != rc ) return rc;
    return rc + dev->api->write( dev, s, (whio_size_t)n );
}

int whio_dev_cstring_decode( whio_dev * dev, char ** tgt, uint32_t * length )
{
    if( !dev || ! tgt ) return whio_rc.ArgError;

    { /* check tag byte */
	unsigned char tagbuf[1] = {0}; /* Flawfinder: ignore  This is intended and safe. */
	whio_size_t sz = dev->api->read( dev, tagbuf, 1 );/*Flawfinder: ignore  This is safe used safely.*/
	if( (1 != sz) || (whio_dev_cstring_tag_char != tagbuf[0]) )
	{
	    return sz ? whio_rc.ConsistencyError : whio_rc.IOError;
	}
    }
    uint32_t slen = 0;
    int rc = whio_dev_uint32_decode( dev, &slen );
    if( whio_rc.OK != rc ) return rc;
    if( ! slen )
    {
	*tgt = 0;
	if(length) *length = 0;
	return whio_rc.OK;
    }
    char * buf = (char *)calloc( slen + 1, sizeof(char) );
    if( ! buf ) return whio_rc.AllocError;
    if( slen != dev->api->read( dev, buf /*Flawfinder: ignore  This is safe in conjunction with slen. See above. */, slen ) )
    {
	free( buf );
	return whio_rc.IOError;
    }
    *tgt = buf;
    if( length ) *length = slen;
    return whio_rc.OK;
}

static long whio_dev_printf_appender( void * arg, char const * data, long n )
{
    if( ! arg || !data || (n<1) ) return -1;
    size_t sz = n;
    if( n < sz ) return -1; /* negative n */
    whio_dev * dev = (whio_dev*)arg;
    sz = dev->api->write( dev, data, sz );
    return (sz == whio_rc.SizeTError) ? 0 : (long) sz; // FIXME: check for overflow!
}

size_t whio_dev_writefv( whio_dev * dev, const char *fmt, va_list ap )
{
    long rc = whprintfv( whio_dev_printf_appender, dev, fmt, ap );
    return (rc < 0) ? 0 : (size_t)rc;
}

size_t whio_dev_writef( whio_dev * dev, const char *fmt, ... )
{
    va_list vargs;
    va_start( vargs, fmt );
    size_t rc = whio_dev_writefv( dev, fmt, vargs );
    va_end(vargs);
    return rc;
}

whio_size_t whio_dev_read( whio_dev * dev, void * dest, whio_size_t n )
{
    return dev ? dev->api->read( dev, dest, n ) : 0; /*Flawfinder: ignore  Bounds check is done in proxied function (cannot be done here). */
}

int whio_dev_eof( whio_dev * dev )
{
    return dev ? dev->api->eof( dev ) : whio_rc.ArgError;
}

whio_size_t whio_dev_tell( whio_dev * dev )
{
    return dev ? dev->api->tell( dev ) : whio_rc.SizeTError;;
}

whio_size_t whio_dev_seek( whio_dev * dev, off_t pos, int whence )
{
    return dev ? dev->api->seek( dev, pos, whence ) : whio_rc.SizeTError;
}

int whio_dev_flush( whio_dev * dev )
{
    return dev ? dev->api->flush( dev ) : whio_rc.ArgError;
}

int whio_dev_truncate( whio_dev * dev, off_t size )
{
    return dev ? dev->api->truncate( dev, size ) : whio_rc.ArgError;
}

void whio_dev_finalize( whio_dev * dev )
{
    if( dev ) dev->api->finalize( dev );
    return;
}

bool whio_dev_close( whio_dev * dev )
{
    return dev ? dev->api->close( dev ) : false;
}

whio_fetch_result * whio_dev_fetch( whio_dev * dev, whio_size_t n )
{
    if( ! dev ) return 0;
    whio_fetch_result * rc = (whio_fetch_result*)malloc(sizeof(whio_fetch_result));
    if( ! rc ) return 0;
    rc->alloced = 0;
    rc->requested = n;
    rc->read = 0;
    if( ! n ) return rc;
    const whio_size_t sza = n+1; /* the +1 is necessary so we can ensure nulls for script-side strings. */
    rc->data = (char *)malloc(sza);
    if( ! rc->data )
    {
	free(rc);
	return 0;
    }
    rc->alloced = sza;
    memset( rc->data, 0, sza );
    rc->read = dev->api->read( dev, rc->data, n ); /*Flawfinder: ignore rc->data will always be longer than (see above). */
    return rc;
}

int whio_dev_fetch_r( whio_dev * dev, whio_size_t n, whio_fetch_result * tgt )
{
    if( ! dev || !tgt ) return whio_rc.ArgError;
    if( ! n )
    {
	tgt->requested = tgt->read = n;
	return whio_rc.OK;
    }
    tgt->read = 0;
    if( !tgt->data || (tgt->alloced < n) )
    {
	whio_size_t sza = n + 1;
	void * p = realloc( tgt->data, sza );
	if( ! p ) return whio_rc.AllocError;
	memset( p, 0, sza );
	tgt->data = p;
	tgt->alloced = sza;
    }
    tgt->requested = n;
    memset( tgt->data, 0, n );
    tgt->read = dev->api->read( dev, tgt->data, n );  /*Flawfinder: ignore  tgt->data will always be  longer than n as long as tgt->allocated is set properly. */
    return whio_rc.OK;
}

int whio_dev_fetch_free( whio_fetch_result * r )
{
    if( r )
    {
	free(r->data);
	free(r);
	return whio_rc.OK;
    }
    return whio_rc.ArgError;
}
int whio_dev_fetch_free_data( whio_fetch_result * r )
{
    if( r )
    {
	free(r->data);
	r->alloced = 0;
	r->data = 0;
	return whio_rc.OK;
    }
    return whio_rc.ArgError;
}


const whio_blockdev whio_blockdev_init = whio_blockdev_init_m;
#if WHIO_USE_STATIC_MALLOC
enum {
/**
   The number of elements to statically allocate
   in the whio_blockdev_alloc_slots object.
*/
whio_blockdev_alloc_count = 10
};
#define whio_blockdev_alloc_slots_whio_blockdev_INIT {0 /* FILL THIS OUT FOR whio_blockdev OBJECTS! */}
static struct
{
    whio_blockdev objs[whio_blockdev_alloc_count]; /* Flawfinder: ignore This is intentional. */
    char used[whio_blockdev_alloc_count]; /* Flawfinder: ignore This is intentional. */
} whio_blockdev_alloc_slots = { { whio_blockdev_init_m }, {0} };
#endif

whio_blockdev * whio_blockdev_alloc()
{
    whio_blockdev * obj = 0;
#if WHIO_USE_STATIC_MALLOC
    size_t i = 0;
    for( ; i < whio_blockdev_alloc_count; ++i )
    {
	if( whio_blockdev_alloc_slots.used[i] ) continue;
	whio_blockdev_alloc_slots.used[i] = 1;
	obj = &whio_blockdev_alloc_slots.objs[i];
	break;
    }
#endif /* WHIO_USE_STATIC_MALLOC */
    if( ! obj ) obj = (whio_blockdev *) malloc( sizeof(whio_blockdev) );
    if( obj ) *obj = whio_blockdev_init;
    return obj;
}

void whio_blockdev_free( whio_blockdev * obj )
{
    whio_blockdev_cleanup( obj );
#if WHIO_USE_STATIC_MALLOC
    if( (obj < &whio_blockdev_alloc_slots.objs[0]) ||
	(obj > &whio_blockdev_alloc_slots.objs[whio_blockdev_alloc_count-1]) )
    { /* it does not belong to us */
	free(obj);
	return;
    }
    else
    {
	const size_t ndx = (obj - &whio_blockdev_alloc_slots.objs[0]);
	whio_blockdev_alloc_slots.objs[ndx] = whio_blockdev_init;
	whio_blockdev_alloc_slots.used[ndx] = 0;
	return;
    }
#else
    if( obj ) *obj = whio_blockdev_init;
    free(obj);
#endif /* WHIO_USE_STATIC_MALLOC */
}

bool whio_blockdev_cleanup( whio_blockdev * self )
{
    if( ! self ) return false;
    if( self->impl.fence )
    {
	self->impl.fence->api->finalize( self->impl.fence );
	self->impl.fence = 0;
    }
    *self = whio_blockdev_init;
    return true;
}


int whio_blockdev_setup( whio_blockdev * self, whio_dev * parent, whio_size_t parent_offset,
			   whio_size_t block_size, whio_size_t count, void const * prototype )
{
    if( ! self || ! parent || ! count || ! block_size ) return whio_rc.ArgError;
    *self = whio_blockdev_init;
    self->impl.fence = whio_dev_subdev_create( parent, parent_offset, parent_offset + (count * block_size) );
    if( ! self->impl.fence ) return whio_rc.AllocError;
    self->blocks.prototype = prototype;
    self->blocks.size = block_size;
    self->blocks.count = count;
    return whio_rc.OK;
}

int whio_blockdev_wipe( whio_blockdev * self, whio_size_t id )
{
    return whio_blockdev_write( self, id, self->blocks.prototype );
}

bool whio_blockdev_in_range( whio_blockdev const * self, whio_size_t id )
{
    return !self
	? false
	: (id < self->blocks.count);
}

/**
   Returns the on-disk position of the given block ID, relative to
   self, or whio_rc.SizeTError if !self or if id is not in range for
   self.
*/
static whio_size_t whio_block_offset_for_id( whio_blockdev * self, whio_size_t id )
{
    return ( ! self || !whio_blockdev_in_range( self, id ) )
	? whio_rc.SizeTError
	: (id * self->blocks.size);
}

int whio_blockdev_write( whio_blockdev * self, whio_size_t id, void const * src )
{
    if( ! src ) return whio_rc.ArgError;
    whio_size_t pos = whio_block_offset_for_id( self, id );
    if( whio_rc.SizeTError == pos )
    {
	WHIO_DEBUG("id #%"PRIu32" is not valid for this whio_blockdev. block count=%"PRIu32"\n",id,self->blocks.count);
	return whio_rc.RangeError;
    }
    if( ! src ) return false;
    if( pos != self->impl.fence->api->seek( self->impl.fence, pos, SEEK_SET ) ) return whio_rc.IOError;
    return (self->blocks.size == self->impl.fence->api->write( self->impl.fence, src, self->blocks.size ))
	? whio_rc.OK
	: whio_rc.IOError;
}

int whio_blockdev_read( whio_blockdev * self, whio_size_t id, void * dest )
{
    whio_size_t pos = whio_block_offset_for_id( self, id );
    if( whio_rc.SizeTError == pos ) return whio_rc.RangeError;
    if( pos != self->impl.fence->api->seek( self->impl.fence, pos, SEEK_SET ) ) return whio_rc.IOError;
    return (self->blocks.size == self->impl.fence->api->read( self->impl.fence, dest, self->blocks.size ))  /*Flawfinder: ignore  Bounds check is mostly done in self->impl.fence->api->read(). Bounds of dest must be ensured by the caller. */
	? whio_rc.OK
	: whio_rc.IOError;
}
