/*
 *   
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

class TaskContextSave {
protected:
  int _prev_task_id;
  int _current_task_id;
  bool _status;
#ifdef AZZERT
  int level;
  static int _count;
#endif
  void init();
  void dispose();
public:
  int _number_of_java_classes;
  TaskContextSave()  {init();}
  ~TaskContextSave() {dispose();}

  bool status();

  enum {
    INVALID = 0,
    VALID = 1
  };

  friend class TaskGCContext;
  friend class TaskContext;
};


class TaskGCContext : public TaskContextSave {
  void init(int task_id);
  void dispose();
public:
  TaskGCContext(int task_id) { init(task_id); };
#if ENABLE_ISOLATES
  TaskGCContext(const OopDesc* const object);
#endif
  ~TaskGCContext() {dispose();}
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
  void dispose();
public:

  TaskContext(int task_id) {init(task_id);}

  TaskContext() {}

  static void set_current_task_id(int task_id);

  static int current_task_id();

  static int number_of_java_classes();

  static void set_number_of_java_classes(int number);

  static void set_current_task(int task_id);

  static void update_list_bases(int task_id);

  static void set_class_list(ObjArray *cl);

  static void set_mirror_list(ObjArray *ml);

  friend class Universe;
};
