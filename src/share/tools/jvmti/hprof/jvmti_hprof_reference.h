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

#ifndef HPROF_REFERENCE_H
#define HPROF_REFERENCE_H

void     reference_init(void);
RefIndex reference_obj(RefIndex next, jvmtiHeapReferenceKind kind,
		ObjectIndex object_index, jint index, jint length);
RefIndex reference_prim_field(RefIndex next, jvmtiHeapReferenceKind refKind,
              jvmtiPrimitiveType primType, jvalue value, jint field_index);
RefIndex reference_prim_array(RefIndex next, jvmtiPrimitiveType element_type, 
		const void *elements, jint count);
void     reference_cleanup(void);
void     reference_dump_class(JNIEnv *env, ObjectIndex object_index,
		RefIndex list);
void     reference_dump_instance(JNIEnv *env, ObjectIndex object_index,
		RefIndex list);

#endif
