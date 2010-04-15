/*
 * @(#)jcov_events.h	1.13 06/10/10
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

#ifndef _JCOV_EVENTS_H
#define _JCOV_EVENTS_H

#include "jvmpi.h"

void jcov_events_init(void);
void jcov_class_load_hook_event(JVMPI_Event *event);
void jcov_class_load_event(JVMPI_Event *event);
void jcov_req_class_load_event(JVMPI_Event *event);
void jcov_class_unload_event(JVMPI_Event *event);
void jcov_object_move_event(JVMPI_Event *event);
void jcov_thread_start_event(JVMPI_Event *event);
void jcov_thread_end_event(JVMPI_Event *event);
void jcov_method_entry_event(JVMPI_Event *event);
void jcov_instr_exec_event(JVMPI_Event *event);
void jcov_gc_start_event(void);
void jcov_gc_finish_event(void);
void jcov_jvm_shut_down_event(void);

extern JVMPI_RawMonitor jcov_instr_count_lock;
#define LOCK_INSTR() \
    (jvmpi_interface->RawMonitorEnter)(jcov_instr_count_lock)
#define UNLOCK_INSTR() \
    (jvmpi_interface->RawMonitorExit)(jcov_instr_count_lock)

#endif /* _JCOV_EVENTS_H */
