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

#if ENABLE_ISOLATES
class TaskContextSave {
protected:
  static TaskContextSave _global_context;
  int _number_of_java_classes;
  int _prev_task_id;
  int _current_task_id;
  bool _valid;

#ifdef AZZERT
  int level;
  static int _count;
#endif

  void init   ( void );
  void dispose( void );
public:
  TaskContextSave( void ) { init();    }
 ~TaskContextSave( void ) { dispose(); }

  bool valid( void ) const { return _valid; }

  friend class TaskGCContext;
  friend class TaskContext;
};

class TaskGCContext : public TaskContextSave {
  void init( const int task_id );
  void init( const OopDesc* object );
  void dispose( void );
  void set ( const int task_id );
public:
  TaskGCContext( const int task_id ) { init( task_id ); }
  TaskGCContext( const OopDesc* object ) { init( object ); }
 ~TaskGCContext( void ) { dispose(); }
};

#if ENABLE_OOP_TAG
class TaskGCContextDebug : public TaskContextSave {
  void init(int class_id, int task_id);
  void dispose();
public:
  TaskGCContextDebug(int class_id, int task_id) {init(class_id, task_id);}
  ~TaskGCContextDebug() {dispose();}
};
#endif

class TaskContext : public TaskContextSave {
  void init(int task_id);
public:
  TaskContext(int task_id) {init(task_id);}
  TaskContext() {}

  static void set_current_task_id( const int task_id ) {
    _global_context._current_task_id = task_id;
  }
  static int current_task_id( void ) {
    return _global_context._current_task_id;
  }

  static int number_of_java_classes( void ) {
    return _global_context._number_of_java_classes;
  }
  static void set_number_of_java_classes(int number);
  static void set_current_task(int task_id);

  static void set_class_list(ObjArray *cl);
  static void set_mirror_list(ObjArray *ml);

};

class TaskAllocationContext : public TaskContext {
public:
  TaskAllocationContext(int task_id) : TaskContext(task_id) {
    ObjectHeap::on_task_switch(task_id);
  }

 ~TaskAllocationContext( void ) {
    ObjectHeap::on_task_switch(_prev_task_id);
  }
};

#else // ENABLE_ISOLATES

class TaskGCContext {
public:
  static bool valid ( void ) { return true; }
  TaskGCContext( const int      /*task_id*/ ) {}
  TaskGCContext( const OopDesc* /*object*/  ) {}
 ~TaskGCContext( void ) {}
};

class TaskContext {
  static int _number_of_java_classes;
public:
  static int current_task_id( void ) {
    return 1;
  }
  static void set_number_of_java_classes(int number) {
    _number_of_java_classes = number;
  }
  static void set_current_task_id( const int /*task_id*/ ) {}
  static void set_current_task   ( const int /*task_id*/ ) {}
  static int number_of_java_classes( void ) {
    return _number_of_java_classes;
  }
};

class TaskAllocationContext : public TaskContext {
public:
  TaskAllocationContext(int task_id) {}
};

#endif // ENABLE_ISOLATES
