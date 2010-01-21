/*
 * @(#)jvm.h	1.91 06/10/10
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

#ifndef _JAVASOFT_JVM_H_
#define _JAVASOFT_JVM_H_

/* Fix for bug #6264809 JVM_O_DELETE is not really used anywhere. */
/* #define JVM_O_DELETE 0x10000 */

#include "javavm/include/porting/java_props.h"
#ifdef JAVASE
/* NOTE: This is a special case.  xxx_md.h files are usually not included
   directly, but should go through the porting later in:
      src/share/javavm/include/porting/
   Do not replicate this code pattern unless you really know what you are
   doing.
*/
#include "jvm_md.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * This file contains additional functions exported from the VM.
 * These functions are complementary to the standard JNI support.
 * There are three parts to this file:
 * 
 * First, this file contains the VM-related functions needed by native
 * libraries in the standard Java API. For example, the java.lang.Object
 * class needs VM-level functions that wait for and notify monitors.
 * 
 * Second, this file contains the functions and constant definitions
 * needed by the byte code verifier and class file format checker.
 * These functions allow the verifier and format checker to be written
 * in a VM-independent way.
 * 
 * Third, this file contains various I/O and nerwork operations needed
 * by the standard Java I/O and network APIs.
 */

/*
 * Bump the version number when either of the following happens:
 *
 * 1. There is a change in JVM_* functions.
 *
 * 2. There is a change in the contract between VM and Java classes.
 *    For example, if the VM relies on a new private field in Thread
 *    class.
 */

#define JVM_INTERFACE_VERSION 4

JNIEXPORT jint JNICALL
JVM_GetInterfaceVersion(void);

/*************************************************************************
 PART 1: Functions for Native Libraries
 ************************************************************************/
/*
 * java.lang.Object
 */
JNIEXPORT jint JNICALL
JVM_IHashCode(JNIEnv *env, jobject obj);

JNIEXPORT void JNICALL
JVM_MonitorWait(JNIEnv *env, jobject obj, jlong ms);

JNIEXPORT void JNICALL
JVM_MonitorNotify(JNIEnv *env, jobject obj);

JNIEXPORT void JNICALL
JVM_MonitorNotifyAll(JNIEnv *env, jobject obj);

JNIEXPORT jobject JNICALL
JVM_Clone(JNIEnv *env, jobject obj);

/*
 * java.lang.String
 */
JNIEXPORT jstring JNICALL
JVM_InternString(JNIEnv *env, jstring str);

/*
 * java.lang.System
 */
JNIEXPORT jlong JNICALL
JVM_CurrentTimeMillis(JNIEnv *env, jclass ignored);

JNIEXPORT void JNICALL
JVM_ArrayCopy(JNIEnv *env, jclass ignored, jobject src, jint src_pos, 
	      jobject dst, jint dst_pos, jint length);

/*
 * Initialize VM specific properties given a Properties object p, and an
 * sprops structure returned by the platform.
 */
JNIEXPORT jobject JNICALL
JVM_InitProperties(JNIEnv *env, jobject p, java_props_t *sprops);

/*
 * java.io.File
 */
JNIEXPORT void JNICALL
JVM_OnExit(void (*func)(void));

/*
 * java.lang.Runtime
 */
JNIEXPORT void JNICALL
JVM_Exit(jint code);

JNIEXPORT void JNICALL
JVM_Halt(jint code);

JNIEXPORT void JNICALL
JVM_GC(void);

/*
 * Force a garbage collection to happen. Possible values for the
 * flags parameter are (may be combined using binary OR):
 *
 * JVM_COLLECT_YOUNG_SPACE_ONLY:
 *   - If false, the entire normal object space is collected.
 *     If true, only the young object space is collected
 *
 * JVM_COLLECT_COMPILER_AREA:
 *     NOTE: Ignored for the CVM implementation
 *   - If true, the compiled area may be shrunk to accomodate
 *     the requested size.
 *   - If false, the compiler area is not affected by the garbage 
 *     collection, even if the requested size cannot otherwise be
 *     satisfied.
 *
 * The requested_free_bytes parameter specifies the desired amount of
 * free space (in bytes) that should be available in the heap for
 * object allocation.
 */
JNIEXPORT int JNICALL
JVM_GarbageCollect(int flags, int requested_free_bytes);

#define JVM_COLLECT_YOUNG_SPACE_ONLY     (1 << 0)
#define JVM_COLLECT_COMPILER_AREA        (1 << 1)


/* Returns the number of real-time milliseconds that have elapsed since the
 * least-recently-inspected heap object was last inspected by the garbage
 * collector.
 *
 * For simple stop-the-world collectors this value is just the time
 * since the most recent collection.  For generational collectors it is the
 * time since the oldest generation was most recently collected.  Other
 * collectors are free to return a pessimistic estimate of the elapsed time, or
 * simply the time since the last full collection was performed.
 *
 * Note that in the presence of reference objects, a given object that is no
 * longer strongly reachable may have to be inspected multiple times before it
 * can be reclaimed.
 */
JNIEXPORT jlong JNICALL
JVM_MaxObjectInspectionAge(void);

JNIEXPORT void JNICALL
JVM_TraceInstructions(jboolean on);

JNIEXPORT void JNICALL
JVM_TraceMethodCalls(jboolean on);

JNIEXPORT jlong JNICALL
JVM_TotalMemory(void);

JNIEXPORT jlong JNICALL
JVM_FreeMemory(void);

#ifndef CDC_10
JNIEXPORT jlong JNICALL
JVM_MaxMemory(void);
#endif

JNIEXPORT jint JNICALL
JVM_ActiveProcessorCount(void);

JNIEXPORT void * JNICALL 
JVM_LoadLibrary(const char *name);

JNIEXPORT void JNICALL 
JVM_UnloadLibrary(void * handle);

JNIEXPORT void * JNICALL 
JVM_FindLibraryEntry(void *handle, const char *name);

JNIEXPORT jboolean JNICALL
JVM_IsSupportedJNIVersion(jint version);

/*
 * java.lang.Float and java.lang.Double
 */
JNIEXPORT jboolean JNICALL
JVM_IsNaN(jdouble d);

/*
 * java.lang.Throwable
 */
JNIEXPORT void JNICALL
JVM_FillInStackTrace(JNIEnv *env, jobject throwable);

JNIEXPORT void JNICALL
JVM_PrintStackTrace(JNIEnv *env, jobject throwable, jobject printable);

#ifndef CDC_10
JNIEXPORT jint JNICALL
JVM_GetStackTraceDepth(JNIEnv *env, jobject throwable);

JNIEXPORT jobject JNICALL
JVM_GetStackTraceElement(JNIEnv *env, jobject throwable, jint index);
#endif

/*
 * java.lang.Compiler
 */
JNIEXPORT void JNICALL
JVM_InitializeCompiler (JNIEnv *env, jclass compCls);

JNIEXPORT jboolean JNICALL
JVM_IsSilentCompiler(JNIEnv *env, jclass compCls);

JNIEXPORT jboolean JNICALL
JVM_CompileClass(JNIEnv *env, jclass compCls, jclass cls);

JNIEXPORT jboolean JNICALL
JVM_CompileClasses(JNIEnv *env, jclass cls, jstring jname);

JNIEXPORT jobject JNICALL
JVM_CompilerCommand(JNIEnv *env, jclass compCls, jobject arg);

JNIEXPORT void JNICALL
JVM_EnableCompiler(JNIEnv *env, jclass compCls);

JNIEXPORT void JNICALL
JVM_DisableCompiler(JNIEnv *env, jclass compCls);

/*
 * java.lang.Thread
 */
JNIEXPORT void JNICALL
JVM_StartThread(JNIEnv *env, jobject thread, jint priority);

JNIEXPORT void JNICALL
JVM_StopThread(JNIEnv *env, jobject thread, jobject exception);

JNIEXPORT jboolean JNICALL
JVM_IsThreadAlive(JNIEnv *env, jobject thread);

JNIEXPORT void JNICALL
JVM_SuspendThread(JNIEnv *env, jobject thread);

JNIEXPORT void JNICALL
JVM_ResumeThread(JNIEnv *env, jobject thread);

JNIEXPORT void JNICALL
JVM_SetThreadPriority(JNIEnv *env, jobject thread, jint prio);

JNIEXPORT void JNICALL
JVM_Yield(JNIEnv *env, jclass threadClass);

JNIEXPORT void JNICALL
JVM_Sleep(JNIEnv *env, jclass threadClass, jlong millis);

JNIEXPORT jobject JNICALL
JVM_CurrentThread(JNIEnv *env, jclass threadClass);

JNIEXPORT jint JNICALL
JVM_CountStackFrames(JNIEnv *env, jobject thread);

JNIEXPORT void JNICALL
JVM_Interrupt(JNIEnv *env, jobject thread);

JNIEXPORT jboolean JNICALL
JVM_IsInterrupted(JNIEnv *env, jobject thread, jboolean clearInterrupted);

#ifndef CDC_10
JNIEXPORT jboolean JNICALL
JVM_HoldsLock(JNIEnv *env, jclass threadClass, jobject obj);
#endif

/*
 * java.lang.SecurityManager
 */
JNIEXPORT jclass JNICALL
JVM_CurrentLoadedClass(JNIEnv *env);

JNIEXPORT jobject JNICALL
JVM_CurrentClassLoader(JNIEnv *env);

JNIEXPORT jobjectArray JNICALL
JVM_GetClassContext(JNIEnv *env);

JNIEXPORT jint JNICALL
JVM_ClassDepth(JNIEnv *env, jstring name);

JNIEXPORT jint JNICALL
JVM_ClassLoaderDepth(JNIEnv *env);

/*
 * java.lang.Package
 */
JNIEXPORT jstring JNICALL
JVM_GetSystemPackage(JNIEnv *env, jstring name);

JNIEXPORT jobjectArray JNICALL
JVM_GetSystemPackages(JNIEnv *env);

/*
 * java.io.ObjectInputStream
 */
JNIEXPORT jobject JNICALL
JVM_AllocateNewObject(JNIEnv *env, jobject obj, jclass currClass,
                      jclass initClass);

JNIEXPORT jobject JNICALL
JVM_AllocateNewArray(JNIEnv *env, jobject obj, jclass currClass,
                     jint length);

JNIEXPORT jobject JNICALL
JVM_LatestUserDefinedLoader(JNIEnv *env);

/*
 * java.lang.reflect.Array
 */
JNIEXPORT jint JNICALL
JVM_GetArrayLength(JNIEnv *env, jobject arr);

JNIEXPORT jobject JNICALL
JVM_GetArrayElement(JNIEnv *env, jobject arr, jint index);

JNIEXPORT jvalue JNICALL
JVM_GetPrimitiveArrayElement(JNIEnv *env, jobject arr, jint index, jint wCode);

JNIEXPORT void JNICALL
JVM_SetArrayElement(JNIEnv *env, jobject arr, jint index, jobject val);

JNIEXPORT void JNICALL
JVM_SetPrimitiveArrayElement(JNIEnv *env, jobject arr, jint index, jvalue v,
			     CVMBasicType vCode);

JNIEXPORT jobject JNICALL
JVM_NewArray(JNIEnv *env, jclass eltClass, jint length);

JNIEXPORT jobject JNICALL
JVM_NewMultiArray(JNIEnv *env, jclass eltClass, jintArray dim);

/*
 * java.lang.reflect.Field
 */
JNIEXPORT jobject JNICALL
JVM_GetField(JNIEnv *env, jobject field, jobject obj);

JNIEXPORT jvalue JNICALL
JVM_GetPrimitiveField(JNIEnv *env, jobject field, jobject obj,
                      unsigned char wCode);

JNIEXPORT void JNICALL
JVM_SetField(JNIEnv *env, jobject field, jobject obj, jobject val);

JNIEXPORT void JNICALL
JVM_SetPrimitiveField(JNIEnv *env, jobject field, jobject obj, jvalue v,
                      unsigned char vCode);


/*
 * java.lang.reflect.Method
 */
JNIEXPORT jobject JNICALL
JVM_InvokeMethod(JNIEnv *env, jobject method, jobject obj, jobjectArray args0);

/*
 * java.lang.reflect.Constructor
 */
JNIEXPORT jobject JNICALL
JVM_NewInstanceFromConstructor(JNIEnv *env, jobject c, jobjectArray args0);

/*
 * java.lang.Class and java.lang.ClassLoader
 */
/*
 * Returns the class in which the code invoking the native method
 * belongs.
 *
 * Note that in JDK 1.1, native methods did not create a frame.
 * In 1.2, they do. Therefore native methods like Class.forName
 * can no longer look at the current frame for the caller class.
 */
JNIEXPORT jclass JNICALL
JVM_GetCallerClass(JNIEnv *env, int n);

/*
 * Find primitive classes
 * utf: class name
 */
JNIEXPORT jclass JNICALL
JVM_FindPrimitiveClass(JNIEnv *env, const char *utf);

/*
 * Link the class
 */
JNIEXPORT void JNICALL
JVM_ResolveClass(JNIEnv *env, jclass cls);

/*
 * Find a class from a given class loader. Throw ClassNotFoundException
 * or NoClassDefFoundError depending on the value of the last
 * argument.
 */
JNIEXPORT jclass JNICALL
JVM_FindClassFromClassLoader(JNIEnv *env, const char *name, jboolean init,
			     jobject loader, jboolean throwError);

/* Find a loaded class cached by the VM */
JNIEXPORT jclass JNICALL
JVM_FindLoadedClass(JNIEnv *env, jobject loader, jstring name);

/* Define a class */
JNIEXPORT jclass JNICALL
JVM_DefineClass(JNIEnv *env, const char *name, jobject loader, const jbyte *buf,
                jsize len, jobject pd);


/*
 * Reflection support functions
 */

JNIEXPORT jstring JNICALL
JVM_GetClassName(JNIEnv *env, jclass cls);

JNIEXPORT jobjectArray JNICALL
JVM_GetClassInterfaces(JNIEnv *env, jclass cls);

JNIEXPORT jobject JNICALL
JVM_GetClassLoader(JNIEnv *env, jclass cls);

JNIEXPORT jboolean JNICALL
JVM_IsInterface(JNIEnv *env, jclass cls);

JNIEXPORT jobjectArray JNICALL
JVM_GetClassSigners(JNIEnv *env, jclass cls);

JNIEXPORT void JNICALL
JVM_SetClassSigners(JNIEnv *env, jclass cls, jobjectArray signers);

JNIEXPORT jobject JNICALL
JVM_GetProtectionDomain(JNIEnv *env, jclass cls);

JNIEXPORT void JNICALL
JVM_SetProtectionDomain(JNIEnv *env, jclass cls, jobject protection_domain);

JNIEXPORT jboolean JNICALL
JVM_IsArrayClass(JNIEnv *env, jclass cls);

JNIEXPORT jboolean JNICALL
JVM_IsPrimitiveClass(JNIEnv *env, jclass cls);

JNIEXPORT jclass JNICALL
JVM_GetComponentType(JNIEnv *env, jclass cls);

JNIEXPORT jint JNICALL
JVM_GetClassModifiers(JNIEnv *env, jclass cls);

#ifdef JAVASE
/* Generics support (JDK 1.5) */
JNIEXPORT jstring JNICALL
JVM_GetClassSignature(JNIEnv *env, jclass cls);

/* Annotations support (JDK 1.5) */
JNIEXPORT jbyteArray JNICALL
JVM_GetClassAnnotations(JNIEnv *env, jclass cls);
#endif

/*
 * reflecting fields and methods.
 * which: 0 --- MEMBER_PUBLIC
 *        1 --- MEMBER_DECLARED
 */

JNIEXPORT jobjectArray JNICALL
JVM_GetClassFields(JNIEnv *env, jclass cls, jint which);

JNIEXPORT jobjectArray JNICALL
JVM_GetClassMethods(JNIEnv *env, jclass cls, jint which);

JNIEXPORT jobjectArray JNICALL
JVM_GetClassConstructors(JNIEnv *env, jclass cls, jint which);

JNIEXPORT jobject JNICALL
JVM_GetClassField(JNIEnv *env, jclass cls, jstring name, jint which);

JNIEXPORT jobject JNICALL
JVM_GetClassMethod(JNIEnv *env, jclass cls, jstring name, jobjectArray types,
		   jint which);
JNIEXPORT jobject JNICALL
JVM_GetClassConstructor(JNIEnv *env, jclass cls, jobjectArray types,
			jint which);

JNIEXPORT jobjectArray JNICALL
JVM_GetDeclaredClasses(JNIEnv *env, jclass ofClass);

JNIEXPORT jclass JNICALL
JVM_GetDeclaringClass(JNIEnv *env, jclass ofClass);

/*
 * Implements Class.newInstance
 */
JNIEXPORT jobject JNICALL
JVM_NewInstance(JNIEnv *env, jclass cls);

/*
 * java.security.*
 */

JNIEXPORT jobject JNICALL
JVM_DoPrivileged(JNIEnv *env, jclass cls, 
		 jobject action, jobject context, jboolean wrapException);

JNIEXPORT jobject JNICALL
JVM_GetInheritedAccessControlContext(JNIEnv *env, jclass cls);

JNIEXPORT jobject JNICALL
JVM_GetStackAccessControlContext(JNIEnv *env, jclass cls);

/*
 * sun.misc.Signal stuff. Not a standard Java API. Don't need to implement
 * them on all VMs.
 */

JNIEXPORT void * JNICALL
JVM_RegisterSignal(jint sig, void *handler);

JNIEXPORT jboolean JNICALL
JVM_RaiseSignal(jint sig);

JNIEXPORT jint JNICALL
JVM_FindSignal(const char *name);

#ifndef CDC_10
/*
 * Retrieve the assertion directives for the specified class.
 */
JNIEXPORT jboolean JNICALL
JVM_DesiredAssertionStatus(JNIEnv *env, jclass unused, jclass cls);

/*
 * Retrieve the assertion directives from the VM.
 */
JNIEXPORT jobject JNICALL
JVM_AssertionStatusDirectives(JNIEnv *env, jclass unused);
#endif

#ifdef JAVASE
/*
 * sun.misc.AtomicLong
 */
JNIEXPORT jboolean JNICALL
JVM_SupportsCX8(void);

JNIEXPORT jboolean JNICALL
JVM_CX8Field(JNIEnv *env, jobject obj, jfieldID fldID, jlong oldVal, jlong newVal);

#endif

/* Constants in class files */

#define JVM_ACC_PUBLIC        0x0001  /* visible to everyone */
#define JVM_ACC_PRIVATE       0x0002  /* visible only to the defining class */
#define JVM_ACC_PROTECTED     0x0004  /* visible to subclasses */
#define JVM_ACC_STATIC        0x0008  /* instance variable is static */
#define JVM_ACC_FINAL         0x0010  /* no further subclassing, overriding */
#define JVM_ACC_SYNCHRONIZED  0x0020  /* wrap method call in monitor lock */
#define JVM_ACC_SUPER         0x0020  /* funky handling of invokespecial */
#define JVM_ACC_VOLATILE      0x0040  /* can not cache in registers */
#define JVM_ACC_BRIDGE        0x0040  /* bridge method generated by compiler */
#define JVM_ACC_TRANSIENT     0x0080  /* not persistant */
#define JVM_ACC_VARARGS       0x0080  /* method declared with variable number of args */
#define JVM_ACC_NATIVE        0x0100  /* implemented in C */
#define JVM_ACC_INTERFACE     0x0200  /* class is an interface */
#define JVM_ACC_ABSTRACT      0x0400  /* no definition provided */
#define JVM_ACC_STRICT	      0x0800  /* strict floating point */
#define JVM_ACC_SYNTHETIC     0x1000  /* compiler-generated class, method or field */

#define JVM_ACC_ANNOTATION    0x2000  /* annotation type */
#define JVM_ACC_ENUM          0x4000  /* field is declared as element of enum */

#define JVM_ACC_PUBLIC_BIT        0
#define JVM_ACC_PRIVATE_BIT       1
#define JVM_ACC_PROTECTED_BIT     2
#define JVM_ACC_STATIC_BIT        3
#define JVM_ACC_FINAL_BIT         4
#define JVM_ACC_SYNCHRONIZED_BIT  5
#define JVM_ACC_SUPER_BIT         5
#define JVM_ACC_VOLATILE_BIT      6
#define JVM_ACC_BRIDGE_BIT        6
#define JVM_ACC_TRANSIENT_BIT     7
#define JVM_ACC_VARARGS_BIT       7
#define JVM_ACC_NATIVE_BIT        8
#define JVM_ACC_INTERFACE_BIT     9
#define JVM_ACC_ABSTRACT_BIT      10
#define JVM_ACC_STRICT_BIT        11
#define JVM_ACC_SYNTHETIC_BIT     12
#define JVM_ACC_ANNOTATION_BIT    13
#define JVM_ACC_ENUM_BIT          14

enum {
    JVM_CONSTANT_Utf8 = 1,
    JVM_CONSTANT_Unicode,		/* unused */
    JVM_CONSTANT_Integer,
    JVM_CONSTANT_Float,
    JVM_CONSTANT_Long,      
    JVM_CONSTANT_Double,
    JVM_CONSTANT_Class,
    JVM_CONSTANT_String,
    JVM_CONSTANT_Fieldref,
    JVM_CONSTANT_Methodref,
    JVM_CONSTANT_InterfaceMethodref,
    JVM_CONSTANT_NameAndType
};

/* Used in the newarray instruction. */

#define JVM_T_BOOLEAN 4
#define JVM_T_CHAR    5
#define JVM_T_FLOAT   6
#define JVM_T_DOUBLE  7
#define JVM_T_BYTE    8
#define JVM_T_SHORT   9
#define JVM_T_INT    10
#define JVM_T_LONG   11

/* JVM method signatures */

#define JVM_SIGNATURE_ARRAY		'['
#define JVM_SIGNATURE_BYTE		'B'
#define JVM_SIGNATURE_CHAR		'C'
#define JVM_SIGNATURE_CLASS		'L'
#define JVM_SIGNATURE_ENDCLASS	        ';'
#define JVM_SIGNATURE_ENUM		'E'
#define JVM_SIGNATURE_FLOAT		'F'
#define JVM_SIGNATURE_DOUBLE            'D'
#define JVM_SIGNATURE_FUNC		'('
#define JVM_SIGNATURE_ENDFUNC	        ')'
#define JVM_SIGNATURE_INT		'I'
#define JVM_SIGNATURE_LONG		'J'
#define JVM_SIGNATURE_SHORT		'S'
#define JVM_SIGNATURE_VOID		'V'
#define JVM_SIGNATURE_BOOLEAN	        'Z'

/*
 * StackMap type tags. Used in the CLDC StackMap attribute
 * as well as the JDK StackMapTable attribute.
 */
enum {
    JVM_STACKMAP_TOP = 0,
    JVM_STACKMAP_INT,
    JVM_STACKMAP_FLOAT,
    JVM_STACKMAP_DOUBLE,
    JVM_STACKMAP_LONG,
    JVM_STACKMAP_NULL,
    JVM_STACKMAP_UNINIT_THIS,
    JVM_STACKMAP_OBJECT,
    JVM_STACKMAP_UNINIT_OBJECT,
};

/* 
 * A function defined by the byte-code verifier and called by the VM.
 * This is not a function implemented in the VM.
 *
 * Returns JNI_FALSE if verification fails. A detailed error message
 * will be places in msg_buf, whose length is specified by buf_len.
 */ 
typedef jboolean (*verifier_fn_t)(JNIEnv *env, 
				  jclass cb,
				  char * msg_buf, 
				  jint buf_len);


/*
 * Support for a VM-independent class format checker.
 */ 
typedef struct {
    unsigned long code;    /* byte code */
    unsigned long excs;    /* exceptions */
    unsigned long etab;    /* catch table */
    unsigned long lnum;    /* line number */
    unsigned long lvar;    /* local vars */
} method_size_info;

typedef struct {
    unsigned int constants;    /* constant pool */
    unsigned int fields;
    unsigned int methods;
    unsigned int interfaces;
    unsigned int fields2;      /* number of static 2-word fields */
    unsigned int innerclasses; /* # of records in InnerClasses attr */

    method_size_info clinit;   /* memory used in clinit */
    method_size_info main;     /* used everywhere else */
} class_size_info;

/* 
 * Functions defined in libjava.so to perform string conversions.
 * 
 */

typedef jstring (*to_java_string_fn_t)(JNIEnv *env, char *str);

typedef char *(*to_c_string_fn_t)(JNIEnv *env, jstring s, jboolean *b);

/* This is the function defined in libjava.so that performs class
 * format checks. This functions fills in size information about
 * the class file and returns:
 *  
 *   0: good
 *  -1: out of memory
 *  -2: bad format
 *  -3: unsupported version
 *  -4: bad class name
 */

typedef jint (*check_format_fn_t)(char *class_name,
				  unsigned char *data,
				  unsigned int data_size,
				  class_size_info *class_size,
				  char *message_buffer,
				  jint buffer_length,
				  jboolean measure_only,
				  jboolean check_relaxed);

#define JVM_RECOGNIZED_CLASS_MODIFIERS (JVM_ACC_PUBLIC | \
					JVM_ACC_FINAL | \
					JVM_ACC_SUPER | \
					JVM_ACC_INTERFACE | \
					JVM_ACC_ABSTRACT | \
                                        JVM_ACC_ANNOTATION | \
					JVM_ACC_SYNTHETIC | \
					JVM_ACC_ENUM)    
       
#define JVM_RECOGNIZED_FIELD_MODIFIERS (JVM_ACC_PUBLIC | \
					JVM_ACC_PRIVATE | \
					JVM_ACC_PROTECTED | \
					JVM_ACC_STATIC | \
					JVM_ACC_FINAL | \
					JVM_ACC_VOLATILE | \
					JVM_ACC_TRANSIENT | \
					JVM_ACC_ENUM | \
					JVM_ACC_SYNTHETIC)

#define JVM_RECOGNIZED_METHOD_MODIFIERS (JVM_ACC_PUBLIC | \
					 JVM_ACC_PRIVATE | \
					 JVM_ACC_PROTECTED | \
					 JVM_ACC_STATIC | \
					 JVM_ACC_FINAL | \
					 JVM_ACC_SYNCHRONIZED | \
					 JVM_ACC_BRIDGE | \
					 JVM_ACC_VARARGS | \
					 JVM_ACC_NATIVE | \
					 JVM_ACC_ABSTRACT | \
					 JVM_ACC_STRICT | \
					 JVM_ACC_SYNTHETIC)


/*************************************************************************
 PART 3: I/O and Network Support
 ************************************************************************/

/* Note that the JVM IO functions are expected to return JVM_IO_ERR
 * when there is any kind of error. The caller can then use the
 * platform specific support (e.g., errno) to get the detailed 
 * error info.  The JVM_GetLastErrorString procedure may also be used
 * to obtain a descriptive error string.
 */
#define JVM_IO_ERR  (-1)

/* For interruptible IO. Returning JVM_IO_INTR indicates that an IO
 * operation has been disrupted by Thread.interrupt. There are a 
 * number of technical difficulties related to interruptible IO that
 * need to be solved. For example, most existing programs do not handle
 * InterruptedIOExceptions specially, they simply treat those as any 
 * IOExceptions, which typically indicate fatal errors.
 *
 * There are also two modes of operation for interruptible IO. In the
 * resumption mode, an interrupted IO operation is guaranteed not to
 * have any side-effects, and can be restarted. In the termination mode,
 * an interrupted IO operation corrupts the underlying IO stream, so
 * that the only reasonable operation on an interrupted stream is to
 * close that stream. The resumption mode seems to be impossible to
 * implement on Win32 and Solaris. Implementing the termination mode is
 * easier, but it's not clear that's the right semantics.
 *
 * Interruptible IO is not supported on Win32.It can be enabled/disabled
 * using a compile-time flag on Solaris. Third-party JVM ports do not 
 * need to implement interruptible IO.
 */
#define JVM_IO_INTR (-2)

/* Write a string into the given buffer, in the platform's local encoding,
 * that describes the most recent system-level error to occur in this thread.
 * Return the length of the string or zero if no error occurred.
 */
JNIEXPORT jint JNICALL
JVM_GetLastErrorString(char *buf, int len);

/*
 * Convert a pathname into native format.  This function does syntactic
 * cleanup, such as removing redundant separator characters.  It modifies
 * the given pathname string in place.
 */
JNIEXPORT char * JNICALL
JVM_NativePath(char *);

/*
 * JVM I/O error codes
 */
#define JVM_EEXIST       -100

/*
 * Open a file descriptor. This function returns a negative error code
 * on error, and a non-negative integer that is the file descriptor on
 * success.  
 */
JNIEXPORT jint JNICALL
JVM_Open(const char *fname, jint flags, jint mode);

/*
 * Close a file descriptor. This function returns -1 on error, and 0
 * on success.
 *
 * fd        the file descriptor to close.
 */
JNIEXPORT jint JNICALL
JVM_Close(jint fd);

/*
 * Read data from a file decriptor into a char array.
 *
 * fd        the file descriptor to read from.
 * buf       the buffer where to put the read data.
 * nbytes    the number of bytes to read.
 *
 * This function returns -1 on error, and 0 on success.
 */
JNIEXPORT jint JNICALL
JVM_Read(jint fd, char *buf, jint nbytes);

/*
 * Write data from a char array to a file decriptor.
 *
 * fd        the file descriptor to read from.
 * buf       the buffer from which to fetch the data.
 * nbytes    the number of bytes to write.
 *
 * This function returns -1 on error, and 0 on success.
 */
JNIEXPORT jint JNICALL
JVM_Write(jint fd, char *buf, jint nbytes);

/*
 * Returns the number of bytes available for reading from a given file
 * descriptor
 */
JNIEXPORT jint JNICALL
JVM_Available(jint fd, jlong *pbytes);

/*
 * Move the file descriptor pointer from whence by offset.
 *
 * fd        the file descriptor to move.
 * offset    the number of bytes to move it by.
 * whence    the start from where to move it.
 *
 * This function returns the resulting pointer location.
 */
JNIEXPORT jlong JNICALL
JVM_Lseek(jint fd, jlong offset, jint whence);

/*
 * Set the length of the file associated with the given descriptor to the given
 * length.  If the new length is longer than the current length then the file
 * is extended; the contents of the extended portion are not defined.  The
 * value of the file pointer is undefined after this procedure returns.
 */
JNIEXPORT jint JNICALL
JVM_SetLength(jint fd, jlong length);

/*
 * Synchronize the file descriptor's in memory state with that of the
 * physical device.  Return of -1 is an error, 0 is OK.
 */
JNIEXPORT jint JNICALL
JVM_Sync(jint fd);

/*
 * Networking library support
 */

JNIEXPORT jint JNICALL
JVM_InitializeSocketLibrary(void);

struct sockaddr;

JNIEXPORT jint JNICALL
JVM_Socket(jint domain, jint type, jint protocol);

JNIEXPORT jint JNICALL
JVM_SocketClose(jint fd);

JNIEXPORT jint JNICALL
JVM_SocketShutdown(jint fd, jint howto);

JNIEXPORT jint JNICALL
JVM_Recv(jint fd, char *buf, jint nBytes, jint flags);

JNIEXPORT jint JNICALL
JVM_Send(jint fd, char *buf, jint nBytes, jint flags);

JNIEXPORT jint JNICALL
JVM_Timeout(int fd, long timeout);

JNIEXPORT jint JNICALL
JVM_Listen(jint fd, jint count);

JNIEXPORT jint JNICALL
JVM_Connect(jint fd, struct sockaddr *him, jint len);

JNIEXPORT jint JNICALL
JVM_Bind(jint fd, struct sockaddr *him, jint len);

JNIEXPORT jint JNICALL
JVM_Accept(jint fd, struct sockaddr *him, jint *len);

JNIEXPORT jint JNICALL
JVM_RecvFrom(jint fd, char *buf, int nBytes,
                  int flags, struct sockaddr *from, int *fromlen);

JNIEXPORT jint JNICALL
JVM_SendTo(jint fd, char *buf, int len,
                int flags, struct sockaddr *to, int tolen);

JNIEXPORT jint JNICALL
JVM_SocketAvailable(jint fd, jint *result);


JNIEXPORT jint JNICALL
JVM_GetSockName(jint fd, struct sockaddr *him, int *len);

JNIEXPORT jint JNICALL
JVM_GetSockOpt(jint fd, int level, int optname, char *optval, int *optlen);

JNIEXPORT jint JNICALL
JVM_SetSockOpt(jint fd, int level, int optname, const char *optval, int optlen);

JNIEXPORT struct protoent * JNICALL
JVM_GetProtoByName(char* name);

JNIEXPORT struct hostent* JNICALL
JVM_GetHostByAddr(const char* name, int len, int type);

JNIEXPORT struct hostent* JNICALL
JVM_GetHostByName(char* name);

JNIEXPORT int JNICALL
JVM_GetHostName(char* name, int namelen);

/* 
 * The standard printing functions supported by the Java VM. (Should they
 * be renamed to JVM_* in the future?  
 */

/* 
 * BE CAREFUL! The following functions do not implement the
 * full feature set of standard C printf formats.
 */
int
jio_vsnprintf(char *str, size_t count, const char *fmt, va_list args);

int
jio_snprintf(char *str, size_t count, const char *fmt, ...);

int
jio_fprintf(FILE *, const char *fmt, ...);

int
jio_vfprintf(FILE *, const char *fmt, va_list args);


JNIEXPORT void * JNICALL
JVM_RawMonitorCreate(void);

JNIEXPORT void JNICALL
JVM_RawMonitorDestroy(void *mon);

JNIEXPORT jint JNICALL
JVM_RawMonitorEnter(void *mon);

JNIEXPORT void JNICALL
JVM_RawMonitorExit(void *mon);

#ifdef JAVASE

/*
 * java.lang.management support
 */
JNIEXPORT void* JNICALL
JVM_GetManagement(jint version);

/* Generics reflection support.
 *
 * Returns information about the given class's EnclosingMethod
 * attribute, if present, or null if the class had no enclosing
 * method.
 *
 * If non-null, the returned array contains three elements. Element 0
 * is the java.lang.Class of which the enclosing method is a member,
 * and elements 1 and 2 are the java.lang.Strings for the enclosing
 * method's name and descriptor, respectively.
 */
JNIEXPORT jobjectArray JNICALL
JVM_GetEnclosingMethodInfo(JNIEnv* env, jclass ofClass);

/* =========================================================================
 * The following defines a private JVM interface that the JDK can query
 * for the JVM version and capabilities.  sun.misc.Version defines 
 * the methods for getting the VM version and its capabilities.
 *
 * When a new bit is added, the following should be updated to provide
 * access to the new capability:
 *    HS:   JVM_GetVersionInfo and Abstract_VM_Version class
 *    SDK:  Version class
 *
 * Similary, a private JDK interface JDK_GetVersionInfo0 is defined for 
 * JVM to query for the JDK version and capabilities.
 *
 * When a new bit is added, the following should be updated to provide
 * access to the new capability:
 *    HS:   JDK_Version class
 *    SDK:  JDK_GetVersionInfo0
 *
 * ==========================================================================
 */
typedef struct {
    /* Naming convention of RE build version string: n.n.n[_uu[c]][-<identifier>]-bxx */
    unsigned int jvm_version;   /* Consists of major, minor, micro (n.n.n) */
                                /* and build number (xx) */
    unsigned int update_version : 8;         /* Update release version (uu) */
    unsigned int special_update_version : 8; /* Special update release version (c)*/
    unsigned int reserved1 : 16; 
    unsigned int reserved2; 

    /* The following bits represents JVM supports that JDK has dependency on.
     * JDK can use these bits to determine which JVM version
     * and support it has to maintain runtime compatibility.
     *
     * When a new bit is added in a minor or update release, make sure
     * the new bit is also added in the main/baseline.
     */
    unsigned int is_attach_supported : 1;
    unsigned int is_kernel_jvm : 1;
    unsigned int : 30;
    unsigned int : 32;
    unsigned int : 32;
} jvm_version_info;

#define JVM_VERSION_MAJOR(version) ((version & 0xFF000000) >> 24)
#define JVM_VERSION_MINOR(version) ((version & 0x00FF0000) >> 16)
#define JVM_VERSION_MICRO(version) ((version & 0x0000FF00) >> 8)

/* Build number is available only for RE builds.
 * It will be zero for internal builds.
 */
#define JVM_VERSION_BUILD(version) ((version & 0x000000FF))

JNIEXPORT void JNICALL
JVM_GetVersionInfo(JNIEnv* env, jvm_version_info* info, size_t info_size);

typedef struct {
    // Naming convention of RE build version string: n.n.n[_uu[c]][-<identifier>]-bxx
    unsigned int jdk_version;   /* Consists of major, minor, micro (n.n.n) */
                                /* and build number (xx) */
    unsigned int update_version : 8;         /* Update release version (uu) */
    unsigned int special_update_version : 8; /* Special update release version (c)*/
    unsigned int reserved1 : 16; 
    unsigned int reserved2; 

    /* The following bits represents new JDK supports that VM has dependency on.
     * VM implementation can use these bits to determine which JDK version
     * and support it has to maintain runtime compatibility.
     *
     * When a new bit is added in a minor or update release, make sure
     * the new bit is also added in the main/baseline.
     */
    unsigned int thread_park_blocker : 1;
    unsigned int : 31;
    unsigned int : 32;
    unsigned int : 32;
} jdk_version_info;

#define JDK_VERSION_MAJOR(version) ((version & 0xFF000000) >> 24)
#define JDK_VERSION_MINOR(version) ((version & 0x00FF0000) >> 16)
#define JDK_VERSION_MICRO(version) ((version & 0x0000FF00) >> 8)

/* Build number is available only for RE build (i.e. JDK_BUILD_NUMBER is set to bNN)
 * It will be zero for internal builds.
 */
#define JDK_VERSION_BUILD(version) ((version & 0x000000FF))

/* 
 * This is the function JDK_GetVersionInfo0 defined in libjava.so 
 * that is dynamically looked up by JVM.  
 */
typedef void (*jdk_version_info_fn_t)(jdk_version_info* info, size_t info_size);

#endif /* JAVASE */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !_JAVASOFT_JVM_H_ */
