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

class RefNodeDesc : public OopDesc {
#if ENABLE_JAVA_DEBUGGER
public:
  // Returns the size of the object
  size_t object_size() {
    return allocation_size();
  }

  void variable_oops_do(void do_oop(OopDesc**)) {
    if (_ref_obj != NULL &&
        ObjectHeap::contains(_ref_obj) &&
        ObjectHeap::test_bit_for((OopDesc**)_ref_obj)) {
      // if the object is not marked, don't mark it
      // if the object IS marked, then no harm re-marking it; but the
      // real reason for this is so that when we go through the
      // update_object_pointers phase we call the do_oop routine IFF the
      // object is marked.
      do_oop((OopDesc **)&_ref_obj);
    }
    if (_next_by_ref != NULL) {
      do_oop((OopDesc**)&_next_by_ref);
    }
    if (_next_by_id != NULL) {
      do_oop((OopDesc**)&_next_by_id);
    }
  }

  jint              _seq_num;
  ReturnOop         _ref_obj;
  ReturnOop         _next_by_ref;
  ReturnOop         _next_by_id;
#else
  void variable_oops_do(oop_doer /*do_oop*/) {}
#endif

protected:
  static jint header_size() { return sizeof(RefNodeDesc); }

private:
  static size_t allocation_size() {
    return align_allocation_size(header_size());
  }

  // Initializes the object after allocation
  void initialize(OopDesc* klass) {
    OopDesc::initialize(klass);
  }

  friend class OopDesc;
  friend class Universe;
  friend class RefNode;
  friend class FarClassDesc;
  friend class JavaDebugger;
};
