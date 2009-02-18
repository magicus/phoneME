/*
 * @(#)hprof_thread.c	1.28 06/10/10
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

static hprof_thread_local_t *hprof_alloc_thread_local_info(void);
static void hprof_free_thread_local_info(JNIEnv *env_id);

static unsigned int thread_serial_number = 1;

static hprof_hash_t hprof_thread_table;

/* list of active threads */
live_thread_t *live_thread_list = NULL;
int num_live_threads = 0;

static unsigned int hash_thread(void *_thread)
{
    hprof_thread_t *thread = _thread;
    return (unsigned int)thread->env_id % hprof_thread_table.size;
}

static unsigned int size_thread(void *_thread)
{
    return sizeof(hprof_thread_t);
}

static int compare_thread(void *_thread1, void *_thread2)
{
    hprof_thread_t *thread1 = _thread1;
    hprof_thread_t *thread2 = _thread2;

    return ((long)thread1->env_id - 
	    (long)thread2->env_id);
}

void hprof_thread_table_init(void)
{
    /*
     * With 4005 threads, a size of 1003 yields a 99% occupancy rate
     * with an average element count value of 4.05. Changing the size
     * to 1024 and doing the usual bit shifting with HASH_OBJ_SHIFT
     * results in less than 50% occupancy with an average element count
     * value almost 8.
     *
     * On Win32 the JNIEnv pointer values always have the 0xC bits set
     * and on Solaris the 0x4 bit is always set. Changing the shift
     * value to drop the "constant" bits yields better occupancy and
     * average element count values, but still not as good as the
     * original table size.
     */
    hprof_hash_init(ALLOC_HASH_THREAD, &hprof_thread_table, 1003,
		    hash_thread, size_thread, compare_thread);
}

hprof_thread_t * hprof_intern_thread(JNIEnv *env_id)
{
    hprof_thread_t thread_tmp;
    hprof_thread_t *result;

    thread_tmp.env_id = env_id;
    result = hprof_hash_lookup(&hprof_thread_table, &thread_tmp);
    if (result == NULL) {
        thread_tmp.serial_num = thread_serial_number++;
	/* set thread id to NULL, it'll be set to the right value in
	   thread_start_event */
	thread_tmp.thread_id = NULL;
	result = hprof_hash_put(&hprof_thread_table, &thread_tmp);
	/* We need thread local storage for timing methods or tracking
	 * contended monitor entry.  We have to do it here, because we could
	 * get method entry/exit and monitor enter/entered events with 
	 * JNIEnv *s that haven't been notified in thread start. */
	if (cpu_timing || monitor_tracing) {
	    hprof_thread_local_t *info = hprof_alloc_thread_local_info();
	    CALL(SetThreadLocalStorage)(env_id, info);
	}
    } 
    return result;
}

static int hprof_remove_thread_helper(void *_thread, void *_env)
{
    hprof_thread_t *thread = _thread;
    JNIEnv *env_id = _env;
    return (thread->env_id == env_id);
}

void hprof_remove_thread(JNIEnv *env_id)
{
    hprof_hash_remove(&hprof_thread_table, 
		      hprof_remove_thread_helper, 
		      (void *)env_id);
}

hprof_thread_t *hprof_lookup_thread(JNIEnv *env_id)
{
    hprof_thread_t thread_tmp;
    hprof_thread_t *result;
    thread_tmp.env_id = env_id;
    result = hprof_hash_lookup(&hprof_thread_table, &thread_tmp);
    return result;
}
    
void hprof_thread_start_event(JNIEnv *env_id,
			      char *t_name,
			      char *g_name,
			      char *p_name,
			      jobjectID thread_id,
			      int requested)
{
    hprof_thread_t *result;
    hprof_name_t *thread_name;
    hprof_name_t *group_name;
    hprof_name_t *parent_name;
    hprof_objmap_t *objmap;
    
    CALL(RawMonitorEnter)(data_access_lock);

    objmap = hprof_fetch_object_info(thread_id);
    
    if (objmap == NULL) {
        fprintf(stderr, "HPROF ERROR: unable to map JVMPI thread ID to hprof "
		"thread ID  in thread_start \n");
	goto threadstart_done;
    }

    result = hprof_intern_thread(env_id);
    if (result->thread_id != NULL) {
	goto threadstart_done;
    }

    {
	live_thread_t *newthread =
	    HPROF_CALLOC(ALLOC_TYPE_LIVE_THREAD, sizeof(live_thread_t));

	/* add to the list of live threads */
	newthread->next = live_thread_list;
	newthread->tid = objmap;
	newthread->env = env_id;
	newthread->cpu_sampled = 1;
	live_thread_list = newthread;
	num_live_threads++;
    }

    result = hprof_intern_thread(env_id);
    if (result->thread_id != NULL) {
        fprintf(stderr, "HPROF ERROR : thread ID already in use\n");
	goto threadstart_done;
    }
    result->thread_id = objmap;
    thread_name = hprof_intern_name(t_name);
    group_name = hprof_intern_name(g_name);
    parent_name = hprof_intern_name(p_name);
#ifdef HASH_STATS
    if (cpu_timing) {
        hprof_thread_local_t *info = 
            (hprof_thread_local_t *)(CALL(GetThreadLocalStorage)(env_id));
	info->thread_name = thread_name;
	info->group_name  = group_name;
	info->parent_name = parent_name;
    }
#endif /* HASH_STATS */
    
    if (output_format == 'b') {
        int trace_num;
	if (requested) {
	    trace_num = 0;
	} else {
	    hprof_trace_t *htrace = hprof_get_trace(env_id, max_trace_depth);
	    if (htrace == NULL) {
	        fprintf(stderr, "HPROF ERROR : got NULL trace in thread_start\n");
		goto threadstart_done;
	    }
	    trace_num = htrace->serial_num;
	}
	hprof_write_header(HPROF_START_THREAD, sizeof(void *) * 4 + 8);
	hprof_write_u4(result->serial_num);
	hprof_write_id(objmap);
	hprof_write_u4(trace_num);
	hprof_write_id(thread_name);
	hprof_write_id(group_name);
	hprof_write_id(parent_name);
    } else if ((!cpu_timing) || (timing_format != OLD_PROF_OUTPUT_FORMAT)) {
        /* we don't want thread info for the old prof output format */
        hprof_printf("THREAD START "
		     "(obj=%x, id = %d, name=\"%s\", group=\"%s\")\n",
		     objmap, result->serial_num,
		     thread_name->name, group_name->name);
    }
 threadstart_done:
    CALL(RawMonitorExit)(data_access_lock); 
}

void hprof_thread_end_event(JNIEnv *env_id)
{
    hprof_thread_t thread_tmp;
    hprof_thread_t *thread;
    
    CALL(RawMonitorEnter)(data_access_lock);
#ifdef XXX_HASH_STATS
{
    /* use the code to get current thread hash stats */
    static int done = 0;

    if (!done) {
	hprof_print_thread_hash_stats(stderr);
	done = 1;
    }
}
#endif /* HASH_STATS */

    {
        /* remove from list of live threads */
        live_thread_t **p;
	p = &live_thread_list;
	while (*p) {
	    live_thread_t *t = *p;
	    if (t->env == env_id) {
	        *p = t->next;
		hprof_free(t);
		break;
	    }
	    p = &(t->next);
	}
	num_live_threads--;
    }
    thread_tmp.env_id = env_id;
    thread = hprof_hash_lookup(&hprof_thread_table, &thread_tmp);
    if (thread == NULL) {
        fprintf(stderr, "HPROF ERROR : unknown thread ID in thread_end\n");
    } else {
        if (output_format == 'b') {
	    hprof_write_header(HPROF_END_THREAD, 4);
	    hprof_write_u4(thread->serial_num);
	} else if ((!cpu_timing) || (timing_format != OLD_PROF_OUTPUT_FORMAT)) {
	    /* we don't want thread info for the old prof output format */
	    hprof_printf("THREAD END (id = %d)\n", thread->serial_num);
	}
	if (cpu_timing) {
	    /* bill the thread local table if method timing is on */
	    hprof_bill_frames_cost_table(env_id);
	}
	if (cpu_timing || monitor_tracing) {
	    /* free the thread local table if we allocated one */
	    hprof_free_thread_local_info(env_id);
	}
	hprof_remove_thread(env_id);
    }
    CALL(RawMonitorExit)(data_access_lock);
}

hprof_thread_t * hprof_fetch_thread_info(JNIEnv *env)
{
    hprof_thread_t *thread = hprof_lookup_thread(env);
    if (thread == NULL) {
        jobjectID tobj = CALL(GetThreadObject)(env);
	if (tobj) {
	    CALL(RequestEvent)(JVMPI_EVENT_THREAD_START, tobj);
	}
	thread = hprof_lookup_thread(env);
    }
    return thread;
}

void hprof_print_thread_info(JNIEnv *env, int leading_comma)
{
    hprof_thread_t *thread = hprof_lookup_thread(env);
    if (leading_comma) {
        hprof_printf(",");
    }
    if (thread == NULL) {
        hprof_printf(" <unknown thread>");
    } else {
        hprof_printf(" thread %d", thread->serial_num);
    }
}

static hprof_thread_local_t *hprof_alloc_thread_local_info(void)
{
    hprof_thread_local_t *info =
        HPROF_CALLOC(ALLOC_TYPE_THREAD_LOCAL, sizeof(hprof_thread_local_t));
    
    if (cpu_timing) {
        char lockname[128];
	static int lock_serial_number = 0;
	
        info->stack = HPROF_CALLOC(ALLOC_TYPE_METHOD_TIME,
	    sizeof(hprof_method_time_t) * HPROF_STACK_LIMIT);
	info->stack_top = info->stack;
	info->stack_limit = HPROF_STACK_LIMIT;
	sprintf(lockname, "_hprof_thread_local_lock-%d", lock_serial_number++);
	info->table_lock = CALL(RawMonitorCreate)(lockname);
	info->frames_array_limit = HPROF_FRAMES_ARRAY_LIMIT;
	info->frames_array = HPROF_CALLOC(ALLOC_TYPE_JMETHODID,
	    sizeof(jmethodID) * info->frames_array_limit);
	info->cur_frame_index = 0;
	info->table =
	    HPROF_CALLOC(ALLOC_TYPE_HASH_TABLES + ALLOC_HASH_FRAMES_COST,
	        sizeof(hprof_frames_cost_t *) * HPROF_FRAMES_TABLE_SIZE);
#ifdef HASH_STATS
        info->table_hits   = 0;
        info->table_misses = 0;
	info->thread_name  = NULL;
	info->group_name   = NULL;
	info->parent_name  = NULL;
#endif /* HASH_STATS */
	info->gc_start_time = jint_to_jlong(-1);
    }
    
    if (monitor_tracing) {
        info->mon = HPROF_CALLOC(ALLOC_TYPE_CONTENDED_MONITOR,
	    sizeof(hprof_contended_monitor_t));
	info->mon->time = jint_to_jlong(-1);
	info->mon->trace_serial_num = 0;
	info->mon->num_hits = 0;
    }
    
    return info;
}

static void hprof_free_thread_local_info(JNIEnv *env_id)
{
    hprof_thread_local_t *info = 
        (hprof_thread_local_t *)(CALL(GetThreadLocalStorage)(env_id));
    
    if (info == NULL) {
        fprintf(stderr, 
		"HPROF ERROR: thread local table NULL for env_id = %p," 
		" cannot free it\n", env_id);
	return;
    }

    if (cpu_timing) {
        hprof_frames_cost_t **table;
	int i;

#ifdef HASH_STATS
	if (print_thread_hash_stats_on_exit) {
	    hprof_print_per_thread_hash_stats(stderr, info);
	}
#endif /* HASH_STATS */

	table = info->table;
	for (i = 0; i < HPROF_FRAMES_TABLE_SIZE; i++) {
            hprof_frames_cost_t *fc = table[i];
	    while (fc != NULL) {
	        hprof_frames_cost_t *next = fc->next;
		hprof_free(fc);
		fc = next;
	    }
	}
	hprof_free(info->table);
	CALL(RawMonitorDestroy)(info->table_lock);
	hprof_free(info->stack);
	hprof_free(info->frames_array);
    }

    if (monitor_tracing) {
        hprof_free(info->mon);
    }

    CALL(SetThreadLocalStorage)(env_id,NULL);
    hprof_free(info);
}

static void hprof_bill_frames_cost(hprof_frames_cost_t *fc, JNIEnv *env_id, jmethodID *frames)
{
    jlong self_time = jlong_div(fc->self_time, jint_to_jlong(1000000));/* convert to ms */
    jlong cost = jlong_zero;
    int num_hits = fc->num_hits;
    int bill_it = 0;
            
    if ((timing_format == OLD_PROF_OUTPUT_FORMAT) && (num_hits > 0)) {
        env_id = NULL;                   /* no thread info for old prof */
	cost = jlong_div(fc->total_time, jint_to_jlong(1000000));    /* convert to ms */
	bill_it = 1;
    } else if ((timing_format == NEW_PROF_OUTPUT_FORMAT) &&
	       CVMlongGt(self_time, jlong_zero)) {
        cost = self_time;
	bill_it = 1;
    }
    
    /* if we are billing */
    if (bill_it) {
        int i;
	int n_frames = fc->num_frames;
	hprof_trace_t *trace_tmp = hprof_alloc_tmp_trace(n_frames, env_id);
        hprof_trace_t *result;

	for (i = 0; i < n_frames; i++) {
	    hprof_frame_t *frame = hprof_intern_jvmpi_frame(frames[i],-1);
	    if (frame == NULL) {
	        fprintf(stderr, 
		    "HPROF ERROR: got a NULL frame in bill_frames_cost\n");
		hprof_free(trace_tmp);
		return;
	    } 
	    trace_tmp->frames[i] = frame;
	}
	result = hprof_intern_tmp_trace(trace_tmp);

	/* bill the cost and num_hits and zero out the values in frames_cost
	 * to prevent them from getting billed more than once */
	result->cost = jlong_add(result->cost, cost);
	result->num_hits += num_hits;
	    
	fc->self_time = jlong_zero;
	fc->total_time = jlong_zero;
	fc->num_hits = 0;
    }
}

void hprof_bill_frames_cost_table(JNIEnv *env_id)
{
    hprof_thread_local_t *info;
    int i;

    if (!cpu_timing && !monitor_tracing) {
	return;
    }

    info = (hprof_thread_local_t *)(CALL(GetThreadLocalStorage)(env_id));
    if (info == NULL) {
        fprintf(stderr, 
		"HPROF ERROR: thread local table NULL for env_id = %p," 
		"cannot bill cost\n", env_id);
	return;
    }

    CALL(RawMonitorEnter)(info->table_lock);
    for (i = 0; i < HPROF_FRAMES_TABLE_SIZE; i++) {
        hprof_frames_cost_t *fc = info->table[i];
	while (fc) {
	    hprof_bill_frames_cost(fc, env_id, 
				   (info->frames_array + fc->frames_index));
	    fc = fc->next;
	}
    }
    CALL(RawMonitorExit)(info->table_lock);
}

static void *hprof_bill_thread_local_table(void *_thread, void * _arg)
{
    hprof_thread_t *thread = _thread;
    JNIEnv *env_id = thread->env_id;
    hprof_bill_frames_cost_table(env_id);
    return _arg;
}
    
void hprof_bill_all_thread_local_tables(void) 
{
    hprof_hash_iterate(&hprof_thread_table,
		       hprof_bill_thread_local_table,
		       NULL);
}


#ifdef HASH_STATS
void
hprof_print_per_thread_hash_stats(FILE *fp, hprof_thread_local_t *info)
{
    unsigned int empties = 0;       /* number of empty slots */
    unsigned int i;                 /* scratch index */
    unsigned int prev_elems;        /* previous element count */

    fprintf(fp, "\nThread Hash Table Statistics for '%s'\n\n",
	info->thread_name->name);
    fprintf(fp, "parent name='%s'  group name='%s'\n", info->parent_name->name,
	info->group_name->name);

    if (print_verbose_hash_stats) {
	fprintf(fp, "Table Histogram:\n");
	fprintf(fp, "'K'-1024 elements, '#'-100 elements, '@'-1 element\n");
    }

    prev_elems = 0;    /* skip leading empty slots */
    for (i = 0; i < HPROF_FRAMES_TABLE_SIZE; i++) {
	unsigned int        count;          /* # of markers to print */
	unsigned int        elems = 0;      /* # of elements in slot */
        hprof_frames_cost_t *fc;            /* element index */
	unsigned int        saved_elems;    /* saved elems value */

	for (fc = info->table[i]; fc != NULL; fc = fc->next) {
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

    fprintf(fp,
	"#-hits=%u  #-misses=%u  empty-slots=%.2f%%  avg-#-elems=%.2f\n\n",
        info->table_hits, info->table_misses,
	((float)empties / HPROF_FRAMES_TABLE_SIZE) * 100.0,
	(float)info->table_misses / (HPROF_FRAMES_TABLE_SIZE - empties));
}


void hprof_print_thread_hash_stats(FILE *fp) {
    hprof_print_tbl_hash_stats(fp, &hprof_thread_table);
}
#endif /* HASH_STATS */


#ifdef WATCH_ALLOCS
static void * hprof_free_thread_table_helper(void *_thread, void *_arg)
{
    hprof_thread_t *thread = _thread;
    hprof_free_thread_local_info(thread->env_id);
    return _arg;
}


void hprof_free_thread_table(void)
{
    hprof_hash_iterate(&hprof_thread_table, hprof_free_thread_table_helper,
	NULL);
    hprof_hash_removeall(&hprof_thread_table);
    hprof_hash_free(&hprof_thread_table);

    /* empty the list of live threads */
    {
	live_thread_t *t;
	for (t = live_thread_list; t != NULL; ) {
	    live_thread_t *prev = t;  /* save the one we want to free */
	    t = t->next;
	    hprof_free(prev);
	    num_live_threads--;
	}
    }
}
#endif /* WATCH_ALLOCS */
