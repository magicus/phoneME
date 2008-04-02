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

/** class Task.hpp
 * MVM startup and shutdown routines.
 *
 * This file defines the interface for starting multiple instances of virtual
 * machines.
 *
 */

class Task: public MixedOop {
public:
  HANDLE_DEFINITION_CHECK(Task, MixedOop);

  static Task* current(void) {
    return Universe::current_task_obj();
  }

#if ENABLE_ISOLATES
  static int current_id(void) {
    return TaskContext::current_task_id();
  }
#endif

  static int app_classpath_offset(void) {
    return FIELD_OFFSET(TaskDesc, _app_classpath);
  }
  static int sys_classpath_offset(void) {
    return FIELD_OFFSET(TaskDesc, _sys_classpath);
  }
  static int hidden_packages_offset(void) {
    return FIELD_OFFSET(TaskDesc, _hidden_packages);
  }
  static int restricted_packages_offset(void) {
    return FIELD_OFFSET(TaskDesc, _restricted_packages);
  }
  static int dictionary_offset(void) {
    return FIELD_OFFSET(TaskDesc, _dictionary);
  }

  ReturnOop app_classpath(void) {
    return obj_field(app_classpath_offset());
  }
  void set_app_classpath(Oop* value) {
    obj_field_put(app_classpath_offset(), value);
  }
  void set_app_classpath(OopDesc* value) {
    obj_field_put(app_classpath_offset(), value);
  }

  ReturnOop sys_classpath(void) {
    return obj_field(sys_classpath_offset());
  }
  void set_sys_classpath(Oop* value) {
    obj_field_put(sys_classpath_offset(), value);
  }
  void set_sys_classpath(OopDesc* value) {
    obj_field_put(sys_classpath_offset(), value);
  }

  ReturnOop restricted_packages(void) const {
    return obj_field(restricted_packages_offset());
  }
  void set_restricted_packages(Oop* value) {
    obj_field_put(restricted_packages_offset(), value);
  }
  void set_restricted_packages(OopDesc* value) {
    obj_field_put(restricted_packages_offset(), value);
  }

  ReturnOop hidden_packages(void) const {
    return obj_field(hidden_packages_offset());
  }
  void set_hidden_packages(Oop* value) {
    obj_field_put(hidden_packages_offset(), value);
  }
  void set_hidden_packages(OopDesc* value) {
    obj_field_put(hidden_packages_offset(), value);
  }

  ReturnOop dictionary(void) const {
    return obj_field(dictionary_offset());
  }
  void set_dictionary(Oop* value) {
    obj_field_put(dictionary_offset(), value);
  }
  void set_dictionary(OopDesc* value) {
    obj_field_put(dictionary_offset(), value);
  }

#if USE_BINARY_IMAGE_LOADER
  static int binary_images_offset() {
    return FIELD_OFFSET(TaskDesc, _binary_images);
  }
  ReturnOop binary_images() const {
    return obj_field(binary_images_offset());
  }
  void set_binary_images(ObjArray *value) {
    obj_field_put(binary_images_offset(), value);
  }

  static int names_of_bad_classes_offset() {
    return FIELD_OFFSET(TaskDesc, _names_of_bad_classes);
  }
  ReturnOop names_of_bad_classes() const {
    return obj_field(names_of_bad_classes_offset());
  }

  void set_names_of_bad_classes(Oop *value) {
    obj_field_put(names_of_bad_classes_offset(), value);
  }
  void set_names_of_bad_classes(OopDesc *value) {
    obj_field_put(names_of_bad_classes_offset(), value);
  }

  void free_binary_images(void) const;
#if ENABLE_LIB_IMAGES
void Task::remove_shared_images( void ) const;
  static int classes_in_images_offset() {
    return FIELD_OFFSET(TaskDesc, _classes_in_images);
  }

  int get_shared_libs_num() {
    ObjArray::Raw list = binary_images();        
    if (list.not_null()) {
      int count = 0;
      for (; count < list().length(); count++) {
        if (list().obj_at(count) == NULL) {
          break;
        }        
      }
      return count;
    }
    return 0;
  }

  int classes_in_images() const {
    return int_field(classes_in_images_offset());
  }

  void set_classes_in_images(int classes_in_images) {
    int_field_put(classes_in_images_offset(), classes_in_images);
  }
  int encode_reference(int ref);
  int decode_reference(int ref);
#endif //ENABLE_LIB_IMAGES

#endif //USE_BINARY_IMAGE_LOADER

#if USE_BINARY_IMAGE_LOADER && USE_IMAGE_MAPPING && !ENABLE_LIB_IMAGES
  //in case of ENABLE_LIB_IMAGES we keep all handles in 
  //Universe::global_image_handles
  static int mapped_image_handles_offset(void) {
    return FIELD_OFFSET(TaskDesc, _mapped_image_handles);
  }
  ReturnOop mapped_image_handles(void) const {
    return obj_field(mapped_image_handles_offset());
  }
  void set_mapped_image_handles(TypeArray *value) {
    obj_field_put(mapped_image_handles_offset(), value);
  }
#endif

#if ENABLE_COMPILER && ENABLE_INLINE
  DEFINE_ACCESSOR_OBJ(Task, OopCons, direct_callers);
 public:
#endif

  void iterate(OopVisitor* /*visitor*/) PRODUCT_RETURN;
  static void iterate_oopmaps(oopmaps_doer /*do_map*/, void* /*param*/)
              PRODUCT_RETURN;

  static int string_table_count_offset() {
    return FIELD_OFFSET(TaskDesc, _string_table_count);
  }
  static int symbol_table_count_offset() {
    return FIELD_OFFSET(TaskDesc, _symbol_table_count);
  }
  unsigned string_table_count(void) const {
    return int_field(string_table_count_offset());
  }
  unsigned incr_string_table_count(void) {
    const int value = string_table_count()+1;
    int_field_put(string_table_count_offset(), value);
    return value;
  }
  unsigned symbol_table_count(void) const {
    return int_field(symbol_table_count_offset());
  }
  unsigned incr_symbol_table_count(void) {
    const int value = symbol_table_count()+1;
    int_field_put(symbol_table_count_offset(), value);
    return value;
  }

#if ENABLE_ISOLATES
  static void initialize();
private:
  // Offsets should be placed at the top of C++ class declaration to make
  // gcc 2.9.x generate better code.

  static int task_id_offset() {
    return FIELD_OFFSET(TaskDesc, _task_id);
  }

  static int thread_count_offset() {
    return FIELD_OFFSET(TaskDesc, _thread_count);
  }

  static int startup_phase_count_offset() {
    return FIELD_OFFSET(TaskDesc, _startup_phase_count);
  }

  static int exit_code_offset() {
    return FIELD_OFFSET(TaskDesc, _exit_code);
  }

  static int status_offset() {
    return FIELD_OFFSET(TaskDesc, _status);
  }

  static int priority_offset() {
    return FIELD_OFFSET(TaskDesc, _priority);
  }

  static int exit_reason_offset() {
    return FIELD_OFFSET(TaskDesc, _exit_reason);
  }

  static int special_thread_offset() {
    return FIELD_OFFSET(TaskDesc, _special_thread);
  }

  static int is_terminating_offset() {
    return FIELD_OFFSET(TaskDesc, _is_terminating);
  }

  static int primary_isolate_obj_offset() {
    return FIELD_OFFSET(TaskDesc, _primary_isolate_obj);
  }

  static int seen_isolates_offset() {
    return FIELD_OFFSET(TaskDesc, _seen_isolates);
  }

  static int class_count_offset() {
    return FIELD_OFFSET(TaskDesc, _class_count);
  }
#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  static int profile_id_offset() {
    return FIELD_OFFSET(TaskDesc, _profile_id);
  }
#endif

  static int class_list_offset() {
    return FIELD_OFFSET(TaskDesc, _class_list);
  }

  static int mirror_list_offset() {
    return FIELD_OFFSET(TaskDesc, _mirror_list);
  } 
public:
  static jint clinit_list_offset() {
    return FIELD_OFFSET(TaskDesc, _clinit_list);
  }
private:

  static int transport_offset(void) {
    return FIELD_OFFSET(TaskDesc, _transport);
  }

  static int priority_queue_offset(void) {
    return FIELD_OFFSET(TaskDesc, _priority_queue);
  }
  static int priority_queue_valid_offset(void) {
    return FIELD_OFFSET(TaskDesc, _priority_queue_valid);
  }
  static int last_priority_queue_offset(void) {
    return FIELD_OFFSET(TaskDesc, _last_priority_queue);
  }
  static int string_table_offset(void) {
    return FIELD_OFFSET(TaskDesc, _string_table);
  }
  static int symbol_table_offset(void) {
    return FIELD_OFFSET(TaskDesc, _symbol_table);
  }
  static int global_references_offset(void) {
    return FIELD_OFFSET(TaskDesc, _global_references);
  }
public:
#define  SUSPEND_STATUS  (unsigned(1)<<31)

  // Isolate's state -- must match the value in com.sun.cldc.isolate.Isolate.
  enum {
    TASK_NEW       = 1,    // created by the current isolate
    TASK_STARTED   = 2,    // start() method has been called --
                           // see IsolateEvent.STARTING
    TASK_STOPPING  = 3,    // isolate is stopping -- see IsolateEvent.STOPPING
    TASK_STOPPED   = 4     // isolate was terminated --
                           // see IsolateEvent.TERMINATED
  } State;

  // Exit reasons -- must match the code in
// com.sun.cldc.isolate.IsolateEvent.ExitReason
  enum {
    IMPLICIT_EXIT  = 1,      // Last thread of isolate exit normally
    SELF_EXIT      = 2,      // a thread of the isolate called
                             // System.exit / isolate.exit()
    SELF_HALT      = 3,      // a thread of the isolate called isolate.halt
    OTHER_EXIT     = 4,      // another isolate invoked isolate.exit
    OTHER_HALT     = 5,      // another isolate invoked isolate.halt
    UNCAUGHT_EXCEPTION = 6   // uncaught exception on the last thread of
                             // the isolate.
  };

  enum {
    SUSPEND         = 0,
    PRIORITY_MIN    = 1,
    PRIORITY_NORMAL = 2,
    PRIORITY_MAX    = 3
  } Priority;

  static int allocate_task_id(JVM_SINGLE_ARG_TRAPS);
  static ReturnOop allocate_task(int id JVM_TRAPS);
  static void fast_bootstrap(JVM_SINGLE_ARG_TRAPS);
  static void setup_task_mirror(JavaClass *ic JVM_TRAPS);
  static void setup_mirrors(JVM_SINGLE_ARG_TRAPS);
  static bool init_first_task(JVM_SINGLE_ARG_TRAPS);
  static ReturnOop create_task(const int id, IsolateObj* isolate JVM_TRAPS);
  static void start_task(Thread *thread JVM_TRAPS);

  void forward_stop(int ecode, int ereason JVM_TRAPS);
  void terminate_current_isolate(Thread *THREAD JVM_TRAPS);
  void stop(int exit_code, int exit_reason JVM_TRAPS);
  void set_hint(int hint, int param);

  void suspend();
  void resume();
  bool is_suspended();
  bool is_restricted_package(const char* name, int len);  
  bool is_hidden_class(Symbol* class_name);  

  static bool is_valid_task_id(int task_id);

  static void suspend_task(int task_id);
  static void resume_task(int task_id);

  static bool is_suspended_task(int task_id);

  bool load_main_class(Thread *thread JVM_TRAPS);

  int task_id( void ) const {
    return int_field(task_id_offset());
  }
  void set_task_id(int id) {
    int_field_put(task_id_offset(), id);
  }

  // obtaining startup state info
  ReturnOop args();
  ReturnOop main_class();


  int exit_code() const {
    return int_field(exit_code_offset());
  }
  void set_exit_code(int value) {
    int_field_put(exit_code_offset(), value);
  }

  int exit_reason() const {
    return int_field(exit_reason_offset());
  }
  void set_exit_reason(int value) {
    int_field_put(exit_reason_offset(), value);
  }

  int status() const {
    return (int_field(status_offset()) & ~SUSPEND_STATUS);
  }
  void set_status(int value) {
    int suspend = status() & SUSPEND_STATUS;
    int_field_put(status_offset(), value | suspend);
  }

  int raw_status() const {
    return int_field(status_offset());
  }

  int priority() const {
    return int_field(priority_offset());
  }
  void set_priority(int value) {
    int_field_put(priority_offset(), value);
  }

  int thread_count() const {
    return int_field(thread_count_offset());
  }
  void set_thread_count(int value) {
    int_field_put(thread_count_offset(), value);
  }

  int startup_phase_count() const {
    return int_field(startup_phase_count_offset());
  }
  void set_startup_phase_count(int value) {
    int_field_put(startup_phase_count_offset(), value);
  }

  ReturnOop special_thread( void ) const {
    return obj_field(special_thread_offset());
  }
  void clear_special_thread( void ) {
    obj_field_clear(special_thread_offset());
  }
  void set_special_thread(Oop* value) {
    obj_field_put(special_thread_offset(), value);
  }
  void set_special_thread(OopDesc* value) {
    obj_field_put(special_thread_offset(), value);
  }

  static void clinit(InstanceClass *ic JVM_TRAPS);

  // Clean up on task termination
  static void cleanup_terminated_task(int task_id JVM_TRAPS);
  static void cleanup_unstarted_task(int task_id);

  bool is_terminating( void ) const {
    return (int_field(is_terminating_offset()) != 0);
  }
  void set_terminating( void ) {
    int_field_put(is_terminating_offset(), 1);
  }
  void clear_terminating( void ) {
    int_field_put(is_terminating_offset(), 0);
  }

  int class_count() const {
    return int_field(class_count_offset());
  }

#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  int profile_id() const {
    return int_field(profile_id_offset());
  }

  void set_profile_id(int id) {
    int_field_put(profile_id_offset(), id);
  }
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT
  void set_class_count(const int id) {
    int_field_put(class_count_offset(), id);
  }

  ReturnOop class_list( void ) const {
    return obj_field(class_list_offset());
  }
  void set_class_list(Oop* value) {
    obj_field_put(class_list_offset(), value);
  }
  void set_class_list(OopDesc* value) {
    obj_field_put(class_list_offset(), value);
  }

  ReturnOop mirror_list( void ) const {
    return obj_field(mirror_list_offset());
  }
  void set_mirror_list(Oop* value) {
    obj_field_put(mirror_list_offset(), value);
  }
  void set_mirror_list(OopDesc* value) {
    obj_field_put(mirror_list_offset(), value);
  }

  ReturnOop clinit_list( void ) const {
    return obj_field(clinit_list_offset());
  }
  void set_clinit_list(Oop* value){
    obj_field_put(clinit_list_offset(), value);
  }
  void set_clinit_list(OopDesc* value){
    obj_field_put(clinit_list_offset(), value);
  }

  ReturnOop primary_isolate_obj( void ) const {
    return obj_field(primary_isolate_obj_offset());
  }
  void clear_primary_isolate_obj( void ) {
    obj_field_clear(primary_isolate_obj_offset());
  }
  void set_primary_isolate_obj(Oop* value) {
    obj_field_put(primary_isolate_obj_offset(), value);
  }
  void set_primary_isolate_obj(OopDesc* value) {
    obj_field_put(primary_isolate_obj_offset(), value);
  }

  ReturnOop seen_isolates( void ) const {
    return obj_field(seen_isolates_offset());
  }
  void clear_seen_isolates( void ) {
    obj_field_clear(seen_isolates_offset());
  }
  void set_seen_isolates(Oop* value) {
    obj_field_put(seen_isolates_offset(), value);
  }
  void set_seen_isolates(OopDesc* value) {
    obj_field_put(seen_isolates_offset(), value);
  }
  void add_to_seen_isolates(IsolateObj *iso) {
    IsolateObj::Raw seen = seen_isolates();
    iso->set_next(&seen);
    set_seen_isolates(iso);
  }

  ReturnOop transport(void) const {
    return obj_field(transport_offset());
  }
  void set_transport(Oop *value) {
    obj_field_put(transport_offset(), value);
  }

  ReturnOop priority_queue(void) const {
    return obj_field(priority_queue_offset());
  }
  void set_priority_queue(Oop* value) {
    obj_field_put(priority_queue_offset(), value);
  }
  void set_priority_queue(OopDesc* value) {
    obj_field_put(priority_queue_offset(), value);
  }

  ReturnOop priority_queue(const int priority) const {
    ObjArray::Raw oa = obj_field(priority_queue_offset());
    return oa().obj_at(priority);
  }
  void set_priority_queue(Oop* value, const int priority) {
    ObjArray::Raw oa = obj_field(priority_queue_offset());
    oa().obj_at_put(priority, value);
  }
  void set_priority_queue(OopDesc* value, const int priority) {
    ObjArray::Raw oa = obj_field(priority_queue_offset());
    oa().obj_at_put(priority, value);
  }

  int priority_queue_valid(void) const {
    return int_field(priority_queue_valid_offset());
  }
  static int priority_queue_valid(const int task_id) {
    Task::Raw task = get_task(task_id);
    GUARANTEE(!task.is_null(), "Task is null");
    return task().int_field(priority_queue_valid_offset());
  }
  static void set_priority_queue_valid(const int task_id, const int priority) {
    Task::Raw task = get_task(task_id);
    GUARANTEE(!task.is_null(), "Task is null");
    task().int_field_put(priority_queue_valid_offset(),
                         (task().priority_queue_valid() | (1 << priority)));
  }
  static void clear_priority_queue_valid(const int task_id, const int priority){
    Task::Raw task = get_task(task_id);
    GUARANTEE(!task.is_null(), "Task is null");
    task().int_field_put(priority_queue_valid_offset(),
                         (task().priority_queue_valid() &(~(1<<(priority)))));
  }
  void clear_all_priority_queue_valid(void) {
    int_field_put(priority_queue_valid_offset(), 0);
  }

  int last_priority_queue(void) const {
    return int_field(last_priority_queue_offset());
  }
  static void set_last_priority_queue(const int task_id, const int priority) {
    Task::Raw task = get_task(task_id);
    GUARANTEE(!task.is_null(), "Task is null");
    task().int_field_put(last_priority_queue_offset(), priority);
  }

  static int get_priority(int task_id);

  ReturnOop string_table(void) const {
    return obj_field(string_table_offset());
  }
  void set_string_table(Oop* value) {
    obj_field_put(string_table_offset(), value);
  }
  void set_string_table(OopDesc* value) {
    obj_field_put(string_table_offset(), value);
  }

  ReturnOop symbol_table(void) const {
    return obj_field(symbol_table_offset());
  }
  void set_symbol_table(Oop* value) {
    obj_field_put(symbol_table_offset(), value);
  }
  void set_symbol_table(OopDesc* value) {
    obj_field_put(symbol_table_offset(), value);
  }

  ReturnOop global_references(void) const {
    return obj_field(global_references_offset());
  }
  void set_global_references(Oop* value) {
    obj_field_put(global_references_offset(), value);
  }
  void set_global_references(OopDesc* value) {
    obj_field_put(global_references_offset(), value);
  }

  static int get_num_tasks() {
    return _num_tasks;
  }

  void add_thread();

  // return true if this was the last thread
  bool remove_thread();

  static ReturnOop get_termination_object();

  // Returns an ObjArray that contains Isolate objects that that represent
  // all currently active tasks. If a task is be represented by multiple
  // Isolate objects, this function returns only the Isolate object that's
  // visible to the current task.
  ReturnOop get_visible_active_isolates(JVM_SINGLE_ARG_TRAPS);

  void init_classes_inited_at_build(JVM_SINGLE_ARG_TRAPS);

#ifndef PRODUCT
  // for debugging purposes
  static int _num_tasks_stopping;
#endif
#if ENABLE_OOP_TAG
  static int _seq_num;
  static  int current_task_seq(int id);
  static int seq_offset() {
    return (FIELD_OFFSET(TaskDesc, _seq));
  }
  int seq() {
    return int_field(seq_offset());
  }
  void set_seq(int val) {
    int_field_put(seq_offset(), val);
  }
#endif
private:
  static void update_compilation_allowed();
  static int _num_tasks;
#endif
  // ENABLE_ISOLATES
public:
  enum {
    FIRST_TASK = 1,
    INVALID_TASK_ID = -1,
    NEW_TABLE_LEN = 64
  };
  static ReturnOop get_task(int id);

#if !ENABLE_ISOLATES
  static int task_id( void ) { return FIRST_TASK; }
#endif

#if USE_BINARY_IMAGE_LOADER
  void link_dynamic(JVM_SINGLE_ARG_TRAPS);
#if ENABLE_LIB_IMAGES
  void add_binary_image(ROMBundle* JVM_TRAPS);
#endif //ENABLE_LIB_IMAGES
#if USE_IMAGE_MAPPING
  void add_binary_image_handle(void* JVM_TRAPS);
#endif
#endif //USE_BINARY_IMAGE_LOADER
};
