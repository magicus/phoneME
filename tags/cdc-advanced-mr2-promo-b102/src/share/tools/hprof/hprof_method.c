/*
 * @(#)hprof_method.c	1.23 06/10/10
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

static hprof_hash_t hprof_method_table;

static unsigned int hash_method(void *_method)
{
    hprof_method_t *method = _method;
    return ((unsigned int)method->method_id >> HASH_OBJ_SHIFT) %
	hprof_method_table.size;
}

static unsigned int size_method(void *_method)
{
    return sizeof(hprof_method_t);
}

static int compare_method(void *_method1, void *_method2)
{
    hprof_method_t *method1 = _method1;
    hprof_method_t *method2 = _method2;

    return ((long)method1->method_id - 
	    (long)method2->method_id);
}

void hprof_method_table_init(void)
{
    hprof_hash_init(ALLOC_HASH_METHOD, &hprof_method_table, 4096,
		    hash_method, size_method, compare_method);
}

hprof_method_t * hprof_lookup_method(jmethodID method_id)
{
    hprof_method_t method_tmp;
    method_tmp.method_id = method_id;
    return hprof_hash_lookup(&hprof_method_table, &method_tmp);
}

hprof_method_t * hprof_intern_method(JVMPI_Method *jmethod, hprof_class_t *hclass) 
{
    hprof_method_t method_tmp;
    hprof_method_t *method;

    method_tmp.method_id = jmethod->method_id;
    method_tmp.method_name = hprof_intern_name(jmethod->method_name);
    method_tmp.method_signature = hprof_intern_name(jmethod->method_signature);
    method_tmp.class = hclass; 
    method = hprof_hash_intern(&hprof_method_table, &method_tmp);
    if (method == NULL) {
        fprintf(stderr, "HPROF ERROR: failed to intern method\n");
    }
    return method;
}

void hprof_method_entry_event(JNIEnv *env_id, 
			      jmethodID method_id)
{
    jlong start_time = CALL(GetCurrentThreadCpuTime)();
    hprof_thread_local_t *info = 
        (hprof_thread_local_t *)(CALL(GetThreadLocalStorage)(env_id));
       
    if (info == NULL) {
        /* we are seeing JNIEnv * for the first time, so we need to set up it's
	 * thread local table */
        hprof_intern_thread(env_id);
	info = (hprof_thread_local_t *)(CALL(GetThreadLocalStorage)(env_id));
	if (info == NULL) {
	    fprintf(stderr, 
		    "HPROF ERROR: thread local table NULL in method_entry %p\n",
		    env_id);
	    return;
	} 
    }
    { 
        hprof_method_time_t *stack_top = info->stack_top;
      	int limit = info->stack_limit;
    
	if (stack_top == (info->stack +  limit)) {
	    /* overflow - expand stack */
	    hprof_method_time_t *newstack = HPROF_CALLOC(ALLOC_TYPE_METHOD_TIME,
		sizeof(hprof_method_time_t) * 2 * limit);
#ifdef WATCH_ALLOCS
	    /* track number of threads that realloc and number of reallocs */
	    if (limit == HPROF_STACK_LIMIT) method_time_stack_threads++;
            method_time_stack_reallocs++;
#endif /* WATCH_ALLOCS */
	    memcpy(newstack, info->stack, limit*sizeof(hprof_method_time_t));
	    hprof_free(info->stack);
	    info->stack_limit = 2*limit;
	    info->stack = newstack;
	    info->stack_top = newstack + limit;
	    stack_top = info->stack_top;
	}
	stack_top->method_id = method_id;
	stack_top->start_time = start_time;
	stack_top->time_in_callees = jlong_zero;
	stack_top->time_in_gc = jlong_zero;
	info->stack_top++;
#ifdef WATCH_ALLOCS
	if (info->stack_top - info->stack > method_time_stack_peak) {
	    /* track peak stack usage */
	    method_time_stack_peak = info->stack_top - info->stack;
	}
#endif /* WATCH_ALLOCS */
    }
	
}  

void hprof_method_exit_event(JNIEnv *env_id, 
			     jmethodID method_id)
{
    hprof_method_time_t *stack_top;
    hprof_frames_cost_t *frames_cost_ptr;
    hprof_thread_local_t *info;
    int stack_depth;
    int trace_depth;
    unsigned int hash = 0;
    jlong total_time = jlong_zero;
    int i;
    

    info = (hprof_thread_local_t *)(CALL(GetThreadLocalStorage)(env_id));
    if (info == NULL) { /* error if thread local info is NULL */
        fprintf(stderr, 
		"HPROF ERROR: thread local table NULL in method exit %p\n",
		env_id);
        return;
    } 
        
    stack_depth = info->stack_top - info->stack; /* call stack depth */
    if (stack_depth == 0) {
        fprintf(stderr, "HPROF ERROR : stack underflow in method exit\n");
	return;
    }
    
    /* the depth of frames we should keep track for reporting */
    if (prof_trace_depth > stack_depth) {
        trace_depth = stack_depth;
    } else {
        trace_depth = prof_trace_depth;
    }
    
    info->stack_top--;            /* pop (info->stack_top = next free frame) */
    stack_top = info->stack_top; /* top method = method that exited */ 
    
    
    if (stack_top->method_id != method_id) {
        fprintf(stderr, 
		"HPROF ERROR: method on stack top != method exiting..\n");
	return;
    }
	
    /*
     * Compute the hash to store the call stack frames (list of method ids)
     * and the cost (self time, total time and count) for this method.
     * Since method ids are really object pointers, we strip off the low
     * order zeros to get a better hash.
     */
    hash = 0;
    for (i = 0; i < trace_depth; i++) {
        hash = 37 * hash +
            ((unsigned int)((stack_top - i)->method_id) >> HASH_OBJ_SHIFT);
    }
    hash &= HPROF_FRAMES_TABLE_MASK;

    CALL(RawMonitorEnter)(info->table_lock);  
    frames_cost_ptr = info->table[hash];
    
    /* hash into the table and try to find a match - if the same call stack 
     * trace exists, we just need to increment it's cost. */
    while (frames_cost_ptr != NULL) {
        jmethodID *frames = info->frames_array + frames_cost_ptr->frames_index;
	int num_frames = frames_cost_ptr->num_frames;
	
	if (num_frames == trace_depth) {
	    int found = 1; 
	    for (i = 0; ((i < trace_depth) && (found)); i++) { 
	        if ((stack_top - i)->method_id != (*(frames+i))) {
		    found = 0;
		    break;
		}
		
	    }
	    if (found) {
	        break;
	    }
	} 
	frames_cost_ptr = frames_cost_ptr->next;
    }
	
    if (frames_cost_ptr == NULL) { /* we haven't found a match */
        int frames_array_limit = info->frames_array_limit;
	int cur_frame_index = info->cur_frame_index;
	jmethodID *cur_frame;
	
#ifdef HASH_STATS
        info->table_misses++;
#endif /* HASH_STATS */
	/* check for frames_array overflow, expand if necessary */
	if ((cur_frame_index + trace_depth) > frames_array_limit) { 
	    jmethodID *new_frames_array;

	    frames_array_limit = 2*frames_array_limit;
	    new_frames_array = HPROF_CALLOC(ALLOC_TYPE_JMETHODID,
		sizeof(jmethodID) * frames_array_limit);
#ifdef WATCH_ALLOCS
	    /* track number of threads that realloc and number of reallocs */
	    if (frames_array_limit / 2 == HPROF_FRAMES_ARRAY_LIMIT) {
		jmethodID_array_threads++;
            }
            jmethodID_array_reallocs++;
#endif /* WATCH_ALLOCS */
	    memcpy(new_frames_array, info->frames_array, 
		   cur_frame_index*sizeof(jmethodID));
	    hprof_free(info->frames_array);
	    info->frames_array_limit = frames_array_limit;
	    info->frames_array = new_frames_array;
	}

	/* alloc a new frame_cost */
	frames_cost_ptr =
	    HPROF_CALLOC(ALLOC_TYPE_FRAMES_COST, sizeof(hprof_frames_cost_t));
	    
	/* copy the frames into the frames array and store the index into the 
	 * frames array for the top frame (method id) and the # of frames */
	cur_frame = info->frames_array + cur_frame_index;
	for (i = 0; i < trace_depth; i++) {
	    (*(cur_frame + i)) = (stack_top - i)->method_id;
	}
	info->cur_frame_index = cur_frame_index + trace_depth;
#ifdef WATCH_ALLOCS
        jmethodID_array_total += trace_depth; /* track total array usage */
	if (info->cur_frame_index > jmethodID_array_peak) {
	    /* track peak array usage */
	    jmethodID_array_peak = info->cur_frame_index;
	}
#endif /* WATCH_ALLOCS */
	    
	frames_cost_ptr->frames_index = cur_frame_index;
	frames_cost_ptr->num_frames = trace_depth;
	frames_cost_ptr->self_time = jlong_zero;
	frames_cost_ptr->total_time = jlong_zero;
	frames_cost_ptr->num_hits = 0;
	/* chain it to the hash table */
	frames_cost_ptr->next = info->table[hash];
	info->table[hash] = frames_cost_ptr;
    } 
#ifdef HASH_STATS
    else {
        info->table_hits++;
    }
#endif /* HASH_STATS */

    /* bill the cost */
    total_time = CVMlongSub(CALL(GetCurrentThreadCpuTime)(),
			    stack_top->start_time);
	
    if (CVMlongGt(total_time, jlong_zero) && (stack_depth > 1)) {  /* if a caller exists */
        (stack_top - 1)->time_in_callees = 
	    jlong_add((stack_top - 1)->time_in_callees, total_time);
    }

    frames_cost_ptr->self_time =
	jlong_add(frames_cost_ptr->self_time,
		  jlong_sub(total_time,
			    jlong_add(stack_top->time_in_callees,
				      stack_top->time_in_gc)));
    frames_cost_ptr->total_time = 
	jlong_add(frames_cost_ptr->total_time, total_time);
    frames_cost_ptr->num_hits++;
    CALL(RawMonitorExit)(info->table_lock);
}


#ifdef HASH_STATS
void hprof_print_method_hash_stats(FILE *fp) {
    hprof_print_tbl_hash_stats(fp, &hprof_method_table);
}
#endif /* HASH_STATS */


#ifdef WATCH_ALLOCS
void hprof_free_method_table(void)
{
    hprof_hash_removeall(&hprof_method_table);
    hprof_hash_free(&hprof_method_table);
}
#endif /* WATCH_ALLOCS */
