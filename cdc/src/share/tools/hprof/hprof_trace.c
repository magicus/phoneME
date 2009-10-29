/*
 * @(#)hprof_trace.c	1.18 06/10/10
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

static unsigned int trace_serial_number = 1;
static hprof_hash_t hprof_trace_table;

static unsigned int hash_trace(void * _trace)
{
    hprof_trace_t *trace = _trace;
    unsigned int hash = trace->thread_serial_num + 37 * trace->n_frames;
    int i;
    for (i = 0; i < trace->n_frames; i++) {
        hash = hash * 37 + ((unsigned int)trace->frames[i] >> HASH_OBJ_SHIFT);
    }
    return hash % hprof_trace_table.size;
}

static unsigned int size_trace(void * _trace)
{
    hprof_trace_t *trace = _trace;
    return ((char *)&(trace->frames) - (char *)trace) + 
        trace->n_frames * sizeof(hprof_frame_t *);
}

static int compare_trace(void *_trace1, void *_trace2)
{
    hprof_trace_t *trace1 = _trace1;
    hprof_trace_t *trace2 = _trace2;
    int result = trace1->thread_serial_num - trace2->thread_serial_num;
    int i;
    if (result) {
        return result;
    }
    result = trace1->n_frames - trace2->n_frames;
    if (result) {
        return result;
    }
    for (i = 0; i < trace1->n_frames; i++) {
        result = trace1->frames[i] - trace2->frames[i];
	if (result) {
	    return result;
	}
    }
    return 0;
}

void hprof_trace_table_init(void)
{
    hprof_hash_init(ALLOC_HASH_TRACE, &hprof_trace_table, 256,
		    hash_trace, size_trace, compare_trace);
}

static hprof_hash_t hprof_frame_table;

static unsigned int hash_frame(void *_frame)
{
    hprof_frame_t *frame = _frame;
    return (((unsigned int)frame->method >> HASH_OBJ_SHIFT) +
	31 * (unsigned int)frame->lineno) % hprof_frame_table.size;
}

static unsigned int size_frame(void *_frame)
{
    return sizeof(hprof_frame_t);
}

static int compare_frame(void *_frame1, void *_frame2)
{
    hprof_frame_t *frame1 = _frame1;
    hprof_frame_t *frame2 = _frame2;

    int result = frame1->lineno - frame2->lineno;
    if (result) {
        return result;
    }
    return frame1->method - frame2->method;
}

void hprof_frame_table_init(void)
{
    hprof_hash_init(ALLOC_HASH_FRAME, &hprof_frame_table, 256,
		    hash_frame, size_frame, compare_frame);
}

hprof_frame_t *
hprof_intern_jvmpi_frame(jmethodID method_id, int lineno)
{
    hprof_frame_t frame_tmp;

    frame_tmp.marked = 0;
    if (lineno_in_traces) {
        frame_tmp.lineno = lineno;
    } else {
        frame_tmp.lineno = -1;
    }
    
    frame_tmp.method = hprof_lookup_method(method_id);
    if (frame_tmp.method == NULL) {
        fprintf(stderr, "HPROF ERROR: unable to resolve a method id\n");
	return NULL;
    }

    return hprof_hash_intern(&hprof_frame_table, &frame_tmp);
}

hprof_trace_t *hprof_alloc_tmp_trace(int n_frames, JNIEnv *env_id)
{
    hprof_trace_t *trace_tmp = 0;
    int thread_serial_num = 0;
    trace_tmp = HPROF_CALLOC(ALLOC_TYPE_ARRAY,
	n_frames * sizeof(hprof_frame_t *) +
	((char *)&(trace_tmp->frames) - (char *)trace_tmp));
    trace_tmp->n_frames = n_frames;
    trace_tmp->serial_num = 0;
    trace_tmp->marked = 0;
    trace_tmp->num_hits = 0;
    trace_tmp->cost = jlong_zero;
    
    /* include thread info only if thread_in_traces is true and env_id is not
     * NULL.  even if thread_in_traces is true, other hprof functions may want to 
     * intern traces without the thread info, e.g. hprof_bill_frames_cost for
     * the old prof output format. */
    if ((!thread_in_traces) || (env_id == NULL)) {
        thread_serial_num = 0;
    } else {
        hprof_thread_t *thread = hprof_intern_thread(env_id);
	thread_serial_num = thread->serial_num;
    }
    trace_tmp->thread_serial_num = thread_serial_num;
    
    return trace_tmp;
}

hprof_trace_t *hprof_intern_tmp_trace(hprof_trace_t *trace_tmp)
{
    hprof_trace_t *result = hprof_hash_lookup(&hprof_trace_table, trace_tmp);
    if (result == NULL) {
        trace_tmp->serial_num = trace_serial_number++;
	result = hprof_hash_put(&hprof_trace_table, trace_tmp);
    }
    hprof_free(trace_tmp);
    
    return result;
}
    
hprof_trace_t *
hprof_intern_jvmpi_trace(JVMPI_CallFrame *frames, int n_frames, JNIEnv *env_id)
{
    int i;
    hprof_trace_t *trace_tmp = hprof_alloc_tmp_trace(n_frames, env_id);
    
    /* intern all the frames in the trace */
    for (i = 0; i < n_frames; i++) {
        hprof_frame_t *frame = hprof_intern_jvmpi_frame(frames[i].method_id , 
							frames[i].lineno);
	if (frame == NULL) {
	    hprof_free(trace_tmp);
	    return NULL;
	}
	trace_tmp->frames[i] = frame;
    }
    return hprof_intern_tmp_trace(trace_tmp);
}

hprof_trace_t *hprof_get_trace(JNIEnv *env_id, int depth)
{
    hprof_trace_t *htrace;
    JVMPI_CallTrace jtrace;

    /* allocate space for the trace */
    jtrace.frames = 
        HPROF_CALLOC(ALLOC_TYPE_CALLFRAME, depth*sizeof(JVMPI_CallFrame));

    /* Get the trace from the JVM */
    jtrace.env_id = env_id;
    CALL(GetCallTrace)(&jtrace, depth);
        
    /* intern the trace */
    htrace = hprof_intern_jvmpi_trace(jtrace.frames, jtrace.num_frames, jtrace.env_id);
    
    /* Free up the space */
    hprof_free(jtrace.frames);

    return htrace;
}
    
    
    
/* Write out a stack trace.
 * Works differently depending on the output_format
 */
static void hprof_output_trace(hprof_trace_t *trace)
{
    int i;
    if (trace->marked) {
        return;
    }
    trace->marked = 1;
    
    if (output_format == 'b') {
        for (i = 0; i < trace->n_frames; i++) {
	    if (trace->frames[i]->marked == 0) {
	        hprof_frame_t *frame = trace->frames[i];
	        hprof_method_t *method = frame->method;
		hprof_name_t *method_name = method->method_name;
		hprof_name_t *method_signature = method->method_signature;
		hprof_class_t *class = method->class;
		hprof_name_t *src_name = class->src_name;
	        
		frame->marked = 1;
		hprof_write_header(HPROF_FRAME, sizeof(void *) * 4 + 8);
		hprof_write_id(frame);
		hprof_write_id(method_name);
		hprof_write_id(method_signature);
		hprof_write_id(src_name);
		hprof_write_u4(class->serial_num);
		hprof_write_u4(frame->lineno);
	    }	    
        }
	hprof_write_header(HPROF_TRACE, sizeof(void *) * trace->n_frames + 12);
	hprof_write_u4(trace->serial_num);
	hprof_write_u4(trace->thread_serial_num);
	hprof_write_u4(trace->n_frames);
        for (i = 0; i < trace->n_frames; i++) {
	    hprof_write_id(trace->frames[i]);
	}
    } else {
        hprof_printf("TRACE %u:", trace->serial_num);
	if (trace->thread_serial_num) {
	    hprof_printf(" (thread=%d)", trace->thread_serial_num);
	}
	hprof_printf("\n");
	if (trace->n_frames == 0) {
	    hprof_printf("\t<empty>\n");
	}
	for (i = 0; i < trace->n_frames; i++) {
	    hprof_frame_t *frame = trace->frames[i];
	    hprof_method_t *method = frame->method;
	    hprof_class_t *class = method->class;
            const char *srcname = class->src_name->name;
            const char *classname = class->name->name;
            const char *methodname = method->method_name->name;
	    char lineno_buf[256];
	    int lineno = frame->lineno;

	    if (lineno == -2) {
	        strcpy(lineno_buf, "Compiled method");
	    } else if (lineno == -3) {
	        strcpy(lineno_buf, "Native method");
            } else if (lineno == -1) {
                strcpy(lineno_buf, "Unknown line");
	    } else {
	        sprintf(lineno_buf, "%d", lineno);
	    }
	    hprof_printf("\t%s.%s(%s:%s)\n", 
			 classname, methodname, srcname, lineno_buf);
	}
    }
}

static void *hprof_output_unmarked_trace(void *_trace, void *arg)
{
    hprof_trace_t *trace = _trace;
    hprof_output_trace(trace);
    return arg;
}

void hprof_output_unmarked_traces(void)
{
    hprof_hash_iterate(&hprof_trace_table, 
		       hprof_output_unmarked_trace, 
		       0);
}


static void * hprof_trace_collect(void *_trace, void *_arg)
{
    hprof_trace_iterate_t *arg = _arg;
    hprof_trace_t *trace = _trace;
    arg->traces[arg->index++] = trace;
    arg->total_count += jlong_to_jint(trace->cost);
    return _arg;
}

static int hprof_trace_compare_cost(const void *p_trace1, const void *p_trace2)
{
    hprof_trace_t *trace1 = *(hprof_trace_t **)p_trace1;
    hprof_trace_t *trace2 = *(hprof_trace_t **)p_trace2;
    return jlong_to_jint(jlong_sub(trace2->cost, trace1->cost));
}

static int hprof_trace_compare_num_hits(const void *p_trace1, 
					const void *p_trace2)
{
    hprof_trace_t *trace1 = *(hprof_trace_t **)p_trace1;
    hprof_trace_t *trace2 = *(hprof_trace_t **)p_trace2;
    return trace2->num_hits - trace1->num_hits;
}

/* output info on the cost associated with traces  */
void
hprof_output_trace_cost(float cutoff, char *record_name)
{
    hprof_trace_iterate_t iterate;
    int i, trace_table_size, n_items;
    float accum, percent;

    CALL(RawMonitorEnter)(data_access_lock);

    /* First write all trace we might refer to. */
    hprof_output_unmarked_traces();

    iterate.traces = HPROF_CALLOC(ALLOC_TYPE_ARRAY,
	hprof_trace_table.n_entries * sizeof(void *));
    iterate.total_count = iterate.index = 0;
    hprof_hash_iterate(&hprof_trace_table, hprof_trace_collect, &iterate);

    trace_table_size = iterate.index;

    /* sort all the traces according to the cost */
    qsort(iterate.traces, trace_table_size, 
	  sizeof(void *), hprof_trace_compare_cost);
    n_items = 0;
    for (i = 0; i < trace_table_size; i++) {
        hprof_trace_t *trace = iterate.traces[i];
	percent = CVMlong2Float(trace->cost) / (float)iterate.total_count;
	if (percent < cutoff) {
	    break;
	}
	n_items++;
    }
    /* output the info */
    if (output_format == 'a') {
        time_t t = time(0);	
	hprof_printf("%s BEGIN (total = %u) %s", record_name, 
		     iterate.total_count, ctime(&t));
	hprof_printf("rank   self  accum   count trace method\n");
	accum = 0;
	for (i = 0; i < n_items; i++) {
	    hprof_trace_t *trace = iterate.traces[i];

	    percent = CVMlong2Float(trace->cost) /
		(float)iterate.total_count * 100.0;
	    accum += percent;
	    hprof_printf("%4u %5.2f%% %5.2f%% %7u %5u",
			 i + 1, percent, accum, trace->num_hits,
			 trace->serial_num);
	    if (trace->n_frames > 0) {
	        hprof_frame_t *frame = trace->frames[0];
		hprof_method_t *method = frame->method;
		hprof_printf(" %s.%s\n", method->class->name->name, 
			     method->method_name->name);
	    } else {
	        hprof_printf(" <empty trace>\n");
	    }
	}
	hprof_printf("%s END\n", record_name);
    } else {
        hprof_write_header(HPROF_CPU_SAMPLES, n_items * 8 + 4 + 4);
	hprof_write_u4(iterate.total_count);
	hprof_write_u4(n_items);
	for (i = 0; i < n_items; i++) {
	    hprof_trace_t *trace = iterate.traces[i];
	    hprof_write_u4(jlong_to_jint(trace->cost));
	    hprof_write_u4(trace->serial_num);	    
	}
    }
    hprof_free(iterate.traces);

    CALL(RawMonitorExit)(data_access_lock);

}

/* output the trace cost in old prof format */
void hprof_output_trace_cost_in_prof_format(void) 
{
    hprof_trace_iterate_t iterate;
    int i, trace_table_size;

    CALL(RawMonitorEnter)(data_access_lock);

    iterate.traces = HPROF_CALLOC(ALLOC_TYPE_ARRAY,
	hprof_trace_table.n_entries * sizeof(void *));
    iterate.index = 0;
    hprof_hash_iterate(&hprof_trace_table, hprof_trace_collect, &iterate);
    trace_table_size = iterate.index;
    /* sort all the traces according to the number of hits */
    qsort(iterate.traces, trace_table_size, 
	  sizeof(void *), hprof_trace_compare_num_hits);

    hprof_printf("count callee caller time\n");
    for (i = 0; i < trace_table_size; i++) {
        hprof_trace_t *trace = iterate.traces[i];
	int num_frames = trace->n_frames;
	int num_hits = trace->num_hits;

	if (num_hits == 0) {
	    goto done;
	}
	
	hprof_printf("%d ", trace->num_hits);
	if (num_frames >= 1) {
	    hprof_method_t *callee = trace->frames[0]->method; 
	    hprof_printf("%s.%s%s ", 
			 callee->class->name->name, 
			 callee->method_name->name, 
			 callee->method_signature->name);
	} else {
	    hprof_printf("%s ", "<unknown callee>");
	}
	if (num_frames > 1) {
	    hprof_method_t *caller = trace->frames[1]->method;
	    hprof_printf("%s.%s%s ", 
			 caller->class->name->name, 
			 caller->method_name->name, 
			 caller->method_signature->name);
	} else {
	    hprof_printf("%s ", "<unknown caller>");
	}
	hprof_printf("%d\n", jlong_to_jint(trace->cost));
    }

 done:
    CALL(RawMonitorExit)(data_access_lock);
}
       

static void * hprof_clear_trace_cost_helper(void *_trace, void *_arg)
{
    hprof_trace_t *trace = _trace;
    trace->cost = jlong_zero;
    return _arg;
}

void hprof_clear_trace_cost(void)
{
    hprof_hash_iterate(&hprof_trace_table, hprof_clear_trace_cost_helper, NULL);
}


#ifdef HASH_STATS
void hprof_print_frame_hash_stats(FILE *fp) {
    hprof_print_tbl_hash_stats(fp, &hprof_frame_table);
}


void hprof_print_trace_hash_stats(FILE *fp) {
    hprof_print_tbl_hash_stats(fp, &hprof_trace_table);
}
#endif /* HASH_STATS */


#ifdef WATCH_ALLOCS
void hprof_free_frame_table(void)
{
    hprof_hash_removeall(&hprof_frame_table);
    hprof_hash_free(&hprof_frame_table);
}


void hprof_free_trace_table(void)
{
    hprof_hash_removeall(&hprof_trace_table);
    hprof_hash_free(&hprof_trace_table);
}
#endif /* WATCH_ALLOCS */
