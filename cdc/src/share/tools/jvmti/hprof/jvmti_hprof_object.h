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

#ifndef HPROF_OBJECT_H
#define HPROF_OBJECT_H

void         object_init(void);
ObjectIndex  object_new(SiteIndex site_index, jint size, ObjectKind kind,
			SerialNumber thread_serial_num);
SiteIndex    object_get_site(ObjectIndex index);
jint         object_get_size(ObjectIndex index);
ObjectKind   object_get_kind(ObjectIndex index);
ObjectKind   object_free(ObjectIndex index);
void         object_list(void);
void         object_cleanup(void);
  
void         object_set_thread_serial_number(ObjectIndex index, 
					     SerialNumber thread_serial_num);
SerialNumber object_get_thread_serial_number(ObjectIndex index);
RefIndex     object_get_references(ObjectIndex index);
void         object_set_references(ObjectIndex index, RefIndex ref_index);
void         object_clear_references(void);
void         object_reference_dump(JNIEnv *env);

#endif
