/*
 * @(#)hprof_monitor.h	1.13 06/10/10
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

#ifndef _HPROF_MONITOR_H
#define _HPROF_MONITOR_H

#include "hprof_name.h"

void hprof_raw_monitor_table_init(void);
void hprof_contended_monitor_table_init(void);
void hprof_clear_contended_monitor_table(void);
void hprof_raw_monitor_event(JVMPI_Event *event,
                             const char *name,
			     JVMPI_RawMonitor mid);
void hprof_monitor_event(JVMPI_Event *event, jobjectID obj);
void hprof_monitor_wait_event(JVMPI_Event *event, jobjectID obj,
			      jlong timeout);
void hprof_dump_monitors(void);
void hprof_monitor_dump_event(JVMPI_Event *event);
void hprof_output_cmon_times(float cutoff);
#ifdef HASH_STATS
void hprof_print_contended_monitor_hash_stats(FILE *fp);
void hprof_print_raw_monitor_hash_stats(FILE *fp);
#endif /* HASH_STATS */
#ifdef WATCH_ALLOCS
void hprof_free_contended_monitor_table(void);
void hprof_free_raw_monitor_table(void);
#endif /* WATCH_ALLOCS */

typedef struct {
    hprof_name_t *name;
    JVMPI_RawMonitor id;
} hprof_raw_monitor_t;

typedef struct {
    jint type;                        /* Java or Raw? */
    void *mon_info;                   /* (hprof_objmap_t *) or
					 (hprof_raw_monitor_t *) */
    unsigned int trace_serial_num;    /* trace where contention occured */
    jlong time;                       /* time associated with the contention */
    jint num_hits;                    /* # of times this monitor is contended 
					 at this trace */
} hprof_contended_monitor_t;

/* Auxilary data structure used for collecting contended monitor info. */
typedef struct {
    hprof_contended_monitor_t **cmons;               
    int index;
    jlong total_time;
} hprof_cmon_iterate_t;

#endif /* _HPROF_MONITOR_H */
