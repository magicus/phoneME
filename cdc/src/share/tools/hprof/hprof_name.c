/*
 * @(#)hprof_name.c	1.16 06/10/10
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

static hprof_hash_t hprof_name_table;

static unsigned int hash_name(void *_hname)
{
    hprof_name_t *hname = _hname;
    unsigned int hash = 0;
    int len = strlen(hname->name);
    int i;

    for (i = 0; i < len; i++) {
        hash = 37*hash + hname->name[i];
    }
    return hash % hprof_name_table.size;
}

static unsigned int size_name(void *_name)
{
    return sizeof(hprof_name_t);
}

static int compare_name(void *_hname1, void *_hname2)
{
    hprof_name_t *hname1 = _hname1;
    hprof_name_t *hname2 = _hname2;

    return (strcmp(hname1->name, hname2->name));
}

void hprof_name_table_init(void)
{
    hprof_hash_init(ALLOC_HASH_NAME, &hprof_name_table, 8192,
		    hash_name, size_name, compare_name);
}

void hprof_output_name(hprof_name_t *name)
{
    if (output_format == 'b') {
        if (name->marked) {
	    return;
	} else {
	    name->marked = 1;
	    if (name->name) {
	        hprof_write_header(HPROF_UTF8, strlen(name->name) + sizeof(void *));
		hprof_write_id(name);
                hprof_write_raw((void *)name->name, strlen(name->name));
	    }
	}
    }
}

hprof_name_t * hprof_intern_name(const char *name)
{
    hprof_name_t name_tmp;
    hprof_name_t *result;
    
    if (name == NULL) {
        name = "<Unknown>";
    }
    name_tmp.name = name;
    result = hprof_hash_lookup(&hprof_name_table, &name_tmp);
    if (result == NULL) {
        name_tmp.name = strdup(name);
	name_tmp.marked = 0;
	result = hprof_hash_put(&hprof_name_table, &name_tmp);
    }
    hprof_output_name(result);
    return result;
}


#ifdef HASH_STATS
void hprof_print_name_hash_stats(FILE *fp) {
    hprof_print_tbl_hash_stats(fp, &hprof_name_table);
}
#endif /* HASH_STATS */


#ifdef WATCH_ALLOCS
void hprof_free_name_table(void)
{
    hprof_hash_removeall(&hprof_name_table);
    hprof_hash_free(&hprof_name_table);
}
#endif /* WATCH_ALLOCS */
