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

class TaskDescPointers: public MixedOopDesc {
protected:
  ObjArrayDesc*     _app_classpath;
  ObjArrayDesc*     _sys_classpath;
  ObjArrayDesc*     _hidden_packages;
  ObjArrayDesc*     _restricted_packages;
  ObjArrayDesc*     _dictionary;

#if USE_BINARY_IMAGE_LOADER
  ObjArrayDesc*     _binary_images;
  ObjArrayDesc*     _names_of_bad_classes; // ObjArray
#endif

#if USE_BINARY_IMAGE_LOADER && USE_IMAGE_MAPPING && !ENABLE_LIB_IMAGES
  TypeArrayDesc*    _mapped_image_handles;
#endif

#if ENABLE_COMPILER && ENABLE_INLINE
  ObjArrayDesc*     _direct_callers;
#endif

#if ENABLE_ISOLATES
  // All information regarding a task can be found in the current isolate
  // object of that task, referenced from the static variable Isolate._current.
  // This static variable itself can be found from the TaskMirror object
  // reference from the embedded table in the Isolate class, at the offset
  // corresponding to the task id kept in this task object.
  OopDesc*          _special_thread;

  // A Task may be represented by multiple Isolate objects. 
  // _primary_isolate_obj is the Isolate object that's visible to this
  // Task.
  OopDesc*          _primary_isolate_obj;

  // List of all Isolate objects that have been 'seen' by this Task. I.e.,
  // This includes:
  //   - the primary isolate object
  //   - isolate objects that have been created by this Task
  //   - isolate objects that have been created by executing
  //     the Java method Isolate.isolates() in this Task.
  OopDesc*          _seen_isolates;

  
  OopDesc*          _transport;         // Used by JAVA_DEBUGGER
  OopDesc*          _class_list;
  OopDesc*          _mirror_list;
  OopDesc*          _clinit_list;       // list of class task mirror of 
                                        // classes being initialized by this
                                        // task.
  OopDesc*          _priority_queue;    // list of thread queues of this task
  OopDesc*          _string_table;      // string table for this task
  OopDesc*          _symbol_table;      // symbol table for this task
  OopDesc*          _global_references; // global references for this task
#endif //  ENABLE_ISOLATES
};

class TaskDesc : public TaskDescPointers {
public:
  // Returns the size of the object
  size_t object_size() {
    return allocation_size();
  }

protected:
  static jint header_size() { return sizeof(TaskDesc); }

private:
  static size_t allocation_size() {
    return align_allocation_size(header_size());
  }

  static int pointer_count() {
    return (sizeof(TaskDescPointers) - sizeof(MixedOopDesc)) /
      sizeof(OopDesc*);
  }

  jint              _string_table_count;
  jint              _symbol_table_count;
#if ENABLE_LIB_IMAGES && USE_BINARY_IMAGE_LOADER
  jint               _classes_in_images;
#endif

#if ENABLE_ISOLATES
  // Could be one of the Task::State enums
  int               _status;

  // Could be one of the Task::Priority enums
  int               _priority;

#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  // Current profile ID.
  jint              _profile_id;
#endif

  // Number of Java classes loaded by this Task (including the
  // romized classes). This is equal to the number of non-null items
  // in _class_list.
  jint              _class_count;

  jint              _exit_code;
  jint              _exit_reason;
  jint              _is_terminating;
  jint              _task_id;
  jint              _thread_count;
  jint              _startup_phase_count;
  jint              _priority_queue_valid;
  jint              _last_priority_queue;
#if ENABLE_OOP_TAG
  jint              _seq;
#endif
#endif // ENABLE_ISOLATES

  friend class FarClassDesc;
  friend class Universe;
  friend class OopDesc;
  friend class Task;
};
