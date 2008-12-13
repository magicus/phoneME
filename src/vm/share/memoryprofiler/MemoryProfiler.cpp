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

# include "incls/_precompiled.incl"
# include "incls/_MemoryProfiler.cpp.incl"

#if ENABLE_MEMORY_PROFILER

#define CLASS_ID_OFFSET 0
#define TASK_ID_OFFSET 16
#define MISC_OFFSET    23

void *MemoryProfiler::memory_profiler_cmds[] = { 
  (void *)7, 
  (void *)MemoryProfiler::get_global_data,
  (void *)MemoryProfiler::retrieve_all_data, 
  (void *)MemoryProfiler::get_all_classes,
  (void *)MemoryProfiler::get_roots,
  (void *)MemoryProfiler::suspend_vm,
  (void *)MemoryProfiler::resume_vm,
  (void *)MemoryProfiler::get_stack_trace
};

OopDesc**           MemoryProfiler::_current_object = NULL;
PacketOutputStream* MemoryProfiler::_current_out = NULL;
OopDesc*            MemoryProfiler::_current_stack = NULL;
int                 MemoryProfiler::_stack_count = 0;

void MemoryProfiler::get_global_data(PacketInputStream *in,
                                     PacketOutputStream *out) {
  (void)in;
  out->write_int((int)(_heap_start));
  out->write_int((int)(_heap_top));
  out->write_int((int)(_old_generation_end));
  out->write_int((int)(_inline_allocation_top));
  out->send_packet();
}

void MemoryProfiler::get_all_classes(PacketInputStream *in,
                                     PacketOutputStream *out) {
  (void)in;
  UsingFastOops fast;
  int i = 0;
  int count = 0;
  JavaClass::Fast clazz;
  for (i = 0; i < ROM::number_of_system_classes(); i++) {
    clazz = Universe::class_from_id(i);
    if (!clazz().is_fake_class()) 
                           count++;
  }
#if ENABLE_ISOLATES
  TaskList::Fast tlist = Universe::task_list();
  const int len = tlist().length();
  int task_id = Task::FIRST_TASK;
  Task::Fast task;
  ObjArray::Fast classlist;
  for (task_id = Task::FIRST_TASK; task_id < len; task_id++) {
    task =  tlist().obj_at(task_id);
    if (task.is_null()) {
      continue;
    }
    i = ROM::number_of_system_classes();
    classlist = task().class_list();
    for (; i < classlist().length(); i++) {
      clazz = classlist().obj_at(i);
      if (clazz.is_null()) 
        continue;
      count++;
    }
  }
#else
  for (i = ROM::number_of_system_classes(); i < Universe::number_of_java_classes(); i++) {
    clazz = Universe::class_from_id(i);
    if (!clazz().is_fake_class()) 
                           count++;
  }
#endif

  out->write_int(count);
  
  for (i = 0; i < ROM::number_of_system_classes(); i++) {
    clazz = Universe::class_from_id(i);
    if (!clazz().is_fake_class()) {
      out->write_int(i);
      out->write_class_name(&clazz);
    }
  }
#if ENABLE_ISOLATES
  task_id = Task::FIRST_TASK;
  for (task_id = Task::FIRST_TASK; task_id < len; task_id++) {
    task = tlist().obj_at(task_id);
    if (task.is_null()) {
      continue;
    }
    i = ROM::number_of_system_classes();
    classlist = task().class_list();
    for (; i < classlist().length(); i++) {
      clazz = classlist().obj_at(i);
      if (clazz.is_null()) 
        continue;
      
      out->write_int(i | (task_id << 16));
      out->write_class_name(&clazz);
    }
  }   
#else
  for (i = ROM::number_of_system_classes(); i < Universe::number_of_java_classes(); i++) {
    clazz = Universe::class_from_id(i);
    if (!clazz().is_fake_class()) {
      out->write_int(i);
      out->write_class_name(&clazz);
    }
  }
#endif
  out->send_packet();
}

void MemoryProfiler::get_roots(PacketInputStream *in, PacketOutputStream *out) {
  (void)in;
  int i = 0;
  for (; i < Universe::__number_of_persistent_handles; i++) {
    out->write_int((int)persistent_handles[i]);
  }    
  out->write_int((juint)-1);
  out->send_packet();
}

static void do_nothing(OopDesc** /*p*/) {}
int MemoryProfiler::link_count;
void MemoryProfiler::retrieve_all_data(PacketInputStream *in,
                                       PacketOutputStream *out) {  
  (void)in;
  if (_current_object == NULL) {
    _current_object = _heap_start;

    //calculate number of java stacks
    _stack_count = 0;
    OopDesc* obj_iterator = (OopDesc*)_heap_start;
    while (obj_iterator < (OopDesc*)_inline_allocation_top) {
      Oop::Raw obj = obj_iterator;
      if (obj_iterator->is_execution_stack()) {
        _stack_count++;
      }
      obj_iterator = DERIVED( OopDesc*, obj_iterator, obj().object_size() );
    }
    if (_stack_count > Universe::mp_stack_list()->length()) {
      SETUP_ERROR_CHECKER_ARG;
      Oop::Raw new_mp_stack_list = Universe::new_obj_array(_stack_count JVM_NO_CHECK);
      if (new_mp_stack_list.is_null()) { //oom
        SHOULD_NOT_REACH_HERE();
        //IMPL_NOTE::how to handle this error????
      }
      *Universe::mp_stack_list() = new_mp_stack_list.obj();
    }
    _stack_count = 0;
  }
  _current_out = out;
  Scheduler::gc_prologue(do_nothing); //we need it to execute oops_do for Frames
  Oop obj;
  int currently_written_words = 0;
  while( _current_object < _inline_allocation_top) {
    obj = (OopDesc*)_current_object;
    link_count = 0;
    int link_factor = 1;
    if (obj.obj()->is_execution_stack()) {
      link_factor = 2; //we will write offset for each link!
      ExecutionStack ex_stack = obj.obj();
      Thread thrd = ex_stack.thread();      
      if (thrd.is_null()) {
        link_count = 0;
      } else {
        if (thrd.task_id() != Task::INVALID_TASK_ID) { 
          //this is terminated thread and we couldn't do oops_do for stack
          obj.obj()->oops_do(&link_counter);
        } else {
          link_count = 0;
        }
      }
    } else {
      obj.obj()->oops_do(&link_counter);
    }
    
    if (currently_written_words == 0 && 9 + link_factor*link_count > 900) {
      out->check_buffer_size((9 + link_factor*link_count)*sizeof(int)); //we must write at least this object
    } else if (currently_written_words + 9 + link_factor*link_count > 900) { //we don't have enough space in buffer
      break;
    }    
    currently_written_words += 4 + link_factor*link_count;
    dump_object(&obj);
    _current_object = DERIVED( OopDesc**, _current_object, obj.object_size() );
  }
  if (_current_object >= _inline_allocation_top) {
    _current_object = NULL;// we finished memory dumping
  }

  Scheduler::gc_epilogue();
  if (_current_object == NULL) {
    out->write_int((juint)-1);    
  } else {
    out->write_int((juint)-2);    
  }
  out->send_packet();
}

int MemoryProfiler::get_mp_class_id(JavaClass* clazz) {  
  int mp_class_id = clazz->class_id();
  GUARANTEE(!(mp_class_id & ~0xFFFF), "we are limited to 64k classes in system");
#if ENABLE_ISOLATES
  if (mp_class_id >= ROM::number_of_system_classes()) {
    TaskList::Raw tlist = Universe::task_list();    
    const int len = tlist().length();
    int task_id = Task::FIRST_TASK;
    for (task_id = Task::FIRST_TASK; task_id < len; task_id++) {
      Task::Raw task( tlist().obj_at(task_id) );
      if (task.is_null()) continue;
      ObjArray::Raw class_list = task().class_list();
      JavaClass::Raw task_klass = class_list().obj_at(mp_class_id);
      if (clazz->equals(task_klass)) {
        //we have less than 128 tasks now, but who knows future...
        GUARANTEE(!(task_id & ~0x7F), "we are limited to 128 tasks in system");
        mp_class_id |= (task_id << TASK_ID_OFFSET);
        break;
      }
    }
    GUARANTEE(mp_class_id & ~0xFFFF, "we must find class's task");
  }
#endif //ENABLE_ISOLATES
  return mp_class_id;
}

void MemoryProfiler::dump_object(Oop* p) {  
  GUARANTEE(ObjectHeap::contains(p->obj()), "sanity!");
  GUARANTEE(MemoryProfiler::_current_out != NULL, "sanity!");
  _current_out->write_int((int)p->obj());  
  FarClassDesc* const blueprint = (FarClassDesc*)p->blueprint();        
  _current_out->write_int(p->object_size());  
  if (p->is_instance() || p->is_obj_array() || p->is_type_array()) {
    JavaClass::Raw klass = blueprint;
    _current_out->write_int(get_mp_class_id(&klass));
#if ENABLE_ISOLATES
  } else if (p->is_task_mirror()) {    
    TaskMirror::Raw mirror = p->obj();
    JavaClass::Raw cls = mirror().containing_class();
    int class_id = get_mp_class_id(&cls);
    int object_id = class_id | (1 << MISC_OFFSET);
    _current_out->write_int(object_id);    
#endif
  } else if (p->obj()->is_execution_stack()) { 
    _current_out->write_int(2 << MISC_OFFSET);
    Universe::mp_stack_list()->obj_at_put(_stack_count, p->obj());
    _current_out->write_int(_stack_count++);
    _current_stack = p->obj();
  } else {
    _current_out->write_int(3 << MISC_OFFSET);
  }
  _current_out->write_int(link_count);
  if (link_count != 0) {
    if (p->obj()->is_execution_stack()) {
      p->obj()->oops_do(&stack_link_dumper);  
    } else {
      p->obj()->oops_do(&link_dumper);  
    }
  }
}

void MemoryProfiler::link_counter(OopDesc** p) {
  if (ObjectHeap::contains(p))
    link_count++;
}

void MemoryProfiler::link_dumper(OopDesc** p) {
  if (ObjectHeap::contains(p)) {
    MemoryProfiler::_current_out->write_int((int)*p);
  }
}

void MemoryProfiler::stack_link_dumper(OopDesc** p) {
  if (ObjectHeap::contains(p)) {
    MemoryProfiler::_current_out->write_int((int)*p);
    MemoryProfiler::_current_out->write_int((OopDesc*)p-_current_stack);
  }
}

void MemoryProfiler::suspend_vm(PacketInputStream *in, PacketOutputStream *out)
{
  (void)in;
  ThreadReferenceImpl::suspend_all_threads(-1, false);
  out->send_packet();
}

void MemoryProfiler::resume_vm(PacketInputStream *in, PacketOutputStream *out)
{
  (void)in;
  ThreadReferenceImpl::resume_all_threads(-1);  
  out->send_packet();
  JavaDebugger::set_loop_count(-1);
}

void MemoryProfiler::get_stack_trace(PacketInputStream *in, PacketOutputStream *out) {
  int stack_id = in->read_int();
  OopDesc* stack_address = Universe::mp_stack_list()->obj_at(stack_id);
  int offset = in->read_int();
  OopDesc** ptr_address = (OopDesc**)(stack_address + offset);
  if (stack_address->is_execution_stack()) {
    ExecutionStack::Raw stack = stack_address;
    Thread::Raw thrd = stack().thread();
    Frame frm(&thrd);
    Frame last_frm = frm;
    while (frm.fp() < (unsigned char*)ptr_address) {
      last_frm = frm;
      if (frm.is_java_frame()) {
        frm.as_JavaFrame().caller_is(frm);
      } else {
        if (frm.as_EntryFrame().is_first_frame()) {
          //write empty string here
          out->send_packet();
        }
        frm.as_EntryFrame().caller_is(frm);
      }
    }
    print_stack_trace(out, create_stack_trace(last_frm));
  } else { //problem here - we got wrong address
  }
  out->send_packet();
} 

ReturnOop MemoryProfiler::create_stack_trace(Frame frame) {
  UsingFastOops fast_oops;
  Frame pass1(frame);
  Frame pass2(frame);
  Method::Fast method;
  Symbol::Fast name;
  InstanceClass::Raw holder;

  int stack_size = 0;

  {
    FrameStream st(pass1);
    stack_size = 0;
    while (!st.at_end()) {
      st.next();
      stack_size++;
    }
  }

  // Allocate the trace
  SETUP_ERROR_CHECKER_ARG;
  ObjArray::Fast methods = Universe::new_obj_array(stack_size JVM_CHECK_0);
  TypeArray::Fast offsets = Universe::new_int_array(stack_size JVM_CHECK_0);
  ObjArray::Fast trace = Universe::new_obj_array(2 JVM_CHECK_0);
  trace().obj_at_put(0, &methods);
  trace().obj_at_put(1, &offsets);

  // Fill in the trace
  FrameStream st(pass2);
  int index;
  for (index = 0; index < stack_size; index++) {
    method = st.method();
    methods().obj_at_put(index, &method);
    offsets().int_at_put(index, st.bci());
    st.next();
  }

  return trace;
}

void MemoryProfiler::print_stack_trace(PacketOutputStream* out, OopDesc* backtrace) {
  
  if (backtrace == NULL) {
    out->write_byte(0); //empty string
    return;
  }
#if ENABLE_STACK_TRACE
  SETUP_ERROR_CHECKER_ARG;
  TypeArray::Raw buffer = Universe::new_byte_array(4000 JVM_NO_CHECK);
  int length = 0;
  if (buffer.is_null() && CURRENT_HAS_PENDING_EXCEPTION) {//oom
    out->write_byte(0); //empty string
    Thread::clear_current_pending_exception();//shall it be here?    
  }
       
  ObjArray::Raw trace = backtrace;
  ObjArray::Raw methods;
  TypeArray::Raw offsets;
  FixedArrayOutputStream stream((char*)buffer().byte_base_address(), 4000);	  
  methods = trace().obj_at(0);
  offsets = trace().obj_at(1);
  int i;
  for (i=0; i<methods().length(); i++) {
    Method::Raw m = methods().obj_at(i);
    int bci = offsets().int_at(i);
    if (m.is_null()) {
      break;
    }
    stream.print(" - ");

    InstanceClass::Raw ic = m().holder();
    Symbol::Raw class_name = ic().name();
    class_name().print_symbol_on(&stream, true);    
    
    stream.print(".");
  
    Symbol::Raw name = m().name();
#ifndef PRODUCT
    // Non-public methods in a romized image may be renamed to
    // .unknown. to save space. In non-product mode, to aid
    // debugging, we retrieve the original name using
    // ROM::get_original_method_name().
    if (name().equals(Symbols::unknown())) {
      name = ROM::get_original_method_name(&m);
    }
#endif
    name().print_symbol_on(&stream);
    stream.print_cr("(), bci=%d", bci);     
  } 
#endif //ENABLE_STACK_TRACE2
  out->write_int(stream.current_size());
  out->write_bytes(stream.array(), stream.current_size());
}

#endif //ENABLE_MEMORY_PROFILER
