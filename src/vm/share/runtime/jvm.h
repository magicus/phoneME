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

/** \file jvm.h
 * This header file declares all the publicly exported interfaces
 * to the VM (except KNI stuff).
 */

#ifndef _JVM_CONFIG_H_
#include "jvmconfig.h"
#endif

#ifndef _JVM_H_
#define _JVM_H_

/* The proper order of inclusion of the .h files exported by the VM

   + jvmconfig.h
   + kni.h
   + jvm.h
   + jvmspi.h
   + sni.h
   + ani.h

   If you include them out of order, each file uses #ifndef to include
   all the headers that it needs
 */

#ifndef _JAVASOFT_KNI_H_
#include "kni.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------
 * UNICODE-capable filename support. This typedef is also included in
 * GlobalDefinitions.hpp for internal interfaces, hence the
 * #ifndef JVM_PATHCHAR_DEFINED
 *
 * If your system requires unicode for JvmPathChar, make sure to pass
 * -DUSE_UNICODE_FOR_FILENAMES=1 when compiling external code that uses
 * jvm.h
 *----------------------------------------------------------------------*/
#ifndef JVM_PATHCHAR_DEFINED
#define JVM_PATHCHAR_DEFINED 1

/* PCSL requires Unicode filenames */
#if ENABLE_PCSL

#if defined(USE_UNICODE_FOR_FILENAMES) 
#if !USE_UNICODE_FOR_FILENAMES
#error "USE_UNICODE_FOR_FILENAMES must be set if ENABLE_PCSL is set"
#endif
#else
#define USE_UNICODE_FOR_FILENAMES 1
#endif

#else

#ifndef USE_UNICODE_FOR_FILENAMES
#define USE_UNICODE_FOR_FILENAMES 0
#endif

#endif /* ENABLE_PCSL */

#if USE_UNICODE_FOR_FILENAMES
typedef jchar JvmPathChar;
#else
typedef char JvmPathChar;
#endif

#endif /* JVM_PATHCHAR_DEFINED */

/*=========================================================================
 *
 * Functions implemented in the VM core
 *
 *=======================================================================*/

/* 
 * Verify all classes contained in the classpath.
 * Note that for MVM case there's Java API to this functionality:
 *   see com.sun.cldc.isolate.Util.verify()
 * which is preferrable, as it doesn't require VM restart (all job is being done in 
 * the separate VM task)
 */
jboolean JVM_Verify(const JvmPathChar *classpath);

/* Call this before any other Jvm_ functions. */
void JVM_Initialize(void);

/**
 * Parse one command line argument and return the number of command
 * line tokens it takes.
 *
 * Return '0' if no more argument present.
 * Return '-1' if parse error occurs.
 */
int JVM_ParseOneArg(int argc, char **argv);

/* Launch the JVM */
int JVM_Start(const JvmPathChar *classpath, char *main_class, int argc,
              char *argv[]);

/* Launch the JVM */
int JVM_Start2(const JvmPathChar *classpath, char *main_class, int argc, jchar
               *u_argv[]);
/*
 * Stop the JVM -- this function may be called only within native code. It
 * never returns to the caller.
 */
void JVM_Stop(int exit_code);

/*
 * For use in SlaveMode only -- execute bytecodes for a small time slice.
 *
 * Possible return values:
 *
 * -2: Java execution has completed. Either all threads have exited, or
 *     JVM_Stop() has been called by a native methods.
 *
 *     In this case, you should call JVM_CleanUp() to free resources
 *     allocated by the VM.
 *
 * -1: All Java threads are waiting forever (for external events or for
 *     Java locks).
 *
 *     In this case, before calling JVM_TimeSlice again, you need to
 *     wait until at least one Java thread becomes ready to execute
 *     because of an external event.
 *
 * 0 or above: This indicates some Java threads are ready for execution
 *     after the specified time (in milliseconds) has elapsed.
 *
 *     In this case, before calling JVM_TimeSlice again, you need to
 *     wait until the specified time has elapsed, or until at least
 *     one Java thread becomes ready to execute because of an external
 *     event.
 */
jlong JVM_TimeSlice(void);

/*
 * For use in SlaveMode only -- clean up the VM and returns exit
 * code.
 */
int JVM_CleanUp(void);

/*
 * Send hints to the VM to indicate certain execution conditions.
 * This API allows the VM to fine tune its internal parameters to
 * achieve optimal peformance.
 *
 * task_id: In SVM case, task_id must be 0. In MVM case, task_id must
 *          be the value returned by the Java method 
 *          com.sun.cldc.isolate.Isolate.id() for a given task.
 *
 * hint:    Must be one of the JVM_HINT_* parameters listed below.
 * param:   Must be 0.
 */

void JVM_SetHint(int task_id, int hint, int param);

#define JVM_HINT_BEGIN_STARTUP_PHASE 1   /* Application is in start-up phase,
                                          * make the JIT less aggressive */
#define JVM_HINT_END_STARTUP_PHASE   2
#define JVM_HINT_VISUAL_OUTPUT       3   /* Application has just made some
                                          * visual output. VM should try to
                                          * avoid excessive compilation for
                                          * a little while to avoid 
                                          * visible jittering */

/*
 * Returns the length of a unicode string
 */
int unicode_strlen(const jchar* str);

#if ENABLE_MONET
/*
 * JVM_CreateAppImage()
 *
 * Load the Java classes from JAR file into the heap, verify the class
 * contents, and write the classes to an Application Image file. This function
 * is typically executed by the Application Management Software (AMS)
 * immediately after a JAR file is downloaded to the device.
 *
 * This function must be called with a clean VM state -- i.e., if a
 * Java application is executing, you must exit the Java application
 * before running the Converter.
 *
 * If the VM is configured to run in MVM mode, and the VM is executing
 * (one or more tasks), this function will fail and no app image will
 * be created. The Java API com.sun.cldc.isolate.Util.createAppImage()
 * should be used instead.
 *
 * The flags argument may contain binary OR'ed values listed below:
 *
 * JVM_REMOVE_CLASSES_FROM_JAR -- if this flag is defined and the conversion
 *                                is successful, all .class files are removed
 *                                from the <jarFile> before this function
 *                                returns.
 *
 * JVM_CreateAppImage() returns 0 if successful.
 */

#define JVM_REMOVE_CLASSES_FROM_JAR    (1 << 1)
#define JVM_GENERATE_SHARED_IMAGE      (1 << 2)

jint JVM_CreateAppImage(const JvmPathChar *jarFile, const JvmPathChar *binFile,
                        int flags);
#endif

/*----------------------------------------------------------------------
 *
 * VM configuration
 *
 *----------------------------------------------------------------------*/

#define JVM_CONFIG_HEAP_CAPACITY                2
#define JVM_CONFIG_HEAP_MINIMUM                 3
#define JVM_CONFIG_SLAVE_MODE                   4
#define JVM_CONFIG_USE_ROM                      5
#define JVM_CONFIG_DEBUGGER_PORT                6
#define JVM_CONFIG_ASYNC_DATA_SIZE              7

#if ENABLE_ISOLATES
#define JVM_CONFIG_FIRST_ISOLATE_RESERVED_MEMORY 8
#define JVM_CONFIG_FIRST_ISOLATE_TOTAL_MEMORY   9
#endif

int  JVM_GetConfig(int name);
void JVM_SetConfig(int name, int value);

/*
 * Returns an ID that's intended to be different every time the MIDP
 * stack is rebuilt. This function is mainly used during development,
 * so that new AppImages can be automatically regenerated after the
 * MIDP stack is rebuilt. This way developers don't need to manually
 * delete AppImages from the device every time MIDP stack is changed.
 *
 * The MIDP code that interfaces with AppImage creation should always
 * check if the second word of an AppImage matches with
 * JVM_GetVersionID(). If not, delete the AppImage file and call
 * JVM_CreateAppImage() again to regenerate it from the original JAR file.
 *
 * This also means that during development, you keep a copy of the
 * original JAR file on the device so that regenartion is possible.
 */
int JVM_GetVersionID();

/*
 * The function should be called by the MIDP code with a number that's 
 * statically generated by the build system every time MIDP is rebuild.
 * Here's an example Makefile rule that regenerates such a number every 
 * time MIDP or CLDC objects files/libraries are rebuilt (Linux).
 *
 * versionid.c: $(MIDP_OBJS) libcldc_vm.a
 *      echo "int midp_versionid = `date +%s;`" > versionid.c
 */
void JVM_SetVersionID(int id);

/*=========================================================================
 *
 * VM internals --> EVERYTHING BELOW THIS LINE
 *
 * The following functions expose some internal features in the
 * VM. They should be used with great care. The API WILL change in
 * future revisions of the VM - GUARANTEED!
 *
 *=======================================================================*/

jlong JVM_JavaMilliSeconds();

#if ENABLE_MULTIPLE_PROFILES_SUPPORT
/*
 * Sets the current profile
 */
int JVM_SetProfile(char *profile_name);
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT

/*----------------------------------------------------------------------
 *
 * Jar file decompression.
 *
 *----------------------------------------------------------------------*/

typedef unsigned int (*JvmGetByteProc)(void *);
jboolean Jvm_inflate(void *data, JvmGetByteProc getByteProc, int compLen,
                     unsigned char** outFileH, int decompLen);

typedef jboolean (*JvmNameFilterProc)(const char* name, void* caller_data);
typedef jboolean (*JvmDoJarEntryProc)(const char* entry_name, void* entry_data,
                                      unsigned int length, void* caller_data);
typedef void* (*JvmAllocProc)(unsigned int size);
typedef void (*JvmFreeProc)(void* address);

#if ENABLE_JAR_READER_EXPORTS
/**
 * Iterates over all entries in the JAR file with the specified name.
 * For each JAR entry the routine does the following:
 * 1.If entry_filter_proc is not NULL, entry_filter_proc() is invoked with
 * the entry name and the caller data as its arguments.
 * 2.If entry_filter_proc is NULL or entry_filter_proc() returns KNI_TRUE,
 * and do_entry_proc is not NULL, the entry is decompressed.
 * 3.If the entry is decompressed successfully, do_entry_proc() is called for it
 * with the following arguments: the entry name, buffer with decompressed entry
 * contents, the uncompressed entry length and the caller data.
 * 4.If do_entry_proc() returns returns KNI_FALSE, no more entries are iterated
 * and JVM_IterateJarEntries() returns KNI_TRUE.
 *
 * The buffer passed to do_entry_proc() must not be accessed after
 * do_entry_proc() returns.
 *
 * The specified alloc_proc and free_proc are used for all memory allocations.
 * If alloc_proc and free_proc are both NULL, the default routines are used for
 * all memory allocations.
 * If one of alloc_proc and free_proc is NULL and the other is not NULL,
 * JVM_IterateJarEntries() immediatelly returns KNI_FALSE.
 *
 * If the routine fails to iterate over all entries in the JAR file or fails
 * to decompress an entry, KNI_FALSE is returned.
 * Otherwise, KNI_TRUE is returned.
 */
#if 0 /* NOT_CURRENTLY_USED */
jboolean JVM_IterateJarEntries(const JvmPathChar* jar_file_name,
                               JvmNameFilterProc entry_filter_proc,
                               JvmDoJarEntryProc do_entry_proc,
                               JvmAllocProc alloc_proc, JvmFreeProc free_proc,
                               void* caller_data);
#endif


/*
 * DEPRECATED -- Jvm_read_jar_entry will be subsumed by Java-level API.
 *               for JAR indexing and decompression.
 */
void Jvm_read_jar_entry(const JvmPathChar* jar_file_name, 
                        const char* entry_name, jobject entry);

#endif /* ENABLE_JAR_READER_EXPORTS */

/*----------------------------------------------------------------------
 *
 * Assertion support
 *
 *----------------------------------------------------------------------*/

#ifdef AZZERT

#ifndef STR
#define STR(a) #a
#endif

void JVM_ReportAssertionFailure(const char* code_str, const char* file_name,
                                int line_no, const char* message);

#define JVM_ASSERT(b, msg) {                                      \
    if (!(b)) {                                                   \
      JVM_ReportAssertionFailure(STR(b),__FILE__, __LINE__, msg); \
    }                                                             \
  }

#else
#define JVM_ASSERT(b, msg)
#endif

/*----------------------------------------------------------------------
 *
 * Java Debugger API
 *
 *----------------------------------------------------------------------*/

/* run the debugger command loop */
void JVM_ProcessDebuggerCmds();

#if ENABLE_JAVA_DEBUGGER

/* get the name of the transport */
char * JVM_GetDebuggerTransport();

/* get the file descriptor of the debugger socket */
int JVM_GetDebuggerSocketFd();


/* is debugger connected? */
jboolean JVM_IsDebuggerActive();

/* VM started with -debugger ? */
jboolean JVM_IsDebuggerOptionOn();

#else
#define JVM_IsDebuggerActive() 0
#define JVM_IsDebuggerOptionOn() 0
#endif


/*----------------------------------------------------------------------
 *
 * Java Profiler API
 *
 *----------------------------------------------------------------------*/
#if ENABLE_WTK_PROFILER

/* Start profilation process */
#define JAVA_PROFILER_COMMAND_START           1
/* Stop profilation process */
#define JAVA_PROFILER_COMMAND_STOP            2 
/* 
 * Dump profiler data to file and clear all used memory.
 * Data will also be dumped on VM or task (in MVM case) exit.
 * Optional argument is used in MVM case to denote which task's
 * data should be dumped (it should be result of (void*)JVM_CurrentIsolateID())
 * or NULL, what means all data gathered.
 */
#define JAVA_PROFILER_COMMAND_DUMP_AND_CLEAR  3

/* 
 * Send profiler command (see above defines for possible command values).
 * Returns true on success and false in error happened.
 */ 
jboolean JVM_SendProfilerCommand(int cmd, void* arg);

#endif


/*----------------------------------------------------------------------
 *
 * VM Performance Counter API
 *
 *----------------------------------------------------------------------*/

#if ENABLE_PERFORMANCE_COUNTERS

typedef struct _JVM_PerformanceCounters {
  int mem_used;
  int mem_free;
  int mem_total;
  int total_bytes_collected;
  int num_of_c_alloc_objs;    /* Number of objects allocated in C code,
                               * via ObjectHeap::allocate() */
  int num_of_gc;
  int num_of_compiler_gc;
  int num_of_full_gc;
  int num_of_threads;

  /*
   * Number of timer ticks received during Vm execution
   */
  int num_of_timer_ticks;

  /*
   * The following fields are measured by high-resolution system time.
   * See vm_hrtick_frequency.
   */
  jlong vm_start_hrtick;      /* The hrtick at which the VM started */
  jlong vm_current_hrtick;    /* The current hrtick. Call
                               * JVM_GetPerformanceCounters() to get the latest
                               * value */

  jlong total_in_vm_hrticks;  /* amount of time in VM code vs. comp/interp. See
                               * JVM.cpp for enabling this counter */

  /*----------------------------------------------------------------------
   * Class loading/verification
   *----------------------------------------------------------------------*/

  int num_of_class_loaded;     /* Number of classes loaded */

  jlong total_gc_hrticks;      /* Total number of hrticks spent inside GC */
  jlong max_gc_hrticks;        /* Number of hrticks spent in the longest GC */

  jlong total_event_checks;    /* Number times of JVMSPI_CheckEvents called */
  jlong total_event_hrticks;   /* Total hrticks spent for reading events */

  jlong total_load_hrticks;    /* Total number of hrticks in class loading */
                               /* Includes binary loading if any */
  jlong binary_load_hrticks;   /* Number of hrticks in binary image loading */
  jlong binary_link_hrticks;   /* Number of hrticks in binary image linking */
  jlong max_load_hrticks;      /* Number of hrticks spent in the longest
                                * class loading. */

  jlong total_verify_hrticks;  /* Total number of hrticks in verification */
  jlong max_verify_hrticks;    /* Number of hrticks spent in the longest
                                * verification. */

  /*----------------------------------------------------------------------
   * Romization
   *----------------------------------------------------------------------*/
#ifdef ENABLE_ROM_GENERATOR
  int num_of_romizer_steps;    /* Number of calls into the
                                * {Source,Binary}ROMWriter::execute() method */
  jlong total_romizer_hrticks; /* Total number of hrticks spent in romizer */
  jlong max_romizer_hrticks;   /* The longest time spent inside a single
                                * call to {Source,Binary}ROMWriter::execute()*/
#endif

  /*----------------------------------------------------------------------
   * Compilation
   *----------------------------------------------------------------------*/

  int num_of_compilations;    /* Number of compilation started */
  int num_of_compilations_finished;
                              /* Number of compilation that actually finished
                               * (without running out of memory or being 
                               * aborted) */
  int num_of_compilations_failed;
                              /* Number of compilation that failed (due to
                               * lack of memory) */
  int compilation_resume_count;
                              /* How many times have we resumed compilation*/
  jlong total_compile_hrticks; /* Total number of hrticks spent in compiler */
  jlong max_compile_hrticks;   /* Number of hrticks spent in longest compile */
  jlong total_compile_mem;     /* Total memory allocated during compilation */
  jlong max_compile_mem;       /* Memory allocated during the largest
                                * compilation */
  jlong total_compiled_methods;/* Total bytes of memory for CompiledMethod */
  jlong max_compiled_method;   /* Size of the largest CompiledMethod */
  jlong total_compiled_bytecodes;/* Total bytes of bytecodes compiled */
  jlong max_compiled_bytecodes;/* Bytes of bytecodes in largest Method to be
                                * compiled */

  int uncommon_traps_generated;/* Number of uncommon traps generated by the
                                * compiler */
  int uncommon_traps_taken;    /* Number of uncommon traps taken during
                                * execution of compiled code */


  /*----------------------------------------------------------------------
   * Information about the underlying high-res ticks facility
   *----------------------------------------------------------------------*/
  jlong hrtick_frequency;      /* How many high-resolution ticks are 
                                * generated per second. */
  jlong hrtick_overhead_per_1000;/* How many hrticks does it take to read
                                * the high-res clock for 1000 times in a tight
                                * loop. */
  jlong hrtick_read_count;     /* How many times have we read the high-res
                                * clock */

} JVM_PerformanceCounters;

void JVM_ResetPerformanceCounters();
JVM_PerformanceCounters* JVM_GetPerformanceCounters();

#endif  /* ENABLE_PERFORMANCE_COUNTERS */

/*
 * Returns the addresses of the start, limit and end of the Java heap. The
 * Java heap is configured as the following:
 *
 * [ ...........[              ]
 * ^            ^              ^
 * |            heap_limit     heap_end
 * heap_start
 *
 * The VM will allocate only in the space between [heap_start, heap_limit).
 * The space between [heap_limit, heap_end] are free for native methods
 * to store arbitrary data into. The main purpose of having such a region is
 * for implementing a customed malloc() library that co-exists with the
 * Java heap.
 *
 * When the VM starts up, heap_limit is always at heap_end. Native
 * methods may call JVM_SetHeapLimit() to adjust heap_limit.
 *
 * The functions works only for an non-expanding Java heap. E.g.,
 * HeapMin must be equal to HeapCapacity.
 */
void JVM_GetHeapInfo(void **heap_start, void **heap_limit, void **heap_end);

/*
 * Sets the address of the new limit. The valid range is
 *  heap_start <= new_heap_limit <= heap_end.
 *
 * The function returns the new value of heap_limit. If new_heap_limit
 * is higher than or equal to the current value of heap_limit, this
 * operation always succeeds, no GC will happen, and the function
 * returns the same value as new_heap_limit.
 *
 * If new_heap_limit is lower than the current value of heap_limit,
 * the VM may execute GC. Still, the request may not be fully
 * satisfied. This function returns the new value of heap_limit, which
 * may be higher than the requested new_heap_limit.
 *
 * The functions works only for an non-expanding Java heap. E.g.,
 * HeapMin must be equal to HeapCapacity.
 *
 * Note: the return value is always rounded up to the next 4-byte boundary.
 */
void * JVM_SetHeapLimit(void *new_heap_limit);

/*
 * Force a garbage collection to happen. Possible values for the
 * flags parameter are (may be combined using binary OR):
 *
 * JVM_COLLECT_YOUNG_SPACE_ONLY:
 *   - If false, the entire normal object space is collected.
 *     If true, only the young object space is collected
 *
 * JVM_COLLECT_COMPILER_AREA:
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
int JVM_GarbageCollect(int flags, int requested_free_bytes);

#define JVM_COLLECT_YOUNG_SPACE_ONLY     (1 << 0)
#define JVM_COLLECT_COMPILER_AREA        (1 << 1)


/*
 * Register a new event type
 * Event name must be a non-empty C string of no more than
 *   JVM_MAX_EVENT_NAME_LENGTH letters, digits and underscores.
 *
 * Returns unique ID for the event
 * If extended event logger is disabled or
 *    max number of event types is exceeded,
 * returns -1.
 *
 * Repeated registration of the same type name returns previous ID.
 */
int JVM_RegisterEventType(const char* name);
#define JVM_MAX_EVENT_NAME_LENGTH 32

/*
 * Record an event start/end JVM EventLogger:
 *
 * JVM_EVENT_SCREEN_UPDATE
 * or a registered event type if extended event logger is enabled.
 *
 * If event logger is enabled and event type is valid, records the event and
 * returns KNI_TRUE. Otherwise returns KNI_FALSE.
 */
jboolean JVM_LogEventStart(int type);
jboolean JVM_LogEventEnd  (int type);
#define JVM_EVENT_SCREEN_UPDATE 0


#if ENABLE_ISOLATES

/*
 * Returns a small integer ID that uniquely identifies the current isolate.
 * The ID is always >= 0 and < JVM_MaxIsolates().
 */
int JVM_CurrentIsolateID();

/* backwards compatibility, will remove by final release */
#define JVM_CurrentIsolateId() JVM_CurrentIsolateID()

/*
 * Returns the upper (exclusive) limit all possible isolate IDs.
 */
int JVM_MaxIsolates();

/*
 * Suspends all threads in an isolate with the specified ID from execution. 
 *
 * This function has no effect if there is no isolate with the specified ID.
 *
 * This function will suspend the isolate only if the isolate is currently
 * started, not suspended and not terminated. Otherwise this function
 * has no effect.
 *
 * Returns KNI_TRUE if there is an isolate with the specified ID and that 
 * isolate is successfully suspended. Otherwise, returns KNI_FALSE.
 */
jboolean JVM_SuspendIsolate(int isolate_id);

/*
 * Returns KNI_TRUE if there is an isolate with the specified ID and that 
 * isolate is currently suspended. Otherwise, returns KNI_FALSE. 
 */
jboolean JVM_IsIsolateSuspended(int isolate_id);

/*
 * Resumes execution of all threads in an isolate with the specified ID.
 *
 * This function has no effect if there is no isolate with the specified ID.
 *
 * This method will resume the isolate only if the isolate is currently
 * started, suspended and not terminated. Otherwise this function has  
 * no effect.
 *
 * Returns KNI_TRUE if there is an isolate with the specified ID and that 
 * isolate is successfully resumed. Otherwise, returns KNI_FALSE.
 */
jboolean JVM_ResumeIsolate(int isolate_id);

#endif /* ENABLE_ISOLATES */

/*
 * Enables or disables class verification for the current VM in SVM mode,
 * and for the current task in MVM mode.
 *
 * Whether VM is not running or there is no current active task, the
 * function enables/disables class verification for newly started VM/task.
 *
 * In the case class verification is disabled, all classes newly loaded
 * in the VM/task won't be verified and therefore can be loaded faster.
 * It's responsibility of the caller to guarantee that only trusted or
 * preverified classes can be loaded with no verification.
 *
 * Returns the previous state of class verification in 
 * the current VM/task.
 */
jboolean JVM_SetUseVerifier(jboolean use_verifier);

/*
 * Get state of the class verifier for the current VM/task in SVM/MVM
 * modes correspondingly.
 *
 * Returns true if class verification is enabled, otherwise false.
 */
jboolean JVM_GetUseVerifier();

#if ENABLE_FLOAT

JVM_SOFTFP_LINKAGE jfloat  jvm_fadd(jfloat x, jfloat y);
JVM_SOFTFP_LINKAGE jfloat  jvm_fsub(jfloat x, jfloat y);
JVM_SOFTFP_LINKAGE jfloat  jvm_fmul(jfloat x, jfloat y);
JVM_SOFTFP_LINKAGE jfloat  jvm_fdiv(jfloat x, jfloat y);
JVM_SOFTFP_LINKAGE jfloat  jvm_frem(jfloat x, jfloat y);

JVM_SOFTFP_LINKAGE jdouble jvm_dadd(jdouble x, jdouble y);
JVM_SOFTFP_LINKAGE jdouble jvm_dsub(jdouble x, jdouble y);
JVM_SOFTFP_LINKAGE jdouble jvm_dmul(jdouble x, jdouble y);
JVM_SOFTFP_LINKAGE jdouble jvm_ddiv(jdouble x, jdouble y);
JVM_SOFTFP_LINKAGE jdouble jvm_drem(jdouble x, jdouble y);

JVM_SOFTFP_LINKAGE jdouble jvm_i2d(jint x);
JVM_SOFTFP_LINKAGE jint    jvm_d2i(jdouble x);
JVM_SOFTFP_LINKAGE jdouble jvm_f2d(jfloat x);
JVM_SOFTFP_LINKAGE jfloat  jvm_d2f(jdouble x);
JVM_SOFTFP_LINKAGE jfloat  jvm_i2f(jint x);
JVM_SOFTFP_LINKAGE jint    jvm_f2i(jfloat x);
JVM_SOFTFP_LINKAGE jfloat  jvm_l2f(jlong x);
JVM_SOFTFP_LINKAGE jlong   jvm_f2l(jfloat x);
JVM_SOFTFP_LINKAGE jdouble jvm_l2d(jlong x);
JVM_SOFTFP_LINKAGE jlong   jvm_d2l(jdouble x);

JVM_SOFTFP_LINKAGE jint    jvm_dcmpl(jdouble x, jdouble y);
JVM_SOFTFP_LINKAGE jint    jvm_dcmpg(jdouble x, jdouble y);
JVM_SOFTFP_LINKAGE jint    jvm_fcmpl(jfloat x, jfloat y);
JVM_SOFTFP_LINKAGE jint    jvm_fcmpg(jfloat x, jfloat y);

JVM_SOFTFP_LINKAGE jdouble jvm_dneg(jdouble x);

JVM_SOFTFP_LINKAGE jint    jvm_floatToIntBits(jfloat x);
JVM_SOFTFP_LINKAGE jlong   jvm_doubleToLongBits(jdouble x);

JVM_SOFTFP_LINKAGE jdouble jvm_sin(jdouble x);
JVM_SOFTFP_LINKAGE jdouble jvm_cos(jdouble x);
JVM_SOFTFP_LINKAGE jdouble jvm_tan(jdouble x);
JVM_SOFTFP_LINKAGE jdouble jvm_sqrt(jdouble x);
JVM_SOFTFP_LINKAGE jdouble jvm_ceil(jdouble x);
JVM_SOFTFP_LINKAGE jdouble jvm_floor(jdouble x);
JVM_SOFTFP_LINKAGE jdouble jvm_fabs(jdouble x);

#if ENABLE_CLDC_111

JVM_SOFTFP_LINKAGE jdouble jvm_asin(jdouble x);
JVM_SOFTFP_LINKAGE jdouble jvm_acos(jdouble x);
JVM_SOFTFP_LINKAGE jdouble jvm_atan(jdouble x);
JVM_SOFTFP_LINKAGE jdouble jvm_atan2(jdouble x, jdouble y);

#endif

JVM_SOFTFP_LINKAGE jdouble jvm_double_from_msw_lsw(jint msw, jint lsw);
JVM_SOFTFP_LINKAGE jint    jvm_double_msw(jdouble d);
JVM_SOFTFP_LINKAGE jint    jvm_double_lsw(jdouble d);


#endif /* ENABLE_FLOAT */
#ifdef __cplusplus
}
#endif

#endif  /* _JVM_H_ */
