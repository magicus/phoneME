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

#ifndef HPROF_CLASS_H
#define HPROF_CLASS_H

void            class_init(void);
ClassIndex      class_find_or_create(const char *sig, LoaderIndex loader);
ClassIndex      class_create(const char *sig, LoaderIndex loader);
SerialNumber    class_get_serial_number(ClassIndex index);
StringIndex     class_get_signature(ClassIndex index);
ClassStatus     class_get_status(ClassIndex index);
void            class_add_status(ClassIndex index, ClassStatus status);
void            class_all_status_remove(ClassStatus status);
void            class_do_unloads(JNIEnv *env);
void            class_list(void);
void            class_delete_global_references(JNIEnv* env);
void            class_cleanup(void);
void            class_set_methods(ClassIndex index, const char**name,
                                const char**descr,  int count);
jmethodID       class_get_methodID(JNIEnv *env, ClassIndex index, 
                                MethodIndex mnum);
jclass          class_new_classref(JNIEnv *env, ClassIndex index, 
                                jclass classref);
void            class_delete_classref(JNIEnv *env, ClassIndex index);
jclass          class_get_class(JNIEnv *env, ClassIndex index);
void            class_set_inst_size(ClassIndex index, jint inst_size);
jint            class_get_inst_size(ClassIndex index);
void            class_set_object_index(ClassIndex index, 
				ObjectIndex object_index);
ObjectIndex     class_get_object_index(ClassIndex index);
ClassIndex      class_get_super(ClassIndex index);
void            class_set_super(ClassIndex index, ClassIndex super);
void            class_set_loader(ClassIndex index, LoaderIndex loader);
LoaderIndex     class_get_loader(ClassIndex index);
void            class_prime_system_classes(void);
jint            class_get_all_fields(JNIEnv *env, ClassIndex cnum,
				     jint *pfield_count, FieldInfo **pfield);

#endif
