#if !defined(WANDERINGHORSE_NET_WHEFS_ENCODE_H_INCLUDED)
#define WANDERINGHORSE_NET_WHEFS_ENCODE_H_INCLUDED 1
/*
  Author: Stephan Beal (http://wanderinghorse.net/home/stephan/

  License: Public Domain
*/

#include <wh/whefs/whefs_config.h>
#include <stddef.h> /* size_t on my box */
/**
   This file contains the whefs_inode parts of the whefs
   private/internal API.
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
   This enum defines some on-disk sizes for the utility routines
   which encode/decode data to/from whio_dev objects.
*/
enum whefs_sizeof_encoded {

/** @var whefs_sizeof_encoded_uint64

   whefs_sizeof_encoded_uint64 is the length (in bytes) of an encoded uint64 value.
   It is 9: 1 tag byte + 8 data bytes.

   @see whefs_uint64_decode()
   @see whefs_uint64_encode()
*/
whefs_sizeof_encoded_uint64 = 9,
/** @var whefs_sizeof_encoded_uint32

   whefs_sizeof_encoded_uint32 is the length (in bytes) of an encoded uint32 value.
   It is 5: 1 tag byte + 4 data bytes.

   @see whefs_uint32_decode()
   @see whefs_uint32_encode()
*/
whefs_sizeof_encoded_uint32 = 5,

/** @var whefs_sizeof_encoded_uint16

   whefs_sizeof_encoded_uint16 is the length (in bytes) of an encoded uint16 value.
   It is 3: 1 tag byte + 2 data bytes.

   @see whefs_uint16_decode()
   @see whefs_uint16_encode()
*/
whefs_sizeof_encoded_uint16 = 3,

/** @var whefs_sizeof_encoded_uint8

   whefs_sizeof_encoded_uint8 is the length (in bytes) of an encoded uint8 value.
   It is 2: 1 tag byte + 1 data byte.

   @see whefs_uint8_decode()
   @see whefs_uint8_encode()
*/
whefs_sizeof_encoded_uint8 = 2,

/** @var whefs_size_cstring

   whefs_size_cstring is the encoded length of a C-style string,
   NOT INCLUDING the actual string bytes. i.e. this is the header
   size.

   @see whefs_cstring_decode()
   @see whefs_cstring_encode()
*/
whefs_sizeof_encoded_cstring = 1 + whefs_sizeof_encoded_uint32,

/**
   The on-disk size of an inode record, not including the
   inode name.
*/
whefs_sizeof_encoded_inode = 1 /* tag byte */
        + whefs_sizeof_encoded_id_type /* id */
        + whefs_sizeof_encoded_uint8 /* flags */
	+ whefs_sizeof_encoded_uint32 /* mtime */
	+ whefs_sizeof_encoded_uint32 /* data_size */
        + whefs_sizeof_encoded_id_type /* first_block */,

/**
   This is the on-disk size of the HEADER for an inode name. The
   actual length is this number plus the associated
   whefs_fs_options::filename_length.
*/
whefs_sizeof_encoded_inode_name_header = 1 /* tag byte */
    + whefs_sizeof_encoded_id_type /* id */
    + whefs_sizeof_encoded_uint16 /* length */,

/**
   The size of the internal stack-alloced buffers needed for encoding
   inode name strings, including their metadata.


   Hacker's note: the on-disk size of an encoded inode name for a
   given whefs_fs object can be fetched with whefs_fs_sizeof_name().
*/
whefs_sizeof_encoded_inode_name = 1 /* tag bytes */
+ whefs_sizeof_encoded_inode_name_header
+ WHEFS_MAX_FILENAME_LENGTH
+ 1 /* trailing null */,

/**
   The on-disk size of the metadata parts of a block, which preceeds
   the data area of the block.
*/
whefs_sizeof_encoded_block = 1 /* tag char */
    + whefs_sizeof_encoded_id_type /* bl->id */
    + whefs_sizeof_encoded_uint16 /* bl->flags */
    + whefs_sizeof_encoded_id_type /* bl->next_block */

};

/**
   Encodes a 32-bit integer value into 5 bytes - a leading tag/check
   byte, then the 4 bytes of the number, in big-endian format. Returns
   the number of bytes written, which will be equal to
   whefs_sizeof_encoded_uint32 on success.

   dest must be valid memory at least whefs_sizeof_encoded_uint32 bytes long.

   @see whefs_uint32_decode()
*/
size_t whefs_uint32_encode( unsigned char * dest, uint32_t i );

/**
   The converse of whefs_uint32_encode(), this tries to read an
   encoded 32-bit value from the given memory. On success it returns
   whefs_rc.OK and sets tgt (if not null) to that value. On error it
   returns some other value from whefs_rc and does not modify tgt.

   src must be valid memory at least whefs_sizeof_encoded_uint32 bytes
   long.

   Error values include:

   - whefs_rc.ArgError = !src

   - whefs_rc.ConsistencyError = the bytes at the current location
   were not encoded with whefs_uint32_encode().

   @see whefs_uint32_encode()

*/
int whefs_uint32_decode( unsigned char const * src, uint32_t * tgt );

/**
   Similar to whefs_uint32_encode(), with the same conventions, but
   works on 16-bit numbers. Returns the number of bytes written, which
   will be equal to whefs_sizeof_encoded_uint16 on success.

   dest must be valid memory at least whefs_sizeof_encoded_uint16
   bytes long.

   @see whefs_uint16_decode()
*/
size_t whefs_uint16_encode( unsigned char * dest, uint16_t i );

/**
   Similar to whefs_uint32_decode(), with the same conventions and
   error codes, but works on 16-bit numbers.  On success it returns
   whefs_rc.OK and sets target to that value. On error it returns some
   other value from whefs_rc and does not modify tgt.

   src must be valid memory at least whefs_sizeof_encoded_uint16 bytes
   long.


   @see whefs_uint16_encode()
*/

int whefs_uint16_decode( unsigned char const * src, uint16_t * tgt );

/**
   The uint8 counterpart of whefs_uint16_encode(). Returns
   whefs_sizeof_encoded_uint8 on success and 0 on error. The only
   error condition is that dest is null.
*/
size_t whefs_uint8_encode( unsigned char * dest, uint8_t i );

/**
   The uint8 counterpart of whefs_uint16_decode(). Returns whefs_rc.OK
   on success. If !src then whefs_rc.ArgError is returned. If src
   does not appear to be an encoded value then whefs_rc.ConsistencyError
   is returned.
*/
int whefs_uint8_decode( unsigned char const * src, uint8_t * tgt );


/**
   The 64-bit variant of whefs_uint32_encode(). Follows the same
   conventions as that function but handles a uint64_t value instead
   of uint32_t.

   @see whefs_uint16_encode()
   whefs_uint32_encode()
   whefs_uint64_decode()
*/
size_t whefs_uint64_encode( unsigned char * dest, uint64_t i );

/**
   The 64-bit variant of whefs_uint32_decode(). Follows the same
   conventions as that function but handles a uint64_t value instead
   of uint32_t.

   @see whefs_uint16_decode()
   whefs_uint32_decode()
   whefs_uint64_encode()
*/
int whefs_uint64_decode( unsigned char const * src, uint64_t * tgt );

/**
   Uses whefs_uint32_encode() to write n elements from the given
   array to dev.  Returns whefs_rc.OK on success. Returns the number of
   items written.

   @see whefs_uint32_encode()
*/
size_t whefs_uint32_array_encode( unsigned char * dest, size_t n, uint32_t const * list );

/**
   Reads n consecutive numbers from src, populating list (which must
   point to at least n uint32_t objects) with the results. Returns the
   number of items read, which will be less than n on error.

   @see whefs_uint32_decode()
*/
size_t whefs_uint32_array_decode( unsigned char const * src, size_t n, uint32_t * list );

/**
   Encodes a C string into the destination by writing a tag byte, the length of
   the string, and then the string bytes. If n is 0 then n is equivalent to
   strlen(s). Zero is also legal string length.

   Returns the number of bytes written, which will be (n +
   whefs_sizeof_encoded_cstring) on success, 0 if !dev or !s.

   dest must be at least (n + whefs_sizeof_encoded_cstring) bytes long,
   and on success exactly that many bytes will be written. The null
   terminator (if any) is not stored and not counted in the length.
   s may contain null characters.

   @see whefs_cstring_decode()
*/
size_t whefs_cstring_encode( unsigned char * dest, char const * s, uint32_t n );

/**
   The converse of whefs_cstring_encode(), this routine tries to
   decode a string from the current location in the device.

   src must contain at least (whefs_sizeof_encoded_cstring + N) bytes,
   where N is the number which is encoded in the first part of the data.
   On success exactly that many bytes will be read from src. The null
   terminator (if any) is not stored and not counted in the length.
   s may contain null characters.

   On success, tgt is assigned to the new (null-terminated) string
   (allocated via calloc()) and length (if it is not null) is set to
   the length of the string (not counting the terminating null). The
   caller must free the string using free(). If the string has a
   length of 0 then tgt is set to 0, not "", and no memory is
   allocated.

   Neither dev nor tgt may be 0, but length may be 0.

   Returns whefs_rc.OK on success.

   On error, neither tgt nor length are modified and some non-OK value
   is returned:

   - whefs_rc.ArgError = dev or tgt are 0.

   - whefs_rc.ConsistencyError = src does not contain a string written
   by whefs_cstring_encode().

   Example:

@code
char * str = 0;
size_t len = 0;
int rc = whefs_cstring_decode( mySource, &str, &len );
if( whio_rc.OK != rc ) ... error ...
... use str ...
free(str);
@endcode


   @see whefs_cstring_encode()
*/
int whefs_cstring_decode( unsigned char const * src, char ** tgt, size_t * length );

/**
   Generates a hash code for the first n bytes of the given memory,
   or 0 if n is 0 or data is null.

   The exact hash routine is unspecified, and may change from version
   to version if a compelling reason to do so is found. The hash code
   is intended to be used in ways that will not cause an
   imcompatibility in whefs file format if the hash implementation
   changes.
 */
uint64_t whefs_bytes_hash( void const * data, uint32_t n );
#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* WANDERINGHORSE_NET_WHEFS_ENCODE_H_INCLUDED */
