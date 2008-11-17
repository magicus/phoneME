/*
 * @(#)hprof_monitor.c	1.23 06/10/10
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

static hprof_hash_t hprof_raw_monitor_table;
static hprof_hash_t hprof_contended_monitor_table;

#ifdef DEBUG
#define SANITY_CHECK(name) \
    if (!monitor_tracing) {                                               \
        fprintf(stderr,                                                   \
	    "HPROF ERROR: %s: called while !monitor_tracing, exiting.\n", \
	    name);                                                        \
	CALL(ProfilerExit)((jint)1);                                      \
    }
#else /* !DEBUG */
#define	SANITY_CHECK(name)
#endif /* DEBUG */

static unsigned int hash_raw_monitor(void *_rmon)
{
    hprof_raw_monitor_t *rmon = _rmon;

    SANITY_CHECK("hash_raw_monitor");
    return ((long)rmon->id + 37*(long)rmon->name) %
	hprof_raw_monitor_table.size;
}

static unsigned int size_raw_monitor(void *_rmon)
{
    SANITY_CHECK("size_raw_monitor");
    return sizeof(hprof_raw_monitor_t);
}

static int compare_raw_monitor(void *_rmon1, void *_rmon2)
{
    hprof_raw_monitor_t *rmon1 = _rmon1;
    hprof_raw_monitor_t *rmon2 = _rmon2;
    int result;

    SANITY_CHECK("compare_raw_monitor");
    result = ((long)rmon1->name - (long)rmon2->name);
    if (result) {
        return result;
    }

    result = ((long)rmon1->id - (long)rmon2->id);
    return result;
}

void hprof_raw_monitor_table_init(void)
{
    SANITY_CHECK("hprof_raw_monitor_table_init");
    hprof_hash_init(ALLOC_HASH_RAW_MONITOR, &hprof_raw_monitor_table, 2003,
		    hash_raw_monitor, size_raw_monitor, compare_raw_monitor);
}

hprof_raw_monitor_t * 
hprof_intern_raw_monitor(JVMPI_RawMonitor id, hprof_name_t *name)
{
    hprof_raw_monitor_t rmon_tmp;

    SANITY_CHECK("hprof_intern_raw_monitor");
    rmon_tmp.name = name;
    rmon_tmp.id = id;
    return hprof_hash_intern(&hprof_raw_monitor_table, &rmon_tmp);
}

static unsigned int hash_contended_monitor(void *_cmon)
{
    hprof_contended_monitor_t *cmon = _cmon;
    unsigned int hash;

    SANITY_CHECK("hash_contended_monitor");
    hash = cmon->type + 37*(long)cmon->mon_info;
    return (37*hash + cmon->trace_serial_num) %
	hprof_contended_monitor_table.size;
}

static unsigned int size_contended_monitor(void *_cmon)
{
    SANITY_CHECK("size_contended_monitor");
    return sizeof(hprof_contended_monitor_t);
}

static int compare_contended_monitor(void *_cmon1, void *_cmon2)
{
    hprof_contended_monitor_t *cmon1 = _cmon1;
    hprof_contended_monitor_t *cmon2 = _cmon2;
    int result;

    SANITY_CHECK("compare_contended_monitor");
    result = cmon1->type - cmon2->type;
    if (result) {
        return result;
    }
    
    result = cmon1->trace_serial_num - cmon2->trace_serial_num;
    if (result) {
        return result;
    }

    result = ((long)cmon1->mon_info - (long)cmon2->mon_info);
    return result;
}

void hprof_contended_monitor_table_init(void)
{
    SANITY_CHECK("hprof_contended_monitor_table_init");
    hprof_hash_init(ALLOC_HASH_CONTENDED_MONITOR,
		    &hprof_contended_monitor_table, 2003,
		    hash_contended_monitor, size_contended_monitor, 
		    compare_contended_monitor);
}

static void * hprof_clear_cmon_helper(void *_cmon, void *_arg)
{
    hprof_contended_monitor_t *cmon = _cmon;
    SANITY_CHECK("hprof_clear_cmon_helper");
    cmon->time = jlong_zero;
    return _arg;
}

void hprof_clear_contended_monitor_table(void)
{
    SANITY_CHECK("hprof_clear_contended_monitor_table");
    hprof_hash_iterate(&hprof_contended_monitor_table, 
		       hprof_clear_cmon_helper, NULL);
}

static void * hprof_contended_monitor_collect(void *_cmon, void *_arg)
{
    hprof_cmon_iterate_t *arg = _arg;
    hprof_contended_monitor_t *cmon = _cmon;

    SANITY_CHECK("hprof_contended_monitor_collect");
    arg->cmons[arg->index++] = cmon;
    arg->total_time = jlong_add(arg->total_time, cmon->time);
    return _arg;
}

static int hprof_contended_monitor_compare(const void *p_cmon1, 
					   const void *p_cmon2)
{
    hprof_contended_monitor_t *cmon1;
    hprof_contended_monitor_t *cmon2;

    SANITY_CHECK("hprof_contended_monitor_compare");
    cmon1 = *(hprof_contended_monitor_t **)p_cmon1;
    cmon2 = *(hprof_contended_monitor_t **)p_cmon2;
    return jlong_to_jint(jlong_sub(cmon2->time, cmon1->time));
}

/* contended monitor output */
void hprof_output_cmon_times(float cutoff)
{
    hprof_cmon_iterate_t iterate;
    int i, contended_monitor_table_size, n_items;
    float accum, percent;

    SANITY_CHECK("hprof_output_cmon_times");
    /* First write all trace we might refer to. */
    hprof_output_unmarked_traces();

    if (hprof_contended_monitor_table.n_entries)
        iterate.cmons = HPROF_CALLOC(ALLOC_TYPE_ARRAY,
	    hprof_contended_monitor_table.n_entries * sizeof(void *));
    else
	iterate.cmons = NULL;

    iterate.total_time = jlong_zero;
    iterate.index = 0;
    hprof_hash_iterate(&hprof_contended_monitor_table, 
		       hprof_contended_monitor_collect, &iterate);

    contended_monitor_table_size = iterate.index;

    /* sort all the traces according to the cost */
    qsort(iterate.cmons, contended_monitor_table_size, 
	  sizeof(void *), hprof_contended_monitor_compare);
    n_items = 0;
    for (i = 0; i < contended_monitor_table_size; i++) {
        hprof_contended_monitor_t *cmon = iterate.cmons[i];
	percent =
	    CVMlong2Float(cmon->time) / CVMlong2Float(iterate.total_time);
	if (percent < cutoff) {
	    break;
	}
	n_items++;
    }
    /* output the info */
    if (output_format == 'a') {
        time_t t = time(0);
	int total_time = jlong_to_jint(jlong_div(iterate.total_time,
						 jint_to_jlong(1000000)));
	hprof_printf("MONITOR TIME BEGIN (total = %u ms) %s",  
		     total_time, ctime(&t));
	if (total_time > 0) {
	    hprof_printf("rank   self  accum   count trace monitor\n");
	    accum = 0;
	    for (i = 0; i < n_items; i++) {
	        hprof_contended_monitor_t *cmon = iterate.cmons[i];

		percent = CVMlong2Float(cmon->time) / 
		    CVMlong2Float(iterate.total_time) * 100.0;
		accum += percent;
		hprof_printf("%4u %5.2f%% %5.2f%% %7u %5u",
			     i + 1, percent, accum, cmon->num_hits,
			     cmon->trace_serial_num);
		if (cmon->type == JVMPI_MONITOR_RAW) {
		    hprof_raw_monitor_t *rmon = cmon->mon_info;
		    hprof_printf(" \"%s\"(%x) (Raw)\n", rmon->name->name, rmon->id);
		}

		if (cmon->type == JVMPI_MONITOR_JAVA) {
		    hprof_objmap_t *objmap = cmon->mon_info;
		    hprof_objmap_print(objmap);
		    hprof_printf(" (Java)\n");
		}
	    }
	}
	hprof_printf("MONITOR TIME END\n");
    } 
    if (iterate.cmons != NULL) hprof_free(iterate.cmons);
}

static void hprof_contended_monitor_enter(jint type, void *mon_info,
					  JNIEnv *env_id,
					  jlong cur_time)
{
    hprof_thread_local_t *info;
    hprof_contended_monitor_t *mon;

    SANITY_CHECK("hprof_contended_monitor_enter");
    info = (hprof_thread_local_t *)(CALL(GetThreadLocalStorage)(env_id));
    if (info == NULL) {
        /* we are seeing JNIEnv * after it's thread end !!Workaround */
        hprof_intern_thread(env_id);
	info = (hprof_thread_local_t *)(CALL(GetThreadLocalStorage)(env_id));
	if (info == NULL) {
	    fprintf(stderr, 
		    "HPROF ERROR: thread local table NULL in contended monitor " 
		    "enter %p\n", env_id);
	    return;
	}
    }
 
    mon = info->mon;
    if (CVMlongGe(mon->time, jlong_zero)) {
        fprintf(stderr,
		"HPROF ERROR: contended monitor, enter instead of entered\n");
	return;
    }
    mon->time = cur_time;
    mon->type = type;
    mon->mon_info = mon_info;
}

static void hprof_contended_monitor_entered(jint type, void *mon_info,
					    JNIEnv *env_id, 
					    jlong cur_time)
{
    hprof_thread_local_t *info;
    hprof_contended_monitor_t *mon;
    hprof_contended_monitor_t *imon;
    hprof_trace_t *htrace;

    SANITY_CHECK("hprof_contended_monitor_entered");
    info = (hprof_thread_local_t *)(CALL(GetThreadLocalStorage)(env_id));
    if (info == NULL) {
        fprintf(stderr, 
		"HPROF ERROR: thread local table NULL in contended monitor " 
		"entered %p\n", env_id);
	return;
    }

    mon = info->mon;
    
    if (CVMlongLt(mon->time, jlong_zero)) {
        fprintf(stderr,
		"HPROF ERROR: contended monitor, entered instead of enter\n");
	return;
    } 
    
    if (mon->type != type) {
        fprintf(stderr, "HPROF ERROR: contended monitor, type mismatch\n");
	return;
    }
    
    if (mon->mon_info != mon_info) {
        fprintf(stderr, "HPROF ERROR: contended monitor, monitor mismatch\n");
	return;
    }
	    
    htrace = hprof_get_trace(env_id, max_trace_depth);
    mon->trace_serial_num = htrace->serial_num;
    imon = hprof_hash_lookup(&hprof_contended_monitor_table, mon);
    if (imon == NULL) {
        mon->time = jlong_sub(cur_time, mon->time);
	mon->num_hits = 1;
	hprof_hash_put(&hprof_contended_monitor_table, mon);
    } else {
        imon->time = jlong_add(imon->time, jlong_sub(cur_time, mon->time));
	imon->num_hits++;
    }

    info->mon->time = jint_to_jlong(-1);
}
    
void hprof_raw_monitor_event(JVMPI_Event *event,
                             const char *name,
			     JVMPI_RawMonitor mid)
{
    jlong cur_time;
    hprof_name_t *hname;
    hprof_raw_monitor_t *rmon;
    JNIEnv *env_id;

    SANITY_CHECK("hprof_raw_monitor_event");
    cur_time = CALL(GetCurrentThreadCpuTime)();
    hname = hprof_intern_name(name);
    rmon = hprof_intern_raw_monitor(mid, hname);
    env_id = event->env_id;

    CALL(RawMonitorEnter)(data_access_lock);
    
    if (output_format == 'a') {
    	switch(event->event_type) {
	case JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTER: 
	    hprof_contended_monitor_enter(JVMPI_MONITOR_RAW, rmon,
					  env_id, cur_time);
	    break;
	case JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTERED: 
	    hprof_contended_monitor_entered(JVMPI_MONITOR_RAW, rmon,
					    env_id, cur_time);
	    break;
	}
    }
    
    CALL(RawMonitorExit)(data_access_lock);
}

void hprof_monitor_event(JVMPI_Event *event, jobjectID obj)
{
    jlong cur_time;
    JNIEnv *env_id;
    hprof_objmap_t *objmap; 

    SANITY_CHECK("hprof_monitor_event");
    cur_time = CALL(GetCurrentThreadCpuTime)();
    env_id = event->env_id;

    if (obj == NULL) {
        return;
    }

    CALL(RawMonitorEnter)(data_access_lock);
    
    objmap = hprof_fetch_object_info(obj);
    if (objmap == NULL) {
        fprintf(stderr, "HPROF ERROR: unknown object ID 0x%p\n", obj);
    }
        
    if (output_format == 'a') {
        switch (event->event_type) {
	case JVMPI_EVENT_MONITOR_CONTENDED_ENTER:
	    hprof_contended_monitor_enter(JVMPI_MONITOR_JAVA, objmap,
					  env_id, cur_time);
	    break;
	case JVMPI_EVENT_MONITOR_CONTENDED_ENTERED:
	    hprof_contended_monitor_entered(JVMPI_MONITOR_JAVA, objmap,
					  env_id, cur_time);
	    break;
	}
    }
    
    CALL(RawMonitorExit)(data_access_lock);
}

void hprof_monitor_wait_event(JVMPI_Event *event, jobjectID obj,
			      jlong timeout)
{
    SANITY_CHECK("hprof_monitor_wait_event");
    CALL(RawMonitorEnter)(data_access_lock);
    if (output_format == 'a') {
	hprof_fetch_thread_info(event->env_id);
        if (obj == NULL) {
	    hprof_printf("SLEEP:");
	    hprof_printf(" timeout=%d,", timeout);
	    hprof_print_thread_info(event->env_id, FALSE);
	    hprof_printf("\n");
	    goto done;
	}
	hprof_printf("WAIT: MONITOR");
	hprof_print_object_info(obj);
	hprof_printf(", timeout=%d,", timeout);
	hprof_print_thread_info(event->env_id, FALSE);
	hprof_printf("\n");
    }
 done:
    CALL(RawMonitorExit)(data_access_lock);
}

void hprof_dump_monitors(void)
{
    SANITY_CHECK("hprof_dump_monitors");
    CALL(RequestEvent)(JVMPI_EVENT_MONITOR_DUMP, NULL);
}

void hprof_monitor_dump_event(JVMPI_Event *event)
{
    int i;
    hprof_trace_t **traces = NULL;

    SANITY_CHECK("hprof_monitor_dump_event");
    CALL(RawMonitorEnter)(data_access_lock);
    if (event->u.monitor_dump.num_traces) {
        traces = HPROF_CALLOC(ALLOC_TYPE_ARRAY,
	    event->u.monitor_dump.num_traces * sizeof(void *));
    }

    for (i = 0; i < event->u.monitor_dump.num_traces; i++) {
        JVMPI_CallTrace *jtrace = &(event->u.monitor_dump.traces[i]);
	traces[i] = hprof_intern_jvmpi_trace(jtrace->frames,
					     jtrace->num_frames,
					     jtrace->env_id);
    }
    hprof_output_unmarked_traces();

    if (output_format == 'a') {
        hprof_dump_seek(event->u.monitor_dump.begin);
	while(hprof_dump_cur() < event->u.monitor_dump.end) {
	    int kind = hprof_dump_read_u1();
	    int n;
	    JNIEnv *thread_env;
	    if (kind == JVMPI_MONITOR_JAVA) {
                hprof_dump_read_id();
	    } else if (kind == JVMPI_MONITOR_RAW) {
	        hprof_dump_read_id();
		hprof_dump_read_id();
	    } else {
	        fprintf(stderr, "HPROF ERROR: bad monitor kind: %d\n", kind);
	    }
	    thread_env = hprof_dump_read_id();
	    if (thread_env) {
	        hprof_fetch_thread_info(thread_env);
	    }
	    hprof_dump_read_u4();
	    n = hprof_dump_read_u4();
	    for (i = 0; i < n; i++) {
	        thread_env = hprof_dump_read_id();
		hprof_fetch_thread_info(thread_env);
	    }
	    n = hprof_dump_read_u4();
	    for (i = 0; i < n; i++) {
	        thread_env = hprof_dump_read_id();
		hprof_fetch_thread_info(thread_env);
	    }
	}
        hprof_printf("MONITOR DUMP BEGIN\n");
	for (i = 0; i < event->u.monitor_dump.num_traces; i++) {
	    JVMPI_CallTrace *jtrace = &(event->u.monitor_dump.traces[i]);
	    hprof_thread_t *thread = hprof_lookup_thread(jtrace->env_id);
	    int status = event->u.monitor_dump.threads_status[i];
	    hprof_printf("    THREAD %d, trace %d, status: ",
			 thread->serial_num,
			 traces[i]->serial_num);
	    if (status & JVMPI_THREAD_SUSPENDED) {
	        hprof_printf("S|");
		status &= ~JVMPI_THREAD_SUSPENDED;
	    }
	    if (status & JVMPI_THREAD_INTERRUPTED) {
	        hprof_printf("intr|");
		status &= ~JVMPI_THREAD_INTERRUPTED;
	    }
	    switch (status) {
	    case JVMPI_THREAD_RUNNABLE:
	        hprof_printf("R");
		break;
	    case JVMPI_THREAD_MONITOR_WAIT:
	        hprof_printf("MW");
		break;
	    case JVMPI_THREAD_CONDVAR_WAIT:
	        hprof_printf("CW");
		break;
	    }
	    hprof_printf("\n");
	}
        hprof_dump_seek(event->u.monitor_dump.begin);
	while(hprof_dump_cur() < event->u.monitor_dump.end) {
	    int kind = hprof_dump_read_u1();
	    void *owner;
	    int entry_count;
	    int n;
	    if (kind == JVMPI_MONITOR_JAVA) {
	        jobjectID obj = hprof_dump_read_id();
	        hprof_printf("    MONITOR");
		hprof_print_object_info(obj);
	    } else if (kind == JVMPI_MONITOR_RAW) {
	        char *name = hprof_dump_read_id();
		JVMPI_RawMonitor mid = hprof_dump_read_id();
	        hprof_printf("    RAW MONITOR");
		hprof_printf(" \"%s\"(0x%x)", name, mid);
	    } else {
	        fprintf(stderr, "HPROF ERROR: bad monitor kind: %d\n", kind);
	    }
	    owner = hprof_dump_read_id();
	    entry_count = hprof_dump_read_u4();
	    if (owner) {
	        hprof_printf("\n\towner:");
		hprof_print_thread_info(owner, FALSE);
		hprof_printf(", entry count: %d", entry_count);
	    } else {
	        hprof_printf(" unowned");
	    }
	    n = hprof_dump_read_u4();
	    if (n > 0) {
	        hprof_printf("\n\twaiting to enter:");
		for (i = 0; i < n; i++) {
		    void *thr = hprof_dump_read_id();
		    hprof_print_thread_info(thr, i > 0);
		}
	    }
	    n = hprof_dump_read_u4();
	    if (n > 0) {
	        hprof_printf("\n\twaiting to be notified:");
	        for (i = 0; i < n; i++) {
		    void *thr = hprof_dump_read_id();
		    hprof_print_thread_info(thr, i > 0);
		}
	    }
	    hprof_printf("\n");
	}
	hprof_printf("MONITOR DUMP END\n");
    }
    if (traces != NULL) hprof_free(traces);
    CALL(RawMonitorExit)(data_access_lock);
}


#ifdef HASH_STATS
void hprof_print_contended_monitor_hash_stats(FILE *fp) {
    SANITY_CHECK("hprof_print_contended_monitor_hash_stats");
    hprof_print_tbl_hash_stats(fp, &hprof_contended_monitor_table);
}


void hprof_print_raw_monitor_hash_stats(FILE *fp) {
    SANITY_CHECK("hprof_print_raw_monitor_hash_stats");
    hprof_print_tbl_hash_stats(fp, &hprof_raw_monitor_table);
}
#endif /* HASH_STATS */


#ifdef WATCH_ALLOCS
void hprof_free_contended_monitor_table(void)
{
    SANITY_CHECK("hprof_free_contended_monitor_table");
    hprof_hash_removeall(&hprof_contended_monitor_table);
    hprof_hash_free(&hprof_contended_monitor_table);
}


void hprof_free_raw_monitor_table(void)
{
    SANITY_CHECK("hprof_free_raw_monitor_table");
    hprof_hash_removeall(&hprof_raw_monitor_table);
    hprof_hash_free(&hprof_raw_monitor_table);
}
#endif /* WATCH_ALLOCS */
