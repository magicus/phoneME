/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */
#include <jni.h>
#include <jvmdi.h>
#include "util_md.h"
#ifndef JPDA_NO_DLALLOC
#include "dlAlloc.h"
#endif

#define MIN(a, b) ((a) < (b)) ? (a) : (b)
     
/*
 * Globals used throughout the back end
 */
extern JavaVM *jvm;
extern JVMDI_Interface_1 *jvmdi;
extern jboolean vmDead;

/*
 * JNI signature constants, beyond those defined in JDWP_Tag_*
 */
#define SIGNATURE_BEGIN_ARGS    '('
#define SIGNATURE_END_ARGS      ')'
#define SIGNATURE_END_CLASS     ';'

/*
 * Modifier flags for classes, fields, methods
 */
#define MOD_PUBLIC       0x0001     /* visible to everyone */
#define MOD_PRIVATE      0x0002     /* visible only to the defining class */
#define MOD_PROTECTED    0x0004     /* visible to subclasses */
#define MOD_STATIC       0x0008     /* instance variable is static */
#define MOD_FINAL        0x0010     /* no further subclassing, overriding */
#define MOD_SYNCHRONIZED 0x0020     /* wrap method call in monitor lock */
#define MOD_VOLATILE     0x0040     /* can cache in registers */
#define MOD_TRANSIENT    0x0080     /* not persistant */
#define MOD_NATIVE       0x0100     /* implemented in C */
#define MOD_INTERFACE    0x0200     /* class is an interface */
#define MOD_ABSTRACT     0x0400     /* no definition provided */
/*
 * Additional modifiers not defined as such in the JVM spec
 */
#define MOD_SYNTHETIC    0xf0000000  /* not in source code */

/*
 * jlong conversion macros
 */
#define jlong_zero	 ((jlong) 0)
#define jlong_one	 ((jlong) 1)

#define jlong_to_ptr(a)  ((void*)(a))
#define ptr_to_jlong(a)  ((jlong)(a))
#define jint_to_jlong(a) ((jlong)(a))
#define jlong_to_jint(a) ((jint)(a))

/*
 * util funcs
 */
void util_initialize();   
void util_reset();

struct PacketInputStream;
struct PacketOutputStream;

jint uniqueID(void); 
jbyte referenceTypeTag(jclass clazz);
jbyte specificTypeKey(jobject object);
jboolean isObjectTag(jbyte tag);
jint spawnNewThread(void (*func)(void *), void *arg, char *name);
void eventThreadAndClass(JVMDI_Event *event, jthread *thread, jclass *clazz);
jthread eventThread(JVMDI_Event *event);
jclass eventClass(JVMDI_Event *event);
jobject eventInstance(JVMDI_Event *event);
void convertSignatureToClassname(char *convert);
void writeCodeLocation(struct PacketOutputStream *out, jclass clazz, 
                       jmethodID method, jlocation location);

/*
 * Command handling helpers shared among multiple command sets
 */
jint filterDebugThreads(jthread *threads, int count);


void sharedGetFieldValues(struct PacketInputStream *in, 
                          struct PacketOutputStream *out, 
                          jboolean isStatic);
jboolean sharedInvoke(struct PacketInputStream *in,
                      struct PacketOutputStream *out);

jvmdiError fieldSignature(jclass clazz, jfieldID field, char **sigPtr);
/*jvmdiError variableSignature(jframeID frame, jint slot, char **sigPtr);*/

/*
 * Thin wrappers on top of JNI
 */
JNIEnv *getEnv(void);
jboolean isClass(jobject object);
jboolean isThread(jobject object);
jboolean isThreadGroup(jobject object);
jboolean isString(jobject object);
jboolean isClassLoader(jobject object);
jboolean isArray(jobject object);
void exitWithError(char *, char *, int, char *, jint);
void exitWithJVMDIError(jvmdiError error);
char *getPropertyCString(char *propertyName);

/*
 * This calls into java; do not use after startup.
 */
jthread currentThread(void);

/*
 * Thin wrappers on top of JVMDI
 */
jint jvmdiVersion(void);
jint jvmdiMajorVersion(void);
jint jvmdiMinorVersion(void);
jboolean canGetSourceDebugExtension(void);
int getSourceDebugExtension(jclass clazz, char **extensionPtr);

JVMDI_RawMonitor debugMonitorCreate(char *name);
void debugMonitorEnter(JVMDI_RawMonitor theLock);
void debugMonitorExit(JVMDI_RawMonitor theLock);
void debugMonitorWait(JVMDI_RawMonitor theLock);
void debugMonitorTimedWait(JVMDI_RawMonitor theLock, jlong millis);
void debugMonitorNotify(JVMDI_RawMonitor theLock);
void debugMonitorNotifyAll(JVMDI_RawMonitor theLock);
void debugMonitorDestroy(JVMDI_RawMonitor theLock);

jthread *allThreads(jint *count);
void threadInfo(jthread thread, JVMDI_thread_info *info);

jthreadGroup *topThreadGroups(jint *count);
void threadGroupInfo(jthreadGroup, JVMDI_thread_group_info *info);

char *classSignature(jclass);
jint classStatus(jclass);
jint classModifiers(jclass);
jobject classLoader(jclass);
jmethodID *declaredMethods(jclass, jint *count);
jfieldID *declaredFields(jclass, jint *count);
jclass *implementedInterfaces(jclass, jint *count);
jboolean isArrayClass(jclass);
jboolean isInterface(jclass);
jboolean isMethodNative(jclass, jmethodID);

jint objectHashCode(jobject);

jclass *allLoadedClasses(jint *count);
jclass *loadedClasses(jobject classLoader, jint *count);

jint frameCount(jthread thread,  jint *count);

void freeGlobalRefs(jobject *refs, jint count);
void freeGlobalRefsPartial(jobject *refs, jint startIndex, jint count);

void util_setAllocLock(JVMDI_RawMonitor lock);

#ifdef JDWP_ALLOC_TRACE
void *jdwpAllocReal(jint numBytes);
void jdwpFreeReal(void *buffer);
void *jdwpAllocTrace(char *fn, int ln, jint numBytes);
void jdwpFreeTrace(char *fn, int ln, void *buffer);
#define jdwpAlloc(nb) jdwpAllocTrace(__FILE__, __LINE__, nb)
#define jdwpFree(buf) jdwpFreeTrace(__FILE__, __LINE__, buf)
#else
void *jdwpAlloc(jint numBytes);
void jdwpFree(void *buffer);
#endif
void *jdwpClearedAlloc(jint numBytes);
void *jdwpRealloc(void *original, jint newSize);
char *jdwpStrdup(char *);

void util_lock(void);
void util_unlock(void);
/*
 * Local Reference management. The two macros below are used 
 * throughout the back end whenever space for JNI local references
 * is needed in the current frame. 
 */
#define WITH_LOCAL_REFS(env, number) \
    createLocalRefSpace(env, number); {
#define END_WITH_LOCAL_REFS(env) \
    (*env)->PopLocalFrame(env, NULL); }

void createLocalRefSpace(JNIEnv *env, jint capacity);

#define ERROR_EXIT(message, code) \
    exitWithError(__FILE__, __DATE__, __LINE__, (message), (code))  
                                           \
#define ERROR_MESSAGE_EXIT(message) ERROR_EXIT(message, 0)
#define ERROR_CODE_EXIT(code) ERROR_EXIT("Unexpected error", code)
#define ALLOC_ERROR_EXIT()    ERROR_MESSAGE_EXIT("Allocation failure")

extern jboolean assertOn;
extern jboolean assertFatal;

void jdiAssertionFailed(char *fileName, int lineNumber, char *msg);

#define JDI_ASSERT(expression)  \
do {                            \
    if (assertOn && !(expression)) {		\
        jdiAssertionFailed(__FILE__, __LINE__, #expression); \
    }					        \
} while (0)

#define JDI_ASSERT_MSG(expression, msg)  \
do {                            \
    if (assertOn && !(expression)) {		\
        jdiAssertionFailed(__FILE__, __LINE__, msg); \
    }					        \
} while (0)

#define JDI_ASSERT_FAILED(msg)  \
   jdiAssertionFailed(__FILE__, __LINE__, msg)



    


