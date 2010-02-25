/*
 * @(#)hprof_jni.c	1.13 06/10/10
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

static hprof_globalref_t *hprof_globalrefs = NULL;

static void
hprof_globalref_add(hprof_objmap_t *obj_id, 
		    jobject ref_id, 
		    unsigned int trace_serial_num)
{
    hprof_globalref_t *globalref =
	HPROF_CALLOC(ALLOC_TYPE_GLOBALREF, sizeof(hprof_globalref_t));
    globalref->obj_id = obj_id;
    globalref->ref_id = ref_id;
    globalref->trace_serial_num = trace_serial_num;
    globalref->next = hprof_globalrefs;
    hprof_globalrefs = globalref;
}

static void 
hprof_globalref_del(jobject ref_id)
{
    hprof_globalref_t **p = &hprof_globalrefs;
    hprof_globalref_t *globalref;
    while ((globalref = *p)) {
        if (globalref->ref_id == ref_id) {
	    *p = globalref->next;
	    hprof_free(globalref);
	    return;
	}
	p = &(globalref->next);
    }
}

hprof_globalref_t * hprof_globalref_find(jobject ref_id)
{
    hprof_globalref_t *globalref = hprof_globalrefs;
    while (globalref) {
        if (globalref->ref_id == ref_id) {
	    return globalref;
	}
	globalref = globalref->next;
    }
    return NULL;
}

void hprof_jni_globalref_alloc_event(JNIEnv *env_id, jobjectID obj_id, jobject ref_id)
{
    hprof_trace_t *htrace;
    hprof_objmap_t *objmap;
    
    CALL(RawMonitorEnter)(data_access_lock);
    htrace = hprof_get_trace(env_id, max_trace_depth);
    if (htrace == NULL) {
        fprintf(stderr, "HPROF ERROR: got NULL trace in jni_globalref_alloc\n");
	goto globalref_alloc_done;
    } 
    
    objmap = hprof_fetch_object_info(obj_id);
    if (objmap == NULL) {
        fprintf(stderr, "HPROF ERROR: unable to map JVMPI obj ID to hprof "
		"obj ID in globalref_alloc \n");
	goto globalref_alloc_done;
    }
   
    hprof_globalref_add(objmap, ref_id, htrace->serial_num);

 globalref_alloc_done:
    CALL(RawMonitorExit)(data_access_lock);
}
    
void hprof_jni_globalref_free_event(JNIEnv *env_id, jobject ref_id)
{
    CALL(RawMonitorEnter)(data_access_lock); 
    hprof_globalref_del(ref_id);
    CALL(RawMonitorExit)(data_access_lock);
}

