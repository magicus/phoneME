/*
 * @(#)hprof_object.c	1.18 06/10/10
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

#include "hprof.h"

/* The table that maps objects to allocation sites */
static hprof_objmap_t **hprof_objmap_table;
static hprof_objmap_t *objmap_free_list = NULL;
static int table_size = HPROF_SMALL_OBJMAP_TABLE_SIZE;
#ifdef HASH_STATS
static unsigned int table_deletes;        /* # of entries deleted */
static unsigned int table_hits;           /* # of hash hits */
static unsigned int table_misses;         /* # of hash misses */
#endif /* HASH_STATS */

#define HASH_OBJ(obj) (((unsigned long)obj >> HASH_OBJ_SHIFT) % table_size)

void hprof_objmap_init(void)
{
    if (alloc_sites || heap_dump) table_size = HPROF_LARGE_OBJMAP_TABLE_SIZE;
    hprof_objmap_table =
	HPROF_CALLOC(ALLOC_TYPE_HASH_TABLES + ALLOC_HASH_OBJMAP,
	    table_size * sizeof(hprof_objmap_t *));
#ifdef HASH_STATS
    table_deletes = 0;
    table_hits    = 0;
    table_misses  = 0;
#endif /* HASH_STATS */
}   

static void add_alloc_stats(hprof_site_t *site, int size)
{
     site->n_alloced_instances += 1;
     site->n_live_instances += 1;
     site->n_alloced_bytes += size;
     site->n_live_bytes += size;
     site->changed = 1;
     
     total_alloced_bytes = jlong_add(total_alloced_bytes, jint_to_jlong(size));
     total_alloced_instances = jlong_add(total_alloced_instances, jint_to_jlong(1));
     total_live_bytes += size;
     total_live_instances += 1;
}

static void sub_alloc_stats(hprof_site_t *site, int size)
{
    site->n_live_instances--;
    site->n_live_bytes -= size;
    site->changed = 1;
    total_live_instances--;
    total_live_bytes -= size;
}
   
	
/* The objmap table maps objects to where they are allocated. Since this
 * is a much bigger table than site or trace tables, we do not use the
 * generic hash table routines for this purpose. Instead, we write
 * specialized code for better performance.
 */
static hprof_objmap_t *hprof_objmap_add(jobjectID obj_id, jint arena_id,
					hprof_site_t *site, int size)
{
    int index = HASH_OBJ(obj_id);
    hprof_objmap_t *bucket;
    if (objmap_free_list) {
        bucket = objmap_free_list;
	objmap_free_list = bucket->next;
    } else {
        bucket = HPROF_CALLOC(ALLOC_TYPE_OBJMAP, sizeof(hprof_objmap_t));
    }
    bucket->size = size;
    bucket->site = site;
    bucket->obj_id = obj_id;
    bucket->arena_id = arena_id;
    bucket->next = hprof_objmap_table[index];
    hprof_objmap_table[index] = bucket;
        
    if (site != NULL) {
        add_alloc_stats(site , size);
    }
    return bucket;
}

static void hprof_objmap_del(jobjectID obj_id)
{
    int index = HASH_OBJ(obj_id);
    hprof_objmap_t **p = hprof_objmap_table + index;
    hprof_objmap_t *bucket;
    while ((bucket = *p)) {
        if (bucket->obj_id == obj_id) {
	    *p = bucket->next;
	    bucket->next = objmap_free_list;
	    objmap_free_list = bucket;
	    if (bucket->site != NULL) {
	        sub_alloc_stats(bucket->site, bucket->size);
	    }
	    return;
	}
	p = &(bucket->next);
    }
    return;
}

static void hprof_objmap_del_arena(jint arena_id)
{
    int i;
    for (i = 0; i < table_size; i++) {
        hprof_objmap_t **p = hprof_objmap_table + i;
	hprof_objmap_t *bucket;
	while ((bucket = *p)) {
	    if (bucket->arena_id == arena_id) {
	        *p = bucket->next;
		bucket->next = objmap_free_list;
		objmap_free_list = bucket;
		if ((bucket->site != NULL) && (bucket->size != 0)) {
		    sub_alloc_stats(bucket->site, bucket->size);
		}
	    }
	    p = &(bucket->next);
	}
    }
}

static void hprof_objmap_move(jobjectID obj_id, jint arena_id,
			      jobjectID new_obj_id, jint new_arena_id)
{
    
    int old_index = HASH_OBJ(obj_id);
    int new_index = HASH_OBJ(new_obj_id);
    hprof_objmap_t **p = hprof_objmap_table + old_index;
    hprof_objmap_t *bucket;
    
    /* remove it from the old position in the hash table */
    while ((bucket = *p)) {
        if (bucket->obj_id == obj_id) {
	    *p = bucket->next;
	    break;
	}
	p = &(bucket->next);
    }
    
    if (bucket != NULL) { 
        /* rehash using the new obj_id; NOTE - the bucket remains the same */
        bucket->obj_id = new_obj_id;
	bucket->arena_id = new_arena_id;
	bucket->next = hprof_objmap_table[new_index];
	hprof_objmap_table[new_index] = bucket;
    } /* we ignore moves for objects with no prior record */
}

hprof_objmap_t * hprof_objmap_lookup(jobjectID obj_id)
{
    int index = HASH_OBJ(obj_id);
    hprof_objmap_t *bucket = hprof_objmap_table[index];
    while (bucket) {
        if (bucket->obj_id == obj_id) {
#ifdef HASH_STATS
            table_hits++;
#endif /* HASH_STATS */
	    return bucket;
	}
	bucket = bucket->next;
    }
#ifdef HASH_STATS
    table_misses++;
#endif /* HASH_STATS */
    return NULL;
}

void hprof_objmap_print(hprof_objmap_t *objmap)
{
    hprof_class_t *hclass = objmap->site->class;
    hprof_printf(" ");
    switch(objmap->site->is_array) {
    case JVMPI_NORMAL_OBJECT:
        hprof_printf("%s", hclass ? hclass->name->name : "<unknown class>");
	break;
    case JVMPI_CLASS:
        hprof_printf("[L%s;", hclass->name->name);
	break;
    case JVMPI_BOOLEAN:
        hprof_printf("[Z");
	break;
    case JVMPI_BYTE:
        hprof_printf("[B");
	break;
    case JVMPI_CHAR:
        hprof_printf("[C");
	break;
    case JVMPI_SHORT:
        hprof_printf("[S");
	break;
    case JVMPI_INT:
        hprof_printf("[I");
	break;
    case JVMPI_LONG:
        hprof_printf("[J");
	break;
    case JVMPI_FLOAT:
        hprof_printf("[F");
	break;
    case JVMPI_DOUBLE:
        hprof_printf("[D");
	break;
    }
    hprof_printf("(%x)", objmap);
}

void hprof_obj_alloc_event(JNIEnv *env_id,
			   jobjectID class_id,
			   int is_array,
			   int size,
			   jobjectID obj_id,
			   jint arena_id,
			   int requested)
{ 
    hprof_site_t *site;
    int trace_num = 0;

    CALL(RawMonitorEnter)(data_access_lock);
    if (requested) {
        if (hprof_objmap_lookup(obj_id)) {
	    goto done;
	}
        trace_num = 0;
    } else {
        hprof_trace_t *htrace = hprof_get_trace(env_id, max_trace_depth);
	if (htrace == NULL) {
	    fprintf(stderr, "HPROF ERROR: got NULL trace in obj_alloc\n");
	} else {
	    trace_num = htrace->serial_num;
	}
    }
    site = hprof_intern_site(class_id, is_array, trace_num);
    hprof_objmap_add(obj_id, arena_id, site, size);
 done:
    CALL(RawMonitorExit)(data_access_lock);
}

void hprof_obj_free_event(JNIEnv *env_id,
			  jobjectID obj_id)
{
    /* data_access_lock already entered in GC_START event */
    hprof_objmap_del(obj_id);
}

void hprof_delete_arena_event(JNIEnv *env_id, 
			      jint arena_id)
{
    /* data_access_lock already entered in GC_START event */
    hprof_objmap_del_arena(arena_id);
}

void hprof_obj_move_event(JNIEnv *env_id,
			  jobjectID obj_id,
			  jint arena_id,
			  jobjectID new_obj_id,
			  jint new_arena_id)
{
    /* data_access_lock already entered in GC_START event */
    hprof_objmap_move(obj_id, arena_id, new_obj_id, new_arena_id);
}

hprof_objmap_t * hprof_fetch_object_info(jobjectID obj)
{
    hprof_objmap_t * objmap;
    if (obj == NULL) {
        return NULL;
    }
    objmap = hprof_objmap_lookup(obj);
    if (objmap == NULL) {
#ifdef OBJMAP_DEBUG
        fprintf(stderr, "requesting info for obj %x ", obj);
#endif
        CALL(RequestEvent)(JVMPI_EVENT_OBJ_ALLOC, obj);
	objmap = hprof_objmap_lookup(obj);
#ifdef OBJMAP_DEBUG
	fprintf(stderr, "=> %x\n", objmap);
#endif
    }
    return objmap;
}

void hprof_print_object_info(jobjectID obj)
{
    hprof_objmap_t *objmap = hprof_fetch_object_info(obj);
    hprof_class_t *hclass = objmap->site->class;
    if (objmap == NULL) {
        fprintf(stderr, "HPROF ERROR: unknown object ID 0x%p\n", obj);
    }
    hprof_printf(" ");
    switch(objmap->site->is_array) {
    case JVMPI_NORMAL_OBJECT:
        hprof_printf("%s", hclass ? hclass->name->name : "<unknown class>");
	break;
    case JVMPI_CLASS:
        hprof_printf("[L%s;", hclass->name->name);
	break;
    case JVMPI_BOOLEAN:
        hprof_printf("[Z");
	break;
    case JVMPI_BYTE:
        hprof_printf("[B");
	break;
    case JVMPI_CHAR:
        hprof_printf("[C");
	break;
    case JVMPI_SHORT:
        hprof_printf("[S");
	break;
    case JVMPI_INT:
        hprof_printf("[I");
	break;
    case JVMPI_LONG:
        hprof_printf("[J");
	break;
    case JVMPI_FLOAT:
        hprof_printf("[F");
	break;
    case JVMPI_DOUBLE:
        hprof_printf("[D");
	break;
    }
    hprof_printf("(%x)", objmap);
}


#ifdef HASH_STATS
void hprof_print_objmap_hash_stats(FILE *fp)
{
    unsigned int elem_count = 0;    /* total element count */
    unsigned int empties = 0;       /* number of empty slots */
    unsigned int i;                 /* scratch index */
    unsigned int prev_elems;        /* previous element count */

    fprintf(fp, "\nHash Table Statistics for 'objmap_tbl'\n\n");

    if (print_verbose_hash_stats) {
	fprintf(fp, "Table Histogram:\n");
	fprintf(fp, "'K'-1024 elements, '#'-100 elements, '@'-1 element\n");
    }

    prev_elems = 0;    /* skip leading empty slots */
    for (i = 0; i < table_size; i++) {
        hprof_objmap_t      *bucket;        /* element index */
	unsigned int        count;          /* # of markers to print */
	unsigned int        elems = 0;      /* # of elements in slot */
	unsigned int        saved_elems;    /* saved elems value */

	for (bucket = hprof_objmap_table[i]; bucket; bucket = bucket->next) {
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
	elem_count += elems;

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

    fprintf(fp, "#-elems=%u  #-hits=%u  #-misses=%u  "
	"empty-slots=%.2f%%  avg-#-elems=%.2f\n\n",
	elem_count, table_hits, table_misses,
	((float)empties / table_size) * 100.0,
	(float)elem_count / (table_size - empties));
}
#endif /* HASH_STATS */


#ifdef WATCH_ALLOCS
void hprof_free_objmap_table(void) {
    int i;
    for (i = 0; i < table_size; i++) {
        hprof_objmap_t **p = &hprof_objmap_table[i];
	hprof_objmap_t *bucket;
	while (bucket = *p) {
	    *p = bucket->next;
	    hprof_free(bucket);
#ifdef HASH_STATS
            table_deletes++;
#endif /* HASH_STATS */
	}
    }
    hprof_free(hprof_objmap_table);
}
#endif /* WATCH_ALLOCS */
