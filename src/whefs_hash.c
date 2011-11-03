#include <wh/whefs/whefs.h> /* whefs_rc */
#include <stdlib.h> /* qsort(), bsearch() */
#include <string.h> /* memmove() */

#include <assert.h>
#include "whefs_hash.h"
#include "whefs_details.c" /* ONLY for the debugging code. */

const whefs_hashid whefs_hashid_empty = whefs_hashid_empty_m;

whefs_hashval_type whefs_hash_cstring( char const * vstr)
{
#if 1
    /* "djb2" algo code taken from: http://www.cse.yorku.ca/~oz/hash.html */
    whefs_hashval_type hash = 5381;
    int c = 0;
    if( ! vstr ) return 0U;
    while( (c = *vstr++) )
    {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
#else
 /* "shift-and-xor" algo code taken from: http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx

*/
    whefs_hashval_type hash = 0;
    if(vstr) for( ; *vstr; ++vstr )
    {
        hash ^= ( hash << 5 ) + ( hash >> 2 ) + *vstr;
    }
   return hash;
#endif
}

/**
   A compare routine for bsearch(). Compares the hash fields of lhs
   and rhs based on their hash member. They must be (whefs_hashid
   const *).
*/
static int whefs_hashid_cmp( void const * lhs, void const * rhs )
{
    whefs_hashid const * l  = (whefs_hashid const *)lhs;
    whefs_hashid const * r  = (whefs_hashid const *)rhs;
    if( l == r ) return 0;
    else if( !l && r ) return -1;
    else if( l && !r ) return 1;
    else if( l->hash < r->hash ) return -1;
    else if( l->hash > r->hash ) return 1;
    else return 0;    
}

/**
   A compare routine for bsearch(). Compares the hash fields of lhs
   and rhs based on their hits member. They must be (whefs_hashid
   const *)-compatible.
*/
static int whefs_hashid_cmp_hits( void const * lhs, void const * rhs )
{
    whefs_hashid const * l  = (whefs_hashid const *)lhs;
    whefs_hashid const * r  = (whefs_hashid const *)rhs;
    if( ! l->id )
    {
        if( r->id ) return -1;
        return 0;
    }
    return ( l->hits == r->hits )
        ? 0
        : ( ( l->hits < r->hits ) ? -1 : 1);
}

const whefs_hashid_list whefs_hashid_list_empty = whefs_hashid_list_empty_m;

int  whefs_hashid_list_sort( whefs_hashid_list * li )
{
    whefs_id_type off = 0;
    whefs_hashid * h;
    if( ! li ) return whefs_rc.ArgError;
    li->isSorted = true;
    if( li->count < 2 ) return whefs_rc.OK;
    qsort( li->list, li->count, sizeof(whefs_hashid), whefs_hashid_cmp );
#if 1
    /* shave off zeroed items... */
    h = li->list;
    while( ! h->id && (off<li->count) ) { ++off; ++h; }
    /* FIXME???: simply move li->_head, so we can easily re-use those slots in whefs_hashid_list_add(). */
    if( (h != li->list) && (off < li->count) )
    {
        const whefs_id_type tail = li->alloced; /* li->count? */
        const size_t end = sizeof(whefs_hashid)*(tail - off);
        memmove( li->list, h, end );
        memset( li->list + (tail - off), 0, off );
        li->count -= off;
    }
#endif
    return whefs_rc.OK;
}

int whefs_hashid_list_alloc( whefs_hashid_list ** tgt, whefs_id_type toAlloc )
{
    whefs_hashid_list * obj;
    whefs_hashid * li;
    whefs_id_type i;
    if( ! tgt ) return whefs_rc.ArgError;
    if( ! *tgt )
    {
        if( ! toAlloc ) return whefs_rc.OK;
        *tgt = (whefs_hashid_list*) malloc(sizeof(whefs_hashid_list));
        if( ! tgt ) return whefs_rc.AllocError;
        **tgt = whefs_hashid_list_empty;
    }
    obj = *tgt;
    if( 0 == toAlloc )
    {
	if( obj->alloced )
        {
            WHEFS_DBG_CACHE("Freeing whefs_hashid_list->list with %"WHEFS_ID_TYPE_PFMT"/%"WHEFS_ID_TYPE_PFMT" used/allocated items.",obj->count,obj->alloced);
            free( obj->list );
        }
        *obj = whefs_hashid_list_empty;
        obj->isSorted = true;
	return whefs_rc.OK;
    }
    else if( obj->alloced >= toAlloc )
    {
        return whefs_rc.OK;
    }
    /* else realloc... */
    if( obj->maxAlloc )
    {
        if( toAlloc > obj->maxAlloc )
        {
            WHEFS_DBG_ERR("Returning AllocError because toAlloc (%"WHEFS_ID_TYPE_PFMT") >= list->maxAlloc (%"WHEFS_ID_TYPE_PFMT").",
                          toAlloc, obj->maxAlloc);
            return whefs_rc.AllocError;
        }
    }
    li = (whefs_hashid *) realloc( obj->list, toAlloc * sizeof(whefs_hashid) );
    if( ! li ) return whefs_rc.AllocError;
    obj->list = li;
    obj->alloced = toAlloc;
    obj->isSorted = false; /* ???needed/desired??? */
    i = obj->count;
    if( toAlloc < obj->count )
    {
        obj->count = toAlloc;
    }
    for( ; i < toAlloc; ++i )
    {
	obj->list[i] = whefs_hashid_empty;
    }
    return whefs_rc.OK;
}

void whefs_hashid_list_free( whefs_hashid_list * tgt )
{
    if( tgt )
    {
        whefs_hashid_list_alloc( &tgt, 0 );
        free( tgt );
    }
}

int whefs_hashid_list_chomp_lv( whefs_hashid_list * li )
{
    whefs_id_type i;
    if( ! li ) return whefs_rc.ArgError;
    if( (li->count<2)
        || (li->count < (li->alloced/2) )
       )
    {
        return whefs_rc.OK;
    }
    qsort( li->list, li->count, sizeof(whefs_hashid), whefs_hashid_cmp_hits );
    for( i = li->count/2; i < li->count; ++i )
    {
        li->list[i] = whefs_hashid_empty;
    }
    whefs_hashid_list_sort(li); /* re-order by name hash */
    return whefs_rc.OK;
}

int whefs_hashid_list_add( whefs_hashid_list * tgt, whefs_hashid const * val )
{
    if( ! tgt || !val ) return whefs_rc.ArgError;
    tgt->isSorted = false;
    if( tgt->count >= tgt->alloced )
    {
        int rc;
        whefs_id_type sz = (whefs_id_type)((tgt->count+1) * 2);
        if( tgt->maxAlloc && (sz > tgt->maxAlloc ) )
        {
            sz = tgt->maxAlloc;
        }
        if( sz <= tgt->count )
        { /* overflow or incorrect handling of maxAlloc  */
            return whefs_rc.RangeError;
        }
        rc = whefs_hashid_list_alloc( &tgt, sz );
        if( whefs_rc.OK != rc ) return rc;
    }
    tgt->list[tgt->count++] = *val;
    return whefs_rc.OK;
}

whefs_id_type whefs_hashid_list_index_of( whefs_hashid_list const * src, whefs_hashval_type val )
{
    if( ! src || !src->count /*  || !val*/  ) return whefs_rc.IDTypeEnd;
    if( ! src->isSorted )
    { /* horrible special case to avoid having to re-sort on every inode name-set */
#if 1
        whefs_hashid *H = src->list;
	whefs_id_type i;
        WHEFS_DBG_CACHE("Warning: hashid list is unsorted. Running in O(N) here!");
        for( i = 0; H && (i < src->count); ++i, ++H )
        {
            if( H->hash == val )
            {
                ++H->hits;
                return i;
            }
        }
        return whefs_rc.IDTypeEnd;
#endif
    }
    else {
        whefs_id_type ndx;
        whefs_hashid hv = whefs_hashid_empty;
        void const * f;
        hv.hash = val;
        f = bsearch( &hv, src->list, src->count, sizeof(whefs_hashid), whefs_hashid_cmp );
        if( ! f ) return whefs_rc.IDTypeEnd;
        ndx = (((unsigned char const *)f) -((unsigned char const *)src->list)) / sizeof(whefs_hashid);
        while( ndx && (src->list[ndx-1].hash == val) ) --ndx;
        ++(src->list[ndx].hits);
        WHEFS_DBG_CACHE("Index of hash %"WHEFS_HASHVAL_TYPE_PFMT" = %"WHEFS_ID_TYPE_PFMT, val, ndx);
        return ndx;
    }
}

size_t whefs_hashid_list_sizeof( whefs_hashid_list const * li )
{
    if( ! li ) return 0;
    return sizeof(whefs_hashid_list)
        + (sizeof(whefs_hashid) * li->alloced);
}

int whefs_hashid_list_wipe_index( whefs_hashid_list * tgt, whefs_id_type ndx )
{
    if( ! tgt ) return whefs_rc.ArgError;
    if( tgt->count <= ndx ) return whefs_rc.RangeError;
    tgt->list[ndx] = whefs_hashid_empty;
    tgt->isSorted = false;
    return whefs_rc.OK;
}

int whefs_hashid_list_add_slots( whefs_hashid_list * li, whefs_id_type pos, whefs_id_type count )
{
    /*assert(0 && "Not finished!"); */
    if( ! li || !count ) return whefs_rc.ArgError;
    if( !li->count || (pos>li->count) ) return whefs_rc.RangeError;
    else {
        whefs_id_type last = pos+count;
        whefs_id_type asz = last+1;
        void * from;
        void * to;
        whefs_id_type tondx;
        whefs_id_type howmany;
        whefs_id_type i;
        if( asz < pos /* overflow */ ) return whefs_rc.RangeError;
        if( li->alloced < asz )
        {
            int rc = whefs_hashid_list_alloc( &li, asz );
            if( whefs_rc.OK != rc ) return rc;
        }
        li->isSorted = false;
        from = &li->list[pos];
        tondx = pos + (li->count-pos);
        li->count += count;
        howmany = tondx - pos;
        to = &li->list[last];
        memmove( to, from, sizeof(whefs_hashid) * howmany );
        for( i = pos; i < last; ++i )
        {
            li->list[i] = whefs_hashid_empty;
        }
        return whefs_rc.OK;
    }
}


