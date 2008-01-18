/*
 *
 * Portions Copyright  2000-2007 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 *!c<
 * Copyright 2006 Intel Corporation. All rights reserved.
 *!c>
 */

/** \file Globals.hpp
 * Command line switches.
 */

/*
 * The VM has a special mechanism that allows
 * compilation flags and command line switches to be treated
 * uniformly.  Basically, every compilation flag or option
 * in the VM can be activated/deactivated also
 * from the command line using the following syntax:
 *
 * cldc_vm +<option_name>        <class_name> ; Turns <option_name> on
 * cldc_vm -<option_name>        <class_name> ; Turns <option_name> off
 * cldc_vm =<option_name><value> <class_name> ; Sets  <option_name> to <value>
 *
 * In general, the "+" or "-" syntax is used for turning
 * boolean options on or off, while the "=" syntax is used
 * for changing the value of those options that take a
 * numeric argument.
 *
 * Examples:
 *     cldc_vm +UseCodeFlushing      (Turn code flushing on)
 *     cldc_vm -UseProfiler          (Turn profiler off)
 *     cldc_vm =CompiledCodeFactor15 (Set compiled code factor to 15)
 *
 * Note that most compilation options are available only
 * in non-product builds of the system. Those flags that
 * have been marked as "develop" flags are visible only
 * during development, while the "product" flags are
 * available also in product builds.
 *
 * For a detailed description and categorization of the
 * various compilation flags, refer to the Porting Guide.
 */

/** \class Globals
 *
 * The Globals class handles global configuration options (such as
 * HeapCapacity) which can be set via the command line.
 *
 * The Globals class also supports the printing of various information
 * of the VM, including:
 *
 *    - values of build-time options (such as ENABLE_COMPILER)
 *    - values of various symbolic definitions used in the VM
 *      + persistent handles
 *      + bytecodes
 *      + important enumerations such as T_BOOLEAN, JVM_CONSTANT_String, etc
 */
struct JVMFlag;
class Globals {
 public:
  static bool int_value(char* name, int& value);
  static bool int_value_put(char* name, int value);

  static bool bool_value(char* name, bool& value);
  static bool bool_value_put(char* name, bool value);

  static bool set_numeric_flag(char* name, char* value);

  // Parse the string as argument for setting flag
  // Returns whether the argument was accepted.
  static bool parse_argument(char* arg);

  static void print_version();

  static PRODUCT_CONST JVMFlag* find_flag(char* name);

#ifndef PRODUCT
  static void verify();
#endif

#if USE_DEBUG_PRINTING
  static void print_flags();
  static void print_flags(void *st);
  static void print_build_options();
  static void print_build_options(void *st);
  static void print_error_codes();
  static bool match_flag(const char *flag_name, const char *pattern);
#endif

#if USE_DEBUG_PRINTING
public:
  static void print_definitions();

private:
  static void dump_interpreter_registers();
  static void dump_bytecodes();
  static void dump_persistent_handles();
  static void dump_constantpool_enums();
  static void dump_access_flags();
#endif

};

// "develop" flags are settable / visible only during
// development and are constant in the PRODUCT version

// "product" flags are always settable / visible
// regardless of the build mode

#define GENERIC_RUNTIME_FLAGS(develop, product)                             \
  /* Flags for generating the interpreter loop */                           \
  develop(bool, GenerateAssemblyCode, false,                                \
          "Generate assembly code output")                                  \
                                                                            \
  develop(bool, GenerateOopMaps, false,                                     \
          "Generate OopMaps.cpp")                                           \
                                                                            \
  develop(bool, GenerateDebugAssembly, false,                               \
          "Generate assembly code containing debug assertions")             \
                                                                            \
  develop(bool, GenerateGPTableOnly, false,                                 \
          "Generate only the GP table (used by AOT-enabled romizer")        \
                                                                            \
  develop(bool, GenerateThumbInterwork, false,                              \
          "Enable ARM/THUMB interworking in the generated assembly code")   \
                                                                            \
  develop(bool, GenerateGNUCode, false,                                     \
          "Generate assembly code output for GNU assembler")                \
                                                                            \
  develop(bool, GenerateMicrosoftCode, false,                               \
          "Generate assembly code output for Microsoft assembler")          \
                                                                            \
  develop(bool, GenerateInlineAsm, false,                                   \
          "Generate assembly code output as C with inline assembler")       \
                                                                            \
  develop(bool, AddExternCUnderscore, false,                                \
          "Add a _ prefix to extern C symbols in assembly code output")     \
                                                                            \
  develop(bool, EnableAlignmentInInterpreter, false,                        \
          "Align template entries if align_code_base is set")               \
                                                                            \
  develop(bool, GenerateBruteForceICacheFlush,ENABLE_BRUTE_FORCE_ICACHE_FLUSH,\
          "Generate brute-force icache flushing (large number of no-ops")   \
                                                                            \
  develop(int, BruteForceICacheFlushSize, 32 * 1024,                        \
          "Number of bytes in the brute-force icache flushing function")    \
                                                                            \
  develop(int, MinimumCompileSpace, 30 * 1024,                              \
          "Minimum free heap space to allow compiler invocation")           \
                                                                            \
  product(int, CompilationAbstinenceTicks, 0,                               \
          "Number of ticks to abstain from compilation after incurring "    \
          "compilation or GC")                                              \
                                                                            \
  /* Flags for testing the ROM image */                                     \
  develop(bool, LoadROMDebugSymbols, true,                                  \
          "If enabled, Java stack traces are printed with more information")\
                                                                            \
      /* GC  and heap Flags */                                              \
  product(int, HeapCapacity, 1 * 1024 * 1024,                               \
          "Maximum size of object heap in bytes")                           \
                                                                            \
  product(int, HeapMin, 1 * 1024 * 1024,                                    \
          "Minimin heap size in bytes. 0 means for the system to pick "     \
          "a good initial value. This flag is ignored "                     \
          "if SUPPORTS_ADJUSTABLE_MEMORY_CHUNK=0")                          \
                                                                            \
  develop(int, YoungGenerationTarget, 5,                                    \
          "Target size of young generation (as fraction of heap size, "     \
          "e.g. 5 is 20%")                                                  \
                                                                            \
  develop(int, YoungGenerationSurvivalTargetPercentage, 10,                 \
          "If the survival rate in a young GC is smaller than this, "       \
          "do not expand the young generation")                             \
                                                                            \
  develop(bool, YoungGenerationAtEndOfHeap, false,                          \
          "Put young generation at end of the heap")                        \
                                                                            \
  develop(int, MimimumMarkingStackSize, 2 * 1024,                           \
          "Minimum number of elements available on marking stack")          \
                                                                            \
  develop(int, ExcessiveGC, 0,                                              \
          "Call collect() regularly regardless of used heap. =ExcessiveGCn "\
          "meaps calling collect() at every n-th invocation of "            \
          "ObjectHeap::allocate(). =ExcessiveGC0 turns this feature off")   \
                                                                            \
  develop(int, VerifyGC, 0,                                                 \
          "Verify heap before/after GC. More verifications for higher "     \
          "values")                                                         \
                                                                            \
  develop(int, GCDummies, 0,                                                \
          "Dummy objects allocated at bottom of heap ensuring all objects " \
          "move at GC")                                                     \
                                                                            \
  product(int, CompilerAreaPercentage, 20,                                  \
          "Maximum percentage of heap to use by JIT compiler")              \
                                                                            \
  product(int, MinimumCompilerAreaPercentage, 0,                            \
          "Minimum percentage of heap to use by JIT compiler")              \
                                                                            \
  product(int, CompilerAreaSlack, 0,                                        \
          "Minimum amount of free memory reserved for JIT compiler")        \
                                                                            \
  develop(int, ExcessiveSuspendCompilation, 0,                              \
          "Always suspend compilation after processing each compilation "   \
          "queue element (for debugging background compilation")            \
                                                                            \
      /* Compiler flags */                                                  \
  product(bool, UseCompiler, true,                                          \
          "Should the compiler be used? (false is interpreted mode")        \
                                                                            \
  develop(bool, OptimizeArrayCopy, true,                                    \
          "Use native library for arraycopy")                               \
                                                                            \
  develop(bool, InstallCompiledCode, true,                                  \
          "Should the compiled code be used?")                              \
                                                                            \
  product(bool, MixedMode, true,                                            \
          "Execution mode: (has no effect if !UseCompiler)"                 \
          " true  -> dynamic recompilation"                                 \
          " false -> compile methods before they are executed")             \
                                                                            \
  develop(bool, AbortOnFailedKNILookup, false,                              \
          "Abort the VM when native code tries to look up "                 \
          "non-existent (or renamed) fields using KNI_GetFieldID(). "       \
          "This can be used to check if you have accidentally used "        \
          "ROM optimization options to renamed fields that must not be "    \
          "renamed because they are used by KNI_GetFieldID().")             \
                                                                            \
  develop(bool, UseQuickNativeMethods, true,                                \
          "Execute quick native methods without create a Java frame")       \
                                                                            \
  develop(bool, ShareExceptionStubs, true,                                  \
          "Share exception thrower stubs for compiled methods.")            \
                                                                            \
  develop(bool, InlineIfExceptions, true,                                   \
          "Inline methods that can throw exceptions.")                      \
                                                                            \
  develop(bool, ResolveConstantPoolInCompiler, true,                        \
          "Try to resolve constant pool entries inside the compiler "       \
          "if possible")                                                    \
                                                                            \
  develop(bool, UseVSFMergeOptimization, true,                              \
          "Use optimized VSF merge implementation")                         \
                                                                            \
  develop(bool, GenerateCompilerAssertions, COMPILER_ASSERTION_DEFAULT,     \
          "Generate assertion in compiled code (DEBUG mode only)")          \
                                                                            \
  product(bool, UseHandlers, true,                                          \
          "Thumb2 EE only: generate code using handlers")                   \
                                                                            \
      /* Threading flags */                                                 \
  product(int, StackSize, 4 * 1024,                                         \
          "Minimum stack size in bytes")                                    \
                                                                            \
  product(int, StackPadding, 1 * 1024 + 256,                                \
          "Total amount of stack padding in bytes")                         \
                                                                            \
  develop(int, LeafMethodStackPadding, 256,                                 \
          "The number of bytes in StackPadding reserved for leaf methods")  \
  develop(bool, DisableDeadlockFinder, false,                               \
          "Disable deadlock finder tool")                                   \
  product(int, StackSizeIncrement, 2 * 1024,                                \
          "stack size increment in bytes")                                  \
                                                                            \
  product(int, StackSizeMaximum, 128 * 1024,                                \
          "Max Java stack size in bytes")                                   \
                                                                            \
      /* Runtime flags */                                                   \
  product(bool, SlaveMode, false,                                           \
          "Run the VM in slave mode")                                       \
                                                                            \
  product(bool, UseVerifier, true,                                          \
          "Should class file verification be performed?")                   \
                                                                            \
  product(bool, VerifyOnly, false,                                          \
          "Verify all jar/zip files in the classpath, then exit")           \
                                                                            \
  develop(bool, GenerateCompilerComments, false,                            \
          "Generate comments from the compiler into relocation information")\
                                                                            \
  develop(bool, DumpVSFInComments, false,                                   \
          "Dump the VirtualStackFrames in compiler comments")               \
                                                                            \
  develop(bool, AbortOnInfiniteWait, false,                                 \
          "Abort the VM if all threads are waiting forever")                \
                                                                            \
  product(bool, LogVMOutput, false,                                         \
          "Log VM output on file cldc_vm.log")                              \
                                                                            \
  product(int, CachedAsyncDataSize, 8,                                      \
          "Precreated buffer size for SNI_AllocateReentryData")             \
                                                                            \
  product(bool, EnableLookupTableSizeHeuristic, true,                       \
          "Enable lookup table size heuristic")                             \
                                                                            \
  develop(int, MaxCachedJarParsers, 4,                                      \
          "The maximum number of JarFileParser objects that are kept "      \
          "open at any time. A higher number allows more Jar files "        \
          "to be processed efficiently at the same time")                   \
                                                                            \
  develop(bool, CacheJarEntries, ENABLE_JAR_ENTRY_CACHE,                    \
          "Enable caching JAR layout and entries,"                          \
          " when built with ENABLE_JAR_ENTRY_CACHE=true")                   \
                                                                            \
  develop(int, MaxJarCacheEntryCount, 256,                                  \
          "The maximum number of entries cached for a Jar file")            \
                                                                            \
  develop(bool, PrintAllObjects, false,                                     \
          "Print all object by iterating over the object heap")             \
                                                                            \
  develop(bool, RunFinalizationAtExit, true,                                \
          "Run all finalizers when JVM terminates")                         \
                                                                            \
  develop(bool, PrintLoadedClasses, false,                                  \
          "Print a class after class loading")                              \
                                                                            \
  develop(bool, PrintObjectHistogramData, false,                            \
          "Print a log for computing the object histogram")                 \
                                                                            \
  develop(bool, Deterministic, false,                                       \
          "Make VM behave more deterministically")                          \
                                                                            \
  develop(bool, PrintBytecodeHistogram, false,                              \
          "Prints a histogram of the executed bytecodes")                   \
                                                                            \
  develop(bool, PrintPairHistogram, false,                                  \
          "Prints a histogram of the executed bytecode pairs")              \
                                                                            \
  develop(bool, VerboseVSFMerge, false,                                     \
          "Print VSF merge information")                                    \
                                                                            \
  develop(bool, VerifyVSFMerge, true,                                       \
          "Verify VSF merge produces the correct result")                   \
                                                                            \
  develop(bool, RunCompilerTests, false,                                    \
          "Run compiler tests (use with -compilertestconfig)")              \
                                                                            \
  develop(int, BytecodeHistogramCutOff, 1,                                  \
          "The bytecode histogram cut-off percent")                         \
                                                                            \
  develop(int, PairHistogramCutOff, 1,                                      \
          "The pair histogram cut-off percent")                             \
                                                                            \
  product(int, MaxMethodToCompile, MAX_METHOD_TO_COMPILE,                   \
          "Don't compile method with more than this amount of bytecodes "   \
          "(in bytes)")                                                     \
                                                                            \
  product(int, MaxCompilationTime, 30,                                      \
          "Suspend compilation if a method takes more than this time "      \
          "to compile (in milliseconds.) MaxCompilationTime can be "        \
          "by reimplementing Os::check_compiler_timer()")                   \
                                                                            \
  product(int, InterpretationLogSize, INTERP_LOG_SIZE,                      \
          "How many elements of _interpretation_log[] to examine during "   \
          "timer tick -- set to 0 to disable interpretation log")


#if !ENABLE_SYSTEM_ROM_OVERRIDE && defined(ROMIZING)
#define USE_ROM_RUNTIME_FLAGS(develop, product, always) \
  always(bool, UseROM, 1,                                                   \
          "Should we load the system classes from ROMImage.o?")
#elif defined(ROMIZING)
#define USE_ROM_RUNTIME_FLAGS(develop, product, always) \
  develop(bool, UseROM, 1,                                                  \
          "Should we load the system classes from ROMImage.o?")
#else
#define USE_ROM_RUNTIME_FLAGS(develop, product, always) \
  always(bool, UseROM, 0,                                                   \
          "Should we load the system classes from ROMImage.o?")
#endif

#ifndef PLATFORM_RUNTIME_FLAGS
#define PLATFORM_RUNTIME_FLAGS(develop, product)
#endif


#if ENABLE_WTK_PROFILER                     
#define WTK_PROFILER_RUNTIME_FLAGS(develop, product)                        \
  product(bool, UseExactProfiler, false,                                    \
         "Use exact profiler")                                   \
  product(bool, SaveSerialProfiles, false,                                  \
         "Save a serial of profiles in graph0.prf, graph1.prf, ..., "   \
         "when the VM is restarted inside the same process")
#else
#define WTK_PROFILER_RUNTIME_FLAGS(develop, product)
#endif

// If enable ENABLE_JVMPI_PROFILE, support +UseJvmpiProfiler parameter 
// to enable the JVMPI events sending.
#if ENABLE_JVMPI_PROFILE 
#define JVMPI_PROFILE_RUNTIME_FLAGS(develop, product)                       \
  product(bool, UseJvmpiProfiler, false,                                    \
         "Support JVMPI profile")
#else
#define JVMPI_PROFILE_RUNTIME_FLAGS(develop, product)
#endif

// Set the JVMPI verification time interval. Default value is 0. The
// recommed value is about 2000 (ms).
#if ENABLE_JVMPI_PROFILE_VERIFY && ENABLE_JVMPI_PROFILE
#define JVMPI_PROFILE_VERIFY_RUNTIME_FLAGS(develop, product)                \
    product(int, JvmpiVerifyDumpInterval, 0,                                \
           "Set the timer to dump the jvmpi events")
#else
#define JVMPI_PROFILE_VERIFY_RUNTIME_FLAGS(develop, product)
#endif

// If ENABLE_PROFILER, support +UseProfiler also for product build
#if ENABLE_PROFILER
#define PROFILER_RUNTIME_FLAGS(develop, product)                            \
  product(bool, UseProfiler, false,                                         \
          "Use execution time profiler")
#else
#define PROFILER_RUNTIME_FLAGS(develop, product)                            \
  develop(bool, UseProfiler, false,                                         \
          "Use execution time profiler")
#endif

#ifndef USE_EVENT_LOGGER
#error USE_EVENT_LOGGER must be defined
#endif

#if USE_EVENT_LOGGER
#define EVENT_LOGGER_RUNTIME_FLAGS(develop, product)\
  product(bool, UseEventLogger, false,                                     \
          "Enable EventLogger, and print event log at VM exit")            \
  product(bool, LogEventsToFile, false,                                    \
          "If true, write the event log into event.log. Otherwise dump to "\
          "tty")
#else
#define EVENT_LOGGER_RUNTIME_FLAGS(develop, product)
#endif

/*
 * The following flags are 'product' flags if ENABLE_PERFORMANCE_COUNTERS
 * if false; 'develop' flags otherwise. These flags are useful in discovering
 * the best configuration for a given platform.
 */

#if ENABLE_ISOLATES
  #define ISOLATE_PERFORMANCE_COUNTERS_RUNTIME_FLAGS(op) \
    op(bool, PrintIsolateMemoryUsage, false,                               \
             "Print max memory usage for every isolate")                   \

#else
  #define ISOLATE_PERFORMANCE_COUNTERS_RUNTIME_FLAGS(op)
#endif

#define __PERFORMANCE_COUNTERS_RUNTIME_FLAGS(op) \
  op(bool, OptimizeLoops, true,                                            \
          "Make the compiler optimize loop code")                          \
  op(bool, OptimizeForwardBranches, USE_OPT_FORWARD_BRANCH,                \
          "Optimize simple forward branches")                              \
  op(int, LoopPeelingSizeLimit, 100,                                       \
          "Do not peel the loop if generated code for first run exceeds "  \
          "this limit (in bytes)")                                         \
  op(int, CompiledCodeFactor, 15,                                          \
          "Compute the maximum compiled code size using method code size") \
  op(int, ArrayCopyLoopUnrollingLimit, 10,                                 \
          "Limit on the number of load instructions for unrolled loop"     \
          "for inlined arraycopy (only for ENABLE_INLINED_ARRAYCOPY)")     \
  op(int, ArrayCopyByteInlineLimit, 48,                                    \
          "Maximum length of byte array to be copied with inlined"         \
          "arraycopy (only for ENABLE_INLINED_ARRAYCOPY)."                 \
          "Tune value for your platform.")                                 \
  op(int, ArrayCopyShortInlineLimit, 64,                                   \
          "Maximum length of short array to be copied with inlined"        \
          "arraycopy (only for ENABLE_INLINED_ARRAYCOPY)."                 \
          "Tune value for your platform.")                                 \
  op(bool, PrintPerformanceCounters, false,                                \
          "Print a choice group of 'important' perf counters at VM exit")  \
  op(bool, PrintAllPerformanceCounters, false,                             \
          "Print all performance counters")                                \
  op(bool, PrintCompilerPerformanceCounters, false,                        \
          "Print performance counters related to the compiler")            \
  op(bool, PrintGCPerformanceCounters, false,                              \
          "Print performance counters related to GC")                      \
  op(bool, PrintLoadingPerformanceCounters, false,                         \
          "Print performance counters related to class loading, "          \
          "verifier and binary ROM conversion")                            \
  op(bool, PrintRomizerPerformanceCounters, false,                         \
          "Print performance counters related to romization")              \
  op(bool, PrintThreadPerformanceCounters, false,                          \
          "Print performance counters related to threads and events")      \
  op(bool, RetryCompilation, true,                                         \
          "Retry compilation if CompiledCodeFactor is too small")          \
  op(bool, TestCompiler, false,                                            \
          "Compile all methods specified in the classpath.")               \
  op(bool, TestCompilerStats, false,                                       \
          "Print out statistics during TestCompiler.")                     \
  op(bool, CollectBeforeCompilerTest, true,                                \
          "Do a full GC before compiling each method with +TestCompiler.") \
  op(bool, TestCompileSystemClasses, false,                                \
          "Also compile the methods in the (romized) system classes in "   \
          "in -TestCompiler mode.")                                        \
  op(bool, EnableTicks, true,                                              \
          "Enable simulated timer ticks (for scheduling and compilation)") \
  op(bool, EnableVMHints, true,                                            \
          "Allows JVM_SetHint to take effect")                             \
  op(int, RomizerSleepTime, 0,                                             \
          "Milliseconds to sleep at each invocation to "                   \
          "{Source,Binary}ROMWriter::execute(). This allows testing UI "   \
          "for showing romization progress and cancellation ")             \
  op(bool, PrintVmMIPS, false,                                          \
          "Print VmMIPS scores for ARM CPU."                            \
          "Requires ENABLE_VM_MIPS.")                                   \
  ISOLATE_PERFORMANCE_COUNTERS_RUNTIME_FLAGS(op)

#define __TTY_TRACE_RUNTIME_FLAGS(op, develop)                              \
       op(bool, PrintCompilation, false,                                    \
          "Prints a line for each compiled method")                         \
                                                                            \
       op(bool, PrintCompilationAtExit, false,                              \
          "Prints a line for each compiled method (at VM exit)")            \
                                                                            \
       op(bool, PrintCompiledCode, false,                                   \
          "Prints the native code for all compiled methods")                \
                                                                            \
       op(bool, PrintCompiledCodeAsYouGo, false,                            \
          "Prints the native code for all compiled methods")                \
                                                                            \
       op(bool, PrintLongFrames, false,                                     \
          "Print locals and expression stack when printing frames")         \
                                                                            \
       op(bool, PrintExtraLongFrames, false,                                \
          "Print very detailed information when printing frames")           \
                                                                            \
       op(bool, Verbose, false,                                             \
          "Print additional debugging information")                         \
                                                                            \
       op(bool, VerbosePointers, true,                                      \
          "Print value of pointers in debugging information (ignored "      \
          "if Verbose=0")                                                   \
                                                                            \
       op(bool, TraceMethodInlining, false,                                 \
          "Trace method inlining (only for ENABLE_INLINE)")                 \
                                                                            \
       op(bool, TraceNativeCalls, false,                                    \
          "Trace native method calls")                                      \
                                                                            \
       op(bool, TraceCompiledMethodCache, false,                            \
          "Trace compiled method cache events")                             \
                                                                            \
       op(bool, TraceDebugger, false,                                       \
          "Trace Java debugger support operations")                         \
                                                                            \
       op(bool, TraceRomizer, false,                                        \
          "Trace operations by the (source and binary) romizer")            \
                                                                            \
       op(bool, TraceThreadEvents, false,                                   \
          "Trace thread events (transfer, yield, etc...)")                  \
                                                                            \
       op(bool, TraceThreadsExcessive, false,                               \
          "Trace thread events (transfer, yield, etc...)")                  \
                                                                            \
       op(bool, TraceVerifier, false,                                       \
          "Trace class file verification")                                  \
                                                                            \
       op(bool, TraceStackmaps, false,                                      \
          "Trace stack map generation")                                     \
                                                                            \
       op(bool, TraceStackmapsVerbose, false,                               \
          "Trace stack map generation verbose")                             \
                                                                            \
       op(bool, TraceVerifierByteCodes, false,                              \
          "Trace bytecodes during verification")                            \
                                                                            \
       op(bool, TraceOSR, false,                                            \
          "Trace on-stack-replacements")                                    \
                                                                            \
       op(bool, TraceUncommonTrap, false,                                   \
          "Trace uncommon-traps that are taken")                            \
                                                                            \
       op(bool, TraceClassLoading, false,                                   \
          "Print a line when a class is loaded")                            \
                                                                            \
       op(bool, TraceClassUnloading, false,                                 \
          "Print a line when a class is unloaded")                          \
                                                                            \
       op(bool, EnableClassUnloading, false,                                \
          "Enable removal of unused classes")                               \
                                                                            \
       op(bool, TraceClassInitialization, false,                            \
          "Print a line when a class is being initialized")                 \
                                                                            \
       op(bool, TraceExceptions, false,                                     \
          "Trace exception throwing (with stack traces)")                   \
                                                                            \
       op(bool, TraceGC, false,                                             \
          "Verbose trace of GC")                                            \
                                                                            \
       op(bool, TraceHeapSize, false,                                       \
          "Verbose trace of heap growth/shrinking")                         \
                                                                            \
       op(bool, TraceCompilerGC, false,                                     \
          "Verbose trace of GC in compiler_area")                           \
                                                                            \
       op(bool, TraceFailedCompilation, false,                              \
          "Verbose trace when method fails to compile")                     \
                                                                            \
       op(bool, TraceFinalization, false,                                   \
          "Verbose trace of finalization behavior")                         \
                                                                            \
       op(bool, TraceJarCache, false,                                       \
          "Verbose trace of JarFile cache")                                 \
                                                                            \
       op(bool, VerboseGC, false,                                           \
          "Print user-level GC information")                                \
                                                                            \
       op(bool, VerboseClassLoading, false,                                 \
          "Print user-level class loading information")                     \
                                                                            \
       op(bool, TraceBytecodes, false,                                      \
          "Generate interpreter that traces bytecodes")                     \
                                                                            \
       op(bool, TraceBytecodesCompiler, false,                              \
          "Generate compiled code that traces bytecodes")                   \
                                                                            \
       op(int, TraceBytecodesStart, 0,                                      \
          "Start printing trace at given bytecode number")                  \
                                                                            \
       op(int, TraceBytecodesInterval, 1,                                   \
          "Start printing trace at given bytecode number")                  \
                                                                            \
       op(int, TraceBytecodesStop, -1,                                      \
          "Stop at given bytecode")                                         \
                                                                            \
       op(int, TraceBytecodesVerbose, 0,                                    \
          "More verbose output bytecode")                                   \
                                                                            \
       op(bool, TraceCallInfo, false,                                       \
          "Trace call info records")                                        \
                                                                            \
       op(bool, TracePageAccess, false,                                     \
          "Trace access to the protected page")                             \
                                                                            \
       op(bool, TraceMirandaMethods, false,                                 \
          "Print miranda methods (abstract interface methods added"         \
          " silently by the VM")                                            \
       op(bool, TraceTaskContext,  false,                                   \
          "Trace task switching")                                                \
       op(bool, VerboseNullPointExceptionThrowing,  false,                                   \
          "Print verbose null point exception information")                                      \
       op(bool, VerboseByteCodeEliminate,  false,                                   \
          "Print byte code eliminate information ")   
          

/*
 * The "optional()" flags would be "product()" in product builds w/o Monet
 * support, and "develop()" in all other builds.
 */
#define __ROM_GENERATOR_FLAGS(develop, product, optional) \
  optional(bool, GenerateROMImage, false,                                   \
          "Generate ROM image")                                             \
                                                                            \
  optional(bool, GenerateSharedROMImage, false,                             \
          "Generate ROM image which could be used shared")                  \
                                                                            \
  develop(bool, EnableAllROMOptimizations, false,                           \
          "Turn on all ROM optimizations")                                  \
                                                                            \
  develop(bool, MakeROMDebuggable, false,                                   \
          "Turn off ROM optimizations that change bytecode offsets")        \
                                                                            \
  develop(bool, GenerateROMComments, false,                                 \
          "Generate comments in the ROM image")                             \
                                                                            \
  develop(bool, VerboseROMComments, false,                                  \
          "More verbose comments in the ROM image")                         \
                                                                            \
  develop(bool, LogROMPercentages, true,                                    \
          "Print percentages in ROMLog.txt file")                           \
                                                                            \
  develop(bool, GenerateROMStructs, false,                                  \
          "Generate ROMStructs.h for accessing Romized classes in C code")  \
                                                                            \
  develop(bool, GenerateROMStructsStatics, false,                           \
              "Add structs to ROMStructs.h for accessing static fields")    \
                                                                            \
  develop(int, ROMHashTableDepth, 8,                                        \
          "Average depth of each bucket in the ROM Symbol/String tables")   \
                                                                            \
  optional(bool, EmbeddedROMHashTables, true,                               \
          "Embed ROM hashtables in merged constant pool")                   \
                                                                            \
  develop(bool, GenerateRelaunchableROM, true,                              \
          "Generate a ROM image that can be relaunched in the same process")\
                                                                            \
  optional(bool, RewriteROMConstantPool, true,                              \
          "Rewrite constant pools in ROM to reduce static footprint")       \
                                                                            \
  optional(int, MergedConstantPoolLimit, 65535,                             \
          "Max size of a merged ConstantPool in ROM")                       \
                                                                            \
  optional(bool, RemoveDeadMethods, true,                                   \
          "Remove non-app accessible methods in ROM?")                      \
                                                                            \
  optional(bool, RenameNonPublicROMClasses, false,                          \
          "Rename non-public classes in ROM?")                              \
                                                                            \
  optional(bool, RenameNonPublicROMSymbols, false,                          \
          "Rename non-public fields/methods in ROM?")                       \
                                                                            \
  optional(bool, AggressiveROMSymbolRenaming, false,                        \
          "Rename all fields/methods in non-public romized classes?")       \
                                                                            \
  optional(bool, CompactROMFieldTables, false,                              \
          "Remove redundant entries from field tables of romized classes")  \
                                                                            \
  optional(bool, CompactROMBytecodes, true,                                 \
          "Replace ROM bytecodes with equivalent but shorter sequences")    \
                                                                            \
  optional(bool, CompactROMMethodTables, false,                             \
          "Remove redundant entries from method tables of romized classes") \
                                                                            \
  develop(bool, MakeRestrictedPackagesFinal, true,                          \
          "If possible, make all classes in a restricted package 'final'")  \
                                                                            \
  optional(bool, SimpleROMInliner, true,                                    \
          "Inline simple methods to caller, improve speed and footprint")   \
                                                                            \
  optional(bool, RemoveDuplicatedROMStackmaps, true,                        \
          "Eliminate redundant entries in romized StackmapList")            \
                                                                            \
  develop(bool, RemoveROMUnusedStaticFields, true,                          \
          "Remove all unused static fields from romized classes")           \
                                                                            \
  optional(bool, EnableROMCompilation, true,                                \
          "Compile some ROM methods to machine code")                       \
                                                                            \
  develop(bool, RemoveUnusedSymbols, true,                                  \
          "Remove unused symbols, reduce footprint")                        \
                                                                            \
  optional(bool, OptimizeBytecodes, true,                                   \
          "Replace some bytecodes with shorter sequences")                  \
                                                                            \
  product(bool, OmitLeafMethodFrames, OMIT_LEAF_FRAME_DEFAULT,              \
          "Do not generate call frames for leaf methods when possible "     \
          "(ARM only. IMPL_NOTE: change to a product/true flag)")           \
                                                                            \
  optional(bool, RemoveConvertedClassFiles, false,                          \
          "Remove converted class files from JAR files in classpath")       \
                                                                            \
  optional(bool, PostponeErrorsUntilRuntime, ENABLE_MONET,                  \
          "Don't abort romizer on an error caused by invalid class file. "  \
          "Instead, produce a valid ROM image that reports this error at "  \
          "run-time. By default enabled for application classes "           \
          "conversion and disabled for system classes conversion")          \
                                                                            \
  develop(bool, SaveSerialROMLogs, false,                                   \
         "Save a serial of ROM debug dumps and logs generated in the same " \
         "process in ROM_{dump,log}.txt.00000, ROM_{dump,log}.txt.00001,..")\
                                                                            \
  develop(bool, EnableAOTSymbolTable, false,                                \
          "Generate symbol table for AOT-compiled methods (for certain "    \
          "debuggers/emulators that supports this table")                   \
                                                                            \
  product(int, MaxRomizationTime, 30,                                       \
          "Suspend romization if {Source,Binary}ROMWriter::execute() "      \
          "has spent more than this number of milliseconds in a single "    \
          "invocation, so as to eliminate VM pauses due to romization")     \
                                                                            \
  product(int, LoadXIPImage, USE_IMAGE_MAPPING,                             \
          "Load binary images in XIP mode (execute in place in flash "      \
          "to save RAM usage")                                              \
                                                                            \
  product(bool, ForceImageRelocation, false,                                \
          "Force images to be loaded at an address different than the "     \
          "desired location")                                               \
                                                                            \
  optional(bool, EnableBaseOptimizations, true,                             \
          "Do basic optimizations for ROM generator")


#if ENABLE_ROM_GENERATOR
#define   ROM_GENERATOR_FLAGS(develop, product)                            \
        __ROM_GENERATOR_FLAGS(develop, product, product)
#else
#define   ROM_GENERATOR_FLAGS(develop, product)                            \
        __ROM_GENERATOR_FLAGS(develop, product, develop)
#endif

#if ENABLE_PERFORMANCE_COUNTERS
#define PERFORMANCE_COUNTERS_RUNTIME_FLAGS(develop, product)               \
      __PERFORMANCE_COUNTERS_RUNTIME_FLAGS(product)
#else
#define PERFORMANCE_COUNTERS_RUNTIME_FLAGS(develop, product)               \
      __PERFORMANCE_COUNTERS_RUNTIME_FLAGS(develop)
#endif

#if ENABLE_TTY_TRACE
#define   TTY_TRACE_RUNTIME_FLAGS(always, develop, product) \
        __TTY_TRACE_RUNTIME_FLAGS(product, product)
#else
#define   TTY_TRACE_RUNTIME_FLAGS(always, develop, product) \
        __TTY_TRACE_RUNTIME_FLAGS(develop, develop)
#endif

#if !ENABLE_REMOTE_TRACER
#define REMOTE_TRACER_RUNTIME_FLAGS(develop, product)
#else
#define REMOTE_TRACER_RUNTIME_FLAGS(develop, product)                       \
  product(int, RemoteTracePort, -1,                                         \
          "Enable remote tracing of certain VM parameters by setting port")
#endif

#if !ENABLE_JAVA_DEBUGGER
#define DEBUGGER_RUNTIME_FLAGS(develop, product)
#else
#define DEBUGGER_RUNTIME_FLAGS(develop, product)                            \
  product(int, DefaultDebuggerPort, 2800,                                   \
          "Default port that debugger listens on")
#endif

#if !ENABLE_ISOLATES
#define ISOLATES_RUNTIME_FLAGS(develop, product)
#else
#define ISOLATES_RUNTIME_FLAGS(develop, product)                            \
  product(bool, TaskFairScheduling, true,                                   \
            "Give each task a fair chance to run regardless of thread "     \
            "priority ")                                                    \
                                                                            \
  product(int, SchedulerDivisor, 2,                                         \
            "Divide ticks/sec by this to get number of tokens for normal "  \
            "priority task")                                                \
                                                                            \
  product(int, TaskPriorityScale, 1,                                        \
          "Adjusts scaling of priorities between high and low")             \
                                                                            \
  develop(int, ProhibitCompiledCIB, 0,                                      \
          "Prohibits compilation if it must include a specific barrier")    \
                                                                            \
  develop(int, EmptyCompiledCIB, 0,                                         \
          "Generate marker code only for specific barrier")                 \
                                                                            \
  develop(bool, StopAtCIBHit, false,                                        \
          "Generate breakpoint at barrier hit in compiled methods")         \
                                                                            \
  develop(int, StopAtCompiledCIB, 0,                                        \
          "Generate breakpoint at barrier in compiled methods")             \
                                                                            \
  develop(int, StopAtCompiledStaticAccess, 0,                               \
          "Generate breakpoint at static var access in compiled methods")   \
                                                                            \
  develop(int, StopAtCompiledCIBClearance, 0,                               \
          "Generate breakpoint at barrier mark clearance in "               \
          "compiled methods")                                               \
                                                                            \
  develop(bool, StopAtRealMirrorAccess, false,                              \
          "Generate breakpoint upon getting the java mirror for the "       \
          "current task")                                                   \
                                                                            \
  develop(bool, StopAtNullPtrException, false,                              \
          "Generate compiled code breakpoint upon throwing a null "         \
          "pointer exception")                                              \
                                                                            \
  product(int, ReservedMemory, 0,                                           \
            "Reserved memory for the first isolate")                        \
                                                                            \
  product(int, TotalMemory, max_jint,                                       \
            "Total memory for the first isolate")                           \

#endif

#if ENABLE_INTERNAL_CODE_OPTIMIZER
#if ENABLE_CODE_OPTIMIZER
#define CODE_OPTIMIZER_RUNTIME_FLAGS(develop, product)                      \
  product(bool, OptimizeCompiledCode, true,                                 \
          "Optimize the code generated by the dynamic compiler")            \
                                                                            \
  develop(bool, OptimizeCompiledCodeVerbose, false,                         \
          "Print out instructions changed by the code optimizer")           \
                                                                            \
  product(bool, ExtendBasicBlock, true,                                     \
        "Print out instructions changed by the code optimizer")             \
                                                                            \
  develop(bool, OptimizeCompiledCodeVerboseInternal, false,                 \
          "Print out instructions changed by the code optimizer")
        

#else
#define CODE_OPTIMIZER_RUNTIME_FLAGS(develop, product)                      \
  develop(bool, OptimizeCompiledCode, false,                                \
          "Optimize the code generated by the dynamic compiler")            \
                                                                            \
  develop(bool, OptimizeCompiledCodeVerbose, false,                         \
          "Print out instructions changed by the code optimizer")           \
                                                                            \
  develop(bool, OptimizeCompiledCodeVerboseInternal, false,                 \
          "Print out instructions changed by the code optimizer")           \
                                                                            \
  product(bool, ExtendBasicBlock, true,                                     \
          "Print out instructions changed by the code optimizer")

#endif
#else
#if ENABLE_CODE_OPTIMIZER
#define CODE_OPTIMIZER_RUNTIME_FLAGS(develop, product)                      \
  product(bool, OptimizeCompiledCode, true,                                 \
          "Optimize the code generated by the dynamic compiler")            \
                                                                            \
  develop(bool, OptimizeCompiledCodeVerbose, false,                         \
          "Print out instructions changed by the code optimizer")

#else
#define CODE_OPTIMIZER_RUNTIME_FLAGS(develop, product)                      \
  develop(bool, OptimizeCompiledCode, false,                                \
          "Optimize the code generated by the dynamic compiler")            \
                                                                            \
  develop(bool, OptimizeCompiledCodeVerbose, false,                         \
          "Print out instructions changed by the code optimizer")
#endif
#endif

#if ENABLE_CPU_VARIANT
#define CPU_VARIANT_RUNTIME_FLAGS(develop, product)                         \
  develop(bool, EnableCPUVariant, true,                                     \
          "Allow interpreter to use CPU variant features (Jazelle)"         \
          " if available")                                                  \
                                                                            \
  product(bool, EnableConfigurableBytecodes, true,                          \
          "Use Jazelle configurable bytecodes")
#else
#define CPU_VARIANT_RUNTIME_FLAGS(develop, product)
#endif

#define RUNTIME_FLAGS(develop, product, always)            \
      GENERIC_RUNTIME_FLAGS(develop, product)              \
      USE_ROM_RUNTIME_FLAGS(develop, product, always)      \
      PROFILER_RUNTIME_FLAGS(develop, product)             \
      EVENT_LOGGER_RUNTIME_FLAGS(develop, product)         \
      ROM_GENERATOR_FLAGS(develop, product)                \
      PERFORMANCE_COUNTERS_RUNTIME_FLAGS(develop, product) \
      PLATFORM_RUNTIME_FLAGS(develop, product)             \
      REMOTE_TRACER_RUNTIME_FLAGS(develop, product)        \
      DEBUGGER_RUNTIME_FLAGS(develop, product)             \
      ISOLATES_RUNTIME_FLAGS(develop,product)              \
      CODE_OPTIMIZER_RUNTIME_FLAGS(develop, product)       \
      WTK_PROFILER_RUNTIME_FLAGS(develop, product)         \
      JVMPI_PROFILE_RUNTIME_FLAGS(develop, product)        \
      JVMPI_PROFILE_VERIFY_RUNTIME_FLAGS(develop, product) \
      CPU_VARIANT_RUNTIME_FLAGS(develop, product)          \
      TTY_TRACE_RUNTIME_FLAGS(always, develop, product)

/*
 * Macros for declaring globals variables that can be modified by
 * command-line arguments to the VM.
 *
 * DECLARE_ALWAYS_FLAG:    The flag is always set as a constant, and cannot be
 *                         changed in debug, release or product modes.
 * DECLARE_PRODUCT_FLAG:   The flag is set as a constant in product mode,
 *                         but can be changed in debug and release modes.
 * DECLARE_DEVELOPER_FLAG: The flag may be changed in all modes.
 */

#define DECLARE_ALWAYS_FLAG(type, name, value, doc) \
    const type name = value;

#define DECLARE_PRODUCT_FLAG(type, name, value, doc) \
    extern "C" type name;

#ifdef PRODUCT
#define DECLARE_DEVELOPER_FLAG(type, name, value, doc) \
    const type name = value;
#else
#define DECLARE_DEVELOPER_FLAG(type, name, value, doc)  \
    extern "C" type name;
#endif

#define DEFINE_ALWAYS_FLAG(type, name, value, doc) \
    /* do nothing: flag name is constant */

#define DEFINE_PRODUCT_FLAG(type, name, value, doc) \
    type name = value;

#ifdef PRODUCT
#define DEFINE_DEVELOPER_FLAG(type, name, value, doc) \
    /* do nothing: flag name is constant */
#else
#define DEFINE_DEVELOPER_FLAG(type, name, value, doc) \
    type name = value;
#endif

RUNTIME_FLAGS(DECLARE_DEVELOPER_FLAG,DECLARE_PRODUCT_FLAG,DECLARE_ALWAYS_FLAG)

/*
 * Macros for identifying the VM version.
 */

#if defined PRODUCT
#define JVM_VARIANT "product"
#elif defined  AZZERT
#define JVM_VARIANT "debug"
#else
#define JVM_VARIANT "release"
#endif

#ifndef JVM_RELEASE_VERSION
#define JVM_RELEASE_VERSION "internal"
#endif

#ifndef JVM_BUILD_VERSION
#define JVM_BUILD_VERSION "internal"
#endif

#ifndef JVM_NAME
#define JVM_NAME "CLDC VM"
#endif
