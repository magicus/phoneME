/*
 * @(#)eventFilter.c	1.8 06/10/10
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
/* 
 * eventFilter
 *
 * This module handles event filteration and the enabling/disabling
 * of the corresponding events. Used for filters on JDI EventRequests
 * and also internal requests.  Our data is in a private hidden section
 * of the HandlerNode's data.  See comment for enclosing
 * module eventHandler.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <jvmdi.h>
#include "util.h"
#include "eventFilter.h"
#include "eventFilterRestricted.h"
#include "eventHandlerRestricted.h"
#include "JDWP.h"
#include "stepControl.h"
#include "threadControl.h"

typedef struct ClassFilter {
    jclass clazz;
} ClassFilter;

typedef struct LocationFilter {
    jclass clazz;
    jmethodID method;
    jlocation location;
} LocationFilter;

typedef struct ThreadFilter {
    jthread thread;
} ThreadFilter;

typedef struct CountFilter {
    jint count;
} CountFilter;

typedef struct ConditionalFilter {
    jint exprID;
} ConditionalFilter;

typedef struct FieldFilter {
    jclass clazz;
    jfieldID field;
} FieldFilter;

typedef struct ExceptionFilter {
    jclass exception;
    jboolean caught;
    jboolean uncaught;
} ExceptionFilter;

typedef struct InstanceFilter {
    jobject instance;
} InstanceFilter;

typedef struct StepFilter {
    jint size;
    jint depth;
    jthread thread;
} StepFilter;

typedef struct MatchFilter {
    char *classPattern;
} MatchFilter;

typedef union RequestFilter {
    struct ClassFilter ClassOnly;
    struct LocationFilter LocationOnly;
    struct ThreadFilter ThreadOnly;
    struct CountFilter Count;
    struct ConditionalFilter Conditional;
    struct FieldFilter FieldOnly;
    struct ExceptionFilter ExceptionOnly;
    struct InstanceFilter InstanceOnly;
    struct StepFilter Step;
    struct MatchFilter ClassMatch;
    struct MatchFilter ClassExclude;
} RequestFilter;

typedef struct Filter_ {
    jbyte modifier;
    union RequestFilter u;
} Filter;

/* The filters array is allocated to the specified filterCount.
 * Theoretically, some compiler could do range checking on this
 * array - so, we define it to have a ludicrously large size so
 * that this range checking won't get upset.
 *
 * The actual allocated number of bytes is computed using the
 * offset of "filters" and so is not effected by this number.
 */
#define MAX_FILTERS 10000

typedef struct EventFilters_ {
    jint filterCount;
    Filter filters[MAX_FILTERS];
} EventFilters;

typedef struct EventFilterPrivate_HandlerNode_ {
    EventHandlerRestricted_HandlerNode   not_for_us;
    EventFilters                         ef;
} EventFilterPrivate_HandlerNode;

/**
 * The following macros extract filter info (EventFilters) from private 
 * data at the end of a HandlerNode 
 */
#define EVENT_FILTERS(node) (&(((EventFilterPrivate_HandlerNode*)node)->ef))
#define FILTER_COUNT(node)  (EVENT_FILTERS(node)->filterCount)
#define FILTERS_ARRAY(node) (EVENT_FILTERS(node)->filters)
#define FILTER(node,index)  ((FILTERS_ARRAY(node))[index])
#define KIND(node)          (node->kind)

/***** filter set-up / destruction *****/

/**
 * Allocate a HandlerNode.
 * We do it because eventHandler doesn't know how big to make it.
 */
HandlerNode *
eventFilterRestricted_alloc(jint filterCount)
{
    size_t size = offsetof(EventFilterPrivate_HandlerNode, ef) + 
                  offsetof(EventFilters, filters) + 
                  (filterCount * sizeof(Filter));
    HandlerNode *node = jdwpClearedAlloc(size);

    if (node != NULL) {
        int i;        
        Filter *filter;

        FILTER_COUNT(node) = filterCount;
        
        /* Initialize all modifiers
         */
        for (i = 0, filter = FILTERS_ARRAY(node); 
                                    i < filterCount; 
                                    i++, filter++) {
            filter->modifier = JDWP_REQUEST_NONE;
        }
    }

    return node;
}

/** 
 * Free up global refs held by the filter. 
 * free things up at the JNI level if needed. 
 */
static jint
clearFilters(HandlerNode *node)
{
    JNIEnv *env = getEnv();
    jint i;
    jint error = JVMDI_ERROR_NONE;
    Filter *filter = FILTERS_ARRAY(node);

    for (i = 0; i < FILTER_COUNT(node); ++i, ++filter) {
        switch (filter->modifier) {
            case JDWP_REQUEST_MODIFIER(ThreadOnly): {
                jthread threadOnly = filter->u.ThreadOnly.thread;
                if (threadOnly != NULL) { 
                    (*env)->DeleteGlobalRef(env, threadOnly);
                }
                break;
            }
            case JDWP_REQUEST_MODIFIER(LocationOnly): {
                jclass clazz = filter->u.LocationOnly.clazz;
                (*env)->DeleteGlobalRef(env, clazz);
                break;
            }
            case JDWP_REQUEST_MODIFIER(FieldOnly): {
                jclass clazz = filter->u.FieldOnly.clazz;
                (*env)->DeleteGlobalRef(env, clazz);
                break;
            }
            case JDWP_REQUEST_MODIFIER(ExceptionOnly): {
                (*env)->DeleteGlobalRef(env, 
                          filter->u.ExceptionOnly.exception);
                break;
            }
            case JDWP_REQUEST_MODIFIER(InstanceOnly): {
                (*env)->DeleteGlobalRef(env, 
                          filter->u.InstanceOnly.instance);
                break;
            }
            case JDWP_REQUEST_MODIFIER(ClassOnly): {
                (*env)->DeleteGlobalRef(env,
                          filter->u.ClassOnly.clazz);
                break;
            }
            case JDWP_REQUEST_MODIFIER(ClassMatch): {
                jdwpFree(filter->u.ClassMatch.classPattern);
                break;
            }
            case JDWP_REQUEST_MODIFIER(ClassExclude): {
                jdwpFree(filter->u.ClassExclude.classPattern);
                break;
            }
            case JDWP_REQUEST_MODIFIER(Step): {
                jthread thread = filter->u.Step.thread;
                error = stepControl_endStep(thread);
                if (error == JVMDI_ERROR_NONE) {
                    (*env)->DeleteGlobalRef(env, thread);
                }
                break;
            }
        }
    }
    if (error == JVMDI_ERROR_NONE) {
        FILTER_COUNT(node) = 0; /* blast so we don't clear again */
    }

    return error;
}


/***** filtering *****/

/*
 * Match a string against a wildcard
 * string pattern.
 */
static jboolean 
patternStringMatch(char *classname, const char *pattern)
{
    int pattLen = strlen(pattern);
    int compLen;
    char *start;
    int offset;

    if ((pattern[0] != '*') && (pattern[pattLen-1] != '*')) {
        /* An exact match is required when there is no *: bug 4331522 */ 
        return strcmp(pattern, classname) == 0; 
    } else {
        compLen = pattLen - 1;
        offset = strlen(classname) - compLen;
        if (offset < 0) {
            return JNI_FALSE;
        } else {
            if (pattern[0] == '*') {
                pattern++;
                start = classname + offset;
            }  else {
                start = classname;
            }
            return strncmp(pattern, start, compLen) == 0;
        }
    }
}

/*
 * Match the name of a class against a wildcard
 * string pattern.
 */
static jboolean 
patternMatch(jclass clazz, const char *pattern)
{
    jboolean rc;
    char *signature = classSignature(clazz);

    if (signature == NULL) {
        return JNI_FALSE;
    }
    convertSignatureToClassname(signature);
    rc = patternStringMatch(signature, pattern);
    jdwpFree(signature);

    return rc;
}

/*
 * Determine if this event is interesting to this handler.
 * Do so by checking each of the handler's filters.
 * Return false if any of the filters fail, 
 * true if the handler wants this event.
 * Anyone modifying this function should check 
 * eventFilterRestricted_passesUnloadFilter and 
 * eventFilter_predictFiltering as well.
 *
 * If shouldDelete is returned true, a count filter has expired
 * and the corresponding node should be deleted.
 */
jboolean 
eventFilterRestricted_passesFilter(JNIEnv *env,
                                   JVMDI_Event *event, 
                                   HandlerNode *node, 
                                   jboolean *shouldDelete)
{
    jthread thread;
    jclass clazz;
    Filter *filter = FILTERS_ARRAY(node);
    int i;

    *shouldDelete = JNI_FALSE;
    eventThreadAndClass(event, &thread, &clazz);

    /*
     * Suppress most events if they happen in debug threads
     */
    if ((event->kind != JVMDI_EVENT_CLASS_PREPARE) &&
        (event->kind != JVMDI_EVENT_CLASS_UNLOAD) && 
        (event->kind != JVMDI_EVENT_CLASS_LOAD) &&
        threadControl_isDebugThread(thread)) {
        return JNI_FALSE;
    }

    for (i = 0; i < FILTER_COUNT(node); ++i, ++filter) {
        switch (filter->modifier) {
            case JDWP_REQUEST_MODIFIER(ThreadOnly):
                if (!(*env)->IsSameObject(env, thread,
                               filter->u.ThreadOnly.thread)) {
                    return JNI_FALSE;
                }
                break;
        
            case JDWP_REQUEST_MODIFIER(ClassOnly):
                /* Class filters catch events in the specified
                 * class and any subclass/subinterface.  
                 */
                if (!(*env)->IsAssignableFrom(env, clazz,
                               filter->u.ClassOnly.clazz)) {
                    return JNI_FALSE;
                }
                break;
        
            /* This is kinda cheating assumming the JVMDI event
             * fields will be in the same locations, but it is
             * true now. 
             */
            case JDWP_REQUEST_MODIFIER(LocationOnly):
                if  (event->u.breakpoint.method != 
                          filter->u.LocationOnly.method ||
                     event->u.breakpoint.location != 
                          filter->u.LocationOnly.location ||
                     !(*env)->IsSameObject(env, clazz,
                                filter->u.LocationOnly.clazz)) {
                    return JNI_FALSE;
                }
                break;

            case JDWP_REQUEST_MODIFIER(FieldOnly): 
                /* Field watchpoints can be triggered from the
                 * declared class or any subclass/subinterface. 
                 */
                if ((event->u.field_access.field != 
		     filter->u.FieldOnly.field) ||
                    !(*env)->IsSameObject(env,
                               event->u.field_access.field_clazz,
			       filter->u.FieldOnly.clazz)) {
                    return JNI_FALSE;
                }
                break;
            
            case JDWP_REQUEST_MODIFIER(ExceptionOnly): 
                /* do we want caught/uncaught exceptions */
                if (!((event->u.exception.catch_clazz == NULL)? 
                      filter->u.ExceptionOnly.uncaught : 
                      filter->u.ExceptionOnly.caught)) {
                    return JNI_FALSE;
                }
    
                /* do we care about exception class */
                if (filter->u.ExceptionOnly.exception != NULL) {
                    jclass exception = event->u.exception.exception;
        
                    /* do we want this exception class */
                    if (!(*env)->IsInstanceOf(env, exception,
                            filter->u.ExceptionOnly.exception)) {
                        return JNI_FALSE;
                    }
                }
                break;
            
            case JDWP_REQUEST_MODIFIER(InstanceOnly): {
                jobject eventInst = eventInstance(event);
                jobject filterInst = filter->u.InstanceOnly.instance;
                /* if no error and doesn't match, don't pass
                 * filter 
                 */
                if (eventInst != NULL &&
                      !(*env)->IsSameObject(env, eventInst,
                                            filterInst)) {
                    return JNI_FALSE;
                }
                break;
            }
            case JDWP_REQUEST_MODIFIER(Count): {
                JDI_ASSERT(filter->u.Count.count > 0);
                if (--filter->u.Count.count > 0) {
                    return JNI_FALSE;
                }
                *shouldDelete = JNI_TRUE;
                break;
            }
                
            case JDWP_REQUEST_MODIFIER(Conditional): 
/***
                if (...  filter->u.Conditional.exprID ...) {
                    return JNI_FALSE;
                }
***/
                break;

        case JDWP_REQUEST_MODIFIER(ClassMatch): {              
            if (!patternMatch(clazz,
                       filter->u.ClassMatch.classPattern)) {
                return JNI_FALSE;
            }
            break;
        }

        case JDWP_REQUEST_MODIFIER(ClassExclude): {              
            if (patternMatch(clazz, 
                      filter->u.ClassExclude.classPattern)) {
                return JNI_FALSE;
            }
            break;
        }

        case JDWP_REQUEST_MODIFIER(Step): 
                if (!(*env)->IsSameObject(env, thread,
                               filter->u.Step.thread)) {
                    return JNI_FALSE;
                }
                if (!stepControl_handleStep(env, event)) {
                    return JNI_FALSE;
                }
                break;
                
        
            default: 
                    ERROR_MESSAGE_EXIT("Invalid filter modifier");
                    return JNI_FALSE;
        }
    }
    return JNI_TRUE;
}

/* Determine if this event is interesting to this handler.  Do so
 * by checking each of the handler's filters.  Return false if any
 * of the filters fail, true if the handler wants this event.
 * Special version of filter for unloads since they don't have an
 * event structure or a jclass.  
 *
 * If shouldDelete is returned true, a count filter has expired
 * and the corresponding node should be deleted.
 */
jboolean 
eventFilterRestricted_passesUnloadFilter(JNIEnv *env,
                                         char *classname, 
                                         HandlerNode *node, 
                                         jboolean *shouldDelete)
{
    Filter *filter = FILTERS_ARRAY(node);
    int i;

    *shouldDelete = JNI_FALSE;
    for (i = 0; i < FILTER_COUNT(node); ++i, ++filter) {
        switch (filter->modifier) {
           
            case JDWP_REQUEST_MODIFIER(Count): {
                JDI_ASSERT(filter->u.Count.count > 0);
                if (--filter->u.Count.count > 0) {
                    return JNI_FALSE;
                }
                *shouldDelete = JNI_TRUE;
                break;
            }
                
            case JDWP_REQUEST_MODIFIER(ClassMatch): {       
                if (!patternStringMatch(classname, 
                        filter->u.ClassMatch.classPattern)) {
                    return JNI_FALSE;
                }
                break;
            }
            
            case JDWP_REQUEST_MODIFIER(ClassExclude): {
                if (patternStringMatch(classname, 
                       filter->u.ClassExclude.classPattern)) {
                    return JNI_FALSE;
                }
                break;
            }
            
            default: 
                ERROR_MESSAGE_EXIT("Invalid filter modifier");
                return JNI_FALSE;
        }
    }
    return JNI_TRUE;
}

/**
 * This function returns true only if it is certain that
 * all events for the given node in the given frame will
 * be filtered. It is used to optimize stepping. (If this
 * function returns true the stepping algorithm does not
 * have to step through every instruction in this frame;
 * instead, it can use more efficient method entry/exit
 * events.
 */
jboolean
eventFilter_predictFiltering(HandlerNode *node, jframeID frame)
{
    Filter *filter = FILTERS_ARRAY(node);
    int i;
    jclass clazz;
    jmethodID method;
    jlocation location;
    jboolean willBeFiltered = JNI_FALSE;
    jboolean done = JNI_FALSE;
    JNIEnv *env = getEnv();
    jint error;

    error = jvmdi->GetFrameLocation(frame, &clazz,
                                    &method, &location);
    if (error != JVMDI_ERROR_NONE) {
        return JNI_FALSE;
    }

    for (i = 0; (i < FILTER_COUNT(node)) && (!done); ++i, ++filter) {
        switch (filter->modifier) {
            case JDWP_REQUEST_MODIFIER(ClassOnly):
                if (!(*env)->IsAssignableFrom(env, clazz,
                                 filter->u.ClassOnly.clazz)) {
                    willBeFiltered = JNI_TRUE;
                    done = JNI_TRUE;
                }
                break;
        
            case JDWP_REQUEST_MODIFIER(Count): {   
                /*
                 * If preceeding filters have determined that events will
                 * be filtered out, that is fine and we won't get here.
                 * However, the count must be decremented - even if
                 * subsequent filters will filter these events.  We
                 * thus must end now unable to predict
                 */
                done = JNI_TRUE;
                break;
            }
                
            case JDWP_REQUEST_MODIFIER(ClassMatch): {              
                if (!patternMatch(clazz,
                        filter->u.ClassMatch.classPattern)) {
                    willBeFiltered = JNI_TRUE;
                    done = JNI_TRUE;
                }
                break;
            }
    
            case JDWP_REQUEST_MODIFIER(ClassExclude): {
                if (patternMatch(clazz,
                       filter->u.ClassExclude.classPattern)) {
                    willBeFiltered = JNI_TRUE;
                    done = JNI_TRUE;
                }
                break;
            }
        }
    }
    (*env)->DeleteGlobalRef(env, clazz);
    return willBeFiltered;
}

/**
 * Determine if the given breakpoint node is in the specified class.
 */
jboolean 
eventFilterRestricted_isBreakpointInClass(JNIEnv *env, jclass clazz, 
                                          HandlerNode *node) 
{
    Filter *filter = FILTERS_ARRAY(node);
    int i;

    for (i = 0; i < FILTER_COUNT(node); ++i, ++filter) {
        switch (filter->modifier) {
            case JDWP_REQUEST_MODIFIER(LocationOnly):
                return (*env)->IsSameObject(env, clazz,
                                            filter->u.LocationOnly.clazz);
        }
    }
    return JNI_TRUE; /* should never come here */
}

/***** filter set-up *****/

jint
eventFilter_setConditionalFilter(HandlerNode *node, jint index, 
                                 jint exprID)
{
    ConditionalFilter *filter = &FILTER(node, index).u.Conditional;
    if (index >= FILTER_COUNT(node)) {
        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    }
    FILTER(node, index).modifier = JDWP_REQUEST_MODIFIER(Conditional);
    filter->exprID = exprID;
    return JVMDI_ERROR_NONE;
}

jint
eventFilter_setCountFilter(HandlerNode *node, jint index, 
                           jint count)
{
    CountFilter *filter = &FILTER(node, index).u.Count;
    if (index >= FILTER_COUNT(node)) {
        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    }
    if (count <= 0) {
        return JDWP_ERROR(INVALID_COUNT);
    } else {
        FILTER(node, index).modifier = JDWP_REQUEST_MODIFIER(Count);
        filter->count = count;
        return JVMDI_ERROR_NONE;
    }
}

jint 
eventFilter_setThreadOnlyFilter(HandlerNode *node, jint index,
                                jthread thread)
{
    JNIEnv *env = getEnv();
    ThreadFilter *filter = &FILTER(node, index).u.ThreadOnly;
    if (index >= FILTER_COUNT(node)) {
        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    }
    if (KIND(node) == JVMDI_EVENT_CLASS_UNLOAD) {
        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    } 
    
    /* Create a thread ref that will live beyond */
    /* the end of this call */
    thread = (*env)->NewGlobalRef(env, thread);
    if (thread == NULL) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }
    FILTER(node, index).modifier = JDWP_REQUEST_MODIFIER(ThreadOnly);
    filter->thread = thread;
    return JVMDI_ERROR_NONE;
}

jint 
eventFilter_setLocationOnlyFilter(HandlerNode *node, jint index, 
                                  jclass clazz, jmethodID method,
                                  jlocation location)
{
    JNIEnv *env = getEnv();
    LocationFilter *filter = &FILTER(node, index).u.LocationOnly;
    if (index >= FILTER_COUNT(node)) {
        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    }
    if ((KIND(node) != JVMDI_EVENT_BREAKPOINT) && 
        (KIND(node) != JVMDI_EVENT_FIELD_ACCESS) && 
        (KIND(node) != JVMDI_EVENT_FIELD_MODIFICATION) && 
        (KIND(node) != JVMDI_EVENT_SINGLE_STEP) && 
        (KIND(node) != JVMDI_EVENT_EXCEPTION)) {

        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    }

    /* Create a class ref that will live beyond */
    /* the end of this call */
    clazz = (*env)->NewGlobalRef(env, clazz);
    if (clazz == NULL) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }
    FILTER(node, index).modifier = JDWP_REQUEST_MODIFIER(LocationOnly);
    filter->clazz = clazz;
    filter->method = method;
    filter->location = location;
    return JVMDI_ERROR_NONE;
}

jint 
eventFilter_setFieldOnlyFilter(HandlerNode *node, jint index, 
                               jclass clazz, jfieldID field)
{
    JNIEnv *env = getEnv();
    FieldFilter *filter = &FILTER(node, index).u.FieldOnly;
    if (index >= FILTER_COUNT(node)) {
        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    }
    if ((KIND(node) != JVMDI_EVENT_FIELD_ACCESS) && 
        (KIND(node) != JVMDI_EVENT_FIELD_MODIFICATION)) {

        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    }

    /* Create a class ref that will live beyond */
    /* the end of this call */
    clazz = (*env)->NewGlobalRef(env, clazz);
    if (clazz == NULL) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }
    FILTER(node, index).modifier = JDWP_REQUEST_MODIFIER(FieldOnly);
    filter->clazz = clazz;
    filter->field = field;
    return JVMDI_ERROR_NONE;
}

jint 
eventFilter_setClassOnlyFilter(HandlerNode *node, jint index, 
                               jclass clazz)
{
    JNIEnv *env = getEnv();
    ClassFilter *filter = &FILTER(node, index).u.ClassOnly;
    if (index >= FILTER_COUNT(node)) {
        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    }
    if ((KIND(node) == JVMDI_EVENT_USER_DEFINED) || 
        (KIND(node) == JVMDI_EVENT_CLASS_UNLOAD) ||
        (KIND(node) == JVMDI_EVENT_THREAD_START) ||
        (KIND(node) == JVMDI_EVENT_THREAD_END)) {

        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    }

    /* Create a class ref that will live beyond */
    /* the end of this call */
    clazz = (*env)->NewGlobalRef(env, clazz);
    if (clazz == NULL) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }
    FILTER(node, index).modifier = JDWP_REQUEST_MODIFIER(ClassOnly);
    filter->clazz = clazz;
    return JVMDI_ERROR_NONE;
}

jint 
eventFilter_setExceptionOnlyFilter(HandlerNode *node, jint index, 
                                   jclass exceptionClass, 
                                   jboolean caught,
                                   jboolean uncaught)
{
    JNIEnv *env = getEnv();
    ExceptionFilter *filter = &FILTER(node, index).u.ExceptionOnly;
    if (index >= FILTER_COUNT(node)) {
        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    }
    if (KIND(node) != JVMDI_EVENT_EXCEPTION) { 
        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    }

    if (exceptionClass != NULL) {
        /* Create a class ref that will live beyond */
        /* the end of this call */
        exceptionClass = (*env)->NewGlobalRef(env, exceptionClass);
        if (exceptionClass == NULL) {
            return JVMDI_ERROR_OUT_OF_MEMORY;
        }
    }
    FILTER(node, index).modifier = 
                       JDWP_REQUEST_MODIFIER(ExceptionOnly);
    filter->exception = exceptionClass;
    filter->caught = caught;
    filter->uncaught = uncaught;
    return JVMDI_ERROR_NONE;
}

jint 
eventFilter_setInstanceOnlyFilter(HandlerNode *node, jint index, 
                                  jobject instance)
{
    JNIEnv *env = getEnv();
    InstanceFilter *filter = &FILTER(node, index).u.InstanceOnly;
    if (index >= FILTER_COUNT(node)) {
        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    }

    if (instance != NULL) {
        /* Create an object ref that will live beyond 
         * the end of this call
         */
        instance = (*env)->NewGlobalRef(env, instance);
        if (instance == NULL) {
            return JVMDI_ERROR_OUT_OF_MEMORY;
        }
    }
    FILTER(node, index).modifier =
                       JDWP_REQUEST_MODIFIER(InstanceOnly);
    filter->instance = instance;
    return JVMDI_ERROR_NONE;
}

jint 
eventFilter_setClassMatchFilter(HandlerNode *node, jint index, 
                                char *classPattern)
{
    MatchFilter *filter = &FILTER(node, index).u.ClassMatch;
    if (index >= FILTER_COUNT(node)) {
        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    }
    if ((KIND(node) == JVMDI_EVENT_USER_DEFINED) || 
        (KIND(node) == JVMDI_EVENT_THREAD_START) ||
        (KIND(node) == JVMDI_EVENT_THREAD_END)) {

        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    }

    FILTER(node, index).modifier = 
                       JDWP_REQUEST_MODIFIER(ClassMatch);
    filter->classPattern = classPattern;
    return JVMDI_ERROR_NONE;
}

jint 
eventFilter_setClassExcludeFilter(HandlerNode *node, jint index, 
                                  char *classPattern)
{
    MatchFilter *filter = &FILTER(node, index).u.ClassExclude;
    if (index >= FILTER_COUNT(node)) {
        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    }
    if ((KIND(node) == JVMDI_EVENT_USER_DEFINED) || 
        (KIND(node) == JVMDI_EVENT_THREAD_START) ||
        (KIND(node) == JVMDI_EVENT_THREAD_END)) {

        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    }

    FILTER(node, index).modifier = 
                       JDWP_REQUEST_MODIFIER(ClassExclude);
    filter->classPattern = classPattern;
    return JVMDI_ERROR_NONE;
}

jint 
eventFilter_setStepFilter(HandlerNode *node, jint index, 
                          jthread thread, jint size, jint depth)
{
    jint error;
    JNIEnv *env = getEnv();
    StepFilter *filter = &FILTER(node, index).u.Step;
    if (index >= FILTER_COUNT(node)) {
        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    }
    if (KIND(node) != JVMDI_EVENT_SINGLE_STEP) { 
        return JVMDI_ERROR_ILLEGAL_ARGUMENT;
    }

    /* Create a thread ref that will live beyond */
    /* the end of this call */
    thread = (*env)->NewGlobalRef(env, thread);
    if (thread == NULL) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }
    error = stepControl_beginStep(thread, size,depth, node);
    if (error != JVMDI_ERROR_NONE) {
        (*env)->DeleteGlobalRef(env, thread);
        return error;
    }
    FILTER(node, index).modifier = JDWP_REQUEST_MODIFIER(Step);
    filter->thread = thread;
    filter->depth = depth;
    filter->size = size;
    return JVMDI_ERROR_NONE;
}

/***** JVMDI event enabling / disabling *****/

/**
 * Return the Filter that is of the specified type (modifier).
 * Return NULL if not found.
 */
static Filter *
findFilter(HandlerNode *node, jint modifier)
{
    int i;
    Filter *filter;
    for (i = 0, filter = FILTERS_ARRAY(node); 
                      i <FILTER_COUNT(node); 
                      i++, filter++) {
        if (filter->modifier == modifier) {
            return filter;
        }
    }
    return NULL;
}

/**
 * Determine if the specified breakpoint node is in the
 * same location as the LocationFilter passed in arg.
 *
 * This is a match function called by a
 * eventHandlerRestricted_iterator invokation.
 */
static jboolean
matchBreakpoint(JNIEnv *env, HandlerNode *node, void *arg)
{
    LocationFilter *goal = (LocationFilter *)arg;
    Filter *filter = FILTERS_ARRAY(node);
    int i;
        
    for (i = 0; i < FILTER_COUNT(node); ++i, ++filter) {
        switch (filter->modifier) {
        case JDWP_REQUEST_MODIFIER(LocationOnly): {
            LocationFilter *trial = &(filter->u.LocationOnly);
            if (trial->method == goal->method &&
                trial->location == goal->location &&
                (*env)->IsSameObject(env, 
                                     trial->clazz, goal->clazz)) {
                return JNI_TRUE;
            }
        }
        }
    }
    return JNI_FALSE;
}

/**
 * Set a breakpoint if this is the first one at this location.
 */
static jint
setBreakpoint(HandlerNode *node)
{
    jint error = JVMDI_ERROR_NONE;
    Filter *filter;

    filter = findFilter(node, JDWP_REQUEST_MODIFIER(LocationOnly));
    if (filter == NULL) {
        /* bp event with no location filter */
        error = JVMDI_ERROR_INTERNAL; 
    } else {
        LocationFilter *lf = &(filter->u.LocationOnly);

        /* if this is the first handler for this 
         * location, set bp at jvmdi level 
         */
        if (!eventHandlerRestricted_iterator(
                JVMDI_EVENT_BREAKPOINT, matchBreakpoint, lf)) {
            error = jvmdi->SetBreakpoint(lf->clazz, lf->method, 
                                         lf->location);
        }
    }
    return error;
}

/**
 * Clear a breakpoint if this is the last one at this location.
 */
static jint
clearBreakpoint(HandlerNode *node)
{
    jint error = JVMDI_ERROR_NONE;
    Filter *filter;

    filter = findFilter(node, JDWP_REQUEST_MODIFIER(LocationOnly));
    if (filter == NULL) {
        /* bp event with no location filter */
        error = JVMDI_ERROR_INTERNAL; 
    } else {
        LocationFilter *lf = &(filter->u.LocationOnly);

        /* if this is the last handler for this 
         * location, clear bp at jvmdi level 
         */
        if (!eventHandlerRestricted_iterator(
                JVMDI_EVENT_BREAKPOINT, matchBreakpoint, lf)) {
            error = jvmdi->ClearBreakpoint(lf->clazz, lf->method, 
                                           lf->location);
        }
    }
    return error;
}

/**
 * Determine if the specified watchpoint node has the
 * same field as the FieldFilter passed in arg.
 *
 * This is a match function called by a
 * eventHandlerRestricted_iterator invokation.
 */
static jboolean
matchWatchpoint(JNIEnv *env, HandlerNode *node, void *arg)
{
    FieldFilter *goal = (FieldFilter *)arg;
    Filter *filter = FILTERS_ARRAY(node);
    int i;
        
    for (i = 0; i < FILTER_COUNT(node); ++i, ++filter) {
        switch (filter->modifier) {
        case JDWP_REQUEST_MODIFIER(FieldOnly): {
            FieldFilter *trial = &(filter->u.FieldOnly);
            if (trial->field == goal->field &&
                (*env)->IsSameObject(env, 
                                     trial->clazz, goal->clazz)) {
                return JNI_TRUE;
            }
        }
        }
    }
    return JNI_FALSE;
}

/**
 * Set a watchpoint if this is the first one on this field.
 */
static jint
setWatchpoint(HandlerNode *node)
{
    jint error = JVMDI_ERROR_NONE;
    Filter *filter;

    filter = findFilter(node, JDWP_REQUEST_MODIFIER(FieldOnly));
    if (filter == NULL) {
        /* event with no field filter */
        error = JVMDI_ERROR_INTERNAL; 
    } else {
        FieldFilter *ff = &(filter->u.FieldOnly);

        /* if this is the first handler for this 
         * field, set wp at jvmdi level 
         */
        if (!eventHandlerRestricted_iterator(
                KIND(node), matchWatchpoint, ff)) {
            error = (KIND(node) == JVMDI_EVENT_FIELD_ACCESS) ?
                jvmdi->SetFieldAccessWatch(ff->clazz, 
                                           ff->field) :
                jvmdi->SetFieldModificationWatch(ff->clazz,
                                                 ff->field);
        }
    }
    return error;
}

/**
 * Clear a watchpoint if this is the last one on this field.
 */
static jint
clearWatchpoint(HandlerNode *node)
{
    jint error = JVMDI_ERROR_NONE;
    Filter *filter;

    filter = findFilter(node, JDWP_REQUEST_MODIFIER(FieldOnly));
    if (filter == NULL) {
        /* event with no field filter */
        error = JVMDI_ERROR_INTERNAL; 
    } else {
        FieldFilter *ff = &(filter->u.FieldOnly);

        /* if this is the last handler for this 
         * field, clear wp at jvmdi level 
         */
        if (!eventHandlerRestricted_iterator(
                KIND(node), matchWatchpoint, ff)) {
            error = (KIND(node) == JVMDI_EVENT_FIELD_ACCESS) ?
                jvmdi->ClearFieldAccessWatch(ff->clazz, 
                                             ff->field) :
                jvmdi->ClearFieldModificationWatch(ff->clazz,
                                                   ff->field);
        }
    }
    return error;
}

/**
 * Determine the thread this node is filtered on.
 * NULL if not thread filtered.
 */
static jthread 
requestThread(HandlerNode *node) 
{
    int i;
    Filter *filter = FILTERS_ARRAY(node);

    for (i = 0; i < FILTER_COUNT(node); ++i, ++filter) {
        switch (filter->modifier) {
            case JDWP_REQUEST_MODIFIER(ThreadOnly):
                return filter->u.ThreadOnly.thread;
        }
    }

    return NULL;
}

/**
 * Determine if the specified node has a
 * thread filter with the thread passed in arg.
 *
 * This is a match function called by a
 * eventHandlerRestricted_iterator invokation.
 */
static jboolean
matchThread(JNIEnv *env, HandlerNode *node, void *arg)
{
    jthread goalThread = (jthread)arg;
    jthread reqThread = requestThread(node);

    /* If the event's thread and the passed thread are the same
     * (or both are NULL), we have a match.
     */
    return (*env)->IsSameObject(env, reqThread, goalThread);
}

/**
 * Do any enabling of events (including setting breakpoints etc)
 * needed to get the events requested by this handler node.
 */
static jint
enableEvents(HandlerNode *node)
{
    jint error = JVMDI_ERROR_NONE;

    switch (KIND(node)) {
        /* The stepping code directly enables/disables stepping as
         * necessary 
         */
        case JVMDI_EVENT_SINGLE_STEP:
        /* Internal thread event handlers are always present
         * (hardwired in the event hook), so we don't change the
         * notification mode here.  
         */
        case JVMDI_EVENT_THREAD_START:
        case JVMDI_EVENT_THREAD_END:
            return error;

        case JVMDI_EVENT_FIELD_ACCESS:
        case JVMDI_EVENT_FIELD_MODIFICATION:
            error = setWatchpoint(node);
            break;

        case JVMDI_EVENT_BREAKPOINT:
            error = setBreakpoint(node);
            break;
    }

    /* Don't globally enable if the above failed */
    if (error == JVMDI_ERROR_NONE) {
        jthread thread = requestThread(node);

        /* If this is the first request of it's kind on this
         * thread (or all threads (thread == NULL)) then enable
         * these events on this thread.
         */
        if (!eventHandlerRestricted_iterator(
                KIND(node), matchThread, thread)) {
            error = threadControl_setEventMode(JVMDI_ENABLE, 
                                               KIND(node), thread);
        }
    }
    return error;
}

/**
 * Do any disabling of events (including clearing breakpoints etc)
 * needed to no longer get the events requested by this handler node.
 */
static jint
disableEvents(HandlerNode *node)
{
    jint error = JVMDI_ERROR_NONE;
    jint error2 = JVMDI_ERROR_NONE;
    jthread thread;


    switch (KIND(node)) {
        /* The stepping code directly enables/disables stepping as
         * necessary 
         */
        case JVMDI_EVENT_SINGLE_STEP:
        /* Internal thread event handlers are always present
         * (hardwired in the event hook), so we don't change the
         * notification mode here.  
         */
        case JVMDI_EVENT_THREAD_START:
        case JVMDI_EVENT_THREAD_END:
            return error;

        case JVMDI_EVENT_FIELD_ACCESS:
        case JVMDI_EVENT_FIELD_MODIFICATION:
            error = clearWatchpoint(node);
            break;

        case JVMDI_EVENT_BREAKPOINT:
            error = clearBreakpoint(node);
            break;
    }

    thread = requestThread(node);

    /* If this is the last request of it's kind on this thread
     * (or all threads (thread == NULL)) then disable these
     * events on this thread.
     *
     * Disable even if the above caused an error
     */
    if (!eventHandlerRestricted_iterator(KIND(node), matchThread, thread)) {
        error2 = threadControl_setEventMode(JVMDI_DISABLE, 
                                            KIND(node), thread);
    }
    return error != JVMDI_ERROR_NONE? error : error2;
}


/***** filter (and event) installation and deinstallation *****/

/**
 * Make the set of event filters that correspond with this
 * node active (including enabling the corresponding events).
 */
jint
eventFilterRestricted_install(HandlerNode *node)
{
    return enableEvents(node);
}

/**
 * Make the set of event filters that correspond with this
 * node inactive (including disabling the corresponding events
 * and freeing resources).
 */
jint
eventFilterRestricted_deinstall(HandlerNode *node)
{
    jint error1, error2;
    
    error1 = disableEvents(node);
    error2 = clearFilters(node);

    return error1 != JVMDI_ERROR_NONE? error1 : error2;
}

