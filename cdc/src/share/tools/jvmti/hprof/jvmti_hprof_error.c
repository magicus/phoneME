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

#include "jvmti_hprof.h"

/* The error handling logic. */

/* 
 * Most hprof error processing and error functions are kept here, along with
 *   termination functions and signal handling (used in debug version only).
 *
 */


static int p = 1; /* Used with pause=y|n option */

/* Private functions */

static void
error_message(const char * format, ...)
{
    va_list ap;

    va_start(ap, format);
    (void)vfprintf(stderr, format, ap);
    va_end(ap);
}

static void
error_abort(void)
{
    /* Important to remove existing signal handler */
    (void)signal(SIGABRT, NULL);
    error_message("HPROF DUMPING CORE\n");
    abort();        /* Sends SIGABRT signal, usually also caught by libjvm */
}

static void
signal_handler(int sig)
{
    /* Caught a signal, most likely a SIGABRT */
    error_message("HPROF SIGNAL %d TERMINATED PROCESS\n", sig);
    error_abort();
}

static void
setup_signal_handler(int sig)
{
    /* Only if debug version or debug=y */
    if ( gdata->debug ) {
        (void)signal(sig, (void *)(void(*)(int))(void*)&signal_handler);
    }
}

static void
terminate_everything(jint exit_code)
{
    if ( exit_code > 0 ) {
        /* Could be a fatal error or assert error or a sanity error */
        error_message("HPROF TERMINATED PROCESS\n");
        if ( gdata->coredump || gdata->debug ) {
            /* Core dump here by request */
            error_abort();
        }
    }
    /* Terminate the process */
    error_exit_process(exit_code);
}

/* External functions */

void
error_setup(void)
{
    setup_signal_handler(SIGABRT); 
}

void
error_do_pause(void)
{
    int pid = md_getpid();
    int timeleft = 600; /* 10 minutes max */
    int interval = 10;  /* 10 second message check */

    /*LINTED*/
    error_message("\nHPROF pause for PID %d\n", (int)pid);
    while ( p && timeleft > 0 ) {
        md_sleep(interval); /* 'assign p=0' to stop pause loop */
        timeleft -= interval;
    }
    if ( timeleft <= 0 ) {
        error_message("\n HPROF pause got tired of waiting and gave up.\n");
    }
}

void
error_exit_process(int exit_code)
{
    exit(exit_code);
}

static const char *
source_basename(const char *file)
{
    const char *p;

    if ( file == NULL ) {
	return "UnknownSourceFile";
    }
    p = strrchr(file, '/');
    if ( p == NULL ) {
	p = strrchr(file, '\\');
    }
    if ( p == NULL ) {
	p = file;
    } else {
	p++; /* go past / */
    }
    return p;
}

void
error_assert(const char *condition, const char *file, int line)
{
    error_message("ASSERTION FAILURE: %s [%s:%d]\n", condition, 
        source_basename(file), line);
    error_abort();
}

void
error_handler(jboolean fatal, jvmtiError error, 
                const char *message, const char *file, int line)
{
    char *error_name;
    
    if ( message==NULL ) {
        message = "";
    }
    if ( error != JVMTI_ERROR_NONE ) {
        error_name = getErrorName(error);
        if ( error_name == NULL ) {
            error_name = "?";
        }
        error_message("HPROF ERROR: %s (JVMTI Error %s(%d)) [%s:%d]\n", 
                            message, error_name, error, 
                            source_basename(file), line);
    } else {
        error_message("HPROF ERROR: %s [%s:%d]\n", message, 
                            source_basename(file), line);
    }
    if ( fatal || gdata->errorexit ) {
        /* If it's fatal, or the user wants termination on any error, die */
        terminate_everything(9);
    }
}

void
debug_message(const char * format, ...)
{
    va_list ap;

    va_start(ap, format);
    (void)vfprintf(stderr, format, ap);
    va_end(ap);
}

void
verbose_message(const char * format, ...)
{
    if ( gdata->verbose ) {
        va_list ap;

        va_start(ap, format);
        (void)vfprintf(stderr, format, ap);
        va_end(ap);
    }
}

