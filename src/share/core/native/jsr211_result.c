/*
 * 
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 */


/**
 * @file jsr211_chapi_result.c
 * @ingroup CHAPI
 * @brief javacall registry access implementation
 */


//#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <javacall_defs.h>

#include <jsrop_memory.h> 
#include <jsrop_suitestore.h> 

#include "jsr211_result.h"

#ifdef _DEBUG
#define TRACE_DATA_OPS
#endif

#define BUFFER_GRANULARITY 0x100
#define LEVELS_COUNT       0x4

#define CHECKRC( e ) \
    if( JSR211_OK != (rc = (e)) ) return rc \

/**
 * Internal structure.
 * Handle to result buffer for serialized data storage.
 */
typedef struct _DATA_BUFFER_{
    size_t  size;
    size_t  bytes_used;
    size_t  level;
    size_t  size_offset[ LEVELS_COUNT ];
    unsigned char   data[0];
} DATA_BUFFER;

typedef unsigned short size_type;

void jsr211_clean_buffer( DATA_BUFFER** buffer );
jsr211_result jsr211_add_level( DATA_BUFFER ** buffer );

JSR211_RESULT_BUFFER jsr211_create_result_buffer(){
    DATA_BUFFER* res = (DATA_BUFFER*)MALLOC( BUFFER_GRANULARITY );
#ifdef TRACE_DATA_OPS
    printf( "jsr211_result: create buffer %p\n", res );
#endif
	if (res == NULL) return NULL;
    memset( res, '\0', BUFFER_GRANULARITY );
	res->size = BUFFER_GRANULARITY - sizeof(DATA_BUFFER);
    jsr211_clean_buffer( &res );
	return (JSR211_RESULT_BUFFER)res;
}

void jsr211_release_result_buffer(JSR211_RESULT_BUFFER resbuf){
#ifdef TRACE_DATA_OPS
    printf( "jsr211_result: release buffer %p\n", resbuf );
#endif
    if (resbuf != NULL) FREE( resbuf );
}

/**
 * Assure result buffer (<code>resbuf</code>) capacity to append additional 
 * portion of data by <code>ext</code> javacall_utf16 units.
 */
static jsr211_result assureBufferCap(DATA_BUFFER** resbuf, size_t ext){

#define ALWAYS_RESERVED_TAIL_BYTES sizeof(jchar)
    ext += ALWAYS_RESERVED_TAIL_BYTES;
#undef ALWAYS_RESERVED_TAIL_BYTES

    if ((*resbuf)->bytes_used + ext > (*resbuf)->size) {
        // calculate new size
        size_t sz = ((sizeof(DATA_BUFFER) + (*resbuf)->bytes_used + ext) / BUFFER_GRANULARITY + 1) * 
                        BUFFER_GRANULARITY;
		DATA_BUFFER* tmp = (DATA_BUFFER*)REALLOC(*resbuf, sz);
#ifdef TRACE_DATA_OPS
        printf( "jsr211_result: assureBufferCap %p -> %p\n", *resbuf, tmp );
#endif
        if (tmp == NULL) return JSR211_FAILED;
        tmp->size = sz - sizeof(DATA_BUFFER);
        *resbuf = tmp;
    }
    return JSR211_OK;
}

void jsr211_clean_buffer( DATA_BUFFER** buffer ) {
    (**buffer).bytes_used = 0;
    (**buffer).level = 0;
    jsr211_add_level( buffer );
}

static void jsr211_inc( DATA_BUFFER * buffer, size_t length ) {
    size_t l;
    buffer->bytes_used += length;
    // increase levels lengths
    for( l = buffer->level; l--; )
        *(size_type *)(buffer->data + buffer->size_offset[ l ]) += length;
}

jsr211_result jsr211_append_data(DATA_BUFFER** buffer, const void * data, size_t length ) {
    jsr211_result rc = assureBufferCap(buffer, sizeof(size_type) + length);
    if( rc == JSR211_OK ){
        DATA_BUFFER * b = *buffer; 
#ifdef TRACE_DATA_OPS
        printf( "jsr211_result: append data %p, length = %d", 
                        *buffer, length );
#endif
        *(size_type *)(b->data + b->bytes_used) = (size_type)length;
        jsr211_inc( b, sizeof(size_type) );
        memcpy( b->data + b->bytes_used, data, length );
        jsr211_inc( b, length );
#ifdef TRACE_DATA_OPS
        printf( ", bytes_used = %d\n", b->bytes_used );
#endif
    }
    return rc;
}

jsr211_result jsr211_add_level( DATA_BUFFER ** buffer ) {
    jsr211_result rc;
    DATA_BUFFER * b = *buffer;
#ifdef TRACE_DATA_OPS
    printf( "jsr211_result: add level %p\n", b );
#endif
    if( b->level >= sizeof(b->size_offset)/sizeof(b->size_offset[0]) )
        return JSR211_FAILED;
    CHECKRC( assureBufferCap(buffer, sizeof(size_type) ) );
    b = *buffer;
    b->size_offset[ b->level ] = b->bytes_used;
    jsr211_inc( b, sizeof(size_type) );
    b->level++;
    return JSR211_OK;
}

jsr211_result jsr211_drop_level( DATA_BUFFER * buffer ) {
#ifdef TRACE_DATA_OPS
    printf( "jsr211_result: drop level %p\n", buffer );
#endif
    if( buffer->level == 0 )
        return JSR211_FAILED;
    buffer->level--;
    return JSR211_OK;
}

JSR211_BUFFER_DATA jsr211_get_result_data(JSR211_RESULT_BUFFER resbuf){
    if (resbuf == NULL) return NULL;
    return ((DATA_BUFFER*)resbuf)->data;
}

void jsr211_get_data( JSR211_BUFFER_DATA handle, const void * * data, size_t * length ){
    *length = *(size_type *)handle;
    *data = (size_type *)handle + 1;
}

JSR211_ENUM_HANDLE jsr211_get_enum_handle( JSR211_BUFFER_DATA data_handle ){
    JSR211_ENUM_HANDLE eh;
    const void * data; size_t length;
    jsr211_get_data( data_handle, &data, &length );
    eh.eptr = (unsigned char *)data + length;
    eh.handle = (unsigned char *)data;
    return eh;
}

JSR211_BUFFER_DATA jsr211_get_next( JSR211_ENUM_HANDLE * eh ){
    JSR211_BUFFER_DATA result = eh->handle;
    const void * data; size_t length;
    if( result >= eh->eptr ) return NULL;
    jsr211_get_data( result, &data, &length );
    eh->handle = (unsigned char *)data + length;
    return result;
}

//---------------------------------------------------------

/**
 * Appends string to output string array.
 * @param str appended string
 * @param str_size the string size
 * @param array string array.
 * @return operation status.
 */
jsr211_result jsr211_appendString( const jchar* str, size_t str_size, /*OUT*/ JSR211_RESULT_STRARRAY array) {
    return jsr211_append_data( (DATA_BUFFER **)array, str, str_size * sizeof(str[0]) );
}

/**
 * Tests if the string is not identical to any of ones included in array.
 * @param str appended string
 * @param str_size the string size
 * @param casesens should comparison be case sensitive
 * @param array string array.
 * @return JSR211_TRUE if string does not present in result yet JSR211_FALSE if does
 */
jsr211_boolean jsr211_isUniqueString(const jchar *str, size_t sz, int casesens, JSR211_RESULT_STRARRAY array) {
    JSR211_BUFFER_DATA bd = jsr211_get_result_data(*array);
    JSR211_ENUM_HANDLE eh = jsr211_get_enum_handle( bd );
    while( (bd = jsr211_get_next( &eh )) != NULL ){
        const void * data; size_t length;
        jsr211_get_data( bd, &data, &length );
		if ( length == sz * sizeof(jchar) ) {
			if (casesens == JAVACALL_TRUE) {
				if( wcsncmp(str, (const jchar *)data, sz) == 0 )
                    return JAVACALL_FALSE;
			} else {
				if( wcsnicmp(str, (const jchar *)data, sz) == 0 )
                    return JAVACALL_FALSE;
			}
		}
    }
    return JAVACALL_TRUE;
}

/**
 * Tests if the handler id is not identical to any of ones included in content handler array.
 * @param id tested content handler id 
 * @param id_sz the id size in jchars
 * @param array array of content handlers.
 * @return JSR211_TRUE if id does not present in result yet JSR211_FALSE if does
 */
jsr211_boolean jsr211_isUniqueHandler(const jchar *id, size_t id_sz, JSR211_RESULT_CHARRAY array) {
    JSR211_ENUM_HANDLE eh = jsr211_get_enum_handle( jsr211_get_result_data(*array) );
    JSR211_BUFFER_DATA bd;
    while( (bd = jsr211_get_next( &eh )) != NULL ){
        JSR211_ENUM_HANDLE ehh = jsr211_get_enum_handle( bd );
        JSR211_BUFFER_DATA id_handle = jsr211_get_next( &ehh );
        const void * data; size_t length;
        jsr211_get_data( id_handle, &data, &length );
		if ( length == id_sz * sizeof(jchar) ) {
			if( wcsncmp(id, (const jchar *)data, id_sz) == 0 )
                return JAVACALL_FALSE;
		}
    }
    return JAVACALL_TRUE;
}

/**
 * Appends string to output string array if it is not already in array.
 * @param str appended string
 * @param str_size the string size
 * @param casesensetive should comparison be case sensitive
 * @param array string array.
 * @return operation status.
 */
jsr211_result jsr211_appendUniqueString(const jchar* str, size_t str_size, int casesens,
												  /*OUT*/ JSR211_RESULT_STRARRAY array){
	if (JSR211_FALSE == jsr211_isUniqueString(str,str_size,casesens,array)) return JSR211_OK;
	return jsr211_appendString(str,str_size,array);
}

//---------------------------------------------------------

/**
 * Serializes handler data into buffer.
 * Variable <code>buf</code> after macros comletion points at the end of 
 * filled area.
 */
static jsr211_result fill_ch_buf(DATA_BUFFER ** buffer, const javacall_utf16* id, size_t id_size, 
						    const jchar* suit, size_t suit_size, const jchar* clas, size_t clas_size, 
                            unsigned short flag) {
    static jchar xd[] = { '0', '1', '2', '3', '4', '5', '6', '7', 
                          '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    jsr211_result rc;
    jchar b[sizeof(flag) * 2]; int i;

    // put id
    CHECKRC( jsr211_append_data(buffer, id, id_size * sizeof(id[0])) );
	// put suite_id
    CHECKRC( jsr211_append_data(buffer, suit, suit_size * sizeof(suit[0])) );
    // put class_name
    CHECKRC( jsr211_append_data(buffer, clas, clas_size * sizeof(clas[0])) );

    // put flag
    for( i = sizeof(flag); i--; flag >>= 8){
        b[ 2 * i + 1 ] = xd[flag & 0x0F];
        b[ 2 * i + 0 ] = xd[(flag & 0xF0) >> 4];
    }
    return jsr211_append_data( buffer, b, sizeof(b) );
}

/**
 * Fills output result structure with handler data.
 * @param id handler ID
 * @param id_size handler ID size
 * @param suit suite Id
 * @param class_name handler class name
 * @param class_name_size handler class name size
 * @param flag handler installation flag
 * @param result output result structure.
 * @return operation status.
 */
jsr211_result jsr211_fillHandler( const jchar* id, size_t id_size,
		            const jchar* suit, size_t suit_size, const jchar* class_name, size_t class_name_size,
                    unsigned short flag, /*OUT*/ JSR211_RESULT_CH result) 
{
    jsr211_clean_buffer( (DATA_BUFFER**)result );
    return fill_ch_buf((DATA_BUFFER**)result, id, id_size,
                suit, suit_size, class_name, class_name_size, flag);
}

/**
 * Appends the handler data to the result array.
 * @param id handler ID
 * @param id_size handler ID size
 * @param suit suite Id
 * @param class_name handler class name
 * @param class_name_size handler class name size
 * @param flag handler installation flag
 * @param array output result array.
 * @return operation status.
 */
jsr211_result jsr211_appendHandler( const jchar* id, size_t id_size,
                    const jchar* suit, size_t suit_size, const jchar* class_name, size_t class_name_size,
                    unsigned short flag, /*OUT*/ JSR211_RESULT_CHARRAY charray ) {
    jsr211_result rc;
    CHECKRC( jsr211_add_level( (DATA_BUFFER**)charray ) );
    CHECKRC( fill_ch_buf((DATA_BUFFER**)charray, id, id_size,
                        suit, suit_size, class_name, class_name_size, flag) );
    CHECKRC( jsr211_drop_level( *(DATA_BUFFER**)charray ) );
    return JSR211_OK;
}
