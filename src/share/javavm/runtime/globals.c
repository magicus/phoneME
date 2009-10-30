/*
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
#include "javavm/include/sync.h"
#include "javavm/include/globals.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/preloader.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/stackmaps.h"
#include "javavm/include/utils.h"
#include "javavm/include/timestamp.h"
#ifndef CDC_10
#include "javavm/include/javaAssertions.h"
#endif

#include "jni_statics.h"

#include "native/java/util/zip/zip_util.h"

#include "javavm/include/typeid.h"
#include "javavm/include/string.h"

#include "javavm/include/gc/gc_impl.h"

#include "javavm/include/clib.h"
#include "javavm/include/porting/memory.h"

#ifdef CVM_JVMTI
#include "javavm/include/jvmti_jni.h"
#include "javavm/include/porting/time.h"
#endif
#ifdef CVM_JVMPI
#include "javavm/include/jvmpi_impl.h"
#endif
#ifdef CVM_JIT
#include "javavm/include/jit_common.h"
#endif
#ifdef CVM_DYNAMIC_LINKING
#include "javavm/include/porting/linker.h"
#endif

/*
 * VM global state 
 */
CVMGlobalState CVMglobals;

/*
 * JNI natives and utilities global state
 */
CVMJNIStatics  CVMjniGlobalStatics;

/*
 * Target specific global state, exported to the HPI
 */
CVMTargetGlobalState * const CVMtargetGlobals = &CVMglobals.target;


#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
/* Pointer to CVMglobals.jit.gcTrapAddr so asm code can easily locate it */
void** const CVMgcTrapAddrPtr = (void**)&CVMglobals.jit.gcTrapAddr;
#endif

#ifdef CVM_AOT
/* Pointer to CVMglobals.jit.codeCacheAOTStart */
void** const CVMJITcodeCacheAOTStart = 
        (void**)&CVMglobals.jit.codeCacheAOTStart;

/* Pointer to CVMglobals.jit.codeCacheAOTEnd */
void** const CVMJITcodeCacheAOTEnd = 
        (void**)&CVMglobals.jit.codeCacheAOTEnd;
#endif

#ifdef CVM_JIT
/* Pointer to CVMglobals.jit.codeCacheDecompileStart */
void** const CVMJITcodeCacheDecompileStart = 
        (void**)&CVMglobals.jit.codeCacheDecompileStart;
#endif

/*
 * Capacities for the class table of dynamcially loaded classes.
 * NOTE: these values also used for the class global roots stack.
 */
#define CVM_MIN_CLASSTABLE_STACKCHUNK_SIZE   500
#define CVM_MAX_CLASSTABLE_STACK_SIZE      20000
#define CVM_INITIAL_CLASSTABLE_STACK_SIZE    500

/* 
 * Capacities for the global roots stack. It holds both regular global
 * roots and jni global refs.
 */
#define CVM_MIN_GLOBALROOTS_STACKCHUNK_SIZE 500
#define CVM_MAX_GLOBALROOTS_STACK_SIZE      \
    100 * CVM_MIN_GLOBALROOTS_STACKCHUNK_SIZE
#define CVM_INITIAL_GLOBALROOTS_STACK_SIZE   \
    CVM_MIN_GLOBALROOTS_STACKCHUNK_SIZE

/*
 * Capacities for the JNI weak global refs stack.
 */
#define CVM_MIN_WEAK_GLOBALROOTS_STACKCHUNK_SIZE \
    CVM_MIN_GLOBALROOTS_STACKCHUNK_SIZE
#ifdef CVM_JVMTI
#define CVM_MAX_WEAK_GLOBALROOTS_STACK_SIZE      \
    CVM_MAX_GLOBALROOTS_STACK_SIZE * 16
#else
#define CVM_MAX_WEAK_GLOBALROOTS_STACK_SIZE      \
    CVM_MAX_GLOBALROOTS_STACK_SIZE
#endif
#define CVM_INITIAL_WEAK_GLOBALROOTS_STACK_SIZE   \
    CVM_INITIAL_GLOBALROOTS_STACK_SIZE

/*
 * Capacities for the Class global roots stack.
 */
#define CVM_MIN_CLASS_GLOBALROOTS_STACKCHUNK_SIZE \
    CVM_MIN_CLASSTABLE_STACKCHUNK_SIZE
#define CVM_MAX_CLASS_GLOBALROOTS_STACK_SIZE      \
    CVM_MAX_CLASSTABLE_STACK_SIZE
#define CVM_INITIAL_CLASS_GLOBALROOTS_STACK_SIZE  \
    CVM_INITIAL_CLASSTABLE_STACK_SIZE

/*
 * Capacities for the ClassLoader global roots stack.
 */
#define CVM_MIN_CLASSLOADER_GLOBALROOTS_STACKCHUNK_SIZE 30
#define CVM_MAX_CLASSLOADER_GLOBALROOTS_STACK_SIZE      \
    10 * CVM_MIN_CLASS_GLOBALROOTS_STACKCHUNK_SIZE
#define CVM_INITIAL_CLASSLOADER_GLOBALROOTS_STACK_SIZE   \
    CVM_MIN_CLASS_GLOBALROOTS_STACKCHUNK_SIZE

/*
 * Capacities for the ProtectionDomain global roots stack.
 */
#define CVM_MIN_PROTECTION_DOMAIN_GLOBALROOTS_STACKCHUNK_SIZE \
    CVM_MIN_CLASSTABLE_STACKCHUNK_SIZE
#define CVM_MAX_PROTECTION_DOMAIN_GLOBALROOTS_STACK_SIZE      \
    CVM_MAX_CLASSTABLE_STACK_SIZE
#define CVM_INITIAL_PROTECTION_DOMAIN_GLOBALROOTS_STACK_SIZE  \
    CVM_INITIAL_CLASSTABLE_STACK_SIZE

static void 
CVMinitJNIStatics()
{
    memset((void*)&CVMjniGlobalStatics, 0, sizeof(CVMJNIStatics));
}

static void CVMdestroyJNIStatics()
{
    /* Nothing to do */
}

/*
 * The table of global sys mutexes so we can easily init, destroy, lock,
 * and unlock all of them.
 */
typedef struct {
    CVMSysMutex* mutex;
    const char*  name;
} CVMGlobalSysMutexEntry;

#define CVM_SYSMUTEX_ENTRY(mutex, name) {&CVMglobals.mutex, name}

static const CVMGlobalSysMutexEntry globalSysMutexes[] = {
#ifdef CVM_CLASSLOADING
    CVM_SYSMUTEX_ENTRY(nullClassLoaderLock, "NULL classloader lock"),
#endif
#ifdef CVM_JIT
    CVM_SYSMUTEX_ENTRY(jitLock, "jit lock"),
#endif
    CVM_SYSMUTEX_ENTRY(heapLock, "heap lock"),
    CVM_SYSMUTEX_ENTRY(threadLock, "thread-list lock"),
    CVM_SYSMUTEX_ENTRY(classTableLock, "class table lock"),
    CVM_SYSMUTEX_ENTRY(loaderCacheLock, "loader cache lock"),
    /* NOTE: This is not a leaf mutex. It must remain below the global
       roots lock since a couple of the JVMTI routines allocate and
       deallocate global roots while holding the debugger lock. In
       addition, others of them perform indirect memory accesses while
       holding the debugger lock, so if we ever had a lock associated
       with those accesses it would need to have a higher rank (less
       importance) than this one. */
#ifdef CVM_JVMTI
    CVM_SYSMUTEX_ENTRY(jvmtiLock, "jvmti lock"),
#endif
    /*
     * All of these are "leaf" mutexes except during gc and during
     * during thread suspension.
     */
    CVM_SYSMUTEX_ENTRY(globalRootsLock, "global roots lock"),
    CVM_SYSMUTEX_ENTRY(weakGlobalRootsLock, "weak global roots lock"),
    CVM_SYSMUTEX_ENTRY(typeidLock, "typeid lock"),
    CVM_SYSMUTEX_ENTRY(syncLock, "fast sync lock"),
    CVM_SYSMUTEX_ENTRY(internLock, "intern table lock"),
#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI) || defined(CVM_JVMTI)
    CVM_SYSMUTEX_ENTRY(gcLockerLock, "gc locker lock"),
#endif
#ifdef CVM_JVMPI
    CVM_SYSMUTEX_ENTRY(jvmpiSyncLock, "jvmpi sync lock"),
#endif
#ifdef CVM_JVMTI
    CVM_SYSMUTEX_ENTRY(jvmtiLockInfoLock, "jvmti lock info lock"),
#endif
#ifdef CVM_TIMESTAMPING
    CVM_SYSMUTEX_ENTRY(timestampListLock, "timestamp list lock"),
#endif
};

#define CVM_NUM_GLOBAL_SYSMUTEXES \
    (sizeof(globalSysMutexes) / sizeof(CVMGlobalSysMutexEntry))

/*
 * The table of global mb's so we can easily init all of them.
 */
typedef struct {
    CVMBool          isStatic;
    CVMClassBlock*   cb;
    const char*      methodName;
    const char*      methodSig;
    CVMMethodBlock** mbPtr;
} CVMGlobalMethodBlockEntry;

static const CVMGlobalMethodBlockEntry globalMethodBlocks[] = {
    /* AccessController.doPrivileged(PrivilegedAction) */
    {
	CVM_TRUE,  /* static */
	CVMsystemClass(java_security_AccessController),
        "doPrivileged", 
	"(Ljava/security/PrivilegedAction;)Ljava/lang/Object;",
        &CVMglobals.java_security_AccessController_doPrivilegedAction1,
        /* NOTE: java.security.AccessController has a static initializer, but
                 a clinit check is not needed because this mb is not used to
                 do any invocations. */
    },
    /* AccessController.doPrivileged(PrivilegedExceptionAction) */
    {
	CVM_TRUE,  /* static */
	CVMsystemClass(java_security_AccessController),
        "doPrivileged", 
	"(Ljava/security/PrivilegedExceptionAction;)Ljava/lang/Object;",
        &CVMglobals.java_security_AccessController_doPrivilegedExceptionAction1
        /* NOTE: java.security.AccessController has a static initializer, but
                 a clinit check is not needed because this mb is not used to
                 do any invocations. */
    },
    /* AccessController.doPrivileged(PrivilegedAction, AccessControlContext) */
    {
	CVM_TRUE,  /* static */
	CVMsystemClass(java_security_AccessController),
        "doPrivileged", 
	"(Ljava/security/PrivilegedAction;Ljava/security/AccessControlContext;)Ljava/lang/Object;",
        &CVMglobals.java_security_AccessController_doPrivilegedAction2
        /* NOTE: java.security.AccessController has a static initializer, but
                 a clinit check is not needed because this mb is not used to
                 do any invocations. */
    },
    /* AccessController.doPrivileged(PrivilegedExceptionAction, 
           AccessControlContext) */
    {
	CVM_TRUE,  /* static */
	CVMsystemClass(java_security_AccessController),
        "doPrivileged", 
	"(Ljava/security/PrivilegedExceptionAction;Ljava/security/AccessControlContext;)Ljava/lang/Object;",
        &CVMglobals.java_security_AccessController_doPrivilegedExceptionAction2
        /* NOTE: java.security.AccessController has a static initializer, but
                 a clinit check is not needed because this mb is not used to
                 do any invocations. */
    },
    /* java.lang.Shutdown.waitAllUserThreadsExitAndShutdown() */
    {
	CVM_TRUE,  /* static */
	CVMsystemClass(java_lang_Shutdown),
        "waitAllUserThreadsExitAndShutdown", "()V",
        &CVMglobals.java_lang_Shutdown_waitAllUserThreadsExitAndShutdown
        /* NOTE: java.lang.Shutdown has a static initializer, but
                 a clinit check is not needed because this mb is not used
                 until after the class is initialized implicitly in
                 JNI_CreateJavaVM().*/
    },
    /* sun.misc.ThreadRegistry.waitAllSystemThreadsExit() */
    {
	CVM_TRUE,  /* static */
	CVMsystemClass(sun_misc_ThreadRegistry),
        "waitAllSystemThreadsExit", "()V",
        &CVMglobals.sun_misc_ThreadRegistry_waitAllSystemThreadsExit
        /* NOTE: sun.misc.ThreadRegistry has a static initializer.  The clinit
                 will be done explicitly in JNI_CreateJavaVM(). */
    },
#ifdef CVM_REFLECT
    /* java.lang.reflect.Constructor.newInstance() */
    {
	CVM_FALSE, /* nonstatic */
	CVMsystemClass(java_lang_reflect_Constructor),
        "newInstance", "([Ljava/lang/Object;)Ljava/lang/Object;",
        &CVMglobals.java_lang_reflect_Constructor_newInstance
        /* NOTE: No clinit check needed because this mb is not used to do any
                 invocations. */
    },
    /* java.lang.reflect.Method.invoke() */
    {
	CVM_FALSE, /* nonstatic */
	CVMsystemClass(java_lang_reflect_Method),
        "invoke", "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;",
        &CVMglobals.java_lang_reflect_Method_invoke
        /* NOTE: No clinit check needed because this mb is not used to do any
                 invocations. */
    },
#endif
    /* java.lang.ref.Finalizer.register() */
    {
	CVM_TRUE,  /* static */
	CVMsystemClass(java_lang_ref_Finalizer),
        "register", "(Ljava/lang/Object;)V",
        &CVMglobals.java_lang_ref_Finalizer_register
        /* NOTE: java.lang.ref.Finalizer has a static initializer.  The clinit
                 will be done explicitly in JNI_CreateJavaVM(). */
    },
#ifdef CVM_LVM
    /* java.lang.ref.Finalizer.runAllFinalizersOfSystemClass() */
    {
	CVM_TRUE,  /* static */
	CVMsystemClass(java_lang_ref_Finalizer),
        "runAllFinalizersOfSystemClass", "()V",
        &CVMglobals.java_lang_ref_Finalizer_runAllFinalizersOfSystemClass
        /* NOTE: java.lang.ref.Finalizer has a static initializer.  The clinit
                 will be done explicitly in JNI_CreateJavaVM(). */
    },
#endif
#ifdef CVM_DEBUG_STACKTRACES
    /* java.lang.Throwable.fillInStackTrace() */
    {
	CVM_FALSE, /* nonstatic */
	CVMsystemClass(java_lang_Throwable),
        "fillInStackTrace", "()Ljava/lang/Throwable;",
        &CVMglobals.java_lang_Throwable_fillInStackTrace
        /* NOTE: No clinit check needed because this mb is not used to do any
                 invocations. */
    },
#endif /* CVM_DEBUG_STACKTRACES */
    /* java.lang.Class.runStaticInitializers() */
    {
	CVM_FALSE, /* nonstatic */
	CVMsystemClass(java_lang_Class),
        "runStaticInitializers", "()V",
        &CVMglobals.java_lang_Class_runStaticInitializers
        /* NOTE: java.lang.Class has a static initializer.  The clinit
                 will be done explicitly in JNI_CreateJavaVM(). */
    },
    /* java.lang.Class.newInstance() */
    {
	CVM_FALSE, /* nonstatic */
	CVMsystemClass(java_lang_Class),
        "newInstance", "()Ljava/lang/Object;",
        &CVMglobals.java_lang_Class_newInstance
        /* NOTE: java.lang.Class has a static initializer.  The clinit
                 will be done explicitly in JNI_CreateJavaVM(). */
    },
#ifdef CVM_CLASSLOADING
    /* java.lang.ClassLoader.NativeLibrary.getFromClass() */
    {
	CVM_TRUE,  /* static */
	CVMsystemClass(java_lang_ClassLoader_NativeLibrary),
        "getFromClass", "()Ljava/lang/Class;",
        &CVMglobals.java_lang_ClassLoader_NativeLibrary_getFromClass
        /* NOTE: java.lang.ClassLoader.NativeLibrary has a static initializer.
                 The clinit will be done explicitly in JNI_CreateJavaVM(). */
    },
    /* java.lang.ClassLoader.addClass() */
    {
	CVM_FALSE, /* nonstatic */
	CVMsystemClass(java_lang_ClassLoader),
        "addClass", "(Ljava/lang/Class;)V",
        &CVMglobals.java_lang_ClassLoader_addClass
        /* NOTE: java.lang.ClassLoader has a static initializer.  The clinit
                 will be done explicitly in JNI_CreateJavaVM(). */
    },
    /* java.lang.ClassLoader.loadClass() */
    {
	CVM_FALSE, /* nonstatic */
	CVMsystemClass(java_lang_ClassLoader),
        "loadClassInternal", "(Ljava/lang/String;)Ljava/lang/Class;",
        &CVMglobals.java_lang_ClassLoader_loadClass
        /* NOTE: java.lang.ClassLoader has a static initializer.  The clinit
                 will be done explicitly in JNI_CreateJavaVM(). */
    },
    /* java.lang.ClassLoader.findNative() */
    {
	CVM_TRUE,  /* static */
	CVMsystemClass(java_lang_ClassLoader),
        "findNative", "(Ljava/lang/ClassLoader;Ljava/lang/String;)J",
        &CVMglobals.java_lang_ClassLoader_findNative
        /* NOTE: java.lang.ClassLoader has a static initializer.  The clinit
                 will be done explicitly in JNI_CreateJavaVM(). */
    },
    /* java.lang.ClassLoader.checkPackageAccess() */
    {
	CVM_FALSE, /* nonstatic */
	CVMsystemClass(java_lang_ClassLoader),
        "checkPackageAccess", 
	"(Ljava/lang/Class;Ljava/security/ProtectionDomain;)V",
        &CVMglobals.java_lang_ClassLoader_checkPackageAccess
        /* NOTE: java.lang.ClassLoader has a static initializer.  The clinit
                 will be done explicitly in JNI_CreateJavaVM(). */
    },
    /* java.lang.ClassLoader.loadBootstrapClass() */
    {
	CVM_TRUE, /* static */
	CVMsystemClass(java_lang_ClassLoader),
        "loadBootstrapClass", 
	"(Ljava/lang/String;)Ljava/lang/Class;",
        &CVMglobals.java_lang_ClassLoader_loadBootstrapClass
        /* NOTE: java.lang.ClassLoader has a static initializer.  The clinit
                 will be done explicitly in JNI_CreateJavaVM(). */
    },
    /* java.lang.Class.loadSuperClasses() */
    {
	CVM_FALSE, /* nonstatic */
	CVMsystemClass(java_lang_Class),
        "loadSuperClasses", "()V",
        &CVMglobals.java_lang_Class_loadSuperClasses
    },
#endif
    /* java.lang.Thread.exit() */
    {
	CVM_FALSE, /* nonstatic */
	CVMsystemClass(java_lang_Thread),
        "exit", "(Ljava/lang/Throwable;)V",
        &CVMglobals.java_lang_Thread_exit
        /* NOTE: java.lang.Thread has a static initializer.  The clinit
                 will be done explicitly in JNI_CreateJavaVM(). */
    },
    /* java.lang.Thread.initMainThread() */
    {
	CVM_TRUE,  /* nonstatic */
	CVMsystemClass(java_lang_Thread),
        "initMainThread", "(IJ)V",
        &CVMglobals.java_lang_Thread_initMainThread
        /* NOTE: java.lang.Thread has a static initializer.  The clinit
                 will be done explicitly in JNI_CreateJavaVM(). */
    },
    /* java.lang.Thread.initAttachedThread() */
    {
	CVM_TRUE,  /* static */
	CVMsystemClass(java_lang_Thread),
        "initAttachedThread",
	    "(Ljava/lang/ThreadGroup;Ljava/lang/String;IJZ)"
	    "Ljava/lang/Thread;",
        &CVMglobals.java_lang_Thread_initAttachedThread
        /* NOTE: java.lang.Thread has a static initializer.  The clinit
                 will be done explicitly in JNI_CreateJavaVM(). */
    },
    /* java.lang.Thread.nextThreadNum() */
    {
	CVM_TRUE,  /* static */
	CVMsystemClass(java_lang_Thread),
        "nextThreadNum", "()I",
        &CVMglobals.java_lang_Thread_nextThreadNum,
        /* NOTE: java.lang.Thread has a static initializer.  The clinit
                 will be done explicitly in JNI_CreateJavaVM(). */
    },

#ifdef CVM_AOT
    /* sun.misc.Warmup.runit() */
    {
        CVM_TRUE, /* static */
        CVMsystemClass(sun_misc_Warmup),
        "runit", "(Ljava/lang/String;Ljava/lang/String;)V",
        &CVMglobals.sun_misc_Warmup_runit,
    },
#endif

#ifdef CVM_JVMPI
    /* java.lang.Thread.initDaemonThread() */
    {
	CVM_TRUE,  /* nonstatic */
	CVMsystemClass(java_lang_Thread),
        "initDaemonThread", "(Ljava/lang/String;I)Ljava/lang/Thread;",
        &CVMglobals.java_lang_Thread_initDaemonThread
        /* NOTE: java.lang.Thread has a static initializer.  The clinit
                 will be done explicitly in JNI_CreateJavaVM(). */
    },
#endif

    /**********************************************************
     * The following are for native application class loading.
     **********************************************************/
    /* sun.misc.Launcher.AppClassLoader.setExtInfo() */
    {
	CVM_TRUE,  /* static */
	CVMsystemClass(sun_misc_Launcher_AppClassLoader),
        "setExtInfo", "()V",
        &CVMglobals.sun_misc_Launcher_AppClassLoader_setExtInfo
        /* NOTE: sun.misc.Launcher.AppClassLoader has a static initializer.
                 The clinit will be checked for and done at the site of use.*/
    },
    /* sun.misc.Launcher.ClassContainer.<init>(URL, String, Jarfile) */
    {
	CVM_FALSE, /* nonstatic */
	CVMsystemClass(sun_misc_Launcher_ClassContainer),
        "<init>", 
	    "(Ljava/net/URL;"
	    "Ljava/lang/String;"
	    "Ljava/util/jar/JarFile;)V",
        &CVMglobals.sun_misc_Launcher_ClassContainer_init
        /* NOTE: sun.misc.Launcher.ClassContainer has no static initializer.
                 Hence, no clinit needed. */
    },
    /* sun.misc.Launcher.ClassContainer.<init>(URL, Class, String, Jarfile) */
    {
	CVM_FALSE, /* nonstatic */
	CVMsystemClass(sun_misc_Launcher_ClassContainer),
        "<init>", 
	    "(Ljava/net/URL;"
	    "Ljava/lang/Class;"
	    "Ljava/lang/String;"
	    "Ljava/util/jar/JarFile;)V",
        &CVMglobals.sun_misc_Launcher_ClassContainer_init_withClass
        /* NOTE: sun.misc.Launcher.ClassContainer has no static initializer.
                 Hence, no clinit needed. */
    },
    /* sun.misc.Launcher.getFileURL(File) */
    {
	CVM_TRUE,  /* static */
	CVMsystemClass(sun_misc_Launcher),
        "getFileURL", "(Ljava/io/File;)Ljava/net/URL;",
        &CVMglobals.sun_misc_Launcher_getFileURL
        /* NOTE: sun.misc.Launcher has a static initializer.  The clinit will
                 be checked for and done at the site of use. */
    },
    /* java.io.FILE.<init>(File) */
    {
	CVM_FALSE, /* nonstatic */
	CVMsystemClass(java_io_File),
        "<init>", "(Ljava/lang/String;)V",
        &CVMglobals.java_io_File_init
        /* NOTE: java.io.FILE has a static initializer.  The clinit will be
                 checked for and done at the site of use. */
    },
    /* java.util.jar.JarFile.<init>(File) */
    {
	CVM_FALSE, /* nonstatic */
	CVMsystemClass(java_util_jar_JarFile),
        "<init>", "(Ljava/io/File;)V",
        &CVMglobals.java_util_jar_JarFile_init
        /* NOTE: java.util.jar.JarFile's superclass has a static initializer.
                 The clinit will be checked for and done at the site of use. */
    },
    /* java.security.CodeSource.<init>(String)  */
    {
	CVM_FALSE, /* nonstatic */
	CVMsystemClass(java_security_CodeSource),
        "<init>", "(Ljava/net/URL;[Ljava/security/cert/Certificate;)V",
        &CVMglobals.java_security_CodeSource_init
        /* NOTE: java.security.CodeSource has no static initializer.
                 Hence, no clinit needed. */
    },
    /* java.security.SecureClassLoader.getProtectionDomain(CodeSource) */
    {
	CVM_FALSE, /* nonstatic */
	CVMsystemClass(java_security_SecureClassLoader),
        "getProtectionDomain", 
	"(Ljava/security/CodeSource;)Ljava/security/ProtectionDomain;",
        &CVMglobals.java_security_SecureClassLoader_getProtectionDomain
        /* NOTE: java.security.SecureClassLoader has a static initializer, but
                 does not need a clinit check because this mb is for a virtual
                 method i.e. the class was already initialized previously
                 when the object was instantiated. */
    },
    /* java.lang.ClassLoader.checkCerts */
    {
	CVM_FALSE, /* nonstatic */
	CVMsystemClass(java_lang_ClassLoader),
        "checkCerts", "(Ljava/lang/String;Ljava/security/CodeSource;)V",
        &CVMglobals.java_lang_ClassLoader_checkCerts
        /* NOTE: java.lang.ClassLoader has a static initializer.
                 Hence, no clinit needed. */
    },
};

static void
CVMdestroyGlobalSysMutexes(int n)
{
    int i;
    for (i = 0; i < n; i++) {
	CVMsysMutexDestroy(globalSysMutexes[i].mutex);
    }
}

static void
CVMdestroyMicroLocks(CVMMicroLock *locks, int n)
{
    int i;
    for (i = 0; i < n; ++i) {
	CVMmicrolockDestroy(&locks[i]);
    }
}

static void
CVMdestroyCstates(int n)
{
    int i;
    for (i = 0; i < n; ++i) {
	CVMcsDestroy(CVM_CSTATE(i));
    }
}

static CVMBool
initCstates()    
{
    int i;
    CVMUint8 rank = CVM_SYSMUTEX_MAX_RANK - CVM_NUM_CONSISTENT_STATES;
    
    for (i = 0; i < CVM_NUM_CONSISTENT_STATES; ++i) {
	if (!CVMcsInit(CVM_CSTATE(i), CVMcstateNames[i], rank)) {
	    CVMdestroyCstates(i);
	    return CVM_FALSE;
	}
	++rank;
    }
    return CVM_TRUE;
}

#define CVM_NUM_GLOBAL_METHODBLOCKS \
    (sizeof(globalMethodBlocks) / sizeof(CVMGlobalMethodBlockEntry))

static const CVMSubOptionData knownOptSubOptions[] = {

#ifdef CVM_HAS_PLATFORM_SPECIFIC_SUBOPTIONS
#include "javavm/include/opt_md.h"
#endif

    {"stackChunkSize", "Java stack chunk size",
	CVM_INTEGER_OPTION,
#ifdef CVM_JIT
	{{32, 64 * 1024, 2 * 1024}},
#else
	{{32, 64 * 1024, 1 * 1024}},
#endif
	&CVMglobals.config.javaStackChunkSize},

    {"stackMinSize", "Java stack minimum size",
	CVM_INTEGER_OPTION,
#ifdef CVM_JIT
	{{32, 64 * 1024, 3*1024}},
#else
	{{32, 64 * 1024, 1*1024}},
#endif
	&CVMglobals.config.javaStackMinSize},

    {"stackMaxSize", "Java stack maximum size",
	CVM_INTEGER_OPTION,
	{{1024, 1 * 1024 * 1024, 128 * 1024}},
	&CVMglobals.config.javaStackMaxSize},

    {NULL, NULL, CVM_NULL_OPTION, {{0, 0, 0}}, NULL}
};

void
CVMoptPrintUsage()
{
    CVMconsolePrintf("Valid -Xopt options include:\n");
    CVMprintSubOptionsUsageString(knownOptSubOptions);
}

CVMBool
CVMoptParseXoptOptions(const char* optAttributesStr)
{
    if (!CVMinitParsedSubOptions(&CVMglobals.parsedSubOptions,
				 optAttributesStr))
    {
	return CVM_FALSE;
    }

    if (!CVMprocessSubOptions(knownOptSubOptions, "-Xopt",
                              &CVMglobals.parsedSubOptions)) {
	CVMoptPrintUsage();
	return CVM_FALSE;
    }

    return CVM_TRUE;
}

CVMBool
CVMoptParseXssOption(const char* xssStr)
{
    CVMInt32 nativeStackSize = CVMoptionToInt32(xssStr);
    if (nativeStackSize == -1) {
	CVMconsolePrintf("Warning: Could not parse "
			 "stack size string \"%s\", setting default\n",
			 xssStr);
	/* gs->nativeStackSize remains 0 */
	return CVM_TRUE;
    } else if (nativeStackSize < CVM_THREAD_MIN_C_STACK_SIZE) {
	CVMconsolePrintf("Stack size %d below minimum stack size (%d)\n",
			 nativeStackSize, CVM_THREAD_MIN_C_STACK_SIZE);
	return CVM_FALSE;
    } else {
	CVMglobals.config.nativeStackSize = nativeStackSize;
	return CVM_TRUE;
    }
}

#if defined(CVM_DEBUG) || defined(CVM_INSPECTOR)
void CVMdumpGlobalsSubOptionValues()
{
    CVMprintSubOptionValues(knownOptSubOptions);
}
#endif /* CVM_DEBUG || CVM_INSPECTOR */

CVMBool CVMinitVMGlobalState(CVMGlobalState *gs, CVMOptions *options)
{
    CVMExecEnv *ee = &gs->mainEE;
    /*
     * First zero everything out
     */
    memset((void*)gs, 0, sizeof(CVMGlobalState));

    /*
     * Now initialize the global state
     */
    CVMtraceMisc(("Initializing global state\n"));

#if CVM_USE_MMAP_APIS
    if (!CVMmemInit()) {
	goto out_of_memory;
    }
#endif

    /* init interpreter loop function pointer */
    gs->CVMgcUnsafeExecuteJavaMethodProcPtr = &CVMgcUnsafeExecuteJavaMethod;

    CVMinitJNIStatics();
    
    gs->exit_procs = NULL;

#ifdef CVM_MTASK
    gs->isServer = options->isServer;
#endif

#ifdef CVM_JVMPI
    /* Initialize these 2 lists before anyone instantiates any monitors: */
    gs->objMonitorList = NULL;
    gs->rawMonitorList = NULL;
#endif
#ifdef CVM_JVMTI
    /* Initialize high res clocks */
    CVMtimeClockInit();
#endif
    /*
     * Random number generator
     */
    CVMrandomInit();

    CVMpreloaderInit();

    gs->threadList = NULL;
    gs->threadIDCount = 0;

#if defined(CVM_HAVE_DEPRECATED) || defined(CVM_THREAD_SUSPENSION)
    gs->suspendCheckerInitialized = CVM_FALSE;
#endif

#ifndef CDC_10
    /* transfer assertion related globals */
    gs->javaAssertionsUserDefault = options->javaAssertionsUserDefault;
    gs->javaAssertionsSysDefault  = options->javaAssertionsSysDefault;
    gs->javaAssertionsClasses     = options->javaAssertionsClasses;
    gs->javaAssertionsPackages    = options->javaAssertionsPackages;
#endif

    gs->unlimitedGCRoots = options->unlimitedGCRoots;

    /*
     * Initalize all of the global sys mutexes.
     */
    {
	CVMUint8 i;
	CVMassert(CVM_NUM_GLOBAL_SYSMUTEXES < 255);
	for (i = 0; i < CVM_NUM_GLOBAL_SYSMUTEXES; i++) {
	    if (!CVMsysMutexInit(globalSysMutexes[i].mutex,
				 globalSysMutexes[i].name,
				 (CVMUint8)(i+1) /* rank */))
	    {
		CVMdestroyGlobalSysMutexes(i);
		goto out_of_memory;
	    }
	}
    }

    if (!CVMcondvarInit(&gs->threadCountCV, &gs->threadLock.rmutex.mutex)) {
	goto out_of_memory;
    }

#ifdef CVM_INSPECTOR
    CVMgcLockerInit(&gs->inspectorGCLocker);
    if (!CVMcondvarInit(&gs->gcLockerCV, &gs->gcLockerLock.rmutex.mutex)) {
	goto out_of_memory;
    }
#endif

    /* NOTE: This is theoretically where the "checked" version of the
       JNI would be installed if we had it in our source tree.  Here,
       if JVMTI is present, we replace some functions in the JNI
       interface with the JVMTI's instrumented versions, which will
       call back into the debugger if native code uses the JNI to set
       or get a field. */

    /* NOTE: this should come BEFORE the first call to CVMinitExecEnv.
       This mutates the global JNI vector which later gets pointed to
       by the various execution environments. */
    /*
     * With JVMTI, we do the instrumentation when an agent connects
     */
#ifdef CVM_JVMTI
    CVMjvmtiSetDebugOption(options->debugging);
#endif    

#ifdef CVM_LVM /* %begin lvm */
    /* Logical VM global variables initialization (phase 1) 
     * Do this before invoking CVMinitExecEnv().
     * Phase 2 is taken place after initializing the main Logical VM */
    if (!CVMLVMglobalsInitPhase1(&gs->lvm)) {
	goto out_of_memory;
    }
#endif /* %end lvm */

    gs->userThreadCount = 1;
    gs->threadCount = 1;

    if (!initCstates()) {
	goto out_of_memory;
    }

    /*
     * Default sizes
     */

    if (!CVMoptParseXoptOptions(options->optAttributesStr)) {
	return CVM_FALSE;
    }

#if defined(CVM_DEBUG)
    CVMconsolePrintf("CVM Configuration:\n");
    CVMdumpGlobalsSubOptionValues();
#endif /* CVM_DEBUG */

    /* In the future, this should be map to a -Xopt sub-option */
    if (options->nativeStackSizeStr != NULL) {
	if (!CVMoptParseXssOption(options->nativeStackSizeStr)) {
	    return CVM_FALSE;
	}
    }

#ifdef CVM_TRACE_ENABLED
    /*
     * Turn off all debugging flags by default.
     */

    if (options->traceFlagsStr != NULL) {
	gs->debugFlags = strtol(options->traceFlagsStr, NULL, 0);
    } else {
	gs->debugFlags = 0;
    }
    CVMtraceInit();
#endif /* CVM_TRACE_ENABLED */

    if (!CVMinitExecEnv(ee, ee, NULL)) {
	return CVM_FALSE;
    }
    if (!CVMattachExecEnv(ee, CVM_TRUE)) {
	goto badEEnoDetach;
    }

    if (!CVMinitGCRootStack(ee, &gs->globalRoots,
		       CVM_INITIAL_GLOBALROOTS_STACK_SIZE,
		       CVM_MAX_GLOBALROOTS_STACK_SIZE, 
		       CVM_MIN_GLOBALROOTS_STACKCHUNK_SIZE,
		       CVM_FRAMETYPE_GLOBALROOT)) {
	goto badEE;
    }

    if (!CVMinitGCRootStack(ee, &gs->weakGlobalRoots,
		       CVM_INITIAL_WEAK_GLOBALROOTS_STACK_SIZE,
		       CVM_MAX_WEAK_GLOBALROOTS_STACK_SIZE, 
		       CVM_MIN_WEAK_GLOBALROOTS_STACKCHUNK_SIZE,
		       CVM_FRAMETYPE_GLOBALROOT)) {
	goto badEE;
    }

    if (!CVMinitGCRootStack(ee, &gs->classGlobalRoots,
		       CVM_INITIAL_CLASS_GLOBALROOTS_STACK_SIZE,
		       CVM_MAX_CLASS_GLOBALROOTS_STACK_SIZE, 
		       CVM_MIN_CLASS_GLOBALROOTS_STACKCHUNK_SIZE,
		       CVM_FRAMETYPE_GLOBALROOT)) {
	goto badEE;
    }

    if (!CVMinitGCRootStack(ee, &gs->classLoaderGlobalRoots,
		       CVM_INITIAL_CLASSLOADER_GLOBALROOTS_STACK_SIZE,
		       CVM_MAX_CLASSLOADER_GLOBALROOTS_STACK_SIZE, 
		       CVM_MIN_CLASSLOADER_GLOBALROOTS_STACKCHUNK_SIZE,
		       CVM_FRAMETYPE_GLOBALROOT)) {
	goto badEE;
    }

    if (!CVMinitGCRootStack(ee, &gs->protectionDomainGlobalRoots,
		       CVM_INITIAL_PROTECTION_DOMAIN_GLOBALROOTS_STACK_SIZE,
		       CVM_MAX_PROTECTION_DOMAIN_GLOBALROOTS_STACK_SIZE, 
		       CVM_MIN_PROTECTION_DOMAIN_GLOBALROOTS_STACKCHUNK_SIZE,
		       CVM_FRAMETYPE_GLOBALROOT)) {
	goto badEE;
    }

    if (!CVMinitStack(ee, &gs->classTable,
		 CVM_INITIAL_CLASSTABLE_STACK_SIZE,
		 CVM_MAX_CLASSTABLE_STACK_SIZE, 
		 CVM_MIN_CLASSTABLE_STACKCHUNK_SIZE,
		 CVMfreelistFrameCapacity, 
		 CVM_FRAMETYPE_CLASSTABLE)) {
	goto out_of_memory;
    }

    CVMfreelistFrameInit((CVMFreelistFrame*)gs->classTable.currentFrame,
                         CVM_FALSE);

    {
	int i;

        for (i = 0; i < CVM_NUM_SYS_MICROLOCKS; ++i) {
            if (!CVMmicrolockInit(&gs->sysMicroLock[i])) {
		CVMdestroyMicroLocks(gs->sysMicroLock, i);
		goto out_of_memory;
	    }
        }
	if (!CVMmicrolockInit(&gs->objGlobalMicroLock)) {
	    CVMmicrolockDestroy(&gs->objGlobalMicroLock);
	    goto out_of_memory;
	}
    }

#ifdef CVM_CCM_COLLECT_STATS
    CVMglobals.ccmStats.okToCollectStats = CVM_TRUE;
#endif

    /*
     * Initialize the target global state
     */
    if (!CVMinitVMTargetGlobalState()) {
	CVMconsolePrintf("Cannot start VM "
			 "(target global state failed to initialize)\n");
	return CVM_FALSE;
    }

#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
    /* This must have bene done by CVMinitVMTargetGlobalState */
    CVMassert(gs->jit.gcTrapAddr != NULL);
#endif

    /*
     * Initialize the particular GC global state
     */
    CVMgcimplInitGlobalState(&gs->gc);

    /*
     * JNI JavaVM state
     */
    CVMinitJNIJavaVM(&gs->javaVM);

    /*
     * TypeId system, which also registers well-known types.
     */
    if (!CVMtypeidInit(ee)) {
        goto out_of_memory;
    }

    /*
     * Intern table, which just has a global variable to set.
     */
    CVMinternInit();
    
    /*
     * Classes system
     */
    if (!CVMclassModuleInit(ee)) {
	return CVM_FALSE;
    }
#ifdef CVM_CLASSLOADING
    gs->classVerificationLevel = options->classVerificationLevel;
#ifdef CVM_SPLIT_VERIFY
    gs->splitVerify = options->splitVerify;
#endif
#endif

    /*
     * Now that both typeid and class system are initialized,
     * register pre-loaded packages.
     */
    CVMtypeidRegisterPreloadedPackages();

    /*
     * Stackmap computer
     */
    CVMstackmapComputerInit();

#ifdef CVM_TIMESTAMPING
    /*
     * Initializes the first wall clock time
     */
    CVMtimeStampWallClkInit();
#endif

    /*
     * Weak reference handling initialization
     */
    CVMweakrefInit();

    /*
     * Initialize CVMMethodBlock*'s in the CVMglobals.
     */
    {
	int i;
	for (i = 0; i < CVM_NUM_GLOBAL_METHODBLOCKS; i++) {
	    CVMMethodTypeID methodTypeID =  
		CVMtypeidLookupMethodIDFromNameAndSig(
		    ee, 
                    globalMethodBlocks[i].methodName, 
		    globalMethodBlocks[i].methodSig);
	    CVMMethodBlock* mb =
		CVMclassGetMethodBlock(globalMethodBlocks[i].cb,
				       methodTypeID,
				       globalMethodBlocks[i].isStatic);
	    CVMassert(mb != NULL);
	    if (globalMethodBlocks[i].mbPtr != NULL) {
		*globalMethodBlocks[i].mbPtr = mb;
	    }
	}
    }

    /* 
     * Lookup fb for java.lang.ref.Reference.lock. GC has to know about
     * this lock to handle weak references correctly.
     */
    {
	CVMFieldTypeID lockTID = CVMtypeidLookupFieldIDFromNameAndSig(
            ee, "lock", "Ljava/lang/Object;");
	CVMFieldTypeID pendingTID = CVMtypeidLookupFieldIDFromNameAndSig(
            ee, "pending", "Ljava/lang/ref/Reference;");
	const CVMClassBlock* cb = CVMsystemClass(java_lang_ref_Reference);
	CVMFieldBlock* lockFB = 
	    CVMclassGetStaticFieldBlock(cb, lockTID);
	CVMFieldBlock* pendingFB = 
	    CVMclassGetStaticFieldBlock(cb, pendingTID);
	CVMassert(lockFB != NULL);
	CVMassert(pendingFB != NULL);
	gs->java_lang_ref_Reference_lock = &CVMfbStaticField(ee, lockFB);
	gs->java_lang_ref_Reference_pending = &CVMfbStaticField(ee, pendingFB);
    }

#ifdef CVM_DEBUG_STACKTRACES
#ifdef CVM_DEBUG
    /* 
     * Lookup fb for System.out. This is useful for debugging sessions
     * if you have a Throwable object that you want to pass to
     * CVMprintStackTrace, since it requires that you pass it an object
     * that implements println().
     */
    {
	CVMFieldTypeID fieldTypeID = CVMtypeidLookupFieldIDFromNameAndSig(
            ee, "out", "Ljava/io/PrintStream;");
	const CVMClassBlock* cb = CVMsystemClass(java_lang_System);
	gs->java_lang_System_out = 
	    CVMclassGetStaticFieldBlock(cb, fieldTypeID);
	CVMassert(gs->java_lang_System_out != NULL);
    }
#endif /* CVM_DEBUG */
#endif /* CVM_DEBUG_STACKTRACES */

    /* For GC statistics */
    gs->measureGC = CVM_FALSE;
    gs->totalGCTime = CVMint2Long(0);

#ifdef CVM_JIT
    if (!CVMjitInit(ee, &gs->jit, options->jitAttributesStr)) {
	CVMconsolePrintf("Cannot start VM "
			 "(jit failed to initialize)\n");
	return CVM_FALSE;
    }
#endif

    if (!CVMgcInitHeap(options)) {
	goto out_of_memory;
    }

    if (!CVMeeSyncInitGlobal(ee, gs)) {
        goto out_of_memory;
    }

    /*
     * Some desperation exception objects we preallocate.
     */
    gs->preallocatedOutOfMemoryError = CVMID_getGlobalRoot(ee);
    gs->preallocatedStackOverflowError = CVMID_getGlobalRoot(ee);
    if (gs->preallocatedOutOfMemoryError == NULL ||
	gs->preallocatedStackOverflowError == NULL) {
	return CVM_FALSE;
    }
    CVMID_allocNewInstance(ee, CVMsystemClass(java_lang_OutOfMemoryError),
			   gs->preallocatedOutOfMemoryError);
    CVMID_allocNewInstance(ee, CVMsystemClass(java_lang_StackOverflowError),
			   gs->preallocatedStackOverflowError);
    if (CVMID_icellIsNull(gs->preallocatedOutOfMemoryError) ||
	CVMID_icellIsNull(gs->preallocatedStackOverflowError)) {
	return CVM_FALSE;
    }

    /*
     * Initialize classpath pathString
     */
#ifdef CVM_CLASSLOADING
    if (options->bootclasspathStr != NULL) {
        gs->bootClassPath.pathString = strdup(options->bootclasspathStr);
    }
    if (options->appclasspathStr != NULL) {
        gs->appClassPath.pathString = strdup(options->appclasspathStr);
    }
#endif

    /*
     * Hooks
     */
    gs->vfprintfHook = options->vfprintfHook;
    gs->exitHook = (void (*)(int))options->exitHook;
    gs->abortHook = (void (*)())options->abortHook;
    gs->safeExitHook = (void (*)(int))options->safeExitHook;
#ifdef CVM_TIMESTAMPING
    gs->timeStampEnabled = options->timeStampEnabled;
#endif

#ifdef CVM_HAVE_PROCESS_MODEL
    gs->fullShutdown = options->fullShutdownFlag;
#else
    gs->fullShutdown = CVM_TRUE;
#endif

    gs->hangOnStartup = options->hangOnStartup;

#ifdef CVM_JVMTI
    /* jvmti global variables initialization */
    CVMjvmtiInitializeGlobals(&gs->jvmti);
#endif

#ifdef CVM_JVMPI
    CVMjvmpiInitializeGlobals(ee, &gs->jvmpiRecord);
#endif

    /* Open up the cvm binary as a shared library for locating JNI methods */
    if (CVMglobals.cvmDynHandle == NULL) {
        CVMglobals.cvmDynHandle = CVMdynlinkOpen(NULL);
        if (CVMglobals.cvmDynHandle == NULL) {
            CVMconsolePrintf("Cannot start VM "
                             "(Could not open cvm as a shared library)\n");
            return CVM_FALSE;
        }
    }

#ifdef CVM_JIT
    /* Initialize the invoker cost of all ROMized methods */
    CVMpreloaderInitInvokeCost();
#endif
    return CVM_TRUE;

badEE:
    CVMdetachExecEnv(ee);
badEEnoDetach:
    CVMdestroyExecEnv(ee);
    CVMremoveThread(ee, ee->userThread);

out_of_memory:

    CVMconsolePrintf("Cannot start VM "
		     "(out of memory while initializing)\n");
    return CVM_FALSE;
}

void CVMdestroyVMGlobalState(CVMExecEnv *ee, CVMGlobalState *gs)
{
    /* These should be roughly in the reverse of initialization order */

#ifdef CVM_JIT
    CVMjitDestroy(&gs->jit);
#endif

#ifdef CVM_LVM /* %begin lvm */
    /* Logical VM global variables deinitialization.
     * Do this where 'ee' is still valid. */
    CVMLVMglobalsDestroy(ee, &gs->lvm);
#endif /* %end lvm */

#ifdef CVM_JVMPI
    CVMjvmpiDestroyGlobals(ee, &gs->jvmpiRecord);
#endif

#ifdef CVM_JVMTI
    /* Free jvmti global variables */
    CVMjvmtiDestroyGlobals(&gs->jvmti);
#endif

    /* 
     * Destroy dynamically loaded classes.
     */
    CVMclassModuleDestroy( ee );


#ifdef CVM_DYNAMIC_LINKING
    if (CVMglobals.cvmDynHandle != NULL) {
	CVMdynlinkClose(CVMglobals.cvmDynHandle);
    }
#endif

    if (gs->preallocatedOutOfMemoryError != NULL) {
	CVMID_freeGlobalRoot(ee, gs->preallocatedOutOfMemoryError);
    }
    if (gs->preallocatedStackOverflowError != NULL) {
	CVMID_freeGlobalRoot(ee, gs->preallocatedStackOverflowError);
    }

#if defined(CVM_HAVE_DEPRECATED) || defined(CVM_THREAD_SUSPENSION)
    if (CVMglobals.suspendCheckerInitialized) {
	CVMsuspendCheckerDestroy();
	CVMglobals.suspendCheckerInitialized = CVM_FALSE;
    }
#endif

    CVMeeSyncDestroyGlobal(ee, gs);

    CVMgcDestroyHeap();

#ifdef CVM_INSPECTOR
    CVMcondvarDestroy(&gs->gcLockerCV);
    CVMgcLockerDestroy(&gs->inspectorGCLocker);
#endif
    CVMcondvarDestroy(&gs->threadCountCV);

    CVMdetachExecEnv(ee);
    CVMdestroyExecEnv(ee);
    CVMremoveThread(ee, ee->userThread);

    /*
     * destroy all of the global sys mutexes.
     */
    CVMdestroyGlobalSysMutexes(CVM_NUM_GLOBAL_SYSMUTEXES);

    CVMdestroyGCRootStack(&gs->globalRoots);
    CVMdestroyGCRootStack(&gs->weakGlobalRoots);
    CVMdestroyGCRootStack(&gs->classGlobalRoots);
    CVMdestroyGCRootStack(&gs->classLoaderGlobalRoots);
    CVMdestroyGCRootStack(&gs->protectionDomainGlobalRoots);
    CVMdestroyStack(&gs->classTable);

#ifdef CVM_CCM_COLLECT_STATS
    CVMglobals.ccmStats.okToCollectStats = CVM_FALSE;
#endif

    CVMdestroyCstates(CVM_NUM_CONSISTENT_STATES);

    CVMdestroyMicroLocks(gs->sysMicroLock, CVM_NUM_SYS_MICROLOCKS);
    CVMmicrolockDestroy(&gs->objGlobalMicroLock);

    CVMdestroyVMTargetGlobalState();

    CVMgcimplDestroyGlobalState(&gs->gc);

    CVMdestroyJNIJavaVM(&gs->javaVM);

    /*
     * Destroy class related data structures
     * Including those for preloaded classes.
     */
    CVMpreloaderDestroy();
    CVMtypeidDestroy();
    CVMinternDestroy();


    /*
     * Destroy stackmap computer related data structures
     */
    CVMstackmapComputerDestroy();

/* Java SE Zip doesn't have this function.  Comment out for now.
   FIXME - make sure this doesn't introduce a leak.
*/
#ifndef JAVASE
    /*
     * Destroy zip_util related data structures
     */
    CVMzutilDestroyZip();
#endif

#ifndef CDC_10
    /* free assertion command line options */
    CVMJavaAssertions_freeOptions();
#endif

    /*
     * Free classpath pathString
     */
#ifdef CVM_CLASSLOADING
    if (gs->bootClassPath.pathString != NULL) {
        free(gs->bootClassPath.pathString);
    }
    if (gs->appClassPath.pathString != NULL) {
        free(gs->appClassPath.pathString);
    }
#endif

    CVMdestroyJNIStatics();
    
#ifdef CVM_JVMPI
    /* These 2 lists should have been cleaned out by now: */
    CVMassert(gs->objMonitorList == NULL);
    CVMassert(gs->rawMonitorList == NULL);
#endif
}

#if defined(CVM_HAVE_DEPRECATED) || defined(CVM_THREAD_SUSPENSION)

void
CVMglobalSysMutexesAcquire(CVMExecEnv* ee)
{
    /*
     * grab all of the global sys mutexes in rank order
     */
    {
	int i;
	for (i = 0; i < CVM_NUM_GLOBAL_SYSMUTEXES; i++) {
	    CVMsysMutexLock(ee, globalSysMutexes[i].mutex);
	}
    }
}

void
CVMglobalSysMutexesRelease(CVMExecEnv* ee)
{
    /*
     * release all of the global sys mutexes in reverse rank order
     */
    {
	int i;
	for (i = CVM_NUM_GLOBAL_SYSMUTEXES - 1; i >= 0; i--) {
	    CVMsysMutexUnlock(ee, globalSysMutexes[i].mutex);
	}
    }
}

#endif /* defined(CVM_HAVE_DEPRECATED) || defined(CVM_THREAD_SUSPENSION) */
