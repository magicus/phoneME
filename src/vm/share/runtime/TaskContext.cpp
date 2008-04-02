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
# include "incls/_TaskContext.cpp.incl"

#if ENABLE_ISOLATES

// The global TaskContext instance
TaskContextSave TaskContextSave::_global_context;

#ifdef AZZERT
int TaskContextSave::_count = 0;
#endif


void TaskContextSave::init() {
#ifdef AZZERT
  level = _global_context._count++;
#endif
  _number_of_java_classes = 0;
  _prev_task_id = _global_context._current_task_id;
  _valid = false;
}

void TaskContextSave::dispose() {
#ifdef AZZERT
  _global_context._count--;
  GUARANTEE(level == _count, "Out of order context switch.");
#endif
  if (_prev_task_id != _global_context._current_task_id) {
    GUARANTEE(Universe::task_from_id(_prev_task_id), "task must be alive");
    TaskContext::set_current_task(_prev_task_id);
  }
}

void TaskContext::init(int task_id) {
  if (TraceTaskContext) {
    tty->print_cr("TC: %d", task_id);
  }

  GUARANTEE(!ObjectHeap::is_gc_active() ||
    task_id == _global_context._current_task_id, "Can't switch tasks during GC");
  _valid = true;
  set_current_task(task_id);
}

void TaskGCContext::init(const OopDesc* object) {
  if (ROM::system_contains(object)) {
    if (TraceTaskContext) {
      tty->print_cr("TGCOROM: 0x%x", (int)object);
    }
    _valid = true;
    return;
  }

  const unsigned task = ObjectHeap::owner_task_id( object );
  if( task < MAX_TASKS ) {
    if (TraceTaskContext) {
      tty->print_cr("TGCO: %p", object);
    }
    init( task );
  }
}

void TaskGCContext::set(const int task_id) {
  Task::Raw task = Universe::task_from_id(task_id);
  if( task.not_null() ) {
    _global_context._current_task_id = task_id;
    _global_context._number_of_java_classes = task().class_count();
    _class_list_base =((address)task().class_list() ) + ObjArray::base_offset();
    _mirror_list_base=((address)task().mirror_list()) + ObjArray::base_offset();
    _valid = true;

    if (TraceTaskContext) {
      tty->print_cr("TGC-set: %d 0x%x", task_id, (int)_class_list_base);
    }
  }
}


void TaskGCContext::init(const int task_id) {
  // Used during GC so that accesses to class_list references correct list
  if (TraceTaskContext) {
    tty->print_cr("TGC-init: %d", task_id);
  }
  set( task_id );
}

void TaskGCContext::dispose( void ) {
  if (TraceTaskContext) {
    tty->print_cr("TGC: dis");
  }
  set( _prev_task_id );
}

#if ENABLE_OOP_TAG
void TaskGCContextDebug::init(int class_id, int tag) {

  // Used during GC so that accesses to class_list references correct list
  if (TraceGC) {
    //    tty->print("TaskContextUser: class_id %d, tag 0x%x", class_id, tag);
  }
  if (class_id >= ROM::number_of_system_classes()) {
    Task::Raw task = Universe::task_from_id(tag & Oop::TASK_ID_MASK);
    if (task.not_null()) {
      if (task().seq() == (tag >> Oop::TASK_SEQ_SHIFT)) {
        _vaild = true;
        if (TraceGC) {
          //    tty->print("old: 0x%x, ", (int)_class_list_base);
        }
        _class_list_base = (address)task().class_list();
        _class_list_base += ObjArray::base_offset();
        if (TraceGC) {
          //          tty->print("new: 0x%x,", (int)_class_list_base);
        }
        _global_context._number_of_java_classes = task().class_count();
      }
    }
  }
  if (TraceGC) {
    //    tty->cr();
  }
}

void TaskGCContextDebug::dispose() {
  {
    Task::Raw task = Universe::task_from_id(_prev_task_id);
    if (task.not_null()) {
      if (TraceGC) {
        //tty->print("~TaskGCContextDebug: old: 0x%x, ",(int)_class_list_base);
      }
      _class_list_base = (address)task().class_list();
      _class_list_base += ObjArray::base_offset();
      if (TraceGC) {
        //        tty->print_cr("new: 0x%x, ", (int)_class_list_base);
      }
      _global_context._number_of_java_classes = task().class_count();
    }
  }
}
#endif

void TaskContext::set_current_task(int task_id) {
  if (TraceTaskContext) {
    tty->print_cr("TSC: %d, %d", task_id, _global_context._current_task_id);
  }
  Task::Raw task = Universe::task_from_id(task_id);
  *Universe::current_task_obj() = task.obj();

  if (task_id != _global_context._current_task_id) {
    Task::Raw prev_task = Universe::task_from_id(_global_context._current_task_id);
    if (prev_task.not_null()) {
      prev_task().set_class_count(_global_context._number_of_java_classes);
    }
    *Universe::class_list()         = task().class_list();
    *Universe::mirror_list()        = task().mirror_list();
    *Universe::current_dictionary() = task().dictionary();
    *StringTable::current()         = task().string_table();
    *SymbolTable::current()         = task().symbol_table();
    *RefArray::current()            = task().global_references();
    _global_context._number_of_java_classes = task().class_count();
    _current_task = task.obj();
    _global_context._current_task_id = task_id;
#if USE_BINARY_IMAGE_LOADER
    ROM::on_task_switch(task_id);
#endif
  }
  GUARANTEE(task.not_null(), "task must be alive");
  //update list bases
  _mirror_list_base = (address)task().mirror_list();
  _mirror_list_base += ObjArray::base_offset();
  _class_list_base = (address)task().class_list();
  _class_list_base += ObjArray::base_offset();
  if (TraceTaskContext) {
    tty->print_cr("TSC: 0x%x", (int)_class_list_base);
  }
#if USE_LARGE_OBJECT_AREA
  GUARANTEE(!LargeObject::contains((LargeObject*)_mirror_list_base),
            "Mirror list points into LargeObject area");
  GUARANTEE(!LargeObject::contains((LargeObject*)_class_list_base),
            "Class list points into LargeObject area");
  GUARANTEE(!LargeObject::contains((LargeObject*)Universe::current_dictionary()),
            "Dictionary list points into LargeObject area");
#endif
}

void TaskContext::set_class_list(ObjArray *cl) {
  *Universe::class_list() = cl->obj();
  _class_list_base = (address)cl->obj();
  _class_list_base += ObjArray::base_offset();
  if (TraceTaskContext) {
    tty->print_cr("TSC2: 0x%x", (int)_class_list_base);
  }
}

void TaskContext::set_number_of_java_classes(int number) {
  _global_context._number_of_java_classes = number;
  if (Task::current()->not_null()) {
    Task::current()->set_class_count(number);
  }
}
#else   //  ENABLE_ISOLATES

int TaskContext::_number_of_java_classes;

#endif  //  ENABLE_ISOLATES

