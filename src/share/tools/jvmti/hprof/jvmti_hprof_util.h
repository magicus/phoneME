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

#ifndef HPROF_UTIL_H
#define HPROF_UTIL_H

/* Macros that protect code from accidently using a local ref improperly */
#define WITH_LOCAL_REFS(env, number)            \
    {                                           \
        JNIEnv *_env = (env);                   \
        pushLocalFrame(_env, (number < 20 ? 20 : number));	\
        { /* BEGINNING OF WITH SCOPE */

#define END_WITH_LOCAL_REFS                     \
        } /* END OF WITH SCOPE */               \
        popLocalFrame(_env, NULL);              \
    }

/* Macro to check for exceptions after JNI calls. */
#define CHECK_EXCEPTIONS(env)                                           \
    {                                                                   \
        JNIEnv *_env = (env);                                           \
        jobject _exception;                                             \
        _exception = exceptionOccurred(_env);                           \
        if ( _exception != NULL ) {                                     \
            exceptionDescribe(_env);                                    \
            HPROF_ERROR(JNI_TRUE, "Unexpected Exception found beforehand");\
        }                                                               \
        {

#define END_CHECK_EXCEPTIONS                                            \
        }                                                               \
        _exception = exceptionOccurred(_env);                           \
        if ( _exception != NULL ) {                                     \
            exceptionDescribe(_env);                                    \
            HPROF_ERROR(JNI_TRUE, "Unexpected Exception found afterward");\
        }                                                               \
    }

JNIEnv *   getEnv(void);

/* JNI support functions */
jobject    newGlobalReference(JNIEnv *env, jobject object);
jobject    newWeakGlobalReference(JNIEnv *env, jobject object);
void       deleteGlobalReference(JNIEnv *env, jobject object);
jobject           newLocalReference(JNIEnv *env, jobject object);
void           deleteLocalReference(JNIEnv *env, jobject object);
void       deleteWeakGlobalReference(JNIEnv *env, jobject object);
jclass     getObjectClass(JNIEnv *env, jobject object);
jmethodID  getMethodID(JNIEnv *env, jclass clazz, const char* name, 
                        const char *sig);
jclass     getSuperclass(JNIEnv *env, jclass klass);
jmethodID  getStaticMethodID(JNIEnv *env, jclass clazz, const char* name, 
                        const char *sig);
jfieldID   getStaticFieldID(JNIEnv *env, jclass clazz, const char* name, 
                        const char *sig);
jclass     findClass(JNIEnv *env, const char *name);
void       setStaticIntField(JNIEnv *env, jclass clazz, jfieldID field, 
                        jint value);
jboolean   isSameObject(JNIEnv *env, jobject o1, jobject o2);
void       pushLocalFrame(JNIEnv *env, jint capacity);
void       popLocalFrame(JNIEnv *env, jobject ret);
jobject    exceptionOccurred(JNIEnv *env);
void       exceptionDescribe(JNIEnv *env);
void       exceptionClear(JNIEnv *env);
void       registerNatives(JNIEnv *env, jclass clazz, 
                        JNINativeMethod *methods, jint count);

/* More JVMTI support functions */
char *    getErrorName(jvmtiError error_number);
jvmtiPhase getPhase(void);
char *    phaseString(jvmtiPhase phase);
void      disposeEnvironment(void);
jlong     getObjectSize(jobject object);
jobject   getClassLoader(jclass klass);
jint      getClassStatus(jclass klass);
jlong     getTag(jobject object);
void      setTag(jobject object, jlong tag);
void      getObjectMonitorUsage(jobject object, jvmtiMonitorUsage *uinfo);
void      getOwnedMonitorInfo(jthread thread, jobject **ppobjects, 
                        jint *pcount);
void      JVMTIgetSystemProperty(const char *name, char **value);
void      getClassSignature(jclass klass, char**psignature, 
                        char **pgeneric_signature);
void      getSourceFileName(jclass klass, char** src_name_ptr);

jvmtiPrimitiveType sigToPrimType(char *sig);
int       sigToPrimSize(char *sig);
char      primTypeToSigChar(jvmtiPrimitiveType primType);

void      getAllClassFieldInfo(JNIEnv *env, jclass klass, 
                        jint* field_count_ptr, FieldInfo** fields_ptr);
void      getMethodName(jmethodID method, char** name_ptr, 
                        char** signature_ptr);
void      getMethodClass(jmethodID method, jclass *pclazz);
jboolean  isMethodNative(jmethodID method);
void      getPotentialCapabilities(jvmtiCapabilities *capabilities);
void      addCapabilities(jvmtiCapabilities *capabilities);
void      setEventCallbacks(jvmtiEventCallbacks *pcallbacks);
void      setEventNotificationMode(jvmtiEventMode mode, jvmtiEvent event, 
                        jthread thread);
void *    getThreadLocalStorage(jthread thread);
void      setThreadLocalStorage(jthread thread, void *ptr);
void      getThreadState(jthread thread, jint *threadState);
void      getThreadInfo(jthread thread, jvmtiThreadInfo *info);
void      getThreadGroupInfo(jthreadGroup thread_group, jvmtiThreadGroupInfo *info);
void      getLoadedClasses(jclass **ppclasses, jint *pcount);
jint      getLineNumber(jmethodID method, jlocation location);
jlong     getMaxMemory(JNIEnv *env);
void      createAgentThread(JNIEnv *env, const char *name, 
                        jvmtiStartFunction func);
jlong     getThreadCpuTime(jthread thread);
void      getStackTrace(jthread thread, jvmtiFrameInfo *pframes, jint depth, 
                        jint *pcount);
void      getThreadListStackTraces(jint count, jthread *threads, 
                        jint depth, jvmtiStackInfo **stack_info);
void      getFrameCount(jthread thread, jint *pcount);
void      followReferences(jvmtiHeapCallbacks *pHeapCallbacks, void *user_data);

/* GC control */
void      runGC(void);

/* Get initial JVMTI environment */
void      getJvmti(void);

/* Get current runtime JVMTI version */
jint      jvmtiVersion(void);

/* Raw monitor functions */
jrawMonitorID createRawMonitor(const char *str);
void          rawMonitorEnter(jrawMonitorID m);
void          rawMonitorWait(jrawMonitorID m, jlong pause_time);
void          rawMonitorNotifyAll(jrawMonitorID m);
void          rawMonitorExit(jrawMonitorID m);
void          destroyRawMonitor(jrawMonitorID m);

/* JVMTI alloc/dealloc */
void *        jvmtiAllocate(int size);
void          jvmtiDeallocate(void *ptr);

/* System malloc/free */
void *        hprof_malloc(int size);
void          hprof_free(void *ptr);

#include "debug_malloc.h"

#ifdef DEBUG
    void *        hprof_debug_malloc(int size, char *file, int line);
    void          hprof_debug_free(void *ptr, char *file, int line);
    #define HPROF_MALLOC(size)  hprof_debug_malloc(size, __FILE__, __LINE__)
    #define HPROF_FREE(ptr)     hprof_debug_free(ptr, __FILE__, __LINE__)
#else
    #define HPROF_MALLOC(size)  hprof_malloc(size)
    #define HPROF_FREE(ptr)     hprof_free(ptr)
#endif

#endif
