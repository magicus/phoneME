/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
void *MemoryProfiler::memory_profiler_cmds[] = { 
  (void *)6, 
  (void *)MemoryProfiler::get_global_data,
  (void *)MemoryProfiler::retrieve_all_data, 
  (void *)MemoryProfiler::get_all_classes,
  (void *)MemoryProfiler::get_roots,
  (void *)MemoryProfiler::suspend_vm,
  (void *)MemoryProfiler::resume_vm

};

bool  MemoryProfiler::_gc_suspended = false;
OopDesc** MemoryProfiler::_current_object = NULL;
PacketOutputStream* MemoryProfiler::_current_out = NULL;

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
  out->write_int(-1);
  out->send_packet();
}

static void do_nothing(OopDesc** /*p*/) {}
int MemoryProfiler::link_count;
void MemoryProfiler::retrieve_all_data(PacketInputStream *in,
                                       PacketOutputStream *out) {  
  (void)in;
  if (_current_object == NULL) {
    _current_object = _heap_start;
  }
  _current_out = out;
  Scheduler::gc_prologue(do_nothing); //we need it to execute oops_do for Frames
  Oop obj;
  int currently_written_words = 0;
  while( _current_object < _inline_allocation_top) {
    obj = (OopDesc*)_current_object;
    link_count = 0;
    if (obj.obj()->is_execution_stack()) {
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
    
    if (currently_written_words == 0 && 9 + link_count > 900) {
      out->check_buffer_size((9 + link_count)*sizeof(int)); //we must write at least this object
    } else if (currently_written_words + 9 + link_count > 900) { //we don't have enough space in buffer
      break;
    }    
    currently_written_words += 4 + link_count;
    dump_object(&obj);
    _current_object = DERIVED( OopDesc**, _current_object, obj.object_size() );
  }
  if (_current_object >= _inline_allocation_top) {
    _current_object = NULL;// we finished memory dumping
  }

  Scheduler::gc_epilogue();
  if (_current_object == NULL) {
    out->write_int(-1);    
  } else {
    out->write_int(-2);    
  }
  out->send_packet();
}

int MemoryProfiler::get_mp_class_id(JavaClass* clazz) {  
  int mp_class_id = clazz->class_id();
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
        mp_class_id |= (task_id << 16);
        break;
      }
    }    
  }
#endif
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
  } else {
    _current_out->write_int(-1);
  }
  _current_out->write_int(link_count);
  if (link_count != 0) {
    p->obj()->oops_do(&link_dumper);  
  }
}

void MemoryProfiler::link_counter(OopDesc** p) {
  if (ObjectHeap::contains(p))
    link_count++;
}

void MemoryProfiler::link_dumper(OopDesc** p) {
  if (ObjectHeap::contains(p))
    MemoryProfiler::_current_out->write_int((int)*p);
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
#endif
