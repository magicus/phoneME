/*
 * @(#)hprof_method.h	1.11 06/10/10
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

#ifndef _HPROF_METHOD_H
#define _HPROF_METHOD_H

void hprof_method_table_init(void);
hprof_method_t * hprof_lookup_method(jmethodID method_id);
hprof_method_t * hprof_intern_method(JVMPI_Method *jmethod, hprof_class_t *hclass);
void hprof_method_entry_event(JNIEnv *env_id, jmethodID method_id);
void hprof_method_exit_event(JNIEnv *env_id, jmethodID method_id);

#ifdef HASH_STATS
void hprof_print_method_hash_stats(FILE *fp);
#endif /* HASH_STATS */
#ifdef WATCH_ALLOCS
void hprof_free_method_table(void);
#endif /* WATCH_ALLOCS */

#endif /* _HPROF_METHOD_H */
