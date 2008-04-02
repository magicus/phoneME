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
 * Task.cpp: MVM startup and shutdown routines.
 *
 * This file defines routines for instantiating multiple virtual machines.
 *
 */

#include "incls/_precompiled.incl"
#include "incls/_Task.cpp.incl"

HANDLE_CHECK(Task, is_task())

#if USE_BINARY_IMAGE_LOADER
void Task::link_dynamic(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  ObjArray::Fast path( app_classpath() );
  FilePath::Fast path0;
#if ENABLE_LIB_IMAGES
  int i; 
  for (i = 0; i < path().length(); i++) {
    path0 = path().obj_at(i);
    const bool status = ROM::link_dynamic(this, &path0 JVM_NO_CHECK);
    if( status ) {
      // the first classpath points to a valid bundle file, and was loaded
      // into the VM. Clear it so that ClassPathAccess won't treat it as
      // a JAR file anymore.
      path().obj_at_clear(i);
    } else {
      //IMPL_NOTE: consider whether this should be fixed! 
      break;
    }
  }
  ROM::set_romized_heap_marker(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
#else //!ENABLE_LIB_IMAGES
  if( path().length() > 0 ) {
    path0 = path().obj_at(0);
    const bool status = ROM::link_dynamic(this, &path0 JVM_NO_CHECK);
    if( status ) {
      // the first classpath points to a valid bundle file, and was loaded
      // into the VM. Clear it so that ClassPathAccess won't treat it as
      // a JAR file anymore.
      path().obj_at_clear(0);
    } else {
      ROMBundle::set_current(NULL);
      free_binary_images();
    }
  }
#endif //ENABLE_LIB_IMAGES
}
#endif //USE_BINARY_IMAGE_LOADER

#if ENABLE_ISOLATES
int     Task::_num_tasks;

#ifndef PRODUCT
int     Task::_num_tasks_stopping;
#endif
#if ENABLE_OOP_TAG
int     Task::_seq_num = 1;
int Task::current_task_seq(int id) {
  ObjArrayDesc *tlist = *(ObjArrayDesc **)Universe::task_list();
  if (tlist == NULL) {
    return _seq_num;
  }
  TaskDesc *t = (TaskDesc *)*tlist->obj_field_addr(tlist->header_size() + (id * sizeof(jobject)));
  if (t == NULL) {
    return _seq_num;
  }
  return *t->int_field_addr(FIELD_OFFSET(TaskDesc, _seq));
}
#endif

void Task::initialize() {
  _num_tasks = 0;
#ifndef PRODUCT
  _num_tasks_stopping = 0;
#endif
}

void Task::setup_task_mirror(JavaClass *klass JVM_TRAPS) {
  if (klass->is_instance_class()) {
    int static_field_size = ((InstanceClass*)klass)->static_field_size();
    int vtable_length = klass->vtable_length();

    klass->setup_task_mirror(static_field_size, vtable_length, 
                             false JVM_CHECK);
    ((InstanceClass*)klass)->initialize_static_fields();
  } else {
    klass->setup_task_mirror(0, 0, false JVM_NO_CHECK_AT_BOTTOM);
  }
}

void Task::setup_mirrors(JVM_SINGLE_ARG_TRAPS) {
  setup_task_mirror(Universe::object_class()  JVM_CHECK);
  setup_task_mirror(Universe::isolate_class() JVM_CHECK);

  setup_task_mirror(Universe::java_lang_Class_class() JVM_CHECK);
  setup_task_mirror(Universe::thread_class()          JVM_CHECK);
  setup_task_mirror(Universe::string_class()          JVM_CHECK);
  setup_task_mirror(Universe::throwable_class()       JVM_CHECK);
  setup_task_mirror(Universe::error_class()           JVM_CHECK);
}

void Task::fast_bootstrap(JVM_SINGLE_ARG_TRAPS) {
  setup_mirrors(JVM_SINGLE_ARG_CHECK);

  Universe::object_class        ()->bootstrap_initialize(JVM_SINGLE_ARG_CHECK);
  Universe::thread_class        ()->bootstrap_initialize(JVM_SINGLE_ARG_CHECK);
  Universe::java_lang_Class_class()->bootstrap_initialize(JVM_SINGLE_ARG_CHECK);
  Universe::isolate_class       ()->bootstrap_initialize(JVM_SINGLE_ARG_CHECK);
  Universe::string_class        ()->bootstrap_initialize(JVM_SINGLE_ARG_CHECK);
  Universe::throwable_class     ()->bootstrap_initialize(JVM_SINGLE_ARG_CHECK);
  Universe::error_class         ()->bootstrap_initialize(JVM_SINGLE_ARG_CHECK);

  // The System class must be initialized here. Otherwise if we call
  // String.getBytes(), among other things, before System is
  // initialized, it will cause a chain reaction and cause the
  // com.sun.cldc.i18n.Helper class to fail to initialize. See CR
  // 6371479: "init_classes_inited_at_build may cause system to fail to start"
  Universe::system_class()->initialize(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

bool Task::init_first_task(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  ObjArray::Fast task_list = Universe::task_list();
  GUARANTEE(!task_list.is_null(), "Task list didn't get initialized");
  Task::Fast task = task_list().obj_at(Task::FIRST_TASK);
  GUARANTEE(!task.is_null(), "Task 1 didn't get allocated");
  GUARANTEE(task.equals(Task::current()), "Must be in first task");

  fast_bootstrap(JVM_SINGLE_ARG_CHECK_0);

  IsolateObj::Fast isolate_obj =
      Universe::new_instance(Universe::isolate_class() JVM_CHECK_0);
  isolate_obj().set_memory_reserve(ReservedMemory);
  isolate_obj().set_memory_limit(TotalMemory);
  isolate_obj().assign_unique_id();
  ObjArray::Raw cp = Universe::make_strings_from_char_arrays(
      task().app_classpath() JVM_OZCHECK(cp));
  isolate_obj().set_app_classpath(&cp);
  isolate_obj().set_use_verifier(UseVerifier);

  task().set_primary_isolate_obj(isolate_obj());
  task().set_priority(PRIORITY_NORMAL);
  task().add_to_seen_isolates(&isolate_obj);
  task().set_status(TASK_STARTED);
  isolate_obj().set_api_access(1);
#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  // First task should have profile_id, which is set by JVM_SetProfile.
  task().set_profile_id(Universe::profile_id());
#endif

  return true;
}

ReturnOop Task::create_task(const int id, IsolateObj* isolate JVM_TRAPS) {
  UsingFastOops fast_oops;

  // Allocate and set java.lang.Thread mirror for current thread
  Thread::Fast new_thread = Thread::allocate(JVM_SINGLE_ARG_CHECK_0);
  {
    ThreadObj::Raw thread_obj =
      Universe::thread_class()->new_initialized_instance(
              Universe::thread_class(),  &new_thread JVM_CHECK_0);
    new_thread().set_thread_obj(&thread_obj);
    thread_obj().set_priority(ThreadObj::PRIORITY_NORMAL);
  }

  Task::Fast task = allocate_task(id JVM_CHECK_0);
  TaskContext temporarily_switch_task(id);

  ObjArray::Fast array;
  array = Universe::copy_strings_to_byte_arrays(isolate->hidden_packages()
                                                JVM_OZCHECK(array));
  task().set_hidden_packages(array);
  array = Universe::copy_strings_to_byte_arrays(isolate->restricted_packages()
                                                JVM_OZCHECK(array));
  task().set_restricted_packages(array);
  array = Universe::copy_strings_to_char_arrays(isolate->sys_classpath()
                                                JVM_OZCHECK(array));

  task().set_sys_classpath(&array);
  array = Universe::copy_strings_to_char_arrays(isolate->app_classpath()
                                                JVM_OZCHECK(array));
  task().set_app_classpath(&array);
  array = Universe::new_obj_array(ThreadObj::NUM_PRIORITY_SLOTS + 1
                                  JVM_OZCHECK(array));
  task().set_priority_queue(&array);
  task().set_priority(isolate->priority());
  IsolateObj::Fast primary_isolate = isolate->duplicate(
    JVM_SINGLE_ARG_OZCHECK(primary_isolate));
  task().set_primary_isolate_obj(&primary_isolate);
#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  task().set_profile_id(isolate->profile_id()); // Set current active profile
#endif
  
  GUARANTEE(!Universe::java_lang_Class_class()->is_null(),
             "Mirror class not initialized");

  new_thread().set_task_id(id);
  // start will only get an exception before adding thread to global list
  Scheduler::start(&new_thread JVM_CHECK_0);
  Scheduler::set_next_runnable_thread(&new_thread);
  return new_thread.obj();
  // We eventually continue below after switching threads
}

/* Then a miracle occurs...
 * Fast bootstrap some system classes so that statics are task local.
 */

void Task::start_task(Thread *thread JVM_TRAPS) {
  UsingFastOops fast_oops;

  Task::Fast task = Task::get_task(thread->task_id());
  task().set_status(TASK_STARTED);
  // do this first so we can unwind this task if there is an exception in
  // this function
  task().add_thread();

  fast_bootstrap(JVM_SINGLE_ARG_CHECK);

  // At this point the task is running. Notify the caller of Isolate.start()
  // to stop waiting.
  IsolateObj::Fast isolate_obj = task().primary_isolate_obj();
  isolate_obj().notify_all_waiters(JVM_SINGLE_ARG_CHECK);
  // Transfer the value from the instance field to the static field
  isolate_obj().set_api_access(isolate_obj().api_access_init());

#if USE_BINARY_IMAGE_LOADER
  task().link_dynamic(JVM_SINGLE_ARG_CHECK);
#endif

  task().init_classes_inited_at_build(JVM_SINGLE_ARG_CHECK);
  task().load_main_class(thread JVM_CHECK);
#if ENABLE_JAVA_DEBUGGER
  if (JavaDebugger::is_debug_isolate_option_on() &&
      isolate_obj().connect_debugger() != 0) {
    JavaDebugger::initialize_java_debugger_task(
          JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  }
#endif
}

int Task::allocate_task_id(JVM_SINGLE_ARG_TRAPS) {
  TaskList::Raw tlist = Universe::task_list();
  int i = FIRST_TASK;
  while( tlist().obj_at(i) != NULL ) {
    if( ++i >= MAX_TASKS ) {
      Throw::isolate_resource_error(too_many_running_isolates JVM_NO_CHECK);
      break;
    }
  }
  return i;
}

void Task::init_classes_inited_at_build(JVM_SINGLE_ARG_TRAPS) {
#if !ENABLE_PREINITED_TASK_MIRRORS  
  UsingFastOops fast;
  ObjArray::Fast list = Universe::inited_at_build();
  if (list.is_null()) {
    return;
  }
  const int len = list().length();
  InstanceClass::Fast klass;

  for (int i=0; i<len; i++) {    
    
    klass = list().obj_at(i);
    
    if (klass.not_null()) {
      klass().initialize(JVM_SINGLE_ARG_CHECK);
    }
  }  
#endif
}

ReturnOop Task::allocate_task(int id JVM_TRAPS) {
  // Now, we've got everything to setup the new task atomically to others.
  UsingFastOops fast_oops;

  Task::Fast task = Universe::new_task(id JVM_CHECK_0);
  task().set_status(TASK_NEW);

  task().set_task_id(id);
#if ENABLE_OOP_TAG
  task().set_seq(++_seq_num);
#endif
  // set exit code and reason to default.
  //  task().set_exit_code(0);
  task().set_exit_reason(IMPLICIT_EXIT);

  Universe::task_list()->obj_at_put(id, &task);
  _num_tasks++;
  if (TraceGC) {
    TTY_TRACE(("Task: new task id %d, class_list 0x%x, mirror_list 0x%x",
               id, (int)task().class_list(),
               (int)task().mirror_list()));
#if ENABLE_OOP_TAG
    TTY_TRACE_CR((", seq %d", task().seq()));
#else
    TTY_TRACE_CR((""));
#endif
  }
  return task.obj();
}

// Upcall the stopping method of an isolate to set it to the stopping state
// and produce stopping events.
void Task::forward_stop(int ecode, int ereason JVM_TRAPS) {
  GUARANTEE(status() != TASK_NEW, "stopping unstarted task");
  if (status() == TASK_STARTED) {
    stop(ecode, ereason JVM_NO_CHECK_AT_BOTTOM);
  }
}

void Task::stop(int exit_code, int exit_reason JVM_TRAPS) {
  set_status(TASK_STOPPING);
  set_exit_code(exit_code);
  set_exit_reason(exit_reason);
#ifndef PRODUCT
  _num_tasks_stopping++;
#endif
  // Here, we first post termination signal to all thread of the stopping
  // isolate. The first thread to pick up the signal will change the state
  // of the isolate and produce the stopping event. To detect that thread,
  // we only have to check if the isolate state is TASK_STOPPING.
  // We can't check for exception because wake_up_term... may set
  // termination object into current thread.  We'll kill this thread
  // momentarily in any case if it is in this task. Otherwise the 
  // exception will propagate to caller.
  Scheduler::wake_up_terminated_sleepers(task_id() JVM_NO_CHECK);
  set_terminating();
  
  // we have to set it here, as otherwise current thread can continue
  // execution and enter into permanent wait()/sleep(), thus never got woken up
  // and never have a chance to terminate. First check to see if this task
  // owns the current thread
  if (Thread::current()->task_id() == task_id()) {
    Thread::set_current_pending_exception(Task::get_termination_object());
  }

  // Here we set the special thread back to NULL just in case it was set.
  clear_special_thread();
}

void Task::cleanup_terminated_task(int id JVM_TRAPS) {
  JVM_IGNORE_TRAPS;

  if (GenerateROMImage || Universe::is_bootstrapping()) {
    return;
  }

#ifdef AZZERT  
  // Need to guarantee that this task is not held by a handle somewhere
  { // No handles here
    Task* const task = (Task*)get_task(id);
    for( Task* o = (Task *)_last_handle; o; o = (Task*)o->previous() ) {
      GUARANTEE(!task->equals(o), "Task held in handle");
    }
  }
#endif

  {
    UsingFastOops fast_oops;
    Task::Fast task = get_task(id);
    // The isolate is already in "STOPPED" state at this point.
    GUARANTEE(task().status()>=TASK_STOPPED && task().thread_count()==0,
              "Invalid state for task cleanup");
#ifndef PRODUCT
    int exitreason = task().exit_reason();
    if (exitreason != IMPLICIT_EXIT && exitreason != UNCAUGHT_EXCEPTION) {
      _num_tasks_stopping--;
    }
#endif
    TaskList::Raw tlist = Universe::task_list();
    GUARANTEE(!tlist.is_null(), "No task list");

    if (TraceGC) {
      TTY_TRACE(("Task: cleanup task %d", id));
#if ENABLE_OOP_TAG
      TTY_TRACE_CR((", seq %d", task().seq()));
#else
      TTY_TRACE_CR((""));
#endif
      TTY_TRACE_CR(("Task: class_list 0x%x, mirror_base 0x%x",
                    (int)task().class_list(), (int)task().mirror_list()));
    }

    JarFileParser::flush_caches();
    ObjectHeap::on_task_termination(task);
    tlist().obj_at_clear(id);

    _num_tasks--;

    // _current_thread == NULL means the current task is being terminated,
    // _current_thread != NULL means the newly created task is being cleaned
    if (_current_thread == NULL) {
      // at this point we must switch the current_task_id to an active task if
      // one exists.  If no tasks exist at this point the VM will exit soon
      // after we return to lightweight_thread_exit
      const int len = tlist().length();
      for( int i = 0;; ) {
        if( ++i >= len ) {
          // Last task has exited, we are only left with the system task
          // Restore the system_class_list and
          // system_mirror_list so the VM can exit gracefully
          *Universe::class_list() = Universe::system_class_list();
          *Universe::mirror_list() = Universe::system_mirror_list();
          *Universe::current_dictionary() = Universe::system_dictionary();
          StringTable::current()->set_null();
          SymbolTable::current()->set_null();
          RefArray::current()->set_null();
          Task::current()->set_null();
          _current_task = NULL;
          Universe::update_relative_pointers();
          break;
        }
        if( tlist().obj_at(i) != NULL ) {
          Universe::set_current_task(i);
          ObjectHeap::on_task_switch(i);
          break;
        }
      }
    }
#if ENABLE_INTERPRETATION_LOG
    Universe::reset_interpretation_log();
#endif
    // Release any lock_obj entries that belong to this task.
    // This would happen if we created hash codes for strings
    GUARANTEE(id != 0, "Trying to release locks for task 0");
    Synchronizer::release_locks_for_task(id);

    Verifier::flush_cache();

#ifdef AZZERT
    // At this point there should be no root that will reference
    // the task we are ending.
#endif
  }
  // 'task' handle is now destroyed so no references to binary image
#if defined(AZZERT) || USE_BINARY_IMAGE_LOADER
  ObjectHeap::full_collect(JVM_SINGLE_ARG_NO_CHECK);
#if ENABLE_ISOLATES
  GUARANTEE( ObjectHeap::get_task_memory_usage(id) <= 
             BoundaryDesc::allocation_size(), "Leftover objects" );
#endif
#endif

#if ENABLE_COMPILER
  update_compilation_allowed();
#endif
}

void Task::cleanup_unstarted_task(int task_id) {
  TaskList::Raw tlist = Universe::task_list();
  GUARANTEE(!tlist.is_null(), "No task list");
  if (tlist().obj_at(task_id) != NULL) {
    tlist().obj_at_clear(task_id);
    _num_tasks--;
  }
}

void Task::terminate_current_isolate(Thread *thread JVM_TRAPS) {
  thread->must_be_current_thread();
  GUARANTEE(status() == TASK_STOPPING || exit_reason() == IMPLICIT_EXIT,
            "Task must be stopping or doing implicit exit");
  set_status(TASK_STOPPED);

#if ENABLE_COMPILER
  {
    // if we own some suspended compilation - clear it
    const CodeGenerator* gen = _compiler_code_generator;
    if( gen && gen->task_id() == current_id() ) {
      Compiler::abort_suspended_compilation();
    }
  }
#endif

  UsingFastOops fast_oops;
  IsolateObj::Fast isolate_obj = primary_isolate_obj();
  isolate_obj().mark_equivalent_isolates_as_terminated(JVM_SINGLE_ARG_CHECK);

  // This is a bit extreme. We could just flush the caches associated
  // with the current task, but most likely this doesn't matter.  
  JarFileParser::flush_caches();

#if ENABLE_JAVA_DEBUGGER
  if (_debugger_active) {
    UsingFastOops fastoops;
    Transport::Fast t = transport();
    if (!t.is_null()) {
      JavaDebugger::close_java_debugger(&t);
    }
  }
#endif
}

void Task::suspend() {
  set_status(status() | SUSPEND_STATUS);
  Task::suspend_task(task_id());
}

void Task::resume() {
  set_status(status());  // status() clears suspend bit
  Task::resume_task(task_id());
}

bool Task::is_suspended() {
  return (raw_status() & SUSPEND_STATUS) != 0;
}

bool Task::is_valid_task_id(int task_id) {
  TaskList::Raw tlist = Universe::task_list();
  return tlist.not_null() && task_id >= 0 && task_id < tlist().length() && 
    Task::get_task(task_id) != NULL;
}

void Task::suspend_task(int task_id) {
  Scheduler::suspend_threads(task_id);
}

void Task::resume_task(int task_id) {
  Scheduler::resume_threads(task_id);
}

bool Task::is_suspended_task(int task_id) {
  Task::Raw task = get_task(task_id);
  if (task.is_null()) {
    return false;
  }
  return task().is_suspended();
}

// Load the main class into the EntryActivation list of the new thread.
bool Task::load_main_class(Thread *new_thread JVM_TRAPS) {

  UsingFastOops fast_oops;
  String::Fast s = main_class();
  Symbol::Fast class_name_symbol =
    SymbolTable::slashified_symbol_for(&s JVM_CHECK_0);
  InstanceClass::Fast klass =
    SystemDictionary::resolve(&class_name_symbol, ErrorOnFailure JVM_CHECK_0);

  // Make sure the class is initialized
  klass().initialize(JVM_SINGLE_ARG_CHECK_0);

  // Find the method to invoke
  Method::Fast main_method =
    klass().lookup_method(Symbols::main_name(),
                          Symbols::string_array_void_signature());
  if (main_method.is_null() || !main_method().is_static()) {
    Throw::no_such_method_error(JVM_SINGLE_ARG_THROW_0); // See CR 6270554.

  }

  JVM::development_prologue();

#if ENABLE_METHOD_TRAPS
  MethodTrap::activate_initial_traps();
#endif

  // Create a delayed activation for the main method
  EntryActivation::Fast entry =
    Universe::new_entry_activation(&main_method, 1 JVM_CHECK_0);
  ObjArray::Raw arguments = args();
  entry().obj_at_put(0, &arguments);
  new_thread->append_pending_entry(&entry);

  return true;
}

ReturnOop Task::get_task(int id) {
  TaskList::Raw tlist = Universe::task_list();
  GUARANTEE(!tlist.is_null(), "Task must exist");
  GUARANTEE(id < tlist().length(), "Task ID out of range");
  return tlist().obj_at(id);
}

ReturnOop Task::main_class() {
  IsolateObj::Raw isolate_obj = primary_isolate_obj();
  return isolate_obj().main_class();
}

ReturnOop Task::args() {
  IsolateObj::Raw isolate_obj = primary_isolate_obj();
  return isolate_obj().main_args();
}

int Task::get_priority(int task_id) {
  Task::Raw task = get_task(task_id);
  GUARANTEE(!task.is_null(), "Scheduler called for null task");
  return task().priority();
}

void Task::add_thread() {
  set_thread_count(thread_count() + 1);
}

bool Task::remove_thread() {
  set_thread_count(thread_count()-1);
  if (thread_count() == 0){
    // Freeze any subsequent thread startup for this isolate and initiate
    // STOPPING.
    if (status()==TASK_STARTED){
      // if status wasn't set, this means that we're either
      // in the last thread of the isolate. This one is either
      // implicitly exiting, or was thrown a exception it didn't catch.
      GUARANTEE(exit_reason() == IMPLICIT_EXIT ||
                exit_reason() == UNCAUGHT_EXCEPTION,
                "invalid exit reason for task.");
      //      set_status(TASK_STOPPING); // WHY commented out??
    } else {
      // The isolate must have been stopped via call to System.exit().
      GUARANTEE(status()==TASK_STOPPING, "wrong task's state");
    }
    // true == this is last thread to terminate
    return true;
  } else {
    return false;
  }
}

ReturnOop Task::get_termination_object() {
  // This is the same uncatchable "exception" object as used by
  // Throw::uncatchable()
  return *Universe::string_class();
}

ReturnOop Task::get_visible_active_isolates(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  ObjArray::Fast result =
      Universe::new_obj_array(_num_tasks-Task::FIRST_TASK JVM_CHECK_0);
  TaskList::Fast tlist = Universe::task_list();
  Task::Fast t;
  IsolateObj::Fast iso;
  int len = tlist().length();
  int n = 0;

  // Task 0 reserved for system use
  for (int id = Task::FIRST_TASK; id < len; id++) {
    t = tlist().obj_at(id);
    if (t.is_null()) {
      continue;
    }
    bool found = false;
    for (iso = this->seen_isolates(); iso.not_null(); iso = iso().next()) {
      if (iso().represents(&t)) {
        found = true;
        break;
      }
    }

    if (!found) {
      iso = t().primary_isolate_obj();
      iso = iso().duplicate(JVM_SINGLE_ARG_CHECK_0);
      add_to_seen_isolates(&iso);
    }

    GUARANTEE(n+Task::FIRST_TASK < _num_tasks, "sanity");
    result().obj_at_put(n, &iso);
    n++;
  }

  GUARANTEE(n+Task::FIRST_TASK == _num_tasks, "All isolates should be found");

  return result.obj();
}

void Task::set_hint(int hint, int /*param*/) {
#if ENABLE_COMPILER
  int count = startup_phase_count();

  switch (hint) {
  case JVM_HINT_BEGIN_STARTUP_PHASE:
    count++;
    break;
  case JVM_HINT_END_STARTUP_PHASE:
    count--;
    GUARANTEE(count >= 0, "sanity");
    break;
  default:
    SHOULD_NOT_REACH_HERE();
  }

  set_startup_phase_count(count);
  update_compilation_allowed();
#endif
}

#if ENABLE_COMPILER
void Task::update_compilation_allowed( void ) {
  bool enable_compilation = true;

  TaskList::Raw tlist = Universe::task_list();
  const int len = tlist().length();
  for( int i = Task::FIRST_TASK; i < len; i++ ) {
    Task::Raw task = tlist().obj_at(i);
    if( task().not_null() && task().startup_phase_count() ) {
      enable_compilation = false;
      break;
    }
  }

  Universe::set_compilation_allowed( enable_compilation );
}
#endif

extern "C" int JVM_CurrentIsolateID() {
  return TaskContext::current_task_id();
}

extern "C" int JVM_MaxIsolates() {
  // IMPL_NOTE: This will change as soon as we removed
  // the hard limit of 8 isolates.
  return MAX_TASKS;
}

extern "C" jboolean JVM_SuspendIsolate(int isolate_id) {
  if (!Task::is_valid_task_id(isolate_id)) {
    return KNI_FALSE;
  }

  Task::Raw task = Task::get_task(isolate_id);

  if (task().raw_status() == Task::TASK_STARTED) {
    task().suspend();
    return KNI_TRUE;
  }

  return KNI_FALSE;
}

extern "C" jboolean JVM_IsIsolateSuspended(int isolate_id) {
  if (!Task::is_valid_task_id(isolate_id)) {
    return KNI_FALSE;
  }

  Task::Raw task = Task::get_task(isolate_id);

  return task().is_suspended();
}

extern "C" jboolean JVM_ResumeIsolate(int isolate_id) {
  if (!Task::is_valid_task_id(isolate_id)) {
    return KNI_FALSE;
  }

  Task::Raw task = Task::get_task(isolate_id);

  if (task().is_suspended()) {
    task().resume();
    return KNI_TRUE;
  }

  return KNI_FALSE;
}

#endif // ENABLE_ISOLATES

#if USE_BINARY_IMAGE_LOADER

#if ENABLE_LIB_IMAGES
#if ENABLE_ISOLATES //there are no shared images in SVM case
void Task::remove_shared_images( void ) const {
  ObjArray::Raw images = binary_images();
  if (images.is_null()) return;
  for( int i = 0; i < images().length(); i++ ) {
    ROMBundle* bundle = (ROMBundle*)images().obj_at(i);
    if (bundle->remove_if_not_currently_shared()) {
      images().obj_at_clear(i);
    }
  }
}
#endif //ENABLE_ISOLATES
void Task::free_binary_images( void ) const {
  AllocationDisabler no_allocations_here;
  ObjArray::Raw images = binary_images();
  if( images.is_null() ) {
    return;
  }
  const int length = images().length();
  for( int i = 0; i < length; i++ ) {
    ROMBundle* bundle = (ROMBundle*)images().obj_at(i);
    if (bundle == NULL) continue;
#if ENABLE_ISOLATES //there are no shared images in SVM case
    Task:Raw task = obj();
    if (!bundle->is_shared(&task)) 
#endif //ENABLE_ISOLATES 
    {
      bundle->remove_from_global_binary_images();
    }
    images().obj_at_clear(i);
  }
}

#else //ENABLE_LIB_IMAGES
void Task::free_binary_images( void ) const {
  AllocationDisabler no_allocations_here;
  ObjArray::Raw images = binary_images();
  if( images.is_null() ) {//no binary images loaded in this task
    return;
  }
  const int length = images().length();
  {
    GUARANTEE( length < 2, "Multiple bundles per task are not enabled" );
    for( int i = 0; i < length; i++ ) {
      ROMBundle* bundle = (ROMBundle*)images().obj_at(i);
      bundle->remove_from_global_binary_images();
      if( ROMBundle::current() == bundle ) {
        ROMBundle::set_current(NULL);
      }
      bundle->free();
    }
  }
#if USE_IMAGE_MAPPING
  {
    TypeArray::Raw handles = mapped_image_handles();    
    for( int i = 0; i < length; i++ ) {      
      OsFile_MappedImageHandle handle = 
          (OsFile_MappedImageHandle)handles().int_at(i);      
      OsFile_UnmapImage(handle);
    }
  }
#endif
}
#endif //!ENABLE_LIB_IMAGES

#if ENABLE_LIB_IMAGES
int Task::decode_reference(int o_ref) {
  int ref = o_ref;
  ObjArray::Raw images = binary_images();
  //for now we always expand binary_images by one item
  int bundle_count = images().length(); 
  GUARANTEE(images().obj_at(images().length() - 1) != NULL, 
        "for now we always expand binary_images by one item");
  unsigned int bundle_num = (bundle_count - (((unsigned int)ref) >> 24)) - 1;
  GUARANTEE(bundle_num <= bundle_count, "sanity");
  unsigned int bundle_offset = ref & 0x00fffffc;    
  return ((int)images().obj_at(bundle_num)) + bundle_offset;
}

int Task::encode_reference(int ref) {
  ObjArray::Raw list = binary_images();          
  GUARANTEE(list.not_null(), "ref must be from binary image");
  GUARANTEE(!(ref & 0x3), "ref must be from binary image");
  int bundle_count = 0;
  unsigned int bundle_num = 0xffffffff; //not set
  unsigned int bundle_offset = 0xffffffff; //not set
  for (; bundle_count < list().length(); bundle_count++) {
    ROMBundle* bundle = (ROMBundle*)list().obj_at(bundle_count);    
    int* rom_start = bundle->text_block();
    int* rom_end = bundle->text_block() + bundle->text_block_size();
    if ((rom_start <= (int*)ref) && ((int*)ref < rom_end)) {
      GUARANTEE((bundle_num == 0xffffffff) && (bundle_offset == 0xffffffff), 
        "this is second image which contains this ref!");
      bundle_num = bundle_count;
      bundle_offset = ((int*)ref - rom_start)*sizeof(int);
#ifndef AZZERT
      break;
#endif
    }
    if (bundle == NULL) {
      break;
    }        
  }
            
  GUARANTEE(bundle_num < 0xff, "we have only 8 bits to encode it");
  GUARANTEE(bundle_offset < 0xffffff, "we have only 24 bits to encode it");
  GUARANTEE((bundle_offset & 0x3) == 0, "it must be valid offset");  
  return bundle_offset | ((bundle_count - bundle_num) << 24);
}

void Task::add_binary_image(ROMBundle* bun JVM_TRAPS) {
  UsingFastOops fast;
  ObjArray::Fast old_binary_images = binary_images();
  ObjArray::Fast binary_images;
  if (old_binary_images.not_null()) {    
    binary_images = Universe::new_obj_array(old_binary_images().length() + 1 JVM_CHECK);
    int i;
    for ( i = 0; i < old_binary_images().length(); i++) {
      binary_images().obj_at_put(i, old_binary_images().obj_at(i));
    }
    binary_images().obj_at_put(i, (OopDesc*)bun);
  } else {
    //IMPL_NOTE:this could be optimized, so we allocate array enough to keep all bundles!
    binary_images = Universe::new_obj_array(1 JVM_CHECK);
    binary_images().obj_at_put(0, (OopDesc*)bun);    
  }
  set_binary_images(&binary_images);
}
#endif //ENABLE_LIB_IMAGES
#if USE_IMAGE_MAPPING && !ENABLE_LIB_IMAGES
void Task::add_binary_image_handle(void* image_handle JVM_TRAPS) {
  TypeArray::Raw new_img_handles = Universe::new_int_array(1 JVM_CHECK);
  new_img_handles().int_at_put(0, (int)image_handle);
  set_mapped_image_handles(&new_img_handles);  
}
#endif //IMAGE_MAPPING && !ENABLE_LIB_IMAGES
#endif // USE_BINARY_IMAGE_LOADER
#if ENABLE_ISOLATES
bool Task::is_restricted_package(const char *name, int pkg_len) {
  ObjArray::Raw restricted_packages_names = restricted_packages();
  if (restricted_packages_names.is_null()) return false;
  for (int i = 0; i < restricted_packages_names().length(); i++) {
    TypeArray::Raw value = restricted_packages_names().obj_at(i);
    char* rp = (char*)value().base_address();
    int len = value().length();
    char* rp2 = rp + (len - 2);
    if(jvm_strncmp(rp2, "/" "*", 2) == 0) {
      if(pkg_len < (len-2)) continue;
      if(jvm_strncmp(rp, name, pkg_len) == 0) {
        return true;
      }
    }
    if (len == pkg_len) {
      if (jvm_memcmp(rp, name, pkg_len) == 0) {
        return true; // we have a match. The package is restricted.
      }
    }
  }
  return false;
}

bool Task::is_hidden_class(Symbol* checking_name) {
  ObjArray::Raw hidden_packages_names = hidden_packages();
  if (hidden_packages_names.is_null()) return false;
  char* checking = checking_name->base_address();
  int checking_len = checking_name->strrchr('/');
  if (checking_len <= 0) {
    checking_len = checking_name->length();
  }

  for (int i = 0; i < hidden_packages_names().length(); i++) {
    TypeArray::Raw pkg_value = hidden_packages_names().obj_at(i);
    char* pkg = (char*)pkg_value().base_address();
    int pkg_len = pkg_value().length();    
    if (Universe::name_matches_pattern(checking, checking_len, pkg, pkg_len)) {
      return true;
    }
  }
  return false;  
}
#endif
#ifndef PRODUCT

void Task::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  iterate_oopmaps(BasicOop::iterate_one_oopmap_entry, (void*)visitor);
#endif
}

void Task::iterate_oopmaps(oopmaps_doer do_map, void *param) {
#if USE_OOP_VISITOR
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, app_classpath);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, sys_classpath);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, hidden_packages);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, restricted_packages);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, dictionary);

#if USE_BINARY_IMAGE_LOADER
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, binary_images);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, names_of_bad_classes);
#endif

#if USE_BINARY_IMAGE_LOADER && USE_IMAGE_MAPPING && !ENABLE_LIB_IMAGES
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, mapped_image_handles);
#endif

#if ENABLE_COMPILER && ENABLE_INLINE
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, direct_callers);
#endif

#if ENABLE_ISOLATES
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, special_thread);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, primary_isolate_obj);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, seen_isolates);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, transport);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, class_list);

  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, mirror_list);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, clinit_list);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, priority_queue);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, string_table);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, symbol_table);
#endif

  // End of all oops

  OOPMAP_ENTRY_4(do_map, param, T_INT,    string_table_count);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    symbol_table_count);
#if ENABLE_LIB_IMAGES && USE_BINARY_IMAGE_LOADER
  OOPMAP_ENTRY_4(do_map, param, T_INT   , classes_in_images);
#endif

#if ENABLE_ISOLATES
  OOPMAP_ENTRY_4(do_map, param, T_INT,    status);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    priority);

  OOPMAP_ENTRY_4(do_map, param, T_INT,    class_count);

  OOPMAP_ENTRY_4(do_map, param, T_INT,    exit_code);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    exit_reason);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    is_terminating);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    task_id);
  OOPMAP_ENTRY_4(do_map, param, T_INT,    thread_count);
#endif
#endif
}
#endif // PRODUCT
