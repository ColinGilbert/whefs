/*
  Author: Stephan Beal (http://wanderinghorse.net/home/stephan/

  License: Public Domain
*/

/* Note from linux stdint.h:

   The ISO C99 standard specifies that in C++ implementations these
   should only be defined if explicitly requested. 
*/
#if defined __cplusplus && ! defined __STDC_CONSTANT_MACROS
#  define __STDC_CONSTANT_MACROS
#endif
#include <wh/whio/whio_encode.h>
#include <wh/whio/whio_common.h> /* only for whio_rc */
#include <stdlib.h> /* calloc() */
#include <string.h> /* memset() */
static const unsigned char whio_uint64_tag_char = 0x80 | 64;
size_t whio_uint64_encode( unsigned char * dest, uint64_t i )
{
    if( ! dest ) return 0;
    static const uint64_t mask = UINT64_C(0x00ff);
    size_t x = 0;
    dest[x++] = whio_uint64_tag_char;
    dest[x++] = (unsigned char)((i>>56) & mask);
    dest[x++] = (unsigned char)((i>>48) & mask);
    dest[x++] = (unsigned char)((i>>40) & mask);
    dest[x++] = (unsigned char)((i>>32) & mask);
    dest[x++] = (unsigned char)((i>>24) & mask);
    dest[x++] = (unsigned char)((i>>16) & mask);
    dest[x++] = (unsigned char)((i>>8) & mask);
    dest[x++] = (unsigned char)(i & mask);
    return whio_sizeof_encoded_uint64;
}


int whio_uint64_decode( unsigned char const * src, uint64_t * tgt )
{
    if( ! src || ! tgt ) return whio_rc.ArgError;
    if( whio_uint64_tag_char != src[0] )
    {
	return whio_rc.ConsistencyError;
    }
#define SHIFT(X) ((uint64_t)src[X])
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


static const unsigned char whio_uint32_tag_char = 0x80 | 32;
size_t whio_uint32_encode( unsigned char * dest, uint32_t i )
{
    if( ! dest ) return 0;
    static const unsigned short mask = 0x00ff;
    size_t x = 0;
    /** We tag all entries with a prefix mainly to simplify debugging
	of the vfs files (it's easy to spot them in a file viewer),
	but it incidentally also gives us a sanity-checker at
	read-time (we simply confirm that the first byte is this
	prefix).
    */
    dest[x++] = whio_uint32_tag_char;
    dest[x++] = (unsigned char)(i>>24) & mask;
    dest[x++] = (unsigned char)(i>>16) & mask;
    dest[x++] = (unsigned char)(i>>8) & mask;
    dest[x++] = (unsigned char)(i & mask);
    return whio_sizeof_encoded_uint32;
}

int whio_uint32_decode( unsigned char const * src, uint32_t * tgt )
{
    if( ! src ) return whio_rc.ArgError;
    if( whio_uint32_tag_char != src[0] )
    {
	//WHIO_DEBUG("read bytes are not an encoded integer value!\n");
	return whio_rc.ConsistencyError;
    }
    uint32_t val = (src[1] << 24)
	+ (src[2] << 16)
	+ (src[3] << 8)
	+ (src[4]);
    if( tgt ) *tgt = val;
    return whio_rc.OK;
}

static const unsigned char whio_uint16_tag_char = 0x80 | 16;
size_t whio_uint16_encode( unsigned char * dest, uint16_t i )
{
    if( ! dest ) return 0;
    static const uint16_t mask = 0x00ff;
    uint8_t x = 0;
    dest[x++] = whio_uint16_tag_char;
    dest[x++] = (unsigned char)(i>>8) & mask;
    dest[x++] = (unsigned char)(i & mask);
    return whio_sizeof_encoded_uint16;
}

int whio_uint16_decode( unsigned char const * src, uint16_t * tgt )
{
    if( ! src ) return whio_rc.ArgError;
    if( whio_uint16_tag_char != src[0] )
    {
	return whio_rc.ConsistencyError;
    }
    uint16_t val = + (src[1] << 8)
	+ (src[2]);
    *tgt = val;
    return whio_rc.OK;
}

static const unsigned char whio_uint8_tag_char = 0x80 | 8;
size_t whio_uint8_encode( unsigned char * dest, uint8_t i )
{
    if( ! dest ) return 0;
    dest[0] = whio_uint8_tag_char;
    dest[1] = i;
    return whio_sizeof_encoded_uint8;
}

int whio_uint8_decode( unsigned char const * src, uint8_t * tgt )
{
    if( ! src ) return whio_rc.ArgError;
    if( whio_uint8_tag_char != src[0] )
    {
	return whio_rc.ConsistencyError;
    }
    *tgt = src[1];
    return whio_rc.OK;
}


size_t whio_uint32_array_encode( unsigned char * dest, size_t n, uint32_t const * list )
{
    size_t i = (dest && n && list) ? 0 : n;
    size_t rc = 0;
    for( ; i < n; ++i, ++rc )
    {
	if( whio_sizeof_encoded_uint32 != whio_uint32_encode( dest, *(list++) ) )
	{
	    break;
	}
    }
    return rc;
}

size_t whio_uint32_array_decode( unsigned char const * src, size_t n, uint32_t * list )
{
    size_t i = (src && n && list) ? 0 : n;
    size_t rc = 0;
    for( ; i < n; ++i, ++rc )
    {
	if( whio_rc.OK != whio_uint32_decode( src, &list[i] ) )
	{
	    break;
	}
    }
    return rc;
}

static const unsigned char whio_cstring_tag_char = 0x80 | '"';
size_t whio_cstring_encode( unsigned char * dest, char const * s, uint32_t n )
{
    if( ! dest || !s ) return 0;
    if( ! n )
    {
	char const * x = s;
	loop: if( x && *x ) { ++x; ++n; goto loop; }
	//for( ; x && *x ; ++x, ++n ){}
    }
    *(dest++) = whio_cstring_tag_char;
    size_t rc = whio_uint32_encode( dest, n );
    if( whio_sizeof_encoded_uint32 != rc ) return rc;
    dest += rc;
    rc = 1 + whio_sizeof_encoded_uint32;
    size_t i = 0;
    for( ; i < n; ++i, ++rc )
    {
	*(dest++) = *(s++);
    }
    *dest = 0;
    return rc;
}

int whio_cstring_decode( unsigned char const * src, char ** tgt, size_t * length )
{
    if( !src || ! tgt ) return whio_rc.ArgError;

    if( whio_cstring_tag_char != *src )
    {
	return whio_rc.ConsistencyError;
    }
    ++src;
    uint32_t slen = 0;
    int rc = whio_uint32_decode( src, &slen );
    if( whio_rc.OK != rc ) return rc;
    if( ! slen )
    {
	*tgt = 0;
	if(length) *length = 0;
	return whio_rc.OK;
    }
    char * buf = (char *)calloc( slen + 1, sizeof(char) );
    if( ! buf ) return whio_rc.AllocError;
    if( length ) *length = slen;
    *tgt = buf;
    size_t i = 0;
    src += whio_sizeof_encoded_uint32;
    for(  ; i < slen; ++i )
    {
	*(buf++) = *(src++);
    }
    *buf = 0;
    return whio_rc.OK;
}

/**
   tag byte for encoded whio_id_type objects.
*/
static const unsigned int whio_size_t_tag_char = 0x08 | 'p';
size_t whio_dev_size_t_encode( whio_dev * dev, whio_size_t v )
{
#if WHIO_SIZE_T_BITS == 64
    return whio_dev_uint64_encode( dev, v );
#elif WHIO_SIZE_T_BITS == 32
    return whio_dev_uint32_encode( dev, v );
#elif WHIO_SIZE_T_BITS == 16
    return whio_dev_uint16_encode( dev, v );
#elif WHIO_SIZE_T_BITS == 8
    if( ! dev ) return whio_rc.ArgError;
    unsigned char buf[2];
    buf[0] = whio_size_t_tag_char;
    buf[1] = v;
    return dev->api->write( dev, buf, 2 );
#else
#error "whio_size_t size (WHIO_SIZE_T_BITS) is not supported!"
#endif
}

int whio_dev_size_t_decode( whio_dev * dev, whio_size_t * v )
{
#if WHIO_SIZE_T_BITS == 64
    return whio_dev_uint64_decode( dev, v );
#elif WHIO_SIZE_T_BITS == 32
    return whio_dev_uint32_decode( dev, v );
#elif WHIO_SIZE_T_BITS == 16
    return whio_dev_uint16_decode( dev, v );
#elif WHIO_SIZE_T_BITS == 8
    if( ! v || ! dev ) return whio_rc.ArgError;
    unsigned char buf[2] = {0,0};
    size_t sz = dev->api->read( dev, buf, 2 );
    if( 2 != sz)
    {
	return whio_rc.IOError;
    }
    else if( buf[0] != whio_size_t_tag_char )
    {
	return whio_rc.ConsistencyError;
    }
    else
    {
	*v = buf[1];
	return whio_rc.OK;
    }
#else
#error "whio_size_t is not a supported type!"
#endif
}

size_t whio_size_t_encode( unsigned char * dest, whio_size_t v )
{
#if WHIO_SIZE_T_BITS == 64
    return whio_uint64_encode( dest, v );
#elif WHIO_SIZE_T_BITS == 32
    return whio_uint32_encode( dest, v );
#elif WHIO_SIZE_T_BITS == 16
    return whio_uint16_encode( dest, v );
#elif WHIO_SIZE_T_BITS == 8
    if( ! dest ) return whio_rc.ArgError;
    dest[0] = whio_size_t_tag_char;
    dest[1] = v;
    return whio_sizeof_encoded_size_t;
#else
#error "whio_size_t size (WHIO_SIZE_T_BITS) is not supported!"
#endif
}

int whio_size_t_decode( unsigned char const * src, whio_size_t * v )
{
#if WHIO_SIZE_T_BITS == 64
    return whio_uint64_decode( src, v );
#elif WHIO_SIZE_T_BITS == 32
    return whio_uint32_decode( src, v );
#elif WHIO_SIZE_T_BITS == 16
    return whio_uint16_decode( src, v );
#elif WHIO_SIZE_T_BITS == 8
    if( ! src ) return whio_rc.ArgError;
    else if( src[0] != whio_size_t_tag_char )
    {
	return whio_rc.ConsistencyError;
    }
    else
    {
	if( v ) *v = src[1];
    }
    return whio_rc.OK;
#else
#error "whio_size_t is not a supported type!"
#endif
}

size_t whio_dev_uint64_encode( whio_dev * dev, uint64_t i )
{
    unsigned char buf[whio_sizeof_encoded_uint64]; /* Flawfinder: ignore This is intentional and safe as long as whio_sizeof_encoded_uint64 is the correct size. */
    size_t const x = whio_uint64_encode( buf, i );
    return ( whio_sizeof_encoded_uint64 == x )
        ? whio_dev_write( dev, buf, whio_sizeof_encoded_uint64 )
        : 0;
}

int whio_dev_uint64_decode( whio_dev * dev, uint64_t * tgt )
{
    if( ! dev || ! tgt ) return whio_rc.ArgError;
    unsigned char buf[whio_sizeof_encoded_uint64]; /* Flawfinder: ignore This is intentional and safe as long as whio_sizeof_encoded_uint64 is the correct size. */
    memset( buf, 0, whio_sizeof_encoded_uint64 );
    size_t rc = dev->api->read( dev, buf  /*Flawfinder: ignore  This is safe in conjunction with whio_sizeof_encoded_uint64*/, whio_sizeof_encoded_uint64 );
    return ( whio_sizeof_encoded_uint64 == rc )
        ? whio_uint64_decode( buf, tgt )
        : whio_rc.IOError;
}


size_t whio_dev_uint32_encode( whio_dev * dev, uint32_t i )
{
    unsigned char buf[whio_sizeof_encoded_uint32]; /* Flawfinder: ignore This is intentional and safe as long as whio_sizeof_encoded_uint32 is the correct size. */
    size_t const x = whio_uint32_encode( buf, i );
    return ( whio_sizeof_encoded_uint32 == x )
        ? whio_dev_write( dev, buf, whio_sizeof_encoded_uint32 )
        : 0;
}

int whio_dev_uint32_decode( whio_dev * dev, uint32_t * tgt )
{
    if( ! dev || ! tgt ) return whio_rc.ArgError;
    unsigned char buf[whio_sizeof_encoded_uint32]; /* Flawfinder: ignore This is intentional and safe as long as whio_sizeof_encoded_uint32 is the correct size. */
    memset( buf, 0, whio_sizeof_encoded_uint32 );
    size_t rc = dev->api->read( dev, buf  /*Flawfinder: ignore  This is safe in conjunction with whio_sizeof_encoded_uint32*/, whio_sizeof_encoded_uint32 );
    return ( whio_sizeof_encoded_uint32 == rc )
        ? whio_uint32_decode( buf, tgt )
        : whio_rc.IOError;
}


static const unsigned char whio_dev_uint16_tag_char = 0x80 | 16;
size_t whio_dev_uint16_encode( whio_dev * dev, uint16_t i )
{
    unsigned char buf[whio_sizeof_encoded_uint16]; /* Flawfinder: ignore This is intentional and safe as long as whio_sizeof_encoded_uint16 is the correct size. */
    size_t const x = whio_uint16_encode( buf, i );
    return ( whio_sizeof_encoded_uint16 == x )
        ? whio_dev_write( dev, buf, whio_sizeof_encoded_uint16 )
        : 0;
}

int whio_dev_uint16_decode( whio_dev * dev, uint16_t * tgt )
{
    if( ! dev || ! tgt ) return whio_rc.ArgError;
    unsigned char buf[whio_sizeof_encoded_uint16]; /* Flawfinder: ignore This is intentional and safe as long as whio_sizeof_encoded_uint16 is the correct size. */
    memset( buf, 0, whio_sizeof_encoded_uint16 );
    size_t rc = dev->api->read( dev, buf  /*Flawfinder: ignore  This is safe in conjunction with whio_sizeof_encoded_uint16*/, whio_sizeof_encoded_uint16 );
    return ( whio_sizeof_encoded_uint16 == rc )
        ? whio_uint16_decode( buf, tgt )
        : whio_rc.IOError;
}

size_t whio_dev_uint32_array_encode( whio_dev * dev, size_t n, uint32_t const * list )
{
    size_t i = (dev && n && list) ? 0 : n;
    size_t rc = 0;
    for( ; i < n; ++i, ++rc )
    {
	if( whio_sizeof_encoded_uint32 != whio_dev_uint32_encode( dev, *(list++) ) )
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
    if( (1 + whio_sizeof_encoded_uint32) != rc ) return rc;
    return rc + dev->api->write( dev, s, (whio_size_t)n );
}

int whio_dev_cstring_decode( whio_dev * dev, char ** tgt, uint32_t * length )
{
    if( !dev || ! tgt ) return whio_rc.ArgError;

    { /* check tag byte */
	unsigned char tagbuf[1] = {0}; /* Flawfinder: ignore  This is intended and safe. */
	whio_size_t const sz = dev->api->read( dev, tagbuf, 1 );/*Flawfinder: ignore  This is safe used safely.*/
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
