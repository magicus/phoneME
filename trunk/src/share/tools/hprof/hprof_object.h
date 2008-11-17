/*
 * @(#)hprof_object.h	1.13 06/10/10
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

#ifndef _HPROF_OBJECT_H
#define _HPROF_OBJECT_H

/* Live objects */
typedef struct hprof_objmap_t {
    struct hprof_objmap_t *next;
    jobjectID obj_id;                  /* object id */
    jint arena_id;                     /* arena_id */
    struct hprof_site_t *site;         /* alloc site */
    unsigned int size;                 /* object size */
} hprof_objmap_t;

void hprof_objmap_init(void);
hprof_objmap_t * hprof_objmap_lookup(jobjectID obj_id);
void hprof_objmap_print(hprof_objmap_t *objmap);

void hprof_obj_alloc_event(JNIEnv *env_id,
			   jobjectID class_id,
			   int is_array,
			   int size,
			   jobjectID obj_id,
			   jint arena_id,
			   int requested);
void hprof_obj_free_event(JNIEnv *env_id,
			  jobjectID obj_id);
void hprof_delete_arena_event(JNIEnv *env_id, 
			      jint arena_id);
void hprof_obj_move_event(JNIEnv *env_id,
			  jobjectID obj_id,
			  jint arena_id,
			  jobjectID new_obj_id,
			  jint new_arena_id);
hprof_objmap_t * hprof_fetch_object_info(jobjectID obj);
void hprof_print_object_info(jobjectID obj);

#ifdef HASH_STATS
void hprof_print_objmap_hash_stats(FILE *fp);
#endif /* HASH_STATS */
#ifdef WATCH_ALLOCS
void hprof_free_objmap_table(void);
#endif /* WATCH_ALLOCS */

/*
 * If alloc_sites or heap_dump is true, then a larger table is used.
 * Otherwise, a small table will do.
 */
#define HPROF_LARGE_OBJMAP_TABLE_SIZE 65536
#define HPROF_SMALL_OBJMAP_TABLE_SIZE 4096

#endif /* _HPROF_OBJECT_H */
