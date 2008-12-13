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

// Interface to java.lang.Thread objects

class ThreadObj : public Instance {
 public:
  HANDLE_DEFINITION_CHECK(ThreadObj, Instance);

  static int priority_offset() {
    return header_size() + 0 * sizeof(jobject);
  }
  static int thread_offset() {
    return header_size() + 2 * sizeof(jobject);
  }
  static int is_terminated_offset() {
    return header_size() + 3 * sizeof(jobject);
  }
  static int is_stillborn_offset() {
    return header_size() + 4 * sizeof(jobject);
  }

#if ENABLE_CLDC_11
  static int name_offset() {
    return header_size() + 5*sizeof(jobject);
  }
#endif

  // Returns VM thread object corresponding to this java.lang.Thread instance.
  ReturnOop thread() {
    return obj_field(thread_offset());
  }
  void set_thread(Oop* value) {
    GUARANTEE(is_unstarted(), "thread should not be started yet");
    obj_field_put(thread_offset(), value); 
  }

  void clear_thread() {
    GUARANTEE(is_terminated(), "Thread should be terminated");
    obj_field_clear(thread_offset());
  }

  // Accessors for thread priority
  jint priority() const {
    return int_field(priority_offset());
  }
  void set_priority(jint value) {
    int_field_put(priority_offset(), value);
  }

  // Accessors for terminated
  bool is_terminated() const {
    return int_field(is_terminated_offset()) != 0;
  };
  void set_terminated() {
    int_field_put(is_terminated_offset(), 1);
  }

  // Accessors for stillborn
  bool is_stillborn() const {
    return int_field(is_stillborn_offset()) != 0;
  };
  void set_stillborn() {
    int_field_put(is_stillborn_offset(), 1);
  }
  void clear_stillborn() {
    int_field_put(is_stillborn_offset(), 0);
  }

#if ENABLE_CLDC_11
  // Accessors for name
  ReturnOop get_name() {
    return obj_field(name_offset());
  }
#endif

  // Testers
  bool is_unstarted();
  bool is_alive();

  enum {
    PRIORITY_NORMAL = 5,
    NUM_PRIORITY_SLOTS = 10
  };

  static void verify_fields() PRODUCT_RETURN;
  void print_value_on(Stream* /*st*/) PRODUCT_RETURN;
};
