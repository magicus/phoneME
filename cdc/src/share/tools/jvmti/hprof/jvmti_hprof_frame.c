/*
 * jvmti hprof
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.	 
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
 *
 */

/* This file contains support for handling frames, or (method,location) pairs. */

#include "jvmti_hprof.h"

/*
 *  Frames map 1-to-1 to (methodID,location) pairs.
 *  When no line number is known, -1 should be used.
 *
 *  Frames are mostly used in traces (see hprof_trace.c) and will be marked
 *    with their status flag as they are written out to the hprof output file.
 *
 */

enum LinenoState {
    LINENUM_UNINITIALIZED = 0,
    LINENUM_AVAILABLE     = 1,
    LINENUM_UNAVAILABLE   = 2
};

typedef struct FrameKey {
    jmethodID   method;
    jlocation   location;
} FrameKey;

typedef struct FrameInfo {
    unsigned short    	lineno;
    unsigned char       lineno_state; /* LinenoState */
    unsigned char       status;
    SerialNumber serial_num;
} FrameInfo;

static FrameKey*
get_pkey(FrameIndex index)
{
    void *key_ptr;
    int   key_len;

    table_get_key(gdata->frame_table, index, &key_ptr, &key_len);
    HPROF_ASSERT(key_len==sizeof(FrameKey));
    HPROF_ASSERT(key_ptr!=NULL);
    return (FrameKey*)key_ptr;
}

static FrameInfo *
get_info(FrameIndex index)
{
    FrameInfo *info;

    info = (FrameInfo*)table_get_info(gdata->frame_table, index);
    return info;
}

static void
list_item(TableIndex i, void *key_ptr, int key_len, void *info_ptr, void *arg)
{
    FrameKey   key;
    FrameInfo *info;
    
    HPROF_ASSERT(key_ptr!=NULL);
    HPROF_ASSERT(key_len==sizeof(FrameKey));
    HPROF_ASSERT(info_ptr!=NULL);
    
    key = *((FrameKey*)key_ptr);
    info = (FrameInfo*)info_ptr;
    debug_message( 
	"Frame 0x%08x: method=%p, location=%d, lineno=%d(%d), status=%d \n",
                i, (void*)key.method, (jint)key.location, 
		info->lineno, info->lineno_state, info->status);
}

void
frame_init(void)
{
    gdata->frame_table = table_initialize("Frame",
                            1024, 1024, 1023, (int)sizeof(FrameInfo));
}

FrameIndex
frame_find_or_create(jmethodID method, jlocation location)
{
    FrameIndex index;
    FrameKey key;
    jboolean new_one;
    
    memset(&key, 0, sizeof(key));
    key.method   = method;
    key.location = location;
    new_one      = JNI_FALSE;
    index        = table_find_or_create_entry(gdata->frame_table, 
			&key, (int)sizeof(key), &new_one, NULL);
    if ( new_one ) {
	FrameInfo *info;
	
	info = get_info(index);
	info->lineno_state = LINENUM_UNINITIALIZED;
	if ( location < 0 ) {
	    info->lineno_state = LINENUM_UNAVAILABLE;
	}
        info->serial_num = gdata->frame_serial_number_counter++;
    }
    return index;
}

void
frame_list(void)
{
    debug_message( 
        "--------------------- Frame Table ------------------------\n");
    table_walk_items(gdata->frame_table, &list_item, NULL);
    debug_message(
        "----------------------------------------------------------\n");
}

void
frame_cleanup(void)
{
    table_cleanup(gdata->frame_table, NULL, NULL);
    gdata->frame_table = NULL;
}

void
frame_set_status(FrameIndex index, jint status)
{
    FrameInfo *info;

    info = get_info(index);
    info->status = (unsigned char)status;
}

void
frame_get_location(FrameIndex index, SerialNumber *pserial_num,
		   jmethodID *pmethod, jlocation *plocation, jint *plineno)
{
    FrameKey  *pkey;
    FrameInfo *info;
    jint       lineno;

    pkey       = get_pkey(index);
    *pmethod   = pkey->method;
    *plocation = pkey->location;
    info       = get_info(index);
    lineno     = (jint)info->lineno;
    if ( info->lineno_state == LINENUM_UNINITIALIZED ) {
	info->lineno_state = LINENUM_UNAVAILABLE;
	if ( gdata->lineno_in_traces ) {
	    if ( pkey->location >= 0 && !isMethodNative(pkey->method) ) {
		lineno = getLineNumber(pkey->method, pkey->location);
		if ( lineno >= 0 ) {
		    info->lineno = (unsigned short)lineno; /* save it */
                    info->lineno_state = LINENUM_AVAILABLE;
		}
	    }
	}
    } 
    if ( info->lineno_state == LINENUM_UNAVAILABLE ) {
	lineno = -1;
    }
    *plineno     = lineno;
    *pserial_num = info->serial_num;
}

jint
frame_get_status(FrameIndex index)
{
    FrameInfo *info;

    info = get_info(index);
    return (jint)info->status;
}

