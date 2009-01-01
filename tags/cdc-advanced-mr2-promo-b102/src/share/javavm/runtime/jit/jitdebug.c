/*
 * @(#)jitdebug.c	1.16 06/10/10
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

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/ccee.h"
#include "javavm/include/globals.h"
#include "javavm/include/stacks.h"
#include "javavm/include/utils.h"

#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitirnode.h"
#include "javavm/include/jit/jitcodebuffer.h"

#ifdef CVM_JIT_DEBUG

/* ========================================================================*/

/* Purpose: Struct for identifying methods by name: */
typedef struct CVMJITMethodSpec CVMJITMethodSpec;
struct CVMJITMethodSpec
{
    const char *fullMethodName;
    CVMClassTypeID clazzID;
    CVMMethodTypeID methodID;
};

/* Purpose: Struct for a list of method specs: */
typedef struct CVMJITMethodList CVMJITMethodList;
struct CVMJITMethodList
{
    CVMBool hasBeenInitialized;
    CVMJITMethodList *next;
    CVMJITMethodSpec *entries;
    CVMUint32 size;
};

#define CVMJIT_DEBUG_METHOD_LIST_BEGIN(_listName)                       \
    static CVMJITMethodSpec _listName##Elements[] = {

#define CVMJIT_DEBUG_METHOD_LIST_END(_listName)                         \
    };                                                                  \
    CVMJITMethodList _listName = {                                      \
        CVM_FALSE,                                                      \
        NULL,                                                           \
        _listName##Elements,                                           \
        sizeof(_listName##Elements)/sizeof(_listName##Elements[0]),     \
    };

/* ======================================================================== */

/* Begin of controls for JIT compile method filter list feature: ========== */

/* Define USE_COMPILATION_LIST_FILTER to enable the use of the compilation
   list filter.  This filter will only allow methods in the list to be
   compiled when compilation is requested for it.  Otherwise, the filter will
   report the method as a bad method.  This option is meant on for use when
   debugging a port of the JIT.  It has no useful application in a release
   build nor normal debug build. */
#undef USE_COMPILATION_LIST_FILTER

/* NOTE: See comments at the top of the file regarding the use of
         USE_COMPILATION_LIST_FILTER. */
#ifdef USE_COMPILATION_LIST_FILTER

CVMJIT_DEBUG_METHOD_LIST_BEGIN(methodsToCompile)
    /* An example of a full method signature for a method to allow for
       insertion of breakpoints for debugging:
       { "java.lang.String.<init>(Ljava/lang/StringBuffer;)V" },
    */
    { "java.lang.System.getSecurityManager()Ljava/lang/SecurityManager;", 0, 0 },
    { "java.util.Properties.setProperty(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;", 0, 0 },
CVMJIT_DEBUG_METHOD_LIST_END(methodsToCompile)

#endif /* USE_COMPILATION_LIST_FILTER */

/* End of controls for JIT compile method filter list feature: ============ */

/* ======================================================================== */

/* Begin of controls for JIT tracing filter list feature: ========== */

/* Define USE_TRACING_LIST_FILTER to enable the use of the tracing list
   filter.  This filter will only allow methods in the list to be traced when
   compilation is requested for it.  Otherwise, the method will not be traced.
   This option is meant on for use when debugging a port of the JIT.  It has
   no useful application in a release build nor normal debug build. 

   Also set FILTERED_TRACE_OPTIONS to the trace options that you want when
   the method is one of the ones in your list.  The FILTERED_TRACE_OPTIONS
   will be added onto the options that are already being traced.  When the
   method is done compiling, the original trace options will be restored.
*/
#undef USE_TRACING_LIST_FILTER
#define FILTERED_TRACE_OPTIONS  (0x00000007)

/* If tracing is not enabled in general, then we must force this option off: */
#ifndef CVM_TRACE_JIT
#undef USE_TRACING_LIST_FILTER
#endif

/* NOTE: See comments at the top of the file regarding the use of
         USE_TRACING_LIST_FILTER. */
#ifdef USE_TRACING_LIST_FILTER

CVMJIT_DEBUG_METHOD_LIST_BEGIN(methodsToTraceCompilation)
    /* An example of a full method signature for a method to allow for
       insertion of breakpoints for debugging:
       { "java.lang.String.<init>(Ljava/lang/StringBuffer;)V" },
    */
    { "java.lang.String.<init>(Ljava/lang/StringBuffer;)V", 0 ,0 },
CVMJIT_DEBUG_METHOD_LIST_END(methodsToTraceCompilation)

#endif /* USE_TRACING_LIST_FILTER */

/* End of controls for JIT tracing  filter list feature: ============ */

/* ======================================================================== */

/* Purpose: Initializes the specified method list. */
static void
CVMJITdebugInitMethodList(CVMExecEnv *ee, CVMJITMethodList *list)
{
    CVMUint32 i;
    CVMJITMethodSpec *entries;

    CVMassert(list != NULL);
    entries = list->entries;
    for (i = 0; i < list->size; i++) {
        const char *fullMethodName;
        char *clazzname;
        char *methodname;
        char *signame;
        CVMClassTypeID clazzID;
        CVMMethodTypeID methodID;
        char *p;
        size_t length;

        fullMethodName = entries[i].fullMethodName;
        length = strlen(fullMethodName) + 1;

        /* The clazzname is at the start of the full methodname: */
        clazzname = malloc(length);
        strcpy(clazzname, fullMethodName);

        /* Replace all '.' with '/' because the typeid system uses
           '/'s: */
        p = clazzname;
        p = strchr(p, '.');
        while (p != NULL) {
            *p++ = '/';
            p = strchr(p, '.');
        }

        /* Find the signame which starts with a '(': */
        p = strchr(clazzname, '(');
        if (p == NULL) {
            methodID = CVM_TYPEID_ERROR;
        } else {
            signame = malloc(strlen(p) + 1);
            strcpy(signame, p);
            *p = 0; /* NULL terminator after methodname. */

            /* Find the methodname which begins after the last '/': */
            p = strrchr(clazzname, '/');
            *p++ = 0; /* NULL terminator between clazz & method. */
            methodname = p;
            methodID =
                CVMtypeidNewMethodIDFromNameAndSig(ee, methodname, signame);
	    if (methodID == CVM_TYPEID_ERROR) {
		CVMconsolePrintf("WARNING: CVMJITdebugInitMethodList failed "
				 "to lookup methodID for %s\n",
				 entries[i].fullMethodName);
		free(clazzname);
		clazzname = NULL;
	    }
            free(signame);
        }

	clazzID = CVM_TYPEID_ERROR;
	if (clazzname != NULL) {
	    clazzID = CVMtypeidNewClassID(ee, clazzname, strlen(clazzname));
	    if (clazzID == CVM_TYPEID_ERROR) {
		CVMconsolePrintf("WARNING: CVMJITdebugInitMethodList failed "
				 "to lookup classID for %s\n",
				 entries[i].fullMethodName);
	    }
	    free(clazzname);
	}
	entries[i].clazzID = clazzID;
	entries[i].methodID = methodID;
#if 0
	if (clazzID != CVM_TYPEID_ERROR) {
	    CVMconsolePrintf("Added(%d): %s\n", i, entries[i].fullMethodName);
	    CVMconsolePrintf("\tclass:  %!C\n", clazzID);
	    if (methodID != CVM_TYPEID_ERROR) {
		CVMconsolePrintf("\tmethod: %!M\n", methodID);
	    }
	}
#endif
    }
    list->hasBeenInitialized = CVM_TRUE;
}

/* Purpose: Checks to see if the specified method is in the specified list. */
CVMBool
CVMJITdebugMethodIsInMethodList(CVMExecEnv *ee, CVMMethodBlock *mb,
                                CVMJITMethodList *list)
{
    CVMClassBlock *cb = CVMmbClassBlock(mb);
    CVMClassTypeID clazzID = CVMcbClassName(cb);
    CVMMethodTypeID methodID = CVMmbNameAndTypeID(mb);

    while (list != NULL) {
        CVMUint32 i;
        CVMJITMethodSpec *entries;

        /* Initialize the method list if it isn't already initialized: */
        if (!list->hasBeenInitialized) {
            CVMJITdebugInitMethodList(ee, list);
        }

        entries = list->entries;
        /* Search for the method in the filter's method list: */
        for (i = 0; i < list->size; i++) {
            if ((clazzID == entries[i].clazzID) &&
                (CVM_TYPEID_ERROR == entries[i].methodID ||
                 methodID == entries[i].methodID)) {
                return CVM_TRUE;
            }
        }
        list = list->next;
    }
    return CVM_FALSE;
}

/* ========================================================================*/

/* Purpose: Initialized the debugging state in the compilation context. */
void
CVMJITdebugInitCompilationContext(CVMJITCompilationContext *con,
                                  CVMExecEnv *ee)
{
}

/* ========================================================================*/

#ifdef CVM_TRACE_JIT
static CVMUint32 CVMJITdebugOldDebugFlags;
static CVMBool CVMJITdebugTraceOn = CVM_FALSE;
#endif

/* Purpose: Setup debug options before compilation begins. */
void
CVMJITdebugInitCompilation(CVMExecEnv *ee, CVMMethodBlock *mb)
{
#if defined(USE_TRACING_LIST_FILTER) && defined(CVM_TRACE_JIT)
    if (CVMJITdebugMethodIsInMethodList(ee, mb, &methodsToTraceCompilation)) {
        CVMJITdebugOldDebugFlags = CVMglobals.debugJITFlags;
        CVMglobals.debugJITFlags |= FILTERED_TRACE_OPTIONS;
        CVMJITdebugTraceOn = CVM_TRUE;
    }
#endif
}

/* Purpose: Clean up debug options after compilation ends. */
void
CVMJITdebugEndCompilation(CVMExecEnv *ee, CVMMethodBlock *mb)
{
#ifdef CVM_TRACE_JIT
    if (CVMJITdebugTraceOn) {
        CVMglobals.debugJITFlags = CVMJITdebugOldDebugFlags;
        CVMJITdebugTraceOn = CVM_FALSE;
    }
#endif
}

/* Purpose: Enables compilation tracing based on some externally detected
            factor.  When some external trigger is encountered during
            compilation, this function can be called to enable tracing for
            that method which is being compiled.  This is useful for
            debugging of compilation of methods with certain unique
            characteristics as opposed to identifying the method by name
            as is the case with the trace filter list. */
void
CVMJITdebugStartTrace(void)
{
#ifdef CVM_TRACE_JIT
    if (!CVMJITdebugTraceOn) {
        CVMJITdebugOldDebugFlags = CVMglobals.debugJITFlags;
        CVMglobals.debugJITFlags |= FILTERED_TRACE_OPTIONS;
        CVMJITdebugTraceOn = CVM_TRUE;
    }
#endif
}

/* Purpose: Checks if the specified method is to be compiled or not. */
CVMBool
CVMJITdebugMethodIsToBeCompiled(CVMExecEnv *ee, CVMMethodBlock *mb)
{
#ifdef USE_COMPILATION_LIST_FILTER
    if (CVMJITdebugMethodIsInMethodList(ee, mb, &methodsToCompile)) {
        return CVM_TRUE;
    }
    return CVM_FALSE;
#else
    return CVM_TRUE;
#endif
}

#endif /* CVM_JIT_DEBUG */

