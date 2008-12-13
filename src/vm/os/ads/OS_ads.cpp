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

/*
 * OS_ads.cpp: ARM ADS implementation of the VM
 *               operating system porting interface
 *
 * This file defines the ADS-specific implementation
 * of the OS porting interface (class OS).  Refer to file
 * "/src/vm/share/runtime/OS.hpp" and the Porting
 * Guide for details.
 */

#include "incls/_precompiled.incl"
#include "incls/_OS_ads.cpp.incl"

// fixed numbers of semihosting API, see ADS Debug Target Guide, 5-21
#define SYS_CLOCK    0x10
#define SYS_TIME     0x11
#define SYS_ELAPSED  0x30
#define SYS_TICKFREQ 0x31

static bool             _compiler_timer_has_ticked = false;
static jlong            _compiler_timer_start;

jlong Os::_java_time_millis_offset = -1;

int __swi(0x123456) semi_host_swi(int arg1);
int __swi(0x123456) semi_host_swi(int arg1, int arg2);

#if ENABLE_DYNAMIC_NATIVE_METHODS
void* Os::loadLibrary(const char* /*libName*/) {
  return 0; //Library loading not supported for this OS
}
void* Os::getSymbol(void* /*handle*/, const char* /*name*/) {
  return 0; //Library loading not supported for this OS
}
#endif

jlong Os::java_time_millis() {
  // SYS_CLOCK returns centiseconds from boot
  int rv;
  if (_java_time_millis_offset == -1) {
    rv = semi_host_swi(SYS_TIME);
    _java_time_millis_offset = (jlong)rv * (jlong)1000;

    rv = semi_host_swi(SYS_CLOCK, 0);
    _java_time_millis_offset -= ((jlong)(rv)) * ((jlong)10);
  }

  rv = semi_host_swi(SYS_CLOCK, 0);
  return ((jlong)rv*10 + _java_time_millis_offset);
}

void Os::sleep(jlong ms) {
  // we do evil busy waiting here
  jlong next_ms = Os::java_time_millis() + ms;
  while (next_ms > Os::java_time_millis()) {
    // do nothing
  }
}

static volatile bool ticker_running = false;

/*
 * Note:
 *
 * Ticker implementation using the ARM Reference Peripheral
 * Specification (RPS). This ticker implementation assumes
 * we control the hardware and the only interrupt source is
 * timer 1 which should be true as long as there is no OS
 * involved and all other I/O is done via SWI/semihosting.
 */

#define IRQVEC (unsigned *) 0x18

/*
 * IRQ handler. Only timer 1 interrupt is processed.
 */
void __irq irq_handler(void) {

  unsigned status;

  status = *IRQStatus;

  if ((ticker_running) && (status & IRQTimer1)) {
    *Timer1Clear = 0;  // clear the interrupt
    if (EnableTicks) {
      real_time_tick(TickInterval);
      _compiler_timer_has_ticked = true;
    }
  } else {
    TTY_TRACE_CR(("irq_handler: unknown interrupt!"));
  }
}

/*
 * Updates content of vector to contain branch instruction
 * to reach to routine. Returns previous vector.
 *
 * NOTE: routine must be within 32 MB range of vector.
 */
unsigned install_handler(unsigned routine, unsigned *vector) {

  unsigned vec, oldvec;

  // Turn vec into branch target value
  vec = ((routine - (unsigned)vector - 0x8) >> 2);
  // Check range limitation
  if (vec & 0xFF000000) {
    return 0;
  }
  // Set top byte to branch instruction
  vec = 0xEA000000 | vec;
  // Swap vectors
  oldvec = *vector;
  *vector = vec;

  OsMisc_flush_icache((address)vector, 4);

  return oldvec;
}

/*
 * Enable interrupts by updating bit 7 in CPSR. Must be
 * executed in priviledged mode.
 */
static __inline void enable_irq(void) {
  int tmp;
  __asm {
    MRS tmp, CPSR
    BIC tmp, tmp, #0x80
    MSR CPSR_c, tmp
  }
}

#if NOT_CURRENTLY_USED
/*
 * Disable interrupts by updating bit 7 in CPSR. Must be
 * executed in priviledged mode.
 */
static __inline void disable_irq(void) {
  int tmp;
  __asm {
    MRS tmp, CPSR
    ORR tmp, tmp, #0x80
    MSR CPSR_c, tmp
  }
}
#endif

void ads_panic() {
   __asm { SWI 0x9f0001 }
}

bool Os::start_ticks() {

  if (ticker_running) {
    return true;
  }

  ticker_running = true;
  install_handler((unsigned)irq_handler, IRQVEC);

  /*
   * The tick is fired for every (COUNTER_LOAD * 16) cycles. So if
   * we want to have one tick per 10 ms on a 16MHZ machine, it's
   *         (16M * (10 / 1000)) / 256
   *
   * For more info, see ADSv1_2/ARMulate/armulext/timer.c in the ADS
   * installation. Note that we must use TimerPrescale256. The other
   * two pre-scale values would cause the ARMUlator to overflow clock
   * counters for large TickInterval (> ~64)
   */
  jint COUNTER_LOAD;

  COUNTER_LOAD = CPUSpeed / 1000;
  COUNTER_LOAD *= TickInterval;
  COUNTER_LOAD /= 256; // prescale by 256

  *IRQEnableClear  =~0;        // disable all interrupts
  *Timer1Control = 0;          // disable timer
  *Timer1Clear = 0;            // clear timer interrupts
  *Timer1Load = COUNTER_LOAD;  // load value
  *Timer1Control = (TimerEnable | TimerPeriodic | TimerPrescale256);
  *IRQEnableSet = IRQTimer1;   // enable timer interrupts

  enable_irq();  // enable interrupts
  return true;
}

void Os::suspend_ticks() {
  if (ticker_running) {
    *IRQEnableClear =~0;    // disable all interrupts
    *Timer1Control = 0;     // disable timer
    *Timer1Clear = 0;       // clear timer interrupts
    ticker_running = false;
  }
}

void Os::resume_ticks() {
  start_ticks();
}

void Os::stop_ticks() {
  suspend_ticks();
}

/*
 * This is an example implementation of compiler timer. We try to base
 * the compiler timer on real_time_tick, as much as possible. This
 * way, we can avoid the overhead of Os::java_time_millis(), which
 * might be significant on actual devices.
 *
 * On an actual device, if MaxCompilationTime is a fixed value, it may
 * be better to use a dedicated OS timer resource to implement the
 * compiler timer. Alternatively, you can make TickInterval a multiple
 * of MaxCompilationTime, so that you can use the same OS timer
 * resource to serve both real_time_tick and the compiler timer.
 */

void Os::start_compiler_timer() {
  if (MaxCompilationTime == TickInterval) {
    // Note: this tend to make the average compilation period to be
    // 0.5 * MaxCompilationTime.
    _compiler_timer_start = (jlong)0;
    _compiler_timer_has_ticked = false;
  } else {
    _compiler_timer_start = Os::java_time_millis();
  }
}

/*
 * Returns true iff the current compilation has taken too long and
 * should be suspended and resumed later.
 */
bool Os::check_compiler_timer() {
  if (_compiler_timer_start == (jlong)0) {
    return _compiler_timer_has_ticked;
  } else {
    jint elapsed_ms = (jint)(Os::java_time_millis() - _compiler_timer_start);
    return (elapsed_ms >= MaxCompilationTime);
  }
}

#if ENABLE_PERFORMANCE_COUNTERS && defined(ARM)
// The following symbols are used by ADS_MEMPROF to determine the internal
// structure of the VM. Thet are not needed when you are running inside
// on a real target port.
#define DECLARE_MEMPROF_CONSTANT(c, o) \
    int MEMPROF_ ## c ## _ ## o;

#define INIT_MEMPROF_CONSTANT(c, o) \
    MEMPROF_ ## c ## _ ## o = c :: o();

#define FORALL_MEMPROF_CONSTANTS(template) \
    template(ClassInfo,      name_offset) \
    template(ConstantPool,   base_offset) \
    template(EntryFrame,     fake_return_address_offset) \
    template(EntryFrame,     real_return_address_offset) \
    template(EntryFrame,     stored_last_fp_offset) \
    template(JavaClass,      class_info_offset) \
    template(JavaFrame,      caller_fp_offset) \
    template(JavaFrame,      method_offset) \
    template(JavaFrame,      return_address_offset) \
    template(Method,         constants_offset) \
    template(Method,         access_flags_offset) \
    template(Method,         holder_id_offset) \
    template(Method,         name_index_offset) \
    template(Method,         native_code_offset) \
    template(Method,         signature_index_offset) \
    template(ExecutionStack, length_offset) \
    template(ExecutionStack, thread_offset) \
    template(Symbol,         length_offset) \
    template(Symbol,         base_offset) \
    template(Thread,         execution_stack_offset )

int MEMPROF_Java_fp_reg;
int MEMPROF_Java_jsp_reg;
int MEMPROF_Java_stack_direction;

FORALL_MEMPROF_CONSTANTS(DECLARE_MEMPROF_CONSTANT)

static void init_memprof_info() {
  FORALL_MEMPROF_CONSTANTS(INIT_MEMPROF_CONSTANT)

  MEMPROF_Java_fp_reg = Assembler::fp;
  MEMPROF_Java_jsp_reg = Assembler::jsp;
  MEMPROF_Java_stack_direction = JavaStackDirection;
}

#else
inline void init_memprof_info() {}
#endif

void Os::initialize() {
  *PROFILER_ATTACHED_REG = 0;
  if (*PROFILER_ATTACHED_REG == 1) {
    /*
     * We're attached to ADS_MEMPROF. Let's get the clock speed and
     * cache information from the profiler
     */
    CPUSpeed = *PROFILER_CPU_SPEED_REG;
    FlushCache = (bool)(*PROFILER_CACHE_EXISTS_REG);
    if (Verbose) {
        TTY_TRACE_CR(("CPU speed = %d", CPUSpeed));
        TTY_TRACE_CR(("CACHE EXISTS = %s", (FlushCache ? "yes" : "no")));
    }
  }

  if (TickInterval == 0) {
    // For TickInterval==0, we automatically adjust TickInterval and
    // InterpretationLogSize according to the CPU speed.
    //
    // Notes to ports: if you're porting to a new device, you may need
    // to fine tune the values of TickInterval and
    // InterpretationLogSize according to your device specifics.

    if (CPUSpeed < 40 * 1000 * 1000) {
      TickInterval = 50; // 50 ms per timer tick, don't compile too much
      InterpretationLogSize = 0;
    } else if (CPUSpeed < 100 * 1000 * 1000) {
      TickInterval = 30; // 30 ms per timer tick
      InterpretationLogSize = (INTERP_LOG_SIZE > 4) ? 4 : INTERP_LOG_SIZE;
    } else {
      TickInterval = 10; // 10 ms per timer tick
      InterpretationLogSize = INTERP_LOG_SIZE;
    }
  }

  // Initialize special information for supporting MEMPROF. It's an no-op
  // for real target builds.
  init_memprof_info();
}

/*
 * The Os::dispose method needs to correctly clean-up
 * all threads and other OS related activity to allow
 * for a clean and complete restart.  This should undo
 * all the work that initialize does.
 */
void Os::dispose() {
}

#if USE_HIGH_RESOLUTION_TIMER
jlong Os::elapsed_counter() {
  *PROFILER_ATTACHED_REG = 0;
  if (*PROFILER_ATTACHED_REG == 1) {
    // If we can read a value of 1 from PROFILER_ATTACHED_REG after
    // writing a 0 there, we know the profiler is attached.

    jint msw = *PROFILER_CYCLES64_MSW_REG;
    jint lsw = *PROFILER_CYCLES64_LSW_REG;
    return jlong_from_msw_lsw(msw, lsw);
  } else {
    jlong rv;
    int addr = (int)&rv;
    semi_host_swi(SYS_ELAPSED, addr);
    return rv;
  }
}

static jlong freq = 0;

jlong Os::elapsed_frequency() {
  *PROFILER_ATTACHED_REG = 0;
  if (*PROFILER_ATTACHED_REG == 1) {
    return (jlong)(*PROFILER_FREQUENCY_REG);
  } else {
    if (freq == 0) {
      int rv = semi_host_swi(SYS_TICKFREQ);
      // TICKFREQ returns megahertzes
      freq = rv * ((jlong)1024) * ((jlong)1024);
    }
    return freq;  // milliseconds
  }
}

void send_name_profiler(char *name, address a, int size) {
  int len = strlen(name);
  Os::suspend_profiler();
  *PROFILER_BEGIN_SEND_NAME_REG = len;
  for (int j = 0; j < len; j++) {
    *PROFILER_BEGIN_SEND_NAME_REG = name[j];
  }
  *PROFILER_BEGIN_SEND_NAME_REG = (unsigned int)a;
  *PROFILER_BEGIN_SEND_NAME_REG = size;
  *PROFILER_END_SEND_NAME_REG = 1;
  Os::resume_profiler();
}

#endif // ENABLE_PERFORMANCE_COUNTERS

void Os::suspend_profiler() {
  *PROFILER_SUSPEND_REG = 1;
}

void Os::resume_profiler() {
  *PROFILER_RESUME_REG = 1;
}

extern "C" {

struct __initial_stackheap {
    unsigned int heap_base;
    unsigned stack_base;
    unsigned heap_limit;
    unsigned stack_limit;
};

/*
 * __user_initial_stackheap() -- this function is called by the
 * start-up code in the ADS C library. This function makes it possible
 * to link cldc_vm.axf with a scatter map. Otherwise, if you use a
 * scatter map but do not define this function, the default
 * __user_initial_stackheap() in the LIBC of ADS would calculate the
 * heap starting address incorrectly, and overwrite RW global
 * variables.
 *
 * Note: we hard-code the location of our heap. Please see ./kvm.load
 * for the memory map of the KVM. This is potentially dangerous, so you
 * must make sure that your kvm.load file does not place anything
 * into the 0x0100:0000 - 0x03ff:ffff range.
 */
__value_in_regs struct __initial_stackheap
__user_initial_stackheap(unsigned /*R0*/, unsigned /*SP*/,
                         unsigned /*R2*/, unsigned /*SL*/)
{
    // This would create a malloc heap with about 48MB capacity.
    struct __initial_stackheap x = {
        (unsigned int)0x02000000,
        (unsigned int)0x03fffff0,
        0,
        0
    };

    return x;
}

void arm_flush_icache(address start, int size);

void OsMisc_flush_icache(address start, int size) {
  if (FlushCache) {
    arm_flush_icache(start, size);
  }
}

/*
 * Make it much larger than the data cache size
 */
static char mem_simulate_buffer1[1 * 1024 * 1024];
static juint mem_buffer_cursor;

/**
 * armulator_simulate_memory_transfer()
 * armulator_simulate_memory_change()

 * These two functions are used to simulate the timing of memory
 * transfer inside file, I/O and graphics operations. Many of these
 * operation are not emulated at the ARM instruction level (e.g., file
 * I/O is re-routed to host OS), without these functions these
 * operations would appear as disproportionally fast.
 *
 * size:   the amount of memory to be transferred/changed (bytes).
 * repeat: number of times the operation should be repeated.
 */
void armulator_simulate_memory_transfer(juint size, int repeat) {
  while (size > sizeof(mem_simulate_buffer1)) {
    size /= 2;
    repeat *= 2;
  }
  for (int i=0; i<repeat; i++) {
    if ((mem_buffer_cursor + size) >= sizeof(mem_simulate_buffer1)) {
      mem_buffer_cursor = 0;
    }
    jvm_memcpy(mem_simulate_buffer1 + mem_buffer_cursor,
               mem_simulate_buffer1 + mem_buffer_cursor, size);
    mem_buffer_cursor += size;
  }
}

void armulator_simulate_memory_change(juint size, int repeat) {
  while (size > sizeof(mem_simulate_buffer1)) {
    size /= 2;
    repeat *= 2;
  }
  for (int i=0; i<repeat; i++) {
    if ((mem_buffer_cursor + size) >= sizeof(mem_simulate_buffer1)) {
      mem_buffer_cursor = 0;
    }
    jvm_memset(mem_simulate_buffer1 + mem_buffer_cursor, i, size);
    mem_buffer_cursor += size;
  }
}

#undef ALSO_LR

} // extern "C"
