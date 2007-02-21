/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
 */


#ifndef _JAVA_TASK_H_
#define _JAVA_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This interface is javacall-specific.
 */
 
#include <midp_jc_event_defs.h>
/* #include <javacall_events.h> */
#include <javacall_lifecycle.h>

/**
 * An entry point of a thread devoted to run java.
 */
void JavaTask(void);

/**
 * This function is IMPORTED by the library.
 */
javacall_result
midpHandleStartArbitraryArgEvent(
        midp_jc_event_start_arbitrary_arg startArbitraryArgEvent);
	
#ifdef __cplusplus
}
#endif

#endif /* _JAVA_TASK_H_ */
