/*
 * @(#)jvmpi_vtune.c	1.3 06/10/10
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights  
 * Reserved.  Use is subject to license terms.  
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

/*
 * Copyright 2005 Intel Corporation. All rights reserved.  
 */

/*
 * The main purpose of this file is to add support for JVMPI profiling of the
 * JIT assembler functions that are copied into the code cache. Fake events
 * are posted to map the assembler functions to fake methods of a fake class.
 * This will only work if the profiler understands how to interpret these
 * events.
 *
 * To enable this support, build using the following flags:
 *
 *    CVM_JVMPI=true CVM_JVMPI_PROFILER=vtune
 */

#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/defs.h"
#include "javavm/include/globals.h"
#ifdef CVM_JIT
#include "javavm/include/jit/jitcodebuffer.h"
#endif

#ifndef CVM_JIT

void
CVMJVMPIprofilerInit(CVMExecEnv* ee)
{
    return;
}

#else

static void
CVMvtunePostFakeClassAndMethodEvent(CVMExecEnv *ee);

void
CVMJVMPIprofilerInit(CVMExecEnv *ee)
{
    CVMvtunePostFakeClassAndMethodEvent(ee);
    return;
}

#define CVMjvmpiInterface() (&CVMjvmpiRec()->interfaceX)
#define CVMjvmpiNotifyEvent(/* JVMPI_Event * */event) { \
    CVMjvmpiInterface()->NotifyEvent(event);            \
}

/*
 * IAI-07: Post a series of events to info VTune the
 * address and code size of glue functions in code cache.
 */
static void
CVMvtunePostFakeClassAndMethodEvent(CVMExecEnv *ee)
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    CVMCCMCodeCacheCopyEntry* entries = jgs->ccmCodeCacheCopyEntries;

    JVMPI_Event event;
    char *class_name = "CodeCacheFakeClass";
    JVMPI_Method *methods = NULL;
    int num_methods = 0;
    int i;

#undef failIfNull
#define failIfNull(x)   \
    if ((x) == NULL) {  \
        goto failed;    \
    }

#undef freeIfNotNull
#define freeIfNotNull(x)    \
    if ((x) != NULL) {      \
        free(x);            \
    }

    if (!CVMjvmpiEventCompiledMethodLoadIsEnabled()
        || !CVMjvmpiEventClassLoadIsEnabled()) {
        return;
    }

    while(entries[num_methods].helperName != NULL) {
        num_methods++;
    }

    methods = (JVMPI_Method *) calloc(num_methods, sizeof(JVMPI_Method));
    failIfNull(methods);

    for (i = 0; i < num_methods; i++) {
        char *method_name = malloc(strlen(entries[i].helperName)+1);

        failIfNull(method_name);
        strcpy(method_name, entries[i].helperName);

         /* Fill in the info for this method: */
        methods[i].method_name = method_name;
        methods[i].method_signature = "()V";
        methods[i].method_id = (jmethodID) entries[i].helper;
        methods[i].start_lineno = -1;
        methods[i].end_lineno = -1;
    }

    /* Fill in the fake class load event info: */
    event.event_type = JVMPI_EVENT_CLASS_LOAD;
    event.env_id = CVMexecEnv2JniEnv(ee);
    event.u.class_load.class_name = class_name;
    event.u.class_load.source_name = "";
    event.u.class_load.num_interfaces = 0;
    event.u.class_load.num_methods = num_methods;
    event.u.class_load.methods = methods;
    event.u.class_load.num_static_fields = 0;
    event.u.class_load.statics = NULL;
    event.u.class_load.num_instance_fields = 0;
    event.u.class_load.instances = NULL;
    event.u.class_load.class_id = (jobjectID) jgs->ccmCodeCacheCopyEntries;

    /* Post the event: */
    CVMjvmpiNotifyEvent(&event);

    for (i = 0; i < num_methods; i++) {
        /* Fill in the glue function load event info: */
        event.event_type = JVMPI_EVENT_LOAD_COMPILED_METHOD;
        event.env_id = CVMexecEnv2JniEnv(ee);
        event.u.compiled_method_load.method_id = (jmethodID)entries[i].helper;
        event.u.compiled_method_load.code_addr =  entries[i].helper;
        if (i < num_methods - 1) {
            event.u.compiled_method_load.code_size = entries[i+1].helper - entries[i].helper;
        }
        else {
            event.u.compiled_method_load.code_size = jgs->codeCacheGeneratedCodeStart - entries[i].helper;
        }
        event.u.compiled_method_load.lineno_table_size = 0;
        event.u.compiled_method_load.lineno_table = NULL;
        /* Post the event: */
        CVMjvmpiNotifyEvent(&event);
    }

doCleanUpAndExit:
    /* Release the methods' info: */
    if (methods != NULL) {
        for (i = 0; i < num_methods; i++) {
            freeIfNotNull(methods[i].method_name);
        }
        free(methods);
    }

    return;

failed:
    goto doCleanUpAndExit;

#undef failIfNull
#undef freeIfNotNull
}

#undef CVMjvmpiNotifyEvent
#undef CVMjvmpiInterface

#endif
