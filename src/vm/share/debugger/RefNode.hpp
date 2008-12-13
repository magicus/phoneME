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

/** \class RefNode
    A RefNode is an object used to map JPDA ID's to object in the VM.
    The debugger is passed opaque ID numbers that refer to active
    objects in the VM.  This class supports a hash table of this object ID's
    as well as GC support for elements in the hash table

*/
#if ENABLE_JAVA_DEBUGGER

class RefNode : public Oop {

public:
  HANDLE_DEFINITION(RefNode, Oop);

  jint seq_num() {
    return int_field(seq_num_offset());
  }
  void set_seq_num(jint val) {
    int_field_put(seq_num_offset(), val);
  }
  ReturnOop ref_obj() {
    return obj_field(ref_obj_offset());
  }
  void set_ref_obj(Oop *p) {
    obj_field_put(ref_obj_offset(), p);
  }

  ReturnOop next(bool by_ref) {
    if (by_ref) {
      return obj_field(next_by_ref_offset());
    } else {
      return obj_field(next_by_id_offset());
    }
  }
  void set_next(Oop *p, bool by_ref) {
    if (by_ref) {
      obj_field_put(next_by_ref_offset(), p);
    } else {
      obj_field_put(next_by_id_offset(), p);
    }
  }
  ReturnOop next_by_ref() {
    return obj_field(next_by_ref_offset());
  }
  void set_next_by_ref(Oop *p) {
    obj_field_put(next_by_ref_offset(), p);
  }
  ReturnOop next_by_id() {
    return obj_field(next_by_id_offset());
  }
  void set_next_by_id(Oop *p) {
    obj_field_put(next_by_id_offset(), p);
  }

  static int seq_num_offset() {
    return FIELD_OFFSET(RefNodeDesc, _seq_num);
  }
  static int ref_obj_offset() {
    return FIELD_OFFSET(RefNodeDesc, _ref_obj);
  }
  static int next_by_ref_offset() {
    return FIELD_OFFSET(RefNodeDesc, _next_by_ref);
  }
  static int next_by_id_offset() {
    return FIELD_OFFSET(RefNodeDesc, _next_by_id);
  }

 };

#endif
