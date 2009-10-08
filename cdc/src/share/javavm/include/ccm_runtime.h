/*
 * @(#)ccm_runtime.h	1.49 06/10/10
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

#ifndef _INCLUDED_CCM_RUNTIME_H
#define _INCLUDED_CCM_RUNTIME_H

#include "javavm/include/defs.h"
#include "javavm/include/basictypes.h"

/* Types of CCM runtime helper functions: 
   =====================================
   All CCM runtime helper functions can be categorized as follows:

    1. STATE_FLUSHED:
        The thread's state has been adequatedly flushed and this helper
        function can become GC safe without any complication.  If this function
        needs to throw an exception, it may call CVMCCMhandleException() to
        directly unwind the stack and handle the exception.

    2. STATE_FLUSHED_TOTALLY:
        Same as STATE_FLUSHED with the addition that the Java stack pointer
        needs to be flushed to the top most frame.  This is not necessary for
        GC safety because the frame's PC (which is flushed) will get us to the
        appropriate stackmap which will tell us about references in the stack
        frame independent of the topOfStack pointer.  However, we need to flush
        the topOfStack pointer for any helpers that may push a frame (usually
        for the purpose of calling another Java method) on to the Java stack.

    3. STATE_NOT_FLUSHED:
        The thread's state has not been adequatedly flushed.  Hence, this
        helper function is not allowed to become GC safe.  If this function
        needs to throw an exception, it will have to call
        CVMCCMreturnWithException() which stores away the needed info to throw
        the exception, set a condition flag, and return to the compiled code.
        The compiled code is responsible for checking the condition flag.  If
        the condition flag indicates that an exception was detected, then the
        compiled code should flush its state (in preparation of becoming GC
        safe), and call CVMCCMruntimeThrowClass() to throw the exception that
        occurred.

        STATE_NOT_FLUSHED helper functions can be further sub-categorized as
        follows:
            1. THROWS_NONE: The helper does not any exceptions.
            2. THROWS_SINGLE: The helper can throw only one type of exception.
                    Hence the type of the actual exception thrown is known at
                    method compile time.
            3. THROWS_MULTIPLE: The helper can throw more than one type of
                    exception.  Hence, the type of the actual exception thrown
                    is NOT known at method compile time.
*/

/* Purpose: Computes value1 / value2. */
/* Type: STATE_NOT_FLUSHED THROWS_SINGLE */
/* Throws: ArithmeticException("/ by zero"). */
CVMJavaInt CVMCCMruntimeIDiv(CVMJavaInt value1, CVMJavaInt value2);

/* Purpose: Computes value1 % value2. */
/* Type: STATE_NOT_FLUSHED THROWS_SINGLE */
/* Throws: ArithmeticException("/ by zero"). */
CVMJavaInt CVMCCMruntimeIRem(CVMJavaInt value1, CVMJavaInt value2);

/* Purpose: Computes value1 * value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeLMul(CVMJavaLong value1, CVMJavaLong value2);

/* Purpose: Computes value1 / value2. */
/* Type: STATE_NOT_FLUSHED THROWS_SINGLE */
/* Throws: ArithmeticException("/ by zero"). */
CVMJavaLong CVMCCMruntimeLDiv(CVMJavaLong value1, CVMJavaLong value2);

/* Purpose: Computes value1 % value2. */
/* Type: STATE_NOT_FLUSHED THROWS_SINGLE */
/* Throws: ArithmeticException("/ by zero"). */
CVMJavaLong CVMCCMruntimeLRem(CVMJavaLong value1, CVMJavaLong value2);

/* Purpose: Computes -value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeLNeg(CVMJavaLong value);

/* Purpose: Computes value1 << value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeLShl(CVMJavaLong value1, CVMJavaInt value2);

/* Purpose: Computes value1 >> value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeLShr(CVMJavaLong value1, CVMJavaInt value2);

/* Purpose: Computes value1 >>> value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeLUshr(CVMJavaLong value1, CVMJavaInt value2);

/* Purpose: Computes value1 & value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeLAnd(CVMJavaLong value1, CVMJavaLong value2);

/* Purpose: Computes value1 | value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeLOr(CVMJavaLong value1, CVMJavaLong value2);

/* Purpose: Computes value1 ^ value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeLXor(CVMJavaLong value1, CVMJavaLong value2);

/* Purpose: Computes value1 + value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMUint32 CVMCCMruntimeFAdd(CVMUint32 value1, CVMUint32 value2);

/* Purpose: Computes value1 - value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMUint32 CVMCCMruntimeFSub(CVMUint32 value1, CVMUint32 value2);

/* Purpose: Computes value1 * value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMUint32 CVMCCMruntimeFMul(CVMUint32 value1, CVMUint32 value2);

/* Purpose: Computes value1 / value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMUint32 CVMCCMruntimeFDiv(CVMUint32 value1, CVMUint32 value2);

/* Purpose: Computes value1 % value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMUint32 CVMCCMruntimeFRem(CVMUint32 value1, CVMUint32 value2);

/* Purpose: Computes -value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMUint32 CVMCCMruntimeFNeg(CVMUint32 value);

/* Purpose: Computes value1 + value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeDAdd(CVMJavaLong value1, CVMJavaLong value2);

/* Purpose: Computes value1 - value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeDSub(CVMJavaLong value1, CVMJavaLong value2);

/* Purpose: Computes value1 * value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeDMul(CVMJavaLong value1, CVMJavaLong value2);

/* Purpose: Computes value1 / value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeDDiv(CVMJavaLong value1, CVMJavaLong value2);

/* Purpose: Computes value1 % value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeDRem(CVMJavaLong value1, CVMJavaLong value2);

/* Purpose: Computes -value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeDNeg(CVMJavaLong value);

/* Purpose: Converts an int value to a long value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeI2L(CVMJavaInt value);

/* Purpose: Converts an int value to a float value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMUint32 CVMCCMruntimeI2F(CVMJavaInt value);

/* Purpose: Converts an int value to a double value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeI2D(CVMJavaInt value);

/* Purpose: Converts a long value to an int value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaInt CVMCCMruntimeL2I(CVMJavaLong value);

/* Purpose: Converts a long value to a float value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMUint32 CVMCCMruntimeL2F(CVMJavaLong value);

/* Purpose: Converts a long value to a double value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeL2D(CVMJavaLong value);

/* Purpose: Converts a float value to an int value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaInt CVMCCMruntimeF2I(CVMUint32 value);

/* Purpose: Converts a float value to a long value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeF2L(CVMUint32 value);

/* Purpose: Converts a float value to a double value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeF2D(CVMUint32 value);

/* Purpose: Converts a double value to an int value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaInt CVMCCMruntimeD2I(CVMJavaLong value);

/* Purpose: Converts a double value to a long value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeD2L(CVMJavaLong value);

/* Purpose: Converts a double value to a float value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMUint32 CVMCCMruntimeD2F(CVMJavaLong value);

/* Purpose: Compares two long values. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaInt CVMCCMruntimeLCmp(CVMJavaLong value1, CVMJavaLong value2);

/* Purpose: Compares value1 with value2. */
/* Returns: -1 if value1 < value2,
             0 if value1 == value2, 
             1 if value1 > value2, and
     nanResult if (value1 == NaN) or (value2 == Nan) */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaInt CVMCCMruntimeFCmp(CVMUint32 value1, CVMUint32 value2,
                             CVMJavaInt nanResult);

/* Purpose: Compares value1 with value2 (NaN yields a "less than" result). */
/* Returns: -1 if value1 < value2,
             0 if value1 == value2, 
             1 if value1 > value2, and
            -1 if (value1 == NaN) or (value2 == Nan)
               i.e. NaN yields a "less than" result. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaInt CVMCCMruntimeDCmpl(CVMJavaLong value1, CVMJavaLong value2);

/* Purpose: Compares value1 with value2 (NaN yields a "greater than" result).*/
/* Returns: -1 if value1 < value2,
             0 if value1 == value2, 
             1 if value1 > value2, and
             1 if (value1 == NaN) or (value2 == Nan)
               i.e. NaN yields a "greater than" result. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaInt CVMCCMruntimeDCmpg(CVMJavaLong value1, CVMJavaLong value2);

/* Purpose: Throws the specified exception. */
/* Type: STATE_FLUSHED THROWS_MULTIPLE */
void CVMCCMruntimeThrowClass(CVMCCExecEnv *ccee, CVMExecEnv *ee,
			     CVMClassBlock *exceptionCb,
			     const char *exceptionString);

/* Purpose: Throws the specified exception. */
/* Type: STATE_FLUSHED THROWS_MULTIPLE */
void CVMCCMruntimeThrowObject(CVMCCExecEnv *ccee, CVMExecEnv *ee,
			      CVMObject *obj);

/* Purpose: Checks to see if the specified object is can be legally casted as
            an instance of the specified class. */
/* Type: STATE_FLUSHED THROWS_MULTIPLE */
/* Throws: ClassCastException, StackOverflowError. */
/* NOTE: The obj is checked for NULL in the helper function.  The JITed
         code need not check for NULL first. */
void CVMCCMruntimeCheckCast(CVMCCExecEnv *ccee, CVMClassBlock *objCb,
                            CVMClassBlock *castCb,
                            CVMClassBlock **cachedCbPtr);

/* Purpose: Checks to see if the an element of type 'rhsCb' can be stored
   into an array whose element type is 'elemCb'. */
/* Type: STATE_FLUSHED THROWS_MULTIPLE */
/* Throws: ArrayStoreException, StackOverflowError. */
/* NOTE: The obj is checked for NULL inline */
void CVMCCMruntimeCheckArrayAssignable(CVMCCExecEnv *ccee, CVMExecEnv *ee,
				       CVMClassBlock *elemCb,
				       CVMClassBlock *rhsCb);

/* Purpose: Checks to see if the specified object is an instance of the
            specified class or its subclasses. */
/* Type: STATE_FLUSHED THROWS_SINGLE */
/* Throws: StackOverflowError. */
/* NOTE: The obj is checked for NULL in the helper function.  The JITed
         code need not check for NULL first. */
CVMJavaInt CVMCCMruntimeInstanceOf(CVMCCExecEnv *ccee, CVMClassBlock *objCb,
                                   CVMClassBlock *instanceofCb,
                                   CVMClassBlock **cachedCbPtr);

/* Purpose: Looks up the target MethodBlock which implements the specified
            interface method. */
/* Type: STATE_FLUSHED THROWS_SINGLE */
/* Throws: IncompatibleClassChangeError. */
/* NOTE: The caller should check if the object is NULL first. */
const CVMMethodBlock *
CVMCCMruntimeLookupInterfaceMB(CVMCCExecEnv *ccee, CVMClassBlock *ocb,
                               const CVMMethodBlock *imb, CVMInt32 *guessPtr);

/* Purpose: Instantiates an object of the specified class. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_SINGLE */
/* Throws: OutOfMemoryError. */
CVMObject *
CVMCCMruntimeNew(CVMCCExecEnv *ccee, CVMClassBlock *newCb);

/* Purpose: Instantiates a primitive array of the specified type and size. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: OutOfMemoryError, NegativeArraySizeException. */
CVMObject *
CVMCCMruntimeNewArray(CVMCCExecEnv *ccee, CVMJavaInt length,
                      CVMClassBlock *arrCB);

/* Purpose: Instantiates an object array of the specified type and size. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: OutOfMemoryError, NegativeArraySizeException, StackOverflowError. */
CVMObject *
CVMCCMruntimeANewArray(CVMCCExecEnv *ccee, CVMJavaInt length,
		       CVMClassBlock *arrayCb);

/* Purpose: Instantiates a multi dimensional object array of the specified
            dimensions. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: OutOfMemoryError, NegativeArraySizeException. */
/* 
 * CVMmultiArrayAlloc() is called from java opcode multianewarray
 * and passes the top of stack as parameter dimensions.
 * Because the width of the array dimensions is obtained via 
 * dimensions[i], dimensions has to be of the same type as 
 * the stack elements for proper access.
 */
CVMObject *
CVMCCMruntimeMultiANewArray(CVMCCExecEnv *ccee, CVMJavaInt nDimensions,
                            CVMClassBlock *arrCb, CVMStackVal32 *dimensions);

/* Purpose: Runs the static initializer for the specified class if
            necessary. */
/* Result:  -Returns true if the class has been intialized.
            -Returns false if class is being intialized by current
	     thread.
	    -Otherwise attempts to intialize the class and will return
	     to the address in the current frame when intialization
	     is complete. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_??? */
CVMBool
CVMCCMruntimeRunClassInitializer(CVMCCExecEnv *ccee, CVMExecEnv *ee,
				 CVMClassBlock *cb);

/* Purpose: Resolves the specified class block constant pool entry. */
/* Result:  Always returns TRUE since no class initialization is needed. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, etc. */
CVMBool
CVMCCMruntimeResolveClassBlock(CVMCCExecEnv *ccee, CVMExecEnv *ee,
                               CVMUint16 cpIndex,
                               CVMClassBlock **cachedConstant);

/* Purpose: Resolves the specified array class block constant pool entry. */
/* Result:  Always returns TRUE since no class initialization is needed. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, etc. */
CVMBool
CVMCCMruntimeResolveArrayClassBlock(CVMCCExecEnv *ccee, CVMExecEnv *ee,
				    CVMUint16 cpIndex,
				    CVMClassBlock **cachedConstant);

/* Purpose: Resolves the specified field block constant pool entry into a
            field offset, and checks if the filed has changed into a static
            field. */
/* Result:  Always returns TRUE since no class initialization is needed. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, IncompatibleClassChangeError, NoSuchFieldError,
           etc. */
CVMBool
CVMCCMruntimeResolveGetfieldFieldOffset(CVMCCExecEnv *ccee,
                                        CVMExecEnv *ee,
                                        CVMUint16 cpIndex,
                                        CVMUint32 *cachedConstant);

/* Purpose: Resolves the specified field block constant pool entry into a
            field offset, checks if the filed has changed into a static field,
            and checks if the field is final (i.e. not writable to). */
/* Result:  Always returns TRUE since no class initialization is needed. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, IncompatibleClassChangeError, NoSuchFieldError,
           etc. */
CVMBool
CVMCCMruntimeResolvePutfieldFieldOffset(CVMCCExecEnv *ccee,
                                        CVMExecEnv *ee,
                                        CVMUint16 cpIndex,
                                        CVMUint32 *cachedConstant);

/* Purpose: Resolves the specified method block constant pool entry. */
/* Result:  Always returns TRUE since no class initialization is needed. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, NoSuchMethodError, etc. */
CVMBool
CVMCCMruntimeResolveMethodBlock(CVMCCExecEnv *ccee, CVMExecEnv *ee,
                                CVMUint16 cpIndex,
                                CVMMethodBlock **cachedConstant);

/* Purpose: Resolves the specified method block constant pool entry for
            invokespecial, and checks if the method has changed into a static
            method. */
/* Result:  Always returns TRUE since no class initialization is needed. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, IncompatibleClassChangeError, NoSuchMethodError,
           etc. */
CVMBool
CVMCCMruntimeResolveSpecialMethodBlock(CVMCCExecEnv *ccee, CVMExecEnv *ee,
                                       CVMUint16 cpIndex,
                                       CVMMethodBlock **cachedConstant);

/* Purpose: Resolves the specified method block constant pool entry into a
            method table offset for invokevirtual, and checks if the method has
            changed into a static method. */
/* Result:  NULL if vtable offset stored in cached word.
 *          else mb pointer if the mb is not in a vtable. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, IncompatibleClassChangeError, NoSuchMethodError,
           etc. */
CVMMethodBlock *
CVMCCMruntimeResolveMethodTableOffset(CVMCCExecEnv *ccee, CVMExecEnv *ee,
                                      CVMUint16 cpIndex,
                                      CVMUint32 *cachedConstant);

/* Purpose: Resolves the specified class block constant pool entry,
            checks if it is Ok to instantiate an instance of this class,
            and runs the static initializer if necessary. */
/* Result:  Same as CVMCCMruntimeRunClassInitializer. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, InstantiationError, etc. */
CVMBool
CVMCCMruntimeResolveNewClassBlockAndClinit(CVMCCExecEnv *ccee,
                                           CVMExecEnv *ee,
                                           CVMUint16 cpIndex,
                                           CVMClassBlock **cachedConstant);

/* Purpose: Resolves the specified static field constant pool entry,
            checks if the field has changed into a non-static field, and
            runs the static initializer if necessary. */
/* Result:  Same as CVMCCMruntimeRunClassInitializer. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, IncompatibleClassChangeError, NoSuchFieldError,
           etc. */
CVMBool
CVMCCMruntimeResolveGetstaticFieldBlockAndClinit(CVMCCExecEnv *ccee,
                                                 CVMExecEnv *ee,
                                                 CVMUint16 cpIndex,
                                                 void **cachedConstant);

/* Purpose: Resolves the specified static field constant pool entry,
            checks if the field has changed into a non-static field,
            checks if the field is final (i.e. not writable to), and
            runs the static initializer if necessary. */
/* Result:  Same as CVMCCMruntimeRunClassInitializer. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, IncompatibleClassChangeError, NoSuchFieldError,
           etc. */
CVMBool
CVMCCMruntimeResolvePutstaticFieldBlockAndClinit(CVMCCExecEnv *ccee,
                                                 CVMExecEnv *ee,
                                                 CVMUint16 cpIndex,
                                                 void **cachedConstant);

/* Purpose: Resolves the specified static method block constant pool entry,
            and runs the static initializer if necessary. */
/* Result:  Same as CVMCCMruntimeRunClassInitializer. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, IncompatibleClassChangeError, NoSuchMethodError,
           etc. */
CVMBool
CVMCCMruntimeResolveStaticMethodBlockAndClinit(CVMCCExecEnv *ccee,
                                               CVMExecEnv *ee,
                                               CVMUint16 cpIndex,
                                               CVMMethodBlock **cachedConstant);

/* Purpose: Performs an atomic 64-bit putstatic. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
void CVMCCMruntimePutstatic64Volatile(CVMJavaLong value, void *staticField);

/* Purpose: Performs an atomic 64-bit getstatic. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeGetstatic64Volatile(void *staticField);

/* Purpose: Performs an atomic 64-bit putfield. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
void CVMCCMruntimePutfield64Volatile(CVMJavaLong value,
				     CVMObject *obj,
				     CVMJavaInt fieldByteOffset);

/* Purpose: Performs an atomic 64-bit getfield. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeGetfield64Volatile(CVMObject *obj,
					    CVMJavaInt fieldByteOffset);

/* Purpose: Perform monitor enter. */
/* Type: STATE_FLUSHED THROWS_MULTIPLE */
void
CVMCCMruntimeMonitorEnter(CVMCCExecEnv *ccee, CVMExecEnv *ee, CVMObject *obj);

/* Purpose: Perform monitor exit. */
/* Type: STATE_FLUSHED THROWS_MULTIPLE */
void
CVMCCMruntimeMonitorExit(CVMCCExecEnv *ccee, CVMExecEnv *ee, CVMObject *obj);

/* Purpose: Rendezvous with GC thread. */
/* Type: STATE_FLUSHED THROWS_NONE */
void
CVMCCMruntimeGCRendezvous(CVMCCExecEnv *ccee, CVMExecEnv *ee);

#if defined(CVMJIT_SIMPLE_SYNC_METHODS) && \
    (CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS)
/* 
 * Simple Sync helper function for unlocking in Simple Sync methods when
 * there is contention for the lock after locking. It is only needed for
 * CVM_FASTLOCK_ATOMICOPS since once locked, CVM_FASTLOCK_MICROLOCK
 * will never allow for contention of Simple Sync methods.
 */
extern void
CVMCCMruntimeSimpleSyncUnlock(CVMExecEnv *ee, CVMObject* obj);
#endif /* CVMJIT_SIMPLE_SYNC_METHODS */

#ifdef CVM_CCM_COLLECT_STATS

/*============================================================================
// The following are used to collect statistics about the runtime activity of
// compiled code:
*/

typedef enum CVMCCMStatsTag {
    CVMCCM_STATS_FIRST_CCMRUNTIME_STATS,
    CVMCCM_STATS_CVMCCMruntimeIDiv = CVMCCM_STATS_FIRST_CCMRUNTIME_STATS,
    CVMCCM_STATS_CVMCCMruntimeIRem,
    CVMCCM_STATS_CVMCCMruntimeLMul,
    CVMCCM_STATS_CVMCCMruntimeLDiv,
    CVMCCM_STATS_CVMCCMruntimeLRem,
    CVMCCM_STATS_CVMCCMruntimeLNeg,
    CVMCCM_STATS_CVMCCMruntimeLShl,
    CVMCCM_STATS_CVMCCMruntimeLShr,
    CVMCCM_STATS_CVMCCMruntimeLUshr,
    CVMCCM_STATS_CVMCCMruntimeLAnd,
    CVMCCM_STATS_CVMCCMruntimeLOr,
    CVMCCM_STATS_CVMCCMruntimeLXor,
    CVMCCM_STATS_CVMCCMruntimeFAdd,
    CVMCCM_STATS_CVMCCMruntimeFSub,
    CVMCCM_STATS_CVMCCMruntimeFMul,
    CVMCCM_STATS_CVMCCMruntimeFDiv,
    CVMCCM_STATS_CVMCCMruntimeFRem,
    CVMCCM_STATS_CVMCCMruntimeFNeg,
    CVMCCM_STATS_CVMCCMruntimeDAdd,
    CVMCCM_STATS_CVMCCMruntimeDSub,
    CVMCCM_STATS_CVMCCMruntimeDMul,
    CVMCCM_STATS_CVMCCMruntimeDDiv,
    CVMCCM_STATS_CVMCCMruntimeDRem,
    CVMCCM_STATS_CVMCCMruntimeDNeg,
    CVMCCM_STATS_CVMCCMruntimeI2L,
    CVMCCM_STATS_CVMCCMruntimeI2F,
    CVMCCM_STATS_CVMCCMruntimeI2D,
    CVMCCM_STATS_CVMCCMruntimeL2I,
    CVMCCM_STATS_CVMCCMruntimeL2F,
    CVMCCM_STATS_CVMCCMruntimeL2D,
    CVMCCM_STATS_CVMCCMruntimeF2I,
    CVMCCM_STATS_CVMCCMruntimeF2L,
    CVMCCM_STATS_CVMCCMruntimeF2D,
    CVMCCM_STATS_CVMCCMruntimeD2I,
    CVMCCM_STATS_CVMCCMruntimeD2L,
    CVMCCM_STATS_CVMCCMruntimeD2F,
    CVMCCM_STATS_CVMCCMruntimeLCmp,
    CVMCCM_STATS_CVMCCMruntimeFCmp,
    CVMCCM_STATS_CVMCCMruntimeDCmpl,
    CVMCCM_STATS_CVMCCMruntimeDCmpg,
    CVMCCM_STATS_CVMCCMruntimeThrowClass,
    CVMCCM_STATS_CVMCCMruntimeThrowObject,
    CVMCCM_STATS_CVMCCMruntimeCheckCast,
    CVMCCM_STATS_CVMCCMruntimeCheckArrayAssignable,
    CVMCCM_STATS_CVMCCMruntimeInstanceOf,
    CVMCCM_STATS_CVMCCMruntimeLookupInterfaceMB,
    CVMCCM_STATS_CVMCCMruntimeNew,
    CVMCCM_STATS_CVMCCMruntimeNewArray,
    CVMCCM_STATS_CVMCCMruntimeANewArray,
    CVMCCM_STATS_CVMCCMruntimeMultiANewArray,
    CVMCCM_STATS_CVMCCMruntimeRunClassInitializer,
    CVMCCM_STATS_CVMCCMruntimeResolveClassBlock,
    CVMCCM_STATS_CVMCCMruntimeResolveArrayClassBlock,
    CVMCCM_STATS_CVMCCMruntimeResolveGetfieldFieldOffset,
    CVMCCM_STATS_CVMCCMruntimeResolvePutfieldFieldOffset,
    CVMCCM_STATS_CVMCCMruntimeResolveMethodBlock,
    CVMCCM_STATS_CVMCCMruntimeResolveSpecialMethodBlock,
    CVMCCM_STATS_CVMCCMruntimeResolveMethodTableOffset,
    CVMCCM_STATS_CVMCCMruntimeResolveNewClassBlockAndClinit,
    CVMCCM_STATS_CVMCCMruntimeResolveGetstaticFieldBlockAndClinit,
    CVMCCM_STATS_CVMCCMruntimeResolvePutstaticFieldBlockAndClinit,
    CVMCCM_STATS_CVMCCMruntimeResolveStaticMethodBlockAndClinit,
    CVMCCM_STATS_CVMCCMruntimePutstatic64Volatile,
    CVMCCM_STATS_CVMCCMruntimeGetstatic64Volatile,
    CVMCCM_STATS_CVMCCMruntimePutfield64Volatile,
    CVMCCM_STATS_CVMCCMruntimeGetfield64Volatile,
    CVMCCM_STATS_CVMCCMruntimeMonitorEnter,
    CVMCCM_STATS_CVMCCMruntimeMonitorExit,
    CVMCCM_STATS_CVMCCMruntimeGCRendezvous,
#if defined(CVMJIT_SIMPLE_SYNC_METHODS) && \
    (CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS)
    CVMCCM_STATS_CVMCCMruntimeSimpleSyncUnlock,
#endif

    /* NOTE: CVMCCM_STATS_NUM_TAGS must be last in this enum list. */
    CVMCCM_STATS_NUM_TAGS                      /* Number of stats tags */
} CVMCCMStatsTag;

/* NOTE: CVMCCMGlobalStats encapsulates the global stats that will be
         collected.
*/
typedef struct CVMCCMGlobalStats CVMCCMGlobalStats;
struct CVMCCMGlobalStats
{
    CVMBool okToCollectStats;
    CVMInt32 stats[CVMCCM_STATS_NUM_TAGS];
};

/* Purpose: Increments the specified CCM stat. */
extern void 
CVMCCMstatsInc(CVMExecEnv *ee, CVMCCMStatsTag tag);

/* Purpose: Dumps the collected CCM stats. */
extern void
CVMCCMstatsDumpStats();

#else

#define CVMCCMstatsInc(ee, tag)
#define CVMCCMstatsDumpStats()

#endif /* CVM_CCM_COLLECT_STATS */


#endif /* _INCLUDED_CCM_RUNTIME_H */
