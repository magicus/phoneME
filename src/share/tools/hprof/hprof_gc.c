/*
 * @(#)hprof_gc.c	1.11 06/10/10
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

void hprof_gc_start_event(JNIEnv *env_id)
{
    jlong start_time;
    hprof_thread_local_t *info; 

    /* grab the data_access_lock here, because obj_move and obj_free come
     * in single threaded mode and we could run into deadlocks by grabbing
     * the lock in those handlers. */
    CALL(RawMonitorEnter)(data_access_lock);  

    if (cpu_timing) {
        start_time = CALL(GetCurrentThreadCpuTime)();
	info = (hprof_thread_local_t *)(CALL(GetThreadLocalStorage)(env_id));
	if (info == NULL) {
	    fprintf(stderr, "HPROF ERROR: gc_start on an unknown thread %p\n",
		    env_id);
	    return;
	}

	if (CVMlongNe(info->gc_start_time, jint_to_jlong(-1))) {
	    fprintf(stderr, "HPROF ERROR: got gc_start instead of gc_end\n");
	    return;
	}
    
	info->gc_start_time = start_time;
    }
}
 
void hprof_gc_finish_event(JNIEnv *env_id, jlong used_objects,
			   jlong used_object_space, jlong total_object_space)
{
    jlong gc_time;
    hprof_thread_local_t *info;

    if (cpu_timing) { /* for subtracting GC time from method time */
        info = (hprof_thread_local_t *)(CALL(GetThreadLocalStorage)(env_id));
    
	if (info == NULL) {
	    fprintf(stderr, "HPROF ERROR: gc_end on an unknown thread %p\n",
		    env_id);
	    return;
	}

	if (info->gc_start_time == (jlong)-1) {
	    fprintf(stderr, "HPROF ERROR: got gc_end instead of gc_start\n");
	    return;
	}

	gc_time = CVMlongSub(CALL(GetCurrentThreadCpuTime)(),
			     info->gc_start_time);
	
        if ((info->stack_top - info->stack) > 0) {
	    (info->stack_top - 1)->time_in_gc =
		CVMlongAdd((info->stack_top - 1)->time_in_gc, gc_time);
	}
	info->gc_start_time = jint_to_jlong(-1); /* reset gc_start_time */
    }
    
    CALL(RawMonitorExit)(data_access_lock); /* we grabbed this in gc_start */
}
    
    
  
