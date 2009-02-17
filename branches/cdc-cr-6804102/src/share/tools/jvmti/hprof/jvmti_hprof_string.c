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

/* Table of byte arrays (e.g. char* string + NULL byte) */

/*
 * Strings are unique by their own contents, since the string itself
 *   is the Key, and the hprof_table.c guarantees that keys don't move,
 *   this works out perfect. Any key in this table can be used as
 *   an char*.
 *
 * This does mean that this table has dynamically sized keys.
 *
 * Care needs to be taken to make sure the NULL byte is included, not for
 *   the sake of hprof_table.c, but so that the key can be used as a char*.
 *
 */

#include "jvmti_hprof.h"

void
string_init(void)
{
    HPROF_ASSERT(gdata->string_table==NULL);
    gdata->string_table = table_initialize("Strings", 4096, 4096, 1024, 0);
}

StringIndex
string_find_or_create(const char *str)
{
    return table_find_or_create_entry(gdata->string_table, 
		(void*)str, (int)strlen(str)+1, NULL, NULL);
}

static void
list_item(TableIndex index, void *str, int len, void *info_ptr, void *arg)
{
    debug_message( "0x%08x: String \"%s\"\n", index, (const char *)str);
}

void
string_list(void)
{
    debug_message( 
        "-------------------- String Table ------------------------\n");
    table_walk_items(gdata->string_table, &list_item, NULL);
    debug_message(
        "----------------------------------------------------------\n");
}

void
string_cleanup(void)
{
    table_cleanup(gdata->string_table, NULL, NULL);
    gdata->string_table = NULL;
}

char *
string_get(StringIndex index)
{
    void *key;
    int   key_len;
    
    table_get_key(gdata->string_table, index, &key, &key_len);
    HPROF_ASSERT(key_len>0);
    return (char*)key;
}

int
string_get_len(StringIndex index)
{
    void *key;
    int   key_len;

    table_get_key(gdata->string_table, index, &key, &key_len);
    HPROF_ASSERT(key_len>0);
    return key_len-1;
}

