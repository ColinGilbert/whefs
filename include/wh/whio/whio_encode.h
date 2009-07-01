#if !defined(WANDERINGHORSE_NET_WHIO_ENCODE_H_INCLUDED)
#define WANDERINGHORSE_NET_WHIO_ENCODE_H_INCLUDED 1
/*
  Author: Stephan Beal (http://wanderinghorse.net/home/stephan/

  License: Public Domain
*/

#include <wh/whio/whio_dev.h>
#include <stddef.h> /* size_t on my box */
/** @file whio_encode.h

   This file contains an API for encoding/decoding binary data to/from
   memory buffers and i/o devices.
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
   This enum defines some on-disk sizes for the utility routines
   which encode/decode data to/from whio_dev objects.
*/
enum whio_sizeof_encoded {

/** @var whio_sizeof_encoded_uint64

   whio_sizeof_encoded_uint64 is the length (in bytes) of an encoded uint64 value.
   It is 9: 1 tag byte + 8 data bytes.

   @see whio_uint64_decode()
   @see whio_uint64_encode()
*/
whio_sizeof_encoded_uint64 = 9,
/** @var whio_sizeof_encoded_uint32

   whio_sizeof_encoded_uint32 is the length (in bytes) of an encoded uint32 value.
   It is 5: 1 tag byte + 4 data bytes.

   @see whio_uint32_decode()
   @see whio_uint32_encode()
*/
whio_sizeof_encoded_uint32 = 5,

/** @var whio_sizeof_encoded_uint16

   whio_sizeof_encoded_uint16 is the length (in bytes) of an encoded uint16 value.
   It is 3: 1 tag byte + 2 data bytes.

   @see whio_uint16_decode()
   @see whio_uint16_encode()
*/
whio_sizeof_encoded_uint16 = 3,

/** @var whio_sizeof_encoded_uint8

   whio_sizeof_encoded_uint8 is the length (in bytes) of an encoded uint8 value.
   It is 2: 1 tag byte + 1 data byte.

   @see whio_uint8_decode()
   @see whio_uint8_encode()
*/
whio_sizeof_encoded_uint8 = 2,

/** @var whio_size_cstring

   whio_size_cstring is the encoded length of a C-style string,
   NOT INCLUDING the actual string bytes. i.e. this is the header
   size.

   @see whio_cstring_decode()
   @see whio_cstring_encode()
*/
whio_sizeof_encoded_cstring = 1 + whio_sizeof_encoded_uint32,

/**
   The encoded size of a whio_size_t object. Its size depends
   on the value of WHIO_SIZE_T_BITS.
*/
whio_sizeof_encoded_size_t =
#if WHIO_SIZE_T_BITS == 64
    whio_sizeof_encoded_uint64
#elif WHIO_SIZE_T_BITS == 32
    whio_sizeof_encoded_uint32
#elif WHIO_SIZE_T_BITS == 16
    whio_sizeof_encoded_uint16
#elif WHIO_SIZE_T_BITS == 8
    whio_sizeof_encoded_uint8
#else
#error "whio_size_t is not a supported type!"
#endif

};


/**
   Encodes a 32-bit integer value into 5 bytes - a leading tag/check
   byte, then the 4 bytes of the number, in big-endian format. Returns
   the number of bytes written, which will be equal to
   whio_sizeof_encoded_uint32 on success.

   dest must be valid memory at least whio_sizeof_encoded_uint32 bytes long.

   @see whio_uint32_decode()
*/
size_t whio_uint32_encode( unsigned char * dest, uint32_t i );

/**
   The converse of whio_uint32_encode(), this tries to read an
   encoded 32-bit value from the given memory. On success it returns
   whio_rc.OK and sets tgt (if not null) to that value. On error it
   returns some other value from whio_rc and does not modify tgt.

   src must be valid memory at least whio_sizeof_encoded_uint32 bytes
   long.

   Error values include:

   - whio_rc.ArgError = !src

   - whio_rc.ConsistencyError = the bytes at the current location
   were not encoded with whio_uint32_encode().

   @see whio_uint32_encode()

*/
int whio_uint32_decode( unsigned char const * src, uint32_t * tgt );

/**
   Similar to whio_uint32_encode(), with the same conventions, but
   works on 16-bit numbers. Returns the number of bytes written, which
   will be equal to whio_sizeof_encoded_uint16 on success.

   dest must be valid memory at least whio_sizeof_encoded_uint16
   bytes long.

   @see whio_uint16_decode()
*/
size_t whio_uint16_encode( unsigned char * dest, uint16_t i );

/**
   Similar to whio_uint32_decode(), with the same conventions and
   error codes, but works on 16-bit numbers.  On success it returns
   whio_rc.OK and sets target to that value. On error it returns some
   other value from whio_rc and does not modify tgt.

   src must be valid memory at least whio_sizeof_encoded_uint16 bytes
   long.


   @see whio_uint16_encode()
*/

int whio_uint16_decode( unsigned char const * src, uint16_t * tgt );

/**
   The uint8 counterpart of whio_uint16_encode(). Returns
   whio_sizeof_encoded_uint8 on success and 0 on error. The only
   error condition is that dest is null.
*/
size_t whio_uint8_encode( unsigned char * dest, uint8_t i );

/**
   The uint8 counterpart of whio_uint16_decode(). Returns whio_rc.OK
   on success. If !src then whio_rc.ArgError is returned. If src
   does not appear to be an encoded value then whio_rc.ConsistencyError
   is returned.
*/
int whio_uint8_decode( unsigned char const * src, uint8_t * tgt );


/**
   The 64-bit variant of whio_uint32_encode(). Follows the same
   conventions as that function but handles a uint64_t value instead
   of uint32_t.

   @see whio_uint16_encode()
   whio_uint32_encode()
   whio_uint64_decode()
*/
size_t whio_uint64_encode( unsigned char * dest, uint64_t i );

/**
   The 64-bit variant of whio_uint32_decode(). Follows the same
   conventions as that function but handles a uint64_t value instead
   of uint32_t.

   @see whio_uint16_decode()
   whio_uint32_decode()
   whio_uint64_encode()
*/
int whio_uint64_decode( unsigned char const * src, uint64_t * tgt );

/**
   Uses whio_uint32_encode() to write n elements from the given
   array to dev.  Returns whio_rc.OK on success. Returns the number of
   items written.

   @see whio_uint32_encode()
*/
size_t whio_uint32_array_encode( unsigned char * dest, size_t n, uint32_t const * list );

/**
   Reads n consecutive numbers from src, populating list (which must
   point to at least n uint32_t objects) with the results. Returns the
   number of items read, which will be less than n on error.

   @see whio_uint32_decode()
*/
size_t whio_uint32_array_decode( unsigned char const * src, size_t n, uint32_t * list );

/**
   Encodes a C string into the destination by writing a tag byte, the length of
   the string, and then the string bytes. If n is 0 then n is equivalent to
   strlen(s). Zero is also legal string length.

   Returns the number of bytes written, which will be (n +
   whio_sizeof_encoded_cstring) on success, 0 if !dev or !s.

   dest must be at least (n + whio_sizeof_encoded_cstring) bytes long,
   and on success exactly that many bytes will be written. The null
   terminator (if any) is not stored and not counted in the length.
   s may contain null characters.

   @see whio_cstring_decode()
*/
size_t whio_cstring_encode( unsigned char * dest, char const * s, uint32_t n );

/**
   The converse of whio_cstring_encode(), this routine tries to
   decode a string from the given source memory.

   src must contain at least (whio_sizeof_encoded_cstring + N) bytes,
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

   Returns whio_rc.OK on success.

   On error, neither tgt nor length are modified and some non-OK value
   is returned:

   - whio_rc.ArgError = dev or tgt are 0.

   - whio_rc.ConsistencyError = src does not contain a string written
   by whio_cstring_encode().

   Example:

@code
char * str = 0;
size_t len = 0;
int rc = whio_cstring_decode( mySource, &str, &len );
if( whio_rc.OK != rc ) ... error ...
... use str ...
free(str);
@endcode

   @see whio_cstring_encode()
*/
int whio_cstring_decode( unsigned char const * src, char ** tgt, size_t * length );


/**
   Encodes a 32-bit integer value into 5 bytes - a leading tag/check
   byte, then the 4 bytes of the number, in big-endian format. Returns
   the number of bytes written, which will be equal to
   whio_dev_sizeof_uint32 on success.

   @see whio_dev_uint32_decode()
*/
size_t whio_dev_uint32_encode( whio_dev * dev, uint32_t i );

/**
   The converse of whio_dev_uint32_encode(), this tries to read an encoded
   32-bit value from the current position of dev. On success it returns
   whio_rc.OK and sets target to that value. On error it returns some
   other value from whio_rc and does not modify tgt.

   Error values include:

   - whio_rc.ArgError = !dev or !tgt

   - whio_rc.IOError = an error while reading the value (couldn't read enough bytes)

   - whio_rc.ConsistencyError = the bytes at the current location were not encoded
   with whio_dev_uint32_encode().

   @see whio_dev_uint32_encode()

*/
int whio_dev_uint32_decode( whio_dev * dev, uint32_t * tgt );

/**
   Similar to whio_dev_uint32_encode(), with the same conventions, but
   works on 16-bit numbers. Returns the number of bytes written, which
   will be equal to whio_dev_sizeof_uint16 on success.

   @see whio_dev_uint16_decode()
*/
size_t whio_dev_uint16_encode( whio_dev * dev, uint16_t i );

/**
   Similar to whio_dev_uint32_decode(), with the same conventions and
   error codes, but works on 16-bit numbers.  On success it returns
   whio_rc.OK and sets target to that value. On error it returns some
   other value from whio_rc and does not modify tgt.

   @see whio_dev_uint16_encode()
*/

int whio_dev_uint16_decode( whio_dev * dev, uint16_t * tgt );


/**
   The 64-bit variant of whio_dev_uint32_encode(). Follows the same
   conventions as that function but handles a uint64_t value instead
   of uint32_t.

   @see whio_dev_uint16_encode()
   @see whio_dev_uint32_encode()
   @see whio_dev_uint64_decode()
*/
size_t whio_dev_uint64_encode( whio_dev * fs, uint64_t i );

/**
   The 64-bit variant of whio_dev_uint32_decode(). Follows the same
   conventions as that function but handles a uint64_t value instead
   of uint32_t.

   @see whio_dev_uint16_decode()
   @see whio_dev_uint32_decode()
   @see whio_dev_uint64_encode()
*/
int whio_dev_uint64_decode( whio_dev * dev, uint64_t * tgt );

/**
   Uses whio_dev_uint32_encode() to write n elements from the given
   array to dev.  Returns whio_rc.OK on success. Returns the number of
   items written.

   @see whio_dev_uint32_encode()
*/
size_t whio_dev_uint32_array_encode( whio_dev * dev, size_t n, uint32_t const * list );

/**
   Reads n consecutive numbers from dev, populating list (which must
   point to at least n uint32_t objects) with the results. Returns the
   number of items read, which will be less than n on error.

   @see whio_dev_uint32_decode()
*/
size_t whio_dev_uint32_array_decode( whio_dev * dev, size_t n, uint32_t * list );

/**
   Decodes a whio_size_t object from dev. On success whio_rc.OK is returned
   and tgt (if not null) is modified, otherwise tgt is not modified.
*/
int whio_dev_size_t_decode( whio_dev * dev, whio_size_t * tgt );

/**
   Encodes i using whio_size_t_encode(). Returns
   whio_sizeof_encoded_size_t on success.
*/
size_t whio_dev_size_t_encode( whio_dev * fs, whio_size_t i );

/**
   Encodes a C string into the device by writing a tag byte, the length of
   the string, and then the string bytes. If n is 0 then n is equivalent to
   strlen(s). Zero is also legal string length.

   Returns the number of bytes written, which will be (n +
   whio_dev_size_cstring) on success, 0 if !dev or !s.

   @see whio_dev_cstring_decode()
*/
uint32_t whio_dev_cstring_encode( whio_dev * dev, char const * s, uint32_t n );

/**
   The converse of whio_dev_cstring_encode(), this routine tries to
   decode a string from the current location in the device.

   On success, tgt is assigned to the new (null-terminated) string
   (allocated via calloc()) and length (if it is not null) is set to
   the length of the string (not counting the terminating null). The
   caller must free the string using free(). If the string has a
   length of 0 then tgt is set to 0, not "", and no memory is
   allocated.

   Neither dev nor tgt may be 0, but length may be 0.

   Returns whio_rc.OK on success.

   On error, neither tgt nor length are modified and some non-OK value
   is returned:

   - whio_rc.ArgError = dev or tgt are 0.

   - whio_rc.ConsistencyError = current position of the device does not
   appear to be an encoded string written by whio_dev_cstring_encode().

   - whio_rc.IOError = some form of IO error.


   Example:

@code
char * str = 0;
size_t len = 0;
int rc = whio_dev_cstring_decode( myDevice, &str, &len );
if( whio_rc.OK != rc ) ... error ...
... use str ...
free(str);
@endcode


   @see whio_dev_cstring_encode()
*/
int whio_dev_cstring_decode( whio_dev * dev, char ** tgt, uint32_t * length );

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* WANDERINGHORSE_NET_WHIO_ENCODE_H_INCLUDED */
