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

#ifndef _INCLUDED_GLOBALS_H
#define _INCLUDED_GLOBALS_H

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/stacks.h"
#include "javavm/include/porting/globals.h"
#include "javavm/include/sync.h"
#include "javavm/include/cstates.h"
#include "javavm/include/jni_impl.h"
#include "javavm/include/packages.h"
#include "javavm/include/utils.h"
#include "javavm/include/jvmtiExport.h"
#include "javavm/include/jvmpi_impl.h"
#ifdef CVM_XRUN
#include "javavm/include/xrun.h"
#endif
#ifdef CVM_AGENTLIB
#include "javavm/include/agentlib.h"
#endif
#ifdef CVM_INSPECTOR
#include "javavm/include/inspector.h"
#endif
#ifdef CVM_JIT
#include "javavm/include/jit_common.h"
#ifdef CVM_CCM_COLLECT_STATS
#include "javavm/include/ccm_runtime.h"
#endif /* CVM_CCM_COLLECT_STATS */
#endif /* CVM_JIT */

/*
 * This file is generated from the GC choice given at build time.
 */
#include "generated/javavm/include/gc_config.h"

typedef void (*pProc)(void);
typedef struct exit_proc {
    pProc proc;
    struct exit_proc *next;
} * exit_procPtr;

typedef void (*loopProcPtr)(CVMExecEnv*, CVMMethodBlock *, CVMBool, CVMBool);

struct CVMOptions {
    void *vfprintfHook;
    void *exitHook;
    void *abortHook;
    void *safeExitHook;
#ifdef CVM_TIMESTAMPING
    CVMBool timeStampEnabled;
#endif

    const char *startHeapSizeStr;
    const char *minHeapSizeStr;
    const char *maxHeapSizeStr;
    const char *nativeStackSizeStr;
    const char *gcAttributesStr;
    const char *optAttributesStr;
#ifdef CVM_JIT
    const char *jitAttributesStr;
#endif

#ifdef CVM_TRACE_ENABLED
    const char *traceFlagsStr;
#endif
#ifdef CVM_JVMTI
    CVMBool debugging;
#endif
#ifdef CVM_CLASSLOADING
    CVMUint16 classVerificationLevel;
    const char *bootclasspathStr;
    const char *appclasspathStr;
#ifdef CVM_SPLIT_VERIFY
    CVMBool splitVerify;
#endif
#endif
#ifdef CVM_HAVE_PROCESS_MODEL
    CVMBool fullShutdownFlag;
#endif
    CVMBool hangOnStartup;
    CVMBool unlimitedGCRoots;
#ifndef CDC_10
    CVMBool javaAssertionsUserDefault; /* User class default (-ea/-da). */
    CVMBool javaAssertionsSysDefault;  /* System class default (-esa/-dsa). */
    /* classes with assertions enabled or disabled */
    CVMJavaAssertionsOptionList* javaAssertionsClasses;
    /* packages with assertions enabled or disabled */
    CVMJavaAssertionsOptionList* javaAssertionsPackages;
#endif
#ifdef CVM_MTASK
    CVMBool isServer;
#endif
};

struct CVMGlobalState {
    /*
     * Global variables go here.
     */

    /*
     * The allocation pointer and limit. These are used for fast heap
     * allocation from assembler code.
     */
    CVMUint32**     allocPtrPtr;
    CVMUint32**     allocTopPtr;

#ifdef CVM_ADV_ATOMIC_SWAP
    /* This is the heap lock if atomic swap is supported */
    /* 
     * fastHeapLock is used in 
     * CVMatomicSwap(&fastHeapLock, 1) == 0
     * which requires a CVMAddr *
     */
    volatile CVMAddr fastHeapLock;
#else
    CVMAddr unused1; /* keep offsets consistent */
#endif

#ifdef CVM_TRACE_ENABLED
    CVMUint32 debugFlags;
#else
    CVMUint32 unused2; /* keep offsets consistent */
#endif

#ifdef CVM_TRACE_JIT
    CVMUint32 debugJITFlags;
#else
    CVMUint32 unused3; /* keep offsets consistent */
#endif

    CVMUint32 unused4; /* make sure cstate has 8-byte alignment */

    /* shared data for consistent states */
    CVMCState cstate[CVM_NUM_CONSISTENT_STATES];

    CVMMicroLock objGlobalMicroLock;

    CVMSysMutex globalRootsLock; /* protect globalRoots */
    CVMStack globalRoots;	 /* stack for allocating global roots,
				    used for JNI global refs and CVM
				    global roots. */

    CVMSysMutex weakGlobalRootsLock; /* protect weakGlobalRoots */
    CVMStack weakGlobalRoots;	 /* stack for allocating JNI weak
                                    global refs */

    /*
     * Class and ClassLoader global roots:
     * 
     * The CVMcbJavaInstance and CVMcbClassLoader fields can't be
     * regular global roots, or gc would have to make special checks
     * on each global root in order to make class unloading work. It
     * would need to check if the global root is a Class or ClassLoader
     * instance, and skip them on full gc's (which do class unloading).
     *
     * The classGlobalRoots are for CVMcbJavaInstance fields and the
     * classLoaderGlobalRoots are for CVMcbClassLoader fields. The latter
     * only has one allocated per ClassLoader, which will be shared among
     * all classes loaded by the ClassLoader. Access to both types of roots
     * is protected using the classTable lock.
     */
    CVMStack classGlobalRoots;	      /* for allocating Class global roots */
    CVMStack classLoaderGlobalRoots;  /* for allocating ClassLoader global */

    /*
     * The CVMcbProtectionDomain field can't be a regular global root.
     * Since a ProtectionDomain (now) points to a ClassLoader, the ClassLoader
     * points to the Class, and the class points to the ProtectionDomain
     * via this global root, it would make class unloading impossible.
     * So we set up a different set of global roots which are marked
     * when their referencing classes are scanned.
     */
    CVMStack protectionDomainGlobalRoots;

     
				    
    /*
     * Table of all dynamically loaded classes. Protected by the 
     * classTable lock.
     */
    CVMStack    classTable;

    /*
     * Lists of classes to and ClassLoader globa roots to free. These lists
     * are setup by CVMclassDoClassUnloadingPass1() and processed by
     * CVMclassDoClassUnloadingPass2().
     */
    CVMClassBlock*       freeClassList;
    CVMClassLoaderICell* freeClassLoaderList;

    /*
     * ClassTable lock - It not only protects the classTable, but also
     * the classGlobalRoots and  classLoaderGlobalRoots.
     */
    CVMSysMutex classTableLock; 
#define CVM_CLASSTABLE_LOCK(ee)   \
    CVMsysMutexLock(ee, &CVMglobals.classTableLock)
#define CVM_CLASSTABLE_UNLOCK(ee) \
    CVMsysMutexUnlock(ee, &CVMglobals.classTableLock)
#define CVM_CLASSTABLE_ASSERT_LOCKED(ee) \
    CVMassert(CVMsysMutexIAmOwner(ee, &CVMglobals.classTableLock));

    /*
     * Cache of all <Class,ClassLoader> pairs that we have loaded. Protected
     * by the loaderCache lock.
     */
    CVMLoaderCacheEntry** loaderCache;

    /*
     * Database of loader constraints. See loaderconstraints.c for details.
     * Protected by the loaderCache lock.
     */
#ifdef CVM_CLASSLOADING
    CVMLoaderConstraint** loaderConstraints;
#endif

    /*
     * loaderCache lock - It  protects the loaderCache and loaderConstraints
     */
    CVMSysMutex loaderCacheLock; 
#define CVM_LOADERCACHE_LOCK(ee)   \
    CVMsysMutexLock(ee, &CVMglobals.loaderCacheLock)
#define CVM_LOADERCACHE_UNLOCK(ee) \
    CVMsysMutexUnlock(ee, &CVMglobals.loaderCacheLock)
#define CVM_LOADERCACHE_ASSERT_LOCKED(ee) \
    CVMassert(CVMsysMutexIAmOwner(ee, &CVMglobals.loaderCacheLock));
#define CVM_LOADERCACHE_ASSERT_UNLOCKED(ee) \
    CVMassert(!CVMsysMutexIAmOwner(ee, &CVMglobals.loaderCacheLock));

    CVMSysMutex heapLock;	 /* The memory-related lock */


#ifdef CVM_CLASSLOADING
    CVMSysMutex nullClassLoaderLock; /* The NULL classloader lock */
#endif
    
#ifdef CVM_JVMTI
    CVMSysMutex jvmtiLock;
    CVMSysMutex jvmtiLockInfoLock;
#endif

    /*
     * Target global variables
     */
    CVMTargetGlobalState target;

    /*
     * Maximum heap size for CVM - used at Runtime.maxMemory()
     */
    CVMUint32 maxHeapSize;

#ifdef CVM_INSPECTOR
    /* See gc_common.h the GC locker mechanism: */
    CVMGCLocker inspectorGCLocker;
    CVMCondVar gcLockerCV;  /* Used in conjuction with the gcLockerLock. */
#endif
#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI) || defined(CVM_JVMTI)
    CVMSysMutex gcLockerLock;
#endif

    /*
     * GC specific global state
     */
    CVMGCCommonGlobalState gcCommon;
    CVMGCGlobalState gc;

#ifdef CVM_JIT
#ifdef CVM_SEGMENTED_HEAP
    /* 
     * Bounds of youngest generation 
     * used while generating write barriers in JIT 
     */  
    CVMUint32* youngGenLowerBound;
    CVMUint32* youngGenUpperBound;
#endif
 
    /*
     * JIT specific global state
     */
    CVMJITGlobalState jit;
    CVMSysMutex       jitLock;
#ifdef CVM_CCM_COLLECT_STATS
    CVMCCMGlobalStats ccmStats;
#endif /* CVM_CCM_COLLECT_STATS */
#endif /* CVM_JIT */

    /*
     * JNI JavaVM state
     */
    CVMJNIJavaVM javaVM;

    /*
     * The execution environment for the main thread.
     * Although the mainEE is embedded in CVMGlobalState structure
     * for convenience of initialization, its life cycle is independent
     * from CVMglobals.  mainEE gets destroyed when the main thread 
     * (that created the JVM) terminates, which can occur prior to 
     * the destruction of CVMglobals (which happens at the termination 
     * of the JVM.
     */
    CVMExecEnv mainEE;

    /*
     * Thread list
     */
    CVMSysMutex threadLock;	/* protect threadList */
    CVMExecEnv *threadList;	/* linked-list of threads */

    CVMUint32 userThreadCount;
    CVMUint32 threadCount;
    CVMCondVar threadCountCV;

    CVMUint32 threadIDCount;

    CVMSysMutex syncLock;		/* Object monitor "inflation" */

    CVMObjMonitor *objLocksBound;	/* bound to object */
    CVMObjMonitor *objLocksUnbound;
    CVMObjMonitor *objLocksFree;	/* free locks */

    CVMMicroLock sysMicroLock[CVM_NUM_SYS_MICROLOCKS];

    /*
     * Well-known ROMizer generated structures
     */

    /*
     * Well-known types and methods
     */
    CVMMethodTypeID initTid;
    CVMMethodTypeID clinitTid;
    CVMMethodTypeID finalizeTid;
    CVMMethodTypeID cloneTid;

    /* AccessController.doPrivileged() */
    CVMMethodBlock *java_security_AccessController_doPrivilegedAction1;
    CVMMethodBlock *java_security_AccessController_doPrivilegedExceptionAction1;
    CVMMethodBlock *java_security_AccessController_doPrivilegedAction2;
    CVMMethodBlock *java_security_AccessController_doPrivilegedExceptionAction2;

    /* The Finalizer.register() static method is called on all non-trivially
       finalizable objects. */
    CVMMethodBlock* java_lang_ref_Finalizer_register;

    /*
     * The lock for synchronizing with the reference handler thread 
     */
    CVMJavaVal32*   java_lang_ref_Reference_lock;

    /*
     * The list of java.lang.ref.Reference's waiting to be enqueued
     */
    CVMJavaVal32*   java_lang_ref_Reference_pending;

    /*
     * Does the reference handler thread need to do any work after GC?
     */
    CVMBool         referenceWorkTODO;

    CVMMethodBlock* java_lang_Class_runStaticInitializers;
    CVMMethodBlock* java_lang_Class_newInstance;
#ifdef CVM_REFLECT
    CVMMethodBlock* java_lang_reflect_Constructor_newInstance;
    CVMMethodBlock* java_lang_reflect_Method_invoke;
#endif /* CVM_REFLECT */
#ifdef CVM_DEBUG_STACKTRACES
    CVMMethodTypeID printlnTid;
    CVMMethodTypeID getCauseTid;
    CVMMethodBlock* java_lang_Throwable_fillInStackTrace;
#ifdef CVM_DEBUG
    CVMFieldBlock*  java_lang_System_out;
#endif
#endif /* CVM_DEBUG_STACKTRACES */
#ifdef CVM_CLASSLOADING
    CVMMethodBlock* java_lang_ClassLoader_NativeLibrary_getFromClass;
    CVMMethodBlock* java_lang_ClassLoader_addClass;
    CVMMethodBlock* java_lang_ClassLoader_loadClass;
    CVMMethodBlock* java_lang_ClassLoader_findNative;
    CVMMethodBlock* java_lang_ClassLoader_checkPackageAccess;
    CVMMethodBlock* java_lang_ClassLoader_loadBootstrapClass;
    CVMMethodBlock* java_lang_Class_loadSuperClasses;
#endif /* CVM_CLASSLOADING */

    /*
     * Shutdown-related stuff
     */
    CVMMethodBlock* java_lang_Shutdown_waitAllUserThreadsExitAndShutdown;
    CVMMethodBlock* sun_misc_ThreadRegistry_waitAllSystemThreadsExit;

#ifdef CVM_LVM
    CVMMethodBlock* java_lang_ref_Finalizer_runAllFinalizersOfSystemClass;
#endif
    CVMMethodBlock* java_lang_Thread_exit;
    CVMMethodBlock* java_lang_Thread_initMainThread;
    CVMMethodBlock* java_lang_Thread_initAttachedThread;
    CVMMethodBlock* java_lang_Thread_nextThreadNum;

    /*
     * Other stuff needed for class loading.
     */
#ifdef CVM_CLASSLOADING
#ifndef CVM_LVM /* %begin,end lvm */
    /*
     * When Logical VM is supported, systemClassLoader is initialized
     * and stored per-LVM basis.
     */
    CVMClassLoaderICell* systemClassLoader;
#endif /* %begin,end lvm */

    CVMClassPath         bootClassPath;
    CVMClassPath         appClassPath;
    
    CVMUint16            classVerificationLevel;
#ifdef CVM_SPLIT_VERIFY
    CVMBool              splitVerify;
#endif
    void*                cvmDynHandle;
    
    /* Stuff for native application class loading */
    CVMMethodBlock*     sun_misc_Launcher_AppClassLoader_setExtInfo;
    CVMMethodBlock*     sun_misc_Launcher_ClassContainer_init;
    CVMMethodBlock*     sun_misc_Launcher_ClassContainer_init_withClass;
    CVMMethodBlock*     sun_misc_Launcher_getFileURL;
    CVMMethodBlock*     java_io_File_init;
    CVMMethodBlock*     java_util_jar_JarFile_init;
    CVMMethodBlock*     java_security_CodeSource_init;
    CVMMethodBlock*     java_security_SecureClassLoader_getProtectionDomain;
    CVMMethodBlock*     java_lang_ClassLoader_checkCerts;
#endif /* CVM_CLASSLOADING */

#ifdef CVM_AOT
    CVMMethodBlock*     sun_misc_Warmup_runit;
#endif

    /*
     * Simple hash table used to keep track of packages loaded by the system
     * and the directory or JAR files that they were loaded from. Used in
     * java.lang.Package to maintain version information for loaded system
     * packages.
     */
    CVMPackage* packages[CVM_PKGINFO_HASHSIZE];
    CVMUint16   numPackages;

#ifdef CVM_DUAL_STACK
    const CVMClassRestrictions* dualStackMemberFilter;
    /* sun.misc.MIDPImplementationClassLoader class type id */
    CVMClassTypeID midpImplClassLoaderTid;
    /* sun.misc.MIDletClassLoader class type id */
    CVMClassTypeID midletClassLoaderTid;
#endif

#ifdef CVM_JVMTI
    CVMJvmtiGlobals jvmti;
#endif

#ifdef CVM_JVMPI
    /* JVMPI flags: */
    CVMJvmpiRecord jvmpiRecord;
    CVMProfiledMonitor *objMonitorList;
    CVMProfiledMonitor *rawMonitorList;
    CVMSysMutex jvmpiSyncLock;  /* Protect insertion into the Monitor Lists. */
    CVMMethodBlock* java_lang_Thread_initDaemonThread;
#endif

    /*
     * Type system globals.
     * There is only one lock (for now) governing
     * all access to it.
     */
    CVMSysMutex typeidLock; /* protect table insertion */
    CVMUint32	typeIDscalarSegmentSize;
    CVMUint32	typeIDmethodTypeSegmentSize;
    CVMUint32	typeIDmemberNameSegmentSize;

    /*
     * The lock on the intern table (which is now a C data structure)
     * To protect the table during search/mutation, including GC
     * sweeping!
     */
    CVMSysMutex	internLock;
    CVMUint32	stringInternSegmentSizeIdx;

    /*
     * The one word memory for the random number generator
     */
    CVMInt32  lastRandom;

    /*
     * Some desperation exception objects we preallocate.
     */
    CVMThrowableICell* preallocatedOutOfMemoryError;
    CVMThrowableICell* preallocatedStackOverflowError;

    /*
     * -Xopt options
     */

    CVMParsedSubOptions parsedSubOptions;

    struct {
	CVMUint32 nativeStackSize;
	CVMUint32 javaStackMinSize;
	CVMUint32 javaStackMaxSize;
	CVMUint32 javaStackChunkSize;
    } config;

    /*
     * Hooks
     */
    void *vfprintfHook;
    void (*exitHook)(int);
    void (*abortHook)();

    void (*safeExitHook)(int);

    CVMBool abort_entered;

#ifdef CVM_TIMESTAMPING
    CVMBool timeStampEnabled;
    CVMSysMutex	timestampListLock;
    CVMInt64 firstWallclockTime;
#endif

    /*
     * Shutdown flags
     */
    CVMBool fullShutdown;

    /*
     * True if we should hang in an infinite loop when starting the vm.
     * This is useful when debugging CVM as a shared library, since it
     * is often not possible to set breakpoints until after CVM is invoked.
     */
    volatile CVMBool hangOnStartup;

    /* If true, no limit on the size of global roots stacks. */
    CVMBool unlimitedGCRoots;

#ifdef CVM_XRUN
    /*
     * A list to keep track of shared native library loaded by the
     * -Xrun command argument. It contains the shared library object
     * handle and the function pointer of calling JVM_onUnLoad at VM
     * exit.
     */
    CVMXrunTable onUnloadTable;
#endif
#ifdef CVM_AGENTLIB
    /*
     * A list to keep track of shared native library loaded by the
     * -agentlib/-agentpath command argument. It contains the shared library
     * object handle and the function pointer of calling Agent_OnUnLoad at VM
     * exit.
     */
    CVMAgentTable agentTable;
#endif
    /*
     * exit_procs contains exitHandle() function pointer to exit 
     *            JVMTI event at VM exit.
     */
    exit_procPtr exit_procs;

    CVMObject* discoveredSoftRefs;
    CVMObject* discoveredWeakRefs;
    CVMObject* discoveredFinalRefs;
    CVMObject* discoveredPhantomRefs;

    /* Three deferred queues for weakrefs handling */
    /* 1. Those weakrefs that are not dying. Defer setting their next
       pointers to NULL */
    CVMObject* deferredWeakrefs;
    /* 2. Those that are dying and with referents to be cleared go
       into here. In case of undo, we won't clear the referents. */
    CVMObject* deferredWeakrefsToClear;
    /* 3. Those that are dying and with live referent's go into
       here. If we don't do an undo, we put these items in the pending
       queue. */
    CVMObject* deferredWeakrefsToAddToPending;

#if defined(CVM_HAVE_DEPRECATED) || defined(CVM_THREAD_SUSPENSION)
    CVMBool suspendCheckerInitialized;
    /* These are used to implement the suspension checker mechanism.  See
       notes in interpreter.c for how it works: */
    CVMThreadID suspendCheckerThreadInfo;
    volatile CVMUint32 suspendCheckerState;
    CVMMutex suspendCheckerLock;
    CVMCondVar suspendCheckerCV;
    CVMCondVar suspendCheckerAckCV;
#endif

#ifdef CVM_LVM /* %begin lvm */
    /* Logical VM specific globally defined variables */
    CVMLVMGlobals lvm;
#endif /* %end lvm */

#ifdef CVM_MTASK
    CVMInt32 commFd; /* A pipe to the mTASK process */
    CVMInt32 clientId; /* The client ID as it is recognized by mTASK */
    /* Are we running in server mode? */
    CVMBool isServer;
    CVMInt32 serverPort; /* The port number master mTASK process listens to */
#endif

    /* Variables for GC statistics */
    CVMBool measureGC;
    CVMInt64 totalGCTime;
    CVMInt64 startGCTime;
    CVMInt64 initFreeMemory;

#ifndef CDC_10
    /* java assertion related globals */
    CVMBool javaAssertionsUserDefault; /* User class default (-ea/-da). */
    CVMBool javaAssertionsSysDefault;  /* System class default (-esa/-dsa). */
    /* classes with assertions enabled or disabled */
    CVMJavaAssertionsOptionList* javaAssertionsClasses;
    /* packages with assertions enabled or disabled */
    CVMJavaAssertionsOptionList* javaAssertionsPackages;
#endif

    CVMBool userHomePropSpecified; /* true if -Duser.home specified */
    CVMBool userNamePropSpecified; /* true if -Duser.name specified */

#ifdef CVM_INSPECTOR
    CVMInspector inspector;
#endif

    loopProcPtr CVMgcUnsafeExecuteJavaMethodProcPtr;

};

#define CVM_CSTATE(x)  (&CVMglobals.cstate[(x)])

#ifdef CVM_LVM /* %begin lvm */
#define CVMsystemClassLoader(ee)  CVMLVMeeSystemClassLoader(ee)
#else /* %end lvm */
#define CVMsystemClassLoader(ee)  (CVMglobals.systemClassLoader)
#endif /* %begin,end lvm */

/*
 * Defined in share/javavm/runtime/globals.c
 */

extern void
CVMoptPrintUsage();

extern CVMBool
CVMoptParseXoptOptions(const char* optAttributesStr);

extern CVMBool
CVMoptParseXssOption(const char* optAttributesStr);

#if defined(CVM_DEBUG) || defined(CVM_INSPECTOR)
extern void CVMdumpGlobalsSubOptionValues();
#endif /* CVM_DEBUG || CVM_INSPECTOR */

extern CVMGlobalState CVMglobals;
extern CVMBool
CVMinitVMGlobalState(CVMGlobalState *, CVMOptions *options);
extern void
CVMdestroyVMGlobalState(CVMExecEnv *ee, CVMGlobalState *);

extern void
CVMglobalSysMutexesAcquire(CVMExecEnv* ee);
extern void
CVMglobalSysMutexesRelease(CVMExecEnv* ee);

#endif /* _INCLUDED_GLOBALS_H */
