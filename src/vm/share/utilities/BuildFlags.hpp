/*
 *   
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
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

/** \file BuildFlags.hpp
 * Compilation flags.
 *
 * All external compilation flags (especially '-D...' constituents of
 * 'CFLAGS' in our makefiles) that are addressed by #ifdef of #if in
 * the source code are explained here, even if they are not specified
 * by us, tool-specific or platform-specific.  This is the one place
 * where you find explanations for them all.
 *
 * All implications between compilations flags are encoded here.
 */

/*
 * Flags set by the C/C++ compiler we are using:
 */

// __cplusplus            Is set iff a C++ compiler is used, as opposed to
//                        a C compiler.

// __GNUC__               Is set iff gcc or g++ is used.

// _MSC_VER               Is set iff VisualC++ or Embedded Visual C++ is used.

/*
 * Flags that our build system sets inside makefiles.
 */

// __SYMBIAN_7__          Target OS is Symbian OS 7

// __WINC__               Target shell is Symbian WINC

// __WINS__               Target shell is Symbian WINS

// _DEBUG                 In a debug target build

// _X86_                  Generating X86 CPU instructions

// ARM                    Generating ARM CPU instructions

// AZZERT                 Add assertion code (debug build)

// LINUX                  Target OS is Linux

// CROSS_GENERATOR        In an loopgen or romgen build

// PRODUCT                In a product target build

// UNDER_ADS              Building with the ADS tool chain

// UNDER_CE               Target OS is Windows/CE and the
//                        tool chain is MS Embedded Visual C++

// WIN32                  Target platform is the Win32 API

#ifndef CROSS_GENERATOR
#define CROSS_GENERATOR 0
#endif

//============================================================================
// *** NOTE: the formatting of this block is important. Do not change. ***
//
//     This section (between the ENABLE_FLAGS_BEGIN and
//     ENABLE_FLAGS_END markers) is processed by the program
//     src/tools/buildtool/BuildTool.java to create the file jvmconfig.h.
//
//     The following table lists all the ENABLE_XXX flags that can be
//     used to configure the VM's behavior without editing code. An
//     ENABLE_XXX flag must have the value 0 or 1. The value is
//     determined by the following precedence rules:
//
//     [1] The "User" value, taken from your environment
//         variables when you build the VM.
//     [2] The "Platform Default" value, set in a platform
//         configuration file (e.g., build/linux_i386/linux_i386.cfg)
//     [3] The "VM Default" value, as listed in the following table.
//
//     I.e., create_jvmconfig_h.pl will first look for the User
//     value, then the Platform Default value, and finally the
//     VM Default value.
//
//     To set your User value, set an environment variable
//     like this:
//
//         set ENABLE_PERFORMANCE_COUNTERS=true
//         gnumake
//
//     To set the Platform Default value for your platform, add the
//     a few lines to your <platform>/<platform>.cfg file like this:
//
//         ifndef ENABLE_TIMER_THREAD
//         export ENABLE_TIMER_THREAD     := false
//         export ENABLE_TIMER_THREAD__BY := linux_i386.cfg
//         endif
//
//     The VM Default settings in the following table are in the
//     format <np>,<p>, where <np> is the default value for
//     non-PRODUCT builds, and <p> is the default value for the
//     PRODUCT build. <np> and <p> can have the following values:
//
//         0      This flag is disabled by default
//         1      This flag is enabled by default
//         +      This flag is always enabled,
//                regardless of User or Platform Default settings.
//         -      This flag is always disabled,
//                regardless of User or Platform Default settings.
//
//============================================================================
// {{ ENABLE_FLAGS_BEGIN
//============================================================================
// Name            VM Default (NP,P)  Meaning
//============================================================================
// ENABLE_ARM_V5TE               0,0  Support instructions in the ARMv5TE
//                                    architecture (such as BLX or CLZ).
//
// ENABLE_ARM_V6                 0,0  Support instructions in the ARMv6
//                                    architecture.
//
// ENABLE_ARM_V6T2               0,0  Support instructions in the ARMv6T2
//                                    architecture (Thumb-2).
//
// ENABLE_ARM_V7                 0,0  Support instructions in the ARMv7
//                                    architecture.
//
// ENABLE_ARM_VFP                0,0  Support ARM VFP instructions.
//
// ENABLE_ARM9_VFP_BUG_WORKAROUND 0,0  Workaround ARM9+VFP hardware feature
//
// ENABLE_ARM11_JAZELLE_DLOAD_BUG_WORKAROUND 0,0  Workaround ARM11 Jazelle hardware bug
//
// ENABLE_BRUTE_FORCE_ICACHE_FLUSH 0,0 Generate brute-force code to flush
//                                    the instruction cache (for platforms
//                                    without OS support).
//
// ENABLE_BYTECODE_FLUSHING      0,0  Special code needed for bytecode
//                                    rewriting
//
// ENABLE_C_INTERPRETER          0,0  Use Java interpreter written in C
//                                    code instead of the generated assembler
//                                    interpreter.
//
// ENABLE_CLDC_11                1,1  Support CLDC 1.1 Specification instead
//                                    of CLDC 1.0.
//
// ENABLE_CLDC_111               0,0  Support CLDC 1.1.1 Specification 
//                                    instead of CLDC 1.0.
//
// ENABLE_CODE_OPTIMIZER         0,0  Enable optimization of code generated
//                                    by dynamic compiler for a specific CPU.
//
// ENABLE_COMPILER               1,1  Add the dynamic adaptive compiler
//                                    for byte code execution.
//
// ENABLE_COMPILER_TYPE_INFO     1,1  Maintain object type information during
//                                    compilation.
//
// ENABLE_CPU_VARIANT            0,0  Enable specialized features for
//                                    a variant of the main CPU type.
//
// ENABLE_DISPATCH_TABLE_ALIGNMENT 0,0 Make dispatch table 1024 aligned
//
// ENABLE_DISPATCH_TABLE_PADDING 0,0  Add extra entries to the dispatch table
//
// ENABLE_DYNAMIC_NATIVE_METHODS 0,0  Add ability to execute user classes 
//                                    containing native methods.
//
// ENABLE_DYNAMIC_RESTRICTED_PACKAGE 0,0 Allow restricted packages to
//                                    be dynamically specified.
//
// ENABLE_FAST_CRC32             1,1  Use fast CRC32 routine? Adds 1KB 
//                                    footprint.
//
// ENABLE_FAST_MEM_ROUTINES      1,1  Use built-in memcmp and memcpy routines
//                                    in the generated interpreter loop.
//
// ENABLE_INLINEASM_INTERPRETER  0,0  If true, the interpreter loop is 
//                                    generated as a C file with
//                                    inlined assembly code.  This
//                                    option is used on x86 only.
//
// ENABLE_INLINED_ARRAYCOPY      1,1  Inline arraycopy() calls in compiled code
//
//
// ENABLE_INTERPRETATION_LOG     1,1  Use a log of the most recently
//                                    interpreted methods to make sure
//                                    hot methods are compiled. Disable this
//                                    option when running on slow devices.
//
// ENABLE_FLOAT                  1,1  Support floating point byte codes.
//
//
// ENABLE_INCREASING_JAVA_STACK  0,0  If true, the Java stack grows
//                                    upwards. E.g., when an item is
//                                    pushed, the stack pointer's value
//                                    increases.
//
// ENABLE_REFLECTION             0,0  Add Reflection support
//
// ENABLE_RVDS                   0,0  Support for ARM RealView
//                                    Developer Suite (2.0 or later)
//
// ENABLE_ISOLATES               0,0  Add Isolate support
//
// ENABLE_JAVA_DEBUGGER          0,0  Add Java debugger support.
//
// ENABLE_JAZELLE                0,0  Enable support for Jazelle(TM)
//                                    hardware acceleration of Java bytecode
//                                    execution. This is a meta flag
//                                    that enables a number of other
//                                    build flags.
//
// ENABLE_HEAP_NEARS_IN_HEAP     0,0  Ensure all nears of romized HEAP objects and prototypical nears
//                                    of all classes are in ROM HEAP block by cloning those nears
//                                    from ROM TEXT and DATA blocks to the ROM HEAP block. 
//                                    Speeds up GC, but slightly increases footprint. 
//
// ENABLE_PREINITED_TASK_MIRRORS 1,1  Put TaskMirror to a separate section of SystemROM image
//                                    to allow loading them during startup of each task.                                   
//
// ENABLE_LIB_IMAGES             0,0  (unsupported) allow loading of multiple
//                                    binary images into the same task.
//
// ENABLE_PAGE_PROTECTION        0,0  Use the mechanism of protected memory
//                                    pages for certain compiler optimizations
//                                    (e.g. check_timer_tick). Works only
//                                    if the feature is supported by OS.
//
// ENABLE_ZERO_YOUNG_GENERATION  1,1  Fills youngen with zero values after GC.
//                                    When the option is off each newly created
//                                    object is cleared right after allocation.
//
// ENABLE_MEMORY_MAPPED_FILES    1,1  Use memory-mapped files for
//                                    loading binary images. This flag takes
//                                    effect only if the target platform
//                                    has SUPPORTS_MEMORY_MAPPED_FILES=1.
//
// ENABLE_MEMORY_PROFILER        0,0  Add Memory Profiler support.
//
// ENABLE_ROM_JAVA_DEBUGGER      0,0  Add Java debugger support for ROMized.
//                                    classes
//
// ENABLE_SYSTEM_CLASSES_DEBUG   0,0  Build system classes with -g and
//                                    +MakeROMDebuggable.  Must be passed in
//                                    via make command line
//
// ENABLE_JAVA_DEBUGGER_OLD_JAVAC 0,0  Add special code in method entry
//                                    to clear locals if code compiled with old
//                                    compiler.
//
// ENABLE_JAVA_STACK_TAGS        0,0  Interleave values on the Java stack
//                                    with tags.
//
// ENABLE_JAR_ENTRY_CACHE        1,1  Cache the JAR entry table for fast
//                                    lookup.
//
// ENABLE_JAR_READER_EXPORTS     1,1  Export routines for the JAR reader.
//
//
// ENABLE_KVM_COMPAT             0,0  Support the kvmcompat module which
//                                    provides limited compatibility with
//                                    KVM-based (pre-KNI) native methods.
//
// ENABLE_NATIVE_ORDER_REWRITING 1,1  Enable rewriting of various bytecodes
//                                    so that its fields are in native ordering
//
// ENABLE_MEASURE_NATIVE_STACK   0,0  Measures the amount of native stack the
//                                    VM uses
//
// ENABLE_MINIMAL_ASSERT_BUILD   0,0  Turn off all non-product options
//                                    unrelated to run-time assertion checks.
//                                    This allows you to build a smaller
//                                    Debug mode VM that still has assertions
//                                    checks.
//
// ENABLE_MONET                  0,0  Enable on-device support (conversion
//                                    and loading) of binary application
//                                    image files for fast class loading.
//
// ENABLE_MONET_COMPILATION      0,0  Enable on-device method precompilation 
//                                    Requires ENABLE_MONET.
//
// ENABLE_MONET_DEBUG_DUMP       1,0  Create debug dump files that describe
//                                    the contents of binary ROM image files.
//
// ENABLE_VM_MIPS             0,0  Enable measuring the speed of ARM CPU.
//                                    Requires ENABLE_PERFORMANCE_COUNTERS.
//
// ENABLE_METHOD_TRAPS           0,0  Add MethodTrap= command-line option for
//                                    special handling of a particular
//                                    Java method invocation.
//                                    Used for startup time measurements.
//
// ENABLE_PCSL                   0,0  Enable support for the Portable Common 
//                                    Services Library.
//
// ENABLE_PERFORMANCE_COUNTERS   1,0  Enable coarse-grain performance counters.
//                                    for methods compiled, memory allocated,
//                                    etc. These counters usually do not
//                                    skew execution time.
//
// ENABLE_DETAILED_PERFORMANCE_COUNTERS 0,0  Enable fine-grain performance
//                                    counters. These counters may skew
//                                    execution time.
//
// ENABLE_EVENT_LOGGER           0,0  Enable event logging. Used for debugging,
//                                    performance and pause measurements.
//
// ENABLE_EXTENDED_EVENT_LOGGER  0,0  Enable event type extension. Used for debugging,
//                                    performance and pause measurements.
//
// ENABLE_PRODUCT_PRINT_STACK    -,0  Include the debug function
//                                    pss() in product build (useful
//                                    for diagnosing deadlocks.
//
// ENABLE_PROFILER               0,0  Add (Java) profiling support.
//
// ENABLE_WTK_PROFILER           0,0  Add WTK-compatible profiling support
//
// ENABLE_REMOTE_TRACER          0,0  Add remote tracing capabilities.
//
// ENABLE_SYSTEM_ROM_OVERRIDE    1,-  Allow a debug-mode VM to override
//                                    the system ROM image by loading system
//                                    classes from classes.zip.
//
// ENABLE_SYSTEM_ROM_HEADER_SKIPPING 1,1 Skip object headers in the system
//                                    ROM image to save space.
//
// ENABLE_ROM_DEBUG_SYMBOLS      1,0  Use extra symbols in ROMImage.cpp
//                                    to aid debugging (e.g., recover original
//                                    name of renamed classes).
//
// ENABLE_SEGMENTED_ROM_TEXT_BLOCK 0,0 Split the TEXT block of a source ROM
//                                    image into several segments.
//
// ENABLE_SOFT_FLOAT             0,0  Use the software floating point
//                                    operations.
//
// ENABLE_SEMAPHORE              1,1  Include com.sun.cldc.util.Semaphore class
//
// ENABLE_ROM_GENERATOR          1,0  Include code for generating 
//                                    (source or binary) ROM image.
//
// ENABLE_INTERPRETER_GENERATOR  1,-  Include code for generating interpreter
//                                    loop in non-PRODUCT build.
//
// ENABLE_STACK_TRACE            1,1  Include code for printing the stack trace
//                                    of Java Throwable objects.
//
// ENABLE_THUMB_LIBC_GLUE        0,0  Linux-only: Use glue code inside
//                                    the VM for invoking functions in
//                                    the GNU LIBC. Use this option if
//                                    the VM is built in THUMB mode
//                                    but your LIBC is not built with
//                                    interworking.
//
// ENABLE_THUMB_VM               0,0  Enable THUMB interworking in interpreter
//                                    and compiled Java methods, so that the
//                                    native code in the VM can be compiled
//                                    in THUMB mode.
//
// ENABLE_THUMB_COMPILER         0,0  Generate compiled Java methods
//                                    in THUMB mode
//
// ENABLE_THUMB_REGISTER_MAPPING 0,0  Generate ARM interpreter with fp and jsp
//                                    as low registers. Critical for THUMB
//                                    compiler.
//
// ENABLE_THUMB_GP_TABLE         0,0  Generate inverted GP table. Allows
//                                    for small GP register offsets to aid
//                                    the THUMB compiler.
//
// ENABLE_TIMER_THREAD           1,1  Use a thread to generate timer ticks
//                                    instead of a timer signal handler.
//
// ENABLE_TTY_TRACE              1,0  Enable the various TraceXXX flags
//
// ENABLE_TOS_CACHING            1,1  Jazelle only. Allow TOS caching.
//
// ENABLE_FULL_STACK             1,1  Jazelle only. Use Full Java Stack.
//
// ENABLE_VERBOSE_ASSERTION      1,0  Print detailed error messages
//                                    when a run-time assertion fails.
//
// ENABLE_VERIFY_ONLY            1,1  Add support for using the VM as a tool
//                                    for classpath verification
//                                    without any byte code execution.
//
// ENABLE_EMBEDDED_CALLINFO      0,0  Compiler-specific.
//                                    Embed call info records in compiled code
//                                    just after the call instruction. This is
//                                    used to debug the correctness of
//                                    ENABLE_APPENDED_CALLINFO.
//
// ENABLE_APPENDED_CALLINFO      1,1  Compiler-specific.
//                                    Append all call info records for
//                                    a method at the end of compiled
//                                    code of that method.
//
// ENABLE_COMPRESSED_VSF         0,0  Compiler-specific.
//                                    Include table of compressed VSF 
//                                    in the Relocation of CompiledMethod
//                                    to reduce the produced code size.
//
//
// ENABLE_INLINE_COMPILER_STUBS  1,1  Compiler-specific.
//                                    Generate inlined code for creating
//                                    new objects and type arrays
//                                    (instead of calling compiler_new_object
//                                    and compiler_new_type_array stubs).
//
//
// ENABLE_XSCALE_WMMX_INSTRUCTIONS      0,0 Use XScale WMMX instructions
//                                          in compiled code and interpreter.
//
// ENABLE_XSCALE_WMMX_TIMER_TICK        0,0 Use XScale WMMX registers to
//                                          check for timer ticks.
//
// ENABLE_XSCALE_WMMX_ARRAYCOPY         0,0 Use XScale WMMX to implement
//                                          System.arraycopy().
//
// ENABLE_REMEMBER_ARRAY_LENGTH         0,0 Remember the length of the last
//                                          accessed array in a register.
//
// ENABLE_LOOP_OPTIMIZATION             0,0 Simplify the code sequence at
//                                          end end of a loop.
//
// ENABLE_XSCALE_PMU_CYCLE_COUNTER      0,0 Use the PMU cycle counter on
//                                          Intel Xscale CPU for performance
//                                          measurement.
//
// ENABLE_NPCE                          0,0 Null-pointer check elimination.
//                                          This requires OS support for
//                                          exceptions when accessing address
//                                          0x0.
//
// ENABLE_INTERNAL_CODE_OPTIMIZER       0,0 Improved code optimizer for
//                                          scheduling ARM instructions.
//
// ENABLE_INLINE                        1,1 Inline simple methods into their
//                                          callers in compiled code.
//
// ENABLE_REMEMBER_ARRAY_CHECK          0,0 Remember the length 
//                                          checking result of a un-changed
//                                          local variable.
//
// ENABLE_STATIC_TRAMPOLINE             0,0 Use static trampoline in dispatch
//                                          to subroutine instead.
//
// ENABLE_TRAMPOLINE                    0,0 Use branch instruction to replace
//                                          mov pc, rc in static method
//                                          invoking. This is faster on
//                                          the Xscale which has branch
//                                          prediction.
//
// ENABLE_CSE                           0,0 Eliminate memory access related
//                                          common byte code  
//
// ENABLE_HARDWARE_TIMER_FOR_TICKS      0,0 Nucleus-XScale only.  Include code
//                                          to set up a hardware timer to
//                                          provide timer ticks to Nucleus.
//                                          Even when enabled the timer is
//                                          still controllable with generic
//                                          runtime flags. See
//                                          Globals_nucleus.hpp.
//
// ENABLE_OOP_TAG                       0,0 Support for debug int in oopdesc
//                                          used for MVM GC tracing
//
// ENABLE_MULTIPLE_PROFILES_SUPPORT     0,0 Add support for using multiple
//                                          profiles that may provide multually
//                                          exclusive APIs.  Allows the hiding
//                                          of certain classes under
//                                          a specific profile.
//
// ENABLE_JVMPI_PROFILE                 0,0 To support JVMPI profiler.
//
// ENABLE_JVMPI_PROFILE_VERIFY          0,0 To support JVMPI profiler
//                                          verification.
// 
// ENABLE_CODE_PATCHING                 0,0 Use code patching mechanism for
//                                          timer tick checking optimizations.
//
// ENABLE_CONDITIONAL_BRANCH_OPTIMIZATIONS 1,1 Use flags set by arithmetic
//                                          instructions
//============================================================================
// ENABLE_FLAGS_END }}
//============================================================================


#if !ENABLE_INTERPRETER_GENERATOR && !(ENABLE_ROM_GENERATOR && !ENABLE_MONET)
#if ENABLE_MINIMAL_ASSERT_BUILD
#undef  ENABLE_ROM_DEBUG_SYMBOLS
#define ENABLE_ROM_DEBUG_SYMBOLS     0
#undef  ENABLE_VERBOSE_ASSERTION
#define ENABLE_VERBOSE_ASSERTION     0
#undef  ENABLE_STACK_TRACE
#define ENABLE_STACK_TRACE           0
#undef  ENABLE_JAR_READER_EXPORTS
#define ENABLE_JAR_READER_EXPORTS    0
#undef  ENABLE_PROFILER
#define ENABLE_PROFILER              0
#undef  ENABLE_WTK_PROFILER
#define ENABLE_WTK_PROFILER          0
#undef  ENABLE_INTERPRETER_GENERATOR
#define ENABLE_INTERPRETER_GENERATOR 0
#undef  ENABLE_TTY_TRACE
#define ENABLE_TTY_TRACE             0
#undef  ENABLE_PERFORMANCE_COUNTERS
#define ENABLE_PERFORMANCE_COUNTERS  0
#undef  ENABLE_SYSTEM_ROM_OVERRIDE
#define ENABLE_SYSTEM_ROM_OVERRIDE   0
#undef  ENABLE_MONET_DEBUG_DUMP
#define ENABLE_MONET_DEBUG_DUMP      0

#define USE_ROM_LOGGING              0
#endif
#endif

#if ENABLE_JAZELLE

#undef ENABLE_INCREASING_JAVA_STACK
#undef ENABLE_CPU_VARIANT
#undef ENABLE_BYTECODE_FLUSHING
#undef ENABLE_DISPATCH_TABLE_ALIGNMENT
#undef ENABLE_DISPATCH_TABLE_PADDING
#undef ENABLE_JAVA_STACK_TAGS
#undef ENABLE_NATIVE_ORDER_REWRITING
#undef ENABLE_FULL_STACK
#undef ENABLE_TOS_CACHING

#if ENABLE_ROM_GENERATOR && !ENABLE_INTERPRETER_GENERATOR && CROSS_GENERATOR
    //
    // Note: The romizer uses ENABLE_INCREASING_JAVA_STACK=1 for
    // the interpreter, GC, etc. But JVM.cpp sets the
    // the variable CompilerJavaStackDirection=1 so that we generate
    // correct compiled Java methods to be executed on the Jazelle hardware.
    //
    // ENABLE_INTERPRETER_GENERATOR=0 is already set inside the makefiles for
    // ARM-based romgen.
    //
#define ENABLE_INCREASING_JAVA_STACK 0
#else
#define ENABLE_INCREASING_JAVA_STACK 1
#endif

    //
    // The following flags must be hard-coded to this set of values
    //
#define ENABLE_CPU_VARIANT                1
#define ENABLE_BYTECODE_FLUSHING          1
#define ENABLE_DISPATCH_TABLE_ALIGNMENT   1
#define ENABLE_DISPATCH_TABLE_PADDING     1
#define ENABLE_JAVA_STACK_TAGS            0
#define ENABLE_NATIVE_ORDER_REWRITING     0
#define ENABLE_FULL_STACK                 0
#define ENABLE_TOS_CACHING                0
#endif // ENABLE_JAZELLE


// USE_BINARY_IMAGE_GENERATOR         Include the generator for binary images
//                                    in the VM to support fast classloading.
//
// USE_SOURCE_IMAGE_GENERATOR         Include the generator for ROMImage.cpp
//                                    Currently USE_BINARY_IMAGE_GENERATOR and.
//                                    USE_SOURCE_IMAGE_GENERATOR are mutually
//                                    exclusive.
//
// USE_PRODUCT_BINARY_IMAGE_GENERATOR Build the binare image generator in
//                                    PRODUCT mode (smaller and faster).
//
// USE_AOT_COMPILATION                Add the ability to compile selected
//                                    methods during image generation
//
// USE_BINARY_IMAGE_LOADER            Include the binary image loader
//                                    in the VM for fast classloading.
//
// USE_UNRESOLVED_NAMES_IN_BINARY_IMAGE
//                                    Set this flag to 1 if you want to
//                                    support off-line application image
//                                    conversion.
//
// USE_ROM_LOGGING                    Write a log of actions taken by the
//                                    source/binary rom writers.
//
// USE_GENERIC_BIT_SETTING_FUNCS      Use C language version of bit-setting
//                                    functions in ObjectHeap.cpp. See
//                                    ObjectHeap_i386.hpp id you need to
//                                    provide an assembly version of these
//                                    functions.
//
// USE_JAR_ENTRY_ENUMERATOR           Add the ability to enumerate over
//                                    all entries in a JAR file (e.g., used by
//                                    the romizer and +TestCompiler)
//

#define USE_SOURCE_IMAGE_GENERATOR    ((!ENABLE_MONET) && ENABLE_ROM_GENERATOR)

#define USE_BINARY_IMAGE_GENERATOR    ENABLE_MONET
#define USE_BINARY_IMAGE_LOADER       ENABLE_MONET

#if defined(PRODUCT) && ENABLE_MONET
#  define USE_PRODUCT_BINARY_IMAGE_GENERATOR 1
#else
#  define USE_PRODUCT_BINARY_IMAGE_GENERATOR 0
#endif

#if USE_BINARY_IMAGE_GENERATOR || USE_BINARY_IMAGE_LOADER
#define USE_UNRESOLVED_NAMES_IN_BINARY_IMAGE 0
#else
#define USE_UNRESOLVED_NAMES_IN_BINARY_IMAGE 0
#endif

#if !defined(PRODUCT) || ENABLE_VERIFY_ONLY || \
     ENABLE_ROM_GENERATOR || ENABLE_PERFORMANCE_COUNTERS || \
     USE_PRODUCT_BINARY_IMAGE_GENERATOR
#  define USE_JAR_ENTRY_ENUMERATOR 1
#else
#  define USE_JAR_ENTRY_ENUMERATOR 0
#endif

// GenerateSystemROMImage is hard-wired: with MONET enabled, we always
// generate a Binary ROM, which is always an application image. Otherwise
// we always generate a Source ROM, which is always a system image.
#if ENABLE_MONET
#undef  ENABLE_ROM_GENERATOR
#define ENABLE_ROM_GENERATOR 1
#endif

#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK && USE_SOURCE_IMAGE_GENERATOR
#define USE_SEGMENTED_TEXT_BLOCK_WRITER 1
#else
#define USE_SEGMENTED_TEXT_BLOCK_WRITER 0
#endif

#if ENABLE_MONET && !ENABLE_ROM_GENERATOR
#error "ENABLE_ROM_GENERATOR must be enabled to support ENABLE_MONET"
#endif

#if ENABLE_MONET_COMPILATION && (!ENABLE_MONET || !ENABLE_COMPILER)
// ENABLE_MONET_COMPILATION makes no sense if Monet or compiler is not enabled
#undef ENABLE_MONET_COMPILATION
#define ENABLE_MONET_COMPILATION 0
#endif

// AOT compilation is supported only for ARM
#if ENABLE_COMPILER && defined(ARM) && \
     (USE_SOURCE_IMAGE_GENERATOR || ENABLE_MONET_COMPILATION)
#  define USE_AOT_COMPILATION 1
#else
#  define USE_AOT_COMPILATION 0
#endif

#if !ENABLE_APPENDED_CALLINFO && !ENABLE_EMBEDDED_CALLINFO
#error "Either ENABLE_APPENDED_CALLINFO or ENABLE_EMBEDDED_CALLINFO must be set"
#endif

#if ENABLE_TIMER_THREAD && !SUPPORTS_TIMER_THREAD
#error "TIMER_THREAD is not supported in this configuration"
#endif

#if ENABLE_COMPRESSED_VSF && \
    (ENABLE_THUMB_GP_TABLE || ENABLE_ARM_VFP || ENABLE_CSE || !defined(ARM))
#error NOT currently supported in this configuration
#endif

#if !ENABLE_COMPRESSED_VSF && ENABLE_PAGE_PROTECTION
#error NOT supported in this configuration
#endif

#if ENABLE_INLINE_COMPILER_STUBS && !ENABLE_ZERO_YOUNG_GENERATION
#error ENABLE_INLINE_COMPILER_STUBS requires ENABLE_ZERO_YOUNG_GENERATION
#endif

#if ENABLE_CODE_PATCHING && \
    (ENABLE_TIMER_THREAD || ENABLE_EMBEDDED_CALLINFO || !defined(ARM))
#error ENABLE_CODE_PATCHING is not supported in this configuration
#endif

#if !defined(ARM) && ENABLE_LOOP_OPTIMIZATION
#error "ENABLE_LOOP_OPTIMIZATION is supported only for ARM"
#endif

#if !ENABLE_TIMER_THREAD && !SUPPORTS_TIMER_INTERRUPT
#error "TIMER_INTERRUPT is not supported in this configuration"
#endif

//
// USE_SINGLE_METHOD_FLUSHING         During code patching use the single 
//                                    flush icache for the method instead of
//                                    several flushes for each instruction.
//
// USE_PATCHED_METHOD_CACHE           Use patched method cache.
//
#ifndef USE_SINGLE_METHOD_FLUSHING
#define USE_SINGLE_METHOD_FLUSHING 1
#endif

#ifndef USE_PATCHED_METHOD_CACHE
#define USE_PATCHED_METHOD_CACHE 1
#endif


#if ENABLE_INTERPRETATION_LOG && !ENABLE_COMPILER
// ENABLE_INTERPRETATION_LOG is useful with the compiler only
#undef  ENABLE_INTERPRETATION_LOG
#define ENABLE_INTERPRETATION_LOG 0
#endif

#if !ENABLE_COMPILER && ENABLE_CODE_OPTIMIZER
// ENABLE_CODE_OPTIMIZER makes no sense if compiler is not enabled
#undef  ENABLE_CODE_OPTIMIZER
#define ENABLE_CODE_OPTIMIZER 0
#endif

#if !ENABLE_CODE_OPTIMIZER && ENABLE_INTERNAL_CODE_OPTIMIZER
#undef ENABLE_INTERNAL_CODE_OPTIMIZER
#define ENABLE_INTERNAL_CODE_OPTIMIZER 0
#endif

#if ENABLE_INTERPRETER_GENERATOR && defined(PRODUCT)
#  error "ENABLE_INTERPRETER_GENERATOR" cannot be defined for PRODUCT build
#endif

#if ENABLE_ROM_JAVA_DEBUGGER
#if !ENABLE_JAVA_DEBUGGER
#error "ENABLE_JAVA_DEBUGGER must be true for ENABLE_ROM_JAVA_DEBUGGER build"
#endif
#endif

#if ENABLE_SYSTEM_CLASSES_DEBUG
#if !ENABLE_ROM_JAVA_DEBUGGER
#error "ENABLE_ROM_JAVA_DEBUGGER must be true for ENABLE_SYSTEM_CLASSES_DEBUG build"
#endif
#endif

// If we have turned on java debugger support and we are building a monet
// image then check for  ROM java debugger support so that binary images
// are completely supported for debugging.  Specifically this turns on
// handling of line number and variable tables
#if ENABLE_JAVA_DEBUGGER && ENABLE_MONET
#if  !ENABLE_ROM_JAVA_DEBUGGER
#error "ENABLE_ROM_JAVA_DEBUGGER must be set if building Binary ROMizer with debugger support"
#endif
#endif

#ifndef USE_ROM_LOGGING 
#  if ENABLE_ROM_GENERATOR && !defined(PRODUCT)
#    define USE_ROM_LOGGING 1
#  else
#    define USE_ROM_LOGGING 0
#  endif
#endif

#ifndef USE_GENERIC_BIT_SETTING_FUNCS
#define USE_GENERIC_BIT_SETTING_FUNCS 1
#endif

// IMPL_NOTE: change this to ENABLE_ROM_IMAGE
//
// ROMIZING               This flag is set to true if we're running
//                        in a VM that has romized system classes.

/*
 * Optional functionality (that may not be fully implemented):
 * (These flags need to be set manually by modifying source code or
 * build instructions)
 */

// USE_DEFAULT_TTY_STREAM             Use the default TTY stream implementation
//                                    in OS.cpp, which assumes availability
//                                    of static C++ constructors.
#ifndef USE_DEFAULT_TTY_STREAM
#define USE_DEFAULT_TTY_STREAM 1
#endif

// USE_BSD_SOCKET                     Implement network I/O using BSD sockets

// USE_CLASS_LOADER                   Implement class loader functionality.
//                                    This feature is not implemented.

#ifndef USE_BSD_SOCKET
#define USE_BSD_SOCKET 0
#endif

// Some system header files defines ARM to be blank, which doesn't
// work with the way we use macros (e.g., #if ARM && LINUX). So
// let's change it to something more pleasant.
#ifdef ARM
#  undef ARM
#  define ARM 1
#endif

#if defined(ARM) && !ENABLE_INTERPRETER_GENERATOR && !CROSS_GENERATOR
#  define ARM_EXECUTABLE 1
#else
#  define ARM_EXECUTABLE 0
#endif

//
// USE_LITERAL_POOL                   Compiled code needs a literal pool
//
// USE_COMPILER_FPU_MAP               If true, the VirtualStackFrame class
//                                    include extra information for FPU
//                                    registers.
//
// USE_COMPILER_LITERALS_MAP          If true, the VirtualStackFrame class
//                                    include extra information for literals.

#if ENABLE_COMPILER && ( defined(ARM) || defined(HITACHI_SH) )
#  define USE_LITERAL_POOL 1
#else
#  define USE_LITERAL_POOL 0
#endif
  

#define USE_COMPILER_FPU_MAP     !USE_LITERAL_POOL
#define USE_COMPILER_LITERALS_MAP USE_LITERAL_POOL

#if ENABLE_COMPILER || ENABLE_ROM_GENERATOR || ENABLE_INTERPRETER_GENERATOR
#  define USE_COMPILER_STRUCTURES 1
#else
#  define USE_COMPILER_STRUCTURES 0
#endif

#if ENABLE_COMPILER && defined(ARM) && !ENABLE_EMBEDDED_CALLINFO
#  define USE_COMPILER_GLUE_CODE 1
#else
#  define USE_COMPILER_GLUE_CODE 0
#endif

// USE_UNICODE_FOR_FILENAMES          Use 16-bit unicode chars for filenames
//                                    instead of ASCII chars

// MAX_PRINTF_BUFFER_SIZE             Size of stack-allocated buffer
//                                    used by tty->print() should never
//                                    exceed MAX_PRINTF_BUFFER_SIZE (in bytes).
//                                    Note: in PRODUCT mode it may be smaller
//                                    than MAX_PRINTF_BUFFER_SIZE.

#ifndef MAX_PRINTF_BUFFER_SIZE
#define MAX_PRINTF_BUFFER_SIZE 4096
#endif

// MAX_METHOD_TO_COMPILE              Default size for the MaxMethodToCompile
//                                    run-time option. The system default
//                                    is 6000 bytes. Platforms can override 
//                                    this in Globals_<os_family>.hpp or in
//                                    Makefiles.
#ifndef MAX_METHOD_TO_COMPILE
#define MAX_METHOD_TO_COMPILE 6000
#endif

/*
 * Bracing comments that contain parseable source code:
 * (Do not use '#if 0' or similar constructs instead - we will zap that code!)
 */

// NOT_CURRENTLY_USED                 Code that illustrates what may get
//                                    added in the  near future

// NO_INLINE_ASSEMBLER                Do not use in-line assembler in source
//                                    code. Use straight C/C++ code instead.

#define NOT_CURRENTLY_USED 0

/*
 * The SUPPORTS_ flags are defined in Globals_<os_family>.hpp to
 * declare whether a feature is supported on the given platform.
 */

// SUPPORTS_ADJUSTABLE_MEMORY_CHUNK   Does this OS port support adjustable
//                                    memory chunks for implementing
//                                    the Java heap? (see OsMemory.hpp)
//
// SUPPORTS_CUSTOM_HEAP_ADJUSTMENT    Does this OS port provide a customed
//                                    set of routines to implement the heap
//                                    expansion policy? This flag is ignored
//                                    for SUPPORTS_ADJUSTABLE_MEMORY_CHUNK=0.
//                                    (see OsMemory.hpp)
//
// SUPPORTS_MEMORY_MAPPED_FILES       Does this OS port
//                                    allow mapping files into a fixed
//                                    memory space?
//
// SUPPORTS_TIMER_THREAD              Does this OS port support clock ticks
//                                    implemented by a timer thread?
//
// SUPPORTS_TIMER_INTERRUPT           Does this OS port support clock ticks
//                                    implemented by a timer interrupt?
//
// SUPPORTS_PROFILER_CONTROL          Is the Os::profiler_control() API
//                                    implemented?
//
// HOST_LITTLE_ENDIAN                 Is the development host a little-endian
//                                    architecture?

#ifndef SUPPORTS_ADJUSTABLE_MEMORY_CHUNK
#define SUPPORTS_ADJUSTABLE_MEMORY_CHUNK 0
#endif

#ifndef SUPPORTS_CUSTOM_HEAP_ADJUSTMENT
#define SUPPORTS_CUSTOM_HEAP_ADJUSTMENT 0
#endif

#ifndef SUPPORTS_PROFILER_CONTROL 
#define SUPPORTS_PROFILER_CONTROL 0
#endif

#ifndef SUPPORTS_MEMORY_MAPPED_FILES
#define SUPPORTS_MEMORY_MAPPED_FILES 0
#endif


#ifndef HOST_LITTLE_ENDIAN
// This should have be set in makefiles, but need to set a default value 
// for win32_i386_ide build.
#  define HOST_LITTLE_ENDIAN 1
#endif

// USE_SET_HEAP_LIMIT                 Set this to 0 in Globals_<os>.hpp
//                                    if your platform does not need
//                                    the JVM_SetHeapLimit API to
//                                    implement a user-administered
//                                    space in the Java heap.
#ifndef USE_SET_HEAP_LIMIT 
#  define USE_SET_HEAP_LIMIT 1
#endif

// ISOLATES_PARAM                     A function parameter that needs to be
//                                    declared or passed only if
//                                    ENABLE_ISOLATES is true.

#if ENABLE_ISOLATES
#  define ISOLATES_PARAM(code) code,
#else
#  define ISOLATES_PARAM(code)
#endif

// These are the 3 choices for loading Monet images. Note that we never
// call malloc() to allocate memory for storing the images, so as to avoid
// fragmenting the malloc heap.
//
// USE_IMAGE_MAPPING                  Load the Monet image(s) using an
//                                    OS-specific file mapping API (such as
//                                    mmap() on Linux). This is the most
//                                    efficient method of loading images, 
//                                    since it potentially supports 
//                                    direct-mapping of Flash to reduce 
//                                    RAM consumption.
//
// USE_IMAGE_PRELOADING               Load the Monet image(s) at the beginning
//                                    of the ObjectHeap. This option is useful
//                                    when running in SVM mode without
//                                    USE_IMAGE_MAPPING support.
//
// USE_LARGE_OBJECT_AREA              In MVM mode, if the OS does not support
//                                    USE_IMAGE_MAPPING, Monet images are
//                                    loaded into large, floating areas
//                                    within the ObjectHeap.

#if !USE_BINARY_IMAGE_LOADER
#  define USE_IMAGE_MAPPING     0
#  define USE_IMAGE_PRELOADING  0
#  define USE_LARGE_OBJECT_AREA 0
#elif (SUPPORTS_MEMORY_MAPPED_FILES && ENABLE_MEMORY_MAPPED_FILES)
#  define USE_IMAGE_MAPPING     1
#  define USE_IMAGE_PRELOADING  0
#  define USE_LARGE_OBJECT_AREA 0
#elif !ENABLE_ISOLATES
#  define USE_IMAGE_MAPPING     0
#  define USE_IMAGE_PRELOADING  1
#  define USE_LARGE_OBJECT_AREA 0
#else
#  define USE_IMAGE_MAPPING     0
#  define USE_IMAGE_PRELOADING  0
#  define USE_LARGE_OBJECT_AREA 1
#endif

#if USE_BINARY_IMAGE_LOADER
#  if (USE_IMAGE_MAPPING + USE_IMAGE_PRELOADING + USE_LARGE_OBJECT_AREA) != 1
#    error Only one of bundle allocation methods must be enabled
#  endif
#endif

// USE_DEBUG_PRINTING                 Include code to print various internal
//                                    data structures and symbolic definitions
//                                    in the VM. This feature can be turned off
//                                    if you want to build a small debug-mode
//                                    VM and you're only interested in the
//                                    run-time assertions.
//
// USE_COMPILER_COMMENTS              Generate compiler comments inside
//                                    disassembled CompiledMethods.
//
// USE_COMPILER_DISASSEMBLER          Generate disassembled listing of
//                                    CompiledMethods.
//
// USE_OOP_VISITOR                    Enable the OopVisitor feature (currently
//                                    used only for USE_DEBUG_PRINTING
//
// USE_VERBOSE_ERROR_MSG              Use a descriptive character string
//                                    in errors thrown by the VM

#if ENABLE_INTERPRETER_GENERATOR || USE_SOURCE_IMAGE_GENERATOR 
// The loopgen and source romgen always need the debug printing code.
#  define USE_DEBUG_PRINTING        1
#  define USE_COMPILER_COMMENTS     ENABLE_COMPILER
#  define USE_COMPILER_DISASSEMBLER ENABLE_COMPILER
#  define USE_OOP_VISITOR           1
#else
#  define USE_DEBUG_PRINTING        (ENABLE_TTY_TRACE && !defined(PRODUCT))
#  define USE_COMPILER_COMMENTS     (ENABLE_COMPILER && ENABLE_TTY_TRACE)
#  define USE_COMPILER_DISASSEMBLER (ENABLE_COMPILER && ENABLE_TTY_TRACE)
#  define USE_OOP_VISITOR           USE_DEBUG_PRINTING
#endif

#if defined(PRODUCT) || (!ENABLE_TTY_TRACE)
#  define USE_VERBOSE_ERROR_MSG 0
#else
#  define USE_VERBOSE_ERROR_MSG 1
#endif

/*
 * Flags that still need to be documented or deleted, i.e. IMPL_NOTE:
 */

// INFLATE_DEBUG_FILE

// INFLATER_COPY


// Leaf method frame omission is implemented only on ARM compiler.
//
#if ARM && !(ENABLE_THUMB_COMPILER||ENABLE_THUMB_REGISTER_MAPPING)
#  define OMIT_LEAF_FRAME_DEFAULT true
#else
#  define OMIT_LEAF_FRAME_DEFAULT false
#endif

// By default, do not generate compiler assertions in AOT (since AOT romgen
// is built in DEBUG mode by default.)
//
#if defined(AZZERT) && !USE_SOURCE_IMAGE_GENERATOR
#  define COMPILER_ASSERTION_DEFAULT 1
#else
#  define COMPILER_ASSERTION_DEFAULT 0
#endif


// USE_INDIRECT_EXECUTION_SENSOR_UPDATE
//                                    Do not embed the execution sensor
//                                    update code inside the compiled
//                                    method prolog. Instead, call a subroutine
//                                    to update the sensor. During the update,
//                                    Method::execution_entry() is modified to
//                                    avoid unnecessary repeated updates.
//                                    The execution entry is restored only
//                                    after a timer tick.

#if (ENABLE_THUMB_COMPILER||ENABLE_THUMB_REGISTER_MAPPING) && !ENABLE_ARM_V6T2
#define USE_INDIRECT_EXECUTION_SENSOR_UPDATE 1
#else
#define USE_INDIRECT_EXECUTION_SENSOR_UPDATE 0
#endif

#if ENABLE_THUMB_COMPILER
#define USE_OPT_FORWARD_BRANCH 0
#else
#define USE_OPT_FORWARD_BRANCH 1
#endif

//----------------------------------------------------------------------------
// Macros for PRODUCT build

// PRODUCT_CONST  -- flags things that are "const"  in PRODUCT build
// PRODUCT_STATIC -- flags things that are "static" in PRODUCT build
#ifdef PRODUCT
#define PRODUCT_CONST  const
#define PRODUCT_STATIC static
#define PRODUCT_INLINE inline
#else
#define PRODUCT_CONST
#define PRODUCT_STATIC
#define PRODUCT_INLINE
#endif

// ROMIZED_PRODUCT -- true if we are building Romized, PRODUCE build, in
//                    which case a lot of code can be conditionally compiled
//                    out.
#if defined(ROMIZING) && (defined(PRODUCT) || !ENABLE_SYSTEM_ROM_OVERRIDE)
#  define ROMIZED_PRODUCT 1
#else
#  define ROMIZED_PRODUCT 0
#endif

#if ENABLE_MEMORY_PROFILER && !ENABLE_JAVA_DEBUGGER
#  error "ENABLE_JAVA_DEBUGGER must be true for ENABLE_MEMORY_PROFILER build"
#endif

#if ENABLE_INLINED_ARRAYCOPY && !ENABLE_COMPILER_TYPE_INFO
#  error "ENABLE_COMPILER_TYPE_INFO must be set for ENABLE_INLINED_ARRAYCOPY"
#endif

//
// USE_FP_RESULT_IN_VFP_REGISTER      Return floating point retult in VFP 
//                                    registers. 
//
#define USE_FP_RESULT_IN_VFP_REGISTER (ENABLE_ARM_VFP)

//
// ENABLE_ARM9_VFP_BUG_WORKAROUND  Workaround for ARM9 + VFP hardware bugs
//
//
#if ENABLE_ARM_VFP
#  ifndef ENABLE_ARM9_VFP_BUG_WORKAROUND
#    define ENABLE_ARM9_VFP_BUG_WORKAROUND 0
#  endif
#endif

//
// USE_EMBEDDED_VTABLE_BITMAP      Embed a vtable bitmap in JavaClassDesc,
//                                 one bit per virtual method to mark
//                                 overridden methods
//
#define USE_EMBEDDED_VTABLE_BITMAP (ENABLE_COMPILER && ENABLE_INLINE)

//
// USE_EVENT_LOGGER                Enable JVM event logging
//
#if ENABLE_EVENT_LOGGER || ENABLE_EXTENDED_EVENT_LOGGER ||\
    (ENABLE_PERFORMANCE_COUNTERS && USE_DEBUG_PRINTING)
#  define USE_EVENT_LOGGER 1
#else
#  define USE_EVENT_LOGGER 0
#endif

//
// USE_HIGH_RESOLUTION_TIMER       Implement access to OS timer
//
#define USE_HIGH_RESOLUTION_TIMER (ENABLE_PERFORMANCE_COUNTERS ||\
  ENABLE_PROFILER || ENABLE_WTK_PROFILER || ENABLE_TTY_TRACE ||\
  USE_EVENT_LOGGER)

#define ENABLE_VSF_MERGE_TEST  0

#ifndef ARM
#  define USE_EVENT_LOG_TIMER_DOWNSAMPLING  1
#else
#  define USE_EVENT_LOG_TIMER_DOWNSAMPLING  0
#endif
