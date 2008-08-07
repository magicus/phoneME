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

/* JVMTI tag definitions. */

/*
 * JVMTI tags are jlongs (64 bits) and how the hprof information is
 *   turned into a tag and/or extracted from a tag is here.
 *
 * Currently a special TAG_CHECK is placed in the high order 32 bits of
 *    the tag as a check.
 * 
 */

#include "jvmti_hprof.h"

#define TAG_CHECK 0xfad4dead

jlong
tag_create(ObjectIndex object_index)
{
    jlong               tag;
    
    HPROF_ASSERT(object_index != 0);
    tag = TAG_CHECK;
    tag = (tag << 32) | object_index;
    return tag;
}

ObjectIndex
tag_extract(jlong tag)
{
    HPROF_ASSERT(tag != (jlong)0);
    if ( ((tag >> 32) & 0xFFFFFFFF) != TAG_CHECK) {
	HPROF_ERROR(JNI_TRUE, "JVMTI tag value is not 0 and missing TAG_CHECK");
    }
    return  (ObjectIndex)(tag & 0xFFFFFFFF);
}

/* Tag a new jobject */
void
tag_new_object(jobject object, ObjectKind kind, SerialNumber thread_serial_num,
		jint size, SiteIndex site_index)
{
    ObjectIndex  object_index;
    jlong        tag;
  
    HPROF_ASSERT(site_index!=0);
    /* New object for this site. */
    object_index = object_new(site_index, size, kind, thread_serial_num);
    /* Create and set the tag. */
    tag = tag_create(object_index);
    setTag(object, tag);
    LOG3("tag_new_object", "tag", (int)tag);
}

/* Tag a jclass jobject if it hasn't been tagged. */
void
tag_class(JNIEnv *env, jclass klass, ClassIndex cnum, 
		SerialNumber thread_serial_num, SiteIndex site_index)
{
    ObjectIndex object_index;
  
    /* If the ClassIndex has an ObjectIndex, then we have tagged it. */
    object_index = class_get_object_index(cnum);
    
    if ( object_index == 0 ) {
        jint        size;
        jlong        tag;
        
	HPROF_ASSERT(site_index!=0);
	
	/* If we don't know the size of a java.lang.Class object, get it */
	size =  gdata->system_class_size;
	if ( size == 0 ) {
	    size  = (jint)getObjectSize(klass);
	    gdata->system_class_size = size;
	}
	
	/* Tag this java.lang.Class object if it hasn't been already */
	tag = getTag(klass);
	if ( tag == (jlong)0 ) {
	    /* New object for this site. */
	    object_index = object_new(site_index, size, OBJECT_CLASS,
					thread_serial_num);
	    /* Create and set the tag. */
	    tag = tag_create(object_index);
	    setTag(klass, tag);
	} else {
	    /* Get the ObjectIndex from the tag. */
	    object_index = tag_extract(tag);
	}
        
	/* Record this object index in the Class table */
	class_set_object_index(cnum, object_index);
    }
}

