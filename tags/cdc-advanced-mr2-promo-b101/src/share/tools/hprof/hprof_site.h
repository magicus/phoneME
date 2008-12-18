/*
 * @(#)hprof_site.h	1.12 06/10/10
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

#ifndef _HPROF_SITE_H
#define _HPROF_SITE_H

/* Allocation sites */
typedef struct hprof_site_t {
    int changed;                       /* objects at this site changed? */
    int is_array;                      /* object arrays:    JVMPI_CLASS
					  primitive arrays: JVMPI_BOOLEAN, ...
					  normal objects:   0 */
    unsigned int trace_serial_num;     /* trace serial number */
    struct hprof_class_t *class;      /* NULL for primitive arrays */
    unsigned int n_alloced_instances; 
    unsigned int n_alloced_bytes;
    unsigned int n_live_instances;
    unsigned int n_live_bytes;
} hprof_site_t;

typedef struct {
    hprof_site_t **sites;
    int index;
    int changed_only;
} hprof_site_iterate_t;

void hprof_site_table_init(void);
hprof_site_t * hprof_intern_site(jobjectID class_id, 
				 int is_array, 
				 int trace_serial_num);
void hprof_output_sites(int flags, float cutoff);
void hprof_clear_site_table(void);

#ifdef HASH_STATS
void hprof_print_site_hash_stats(FILE *fp);
#endif /* HASH_STATS */
#ifdef WATCH_ALLOCS
void hprof_free_site_table(void);
#endif /* WATCH_ALLOCS */

/*
 * If alloc_sites or heap_dump is true, then a larger table is used.
 * Otherwise, a (very) small table will do.
 */
#define HPROF_LARGE_SITE_TABLE_SIZE 1024
#define HPROF_SMALL_SITE_TABLE_SIZE 8

#endif /* _HPROF_SITE_H */

