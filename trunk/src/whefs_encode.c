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
#include "whefs_encode.h"
#include <wh/whefs/whefs.h> /* only for whefs_rc */
#include <stdlib.h> /* calloc() */

static const unsigned char whefs_uint64_tag_char = 0x80 | 64;
size_t whefs_uint64_encode( unsigned char * dest, uint64_t i )
{
    if( ! dest ) return 0;
    static const uint64_t mask = UINT64_C(0x00ff);
    size_t x = 0;
    dest[x++] = whefs_uint64_tag_char;
    dest[x++] = (unsigned char)((i>>56) & mask);
    dest[x++] = (unsigned char)((i>>48) & mask);
    dest[x++] = (unsigned char)((i>>40) & mask);
    dest[x++] = (unsigned char)((i>>32) & mask);
    dest[x++] = (unsigned char)((i>>24) & mask);
    dest[x++] = (unsigned char)((i>>16) & mask);
    dest[x++] = (unsigned char)((i>>8) & mask);
    dest[x++] = (unsigned char)(i & mask);
    return whefs_sizeof_encoded_uint64;
}


int whefs_uint64_decode( unsigned char const * src, uint64_t * tgt )
{
    if( ! src || ! tgt ) return whefs_rc.ArgError;
    if( whefs_uint64_tag_char != src[0] )
    {
	return whefs_rc.ConsistencyError;
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
    return whefs_rc.OK;
}


static const unsigned char whefs_uint32_tag_char = 0x80 | 32;
size_t whefs_uint32_encode( unsigned char * dest, uint32_t i )
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
    dest[x++] = whefs_uint32_tag_char;
    dest[x++] = (unsigned char)(i>>24) & mask;
    dest[x++] = (unsigned char)(i>>16) & mask;
    dest[x++] = (unsigned char)(i>>8) & mask;
    dest[x++] = (unsigned char)(i & mask);
    return whefs_sizeof_encoded_uint32;
}

int whefs_uint32_decode( unsigned char const * src, uint32_t * tgt )
{
    if( ! src ) return whefs_rc.ArgError;
    if( whefs_uint32_tag_char != src[0] )
    {
	//WHIO_DEBUG("read bytes are not an encoded integer value!\n");
	return whefs_rc.ConsistencyError;
    }
    uint32_t val = (src[1] << 24)
	+ (src[2] << 16)
	+ (src[3] << 8)
	+ (src[4]);
    if( tgt ) *tgt = val;
    return whefs_rc.OK;
}

static const unsigned char whefs_uint16_tag_char = 0x80 | 16;
size_t whefs_uint16_encode( unsigned char * dest, uint16_t i )
{
    if( ! dest ) return 0;
    static const uint16_t mask = 0x00ff;
    uint8_t x = 0;
    dest[x++] = whefs_uint16_tag_char;
    dest[x++] = (unsigned char)(i>>8) & mask;
    dest[x++] = (unsigned char)(i & mask);
    return whefs_sizeof_encoded_uint16;
}

int whefs_uint16_decode( unsigned char const * src, uint16_t * tgt )
{
    if( ! src ) return whefs_rc.ArgError;
    if( whefs_uint16_tag_char != src[0] )
    {
	return whefs_rc.ConsistencyError;
    }
    uint16_t val = + (src[1] << 8)
	+ (src[2]);
    *tgt = val;
    return whefs_rc.OK;
}

static const unsigned char whefs_uint8_tag_char = 0x80 | 8;
size_t whefs_uint8_encode( unsigned char * dest, uint8_t i )
{
    if( ! dest ) return 0;
    dest[0] = whefs_uint8_tag_char;
    dest[1] = i;
    return whefs_sizeof_encoded_uint8;
}

int whefs_uint8_decode( unsigned char const * src, uint8_t * tgt )
{
    if( ! src ) return whefs_rc.ArgError;
    if( whefs_uint8_tag_char != src[0] )
    {
	return whefs_rc.ConsistencyError;
    }
    *tgt = src[1];
    return whefs_rc.OK;
}


size_t whefs_uint32_array_encode( unsigned char * dest, size_t n, uint32_t const * list )
{
    size_t i = (dest && n && list) ? 0 : n;
    size_t rc = 0;
    for( ; i < n; ++i, ++rc )
    {
	if( whefs_sizeof_encoded_uint32 != whefs_uint32_encode( dest, *(list++) ) )
	{
	    break;
	}
    }
    return rc;
}

size_t whefs_uint32_array_decode( unsigned char const * src, size_t n, uint32_t * list )
{
    size_t i = (src && n && list) ? 0 : n;
    size_t rc = 0;
    for( ; i < n; ++i, ++rc )
    {
	if( whefs_rc.OK != whefs_uint32_decode( src, &list[i] ) )
	{
	    break;
	}
    }
    return rc;
}

static const unsigned char whefs_cstring_tag_char = 0x80 | '"';
size_t whefs_cstring_encode( unsigned char * dest, char const * s, uint32_t n )
{
    if( ! dest || !s ) return 0;
    if( ! n )
    {
	char const * x = s;
	loop: if( x && *x ) { ++x; ++n; goto loop; }
	//for( ; x && *x ; ++x, ++n ){}
    }
    *(dest++) = whefs_cstring_tag_char;
    size_t rc = whefs_uint32_encode( dest, n );
    if( whefs_sizeof_encoded_uint32 != rc ) return rc;
    dest += rc;
    rc = 1 + whefs_sizeof_encoded_uint32;
    size_t i = 0;
    for( ; i < n; ++i, ++rc )
    {
	*(dest++) = *(s++);
    }
    *dest = 0;
    return rc;
}

int whefs_cstring_decode( unsigned char const * src, char ** tgt, size_t * length )
{
    if( !src || ! tgt ) return whio_rc.ArgError;

    if( whefs_cstring_tag_char != *src )
    {
	return whio_rc.ConsistencyError;
    }
    ++src;
    uint32_t slen = 0;
    int rc = whefs_uint32_decode( src, &slen );
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
    src += whefs_sizeof_encoded_uint32;
    for(  ; i < slen; ++i )
    {
	*(buf++) = *(src++);
    }
    *buf = 0;
    return whio_rc.OK;
}

uint64_t whefs_bytes_hash( void const * data, uint32_t len )
{
    /**
       One-at-a-time hash code taken from:

       http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx
    */
    if( ! data || !len ) return 0;
    unsigned const char *p = data;
    uint64_t h = 0;
    uint32_t i;
    for ( i = 0; i < len; i++ )
    {
	h += p[i];
	h += ( h << 10 );
	h ^= ( h >> 6 );
    }
    h += ( h << 3 );
    h ^= ( h >> 11 );
    h += ( h << 15 );
    return h;
}

/**
   tag byte for encoded whefs_id_type objects.
*/
static const unsigned int whefs_id_type_tag_char = 0x08 | 8;
size_t whefs_dev_id_encode( whio_dev * dev, whefs_id_type v )
{
#if WHEFS_ID_TYPE_BITS == 64
    return whio_dev_uint64_encode( dev, v );
#elif WHEFS_ID_TYPE_BITS == 32
    return whio_dev_uint32_encode( dev, v );
#elif WHEFS_ID_TYPE_BITS == 16
    return whio_dev_uint16_encode( dev, v );
#elif WHEFS_ID_TYPE_BITS == 8
    if( ! dev ) return whefs_rc.ArgError;
    unsigned char buf[2];
    buf[0] = whefs_id_type_tag_char;
    buf[1] = v;
    return dev->api->write( dev, buf, 2 );
#else
#error "whefs_id_type size (WHEFS_ID_TYPE_BITS) is not supported!"
#endif
}

size_t whefs_id_encode( unsigned char * dest, whefs_id_type v )
{
#if WHEFS_ID_TYPE_BITS == 64
    return whefs_uint64_encode( dest, v );
#elif WHEFS_ID_TYPE_BITS == 32
    return whefs_uint32_encode( dest, v );
#elif WHEFS_ID_TYPE_BITS == 16
    return whefs_uint16_encode( dest, v );
#elif WHEFS_ID_TYPE_BITS == 8
    if( ! dest ) return whefs_rc.ArgError;
    dest[0] = whefs_id_type_tag_char;
    dest[1] = v;
    return whefs_sizeof_encoded_id_type;
#else
#error "whefs_id_type size (WHEFS_ID_TYPE_BITS) is not supported!"
#endif
}
int whefs_dev_id_decode( whio_dev * dev, whefs_id_type * v )
{
#if WHEFS_ID_TYPE_BITS == 64
    return whio_dev_uint64_decode( dev, v );
#elif WHEFS_ID_TYPE_BITS == 32
    return whio_dev_uint32_decode( dev, v );
#elif WHEFS_ID_TYPE_BITS == 16
    return whio_dev_uint16_decode( dev, v );
#elif WHEFS_ID_TYPE_BITS == 8
    if( ! v || ! dev ) return whefs_rc.ArgError;
    unsigned char buf[2] = {0,0};
    size_t sz = dev->api->read( dev, buf, 2 );
    if( 2 != sz)
    {
	return whefs_rc.IOError;
    }
    else if( buf[0] != whefs_id_type_tag_char )
    {
	return whefs_rc.ConsistencyError;
    }
    else
    {
	*v = buf[1];
	return whefs_rc.OK;
    }
#else
#error "whefs_id_type is not a supported type!"
#endif
}

int whefs_id_decode( unsigned char const * src, whefs_id_type * v )
{
#if WHEFS_ID_TYPE_BITS == 64
    return whefs_uint64_decode( src, v );
#elif WHEFS_ID_TYPE_BITS == 32
    return whefs_uint32_decode( src, v );
#elif WHEFS_ID_TYPE_BITS == 16
    return whefs_uint16_decode( src, v );
#elif WHEFS_ID_TYPE_BITS == 8
    if( ! src ) return whefs_rc.ArgError;
    else if( src[0] != whefs_id_type_tag_char )
    {
	return whefs_rc.ConsistencyError;
    }
    else
    {
	if( v ) *v = src[1];
    }
    return whefs_rc.OK;
#else
#error "whefs_id_type is not a supported type!"
#endif
}

