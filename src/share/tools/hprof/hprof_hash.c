/*
 * @(#)hprof_hash.c	1.20 06/10/10
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

#include "javavm/include/porting/ansi/string.h"

#include "hprof.h"

/* initialise a hashtable */
void 
hprof_hash_init(int table_type,
		hprof_hash_t *table, 
		int size,
		unsigned int (*hash_f)(void *),
		unsigned int (*size_f)(void *),
		int (*compare_f)(void *, void *))
{
#if defined(HASH_STATS) || defined(WATCH_ALLOCS)
    table->table_type = table_type;
#endif /* HASH_STATS || WATCH_ALLOCS */
    table->n_entries = 0;
    table->size = size;
    table->entries = HPROF_CALLOC(ALLOC_TYPE_HASH_TABLES + table_type,
	size * sizeof(hprof_bucket_t *));
    table->hash_f = hash_f;
    table->size_f = size_f;
    table->compare_f = compare_f;
#ifdef HASH_STATS
    table->table_deletes = 0;
    table->table_hits    = 0;
    table->table_misses  = 0;
#endif /* HASH_STATS */
}

/* iterate through the hash table */
void *
hprof_hash_iterate(hprof_hash_t *table,
		   void * (*f)(void *, void *), 
		   void *arg)
{
    int i;
    for (i = 0; i < table->size; i++) {
        hprof_bucket_t *bucket = table->entries[i];
	while (bucket) {
	    arg = f(bucket->content, arg);
	    bucket = bucket->next;
	}
    }
    return arg;
}

/* remove an entry from the hash table */
void
hprof_hash_remove(hprof_hash_t *table, 
		  int (*f)(void *, void *), 
		  void *arg)
{
    int i;
    for (i = 0; i < table->size; i++) {
        hprof_bucket_t **p = &(table->entries[i]);
	hprof_bucket_t *bucket;
	while ((bucket = *p)) {
	    if (f(bucket->content, arg)) {
	        *p = bucket->next;
		hprof_free(bucket->content);
		hprof_free(bucket);
		table->n_entries--;
#ifdef HASH_STATS
                table->table_deletes++;
#endif /* HASH_STATS */
	        continue;
	    }
	    p = &(bucket->next);
	}
    }    
}

/* remove all entries from the hash table */
void
hprof_hash_removeall(hprof_hash_t *table)
{
    int i;
    for (i = 0; i < table->size; i++) {
        hprof_bucket_t **p = &(table->entries[i]);
	hprof_bucket_t *bucket;
	while ((bucket = *p)) {
	    *p = bucket->next;
	    hprof_free(bucket->content);
	    hprof_free(bucket);
	    table->n_entries--;
#ifdef HASH_STATS
            table->table_deletes++;
#endif /* HASH_STATS */
	}
    }
}

/* lookup a hashtable, if present, a ptr to the contents is
   returned, else a NULL. */
void *
hprof_hash_lookup(hprof_hash_t *table, 
		  void *new)
{
    int index = table->hash_f(new);
    hprof_bucket_t **p = &(table->entries[index]);
    
    while (*p) {
      if (table->compare_f(new, (*p)->content) == 0) {
#ifdef HASH_STATS
	  table->table_hits++;
#endif /* HASH_STATS */
	  return (*p)->content;
      }
      p = &((*p)->next);
    }
#ifdef HASH_STATS
    table->table_misses++;
#endif /* HASH_STATS */
    return NULL;
}

/* put an entry into a hashtable
   CAUTION - this should be used only if 
   hprof_hash_lookup returns NULL */
void * 
hprof_hash_put(hprof_hash_t *table, 
	       void *new)
{
    int            bytes  = table->size_f(new);
    int            index  = table->hash_f(new);
    hprof_bucket_t *new_bucket;
    void           *ptr;
    
#ifdef WATCH_ALLOCS
    int a_type = ALLOC_TYPE_OTHER;

    switch (table->table_type) {
    case ALLOC_HASH_CLASS:             a_type = ALLOC_TYPE_CLASS; break;
    case ALLOC_HASH_CONTENDED_MONITOR: a_type = ALLOC_TYPE_CONTENDED_MONITOR; break;
    case ALLOC_HASH_FRAME:             a_type = ALLOC_TYPE_FRAME; break;
    case ALLOC_HASH_FRAMES_COST:       a_type = ALLOC_TYPE_FRAMES_COST; break;
    case ALLOC_HASH_METHOD:            a_type = ALLOC_TYPE_METHOD; break;
    case ALLOC_HASH_NAME:              a_type = ALLOC_TYPE_NAME; break;
    case ALLOC_HASH_OBJMAP:            a_type = ALLOC_TYPE_OBJMAP; break;
    case ALLOC_HASH_RAW_MONITOR:       a_type = ALLOC_TYPE_RAW_MONITOR; break;
    case ALLOC_HASH_SITE:              a_type = ALLOC_TYPE_SITE; break;
    case ALLOC_HASH_THREAD:            a_type = ALLOC_TYPE_THREAD; break;
    case ALLOC_HASH_TRACE:             a_type = ALLOC_TYPE_TRACE; break;
    }
#endif /* WATCH_ALLOCS */

    new_bucket = HPROF_CALLOC(ALLOC_TYPE_BUCKET, sizeof(hprof_bucket_t));
    ptr = HPROF_CALLOC(a_type, bytes);

    memcpy(ptr, new, bytes);
    new_bucket->next = table->entries[index];
    new_bucket->content = ptr;
    table->entries[index] = new_bucket;
    table->n_entries++;
    return ptr;
}

/* intern an entry into the hash table */
void * 
hprof_hash_intern(hprof_hash_t *table, void *new)
{
    
    void *ptr = hprof_hash_lookup(table, new);
    if (ptr == NULL) {
        ptr = hprof_hash_put(table, new);
    }
    return ptr;
}


#ifdef HASH_STATS
void
hprof_print_tbl_hash_stats(FILE *fp, hprof_hash_t *table)
{
    unsigned int empties = 0;       /* number of empty slots */
    unsigned int i;                 /* scratch index */
    unsigned int prev_elems;        /* previous element count */
    char         *tbl_name;         /* table name */

    switch (table->table_type) {
    case ALLOC_HASH_CLASS:             tbl_name = "class_tbl"; break;
    case ALLOC_HASH_CONTENDED_MONITOR: tbl_name = "contended_monitor_tbl"; break;
    case ALLOC_HASH_FRAME:             tbl_name = "frame_tbl"; break;
    case ALLOC_HASH_FRAMES_COST:       tbl_name = "frames_cost_tbl"; break;
    case ALLOC_HASH_METHOD:            tbl_name = "method_tbl"; break;
    case ALLOC_HASH_NAME:              tbl_name = "name_tbl"; break;
    case ALLOC_HASH_OBJMAP:            tbl_name = "objmap_tbl"; break;
    case ALLOC_HASH_RAW_MONITOR:       tbl_name = "raw_monitor_tbl"; break;
    case ALLOC_HASH_SITE:              tbl_name = "site_tbl"; break;
    case ALLOC_HASH_THREAD:            tbl_name = "thread_tbl"; break;
    case ALLOC_HASH_TRACE:             tbl_name = "trace_tbl"; break;
    default:                           tbl_name = "<UNKNOWN>"; break;
    }

    fprintf(fp, "\nHash Table Statistics for '%s'\n\n", tbl_name);

    if (table->n_entries > 0 && print_verbose_hash_stats) {
	fprintf(fp, "Table Histogram:\n");
	fprintf(fp, "'K'-1024 elements, '#'-100 elements, '@'-1 element\n");
    }

    prev_elems = 0;    /* skip leading empty slots */
    for (i = 0; i < table->size; i++) {
        hprof_bucket_t      *bucket;        /* element index */
	unsigned int        count;          /* # of markers to print */
	unsigned int        elems = 0;      /* # of elements in slot */
	unsigned int        saved_elems;    /* saved elems value */

	for (bucket = table->entries[i]; bucket; bucket = bucket->next) {
	    elems++;
	}
	if (elems == 0) {    /* nothing in this slot */
	    empties++;
	    if (prev_elems != 0) {
		if (print_verbose_hash_stats) fprintf(fp, "      ::\n");
		prev_elems = 0;
	    }
	    continue;
	}
	saved_elems = elems;

	if (print_verbose_hash_stats) {
	    fprintf(fp, "%7u: ", i);
	    count = elems / 1024;
	    if (count != 0) {
		elems -= count * 1024;
		for (; count > 0; count--) fputc('K', fp);
	    }
	    count = elems / 100;
	    if (count != 0) {
		elems -= count * 100;
		for (; count > 0; count--) fputc('#', fp);
	    }
	    for (; elems > 0; elems--) fputc('@', fp);

	    fprintf(fp, " (%u)\n", saved_elems);
	}
	prev_elems = saved_elems;
    }

    if (table->n_entries == 0) {
	fprintf(fp, "TABLE IS EMPTY!!\n");
    } else {
	fprintf(fp, "#-elems=%u  #-deletes=%u  #-hits=%u  #-misses=%u\n"
	    "empty-slots=%.2f%%  avg-#-elems=%.2f\n\n",
	    table->n_entries, table->table_deletes, table->table_hits,
	    table->table_misses, ((float)empties / table->size) * 100.0,
	    (float)table->n_entries / (table->size - empties));
    }
}


void
hprof_print_global_hash_stats(FILE *fp)
{
    hprof_print_class_hash_stats(fp);
    if (monitor_tracing) hprof_print_contended_monitor_hash_stats(fp);
    hprof_print_frame_hash_stats(fp);
    /* hprof_frames_cost_t table is allocated per-thread */
    hprof_print_method_hash_stats(fp);
    hprof_print_name_hash_stats(fp);
    hprof_print_objmap_hash_stats(fp);
    if (monitor_tracing) hprof_print_raw_monitor_hash_stats(fp);
    hprof_print_site_hash_stats(fp);
    hprof_print_thread_hash_stats(fp);
    hprof_print_trace_hash_stats(fp);
}
#endif /* HASH_STATS */


#ifdef WATCH_ALLOCS
void
hprof_hash_free(hprof_hash_t *table) {
    hprof_free(table->entries);
}
#endif /* WATCH_ALLOCS */
