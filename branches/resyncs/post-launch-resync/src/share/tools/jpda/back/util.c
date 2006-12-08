/*
 * @(#)util.c	1.78 06/10/10
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "transport.h"
#include "eventHandler.h"
#include "threadControl.h"
#include "outStream.h"
#include "inStream.h"
#include "commonRef.h"
#include "JDWP.h"
#include "invoker.h"
#include "version.h"
#include "debugInit.h"

JavaVM *jvm;
JVMDI_Interface_1 *jvmdi;

static jclass classClass;
static jclass threadClass;
static jclass threadGroupClass;
static jclass classLoaderClass;
static jclass stringClass;
static jclass systemClass;
static jmethodID threadConstructor;
static jmethodID threadCurrentThread;
static jmethodID threadSetDaemon;
static jmethodID systemGetProperty;

static jthreadGroup systemThreadGroup;

static jint cachedJvmdiVersion = 0;

#define HANDLE_ERROR(error)               \
    do {                                  \
        if (error != JVMDI_ERROR_NONE) {  \
            ERROR_CODE_EXIT(error);       \
        }                                 \
    } while (0)


#define HANDLE_NONMEM_ERROR(error)                  \
    do {                                            \
        if ((error != JVMDI_ERROR_NONE) &&          \
            (error != JVMDI_ERROR_OUT_OF_MEMORY)) { \
            ERROR_CODE_EXIT(error);                 \
        }                                           \
    } while (0)

void 
util_initialize()
{
    jclass localClassClass;
    jclass localThreadClass;
    jclass localThreadGroupClass;
    jclass localClassLoaderClass;
    jclass localStringClass;
    jclass localSystemClass;
    JNIEnv *env = getEnv();
    jint groupCount;
    jthreadGroup *groups;

    WITH_LOCAL_REFS(env, 6);

    /* Find some standard classes */

    localClassClass = (*env)->FindClass(env,"java/lang/Class");
    if (localClassClass == 0) {
        ERROR_MESSAGE_EXIT("Can't find class java.lang.Class");
    }
    localThreadClass = (*env)->FindClass(env,"java/lang/Thread");
    if (localThreadClass == 0) {
        ERROR_MESSAGE_EXIT("Can't find class java.lang.Thread");
    }
    localThreadGroupClass = (*env)->FindClass(env,"java/lang/ThreadGroup");
    if (localThreadGroupClass == 0) {
        ERROR_MESSAGE_EXIT("Can't find class java.lang.ThreadGroup");
    }
    localClassLoaderClass = (*env)->FindClass(env,"java/lang/ClassLoader");
    if (localClassLoaderClass == 0) {
        ERROR_MESSAGE_EXIT("Can't find class java.lang.ClassLoader");
    }
    localStringClass = (*env)->FindClass(env,"java/lang/String");
    if (localStringClass == 0) {
        ERROR_MESSAGE_EXIT("Can't find class java.lang.String");
    }
    localSystemClass = (*env)->FindClass(env,"java/lang/System");
    if (localSystemClass == 0) {
        ERROR_MESSAGE_EXIT("Can't find class java.lang.System");
    }

    classClass = (*env)->NewGlobalRef(env, localClassClass);
    threadClass = (*env)->NewGlobalRef(env, localThreadClass);
    threadGroupClass = (*env)->NewGlobalRef(env, localThreadGroupClass);
    classLoaderClass = (*env)->NewGlobalRef(env, localClassLoaderClass);
    stringClass = (*env)->NewGlobalRef(env, localStringClass);
    systemClass = (*env)->NewGlobalRef(env, localSystemClass);

    if ((classClass == 0) || (threadClass == 0) || (threadGroupClass == 0) ||
        (stringClass == 0) || (classLoaderClass == 0) || (systemClass == 0)) {
        ERROR_MESSAGE_EXIT("Can't create global references");
    }

    threadConstructor = (*env)->GetMethodID(env, threadClass,
                                            "<init>", 
                                            "(Ljava/lang/ThreadGroup;Ljava/lang/String;)V");
    if (threadConstructor == 0) {
        ERROR_MESSAGE_EXIT("Can't find java.lang.Thread constructor");
    }

    threadCurrentThread = (*env)->GetStaticMethodID(env, threadClass,
                                            "currentThread", 
                                            "()Ljava/lang/Thread;");
    if (threadCurrentThread == 0) {
        ERROR_MESSAGE_EXIT("Can't find java.lang.Thread.currentThread method");
    }
    threadSetDaemon = (*env)->GetMethodID(env, threadClass,
                                         "setDaemon", 
                                         "(Z)V");
    if (threadSetDaemon == 0) {
        ERROR_MESSAGE_EXIT("Can't find java.lang.Thread.setDaemon method");
    }
    systemGetProperty = (*env)->GetStaticMethodID(env, systemClass,
                                         "getProperty", 
                                         "(Ljava/lang/String;)Ljava/lang/String;");
    if (systemGetProperty == 0) {
        ERROR_MESSAGE_EXIT("Can't find java.lang.System.getProperty method");
    }


    groups = topThreadGroups(&groupCount);
    if (groups == NULL) {
        ERROR_MESSAGE_EXIT("Can't get system thread group");
    }

    /* Use the first top level thread group for debugger threads */
    systemThreadGroup = groups[0];

    /* delete every other top level thread group reference */
    freeGlobalRefsPartial(groups, 1, groupCount);

    END_WITH_LOCAL_REFS(env);
}

void
util_reset()
{
}

jboolean 
isObjectTag(jbyte tag) {
    return (tag == JDWP_Tag_OBJECT) ||
           (tag == JDWP_Tag_STRING) ||
           (tag == JDWP_Tag_THREAD) ||
           (tag == JDWP_Tag_THREAD_GROUP) ||
           (tag == JDWP_Tag_CLASS_LOADER) ||
           (tag == JDWP_Tag_CLASS_OBJECT) ||
           (tag == JDWP_Tag_ARRAY);
}

jbyte 
specificTypeKey(jobject object) 
{
    JNIEnv *env = getEnv();

    if (object == NULL) {
        return JDWP_Tag_OBJECT;
    } else if ((*env)->IsInstanceOf(env, object, stringClass)) {
        return JDWP_Tag_STRING;
    } else if ((*env)->IsInstanceOf(env, object, threadClass)) {
        return JDWP_Tag_THREAD;
    } else if ((*env)->IsInstanceOf(env, object, threadGroupClass)) {
        return JDWP_Tag_THREAD_GROUP;
    } else if ((*env)->IsInstanceOf(env, object, classLoaderClass)) {
        return JDWP_Tag_CLASS_LOADER;
    } else if ((*env)->IsInstanceOf(env, object, classClass)) {
        return JDWP_Tag_CLASS_OBJECT;
    } else {
        jboolean classIsArray;
        jclass clazz;

        WITH_LOCAL_REFS(env, 1);
        clazz = (*env)->GetObjectClass(env, object);
        classIsArray = isArrayClass(clazz);
        END_WITH_LOCAL_REFS(env);

        return (classIsArray ? JDWP_Tag_ARRAY : JDWP_Tag_OBJECT);
    }
}

static void 
writeFieldValue(PacketOutputStream *out, jobject object, 
                jfieldID field)
{
    JNIEnv *env = getEnv();
    jclass clazz;
    char *signature;
    jint error;
    jbyte typeKey;

    clazz = (*env)->GetObjectClass(env, object);
    error = fieldSignature(clazz, field, &signature);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return;
    }
    typeKey = signature[0];
    jdwpFree(signature);
    
    /*
     * For primitive types, the type key is bounced back as is. Objects
     * are handled in the switch statement below. 
     */
    if ((typeKey != JDWP_Tag_OBJECT) && (typeKey != JDWP_Tag_ARRAY)) {
        outStream_writeByte(out, typeKey);
    }

    switch (typeKey) {
        case JDWP_Tag_OBJECT:
        case JDWP_Tag_ARRAY:   {
            jobject value = (*env)->GetObjectField(env, object, field);
            outStream_writeByte(out, specificTypeKey(value));
            WRITE_LOCAL_REF(env, out, value);
            break;
        }
    
        case JDWP_Tag_BYTE:
            outStream_writeByte(out, 
                      (*env)->GetByteField(env, object, field));
            break;
    
        case JDWP_Tag_CHAR:
            outStream_writeChar(out, 
                      (*env)->GetCharField(env, object, field));
            break;
    
        case JDWP_Tag_FLOAT:
            outStream_writeFloat(out, 
                      (*env)->GetFloatField(env, object, field));
            break;
    
        case JDWP_Tag_DOUBLE:
            outStream_writeDouble(out, 
                      (*env)->GetDoubleField(env, object, field));
            break;
    
        case JDWP_Tag_INT:
            outStream_writeInt(out, 
                      (*env)->GetIntField(env, object, field));
            break;
    
        case JDWP_Tag_LONG:
            outStream_writeLong(out, 
                      (*env)->GetLongField(env, object, field));
            break;
    
        case JDWP_Tag_SHORT:
            outStream_writeShort(out, 
                      (*env)->GetShortField(env, object, field));
            break;
    
        case JDWP_Tag_BOOLEAN:
            outStream_writeBoolean(out, 
                      (*env)->GetBooleanField(env, object, field));
            break;
    }
}

static void 
writeStaticFieldValue(PacketOutputStream *out, jclass clazz, 
                      jfieldID field)
{
    JNIEnv *env = getEnv();
    jint error;
    char *signature;
    jbyte typeKey;
    
    error = fieldSignature(clazz, field, &signature);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return;
    }
    typeKey = signature[0];
    jdwpFree(signature);

    /*
     * For primitive types, the type key is bounced back as is. Objects
     * are handled in the switch statement below. 
     */
    if ((typeKey != JDWP_Tag_OBJECT) && (typeKey != JDWP_Tag_ARRAY)) {
        outStream_writeByte(out, typeKey);
    }

    switch (typeKey) {
        case JDWP_Tag_OBJECT:
        case JDWP_Tag_ARRAY:   {
            jobject value = (*env)->GetStaticObjectField(env, clazz, field);
            outStream_writeByte(out, specificTypeKey(value));
            WRITE_LOCAL_REF(env, out, value);
            break;
        }
    
        case JDWP_Tag_BYTE:
            outStream_writeByte(out, 
                      (*env)->GetStaticByteField(env, clazz, field));
            break;
    
        case JDWP_Tag_CHAR:
            outStream_writeChar(out, 
                      (*env)->GetStaticCharField(env, clazz, field));
            break;
    
        case JDWP_Tag_FLOAT:
            outStream_writeFloat(out, 
                      (*env)->GetStaticFloatField(env, clazz, field));
            break;
    
        case JDWP_Tag_DOUBLE:
            outStream_writeDouble(out, 
                      (*env)->GetStaticDoubleField(env, clazz, field));
            break;
    
        case JDWP_Tag_INT:
            outStream_writeInt(out, 
                      (*env)->GetStaticIntField(env, clazz, field));
            break;
    
        case JDWP_Tag_LONG:
            outStream_writeLong(out, 
                      (*env)->GetStaticLongField(env, clazz, field));
            break;
    
        case JDWP_Tag_SHORT:
            outStream_writeShort(out, 
                      (*env)->GetStaticShortField(env, clazz, field));
            break;
    
        case JDWP_Tag_BOOLEAN:
            outStream_writeBoolean(out, 
                      (*env)->GetStaticBooleanField(env, clazz, field));
            break;
    }
}

void 
sharedGetFieldValues(PacketInputStream *in, PacketOutputStream *out,
                     jboolean isStatic)
{
    JNIEnv *env = getEnv();
    jint length;
    jint i;
    jobject object = NULL;
    jclass clazz = NULL;

    if (isStatic)
        clazz = inStream_readClassRef(in);
    else
        object = inStream_readObjectRef(in);

    length = inStream_readInt(in);
    if (inStream_error(in)) {
        return;
    }

    WITH_LOCAL_REFS(env, length + 1); /* +1 for class with instance fields */

    outStream_writeInt(out, length);
    for (i = 0; (i < length) && !outStream_error(out); i++) {
        jfieldID field = inStream_readFieldID(in);

        if (isStatic)
            writeStaticFieldValue(out, clazz, field);
        else
            writeFieldValue(out, object, field);
    }

    END_WITH_LOCAL_REFS(env);
}

jboolean 
sharedInvoke(PacketInputStream *in, PacketOutputStream *out)
{
    jint i;
    jvalue *arguments;
    jint options;
    jint error;
    jbyte invokeType;
    jclass clazz;
    jmethodID method;
    jint argumentCount;
    jobject instance;
    jthread thread;

    /*
     * Instance methods start with the instance, thread and class, 
     * and statics and constructors start with the class and then the
     * thread.
     */
    if (inStream_command(in) == JDWP_COMMAND(ObjectReference, InvokeMethod)) {
        instance = inStream_readObjectRef(in);
        thread = inStream_readThreadRef(in);
        clazz = inStream_readClassRef(in);
    } else { /* static method or constructor */
        instance = NULL;
        clazz = inStream_readClassRef(in);
        thread = inStream_readThreadRef(in);
    }

    /*
     * ... and the rest of the packet is identical for all commands
     */
    method = inStream_readMethodID(in);
    argumentCount = inStream_readInt(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    arguments = jdwpAlloc(argumentCount * sizeof(*arguments));
    if (arguments == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
        return JNI_TRUE;
    }

    for (i = 0; (i < argumentCount) && !inStream_error(in); i++) {
        arguments[i] = inStream_readValue(in, NULL);
    }
    options = inStream_readInt(in);
    if (inStream_error(in)) {
        jdwpFree(arguments);
        return JNI_TRUE;
    }

    if (inStream_command(in) == JDWP_COMMAND(ClassType, NewInstance)) {
        invokeType = INVOKE_CONSTRUCTOR;
    } else if (inStream_command(in) == JDWP_COMMAND(ClassType, InvokeMethod)) {
        invokeType = INVOKE_STATIC;
    } else if (inStream_command(in) == JDWP_COMMAND(ObjectReference, InvokeMethod)) {
        invokeType = INVOKE_INSTANCE;
    } else {
        outStream_setError(out, JVMDI_ERROR_INTERNAL);
        jdwpFree(arguments);
        return JNI_TRUE;
    }

    /*
     * Request the invoke. If there are no errors in the request,
     * the interrupting thread will actually do the invoke and a 
     * reply will be generated subsequently, so we don't reply here.
     */
    error = invoker_requestInvoke(invokeType, (jbyte)options, inStream_id(in), 
                                  thread, clazz, method, 
                                  instance, arguments);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        jdwpFree(arguments);
        return JNI_TRUE;
    }

    return JNI_FALSE;   /* Don't reply */
}

jint 
uniqueID() 
{
    static jint currentID = 0;
    return currentID++;
}

jint 
filterDebugThreads(jthread *threads, int count)
{
    jint i;
    jint current;
    JNIEnv *env = getEnv();

    /* Squish out all of the debugger-spawned threads */
    for (i = 0, current = 0; i < count; i++) {
        jthread thread = threads[i];
        if (!threadControl_isDebugThread(thread)) {
            if (i > current) {
                threads[current] = thread;
            }
            current++;
        } else {
            (*env)->DeleteGlobalRef(env, thread);
        }
    }
    return current;
}

jbyte 
referenceTypeTag(jclass clazz) 
{
    jbyte tag; 

    if (isInterface(clazz)) {
        tag = JDWP_TypeTag_INTERFACE;
    } else if (isArrayClass(clazz)) {
        tag = JDWP_TypeTag_ARRAY;
    } else {
        tag = JDWP_TypeTag_CLASS;
    }

    return tag;
}

/*  Workaround bug 4196771   */
static jvmdiError
tempFrameCount(JNIEnv *env, jthread thread, jint *countPtr) 
{
    jint error;
    jint frameCount; 
    jframeID frame;
    
    error = jvmdi->GetCurrentFrame(thread, &frame);
    if (error == JVMDI_ERROR_NO_MORE_FRAMES) {
        *countPtr = 0;
        return JVMDI_ERROR_NONE;
    } else if (error != JVMDI_ERROR_NONE) {
        return error;
    }
    frameCount = 1;     /* for current frame */
    while (JNI_TRUE) {
        error = jvmdi->GetCallerFrame(frame, &frame);
        if (error == JVMDI_ERROR_NO_MORE_FRAMES) {
            break;
        }
        if (error != JVMDI_ERROR_NONE) {
            return error;
        }
        ++frameCount;
    }        
    *countPtr = frameCount;
    return JVMDI_ERROR_NONE;
}

jint 
frameCount(jthread thread, jint *count) 
{
    jint frameCount;
    JNIEnv *env = getEnv();
    jint error;
    if (version_supportsFrameCount()) {
        error = jvmdi->GetFrameCount(thread, &frameCount);
    } else {
        error = tempFrameCount(env, thread, &frameCount);
    }
    if (error == JVMDI_ERROR_NONE) {
        *count = frameCount;
    }
    return error;
}

/**
 * Get field signature
 */
jint 
fieldSignature(jclass clazz, jfieldID field, char **signaturePtr)
{
    jint rc;
    char *namePtr;

    rc = jvmdi->GetFieldName(clazz, field, &namePtr, signaturePtr);
    if (rc == JVMDI_ERROR_NONE) {
        jdwpFree(namePtr);
    }

    return rc;
}

/************ COMMENTED OUT ********
jint 
variableSignature(jframeID frame, jint slot, char **sigPtr) 
{
    jint rc;
    jint varCount;
    JVMDI_local_variable_entry *varEntries;
    char *signature = NULL;
    int i;
    jclass clazz;
    jmethodID method;
    jlocation location;

    rc = jvmdi->GetFrameLocation(frame, &clazz, &method, &location);
    if (rc != JVMDI_ERROR_NONE) {
        return rc;
    }

    rc = jvmdi->GetLocalVariableTable(clazz, method, &varCount, &varEntries);
    (*env)->DeleteGlobalRef(env, clazz);
    if (rc != JVMDI_ERROR_NONE) {
        return rc;
    }

    for (i = 0; i < varCount; i++) {
        JVMDI_local_variable_entry *entry = &varEntries[i];
        if ((signature == NULL) && (entry->slot == slot) &&
            (entry->start_location <= location) &&
            (location < entry->start_location + entry->length)) {

            signature = entry->signature;
        } else {
            jdwpFree(entry->signature);
        }
        jdwpFree(entry->name);
    }
    jdwpFree(varEntries);

    if (signature == NULL) {
        rc = JVMDI_ERROR_INVALID_SLOT;
    }

    *sigPtr = signature;

    return rc;
}
************/

/* These validations in validateAssignment() are now done on the front end: 
   deleteClassRefs() and findClass() are also not needed since they are
   only used by validateAssignment().
*/
#if 0
static void 
deleteClassRefs(JNIEnv *env, jclass *classList, jint length) 
{
    int i;
    for (i = 0; i < length; i++) {
        (*env)->DeleteGlobalRef(env, classList[i]);
    }
}

static jclass 
findClass(jclass *classList, jint length, char *signature) 
{
    int i;
    char *candidate;
    jclass match = NULL;

    for (i = 0; (i < length) && (match == NULL); i++) {
        candidate = classSignature(classList[i]);
        if (candidate == NULL) {
             /* %comment gordonh001 */
        }
        if (strcmp(signature, candidate) == 0) {
            match = classList[i];
        }
        jdwpFree(candidate);
    }

    return match;
}

/* These validations are now done on the front end */
jint 
validateAssignment(jclass enclosingClass, jobject sourceObject, char *destSignature)
{
    JNIEnv *env = getEnv();
    jint rc;
    jobject loader;
    jint count;
    jclass *classes;
    jclass match;

    loader = classLoader(enclosingClass);
    classes = loadedClasses(loader, &count);
    if (classes == NULL) {
         *
         * %comment gordonh001 
         *
    }
    match = findClass(classes, count, destSignature);
    if (match == NULL) {
         *
         * %comment gordonh002
         *
        rc = JVMDI_ERROR_CLASS_NOT_PREPARED;
    } else {
        jint status = classStatus(match);
        if ((status & JVMDI_CLASS_STATUS_INITIALIZED) == 0) {
            rc = JVMDI_ERROR_CLASS_NOT_PREPARED;
        } else if (!(*env)->IsInstanceOf(env, sourceObject, match)) {
            rc = JVMDI_ERROR_TYPE_MISMATCH;
        } else {
            rc = JVMDI_ERROR_NONE;
        }
    }
    deleteClassRefs(env, classes, count);
    (*env)->DeleteGlobalRef(env, loader);
    
    return rc;
}
#endif

JNIEnv *
getEnv()
{
    JNIEnv *env;
    jint error;

    error = (*jvm)->GetEnv(jvm, (void **)(void *)&env, JNI_VERSION_1_2);
    if (error != JNI_OK) {
        /*
         * Can't use normal ERROR_EXIT here, since it relies on getEnv().
         */
        fprintf(stderr, 
                "Unable to get JNI 1.2 environment, error code = %d\n", 
                error);
        exit(-1);
    }
    return env;
}

static char *
errorText(jint error) 
{
    switch (error) {
        case JVMDI_ERROR_INVALID_THREAD:       
            return  "JVMDI_ERROR_INVALID_THREAD";       
        case JVMDI_ERROR_INVALID_THREAD_GROUP: 
            return  "JVMDI_ERROR_INVALID_THREAD_GROUP"; 
        case JVMDI_ERROR_INVALID_PRIORITY:     
            return  "JVMDI_ERROR_INVALID_PRIORITY";     
        case JVMDI_ERROR_THREAD_NOT_SUSPENDED: 
            return  "JVMDI_ERROR_THREAD_NOT_SUSPENDED"; 
        case JVMDI_ERROR_THREAD_SUSPENDED:     
            return  "JVMDI_ERROR_THREAD_SUSPENDED";     
        case JVMDI_ERROR_INVALID_OBJECT:       
            return  "JVMDI_ERROR_INVALID_OBJECT";       
        case JVMDI_ERROR_INVALID_CLASS:        
            return  "JVMDI_ERROR_INVALID_CLASS";        
        case JVMDI_ERROR_CLASS_NOT_PREPARED:   
            return  "JVMDI_ERROR_CLASS_NOT_PREPARED";   
        case JVMDI_ERROR_INVALID_METHODID:     
            return  "JVMDI_ERROR_INVALID_METHODID";     
        case JVMDI_ERROR_INVALID_LOCATION:     
            return  "JVMDI_ERROR_INVALID_LOCATION";     
        case JVMDI_ERROR_INVALID_FIELDID:      
            return  "JVMDI_ERROR_INVALID_FIELDID";      
        case JVMDI_ERROR_INVALID_FRAMEID:      
            return  "JVMDI_ERROR_INVALID_FRAMEID";      
        case JVMDI_ERROR_NO_MORE_FRAMES:       
            return  "JVMDI_ERROR_NO_MORE_FRAMES";       
        case JVMDI_ERROR_OPAQUE_FRAME:         
            return  "JVMDI_ERROR_OPAQUE_FRAME";         
        case JVMDI_ERROR_NOT_CURRENT_FRAME:    
            return  "JVMDI_ERROR_NOT_CURRENT_FRAME";    
        case JVMDI_ERROR_TYPE_MISMATCH:        
            return  "JVMDI_ERROR_TYPE_MISMATCH";        
        case JVMDI_ERROR_INVALID_SLOT:         
            return  "JVMDI_ERROR_INVALID_SLOT";         
        case JVMDI_ERROR_DUPLICATE: 
            return  "JVMDI_ERROR_DUPLICATE"; 
        case JVMDI_ERROR_NOT_FOUND:   
            return  "JVMDI_ERROR_NOT_FOUND";   
        case JVMDI_ERROR_INVALID_MONITOR:      
            return  "JVMDI_ERROR_INVALID_MONITOR";      
        case JVMDI_ERROR_NOT_MONITOR_OWNER:    
            return  "JVMDI_ERROR_NOT_MONITOR_OWNER";    
        case JVMDI_ERROR_INTERRUPT:            
            return  "JVMDI_ERROR_INTERRUPT";            
        case JVMDI_ERROR_NULL_POINTER:         
            return  "JVMDI_ERROR_NULL_POINTER";         
        case JVMDI_ERROR_ABSENT_INFORMATION:   
            return  "JVMDI_ERROR_ABSENT_INFORMATION";   
        case JVMDI_ERROR_OUT_OF_MEMORY:        
            return  "JVMDI_ERROR_OUT_OF_MEMORY";        
        case JVMDI_ERROR_ACCESS_DENIED:        
            return  "JVMDI_ERROR_ACCESS_DENIED";        
        case JVMDI_ERROR_VM_DEAD:              
            return  "JVMDI_ERROR_VM_DEAD";              
        case JVMDI_ERROR_INTERNAL:             
            return  "JVMDI_ERROR_INTERNAL";             
        case JVMDI_ERROR_NOT_IMPLEMENTED:      
            return  "JVMDI_ERROR_NOT_IMPLEMENTED";      
        case JVMDI_ERROR_UNATTACHED_THREAD:    
            return  "JVMDI_ERROR_UNATTACHED_THREAD";    
        default:
            return  "Unknown";    
    }
}

void 
exitWithError(char *fileName, char *date, int lineNumber, 
              char *message, jint errorCode)
{
    char buf[500];
    JNIEnv *env = getEnv();

    /* Get past path info */
    char *p1 = strrchr(fileName, '\\');
    char *p2 = strrchr(fileName, '/');
    p1 = ((p1 > p2) ? p1 : p2);
    if (p1 != NULL) {
        fileName = p1 + 1;
    }

    if (errorCode != 0) {		                              
        sprintf(buf, "JDWP \"%s\" (%s), line %d: %s, error code = %d (%s)\n",
                fileName, date, lineNumber, message, errorCode, 
                errorText(errorCode));             \
    } else {                                                       
        sprintf(buf, "JDWP \"%s\" (%s), line %d: %s\n",                    
                fileName, date, lineNumber, message);             \
    }                                                              

    (*env)->FatalError(env, buf);
}

jint 
spawnNewThread(void (*func)(void *), void *arg, char *name)
{
    JNIEnv *env = getEnv();
    jthread thread;
    jint rc;
    jstring nameString;

    WITH_LOCAL_REFS(env, 2);

    nameString = (*env)->NewStringUTF(env, name);
    if ((*env)->ExceptionOccurred(env)) {
        (*env)->ExceptionClear(env);
        rc = JVMDI_ERROR_OUT_OF_MEMORY;
        goto err;
    }

    thread = (*env)->NewObject(env, threadClass, threadConstructor, 
                               systemThreadGroup, nameString);
    if ((*env)->ExceptionOccurred(env)) {
        (*env)->ExceptionClear(env);
        rc = JVMDI_ERROR_OUT_OF_MEMORY;
        goto err;
    }

    /*
     * Make the debugger thread a daemon
     */
    (*env)->CallVoidMethod(env, thread, threadSetDaemon, JNI_TRUE);
    if ((*env)->ExceptionOccurred(env)) {
        (*env)->ExceptionClear(env);
        rc = JVMDI_ERROR_INTERNAL;
        goto err;
    }


    thread = (*env)->NewGlobalRef(env, thread);

    rc = threadControl_addDebugThread(thread);
    if (rc == JVMDI_ERROR_NONE) {
        /*
         * Debugger threads need cycles in all sorts of strange 
         * situations (e.g. infinite cpu-bound loops), so give the 
         * thread a high priority. Note that if the VM has an application
         * thread running at the max priority, there is still a chance
         * that debugger threads will be starved. (There needs to be 
         * a way to give debugger threads a priority higher than any
         * application thread).
         */
        rc = jvmdi->RunDebugThread(thread, func, arg, 
                                    JVMDI_THREAD_MAX_PRIORITY);
    }

err:
    END_WITH_LOCAL_REFS(env);

    return rc;
}

jthread 
currentThread(void)
{
    JNIEnv *env = getEnv();
    jthread thread;

    WITH_LOCAL_REFS(env, 2);

    thread = (*env)->CallStaticObjectMethod(env, threadClass, 
                                            threadCurrentThread);
    thread = (*env)->NewGlobalRef(env, thread);

    END_WITH_LOCAL_REFS(env);

    return thread;
}

/* Return the object instance in which the event occurred */
/* Return NULL if static or if an error occurs */
jobject 
eventInstance(JVMDI_Event *event)  
{
    jthread thread = NULL;
    jclass clazz;
    jmethodID method;
    jframeID frame = NULL;
    jint modifiers;
    jint error;

    switch (event->kind) {
        case JVMDI_EVENT_SINGLE_STEP:  	    
            thread = event->u.single_step.thread;
            clazz = event->u.single_step.clazz;
            method = event->u.single_step.method;
            break;
        case JVMDI_EVENT_BREAKPOINT:   
            thread = event->u.breakpoint.thread;
            clazz = event->u.breakpoint.clazz;
            method = event->u.breakpoint.method;
            break;
        case JVMDI_EVENT_FRAME_POP:    
        case JVMDI_EVENT_METHOD_ENTRY:    
        case JVMDI_EVENT_METHOD_EXIT:    
            thread = event->u.frame.thread;
            clazz = event->u.frame.clazz;
            method = event->u.frame.method;
            frame = event->u.frame.frame;
            break;
        case JVMDI_EVENT_FIELD_ACCESS:    
            return event->u.field_access.object;
        case JVMDI_EVENT_FIELD_MODIFICATION:    
            return event->u.field_modification.object;
        case JVMDI_EVENT_EXCEPTION:    
            thread = event->u.exception.thread;
            clazz = event->u.exception.clazz;
            method = event->u.exception.method;
            break;
        case JVMDI_EVENT_EXCEPTION_CATCH:    
            thread = event->u.exception_catch.thread;
            clazz = event->u.exception_catch.clazz;
            method = event->u.exception_catch.method;
            break;
        default:
            return NULL;
    }
    error = jvmdi->GetMethodModifiers(clazz, method, &modifiers);
    /* fail if error or static (0x8) */
    if (error != JVMDI_ERROR_NONE || (modifiers & 0x8) != 0) { 
        return NULL;       /*   if static, no object */
    } else {     
        jobject object;

        if (frame == NULL) {
	    error = jvmdi->GetCurrentFrame(thread, &frame);
            if (error != JVMDI_ERROR_NONE) { 
                return NULL;   
            }
	}
	/* get slot zero object "this" */
	error = jvmdi->GetLocalObject(frame, 0, &object);
        if (error != JVMDI_ERROR_NONE) { 
            return NULL;   
        }
	return object;
    }
}


void 
eventThreadAndClass(JVMDI_Event *event, jthread *thread, jclass *clazz)
{
    switch (event->kind) {
        case JVMDI_EVENT_SINGLE_STEP:  
            *thread = event->u.single_step.thread;
            *clazz = event->u.single_step.clazz;
            break;
        case JVMDI_EVENT_BREAKPOINT:   
            *thread = event->u.breakpoint.thread;
            *clazz = event->u.breakpoint.clazz;
            break;
        case JVMDI_EVENT_FRAME_POP:    
        case JVMDI_EVENT_METHOD_ENTRY:    
        case JVMDI_EVENT_METHOD_EXIT:    
            *thread = event->u.frame.thread;
            *clazz = event->u.frame.clazz;
            break;
        case JVMDI_EVENT_FIELD_ACCESS:    
            *thread = event->u.field_access.thread;
            *clazz = event->u.field_access.clazz;
            break;
        case JVMDI_EVENT_FIELD_MODIFICATION:    
            *thread = event->u.field_modification.thread;
            *clazz = event->u.field_modification.clazz;
            break;
        case JVMDI_EVENT_EXCEPTION:    
            *thread = event->u.exception.thread;
            *clazz = event->u.exception.clazz;
            break;
        case JVMDI_EVENT_EXCEPTION_CATCH:    
            *thread = event->u.exception_catch.thread;
            *clazz = event->u.exception_catch.clazz;
            break;
        case JVMDI_EVENT_THREAD_START:
        case JVMDI_EVENT_THREAD_END:   
            *thread = event->u.thread_change.thread;
            *clazz = NULL;
            break;
        case JVMDI_EVENT_CLASS_LOAD:   
        case JVMDI_EVENT_CLASS_PREPARE:   
        case JVMDI_EVENT_CLASS_UNLOAD: 
            *thread = event->u.class_event.thread;
            *clazz = event->u.class_event.clazz;
            break;
        default:
            *thread = NULL;
            *clazz = NULL;
            break;
    }
}

jthread
eventThread(JVMDI_Event *event)
{
    jthread thread;
    jclass clazz;

    eventThreadAndClass(event, &thread, &clazz);

    return thread;
}

jclass 
eventClass(JVMDI_Event *event) 
{
    jthread thread;
    jclass clazz;

    eventThreadAndClass(event, &thread, &clazz);

    return clazz;
}


jint
jvmdiVersion(void)
{
    if (cachedJvmdiVersion == 0) {
        HANDLE_ERROR(jvmdi->GetVersionNumber(&cachedJvmdiVersion));
    }
    return cachedJvmdiVersion;
}

jint
jvmdiMajorVersion(void)
{
    return (jvmdiVersion() >> 16) & 0x0fff;
}

jint
jvmdiMinorVersion(void)
{
    return jvmdiVersion() & 0xff;
}

jboolean
canGetSourceDebugExtension(void)
{
    return (jvmdiMajorVersion() > 1) || (jvmdiMinorVersion() >= 2);
}

int 
getSourceDebugExtension(jclass clazz, char **extensionPtr)
{
    /* make sure we don't try access this function pointer */
    /* in a VM that doesn't have this addition */
    if (!canGetSourceDebugExtension()) {
        return JVMDI_ERROR_NOT_IMPLEMENTED;
    }        

    return jvmdi->GetSourceDebugExtension(clazz, extensionPtr);
}
    
/*
 * Convert the signature "Ljava/lang/Foo;" to a 
 * classname "java.lang.Foo" compatible with the pattern.
 * Signature is overwritten in-place.
 */
void
convertSignatureToClassname(char *convert)
{
    char *p;

    p = convert + 1;
    while ((*p != ';') && (*p != '\0')) {
        char c = *p;
        if (c == '/') {
            *(p-1) = '.';
        } else {
            *(p-1) = c;
        }
        p++;
    }
    *(p-1) = '\0';
}

static void
handleInterrupt(void) 
{
    /*
     * An interrupt is handled:
     *
     * 1) for running application threads by deferring the interrupt
     * until the current event handler has concluded. 
     *
     * 2) for debugger threads by ignoring the interrupt; this is the 
     * most robust solution since debugger threads don't use interrupts
     * to signal any condition. 
     *
     * 3) for application threads that have not started or already 
     * ended by ignoring the interrupt. In the former case, the application
     * is relying on timing to determine whether or not the thread sees
     * the interrupt; in the latter case, the interrupt is meaningless. 
     */
    jthread thread = threadControl_currentThread();
    if ((thread != NULL) && (!threadControl_isDebugThread(thread))) {
        threadControl_setPendingInterrupt(thread);
    }
}

void 
debugMonitorEnter(JVMDI_RawMonitor monitor)
{
    jint rc;
    while (JNI_TRUE) {
        rc = jvmdi->RawMonitorEnter(monitor);
        if (rc == JVMDI_ERROR_INTERRUPT) {
            handleInterrupt();
        } else {
            break;
        }
    } 
    HANDLE_ERROR(rc);
}

void 
debugMonitorExit(JVMDI_RawMonitor monitor)
{
    HANDLE_ERROR(jvmdi->RawMonitorExit(monitor));
}

void 
debugMonitorWait(JVMDI_RawMonitor monitor)
{
    jint rc;
    while (JNI_TRUE) {
        rc = jvmdi->RawMonitorWait(monitor, JVMDI_MONITOR_WAIT_FOREVER);
        if (rc == JVMDI_ERROR_INTERRUPT) {
            handleInterrupt();
        } else {
            break;
        }
    } 
    HANDLE_ERROR(rc);
}

void 
debugMonitorTimedWait(JVMDI_RawMonitor monitor, jlong millis)
{
    jint rc;
    while (JNI_TRUE) {
        rc = jvmdi->RawMonitorWait(monitor, millis);
        if (rc == JVMDI_ERROR_INTERRUPT) {
            handleInterrupt();
        } else {
            break;
        }
    } 
    HANDLE_ERROR(rc);
}

void 
debugMonitorNotify(JVMDI_RawMonitor monitor)
{
    HANDLE_ERROR(jvmdi->RawMonitorNotify(monitor));
}

void 
debugMonitorNotifyAll(JVMDI_RawMonitor monitor)
{
    HANDLE_ERROR(jvmdi->RawMonitorNotifyAll(monitor));
}

JVMDI_RawMonitor 
debugMonitorCreate(char *name)
{
    JVMDI_RawMonitor monitor;
    HANDLE_ERROR(jvmdi->CreateRawMonitor(name, &monitor));
    return monitor;
}

void 
debugMonitorDestroy(JVMDI_RawMonitor monitor)
{
    HANDLE_ERROR(jvmdi->DestroyRawMonitor(monitor));
}

/**
 * Return array of all threads
 */
jthread *
allThreads(jint *count) 
{
    jthread *threads = NULL;
    HANDLE_NONMEM_ERROR(jvmdi->GetAllThreads(count, &threads));
    return threads;
}

/**
 * Fill the passed in structure with thread info.
 * name field is JVMDI allocated.  
 * thread_group and context_class_loader are global refs.
 */
void
threadInfo(jthread thread, JVMDI_thread_info *info)
{
    HANDLE_ERROR(jvmdi->GetThreadInfo(thread, info));
}

/**
 * Return array of all threads groups without parents
 */
jthreadGroup *topThreadGroups(jint *count)
{
    jthreadGroup *groups = NULL;
    HANDLE_NONMEM_ERROR(jvmdi->GetTopThreadGroups(count, &groups));
    return groups;
}

/**
 * Fill the passed in structure with thread group info.
 * name field is JVMDI allocated.  parent is global ref.
 */
void
threadGroupInfo(jthreadGroup group, JVMDI_thread_group_info *info)
{
    HANDLE_ERROR(jvmdi->GetThreadGroupInfo(group, info));
}

/**
 * Return class signature string
 */
char *
classSignature(jclass clazz)
{
    char *signature = NULL;
    HANDLE_NONMEM_ERROR(jvmdi->GetClassSignature(clazz, &signature));

    /*
     * Workaround for classic VM bug 4232335. Translate invalid 
     * primitive class signatures to the correct values. Note that 
     * there is no danger here because real classes cannot have the 
     * signatures below due to reserved keyword restrictions.
     */
    if (!version_supportsPrimitiveClassSignatures()) {
        if (strcmp(signature, "Lboolean;") == 0) {
            strcpy(signature, "Z");
        } else if (strcmp(signature, "Lbyte;") == 0) {
            strcpy(signature, "B");
        } else if (strcmp(signature, "Lchar;") == 0) {
            strcpy(signature, "C");
        } else if (strcmp(signature, "Lshort;") == 0) {
            strcpy(signature, "S");
        } else if (strcmp(signature, "Lint;") == 0) {
            strcpy(signature, "I");
        } else if (strcmp(signature, "Llong;") == 0) {
            strcpy(signature, "J");
        } else if (strcmp(signature, "Lfloat;") == 0) {
            strcpy(signature, "F");
        } else if (strcmp(signature, "Ldouble;") == 0) {
            strcpy(signature, "D");
        }
    }
    return signature;
}

jint 
classStatus(jclass clazz)
{
    jint status;
    HANDLE_ERROR(jvmdi->GetClassStatus(clazz, &status));
    return status;
}

jint 
classModifiers(jclass clazz) 
{
    jint modifiers;
    HANDLE_ERROR(jvmdi->GetClassModifiers(clazz, &modifiers));
    return modifiers;
}

jobject 
classLoader(jclass clazz)
{
    jobject loader;
    HANDLE_ERROR(jvmdi->GetClassLoader(clazz, &loader));
    return loader;
}

/**
 * Return array of jmethodIDs, representing declared methods
 */
jmethodID *
declaredMethods(jclass clazz, jint *count)
{
    jmethodID *methods = NULL;
    HANDLE_NONMEM_ERROR(jvmdi->GetClassMethods(clazz, count, &methods));
    return methods;
}

/**
 * Return array of jfieldIDs, representing declared fields
 */
jfieldID *
declaredFields(jclass clazz, jint *count)
{
    jfieldID *fields = NULL;
    HANDLE_NONMEM_ERROR(jvmdi->GetClassFields(clazz, count, &fields));
    return fields;
}

/**
 * Return array of jclass, representing implemented interfaces
 */
jclass *
implementedInterfaces(jclass clazz, jint *count)
{
    jclass *interfaces = NULL;
    HANDLE_NONMEM_ERROR(jvmdi->GetImplementedInterfaces(clazz, count, &interfaces));
    return interfaces;
}

jboolean isArrayClass(jclass clazz)
{
    jboolean isArray;
    HANDLE_ERROR(jvmdi->IsArrayClass(clazz, &isArray));
    return isArray;
}

jboolean isInterface(jclass clazz)
{
    jboolean isInterface;
    HANDLE_ERROR(jvmdi->IsInterface(clazz, &isInterface));
    return isInterface;
}

jboolean isMethodNative(jclass clazz, jmethodID method)
{
    jboolean isNative;
    HANDLE_ERROR(jvmdi->IsMethodNative(clazz, method, &isNative));
    return isNative;
}

jint 
objectHashCode(jobject object)
{
    jint hashCode;
    HANDLE_ERROR(jvmdi->GetObjectHashCode(object, &hashCode));
    return hashCode;
}

jclass *
allLoadedClasses(jint *count)
{
    jclass *classes = NULL;
    HANDLE_NONMEM_ERROR(jvmdi->GetLoadedClasses(count, &classes));
    return classes;
}

jclass *
loadedClasses(jobject classLoader, jint *count)
{
    jclass *classes = NULL;
    HANDLE_NONMEM_ERROR(jvmdi->GetClassLoaderClasses(classLoader, count, &classes));
    return classes;
}

void
createLocalRefSpace(JNIEnv *env, jint capacity) 
{
    /*
     * Save current exception since it might get overwritten by
     * the calls below. Note we must depend on space in the existing
     * frame because asking for a new frame may generate an exception.
     */
    jobject throwable = (*env)->ExceptionOccurred(env);

    /*
     * Use the current frame if necessary; otherwise create a new one
     */
    if ((*env)->PushLocalFrame(env, capacity) < 0) {
        ERROR_MESSAGE_EXIT("Unable to allocate JNI local references");
    }

    /* %comment gordon003 */

    /*
     * Restore exception state from before call
     */
    if (throwable != NULL) {
        (*env)->Throw(env, throwable);
    } else {
        (*env)->ExceptionClear(env);
    }
}

jboolean isClass(jobject object) 
{
    JNIEnv *env = getEnv();
    return (*env)->IsInstanceOf(env, object, classClass);
}

jboolean isThread(jobject object)
{
    JNIEnv *env = getEnv();
    return (*env)->IsInstanceOf(env, object, threadClass);
}

jboolean isThreadGroup(jobject object)
{
    JNIEnv *env = getEnv();
    return (*env)->IsInstanceOf(env, object, threadGroupClass);
}

jboolean isString(jobject object)
{
    JNIEnv *env = getEnv();
    return (*env)->IsInstanceOf(env, object, stringClass);
}

jboolean isClassLoader(jobject object)
{
    JNIEnv *env = getEnv();
    return (*env)->IsInstanceOf(env, object, classLoaderClass);
}

jboolean isArray(jobject object)
{
    JNIEnv *env = getEnv();
    jboolean is;
    jclass clazz;

    WITH_LOCAL_REFS(env, 1);
    clazz = (*env)->GetObjectClass(env, object);
    is = isArrayClass(clazz);
    END_WITH_LOCAL_REFS(env);

    return is;
}

/**
 * Free an array of global refs received from JVMDI.
 * That is, free each element of the array and the array itself.
 */
void 
freeGlobalRefs(jobject *objects, jint count) 
{
    freeGlobalRefsPartial(objects, 0, count);
}

/**
 * Free an array of global refs received from JVMDI.
 * That is, free each element of the array and the array itself.
 * Start freeing at the selected index.
 */
void 
freeGlobalRefsPartial(jobject *objects, jint startIndex, jint count) 
{
    int i;
    JNIEnv *env = getEnv();

    for (i = startIndex; i < count; i++) {
        (*env)->DeleteGlobalRef(env, objects[i]);
    }
    jdwpFree(objects);
}

/**
 * Return property as JDWP allocated string
 */
char *
getPropertyCString(char *propertyName)
{
    JNIEnv *env = getEnv();
    const char *utf;
    char *cstring = NULL;
    jstring valueString;
    jstring nameString;

    WITH_LOCAL_REFS(env, 1);

    nameString = (*env)->NewStringUTF(env, propertyName);
    if ((*env)->ExceptionOccurred(env)) {
        (*env)->ExceptionClear(env);
        /* NULL will be returned below */
    } else {
        valueString = (*env)->CallStaticObjectMethod(env, systemClass, 
                                        systemGetProperty, nameString);
        if ((*env)->ExceptionOccurred(env)) {
            (*env)->ExceptionClear(env);
        } else if (valueString != NULL) {
            utf = (*env)->GetStringUTFChars(env, valueString, NULL);
            cstring = jdwpAlloc(strlen(utf) + 1);
            if (cstring != NULL) {
                strcpy(cstring, utf);
            }
            (*env)->ReleaseStringUTFChars(env, valueString, utf);
        }
    }

    END_WITH_LOCAL_REFS(env);

    return cstring;
}


void 
writeCodeLocation(PacketOutputStream *out, jclass clazz, 
                       jmethodID method, jlocation location)
{
    JNIEnv *env = getEnv();
    jbyte tag;

    if (clazz != NULL) {
        tag = referenceTypeTag(clazz);
    } else {
        tag = JDWP_TypeTag_CLASS;
    }
    outStream_writeByte(out, tag);
    WRITE_GLOBAL_REF(env, out, clazz);
    outStream_writeMethodID(out, method);
    outStream_writeLocation(out, location);
}
/*
 * allocate/reallocate/free functions. These functions normally use
 * a separate allocator from Doug Lea, implemented in dlAlloc.[ch]
 * Through a startup option, the normal CRT malloc functions can be used
 * instead. 
 *
 * The reason for using an alternate malloc: the JVMDI specification 
 * does not make any promises about the state of suspended threads. It
 * is possible for an application thread to be suspended in the middle of 
 * a malloc. In multi-threaded environments, this can mean that a lock 
 * of some kind is held during the suspension, causing any debugger 
 * allocations to hang for the duration of the suspend. By redirecting
 * debugger allocations to here, we avoid the problem. An additional benefit
 * here is that we should be slightly more robust in low memory situations
 * since we are not mallocing from the same place. Also, we are 
 * a little less intrusive which may prevent us from trampling on the 
 * application environment when the debuggee has a heap-related bug 
 * (if the debuggee is a mixed native and Java application). 
 */
 

/*
 * Wrap allocation functions in locks for inefficient but safe behavior in a 
 * multihreaded environment. Avoid namespace problems and allow switching
 * between standard malloc and this one through these wrappers. 
 */

static JVMDI_RawMonitor allocLock = NULL;

void 
util_setAllocLock(JVMDI_RawMonitor lock)
{
    allocLock = lock;
}

#ifdef JDWP_ALLOC_TRACE
int allocSum = 0;

void * 
jdwpAllocReal(jint numBytes) 
{
    return jdwpAllocTrace("REAL", 0, numBytes);
}

void * 
jdwpAllocTrace(char *fn, int ln, jint numBytes)
#else 
void * 
jdwpAlloc(jint numBytes)
#endif
{
    /* To avoid headaches of having to make an unused dlAlloc.c
       compile, allow dl_alloc to be completely eliminated as a
       build-time option */
#ifndef JPDA_NO_DLALLOC
    void *ret; 

    if (debugInit_useStandardAlloc()) {
        return malloc(numBytes);
    } else if (allocLock != NULL) {
        debugMonitorEnter(allocLock);
        ret = dl_malloc(numBytes);
#ifdef JDWP_ALLOC_TRACE
        fprintf(stderr, "WPALOC %x %s:%d %d %d\n", ret, fn , ln,
                numBytes, ++allocSum);
#endif
        debugMonitorExit(allocLock);
    } else {
        /*
         * We need to do some allocations before the allocLock is in place.
         * (The monitor itself requires allocations.) 
         */
        ret = dl_malloc(numBytes);
    }
    return ret;
#else   /* JPDA_NO_DLALLOC */
    return malloc(numBytes);
#endif  /* JPDA_NO_DLALLOC */
}

/*
 * Return numBytes of allocated memory.
 * All bytes cleared to zero.
 */
void * 
jdwpClearedAlloc(jint numBytes) 
{
    void *ret = jdwpAlloc(numBytes);

    if (ret != NULL) {
        memset(ret, 0, (size_t)numBytes);
    }
    return ret;
}

#ifdef JDWP_ALLOC_TRACE
void 
jdwpFreeReal(void *buffer) 
{
    jdwpFreeTrace("REAL", 0, buffer);
}

void 
jdwpFreeTrace(char *fn, int ln, void *buffer) 
#else
void 
jdwpFree(void *buffer) 
#endif
{
#ifndef JPDA_NO_DLALLOC
    if (debugInit_useStandardAlloc()) {
        free(buffer);
    } else if (allocLock != NULL) {
        debugMonitorEnter(allocLock);
        dl_free(buffer);
#ifdef JDWP_ALLOC_TRACE
        if (buffer != NULL) {
            fprintf(stderr, "WPFREE %x %s:%d %d\n", buffer, fn , ln, --allocSum);
        }
#endif
        debugMonitorExit(allocLock);
    } else {
        dl_free(buffer);
    }
#else   /* JPDA_NO_DLALLOC */
    free(buffer);
#endif  /* JPDA_NO_DLALLOC */
}

void *
jdwpRealloc(void *original, jint newSize)
{
#ifndef JPDA_NO_DLALLOC
    void *ret; 

    if (debugInit_useStandardAlloc()) {
        return realloc(original, newSize);
    } else if (allocLock != NULL) {
        debugMonitorEnter(allocLock);
        ret = dl_realloc(original, newSize);
#ifdef JDWP_ALLOC_TRACE
        fprintf(stderr, "WPFREE %x REALLOC %d\n", original, --allocSum);
        fprintf(stderr, "WPALOC %x REALLOC %d %d\n", ret, 
                newSize, ++allocSum);
#endif
        debugMonitorExit(allocLock);
    } else {
        /*
         * We need to do some allocations before the allocLock is in place.
         * (The monitor itself requires allocations.) 
         */
        ret = dl_realloc(original, newSize);
    }
    return ret;
#else   /* JPDA_NO_DLALLOC */
    return realloc(original, newSize);
#endif  /* JPDA_NO_DLALLOC */
}

char *
jdwpStrdup(char *string)
{
    char *newString = jdwpAlloc(strlen(string)+1);
    if (newString != NULL) {
        strcpy(newString, string);
    }
    return newString;
}

void 
util_lock(void)
{
    debugMonitorEnter(allocLock);
}

void 
util_unlock(void)
{
    debugMonitorExit(allocLock);
}

void jdiAssertionFailed(char *fileName, int lineNumber, char *msg)
{
    fprintf(stderr, "ASSERT FAILED: %s : %d - %s\n",
            fileName, lineNumber, msg);
    if (assertFatal) {
        ERROR_MESSAGE_EXIT("Assertion Failed");
    }
}

/******************** COMMENTED OUT ********************
static void printThread(jthread thread, JVMDI_thread_info *info) {
    jint suspendStatus;
    jint hashCode;
    jint status = threadStatus(thread, &suspendStatus);
    jvmdi->GetObjectHashCode(thread, &hashCode);
    fprintf(stderr, "thread '%s' (hash=%d, status=%d, ss=%d)\n", info->name, hashCode, status, suspendStatus);
}

static void printMonitor(jobject monitor, JVMDI_monitor_info *info) {
    int i;
    jint hashCode;
    JNIEnv *env = getEnv();
    jclass clazz = (*env)->GetObjectClass(env, monitor);
    char *sig = classSignature(clazz);
    jvmdi->GetObjectHashCode(monitor, &hashCode);
    fprintf(stderr, "monitor instance of %s (hash=%d, entryct=%d, waiterct=%d)\n", sig, hashCode, info->entryCount, info->waiterCount);
    if (info->owner != NULL) {
        JVMDI_thread_info ti;
        threadInfo(info->owner, &ti);
        fprintf(stderr, "  owner: ");
        printThread(info->owner, &ti);
    }

    for (i=0; i<info->waiterCount; i++) {
        JVMDI_thread_info ti;
        threadInfo(info->waiters[i], &ti);
        fprintf(stderr, "  waiter %d: ", i+1);
        printThread(info->waiters[i], &ti);
    }
}

****************************/

