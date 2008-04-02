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

/*
 * OS_linux.cpp: Linux implementation of the VM
 *               operating system porting interface
 *
 * This file defines the Linux-specific implementation
 * of the OS porting interface (class Os).  Refer to file
 * "/src/vm/share/runtime/OS.hpp" and the Porting
 * Guide for details.
 */

#include "incls/_precompiled.incl"
#include "incls/_OS_linux.cpp.incl"

// this flag allows running with Valgrind, advanced memory checker for
// x86 Linux see http://developer.kde.org/~sewardj/ for more details
#define ENABLE_VALGRIND 0

#if ENABLE_VALGRIND
#include <valgrind/memcheck.h>
#endif

// several meta defines
#define NEED_CLOCK_TICKS USE_HIGH_RESOLUTION_TIMER


#if defined(__i386) || defined(ARM_EXECUTABLE)
#define KNOW_REGISTER_NAMES 1
#endif

#ifdef SOLARIS
#include <sys/time.h> /* for gethrtime() */
#endif

#if ENABLE_DYNAMIC_NATIVE_METHODS || ENABLE_JVMPI_PROFILE
#include <dlfcn.h>
#endif

// IMPL_NOTE: the offset of PC value in ucontext_t structure
// is different for every particular platform and OS.
// Please change the value as suitable for your OS.

#if ARM_EXECUTABLE
class CPUContext {
private:
  address _unused[8];
  address _regs[16];

public:
  address* get_regs()         { return _regs;     }
  address  get_pc()           { return _regs[15]; }
  void     set_pc(address pc) { _regs[15] = pc;   }
};

#elif defined(__i386)

class CPUContext {
private:
  address _regs[20];

public:
  address* get_regs()         { return _regs;     }
  address  get_pc()           { return _regs[19]; }
  void     set_pc(address pc) { _regs[19] = pc;   }
};

#else // all other architectures

class CPUContext {
public:
  address* get_regs()         { UNIMPLEMENTED(); return NULL; }
  address  get_pc()           { UNIMPLEMENTED(); return NULL; }
  void     set_pc(address pc) { UNIMPLEMENTED();              }
};
#endif


#if USE_LIBC_GLUE
// will be defined in interpreter loop as stub that can work from both
// Thumb and ARM
extern "C" void handle_vtalrm_signal_stub(int no, siginfo_t* inf, void* uc);
#else
#define handle_vtalrm_signal_stub handle_vtalrm_signal
#endif
extern "C" void init_jvm_chunk_manager();

#if ENABLE_DYNAMIC_NATIVE_METHODS || ENABLE_JVMPI_PROFILE
#define MAX_PATH 256
void* Os::loadLibrary(const char* libName) {
  char full_lib_name[MAX_PATH];
  char lib_ext[] = {'.','s','o','\0'};
  if (strlen(libName) + strlen(lib_ext) >= MAX_PATH) {
#if ENABLE_TTY_TRACE
    tty->print_cr("Library name is too long. %d chars maximum!", MAX_PATH - strlen(lib_ext) - 1);
    tty->print_cr("Library name %s",libName);
#endif
    return 0;
  }
  strcpy(full_lib_name, libName);
  strcat(full_lib_name, lib_ext);  
  void* result = dlopen(full_lib_name, RTLD_LAZY);
#if ENABLE_TTY_TRACE
  if (result == NULL)  {
    tty->print_cr("Failed to load library %s !",full_lib_name);
  }
#endif
  return result;
}
void* Os::getSymbol(void* handle, const char* name) {
  return dlsym(handle,name);
}
#endif

jlong Os::java_time_millis() {
    struct timeval tv;
    ::jvm_gettimeofday(&tv, NULL);
    /* We adjust to 1000 ticks per second */
    return (jlong)tv.tv_sec * 1000 + tv.tv_usec/1000;
}

/*
 * Sleep for ms Milliseconds, a sleep of 0ms is from the
 * scheduler requiring a yield, therefore we should call
 * sched_yield and not really sleep.
 */
void Os::sleep(jlong ms) {
  if (ms == 0) {
#if ENABLE_TIMER_THREAD
    // ideally we should use pthread_yield() but it's
    // not POSIX, and not defined on ARM, so use sched_yield instead.
    ::sched_yield();
#endif
    return;
  }

  jlong end = Os::java_time_millis() + ms;
  while (Os::java_time_millis() < end) {
    ::usleep(1000);  // sleep for 1 ms
  }
}

static bool   ticker_running = false;
static bool   ticker_stopping = false;
static bool   ticker_stopped = false;

AZZERT_ONLY(static bool is_processing_timer_tick = false;)

#if ENABLE_COMPILER
static bool   _compiler_timer_has_ticked = false;
static jlong  _compiler_timer_start;
#endif

static inline void rt_tick_event();

#if ENABLE_TIMER_THREAD

static sem_t ticker_semaphore;
static bool ticker_created = false;
static pthread_t main_thread_handle;

static pthread_t thread_create(int thread_routine(void *parameter),
                               void *parameter) {
  pthread_attr_t attr;
  pthread_t os_thread;

  if (::pthread_attr_init(&attr) != 0) {
    return 0;
  }

  // Don't leave thread/stack around after exit for join:
  if (::pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
    return 0;
  }

  // Apparently, it is a different type on Linux.
  void* (*routine)(void*) = (void*(*)(void *))thread_routine;

  if (::pthread_create(&os_thread, &attr, routine, NULL) != 0) {
    return 0;
  }
  return os_thread;
}

static int ticker_thread_routine(void *parameter) {
  ticker_stopped = false;
  while (!ticker_stopping) {
    ::usleep(TickInterval * 1000);

    if (ticker_running) {
      rt_tick_event();
    } else {
      ::sem_wait(&ticker_semaphore);
    }
  }
  ticker_stopped = true;
  return 0;
}

bool Os::start_ticks() {
  if (!EnableTicks || Deterministic) {
    return true;
  }

  ticker_running = true;
  ticker_stopping = false;
  if (ticker_created) {
    ::sem_post(&ticker_semaphore);
  } else {
    ticker_created = true;
    if (::sem_init(&ticker_semaphore, 0, 0) != 0) {
      return false;
    }
    if (thread_create(ticker_thread_routine, 0) == 0) {
      return false;
    }
  }
  return true;
}

void Os::suspend_ticks() {
  ticker_running = false;
  Os::sleep(1); // why is this necessary?
}

void Os::resume_ticks() {
  start_ticks();
}

void Os::stop_ticks() {
  if (ticker_created) {
    ticker_stopping = true;
    if (ticker_running) {
      for (int i=0; i<10 && !ticker_stopped; i++) {
        ::usleep(TickInterval * 1000);
      }
    } else {
      // ticker is currently suspended on a semaphore
      ::sem_post(&ticker_semaphore);
      for (int i=0; i<10 && !ticker_stopped; i++) {
        ::usleep(TickInterval * 1000);
      }
      ::sem_destroy(&ticker_semaphore);
    }
    ticker_created = false;
    ticker_running = false;
    ticker_stopped = false;
  }
}

#else // ENABLE_TIMER_THREAD

bool Os::start_ticks() {
  if (ticker_running || !EnableTicks || Deterministic) {
    return true;
  }

  ticker_running = true;

  struct itimerval interval;
  interval.it_interval.tv_sec  = TickInterval / 1000;
  interval.it_interval.tv_usec = (TickInterval % 1000) * 1000;
  interval.it_value.tv_sec  = interval.it_interval.tv_sec;
  interval.it_value.tv_usec = interval.it_interval.tv_usec;

  if (::jvm_setitimer(ITIMER_VIRTUAL, &interval, NULL) != 0) {
    return false;
  }
  return true;
}

void Os::suspend_ticks() {
  if (ticker_running) {
    struct itimerval interval = {{0,0}, {0,0}};
    if (::jvm_setitimer(ITIMER_VIRTUAL, &interval, NULL) != 0) {
      JVM_FATAL(system_resource_unavailable);
    }
    ticker_running = false;
  }
}

void Os::resume_ticks() {
  start_ticks();
}

void Os::stop_ticks() {
  suspend_ticks();
}

#endif // ENABLE_TIMER_THREAD

#if !ENABLE_TIMER_THREAD
extern "C" void handle_vtalrm_signal(int signo, siginfo_t* sigi, void* uc) {
  GUARANTEE(!is_processing_timer_tick, "Sanity");
  AZZERT_ONLY(TemporaryModifyGlobal(&is_processing_timer_tick,
                                    !is_processing_timer_tick);)

  if (ticker_running) {
#if ENABLE_CODE_PATCHING
    if (!VerifyOnly && 
        !Universe::is_bootstrapping() && 
        !Compiler::is_active()) {
      Compiler::patch_checkpoints(((CPUContext*)uc)->get_pc());
    }
#endif
    rt_tick_event();
  } else {
    struct itimerval interval;
    interval.it_interval.tv_sec  = interval.it_value.tv_sec  = 0;
    interval.it_interval.tv_usec = interval.it_value.tv_usec = 0;
    if (::jvm_setitimer(ITIMER_VIRTUAL, &interval, NULL) != 0) {
      JVM_FATAL(system_resource_unavailable);
    }
  }
}
#endif // !ENABLE_TIMER_THREAD

#if ENABLE_TIMER_THREAD && USE_LIBC_GLUE
extern "C" void handle_vtalrm_signal(int signo, siginfo_t* sigi, void* uc) {
  // dummy - do nothing
}
#endif

static bool printing_stack = false;

static void handle_other_signals(int sig) {
#if ENABLE_TIMER_THREAD
  // ignore signal in all threads, but main
  if (pthread_self() != main_thread_handle) {
    // maybe on some Unices (and NPTL-threaded Linux) this should be
    // uncommented (if signal is sent to only one, random, thread
    //pthread_kill(main_thread_handle, sig);
    return;
  }
#endif
  // If we have error inside a pp() call, last_raw_handle, etc, need to be
  // restored.
  DebugHandleMarker::restore();

  const char *name;

  switch (sig) {
  case SIGHUP:
    name = "SIGHUP";
    break;
  case SIGQUIT:
    name = "SIGQUIT";
    break;
  case SIGABRT:
    name = "SIGABRT";
    break;
  case SIGTERM:
    name = "SIGTERM";
    break;
  default:
    name = "??";
  }
#ifndef PRODUCT
  jvm_printf("received signal %s\n", name);
  if (!printing_stack) {
    printing_stack = true;
    pss();
    printing_stack = false;
  }
#endif
  if (sig == SIGHUP || sig == SIGQUIT || sig == SIGTERM) {
    return;
  }

  ::jvm_signal(SIGABRT, NULL);
  ::jvm_abort();
}


#define HAVE_SIGINFO 1
#if HAVE_SIGINFO

static void print_siginfo(siginfo_t *si) {
  const char *name;

  switch (si->si_signo) {
  case SIGILL:
    name = "SIGILL";
    break;
  case SIGBUS:
    name = "SIGBUS";
    break;
  case SIGSEGV:
    name = "SIGSEGV";
    break;
  default:                      // cannot happen
    name = "-unknown-";
    break;
  }
  jvm_fprintf(stderr,
              "Fatal signal %s: errno=%d; code=%d; addr=%p\n",
              name, si->si_errno, si->si_code, si->si_addr);
}

static void print_ucontext(void* context) {
#if defined(__NetBSD__)
  ucontext_t* uc = (ucontext_t*) context;
  jvm_fprintf(stderr, "UContext dump follows:\n");
  jvm_fprintf(stderr, "uc_flags=%x; ss_sp=%p; ss_size=%d, ss_flags=%x\n",
              (unsigned int) uc->uc_flags,
              uc->uc_stack.ss_sp, uc->uc_stack.ss_size, uc->uc_stack.ss_flags);

  mcontext_t *mc = &uc->uc_mcontext;
  int cnt = 4;

#define PRINT_REG(r) do {                                               \
      jvm_fprintf(stderr, "%7.7s=%08x%s", #r, mc->__gregs[_REG_##r],    \
                  --cnt ? "    " : "\n");                               \
      if (cnt == 0)                                                     \
        cnt = 4;                                                        \
  } while (0);

#if defined(__arm__)
  PRINT_REG(R0);   PRINT_REG(R1);   PRINT_REG(R2);   PRINT_REG(R3);
  PRINT_REG(R4);   PRINT_REG(R5);   PRINT_REG(R6);   PRINT_REG(R7);
  PRINT_REG(R8);   PRINT_REG(R9);   PRINT_REG(R10);  PRINT_REG(R11);
  PRINT_REG(R12);  PRINT_REG(R13);  PRINT_REG(R14);  PRINT_REG(PC);
  PRINT_REG(CPSR);
  jvm_fprintf(stderr, "\n");
#endif

#if defined(__sh3__)
  PRINT_REG(R0);   PRINT_REG(R1);   PRINT_REG(R2);   PRINT_REG(R3);
  PRINT_REG(R4);   PRINT_REG(R5);   PRINT_REG(R6);   PRINT_REG(R7);
  PRINT_REG(R8);   PRINT_REG(R9);   PRINT_REG(R10);  PRINT_REG(R11);
  PRINT_REG(R12);  PRINT_REG(R13);  PRINT_REG(R14);  PRINT_REG(SP);
  PRINT_REG(PC);   PRINT_REG(PR);   PRINT_REG(SR);   PRINT_REG(EXPEVT);
  PRINT_REG(MACH); PRINT_REG(MACL);
  jvm_fprintf(stderr, "\n");
#endif

#endif // __NetBSD__
}

#if ENABLE_PAGE_PROTECTION
extern "C" void interpreter_timer_tick();
extern "C" void compiler_timer_tick();
extern "C" void vsf_flush(address* regs);

static inline
bool protected_page_access(int signo, siginfo_t* info, void* context) {
  const juint offset = (address)info->si_addr - _protected_page;
  if (TracePageAccess) {
    TTY_TRACE_CR(("ACCESS_VIOLATION signaled: offset = %d", offset));
  }

  CPUContext* ctx = (CPUContext*)context;
  switch (offset) {
  case COMPILER_TIMER_TICK_SLOT:
    vsf_flush(ctx->get_regs());
    ctx->set_pc((address)compiler_timer_tick);
    return true;
  case INTERPRETER_TIMER_TICK_SLOT:
    ctx->set_pc((address)interpreter_timer_tick);
    return true;
  }
  return false;
}

#else

static inline
bool protected_page_access(int signo, siginfo_t* info, void* context) {
  return false;
}
#endif // ENABLE_PAGE_PROTECTION


static void handle_segv_siginfo(int signo, siginfo_t *info, void *context) {
  if (protected_page_access(signo, info, context)) {
    return;
  }

  print_siginfo(info);
  print_ucontext(context);
#ifndef PRODUCT
  if (!printing_stack) {
    printing_stack = true;
    new_pss();
    printing_stack = false;
  }
#endif
  ::exit(1);
}

#else  // ! HAVE_SIGINFO

// Solaris doesn't have sigcontext
#ifdef SOLARIS
struct sigcontext {
 int dummy;
};
#endif

#if KNOW_REGISTER_NAMES && !defined(PRODUCT)
static void print_regs(int signo, struct sigcontext* sigc) {
  #define PRINT_REG(str, name)                                   \
       jvm_fprintf(stderr, "%4s=0x%08lx   ", #str, sigc->name); \
       if (!--cnt) {  jvm_fprintf(stderr, "\n"); cnt = 4; }

  char* name;
  int   cnt = 4;

  switch (signo) {
  case SIGSEGV:
    name = "SIGSEGV";
#if ARM_EXECUTABLE
    {
      unsigned long fa = 0;
      jvm_fprintf(stderr, "%s[%ld] fault at address: 0x%08lx\n",
        (sigc->error_code & 1) ? "Write" : "Read", sigc->error_code, fa);
    }
#else
    jvm_fprintf(stderr, "%s[%ld] fault at address: 0x%08lx\n",
      (sigc->err & 2) ? "Write" : "Read", sigc->err, sigc->cr2);
#endif
    break;
  case SIGILL:
    name = "SIGILL";
    break;
  case SIGBUS:
    name = "SIGBUS";
    break;
  default:
    name = "-unknown-";
  }
  jvm_fprintf(stderr, "received fatal signal: %s\n", name);
  jvm_fprintf(stderr, "Full registers dump:\n");

#if ARM_EXECUTABLE
  jvm_fprintf(stderr, "[some values may be incorrect on ARM]\n");
  PRINT_REG(IP, arm_ip);
  PRINT_REG(SP, arm_sp);
  PRINT_REG(PC, arm_pc);
  PRINT_REG(LR, arm_lr);
  PRINT_REG(CPSR, arm_cpsr);
  PRINT_REG(R0, arm_r0);
  PRINT_REG(R1, arm_r1);
  PRINT_REG(R2, arm_r2);
  PRINT_REG(R3, arm_r3);
  PRINT_REG(R4, arm_r4);
  PRINT_REG(R5, arm_r5);
  PRINT_REG(R6, arm_r6);
  PRINT_REG(R7, arm_r7);
  PRINT_REG(R8, arm_r8);
  PRINT_REG(R9, arm_r9);
  PRINT_REG(R10, arm_r10);
#else
  PRINT_REG(EAX, eax);
  PRINT_REG(EBX, ebx);
  PRINT_REG(ECX, ecx);
  PRINT_REG(EDX, edx);
  PRINT_REG(EIP, eip);
  PRINT_REG(ESP, esp);
  PRINT_REG(ESI, esi);
  PRINT_REG(EBP, ebp);
  PRINT_REG(EDI, edi);
#endif
  jvm_fprintf(stderr, "\n");
}
#else
static void print_regs(int signo, struct sigcontext* sig) {
  jvm_fprintf(stderr, "Memory access error, please report the bug\n");
}
#endif

static void handle_segv_signal(int signo, struct sigcontext sigc) {
  print_regs(signo, &sigc);

#ifndef PRODUCT
  if (!printing_stack) {
    printing_stack = true;
    new_pss();
    printing_stack = false;
  }
#endif
}

#endif // ! HAVE_SIGINFO

#if ( ENABLE_NPCE ||\
     ( ENABLE_INTERNAL_CODE_OPTIMIZER)) \
     && ARM_EXECUTABLE
static void handle_segv_siginfo_npe(int sig, siginfo_t* info, void* ucpPtr) {
  if (protected_page_access(sig, info, ucpPtr)) {
    return;
  }

  struct ucontext* ucp = (struct ucontext *)ucpPtr;
  unsigned long pc = ucp->uc_mcontext.arm_pc;
  bool is_npe = true;
  bool found = false;
  
  int default_npce_stub = not_found;
  int default_array_check_stub = not_found;

  unsigned long heap_high ;
  unsigned long heap_low  ;
  if( (unsigned int) _heap_start > (unsigned int) _heap_top ) {
    heap_high = (unsigned long) _heap_start;
    heap_low = (unsigned long) _heap_top;
  } else {
    heap_high = (unsigned long) _heap_top;
    heap_low = (unsigned long) _heap_start;
  }

#ifndef PRODUCT
  if (VerboseNullPointExceptionThrowing) {
    TTY_TRACE(("Verbose exception information begin:\n"));      
    TTY_TRACE(("Heap Status:"));        
    TTY_TRACE(("[0x%08x,",heap_low));
    TTY_TRACE(("0x%08x]\n",heap_high));
  }
#endif

  if( pc < heap_low || pc > heap_high ) {
    TTY_TRACE(("Memory access error\nPlease report the bug\n"));
  }
  
  CompiledMethodDesc *cmd = ObjectHeap::method_contains_instruction_of((void *)pc);
  
  if( cmd == NULL){
    TTY_TRACE(("Memory access error\nPlease report the bug\n"));
    ::exit(1);
  }
  
  CompiledMethod::Raw method = cmd;
  Method::Raw raw_method = method().method();
  int code_begin = (int)cmd + CompiledMethod::base_offset();
  int stub_offset = not_found;
  int offset = pc - code_begin;
  int default_stub_offset = not_found;

#ifndef  PRODUCT
  if (VerboseNullPointExceptionThrowing) {
    TTY_TRACE(("Method status:\n  name:"));
    bool renamed;
    Symbol::Raw n = raw_method().get_original_name(renamed);
    n().print_symbol_on(tty);
    TTY_TRACE(("()"));
    TTY_TRACE((",=>[0x%08x]", code_begin));
  }
#endif

  if (VerboseNullPointExceptionThrowing) {
    TTY_TRACE((",offset[%d]\n", offset));
  }
  
#if ENABLE_NPCE 
  for (RelocationReader stream(&method); !stream.at_end(); stream.advance()) {
    if (stream.is_npe_item()) {
#ifndef  PRODUCT
      if (VerboseNullPointExceptionThrowing) {
        TTY_TRACE(("  fail at [%d] should jump to", stream.current(1)));                
        TTY_TRACE(("[%d]\n", stream.code_offset()));

      }
#endif
      //if a ldr/str offset is 0 stand for a record of default npe stub
      if (stream.current(1) == 0) {
        default_npce_stub = stream.code_offset();
      }

      //current(1) return the ldr/str offset
      //if the offset is non zero, the instr must have a correponding 
      //stub.
      if (stream.current(1) == offset) {
        stub_offset = stream.code_offset();
        if (VerboseNullPointExceptionThrowing) {
          TTY_TRACE(("  target stub is 0x%08x\n",stub_offset));
        }
        break;
      }
    }
  }
#endif 

#if ENABLE_INTERNAL_CODE_OPTIMIZER        
  for (RelocationReader stream(&method); !stream.at_end(); stream.advance()) {
    if (stream.is_pre_load_item()) {
#ifndef  PRODUCT
      if (VerboseNullPointExceptionThrowing) {
        TTY_TRACE((" if  failed at [%d] should jump to", stream.current(1)));           
        TTY_TRACE(("[%d]\t", stream.code_offset()));
      }
#endif
      if (stream.current(1) == 0) {
        default_array_check_stub = stream.code_offset();
      }
      if (!found && (stream.current(1) == offset)) {
        if (stream.is_pre_load_item()) {
          is_npe = false;
        }
        if (VerboseNullPointExceptionThrowing) {
          TTY_TRACE_CR(("  found a exception caused index out of bound"))
        }
        found = true;
      }
    }
  }
#endif 

  if (stub_offset != not_found ) {
    stub_offset = stub_offset + code_begin;
    ucp->uc_mcontext.arm_pc = (unsigned long) stub_offset;
#ifndef  PRODUCT
    if(VerboseNullPointExceptionThrowing) {
      TTY_TRACE(("  jump to [0x%08x]\n", stub_offset));
    }
#endif
  if(VerboseNullPointExceptionThrowing){
    TTY_TRACE(("Verbose exception information end\n")); 
  }     
    return;
  }

#if ENABLE_INTERNAL_CODE_OPTIMIZER        
  if (!is_npe) {
    if (default_array_check_stub != not_found ) {
      default_array_check_stub = (int)default_array_check_stub + code_begin;
      ucp->uc_mcontext.arm_pc = (unsigned long) default_array_check_stub ;      
      return ;
    } else {
      ucp->uc_mcontext.arm_pc = 
        (unsigned long)gp_compiler_throw_ArrayIndexOutOfBoundsException_ptr; 
      ucp->uc_mcontext.arm_r0 = raw_method(). max_locals();
      if(VerboseNullPointExceptionThrowing){      
        TTY_TRACE(("Verbose exception information end\n"));               
      }
      return ;
    }
  }
#endif

  if ( default_npce_stub != not_found ) {
    stub_offset = (int)default_npce_stub + code_begin;
    ucp->uc_mcontext.arm_pc = (unsigned long) stub_offset;
#ifndef  PRODUCT
    if (VerboseNullPointExceptionThrowing) {
        TTY_TRACE(("  jump to default [0x%08x]\n", stub_offset));
    }
#endif
    if(VerboseNullPointExceptionThrowing){
      TTY_TRACE(("Verbose exception information end\n"));       
    }
    return;
  }

  int max_locals = raw_method().max_locals();
  ucp->uc_mcontext.arm_pc = (unsigned long) gp_compiler_throw_NullPointerException_ptr;
  ucp->uc_mcontext.arm_r0 = max_locals;
  if (VerboseNullPointExceptionThrowing) {
    TTY_TRACE(("  jump to fixed stub\n"));
  }
  
  if(VerboseNullPointExceptionThrowing){
    TTY_TRACE(("Verbose exception information end\n")); 
  }
  return;

  TTY_TRACE(("Memory access error\nPlease report the bug\n"));
  ::exit(1);
}
#endif //ENABLE_NPCE


#if NEED_CLOCK_TICKS
static jlong _performance_frequency = 0;

#if NEED_XSCALE_PMU_CYCLE_COUNTER
// See more information inside internal_misc/xscale (Sun internal)
static int   pmu_fd          = -1;    /* file descriptor */
static int   pmu_refcount    = 0;     /* fd reference counter */
static char *pmu_counter     = "/dev/ixs_ins_counter"; 
static int   pmu_cpu_clk_mhz = 0;

static inline void ixs_open_ins_counter() {
  if (pmu_fd < 0 && (pmu_fd = jvm_open(pmu_counter, O_RDWR)) < 0 ) {
    perror("pmu counter error: unable to open device\n");
    pmu_fd = -1;
  } else {
    ioctl(pmu_fd, PMU_GET_CPU_CLK, &pmu_cpu_clk_mhz);
    pmu_refcount ++;
  }
}

static inline  void ixs_close_ins_counter() {
  if (pmu_fd >= 0) {
    if (--pmu_refcount == 0) {
      jvm_close(pmu_fd); 
      pmu_fd = -1;
    }
  }
}

static inline jlong ixs_cycles(void) {
  jlong result;
  if (pmu_fd >= 0) {
    ioctl(pmu_fd, PMU_GET_INS_COUNTER, &result);
  }
  return result;
}
#endif

#ifdef __i386__

static inline jlong get_clock_ticks(void)
{
  unsigned long low_time, high_time;
  asm volatile(
               "rdtsc \n\t"
               : "=a" (low_time),
                 "=d" (high_time));
  return (jlong)((unsigned long long)high_time << 32) | (low_time);
}

static inline void init_clock_ticks(void) {
  jlong t1, t2;

  t1 = get_clock_ticks();
  // no more to not affect startup time badly
  Os::sleep(20);
  t2 = get_clock_ticks();

  _performance_frequency = (t2 - t1) * (1000 / 20);
}

#elif defined(SOLARIS)

static inline jlong get_clock_ticks(void)
{
  return (jlong)gethrtime();
}

static inline void init_clock_ticks(void) {
  // gethrtime return nanoseconds
  _performance_frequency = 1000000000;
}

#else

static inline jlong get_clock_ticks(void) {
  struct timeval tv;
  ::jvm_gettimeofday(&tv, NULL);
  return jlong(tv.tv_sec) * jlong(1000 * 1000) + jlong(tv.tv_usec);
}

static inline void init_clock_ticks(void) {
   // micro seconds
  _performance_frequency = 1000 * 1000;
}

#endif

jlong Os::elapsed_counter() {
#if ENABLE_PERFORMANCE_COUNTERS
  jvm_perf_count.hrtick_read_count++;
#endif

#if NEED_XSCALE_PMU_CYCLE_COUNTER
  return ixs_cycles();
#else
  return get_clock_ticks();
#endif
}

jlong Os::elapsed_frequency() {
#if NEED_XSCALE_PMU_CYCLE_COUNTER
  return pmu_cpu_clk_mhz * 1000 * 1000;
#else
  return _performance_frequency;
#endif
}

#endif // NEED_CLOCK_TICKS

#if ENABLE_ARM_VFP
static void ignoreit() {}
#endif

extern "C" void jvm_set_vfp_fast_mode();
void Os::initialize() {
#if ENABLE_ARM_VFP && !CROSS_GENERATOR
  if (RunFastMode) {
    // Linux by default does not use RunFast mode. This option makes
    // it easy to test the VM's compatibility with RunFast mode.
    jvm_set_vfp_fast_mode();
  }
#endif

#if ENABLE_ARM_VFP
  // For some reason this SIG_IGN doesn't work on Linux 2.6 on ARM.
  // ::jvm_signal(SIGFPE, SIG_IGN);
  ::jvm_signal(SIGFPE, (sighandler_t)&ignoreit);
#endif

  // it better be here so that profiler timings not damaged by
  // itimer-generted signals, although nanosleep() shouldn't
  // be heavily affected by signals
#if NEED_CLOCK_TICKS
  init_clock_ticks();
#if NEED_XSCALE_PMU_CYCLE_COUNTER
  ixs_open_ins_counter();
#endif
#endif

#if SUPPORTS_ADJUSTABLE_MEMORY_CHUNK
  // memory manager is in OsMemory
  init_jvm_chunk_manager();
#endif

  int sa_flags = SA_RESTART;

#if !defined(ARM)
  // force execution of signals on separate stack, as otherwise
  // results can be unpredictable if we'll modify stack in
  // Java heap and VM will not know. We handle signals either when
  // using SIGVTALRM, or when printing pss() in case of a crash.
# if defined(PRODUCT)
# define ALT_STACK_SIZE (MINSIGSTKSZ)
# else
# define ALT_STACK_SIZE (MINSIGSTKSZ + 2048)
# endif

  static char alt_stack_buf[ALT_STACK_SIZE];
  stack_t alt_stack;
  alt_stack.ss_sp = alt_stack_buf;
  alt_stack.ss_size = ALT_STACK_SIZE;
  alt_stack.ss_flags = 0;
  ::jvm_sigaltstack(&alt_stack, NULL);
  sa_flags |= SA_ONSTACK;
#endif // !defined(ARM)

#if ENABLE_TIMER_THREAD
  main_thread_handle = pthread_self();
#else
  // setup vtalarm signal handler to provide a source of
  // timer-based interrupts for Java thread switching and dynamical
  // compilation (Hotspot detection)
  static struct sigaction vtalrm_action;
  ::jvm_sigaction (SIGVTALRM, NULL, &vtalrm_action);
  vtalrm_action.sa_handler = (void (*)(int)) handle_vtalrm_signal_stub;
  vtalrm_action.sa_flags = sa_flags | SA_SIGINFO;
  ::jvm_sigaction (SIGVTALRM, &vtalrm_action, NULL);
#endif // ENABLE_TIMER_THREAD

  // setup handler for SEGV - it prints native registers and Java backtrace
#if ( ENABLE_NPCE || ( ENABLE_INTERNAL_CODE_OPTIMIZER ) ) && ARM_EXECUTABLE
  static struct sigaction segv_action;
  ::jvm_sigaction(SIGSEGV, NULL, &segv_action);
  segv_action.sa_sigaction = handle_segv_siginfo_npe;
  segv_action.sa_flags = sa_flags | SA_SIGINFO;
#elif HAVE_SIGINFO
  struct sigaction segv_action;
  segv_action.sa_sigaction = handle_segv_siginfo;
  segv_action.sa_flags = sa_flags | SA_SIGINFO;
  sigemptyset(&segv_action.sa_mask);
#else
  static struct sigaction segv_action;
  ::jvm_sigaction(SIGSEGV, NULL, &segv_action);
  segv_action.sa_handler = (void (*)(int)) handle_segv_signal;
  segv_action.sa_flags = sa_flags;
#endif // ENABLE_NPCE

  ::jvm_sigaction(SIGSEGV, &segv_action, NULL);
  ::jvm_sigaction(SIGILL, &segv_action, NULL);
  ::jvm_sigaction(SIGBUS, &segv_action, NULL);

  // setup handlers of other signals
  static struct sigaction other_action;
  sigemptyset(&other_action.sa_mask);
  other_action.sa_handler = (void (*)(int)) handle_other_signals;
  other_action.sa_flags = sa_flags;

  ::jvm_sigaction(SIGABRT, &other_action, NULL);
  ::jvm_sigaction(SIGQUIT, &other_action, NULL);
  ::jvm_sigaction(SIGTERM, &other_action, NULL);
  ::jvm_sigaction(SIGHUP,  &other_action, NULL);
  ::jvm_signal(SIGPIPE, SIG_IGN);
}


/*
 * The Os::dispose method needs to correctly clean-up
 * all threads and other OS related activity to allow
 * for a clean and complete restart.  This should undo
 * all the work that initialize does.
 */
void Os::dispose() {
#if NEED_XSCALE_PMU_CYCLE_COUNTER
  ixs_close_ins_counter();
#endif
}

extern "C" void arm_flush_icache(address start, int size);
extern "C" void brute_force_flush_icache();

void OsMisc_flush_icache(address start, int size) {
#if ARM_EXECUTABLE
#if ENABLE_BRUTE_FORCE_ICACHE_FLUSH
  // This is a brute-force way of flushing the icache. The function
  // brute_flush_icache() contains 64KB of no-ops.
  volatile int x;
  volatile int * ptr = (int*)brute_force_flush_icache;
  for (int i=0; i<8192; i++) {
    // need to flush writeback cache, too.
    x = *ptr++;
  }
  brute_force_flush_icache(); // IMPL_NOTE: jump to brute_flush_icache() + xx
                              // if size is small ...
#elif defined(__NetBSD__)
  // arch-specific syscall from libarm
  arm_sync_icache((unsigned int)start, size);
#else
  // This is in assembly language
  arm_flush_icache(start, size);
#endif
#endif // ARM_EXECUTABLE

  // Valgrind deploys VM technology with JITter so when we modify ourselves
  // we need to let Valgrind know about it
#if ENABLE_VALGRIND
  VALGRIND_DISCARD_TRANSLATIONS(start, size);
#endif
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
#if ENABLE_COMPILER
  if (MaxCompilationTime == TickInterval) {
    // Note: this tend to make the average compilation period to be
    // 0.5 * MaxCompilationTime.
    _compiler_timer_start = (jlong)0;
    _compiler_timer_has_ticked = false;
  } else {
    _compiler_timer_start = Os::java_time_millis();
  }
#endif
}

/*
 * Returns true iff the current compilation has taken too long and
 * should be suspended and resumed later.
 */
bool Os::check_compiler_timer() {
#if ENABLE_COMPILER
  if (_compiler_timer_start == (jlong)0) {
    return _compiler_timer_has_ticked;
  } else {
    jint elapsed_ms = (jint)(Os::java_time_millis() - _compiler_timer_start);
    return (elapsed_ms >= MaxCompilationTime);
  }
#else
  return true;
#endif
}

static inline void rt_tick_event() {
  real_time_tick(TickInterval);
#if ENABLE_COMPILER
  _compiler_timer_has_ticked = true;
#endif
}
