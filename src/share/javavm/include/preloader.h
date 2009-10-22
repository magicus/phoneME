/*
 * @(#)preloader.h	1.75 06/10/25
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

/*
 * This file defines the interface to the ROMized classes.
 */

#ifndef _INCLUDED_PRELOADER_H
#define _INCLUDED_PRELOADER_H

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#ifdef CVM_USE_MEM_MGR
#include "javavm/include/mem_mgr.h"
#endif

/*
 * Class lookup by name. Only used from debuggers.
 */
#ifdef CVM_DEBUG
extern const CVMClassBlock*
CVMpreloaderLookup(const char* className);
#endif

/*
 * Return list of ROMized ClassLoader names.
 */
const char *
CVMpreloaderGetClassLoaderNames(CVMExecEnv *ee);

/*
 * Register ROMized ClassLoader object.
 */
void
CVMpreloaderRegisterClassLoaderUnsafe(CVMExecEnv *ee, CVMInt32 index,
    CVMClassLoaderICell *loader);


/*
 * Returns the cb of the romized class with the specified CVMClassTypeID.
 * This excludes VM-defined primitive types, such as "int" (though if a
 * user managed to define an "int" class, that would be fine).
 */

extern CVMClassBlock*
CVMpreloaderLookupFromType(CVMExecEnv *ee,
    CVMClassTypeID typeID, CVMObjectICell *loader);

/*
 * Only for lookup of primitive VM-defined types, by type name.
 */
extern CVMClassBlock*
CVMpreloaderLookupPrimitiveClassFromType(CVMClassTypeID classType);

#ifdef CVM_DUAL_STACK
extern CVMClassBlock*
CVMpreloaderLookupFromTypeInAuxTable(CVMClassTypeID typeID);
#endif

/*
 * Preloader initialization and destruction routines
 */
extern void
CVMpreloaderInit();

#ifdef CVM_DEBUG_ASSERTS
/*
 * Double check that objects we think are in ROM really are in ROM
 */
extern CVMBool
CVMpreloaderReallyInROM(CVMObject* ref);
#endif

extern void
CVMpreloaderInitializeStringInterning(CVMExecEnv* ee);

extern void
CVMpreloaderDestroy();

#ifdef CVM_DEBUG
extern void
CVMpreloaderCheckROMClassInitState(CVMExecEnv* ee);
#endif

/*
 * Avoid generating all stackmaps in advance of need. Instead, rewrite
 * code as required so that stackmap generation can be done lazily.
 */
extern CVMBool
CVMpreloaderDisambiguateAllMethods(CVMExecEnv* ee);

#ifdef CVM_JIT
extern void
CVMpreloaderInitInvokeCost();
#endif

/*
 * Iterate over all preloaded classes, and call 'callback' on each class.
 */
extern void
CVMpreloaderIterateAllClasses(CVMExecEnv* ee, 
			      CVMClassCallbackFunc callback,
			      void* data);

#ifdef CVM_JAVASE_CLASS_HAS_REF_FIELD
extern void
CVMpreloaderScanPreloadedClassObjects(CVMExecEnv* ee,
                                      CVMGCOptions* gcOpts,
                                      CVMRefCallbackFunc callback,
                                      void* data);
#endif

#if defined(CVM_INSPECTOR) || defined(CVM_DEBUG_ASSERTS) || defined(CVM_JVMPI)

/* Purpose: Checks to see if the specified object is a preloaded object. */
/* Returns: CVM_TRUE is the specified object is a preloaded object, else
            returns CVM_FALSE. */
CVMBool CVMpreloaderIsPreloadedObject(CVMObject *obj);
#endif

#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI) || defined(CVM_JVMTI)
/* Purpose: Iterate over all preloaded objects and calls Call callback() on
            each preloaded object. */
/* Returns: CVM_FALSE if exiting due to an abortion (i.e. the callback
            requested an abortion by returning CVM_FALSE).
            CVM_TRUE if exiting without an abortion. */
CVMBool
CVMpreloaderIteratePreloadedObjects(CVMExecEnv *ee, 
                                    CVMObjectCallbackFunc callback,
                                    void *callbackData);
#endif

/* Monitor and report "write" to the .bss region. */
#ifdef CVM_USE_MEM_MGR
/* Purpose: Register the BSS region. */
extern void
CVMmemRegisterBSS();

/* Purpose: Notify "write" to the BSS region. */
extern void
CVMmemBssWriteNotify(int pid, void *addr, void *pc, CVMMemHandle *h);

/* Purpose: Report dirty page info for the BSS region. */
extern void
CVMmemBssReportWrites();
#endif /* CVM_USE_MEM_MGR */

/*
 * Provide convenient handles to commonly used classes.
 */
#ifndef IN_ROMJAVA
#define CVM_CLASSBLOCK_DECL(claz) extern const CVMClassBlock claz##_Classblock

/*
 * Well known VM classes
 */
CVM_CLASSBLOCK_DECL(sun_misc_CVM);
CVM_CLASSBLOCK_DECL(sun_misc_Launcher);
CVM_CLASSBLOCK_DECL(sun_misc_Launcher_AppClassLoader);
CVM_CLASSBLOCK_DECL(sun_misc_Launcher_ClassContainer);
CVM_CLASSBLOCK_DECL(sun_misc_ThreadRegistry);
CVM_CLASSBLOCK_DECL(java_lang_Object);
CVM_CLASSBLOCK_DECL(java_lang_AssertionStatusDirectives);
CVM_CLASSBLOCK_DECL(java_lang_Class);
CVM_CLASSBLOCK_DECL(java_lang_ClassLoader);
CVM_CLASSBLOCK_DECL(java_lang_ClassLoader_NativeLibrary);
CVM_CLASSBLOCK_DECL(java_lang_Math);
CVM_CLASSBLOCK_DECL(java_lang_Shutdown);
CVM_CLASSBLOCK_DECL(java_lang_String);
CVM_CLASSBLOCK_DECL(java_lang_Thread);
CVM_CLASSBLOCK_DECL(java_lang_ThreadGroup);
CVM_CLASSBLOCK_DECL(java_lang_Throwable);
CVM_CLASSBLOCK_DECL(java_lang_StackTraceElement);
CVM_CLASSBLOCK_DECL(java_lang_Exception);
CVM_CLASSBLOCK_DECL(java_lang_Error);
CVM_CLASSBLOCK_DECL(java_lang_ThreadDeath);
CVM_CLASSBLOCK_DECL(java_lang_Cloneable);
CVM_CLASSBLOCK_DECL(java_lang_System);
CVM_CLASSBLOCK_DECL(java_io_File);
CVM_CLASSBLOCK_DECL(java_io_Serializable);
CVM_CLASSBLOCK_DECL(java_net_URLConnection);
CVM_CLASSBLOCK_DECL(java_lang_reflect_Field);
CVM_CLASSBLOCK_DECL(java_lang_reflect_Method);
CVM_CLASSBLOCK_DECL(java_lang_reflect_Constructor);
CVM_CLASSBLOCK_DECL(java_lang_ref_SoftReference); 
CVM_CLASSBLOCK_DECL(java_lang_ref_WeakReference);
CVM_CLASSBLOCK_DECL(java_lang_ref_PhantomReference);
CVM_CLASSBLOCK_DECL(java_lang_ref_FinalReference);
CVM_CLASSBLOCK_DECL(java_lang_ref_Finalizer);
CVM_CLASSBLOCK_DECL(java_lang_ref_Reference);
CVM_CLASSBLOCK_DECL(java_util_jar_JarFile);
CVM_CLASSBLOCK_DECL(java_util_ResourceBundle);

/* classes known to JIT for simple synchronization */
#ifdef CVM_JIT
CVM_CLASSBLOCK_DECL(java_lang_StringBuffer);
CVM_CLASSBLOCK_DECL(java_util_Hashtable);
CVM_CLASSBLOCK_DECL(java_util_Random);
CVM_CLASSBLOCK_DECL(java_util_Stack);
CVM_CLASSBLOCK_DECL(java_util_Vector);
CVM_CLASSBLOCK_DECL(java_util_Vector_1);
#endif

#ifdef CVM_LVM /* %begin lvm */
CVM_CLASSBLOCK_DECL(sun_misc_Isolate);
#endif /* %end lvm */

/*
 * Security
 */
CVM_CLASSBLOCK_DECL(java_security_AccessController);
CVM_CLASSBLOCK_DECL(java_security_CodeSource);
CVM_CLASSBLOCK_DECL(java_security_SecureClassLoader);

/*
 * Exception classes
 */

#ifdef CVM_CLASSLOADING
CVM_CLASSBLOCK_DECL(java_lang_ClassCircularityError);
CVM_CLASSBLOCK_DECL(java_lang_ClassFormatError);
CVM_CLASSBLOCK_DECL(java_lang_IllegalAccessError);
CVM_CLASSBLOCK_DECL(java_lang_InstantiationError);
CVM_CLASSBLOCK_DECL(java_lang_LinkageError);
CVM_CLASSBLOCK_DECL(java_lang_UnsupportedClassVersionError);
CVM_CLASSBLOCK_DECL(java_lang_VerifyError);
#endif

#ifdef CVM_DYNAMIC_LINKING
CVM_CLASSBLOCK_DECL(java_lang_UnsatisfiedLinkError);
#endif

#ifdef CVM_REFLECT
CVM_CLASSBLOCK_DECL(java_lang_NegativeArraySizeException);
CVM_CLASSBLOCK_DECL(java_lang_NoSuchFieldException);
CVM_CLASSBLOCK_DECL(java_lang_NoSuchMethodException);
#endif

#if defined(CVM_CLASSLOADING) || defined(CVM_REFLECT)
CVM_CLASSBLOCK_DECL(java_lang_IncompatibleClassChangeError);
#endif

CVM_CLASSBLOCK_DECL(java_lang_AbstractMethodError);
CVM_CLASSBLOCK_DECL(java_lang_ArithmeticException);
CVM_CLASSBLOCK_DECL(java_lang_ArrayIndexOutOfBoundsException);
CVM_CLASSBLOCK_DECL(java_lang_ArrayStoreException);
CVM_CLASSBLOCK_DECL(java_lang_ClassCastException);
CVM_CLASSBLOCK_DECL(java_lang_ClassNotFoundException);
CVM_CLASSBLOCK_DECL(java_lang_CloneNotSupportedException);
CVM_CLASSBLOCK_DECL(java_lang_IllegalAccessException);
CVM_CLASSBLOCK_DECL(java_lang_IllegalArgumentException);
CVM_CLASSBLOCK_DECL(java_lang_IllegalMonitorStateException);
#ifdef CVM_INSPECTOR
CVM_CLASSBLOCK_DECL(java_lang_IllegalStateException);
#endif
CVM_CLASSBLOCK_DECL(java_lang_InstantiationException);
CVM_CLASSBLOCK_DECL(java_lang_InternalError);
CVM_CLASSBLOCK_DECL(java_lang_InterruptedException);
CVM_CLASSBLOCK_DECL(java_lang_NoClassDefFoundError);
CVM_CLASSBLOCK_DECL(java_lang_NoSuchFieldError);
CVM_CLASSBLOCK_DECL(java_lang_NoSuchMethodError);
CVM_CLASSBLOCK_DECL(java_lang_NullPointerException);
CVM_CLASSBLOCK_DECL(java_lang_OutOfMemoryError);
CVM_CLASSBLOCK_DECL(java_lang_StackOverflowError);
CVM_CLASSBLOCK_DECL(java_lang_StringIndexOutOfBoundsException);
CVM_CLASSBLOCK_DECL(java_lang_UnsupportedOperationException);

CVM_CLASSBLOCK_DECL(java_io_InvalidClassException);
CVM_CLASSBLOCK_DECL(java_io_IOException);

CVM_CLASSBLOCK_DECL(java_lang_reflect_Method_ArgumentException);
CVM_CLASSBLOCK_DECL(java_lang_reflect_Method_AccessException);
CVM_CLASSBLOCK_DECL(java_lang_reflect_Constructor_ArgumentException);
CVM_CLASSBLOCK_DECL(java_lang_reflect_Constructor_AccessException);

CVM_CLASSBLOCK_DECL(sun_io_ConversionBufferFullException);
CVM_CLASSBLOCK_DECL(sun_io_UnknownCharacterException);
CVM_CLASSBLOCK_DECL(sun_io_MalformedInputException);

#ifdef CVM_AOT
CVM_CLASSBLOCK_DECL(sun_misc_Warmup);
#endif

/*
 * Arrays of basic types
 */
CVM_CLASSBLOCK_DECL(manufacturedArrayOfBoolean);
CVM_CLASSBLOCK_DECL(manufacturedArrayOfChar);
CVM_CLASSBLOCK_DECL(manufacturedArrayOfFloat);
CVM_CLASSBLOCK_DECL(manufacturedArrayOfDouble);
CVM_CLASSBLOCK_DECL(manufacturedArrayOfByte);
CVM_CLASSBLOCK_DECL(manufacturedArrayOfShort);
CVM_CLASSBLOCK_DECL(manufacturedArrayOfInt);
CVM_CLASSBLOCK_DECL(manufacturedArrayOfLong);
CVM_CLASSBLOCK_DECL(manufacturedArrayOfObject);

/*
 * Primitive type classes.
 */
CVM_CLASSBLOCK_DECL(primitiveClass_boolean);
CVM_CLASSBLOCK_DECL(primitiveClass_char);
CVM_CLASSBLOCK_DECL(primitiveClass_float);
CVM_CLASSBLOCK_DECL(primitiveClass_double);
CVM_CLASSBLOCK_DECL(primitiveClass_byte);
CVM_CLASSBLOCK_DECL(primitiveClass_short);
CVM_CLASSBLOCK_DECL(primitiveClass_int);
CVM_CLASSBLOCK_DECL(primitiveClass_long);
CVM_CLASSBLOCK_DECL(primitiveClass_void);

/*
 * Wrapper object classes.
 */
CVM_CLASSBLOCK_DECL(java_lang_Boolean);
CVM_CLASSBLOCK_DECL(java_lang_Character);
CVM_CLASSBLOCK_DECL(java_lang_Float);
CVM_CLASSBLOCK_DECL(java_lang_Double);
CVM_CLASSBLOCK_DECL(java_lang_Byte);
CVM_CLASSBLOCK_DECL(java_lang_Short);
CVM_CLASSBLOCK_DECL(java_lang_Integer);
CVM_CLASSBLOCK_DECL(java_lang_Long);

#undef CVM_CLASSBLOCK_DECL

extern CVMAddr * const CVM_staticData;
extern CVMUint32 CVM_nStaticData;
extern CVMAddr CVM_StaticDataMaster[];
extern const int CVM_nROMClasses;
#if defined(CVM_JIT) && defined(CVM_MTASK)
extern const int CVM_nROMMethods;
/* Pointer to the invokeCost array for romized methods. */
extern CVMUint16 * const CVMROMMethodInvokeCosts;
#endif

#define CVMpreloaderGetRefStatics()      (&CVM_staticData[1])
#define CVMpreloaderNumberOfRefStatics() (CVM_staticData[0])

/* The address of the CVMClassBlock for java_lang_Object, for instance,
 * is CVMsystemClass(java_lang_Object)
 * It is an R-value only, of course, since the underlying struct is const.
 */
#define CVMsystemClass(cname) ((CVMClassBlock*)(&cname##_Classblock))

/* The address of the CVMClassBlock for an array of basic type 'basictypename.
 * For instance CVMsystemArrayClassOf(Int).
 *
 * It is an R-value only, of course, since the underlying struct is const.
 */
#define CVMsystemArrayClassOf(basictypename) \
    ((CVMClassBlock*)&manufacturedArrayOf##basictypename##_Classblock)

/* The address of the CVMClassBlock for a primitive type.
 * For instance CVMprimitiveClass(void).
 *
 * It is an R-value only, of course, since the underlying struct is const.
 */
#define CVMprimitiveClass(basictypename) \
    ((CVMClassBlock*)&primitiveClass_##basictypename##_Classblock)

#endif

#endif /* _INCLUDED_PRELOADER_H */
