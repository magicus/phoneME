/*
 * jvmti hprof
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

#ifndef HPROF_CHECK_H
#define HPROF_CHECK_H

#define CHECK_FOR_ERROR(condition) \
	( (condition) ? \
	  (void)0 : \
	  HPROF_ERROR(JNI_TRUE, #condition) )
#define CHECK_SERIAL_NO(name, sno) \
	CHECK_FOR_ERROR( (sno) >= gdata->name##_serial_number_start  && \
		      (sno) <  gdata->name##_serial_number_counter)
#define CHECK_CLASS_SERIAL_NO(sno) CHECK_SERIAL_NO(class,sno)
#define CHECK_THREAD_SERIAL_NO(sno) CHECK_SERIAL_NO(thread,sno)
#define CHECK_TRACE_SERIAL_NO(sno) CHECK_SERIAL_NO(trace,sno)
#define CHECK_OBJECT_SERIAL_NO(sno) CHECK_SERIAL_NO(object,sno)

void check_binary_file(char *filename);

#endif
