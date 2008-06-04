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

/* Used to store strings written out to the binary format (see hprof_io.c) */


/* Probably could have used the basic string table, however, some strings
 *   would only be in this table, so it was isolated as a separate table
 *   of strings.
 */

#include "jvmti_hprof.h"
#include "jvmti_hprof_ioname.h"

void
ioname_init(void)
{
    HPROF_ASSERT(gdata->ioname_table==NULL);
    gdata->ioname_table = table_initialize("IoNames", 512, 512, 511, 0);
}

IoNameIndex
ioname_find_or_create(const char *name, jboolean *pnew_entry)
{
    return table_find_or_create_entry(gdata->ioname_table, 
			(void*)name, (int)strlen(name)+1, pnew_entry, NULL);
}

void
ioname_cleanup(void)
{
    table_cleanup(gdata->ioname_table, NULL, NULL);
    gdata->ioname_table = NULL;
}

