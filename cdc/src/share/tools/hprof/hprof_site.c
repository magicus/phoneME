/*
 * @(#)hprof_site.c	1.17 06/10/10
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

static hprof_hash_t hprof_site_table;

static unsigned int hash_site(void *_site)
{
    hprof_site_t *site = _site;
    return (((unsigned int)site->class >> HASH_OBJ_SHIFT) +
        (site->trace_serial_num + site->is_array) * 37) % hprof_site_table.size;
}

static unsigned int size_site(void *_site)
{
    return sizeof(hprof_site_t);
}

static int compare_site(void *_site1, void *_site2)
{
    hprof_site_t *site1 = _site1;
    hprof_site_t *site2 = _site2;
    int result = site1->class - site2->class;
    if (result)
       return result;
    result = site1->is_array - site2->is_array;
    if (result)
       return result;
    return site1->trace_serial_num - site2->trace_serial_num;
}

void hprof_site_table_init(void)
{
    int table_size = HPROF_SMALL_SITE_TABLE_SIZE;

    if (alloc_sites || heap_dump) table_size = HPROF_LARGE_SITE_TABLE_SIZE;

    hprof_hash_init(ALLOC_HASH_SITE, &hprof_site_table, table_size,
		    hash_site, size_site, compare_site);
}

hprof_site_t *
hprof_intern_site(jobjectID class_id, 
		  int is_array, 
		  int trace_serial_num)
{
    hprof_site_t site_tmp;
    hprof_site_t *result;
    
    site_tmp.class = NULL;
    if (class_id != 0) {
        site_tmp.class = hprof_lookup_class(class_id);
    }
    site_tmp.is_array = is_array;
    site_tmp.trace_serial_num = trace_serial_num;
    site_tmp.n_alloced_instances = 0;
    site_tmp.n_alloced_bytes = 0;
    site_tmp.n_live_instances = 0;
    site_tmp.n_live_bytes = 0;

    result = hprof_hash_intern(&hprof_site_table, &site_tmp);
    result->changed = 1;
    return result;
}

static void * hprof_site_collect(void *_site, void *_arg)
{
    hprof_site_iterate_t *arg = _arg;
    hprof_site_t *site = _site;
    if (arg->changed_only && (!site->changed)) {
        return _arg;
    }
    arg->sites[arg->index++] = _site;
    return _arg;
}

void hprof_clear_site_table(void)
{
    hprof_hash_removeall(&hprof_site_table);
}

static void * hprof_site_mark_unchanged(void *_site, void *_arg)
{
    hprof_site_t *site = _site;
    site->changed = 0;
    return _arg;
}

static int hprof_site_compare_f1(const void *p_site1, const void *p_site2)
{
    hprof_site_t *site1 = *(hprof_site_t **)p_site1;
    hprof_site_t *site2 = *(hprof_site_t **)p_site2;
    return site2->n_alloced_bytes - site1->n_alloced_bytes;
}

static int hprof_site_compare_f2(const void *p_site1, const void *p_site2)
{
    hprof_site_t *site1 = *(hprof_site_t **)p_site1;
    hprof_site_t *site2 = *(hprof_site_t **)p_site2;
    return site2->n_live_bytes - site1->n_live_bytes;
}

static char *hprof_get_array_type_name(int is_array)
{
    switch (is_array) {
    case JVMPI_BOOLEAN:
        return "[Z";
    case JVMPI_BYTE:
        return "[B";
    case JVMPI_CHAR:
        return "[C";
    case JVMPI_SHORT:
        return "[S";
    case JVMPI_INT:
        return "[I";
    case JVMPI_LONG:
        return "[J";
    case JVMPI_FLOAT:
        return "[F";
    case JVMPI_DOUBLE:
        return "[D";
    case JVMPI_CLASS:
        return "[L<Unknown>;";
    default:
        return "<Unknown>";
    }
}

/* Output allocation sites, up to the given cut-off point, and according
 * to the given flags:
 *
 *      HPROF_SITE_DUMP_INCREMENTAL only dump what's changed since last dump.
 *      HPROF_SITE_SORT_BY_ALLOC    sort sites by total allocation rather
 *                                  than live data.
 *      HPROF_SITE_FORCE_GC         force a GC before the site dump.
 */

void hprof_output_sites(int flags, float cutoff)
{
    hprof_site_iterate_t iterate;
    int i;
    int site_table_size;
    double accum_percent = 0;
    void *comment_str;

    if (flags & HPROF_SITE_FORCE_GC) {
        CALL(RunGC)();
    }

    CALL(RawMonitorEnter)(data_access_lock);
    site_table_size = hprof_site_table.n_entries;
    iterate.sites =
	HPROF_CALLOC(ALLOC_TYPE_ARRAY, site_table_size * sizeof (void *));
    iterate.index = 0;
    iterate.changed_only = flags & HPROF_SITE_DUMP_INCREMENTAL;
    hprof_hash_iterate(&hprof_site_table, hprof_site_collect, &iterate);

    site_table_size = iterate.index;

    if (flags & HPROF_SITE_SORT_BY_ALLOC) {
        comment_str = "allocated bytes";
        qsort(iterate.sites, site_table_size, sizeof(void *), 
	      hprof_site_compare_f1);
    } else {
        comment_str = "live bytes";
        qsort(iterate.sites, site_table_size, sizeof(void *), 
	      hprof_site_compare_f2); 
    }

    hprof_output_unmarked_traces();
    if (output_format == 'b') {
        int count = 0;
	for (i = 0; i < site_table_size; i++) {
	    hprof_site_t *site = iterate.sites[i];
	    double ratio =
	        (double)site->n_live_bytes / (double)total_live_bytes;
	    if (ratio < cutoff)
	        break;
	    count ++;
	}
        hprof_write_header(HPROF_ALLOC_SITES, count * (4 * 6 + 1) + 2 + 32);
	hprof_write_u2((unsigned short)flags);
	hprof_write_u4(*(int *)(void *)(&cutoff));
	hprof_write_u4(total_live_bytes);
	hprof_write_u4(total_live_instances);
	hprof_write_u4(jlong_high(total_alloced_bytes));
	hprof_write_u4(jlong_low(total_alloced_bytes));
	hprof_write_u4(jlong_high(total_alloced_instances));
	hprof_write_u4(jlong_low(total_alloced_instances));
	hprof_write_u4(count);
    } else {
        time_t t = time(0);
        hprof_printf("SITES BEGIN (ordered by %s) %s", comment_str, ctime(&t));
	hprof_printf(
	    "          percent         live       alloc'ed  stack class\n");
	hprof_printf(
	    " rank   self  accum    bytes objs   bytes objs trace name\n");
    }
    for (i = 0; i < site_table_size; i++) {
	hprof_site_t *site = iterate.sites[i];
	unsigned int class_serial_num;
        const char *class_name;
	double ratio =
	    (double)site->n_live_bytes / (double)total_live_bytes;
	if (ratio < cutoff)
	    break;

	if (site->class == NULL) {
	    class_serial_num = 0;
	    class_name = hprof_get_array_type_name(site->is_array);
	} else {
	    class_serial_num = site->class->serial_num;
	    class_name = site->class->name->name;
	}

	accum_percent += ratio;
	if (output_format == 'b') {
	    hprof_write_u1((unsigned char)site->is_array);
	   
		    
	    hprof_write_u4(class_serial_num);
	    hprof_write_u4(site->trace_serial_num);
	    hprof_write_u4(site->n_live_bytes);
	    hprof_write_u4(site->n_live_instances);
	    hprof_write_u4(site->n_alloced_bytes);
	    hprof_write_u4(site->n_alloced_instances);
	} else {
	    hprof_printf("%5u %5.2f%% %5.2f%% %8u %4u %7u %4u %5u %s\n",
			 i + 1,
			 ratio * 100.0,
			 accum_percent * 100.0,
			 site->n_live_bytes,
			 site->n_live_instances,
			 site->n_alloced_bytes,
			 site->n_alloced_instances,
			 site->trace_serial_num,
			 class_name);
	}
    }
    if (output_format == 'a') {
        hprof_printf("SITES END\n");
    }

    hprof_hash_iterate(&hprof_site_table, hprof_site_mark_unchanged, NULL);

    hprof_free(iterate.sites);
    CALL(RawMonitorExit)(data_access_lock);
}


#ifdef HASH_STATS
void hprof_print_site_hash_stats(FILE *fp) {
    hprof_print_tbl_hash_stats(fp, &hprof_site_table);
}
#endif /* HASH_STATS */


#ifdef WATCH_ALLOCS
void hprof_free_site_table(void)
{
    hprof_hash_removeall(&hprof_site_table);
    hprof_hash_free(&hprof_site_table);
}
#endif /* WATCH_ALLOCS */
