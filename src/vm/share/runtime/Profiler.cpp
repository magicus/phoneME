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
#include "incls/_Profiler.cpp.incl"

#if ENABLE_PROFILER

int _jvm_profiler_in_native_method = 0;

class FlatProfiler;

static jdouble scale(int x, int total) {
  return jvm_f2d(jvm_fdiv(jvm_fmul(jvm_i2f(x), 100.0f), jvm_i2f(total)));
}

class Ticks {
 public:
  int unknown;
  int runtime_compiler;
  int runtime_gc;
  int java_native;

  Ticks() {
    unknown                  = 0;
    runtime_compiler         = 0;
    runtime_gc               = 0;
    java_native              = 0;
  }

  int total_ticks() { return unknown + runtime_compiler + runtime_gc + java_native; }

  void print_ticks(Stream* out, const char* title, int ticks, int total) {
    if (ticks > 0) {
      int msec = ticks * TickInterval;
      out->print("%5.1f%% %6d.%03d %8d",
                 scale(ticks, total), msec / 1000, msec % 1000, ticks);
      out->fill_to(36);
      out->print_cr("%s", title);
    }
  }

  void print(Stream* out, int total) {
    print_ticks(out, "Unknown",           unknown,          total);
    print_ticks(out, "Native",            java_native,      total);
    print_ticks(out, "Compiler",          runtime_compiler, total);
    print_ticks(out, "Garbage collector", runtime_gc,       total);
  }
};

class ProfilerNode {
 private:
  int ticks_in_interpeted;
  int ticks_in_compiled;
 public:
  int task_id;
  ProfilerNode* _next;

  ProfilerNode(int id) {
    ticks_in_interpeted = 0;
    ticks_in_compiled   = 0;
    task_id             = id;
    _next               = NULL;
  }

  void update(bool is_compiled) {
    if (is_compiled) {
      ticks_in_compiled++;
    } else {
      ticks_in_interpeted++;
    }
  }

  void merge(ProfilerNode* other) {
    ticks_in_interpeted += other->ticks_in_interpeted;
    ticks_in_compiled += other->ticks_in_compiled;
  }

  void print(Stream* out, int total) {
    int msec = 0, sec = 0;
    int ticks = total_ticks();

    if (ticks > 0) {
      msec = ticks * TickInterval;
      sec = msec / 1000;
      msec %= 1000;
    }
    
    out->print("%5.1f%% %6d.%03d %8d %8d",
	       scale(ticks, total), (unsigned int)sec, (unsigned int)msec,
               ticks_in_interpeted, ticks_in_compiled);
    out->fill_to(36);
    print_title_on(out);
    out->cr();
  }

  virtual bool match(MethodDesc* method, int id) { 
    (void)id; (void)method; return false; 
  }
  virtual bool match(int id) { (void)id; return true; }
  virtual void print_title_on(Stream* out) JVM_PURE_VIRTUAL_1_PARAM(out);
  virtual void oops_do(void do_oop(OopDesc**)) JVM_PURE_VIRTUAL_1_PARAM(do_oop);

  int total_ticks() {
    return ticks_in_interpeted + ticks_in_compiled;
  }

  void* operator new(size_t size);
  void operator delete(void* p);

  // for sorting
  static int compare(ProfilerNode** a, ProfilerNode** b) {
    if (*b == NULL && *a == NULL) return 0;
    if (*a == NULL)               return +1;
    if (*b == NULL)               return -1;
    return (*b)->total_ticks() - (*a)->total_ticks();
  }
};

class SumNode : public ProfilerNode {
 public:
  SumNode(int id) : ProfilerNode(id) {}
  virtual void print_title_on(Stream* out) { out->print("Total ticks executing Java"); }
};

class MethodNode : public ProfilerNode {
 public:
  MethodNode(MethodDesc* method, int id) : ProfilerNode(id) {
    GUARANTEE(method->is_method(), "must be method()");
    _method = method;
  }
 private:
  MethodDesc* _method;
  virtual void print_title_on(Stream* out) {
    UsingFastOops fast_oops;
    Method::Fast m = _method;

#ifndef PRODUCT
    m().print_name_on(out);
#else
    InstanceClass::Fast ic = m().holder();
    Symbol::Fast class_name = ic().name();
    class_name().print_symbol_on(out, true);
    out->print(".");

    Symbol::Fast name = m().name();
    name().print_symbol_on(out);
#endif
  }
  virtual void oops_do(void do_oop(OopDesc**)) {
    do_oop((OopDesc**) &_method);
  }
  virtual bool match(MethodDesc* method, int id) {
    return method == _method && id == task_id;
  }
  virtual bool match(int id) {
    return id < 0 || id == task_id;
  }
};

class FlatProfiler : public GlobalObj {
private:
  Ticks ticks;
  bool engaged;
  bool ready;

  /// Number of all nodes in node_table
  int num_nodes;

  /// A hashtable that translates Method -> ProfilerNode.
  ProfilerNode** node_table;

  /// Accumulates all ticks
  SumNode sum_node;

public:
  FlatProfiler();

  ~FlatProfiler() {
    clear(-1);
  }

  enum { TableSize = 1024 };

  void engage() {
    engaged = true;
  }

  void disengage() {
    engaged = false;
  }

  bool is_engaged() {
    return engaged;
  }

  bool is_ready() {
    return ready;
  }

  void tick(int delay_time_is_ms) {
    (void)delay_time_is_ms;
    if (!engaged) {
      return;
    }
    ready = true;

#if ENABLE_COMPILER
    if (Compiler::is_active()) {
      ticks.runtime_compiler++;
      return;
    }
#endif
    if (ObjectHeap::is_gc_active()) {
      ticks.runtime_gc++;
      return;
    }    
    if (_jvm_profiler_in_native_method) {
      ticks.java_native++;
    }
  }

  void update(ProfilerNode* node, bool is_compiled) {
    if (_jvm_profiler_in_native_method > 0) {
      ticks.java_native--;
    }

    sum_node.update(is_compiled);
    node->update(is_compiled);
  }

  void update_table(MethodDesc* method, bool is_compiled) {
    ProfilerNode* bucket = node_table[entry_for(method)];
    ProfilerNode* result;

#if ENABLE_ISOLATES
    int id = TaskContext::current_task_id();
#else
    int id = -1;
#endif

    if (bucket == NULL) {
      result = new MethodNode(method, id);
      num_nodes++;
      node_table[entry_for(method)] = result;
    } else {
      for(ProfilerNode* node = bucket; node; node = node->_next) {
        if (node->match(method, id)) {
          update(node, is_compiled);
          return;
        }
        bucket = node;
      }
      result = new MethodNode(method, id);
      num_nodes++;
      bucket->_next = result;
    }
    update(result, is_compiled);
  }

  void profile_method(MethodDesc* method, bool is_compiled) {
    update_table(method, is_compiled);
    ready = false;
  }

  int entry_for(MethodDesc* method) {
    GUARANTEE(method->profile_hash() >= 0,  "must be positive");
    return method->profile_hash() % TableSize;
  }

  void oops_do(void do_oop(OopDesc**));
  void clear(int id);

  void print(Stream* out, int id);

  ProfilerNode** flatten_and_sort(int id, int* size, ProfilerNode* sum_node);
};

void FlatProfiler::oops_do(void do_oop(OopDesc**)) {
  for (int index = 0; index < TableSize; index++) {
    for (ProfilerNode* node = node_table[index]; 
         node != NULL; node = node->_next) {
      node->oops_do(do_oop);
    }
  }
}

void FlatProfiler::clear(int id) {
  for (int index = 0; index < TableSize; index++) {
    for (ProfilerNode** node = &node_table[index]; *node != NULL;) {
      ProfilerNode* current = *node;
      if (current->match(id)) {
        *node = (*node)->_next;
        delete current;
        num_nodes--;
      } else {
        node = &current->_next;
      }
    }
  }

  if (id < 0) {
    FREE_GLOBAL_HEAP_ARRAY(node_table, "profiler table"); 
  }
}

typedef int (_cdecl *sort_function)(const void* token, const void* elem);

ProfilerNode** FlatProfiler::flatten_and_sort(int id, int* size,
                                              ProfilerNode* sum_node) {
  ProfilerNode** flat_table =
    NEW_GLOBAL_HEAP_ARRAY(ProfilerNode*, num_nodes, "flat table");

  int n = 0;

  // Flatten note_table[] into flat_table[]
  for (int index = 0; index < TableSize; index++) {
    for (ProfilerNode* p = node_table[index]; p != NULL; p = p->_next) {
      if (p->match(id)) {
        flat_table[n++] = p;
        sum_node->merge(p);
      }
    }
  }

  // Sort the table based on total ticks
  jvm_qsort(flat_table, n, sizeof(ProfilerNode*), 
            (sort_function) ProfilerNode::compare);

  *size = n;
  return flat_table;
}

void FlatProfiler::print(Stream* out, int id) {
#if ENABLE_ISOLATES
  TaskContext maybeSwitchTask;
#endif

  int size, total;
  SumNode local_sum(id);
  ProfilerNode** flat_table = flatten_and_sort(id, &size, &local_sum);
  if (id == -1) {
    total = sum_node.total_ticks() + ticks.total_ticks();
  } else {
    total = local_sum.total_ticks();
  }

  // Print the node table
  out->print_cr("-----------------------------------");
  for (int index = 0; index < size; index++) {
    ProfilerNode* node = flat_table[index];
#if ENABLE_ISOLATES      
    if (id == -1 && node->task_id > 0) {
      Universe::set_current_task(node->task_id);
    }
#endif
    node->print(out, total);
  }

  FREE_GLOBAL_HEAP_ARRAY(flat_table, "flat table"); 

  if (id == -1) {
    out->cr();
    out->print_cr("[Global tick summary]");
    sum_node.print(out, total);
    ticks.print(out, total);
  } else {
    local_sum.print(out, total);
  }
}

void* ProfilerNode::operator new(size_t size){
  return (ProfilerNode*)GlobalObj::malloc_bytes(size);
}

void ProfilerNode::operator delete(void* p) {
  GlobalObj::free_bytes(p);
}

FlatProfiler::FlatProfiler() : ticks(), sum_node(-1) {
  // Allocate the node table
  node_table = 
    NEW_GLOBAL_HEAP_ARRAY(ProfilerNode*, TableSize, "profiler table");

  // Clear the node table
  for (int index = 0; index < TableSize; index++) {
    node_table[index] = 0;
  }

  engaged = false;
  ready = false;
  num_nodes = 0;
  _jvm_profiler_in_native_method = 0;
}

//
// Profiler implementation
// Allocates a FlatProfiler and delegates all calls.
//

FlatProfiler* profiler = NULL;
Stream* profiler_output;
bool stream_created;

void Profiler::initialize() {
  profiler = new FlatProfiler;
  profiler_output = NULL;
  stream_created = false;
}

void Profiler::engage() {
  profiler->engage();
}

void Profiler::disengage() {
  if (profiler) {
    profiler->disengage();
  }
}

bool Profiler::is_ready() {
  return profiler && profiler->is_ready();
}

void Profiler::dispose() {
  if (profiler) {
    delete profiler;
  }
  profiler = NULL;
  if (stream_created) {
    delete profiler_output;
  }
}

void Profiler::tick(int delay_time_is_ms) {
  if (profiler != NULL && profiler->is_engaged()) {
    profiler->tick(delay_time_is_ms);
  }
}

void Profiler::print(Stream* out, int id) {
  if (profiler != NULL) {
    profiler->print(out, id);
  }
}

void Profiler::oops_do(void do_oop(OopDesc**)) {
  if (profiler != NULL) {
    profiler->oops_do(do_oop);
  }
}

void Profiler::profile_method(Method* method, bool is_compiled) {
  if (profiler != NULL) {
    MethodDesc* m = (MethodDesc*)(method->obj());
    profiler->profile_method(m, is_compiled);
  }
}

void Profiler::dump_and_clear_profile_data(int id) {
  if (profiler != NULL) {
    profiler->disengage();
    profiler->print(get_default_output_stream(), id);
    if (id < 0) {
      dispose();
    } else {
      profiler->clear(id);
      profiler->engage();
    }
  }
}

Stream* Profiler::get_default_output_stream() {
  static JvmPathChar filename[] = {
    'f','l','a','t','.','p','r','f',0
  };

  if (!profiler_output) {
#if defined(GBA)
    profiler_output = tty; // no filesystem here
#else
    profiler_output = new FileStream(filename, 200);
    stream_created = true;
#endif
    profiler_output->print_cr("[Statistical Profiling Information]");
    profiler_output->print_cr("Time %%    Seconds Interpret Compiled");
  }

  return profiler_output;
}

#endif
