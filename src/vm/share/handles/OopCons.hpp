/*
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

// Utility class for a linked list of objects

class OopCons : public ObjArray {
 private:
  enum {
    OopOffset  = 0,
    NextOffset = 1,
    Size       = 2
  };

 public:
  HANDLE_DEFINITION(OopCons, ObjArray);

  static ReturnOop cons_up(const Oop* oop, const OopCons* next JVM_TRAPS) {
    OopCons::Raw cons = Universe::new_obj_array(OopCons::Size JVM_CHECK_0);
    cons().set_next(next);
    cons().set_oop(oop);
    return cons.obj();
  }

  // The oop
  ReturnOop oop() const { 
    return obj_at(OopCons::OopOffset);
  }

  // Link to next element in list
  ReturnOop next() const { 
    return obj_at(OopCons::NextOffset);
  }

  void set_oop(const Oop* oop) { 
    set_oop(oop->obj());
  }

  void set_oop(OopDesc* oop) { 
    obj_at_put(OopCons::OopOffset, oop);
  }

  void set_next(const OopCons* next) { 
    set_next(next->obj()); 
  }

  void set_next(OopDesc* next) { 
    obj_at_put(OopCons::NextOffset, next);
  }
};
