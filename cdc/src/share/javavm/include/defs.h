/*
 * @(#)defs.h	1.93 06/10/10
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

#ifndef _INCLUDED_DEFS_H
#define _INCLUDED_DEFS_H

#include "javavm/include/porting/defs.h"

#ifndef _ASM
#include "javavm/include/porting/ansi/stddef.h"
#include "javavm/include/porting/vm-defs.h"

/*
 * Basic type definitions
 */

typedef char      CVMUtf8;

/* 
 * Casts from pointer types to scalar types have to
 * be casts to the type CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms.
 */
#define CVMoffsetof(T, f)    (CVMAddr)(&(((T *)0)->f))

/*
 * Object format
 */
CVM_STRUCT_TYPEDEF(CVMObjectHeader);
CVM_UNION_TYPEDEF (CVMVariousBits);
CVM_UNION_TYPEDEF (CVMJavaVal8);
CVM_UNION_TYPEDEF (CVMJavaVal16);
CVM_UNION_TYPEDEF (CVMJavaVal32);
CVM_UNION_TYPEDEF (CVMJavaVal64);
CVM_STRUCT_TYPEDEF(CVMjava_lang_Object);
CVM_STRUCT_TYPEDEF(CVMSyncVector);

typedef CVMjava_lang_Object CVMObject;  /* Short-hand for CVMjava_lang_Object */

/*
 * Array types
 */
CVM_STRUCT_TYPEDEF(CVMArrayOfByte);	/* CVMArrayOfByte    */
CVM_STRUCT_TYPEDEF(CVMArrayOfShort);	/* CVMArrayOfShort   */
CVM_STRUCT_TYPEDEF(CVMArrayOfChar);	/* CVMArrayOfChar    */
CVM_STRUCT_TYPEDEF(CVMArrayOfBoolean);	/* CVMArrayOfBoolean */
CVM_STRUCT_TYPEDEF(CVMArrayOfInt);	/* CVMArrayOfInt     */
CVM_STRUCT_TYPEDEF(CVMArrayOfRef);	/* CVMArrayOfRef     */
CVM_STRUCT_TYPEDEF(CVMArrayOfFloat);	/* CVMArrayOfFloat   */
CVM_STRUCT_TYPEDEF(CVMArrayOfLong);	/* CVMArrayOfLong    */
CVM_STRUCT_TYPEDEF(CVMArrayOfDouble);	/* CVMArrayOfDouble  */
CVM_STRUCT_TYPEDEF(CVMArrayOfAnyType);	/* CVMArrayOfAnyType */

/*
 * ICell types
 */
CVM_STRUCT_TYPEDEF(CVMjava_lang_ObjectICell);	/* CVMjava_lang_ObjectICell */
CVM_STRUCT_TYPEDEF(CVMObjectICell);		/* ObjectICell         */

CVM_STRUCT_TYPEDEF(CVMArrayOfByteICell);	/* CVMArrayOfByteICell    */
CVM_STRUCT_TYPEDEF(CVMArrayOfShortICell);	/* CVMArrayOfShortICell   */
CVM_STRUCT_TYPEDEF(CVMArrayOfCharICell);	/* CVMArrayOfCharICell    */
CVM_STRUCT_TYPEDEF(CVMArrayOfBooleanICell);	/* CVMArrayOfBooleanICell */
CVM_STRUCT_TYPEDEF(CVMArrayOfIntICell);		/* CVMArrayOfIntICell     */
CVM_STRUCT_TYPEDEF(CVMArrayOfRefICell);		/* CVMArrayOfRefICell     */
CVM_STRUCT_TYPEDEF(CVMArrayOfFloatICell);	/* CVMArrayOfFloatICell   */
CVM_STRUCT_TYPEDEF(CVMArrayOfLongICell);	/* CVMArrayOfLongICell    */
CVM_STRUCT_TYPEDEF(CVMArrayOfDoubleICell);	/* CVMArrayOfDoubleICell  */
CVM_STRUCT_TYPEDEF(CVMArrayOfAnyTypeICell);	/* CVMArrayOfAnyTypeICell */

/*
 * Class data structures
 */
CVM_STRUCT_TYPEDEF(CVMClassBlock);
CVM_UNION_TYPEDEF (CVMGCBitMap);
CVM_STRUCT_TYPEDEF(CVMBigGCBitMap);
CVM_STRUCT_TYPEDEF(CVMArrayInfo);
CVM_STRUCT_TYPEDEF(CVMInterfaces);
CVM_STRUCT_TYPEDEF(CVMInterfaceTable);
CVM_STRUCT_TYPEDEF(CVMMethodBlock);
CVM_STRUCT_TYPEDEF(CVMMethodBlockImmutable);
CVM_STRUCT_TYPEDEF(CVMMethodRange);
CVM_STRUCT_TYPEDEF(CVMMethodArray);
CVM_STRUCT_TYPEDEF(CVMCheckedExceptions);
CVM_STRUCT_TYPEDEF(CVMJavaMethodDescriptor);
#ifdef CVM_JIT
CVM_STRUCT_TYPEDEF(CVMCompiledMethodDescriptor);
CVM_STRUCT_TYPEDEF(CVMCompiledPcMapTable);
CVM_STRUCT_TYPEDEF(CVMCompiledPcMap);
CVM_STRUCT_TYPEDEF(CVMCompiledGCCheckPCs);
CVM_STRUCT_TYPEDEF(CVMCompiledStackMaps);
CVM_STRUCT_TYPEDEF(CVMCompiledStackMapEntry);
CVM_STRUCT_TYPEDEF(CVMCompiledStackMapLargeEntry);
CVM_STRUCT_TYPEDEF(CVMCompiledInliningInfo);
CVM_STRUCT_TYPEDEF(CVMCompiledInliningInfoEntry);
#endif
CVM_STRUCT_TYPEDEF(CVMExceptionHandler);
CVM_STRUCT_TYPEDEF(CVMLineNumberEntry);
CVM_STRUCT_TYPEDEF(CVMLocalVariableEntry);
CVM_STRUCT_TYPEDEF(CVMFieldBlock);
CVM_STRUCT_TYPEDEF(CVMFieldRange);
CVM_STRUCT_TYPEDEF(CVMFieldArray);
CVM_STRUCT_TYPEDEF(CVMInnerClassInfo);
CVM_STRUCT_TYPEDEF(CVMInnerClassesInfo);
CVM_UNION_TYPEDEF(CVMConstantPool);
CVM_STRUCT_TYPEDEF(CVMTransitionConstantPool);
CVM_UNION_TYPEDEF (CVMConstantPoolEntry);
CVM_STRUCT_TYPEDEF(CVMStackMapEntry);
CVM_STRUCT_TYPEDEF(CVMStackMaps);
#ifdef CVM_DUAL_STACK
CVM_STRUCT_TYPEDEF(CVMClassRestrictions);
#endif

/*
 * Classes which the VM uses directly
 */
typedef CVMObjectICell CVMThreadICell;		/* java.lang.Thread */
typedef CVMObjectICell CVMThrowableICell;	/* java.lang.Throwable */
typedef CVMObjectICell CVMStringICell;		/* java.lang.String */
typedef CVMObjectICell CVMClassICell;		/* java.lang.Class */
typedef CVMObjectICell CVMClassLoaderICell;	/* java.lang.ClassLoader */
typedef CVMObject*     CVMStringObject;         /* used for romized Strings */

/*
 * JIT runtime support
 */
#ifdef CVM_JIT
CVM_STRUCT_TYPEDEF(CVMCCExecEnv);
#endif

/*
 * Interpreter data structures
 */
CVM_STRUCT_TYPEDEF(CVMExecEnv);

/*
 * Synchronization-related types.
 */

typedef CVMBool CVMTryLockFunc     (CVMExecEnv*     ee,
				    CVMObject*      obj);
typedef CVMBool CVMLockFunc        (CVMExecEnv*     ee,
				    CVMObjectICell* indirectObj);

typedef CVMBool CVMNotifyFunc      (CVMExecEnv*     ee,
				    CVMObjectICell* indirectObj);
typedef CVMBool CVMNotifyAllFunc   (CVMExecEnv*     ee,
				    CVMObjectICell* indirectObj);
typedef CVMBool CVMWaitFunc        (CVMExecEnv*     ee,
				    CVMObjectICell* indirectObj,
				    CVMJavaLong     millis);

CVM_STRUCT_TYPEDEF(CVMReentrantMutex);
CVM_STRUCT_TYPEDEF(CVMSysMutex);
CVM_STRUCT_TYPEDEF(CVMSysMonitor);
CVM_STRUCT_TYPEDEF(CVMProfiledMonitor);
CVM_STRUCT_TYPEDEF(CVMNamedSysMonitor);
CVM_STRUCT_TYPEDEF(CVMObjMonitor);
CVM_STRUCT_TYPEDEF(CVMOwnedMonitor);

/*
 * Types for consistent states
 */
CVM_STRUCT_TYPEDEF(CVMCState);
CVM_STRUCT_TYPEDEF(CVMTCState);


/*
 * Class loading data structures
 */
CVM_STRUCT_TYPEDEF(CVMLoaderCacheEntry);
#ifdef CVM_CLASSLOADING
CVM_STRUCT_TYPEDEF(CVMLoaderConstraint);
CVM_STRUCT_TYPEDEF(CVMSeenClass);
CVM_STRUCT_TYPEDEF(CVMClassPathEntry);
#endif

/*
 * Stack data structures
 */
CVM_UNION_TYPEDEF (CVMStackVal32);
CVM_STRUCT_TYPEDEF(CVMStack);
CVM_STRUCT_TYPEDEF(CVMStackChunk);
CVM_STRUCT_TYPEDEF(CVMFrame);
CVM_STRUCT_TYPEDEF(CVMFrameIterator);
CVM_STRUCT_TYPEDEF(CVMLocalRootsFrame);
CVM_STRUCT_TYPEDEF(CVMFreelistFrame);
CVM_STRUCT_TYPEDEF(CVMInterpreterFrame);
CVM_STRUCT_TYPEDEF(CVMJavaFrame);
#ifdef CVM_JIT
CVM_STRUCT_TYPEDEF(CVMCompiledFrame);
#endif
CVM_STRUCT_TYPEDEF(CVMTransitionFrame);
CVM_STRUCT_TYPEDEF(CVMStackWalkContext);

/*
 * VM global state
 */
CVM_STRUCT_TYPEDEF(CVMGlobalState);
CVM_STRUCT_TYPEDEF(CVMGCGlobalState);

/*
 * VM initialization options
 */
CVM_STRUCT_TYPEDEF(CVMOptions);

/*
 * GC-related types
 */
CVM_STRUCT_TYPEDEF(CVMGCOptions);

#ifndef CDC_10
/*
 * Java Assertion related types
 */
CVM_STRUCT_TYPEDEF(CVMJavaAssertionsOptionList);
#endif

/*
 * A 'ref callback' called on each discovered root
 */
typedef void (*CVMRefCallbackFunc)(CVMObject** refAddr, void* data);

/*
 * A predicate to test liveness of a given reference.
 */
typedef CVMBool (*CVMRefLivenessQueryFunc)(CVMObject** refAddr, void* data);

/*
 * A per-object callback function, to be called during heap dumps
 */
typedef CVMBool (*CVMObjectCallbackFunc)(CVMObject* obj, CVMClassBlock* cb, 
                                         CVMUint32  objSize, void* data);

/*
 * Scan all GC references in a frame. Each frame carries a pointer
 * to one of these.
 */
typedef void CVMFrameGCScannerFunc(CVMExecEnv* ee,
				   CVMFrame* thisFrame,
				   CVMStackChunk* thisChunk,
				   CVMRefCallbackFunc refCallback,
				   void* data,
				   CVMGCOptions* gcOpts);

/*
 * JNI types
 */

struct JNINativeInterface;

#if defined(__cplusplus) && !defined(CVM_JNI_C_INTERFACE)
struct JNIEnv_;
typedef JNIEnv_ JNIEnv;
#else
typedef const struct JNINativeInterface *JNIEnv;
#endif

#endif /* !_ASM */
#endif /* _INCLUDED_DEFS_H */
