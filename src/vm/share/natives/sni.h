/*
 *   
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
 */

/**
 * SNI :== Synchronous (I/O) Native Interface.
 *
 * All native methods that use this interface are synchronous calls
 * with potentially blocking lightweight threads:
 *
 * Such threads appear to block for I/O at Java level,
 * but at VM implementation level they NEVER block.
 *
 * The respective native method always returns quickly.
 * The Java thread may get temporarily descheduled afterwards
 * and later thereupon the same native method gets reexecuted.
 */

#ifndef _JVM_CONFIG_H_
#include "jvmconfig.h"
#endif

#ifndef _SNI_H_
#define _SNI_H_

#ifndef _JAVASOFT_KNI_H_
#include "kni.h"
#endif

#ifndef _JVMSPI_H_
#include "jvmspi.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PRODUCT
extern int _jvm_in_raw_pointers_block;
#define SNI_BEGIN_RAW_POINTERS     { ++ _jvm_in_raw_pointers_block;
#define SNI_END_RAW_POINTERS         -- _jvm_in_raw_pointers_block; }
#else
#define SNI_BEGIN_RAW_POINTERS
#define SNI_END_RAW_POINTERS
#endif

/**
 * Notifies the JVM that the current Java thread is to be blocked
 * after return from the native function we are in.  This function
 * should be called only once and native method should return immediately
 * after its invocation, otherwise results will be unpredictable.
 *
 * Return value of native method is meaningless at this point, so
 * anything can be returned, if native method should return value.
 */
void SNI_BlockThread();

/**
 * For use by event checking code to notify the JVM that the specified
 * thread should run again and reenter the native function which
 * previously called SNI_BlockThread().
 *
 * Note: in SlaveMode, after calling SNI_UnblockThread, the event
 * checking code should arrange for JVM_TimeSlice() to be called as
 * soon as possible to resume Java execution.
 */
void SNI_UnblockThread(JVMSPI_ThreadID thread_id);

/**
 * ATTENTION: MAY CAUSE GC!!!
 * Best place this call "early" in your native function to avoid problems!
 *
 * Allocates the specified amount of memory and returns a pointer to it.
 * Returns NULL if not enough space available.
 *
 */
void *SNI_AllocateReentryData(int reentry_data_size);

/**
 * Return pointer and size of a memory block previously allocated by the above.
 * Pass 'NULL' as parameter if you are not interested in having it set to
 * the previously allocated size;
 */
void *SNI_GetReentryData(int *reentry_data_size);

/**
 * Return the blocked threads, if any.
 */
JVMSPI_BlockedThreadInfo *
SNI_GetBlockedThreads(int *number_of_blocked_threads);

/**
 *  Save the thread to the isolateIndex
 *
 */
void SNI_SetSpecialThread(int isolateIndex);

/**
 *  Get the thread from the isolateIndex
 *
 */
 JVMSPI_ThreadID SNI_GetSpecialThread(int isolateIndex);

/**
 * Direct access to the contents of a TypeArray.
 * Caller is responsible for not poking around outside the given object
 * on the heap and for ensuring that no GC happens and moves the object
 * before any subsequent access!!!
 */
void *SNI_GetRawArrayPointer(jarray array);

/**
 * Direct access to the object pointer at the given index on the parameter
 * stack.
 * The caller should be away of not holding on to this pointer across
 * GC points since the underlaying data may move.
 *
 * The index parameter has the same meaning as the index paramater to
 * KNI_GetParameterAsObject. The given parameter must be an object, or the
 * result is undetermined.
 */
void* SNI_GetParameterAsRawPointer(jint index);

/*
 * The order of SNI_BOOLEAN_ARRAY ~ SNI_INT_ARRAY are important, and
 * must match the order of Universe::bool_array_class ~
 * Universe::long_array_class
 */

#define SNI_BOOLEAN_ARRAY   4 /* 0x04  0b0100 */
#define SNI_CHAR_ARRAY      5 /* 0x05  0b0101 */
#define SNI_FLOAT_ARRAY     6 /* 0x06  0b0110 */
#define SNI_DOUBLE_ARRAY    7 /* 0x07  0b0111 */
#define SNI_BYTE_ARRAY      8 /* 0x08  0b1000 */
#define SNI_SHORT_ARRAY     9 /* 0x09  0b1001 */
#define SNI_INT_ARRAY      10 /* 0x0a  0b1010 */
#define SNI_LONG_ARRAY     11 /* 0x0b  0b1011 */
#define SNI_OBJECT_ARRAY    1
#define SNI_STRING_ARRAY    2

/**
 * Create an array of the given size. The elements contained in this
 * array is specified by the type parameter, which must be one of
 * the SNI_XXX_ARRAY type specified above. For example, the calls:
 *
 *     SNI_NewArray(SNI_BOOLEAN_ARRAY, 10, handle1);
 *     SNI_NewArray(SNI_OBJECT_ARRAY,  20, handle2);
 *     SNI_NewArray(SNI_STRING_ARRAY,  30, handle3);
 *
 * are equivalent to the Java code:
 *
 *     array1 = new boolean[10];
 *     array2 = new Object[10];
 *     array3 = new String[10];
 *
 * Note that SNI_OBJECT_ARRAY and SNI_STRING_ARRAY are provided for
 * the creation these two most comply used object arrays. For other
 * object array types, please use KNI_NewObjectArray().
 */
KNIEXPORT void SNI_NewArray(jint type, jint size, jarray arrayHandle);

/**
 * Create an object array of the given size. The elements contained in this
 * array is by the elementType variable. For example, the following code:
 *
 *     KNI_StartHandles(2);
 *     KNI_DeclareHandle(fooClass);
 *     KNI_DeclareHandle(fooArray);
 *
 *     KNI_FindClass("Foo", fooClass);
 *     SNI_NewObjectArray(fooClass, 10, fooArray);
 *     KNI_EndHandlesAndReturnObject(fooArray);
 *
 * Is equivalent to the Java code:
 *
 *     fooArray = new Foo[10];
 */
KNIEXPORT void SNI_NewObjectArray(jclass elementType, jint size,
                                  jarray arrayHandle);

/*
 * Creates a reference ID to the given jobject. The reference ID is a
 * jint, which may be used as input to SNI_GetReference() to retrieve
 * jobject.
 *
 * Returns a non-negative number if successful; in this case, the
 * returned reference ID will remain valid until SNI_DeleteReference()
 * is called.
 *
 *   + If a reference is added via SNI_AddWeakReference, the referenced
 *     object may be garbage collected, in which case a future call to
 *     SNI_GetReference() would return NULL.
 *
 *   + If the reference is added via SNI_AddStrongReference(), before
 *     SNI_DeleteReference() is called on this reference, the
 *     referenced object is not eligible for garbage collection, and thus
 *     SNI_GetReference() will always return the reference object.
 *
 * When using SNI_AddWeakReference() and SNI_AddStrongReference(), be
 * very careful to call SNI_DeleteReference() when you no longer use
 * this reference (e.g., inside a finalize() method). Otherwise memory
 * leak may result.
 *
 * Return -1 if failed (due to lack of memory).
 */

KNIEXPORT jint SNI_AddReference(jobject objectHandle, jboolean isStrong);

#define SNI_AddWeakReference(objectHandle) \
        SNI_AddReference(objectHandle, KNI_FALSE)
#define SNI_AddStrongReference(objectHandle) \
        SNI_AddReference(objectHandle, KNI_TRUE)

/*
 * Returns the jobject corresponding to the given reference <ref>.
 *
 * If the reference was created with SNI_AddWeakReference, the object
 * may be garbage collected, in which case a NULL is returned in the
 * objectHandle.
 */
KNIEXPORT void SNI_GetReference(jint ref, jobject objectHandle);

#define SNI_GetWeakReference(jref, objectHandle) \
        SNI_GetReference(jref, objectHandle)

/*
 * Removes the given reference <ref>. After this function returns, <ref>
 * is no longer valid. Attempts to use an invalid <ref> in
 * SNI_GetReference() yields undefined results.
 */
KNIEXPORT void SNI_DeleteReference(jint ref);

#define SNI_DeleteWeakReference(ref) \
        SNI_DeleteReference(ref)

/*
 * Releases a permit, returning it to the semaphore. If any
 * threads are blocking trying to acquire a permit, then one is
 * selected and given the permit that was just released. That
 * thread is re-enabled for thread scheduling purposes.
 *
 * See src/javaapi/share/com/sun/cldc/util.Semaphore.java for details.
 */
KNIEXPORT void SNI_ReleaseSemaphore(jobject semaphore);

#ifdef __cplusplus
}
#endif

#endif
