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

#ifndef HPROF_ERROR_H
#define HPROF_ERROR_H

/* Macros over assert and error functions so we can capture the source loc. */

#define HPROF_BOOL(x) ((jboolean)((x)==0?JNI_FALSE:JNI_TRUE))

#define HPROF_ERROR(fatal,msg) \
    error_handler(HPROF_BOOL(fatal), JVMTI_ERROR_NONE, msg, __FILE__, __LINE__)

#define HPROF_JVMTI_ERROR(error,msg) \
    error_handler(HPROF_BOOL(error!=JVMTI_ERROR_NONE), \
	    error, msg, __FILE__, __LINE__)

#if defined(DEBUG) || !defined(NDEBUG)
    #define HPROF_ASSERT(cond) \
        (((int)(cond))?(void)0:error_assert(#cond, __FILE__, __LINE__))
#else
    #define HPROF_ASSERT(cond)
#endif

#define LOG_DUMP_MISC           0x1 /* Misc. logging info */
#define LOG_DUMP_LISTS          0x2 /* Dump tables at vm init and death */
#define LOG_CHECK_BINARY        0x4 /* If format=b, verify binary format */

#ifdef HPROF_LOGGING
    #define LOG_STDERR(args) \
        { \
            if ( gdata != NULL && (gdata->logflags & LOG_DUMP_MISC) ) { \
                (void)fprintf args ; \
            } \
        }
#else
    #define LOG_STDERR(args)
#endif

#define LOG_FORMAT(format)      "HPROF LOG: " format " [%s:%d]\n"

#define LOG1(str1)              LOG_STDERR((stderr, LOG_FORMAT("%s"), \
                                    str1, __FILE__, __LINE__ ))
#define LOG2(str1,str2)         LOG_STDERR((stderr, LOG_FORMAT("%s %s"), \
                                    str1, str2, __FILE__, __LINE__ ))
#define LOG3(str1,str2,num)     LOG_STDERR((stderr, LOG_FORMAT("%s %s 0x%x"), \
                                    str1, str2, num, __FILE__, __LINE__ ))

#define LOG(str) LOG1(str)

void       error_handler(jboolean fatal, jvmtiError error, 
		const char *message, const char *file, int line);
void       error_assert(const char *condition, const char *file, int line);
void       error_exit_process(int exit_code);
void       error_do_pause(void);
void       error_setup(void);
void       debug_message(const char * format, ...);
void       verbose_message(const char * format, ...);

#endif
