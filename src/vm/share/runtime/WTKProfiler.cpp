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

#include "incls/_precompiled.incl"
#include "incls/_WTKProfiler.cpp.incl"

/* This profiler originally based on KVM profiler */ 

#if ENABLE_WTK_PROFILER
static inline void* wtk_malloc(size_t size) {
  return GlobalObj::malloc_bytes(size);
}

static inline void wtk_free(void* ptr) {
  GlobalObj::free_bytes(ptr);
}

// you can increase it to speed up profiler a bit
#define TABLE_SIZE        239

// flags used for call records
// method is in ROM, thus we can use raw pointer, as it never moved 
#define METHOD_IN_ROM       (1 << 0)

#if ENABLE_ISOLATES
#define CHECK_TASK(rec, tid) (rec->task_id == tid)
#else
#define CHECK_TASK(rec, tid) 1
#endif

struct WTKCallRecord;

class WTKThreadRecordDesc : public MixedOopDesc {
  juint           _stack_depth;
  jint            _top_call_record; /* in fact - pointer in native heap */
  
private:
  static size_t allocation_size() { 
    return align_allocation_size(sizeof(WTKThreadRecordDesc));
  }
  static int pointer_count() {
    return 0;
  }
  friend class WTKThreadRecord;
};

class WTKThreadRecord : public MixedOop {
public:  
  HANDLE_DEFINITION_CHECK(WTKThreadRecord, MixedOop);

  static ReturnOop allocate_thread_record(JVM_SINGLE_ARG_TRAPS) {
    return Universe::new_mixed_oop(MixedOopDesc::Type_WTKThreadRecord,
                                   WTKThreadRecordDesc::allocation_size(),
                                   WTKThreadRecordDesc::pointer_count()
                                   JVM_NO_CHECK_AT_BOTTOM);
  }

  static jint stack_depth_offset() {
    return FIELD_OFFSET(WTKThreadRecordDesc, _stack_depth);
  }
  static jint top_call_record_offset() {
    return FIELD_OFFSET(WTKThreadRecordDesc, _top_call_record);
  }

  juint stack_depth() {
    return uint_field(stack_depth_offset());
  }

  void set_stack_depth(juint value) {
    uint_field_put(stack_depth_offset(), value);
  }

  WTKCallRecord* top_call_record() {
    return (WTKCallRecord*)int_field(top_call_record_offset());
  }
  
  void set_top_call_record(WTKCallRecord* value) {
    int_field_put(top_call_record_offset(), (jint)value);
  }
};

WTKThreadRecord* thread_record = NULL;

// should be enough
HANDLE_CHECK(WTKThreadRecord, is_mixed_oop())

struct WTKCallRecord {
  //  keep those at the top for better alignment
  jlong            thisTime;  /* Time spend in this.method */
  jlong            totalTime; /* Time spend in this.method and all
                               * of its descendants */
  jlong            startTime; /* Absolute time of the first entering */
  jlong            endTime;   /* Absolute time of the last leaving */
  WTKCallRecord*   next;      /* used for hashtable bookkeeping */
  WTKCallRecord*   child;     /* JProfCallRecord of the method
                               * currently executed by this.method,
                               * or NULL if this method is at the
                               * top of the call stack */
  WTKCallRecord*   parent;    /* JProfCallRecord of the Method that
                               * called this.method, or
                               * NULL if this is at the bottom of a
                               * call stack. */
  OopDesc*         cached_method; /* cached method of the current Frame*/
  juint            id;        /* unique method descriptor */ 
  juint            level;     /* nest level of this method call,
                               * 1 for the method at the stack
                               * bottom  */
  jint             index;     /* index for this call */
  juint            numCalls;  /* Number of times this method
                               * is called */
  jushort          flags;     /* flags to describe this record */
#if ENABLE_ISOLATES
  jushort          task_id;   /* isolate owning this record */
#endif
  
  WTKCallRecord(juint          id,
                WTKCallRecord* parent,
                juint          level, 
                jushort        flags) : 
    thisTime(0), totalTime(0), next(NULL), child(NULL), parent(parent), 
    cached_method(NULL),
    id(id), level(level), numCalls(0), flags(flags) {
    index = ++numCallRecords;
    endTime = startTime = Os::elapsed_counter();
#if ENABLE_ISOLATES
    int tid = TaskContext::current_task_id();
    GUARANTEE(tid > 0 && tid < 0xffff, "must be in bounds");
    task_id = (jushort)tid;
#endif
  }

  void* operator new(size_t size){
    return wtk_malloc(size);
  }
  
  void operator delete(void* p) {
    wtk_free(p);
  }

  static juint                 numCallRecords;
};


jlong WTKProfiler::_lastCycles;
jlong WTKProfiler::_totalCycles = 0;
jlong WTKProfiler::_vmStartTime;
jint  WTKProfiler::_dumpedProfiles = 0;

juint WTKCallRecord::numCallRecords = 0;

static WTKCallRecord*    profilerTable[TABLE_SIZE];

// this one should be optimized for speed
static inline juint encode_heap_method(Method* m) {
  jushort holder_id = m->holder_id();
  InstanceClass::Raw holder = Universe::class_from_id(holder_id);  
  ObjArray::Raw methods;

  methods = holder().methods();
  int numMethods = methods().length();
  int i;
  OopDesc* obj = m->obj();
  OopDesc** raw_method = (OopDesc**)methods().base_address();
  for (i = 0; i < numMethods; i++) {
    if (obj == *raw_method) {
      break;
    }
    raw_method++;
  }
  
  if (i == numMethods) {
    return (juint)-1;
  }

  return (((juint)i) << 16) | holder_id;
}

static inline juint encode_id(Method* m, jushort& flags) {
  // methods in ROM never move
  if (ROM::system_text_contains(m->obj())) {
    flags |= METHOD_IN_ROM;
    return (juint)m->obj();
  }

  return encode_heap_method(m);
}

static inline ReturnOop decode_id(juint id, jushort flags) {
  if (flags & METHOD_IN_ROM) {
    return (ReturnOop)id;
  }

  jushort holder_id = (jushort)(id & 0xffff);
  InstanceClass::Raw holder = Universe::class_from_id(holder_id);

  ObjArray::Raw methods = holder().methods();

  jint idx = (id >> 16) & 0xffff;

  return  methods().obj_at(idx);
}


static inline jdouble perc_scale(jlong val, jlong total) {
  return jvm_dmul(jvm_ddiv(jvm_l2d(val), jvm_l2d(total)), 100.0);
}

void WTKProfiler::initialize() {
  jvm_memset(&profilerTable, 0, sizeof(profilerTable));
  _vmStartTime = Os::elapsed_counter();
}

ReturnOop  WTKProfiler::allocate_thread_data(JVM_SINGLE_ARG_TRAPS) {
  return 
    WTKThreadRecord::allocate_thread_record(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

void WTKProfiler::engage() {  
  _lastCycles = Os::elapsed_counter();
}

void WTKProfiler::disengage() {
  _lastCycles = 0;
  ForAllThreads(thread) {
    WTKThreadRecord::Raw trec(thread().profiler_info());
    if (trec.not_null()) {
      trec().set_top_call_record(NULL);
    }
  }
}

void WTKProfiler::dispose(int id) {
  for (int i=0; i<TABLE_SIZE; i++) {
    WTKCallRecord *rec = profilerTable[i], *next;
    WTKCallRecord *new_head = NULL, *last = NULL;
    
    while (rec) {
      next = rec->next;
      if (id == -1 || CHECK_TASK(rec, id)) {
        delete rec;
      } else {
        if (new_head == NULL) {
          new_head = rec;
        }       
        if (last != NULL) {
          last->next = rec;
        }
        last = rec;
      }
      rec = next;
    }

    
    if (last) {
      last->next = NULL;
    }

    profilerTable[i] = new_head;
  }
}

void WTKProfiler::print(Stream* out, int id) {
#if ENABLE_ISOLATES
  TaskContextSave maybeSwitchTask;
  GUARANTEE(id == -1 || TaskContext::current_task_id() == id, 
            "must be in current isolate");
#else
  GUARANTEE(id == -1, "sane");
#endif

  jlong freq = Os::elapsed_frequency();

  out->print_cr("index\tparent\tdepth\tfunction name\tcount\t"
                "only cycles\tonly msec\tonly %%\tplus kids cycles\t"
                "plus kids msec\tplus kids %%\tabs start\tabs end\t"
                "interrupt msec\tinterrupt count/debug");

  for (int i=0; i<TABLE_SIZE; i++) {
    WTKCallRecord *rec = profilerTable[i];

    while (rec) {
      // ignore irrelevant records
      if (id != -1 && !CHECK_TASK(rec, id)) {
        rec = rec->next;
        continue;
      }
#if ENABLE_ISOLATES      
      // switch to proper isolate, if we have data for multiple isolates 
      // in here
      if (id == -1) {   
        Universe::set_current_task(rec->task_id);
      } 
#endif
      Method::Raw m = decode_id(rec->id, rec->flags);

      // index, parent, depth
      out->print("%d\t%d\t%d\t", 
                 rec->index, 
                 rec->parent ? rec->parent->index : -1,
                 rec->level - 1);

      // handle removed class initializers
      if (m.is_null()) {
        out->print("Unknown<clinit>");
      } else {
        // method class, name and signature
        bool dummy;
        JavaClass::Raw ic = m().holder();
        ClassInfo::Raw info = ic().class_info();
        Symbol::Raw class_name = info().name();
        class_name().print_symbol_on(out);
        out->print(".");        
        Symbol::Raw method_name = m().get_original_name(dummy);
        Signature::Raw sig = m().signature();
        method_name().print_symbol_on(out);
        sig().print_decoded_on(out);    
      }

      // count
      out->print("\t%d", rec->numCalls);

      // only cycles, only ms, only %
      out->print("\t"JVM_LLD"\t"JVM_LLD"\t%.2lf", 
                 rec->thisTime, 
                 rec->thisTime * 1000 / freq,
                 perc_scale(rec->thisTime, _totalCycles)
                 ); 

      // plus kids cycles, plus kids ms, plus kids %
      out->print("\t"JVM_LLD"\t"JVM_LLD"\t%.2lf", 
                 rec->totalTime,
                 rec->totalTime * 1000 / freq,
                 perc_scale(rec->totalTime, _totalCycles)
                 );
      
      // start time, end time (absolute in ms since JVM start)
      jlong startTime = (rec->startTime - _vmStartTime) * 1000000 / freq;
      jlong endTime = (rec->endTime - _vmStartTime) * 1000000 / freq;
      out->print("\t%d.%06d\t%d.%06d",
                 (int) (startTime / 1000000), (int) (startTime % 1000000),
                 (int) (endTime / 1000000), (int) (endTime % 1000000));

      // IMPL_NOTE: not implemented now - to keep proper format only
      // interrupt msec, interrupt count
      out->print("\t0\t0");
      
      out->cr();
      rec = rec->next;
    }
  }
}

static inline int stack_depth(Frame* frame) {
  int rv = 0;
  // following is optimized implementation of the stack depth computation.
  // Note that it uses intrinsic knowledge about frame layout.
  address fp = frame->fp();
  while (true) {    
    address ret_addr = *(address*) (fp + JavaFrame::return_address_offset());

    if (ret_addr == (address)EntryFrame::FakeReturnAddress) {
      // This is an EntryFrame
      fp = *(address*)(fp + EntryFrame::stored_last_fp_offset());
      if (fp == NULL) {
        break;
      }
    } else {
      rv++;
      fp = *(address*)(fp + JavaFrame::caller_fp_offset());
    }
  }
  return rv;
}

static WTKCallRecord* lookup_record(int level, 
                                    WTKCallRecord* parent, 
                                    juint id, 
                                    jushort flags) {
  WTKCallRecord* rec;
#if ENABLE_ISOLATES
  // cache in local
  int task_id = TaskContext::current_task_id();
#endif

  juint idx = (id ^ (juint)parent) % TABLE_SIZE;

  if (profilerTable[idx] == NULL) {
    rec = new WTKCallRecord(id, parent, level, flags);
    if (rec == NULL) {
      goto error;
    }
    profilerTable[idx] = rec;
  } else {
    bool found = false;
    rec = profilerTable[idx];
    do {
      if ((rec->id == id) && (rec->parent == parent) 
          && (rec->level == (juint)level) ) {
#if ENABLE_ISOLATES
        if (!CHECK_TASK(rec, task_id)) {
          continue;
        }
#endif
        // handle recursive case ???
        found = true;
        break;
      }
    } while (rec->next && (rec = rec->next) != NULL);
    if (!found) {
      rec = new WTKCallRecord(id, parent, level, flags);
      if (rec == NULL) {
        goto error;
      }
      rec->next = profilerTable[idx];
      // there're high chances that newly allocated record will be used now
      profilerTable[idx] = rec;
    }
  }

  GUARANTEE(rec != NULL, "sane");
  return rec;

 error:
  return NULL;
}

static inline WTKCallRecord* call_record_from_frame(JavaFrame*     frame, 
                                                    WTKCallRecord* parent,
                                                    int            level) {

  Method::Raw m = frame->method();
  jushort flags = 0;
  juint id = encode_id(&m, flags);

  return lookup_record(level, parent, id, flags);
}

static WTKCallRecord* build_call_tree(JavaFrame* frame, int depth) {
  WTKCallRecord* parent = NULL;
  if (depth > 1) {
    Frame f(*frame);
    frame->caller_is(f);
    while (f.is_entry_frame()) {
      GUARANTEE(!f.as_EntryFrame().is_first_frame(), "inconsistent stack");
      f.as_EntryFrame().caller_is(f);
    }
    parent = build_call_tree((JavaFrame*)&f, depth - 1);
  }
  return call_record_from_frame(frame, parent, depth);
}

static inline void update_from_frame(JavaFrame* frame, 
                                     jlong delta) {
  WTKCallRecord* top = thread_record->top_call_record();  
  jint currentDepth = stack_depth(frame);
  
  if (top == NULL) {
    top = build_call_tree(frame, currentDepth);
    // out of memory
    if (top == NULL) {
      return;
    }
    thread_record->set_top_call_record(top);
    thread_record->set_stack_depth(currentDepth);
  }

  int diff = currentDepth - thread_record->stack_depth();
  WTKCallRecord* rec = NULL;
     
  switch (diff) {
  case 1:
    {
      WTKCallRecord* child = top->child;
      if (child) {      
        // Although doesn't look GC-safe, the only possible problem could
        // very small chance to account wrong method      
        if (child->cached_method == frame->method()) {
          // experiments with Minimi showed that cached value is hit in >50%
          // cases, thus it seems to be worthy to use such optimization
          rec = child;
          goto record_found;
        }
      }
      if (currentDepth == 1) {
        rec = top;
      } else {
        rec = call_record_from_frame(frame, top, currentDepth); 
      }
      // out of memory
      if (rec == NULL) {
        return;
      }
      rec->cached_method = frame->method();
    record_found:
      // we just called into this function
      rec->numCalls++;
      if (currentDepth > 1) {
        rec->parent = top;
        GUARANTEE(top->level+1 == rec->level, "sane");
      }
      break;
    }
  case 0:
    // we returned from the native method or thread switch happened
    // and we returned here after we scheduled again
    // just add time to the current record
    rec = top;
    break;
  case -1:
    // we just returned from the function call to "top"

    // quicker optimization not to call Os::elapsed_counter()
    top->endTime = WTKProfiler::_lastCycles + delta;

    rec = top->parent;
    if (rec) {
      rec->child = top;
    } else {
      // we returned on the top of the thread
      GUARANTEE(currentDepth == 0, "sanity");      
    }
    break;
  default: 
    // some transition missed - need revisit
    SHOULD_NOT_REACH_HERE();
  }
  
  top->thisTime += delta;
  top->totalTime += delta;

  thread_record->set_top_call_record(rec);
  thread_record->set_stack_depth(currentDepth);
    
  // update all callers
  rec = top->parent;
  while (rec != NULL) {
    rec->totalTime += delta;
    rec = rec->parent;
  }
}

static inline void update_in_thread(Thread* thread, jlong delta) {
  // we can be called when thread is already exited, for example 
  // when yield() during thread exit leads to profiler suspend
  if (thread->last_java_sp() == NULL) {
    return;
  }

  JavaFrame frame(thread);
  update_from_frame(&frame, delta);
}


void WTKProfiler::record_method_transition(Thread* thread) {
  if (!UseExactProfiler) {
    return;
  }

  jlong delta = Os::elapsed_counter() - _lastCycles;

  _totalCycles += delta;

  WTKThreadRecord::Raw trec(thread->profiler_info());
  // no memory allocations happens after this one set
  thread_record = &trec;

  update_in_thread(thread, delta);

  AZZERT_ONLY(thread_record = NULL);

  // by doing that we're trying to minimize profiler impact on results
  _lastCycles = Os::elapsed_counter();

}

void WTKProfiler::record_thread_transition() {  
  if (!UseExactProfiler) {
    return;
  }

  jlong delta = Os::elapsed_counter() - _lastCycles;
  _totalCycles += delta;
  
  WTKThreadRecord::Raw trec(Thread::current()->profiler_info());
  // no memory allocations happens after this one set
  thread_record = &trec;

  // we switch to new thread, so just update values in current thread  
  update_in_thread(Thread::current(), delta);

  AZZERT_ONLY(thread_record = NULL);

  // by doing that we're trying to minimize profiler impact on timings
  _lastCycles = Os::elapsed_counter();
}

void WTKProfiler::record_exception(Thread* thread, Frame* new_current_frame) {
  if (!UseExactProfiler) {
    return;
  }
  
  jlong delta = Os::elapsed_counter() - _lastCycles;
  _totalCycles += delta;

  WTKThreadRecord::Raw trec(thread->profiler_info());
  // no memory allocations happens after this one set
  thread_record = &trec;  

  int new_stack_depth =  stack_depth(new_current_frame);
  
  int diff = thread_record->stack_depth() - new_stack_depth;
  WTKCallRecord* rec = thread_record->top_call_record();
  GUARANTEE(diff >= 0, "sane");
    
  // update current top frame
  rec->thisTime += delta;

  // update total in this and all callers
  WTKCallRecord* crec = rec;
  while (crec != NULL) {
    crec->totalTime += delta;
    crec = crec->parent;
  }

  // go to the destination frame
  for (int i=0; i < diff; i++) {
    rec = rec->parent;
  }

  GUARANTEE((rec == NULL  && new_stack_depth == 0) || 
            (rec->level == (juint)new_stack_depth), "sane");
  thread_record->set_top_call_record(rec);
  thread_record->set_stack_depth(new_stack_depth);

  AZZERT_ONLY(thread_record = NULL);
  
  _lastCycles = Os::elapsed_counter();
}

void  WTKProfiler::suspend() {
  // in some sense it's switch to another, non-Java thread
  record_thread_transition();
  
  _lastCycles = 0;
}

void  WTKProfiler::resume() {
  GUARANTEE(_lastCycles == 0, "must be suspended");

  _lastCycles = Os::elapsed_counter();
}


int WTKProfiler::dump_and_clear_profile_data(int id) {
  if (UseExactProfiler) {
    bool do_suspend_resume = _lastCycles != 0;
    if (do_suspend_resume) {
      suspend();
    }

  bool empty = true;
    for (int i=0; i<TABLE_SIZE; i++) {
      if (profilerTable[i] != NULL) {
        empty = false;
        break;
      }
    }

    if (empty) {
        return -1;
    }

    static JvmPathChar filenamen[] = {
      'g','r','a','p','h','?','?','.','p','r','f',0
    };
    static const JvmPathChar filename0[] = {
      'g','r','a','p','h','.','p','r','f',0
    };
    const JvmPathChar* filename;
    // name of profile information file received from a property
    char * property_value = NULL;
    JvmPathChar* filename_property = NULL;

#if !ENABLE_ISOLATES
    if (!SaveSerialProfiles) {
      filename = filename0;
      _dumpedProfiles = 0;
    } else 
#endif
    {
      property_value = JVMSPI_GetSystemProperty("profiler.filename");
      if (property_value != NULL) {
          unsigned  i;
        unsigned int len = strlen(property_value);
          size_t size = (len+1) * sizeof(JvmPathChar);

        filename_property = (JvmPathChar*)OsMemory_allocate(size);
        if (filename_property == NULL) {
              return -1;
          }

          for (i = 0; i < len; i++) {
          filename_property[i] = (JvmPathChar) property_value[i];
          }
        filename_property[i] = 0;
              
        filename = filename_property;
      } else { // Use default filename
        /*
         * If the VM is re-started in the same process, we write the profile
         * information to a new file.
         */
          const int n = _dumpedProfiles % 100;
          filenamen[5] = (JvmPathChar)((n / 10) + '0');
          filenamen[6] = (JvmPathChar)((n % 10) + '0');
          filename = filenamen;
      }
    }

    Stream* out = NULL;
    // for GBA we don't have file system
#ifdef GBA
    out = tty;
#else
    // to have enough width for profiler info
    FileStream s(filename, 200);

    // all this only to allow Unicode filenames
    tty->print("*** Storing profile data to ");
    if (filename == filename0) {
       tty->print_cr("graph.prf");
    } else if (filename == filename_property) {
      tty->print_cr("%s", property_value);
    } else {
      tty->print_cr("graph%02d.prf", 
                    _dumpedProfiles % 100);
    }
    out = &s;
#endif

    if (filename_property != NULL ) {
      OsMemory_free((void *)filename_property);
      filename_property = NULL;
      filename = NULL;
	}

    print(out, id);
    
    dispose(id);
    if (do_suspend_resume) {
      resume();
    }

    return _dumpedProfiles++;
  } else {
    return -1;
  }
}

extern "C" jboolean JVM_SendProfilerCommand(int cmd, void* arg) {
  switch (cmd) {
  case JAVA_PROFILER_COMMAND_START:
    UseExactProfiler = 1;
    WTKProfiler::engage();
    break;
  case  JAVA_PROFILER_COMMAND_STOP:
    WTKProfiler::disengage();
    UseExactProfiler = 0;
    break;
  case JAVA_PROFILER_COMMAND_DUMP_AND_CLEAR:
    {
      GlobalSaver saver(&UseExactProfiler);
      UseExactProfiler = 1;
      int tid = -1;
#if ENABLE_ISOLATES
      if (arg != NULL) {
        tid = (int)arg;
      }
#endif
      int rv = WTKProfiler::dump_and_clear_profile_data(tid);
      return rv != -1;
    }
  default:
    return false;
  }
  
  return true;
}
#endif
