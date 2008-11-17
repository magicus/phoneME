/*
 * @(#)hprof.c	1.57 06/10/10
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

#include "javavm/include/clib.h"

#include "hprof.h"

static void hprof_notify_event(JVMPI_Event *event);
static void hprof_dump_data(void);

JavaVM *jvm;

int hprof_is_on;            /* whether hprof is enabled */
int hprof_fd = -1;	    /* Non-zero file or socket descriptor. */
FILE *hprof_fp = NULL;            /* FILE handle. */
int hprof_socket_p = FALSE; /* True if hprof_fd is a socket. */

#define HPROF_DEFAULT_TRACE_DEPTH 4
int max_trace_depth = HPROF_DEFAULT_TRACE_DEPTH;
int prof_trace_depth = HPROF_DEFAULT_TRACE_DEPTH;
int thread_in_traces = FALSE;
int lineno_in_traces = TRUE;
char output_format = 'a';              /* 'b' for binary */
float hprof_cutoff_point = (float)0.0001;
int dump_on_exit = TRUE;
#ifdef HASH_STATS
int print_global_hash_stats_on_exit = FALSE;
int print_thread_hash_stats_on_exit = FALSE;
int print_verbose_hash_stats        = FALSE;
#endif /* HASH_STATS */
#ifdef WATCH_ALLOCS
unsigned int jmethodID_array_peak;
unsigned int jmethodID_array_reallocs;
unsigned int jmethodID_array_threads;
unsigned int jmethodID_array_total;
unsigned int method_time_stack_peak;
unsigned int method_time_stack_reallocs;
unsigned int method_time_stack_threads;
int          print_alloc_track_on_exit = FALSE;
#endif /* WATCH_ALLOCS */
int cpu_sampling = FALSE;
int monitor_tracing = FALSE;
int heap_dump = FALSE;
int alloc_sites = FALSE;

/* cpu timing output formats */
int cpu_timing = FALSE;
int timing_format = NEW_PROF_OUTPUT_FORMAT;

jlong total_alloced_bytes;
jlong total_alloced_instances;
long total_live_bytes = 0;
long total_live_instances = 0;

/*
 * PRESERVE THIS LOCK ORDER TO AVOID DEADLOCKS!!!
 *
 * hprof_dump_lock => DisableGC/EnableGC => data_access_lock
 *
 */

/* Data access Lock */
JVMPI_RawMonitor data_access_lock;
/* Dump lock */
JVMPI_RawMonitor hprof_dump_lock;

/* profiler interface */
JVMPI_Interface *hprof_jvm_interface;

unsigned int micro_sec_ticks;

/**
 * Memory allocation and deallocation.
 */
#ifdef WATCH_ALLOCS

/* allocation type tracking */
static struct {
    unsigned int obj_count;	/* #-objects of this allocation type */
    unsigned int byte_count;	/* #-bytes used by this allocation type */
} alloc_peak[ALLOC_TYPE_MAX], alloc_track[ALLOC_TYPE_MAX];
static int n_allocs;		/* #-calls to allocator */
static int n_frees;		/* #-calls to free */


static void *_hprof_calloc(unsigned int size);

void *hprof_calloc(unsigned int size)
{
    return hprof_calloc_tag(ALLOC_TYPE_OTHER, size);
}

void *hprof_calloc_tag(unsigned int tag, unsigned int size)
{
    unsigned int *buf;
    if (tag >= ALLOC_TYPE_MAX) {
        fprintf(stderr,
	    "HPROF ERROR: %u: invalid tag value on alloc, exiting.\n", tag);
	CALL(ProfilerExit)((jint)2);
    }
					/* get extra space for tag and size */
    buf = _hprof_calloc(size + 2 * sizeof(unsigned int));
    /* no return check because _hprof_calloc() exits on out of memory */
    buf[0] = tag;			/* save the extra info */
    buf[1] = size;
    alloc_track[tag].obj_count++;
    if (alloc_track[tag].obj_count > alloc_peak[tag].obj_count) {
	alloc_peak[tag].obj_count = alloc_track[tag].obj_count;
    }
    alloc_track[tag].byte_count += size;
    if (alloc_track[tag].byte_count > alloc_peak[tag].byte_count) {
	alloc_peak[tag].byte_count = alloc_track[tag].byte_count;
    }
    n_allocs++;
    return &buf[2];			/* give the caller their memory */
}

static void hprof_free_hash_tables(void) {
    hprof_free_class_table();
    if (monitor_tracing) hprof_free_contended_monitor_table();
    hprof_free_frame_table();
    hprof_free_method_table();
    hprof_free_objmap_table();
    if (monitor_tracing) hprof_free_raw_monitor_table();
    hprof_free_site_table();
    hprof_free_thread_table();
    hprof_free_trace_table();
    /*
     * Free name table after thread table since thread table hash
     * stats are printed when thread table entries are finished
     * and the thread name is used to identify the table.
     */
    hprof_free_name_table();
}

static char *alloc_type_name(unsigned int tag) {
    switch (tag) {
					        /*          1111111111222222 */
				                /* 1234567890123456789012345 */
    case ALLOC_TYPE_ARRAY:                 return "array";
    case ALLOC_TYPE_BUCKET:                return "bucket_t";
    case ALLOC_TYPE_CALLFRAME:             return "JVMPI_CallFrame";
    case ALLOC_TYPE_CALLTRACE:             return "JVMPI_CallTrace";
    case ALLOC_TYPE_CLASS:                 return "class_t";
    case ALLOC_TYPE_CONTENDED_MONITOR:     return "contended_monitor_t";
    case ALLOC_TYPE_FIELD:                 return "field_t";
    case ALLOC_TYPE_FRAME:                 return "frame_t";
    case ALLOC_TYPE_FRAMES_COST:           return "frames_cost_t";
    case ALLOC_TYPE_GLOBALREF:             return "globalref_t";
    case ALLOC_TYPE_JMETHODID:             return "jmethodID";
    case ALLOC_TYPE_LIVE_THREAD:           return "live_thread_t";
    case ALLOC_TYPE_METHOD:                return "method_t";
    case ALLOC_TYPE_METHOD_TIME:           return "method_time_t";
    case ALLOC_TYPE_NAME:                  return "name_t";
    case ALLOC_TYPE_OBJMAP:                return "objmap_t";
    case ALLOC_TYPE_OTHER:                 return "other";
    case ALLOC_TYPE_RAW_MONITOR:           return "raw_monitor_t";
    case ALLOC_TYPE_SITE:                  return "site_t";
    case ALLOC_TYPE_THREAD:                return "thread_t";
    case ALLOC_TYPE_THREAD_LOCAL:          return "thread_local_t";
    case ALLOC_TYPE_TRACE:                 return "trace-pointers";
    default:
	switch (tag - ALLOC_TYPE_HASH_TABLES) {
        case ALLOC_HASH_CLASS:             return "class_tbl";
        case ALLOC_HASH_CONTENDED_MONITOR: return "contended_monitor_tbl";
        case ALLOC_HASH_FRAME:             return "frame_tbl";
        case ALLOC_HASH_FRAMES_COST:       return "frames_cost_tbl";
        case ALLOC_HASH_METHOD:            return "method_tbl";
        case ALLOC_HASH_NAME:              return "name_tbl";
        case ALLOC_HASH_OBJMAP:            return "objmap_tbl";
        case ALLOC_HASH_RAW_MONITOR:       return "raw_monitor_tbl";
        case ALLOC_HASH_SITE:              return "site_tbl";
        case ALLOC_HASH_THREAD:            return "thread_tbl";
        case ALLOC_HASH_TRACE:             return "trace_tbl";
	default:                           return "<UNKNOWN>";
	}
    }
}

void hprof_print_alloc_track(FILE *fp) {
    unsigned int i;			/* scratch index */
    unsigned int current_bytes = 0;	/* current total # of bytes */
    unsigned int current_objects = 0;	/* current total # of objects */
    unsigned int peak_bytes = 0;	/* peak total # of bytes */
    unsigned int peak_objects = 0;	/* peak total # of objects */

    fprintf(fp, "Allocation Tracking Information\n\n");
    fprintf(fp, "#-alloc-calls=%u  #-free-calls=%u\n\n", n_allocs, n_frees);
    fprintf(fp, "                       current  current    peak     peak\n");
    fprintf(fp, "alloc-type             #-objs   #-bytes   #-objs   #-bytes\n");
    fprintf(fp, "=====================  =======  ========  =======  ========\n");
    for (i = 0; i < ALLOC_TYPE_MAX; i++) {
        if (alloc_peak[i].obj_count == 0 && alloc_peak[i].byte_count == 0) {
	    continue;	/* skip entries with no data */
	}
	fprintf(fp, "%-21s  %7u  %8u  %7u  %8u\n", alloc_type_name(i),
	    alloc_track[i].obj_count, alloc_track[i].byte_count,
	    alloc_peak[i].obj_count, alloc_peak[i].byte_count);
	current_objects += alloc_track[i].obj_count;
	current_bytes   += alloc_track[i].byte_count;
	peak_objects    += alloc_peak[i].obj_count;
	peak_bytes      += alloc_peak[i].byte_count;
    }
    fprintf(fp, "=====================  =======  ========  =======  ========\n");
    fprintf(fp, "%-21s  %7u  %8u  %7u  %8u\n\n", "Totals",
	current_objects, current_bytes, peak_objects, peak_bytes);

    if (jmethodID_array_peak + method_time_stack_peak > 0) {
        fprintf(fp, "Growable Array/Stack Details\n\n");
        fprintf(fp, "                   #-reallocs\n");
        fprintf(fp, "                   /#-threads  peak-elems  total-elems\n");
        fprintf(fp, "=================  ==========  ==========  ===========\n");
    }
    if (jmethodID_array_peak > 0) {
	fprintf(fp, "jmethodID array    %5u/%-4u  %10u  %11u\n",
	    jmethodID_array_reallocs, jmethodID_array_threads,
	    jmethodID_array_peak, jmethodID_array_total);
    }
    if (method_time_stack_peak > 0) {
	fprintf(fp, "method_time stack  %5u/%-4u  %10u\n",
	    method_time_stack_reallocs, method_time_stack_threads,
	    method_time_stack_peak);
    }
}

static void *_hprof_calloc(unsigned int size)
#else /* !WATCH_ALLOCS */
void *hprof_calloc(unsigned int size)
#endif /* WATCH_ALLOCS */
{
    void *p = malloc(size);
    if (p == NULL) {
        fprintf(stderr, "HPROF ERROR: ***Out of Memory***, exiting.\n");
#ifdef WATCH_ALLOCS
        hprof_print_alloc_track(stderr);
#endif /* WATCH_ALLOCS */
	CALL(ProfilerExit)((jint)1);
    }
    memset(p, 0, size);
    return p;
}

#ifdef WATCH_ALLOCS
void hprof_free(void *ptr)
{
    if (ptr == NULL) {
        fprintf(stderr,
	    "HPROF ERROR: NULL pointer passed to hprof_free(), exiting.\n");
	CALL(ProfilerExit)((jint)3);
    } else {
						/* back up to extra info */
	unsigned int *buf = (unsigned int *)ptr - 2;
	unsigned int tag = buf[0];		/* short hand for tag value */
	unsigned int size = buf[1];		/* short hand for size */

	if (tag >= ALLOC_TYPE_MAX) {
	    fprintf(stderr,
		"HPROF ERROR: %u: invalid tag value on free, exiting.\n", tag);
	    CALL(ProfilerExit)((jint)4);
	}

	if (alloc_track[tag].obj_count == 0) {
	    fprintf(stderr,
		"HPROF ERROR: object count zero on free, exiting.\n");
	    fprintf(stderr, "HPROF_ERROR: tag=%u  size=%u\n", tag, size);
	    CALL(ProfilerExit)((jint)5);
	}
	if (alloc_track[tag].byte_count < size) {
	    fprintf(stderr,
		"HPROF ERROR: byte count too small on free, exiting.\n");
	    fprintf(stderr, "HPROF_ERROR: tag=%u  size=%u\n", tag, size);
	    CALL(ProfilerExit)((jint)6);
	}
	alloc_track[tag].obj_count--;
	alloc_track[tag].byte_count -= size;
	n_frees++;
	free(buf);
    }
}
#endif /* WATCH_ALLOCS */

/* hprof initialisation */
JNIEXPORT jint JNICALL JVM_OnLoad(JavaVM *vm, char *options, void *reserved)
{
    int res;
    jvm = vm;
    res = (*jvm)->GetEnv(jvm, (void **)(void*)&hprof_jvm_interface,
                         JVMPI_VERSION_1);
    if (res < 0) {
	return JNI_ERR;
    }
    hprof_jvm_interface->NotifyEvent = hprof_notify_event;
    hprof_dump_lock = CALL(RawMonitorCreate)("_hprof_dump_lock");
    hprof_init_setup(options);

    return JNI_OK;
}

static void hprof_reset_data(void)
{
    if (cpu_sampling || cpu_timing) {
        hprof_clear_trace_cost();
    }
    if (monitor_tracing) {
        hprof_clear_contended_monitor_table();
    }
}

static void hprof_dump_data(void)
{
    fprintf(stderr, "Dumping");
    if (monitor_tracing) {
        fprintf(stderr, " contended monitor usage ...");
        hprof_dump_monitors();
	hprof_output_cmon_times(hprof_cutoff_point);
    }
    if (heap_dump) {
        fprintf(stderr, " Java heap ...");
	hprof_get_heap_dump(); 
    }
    if (alloc_sites) {
        fprintf(stderr, " allocation sites ...");
	hprof_output_sites(0, hprof_cutoff_point);
    }
    if (cpu_sampling) {
        fprintf(stderr, " CPU usage by sampling running threads ...");
        hprof_output_trace_cost(hprof_cutoff_point, CPU_SAMPLES_RECORD_NAME);
    }
    if (cpu_timing) {
        hprof_bill_all_thread_local_tables();
	if (timing_format == NEW_PROF_OUTPUT_FORMAT) {
	    fprintf(stderr, " CPU usage by timing methods ...");
	    hprof_output_trace_cost(hprof_cutoff_point, CPU_TIMES_RECORD_NAME);
	} else if (timing_format == OLD_PROF_OUTPUT_FORMAT) {
	    fprintf(stderr, " CPU usage in old prof format ...");
	    hprof_output_trace_cost_in_prof_format();
	}
    }
    hprof_reset_data();
    hprof_flush();
    fprintf(stderr, " done.\n");
}


static void hprof_jvm_shut_down_event(void)
{
    static int already_dumped = FALSE;
    CALL(RawMonitorEnter)(hprof_dump_lock);
    if (!hprof_is_on || already_dumped) {
        CALL(RawMonitorExit)(hprof_dump_lock);
	return;
    }
    already_dumped = TRUE;
    {
	/* Disable all JVM/PI events since the VM has shutdown */
	jint i;
	for (i = 1; i <= JVMPI_MAX_EVENT_TYPE_VAL; i++) {
	    (void) CALL(DisableEvent)(i, NULL);
	}
    }
    if (dump_on_exit) {
        hprof_dump_data();
    }
#ifdef HASH_STATS
    if (print_global_hash_stats_on_exit) {
	hprof_print_global_hash_stats(stderr);
    }
#endif /* HASH_STATS */
#ifdef WATCH_ALLOCS
    if (print_alloc_track_on_exit) {
	/*
	 * Free hash tables after we have printed the stats, but before
	 * we print the allocation statistics. This will make leaks more
	 * obvious.
	 */
	hprof_free_hash_tables();
	hprof_io_free();
	hprof_print_alloc_track(stderr);
    }
#endif /* WATCH_ALLOCS */
    hprof_is_on = FALSE;
    if (hprof_socket_p) {
	hprof_close(hprof_fd);
    } else {
	fclose(hprof_fp);
    }
    CALL(RawMonitorExit)(hprof_dump_lock);
}  

static void hprof_jvm_init_done_event(void)
{
    hprof_start_listener_thread();
    hprof_start_cpu_sampling_thread();
}

static void hprof_notify_event(JVMPI_Event *event)
{
#ifdef XXX_WATCH_ALLOCS
    {
        /* Use this code to generate a snapshot at a given point */
	static char          DUMP_FILE[] = "alloc_track.snapshot";
	static unsigned int  event_count;

	/* periodically check to see if user wants a snapshot */
	if ((event_count++ % 100) == 0 && access(DUMP_FILE, 0) == 0) {
	    FILE *fp = fopen(DUMP_FILE, "a");	/* append the latest */
	    if (fp != NULL) {			/* silent if open fails */
		hprof_print_alloc_track(fp);
		(void)fclose(fp);
	    }
	}
    }
#endif /* WATCH_ALLOCS */

    switch(event->event_type) {
    case JVMPI_EVENT_JVM_INIT_DONE:
        hprof_jvm_init_done_event();
	return;
    case JVMPI_EVENT_JVM_SHUT_DOWN:
        hprof_jvm_shut_down_event();
	return;
    case JVMPI_EVENT_DUMP_DATA_REQUEST:
        CALL(RawMonitorEnter)(hprof_dump_lock);
        hprof_dump_data();
	CALL(RawMonitorExit)(hprof_dump_lock);
	return;
    case JVMPI_EVENT_RESET_DATA_REQUEST:
        hprof_reset_data();
	return;
    case JVMPI_EVENT_CLASS_LOAD: 
    case JVMPI_EVENT_CLASS_LOAD | JVMPI_REQUESTED_EVENT: 
        hprof_class_load_event(event->env_id, 
			       event->u.class_load.class_name,
			       event->u.class_load.source_name,
			       event->u.class_load.num_interfaces,
			       event->u.class_load.num_static_fields,
			       event->u.class_load.statics,
			       event->u.class_load.num_instance_fields,
			       event->u.class_load.instances,
			       event->u.class_load.num_methods,
			       event->u.class_load.methods,
			       event->u.class_load.class_id,
			       event->event_type & JVMPI_REQUESTED_EVENT);
	return;
    case JVMPI_EVENT_CLASS_UNLOAD:
        hprof_class_unload_event(event->env_id,
				 event->u.class_unload.class_id);
	return;
    case JVMPI_EVENT_OBJECT_DUMP | JVMPI_REQUESTED_EVENT:
        hprof_object_dump_event(event->u.object_dump.data);
	return;
    case JVMPI_EVENT_OBJ_ALLOC:
    case JVMPI_EVENT_OBJ_ALLOC | JVMPI_REQUESTED_EVENT:
        hprof_obj_alloc_event(event->env_id,
			      event->u.obj_alloc.class_id,
			      event->u.obj_alloc.is_array,
			      event->u.obj_alloc.size,
			      event->u.obj_alloc.obj_id,
			      event->u.obj_alloc.arena_id,
			      event->event_type & JVMPI_REQUESTED_EVENT);
	return;
    case JVMPI_EVENT_OBJ_FREE:
        hprof_obj_free_event(event->env_id,
			     event->u.obj_free.obj_id);
	return;
    case JVMPI_EVENT_OBJ_MOVE:
        hprof_obj_move_event(event->env_id,
			     event->u.obj_move.obj_id,
			     event->u.obj_move.arena_id,
			     event->u.obj_move.new_obj_id,
			     event->u.obj_move.new_arena_id);
	return;
    case JVMPI_EVENT_DELETE_ARENA:
        hprof_delete_arena_event(event->env_id,
				 event->u.delete_arena.arena_id);
	return;
    case JVMPI_EVENT_THREAD_START:
    case JVMPI_EVENT_THREAD_START | JVMPI_REQUESTED_EVENT:
        hprof_thread_start_event(event->u.thread_start.thread_env_id,
				 event->u.thread_start.thread_name,
				 event->u.thread_start.group_name,
				 event->u.thread_start.parent_name,
				 event->u.thread_start.thread_id,
				 event->event_type & JVMPI_REQUESTED_EVENT);
	return;
    case JVMPI_EVENT_THREAD_END:
        hprof_thread_end_event(event->env_id);
	return;
    case JVMPI_EVENT_METHOD_ENTRY:
        hprof_method_entry_event(event->env_id,
				 event->u.method.method_id);
	return;
    case JVMPI_EVENT_METHOD_EXIT:
        hprof_method_exit_event(event->env_id,
				event->u.method.method_id);
	return;
    case JVMPI_EVENT_HEAP_DUMP | JVMPI_REQUESTED_EVENT:
        hprof_heap_dump_event(event->u.heap_dump.begin,
			      event->u.heap_dump.end,
			      event->u.heap_dump.num_traces,
			      event->u.heap_dump.traces);
	return;
    case JVMPI_EVENT_JNI_GLOBALREF_ALLOC:
        hprof_jni_globalref_alloc_event(event->env_id,
					event->u.jni_globalref_alloc.obj_id,
					event->u.jni_globalref_alloc.ref_id);
	return;
    case JVMPI_EVENT_JNI_GLOBALREF_FREE:
        hprof_jni_globalref_free_event(event->env_id,
				       event->u.jni_globalref_free.ref_id);
	return;
    case JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTER:
    case JVMPI_EVENT_RAW_MONITOR_CONTENDED_ENTERED:
    case JVMPI_EVENT_RAW_MONITOR_CONTENDED_EXIT:
        hprof_raw_monitor_event(event,
				event->u.raw_monitor.name,
				event->u.raw_monitor.id);
	return;
    case JVMPI_EVENT_MONITOR_CONTENDED_ENTER:
    case JVMPI_EVENT_MONITOR_CONTENDED_ENTERED:
    case JVMPI_EVENT_MONITOR_CONTENDED_EXIT:
    case JVMPI_EVENT_MONITOR_WAITED:
    case JVMPI_EVENT_MONITOR_WAIT:
        hprof_monitor_event(event, event->u.monitor.object);
	return;
    case JVMPI_EVENT_MONITOR_DUMP | JVMPI_REQUESTED_EVENT:
        hprof_monitor_dump_event(event);
	return;
    case JVMPI_EVENT_GC_START:
        hprof_gc_start_event(event->env_id);
	return;
    case JVMPI_EVENT_GC_FINISH:
        hprof_gc_finish_event(event->env_id,
			      event->u.gc_info.used_objects,
			      event->u.gc_info.used_object_space,
			      event->u.gc_info.total_object_space);
	return;
    }

}
