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

/** \class JavaVtable
    Used to create the dispatch table for Java methods.
*/

class JavaVTable : public AllStatic {
 public:
  // computes vtable length (in words)
  static void compute_vtable_size(int &vtable_length,
                           InstanceClass* super, ObjArray* methods, 
                           const AccessFlags class_flags,
                           Symbol* classname);

  // returns the vtable size of java.lang.Object
  static int base_vtable_size() {
    return ENABLE_REFLECTION ? 4 : 3;
  }

 private:
};

// Format of an itable
//
//    ---- offset table ---
//    klass_index of interface 1
//    offset to vtable from start of oop
//    ...
//    klass_index of interface n
//    offset to vtable from start of oop
//    --- vtable for interface 1 ---
//    methodOop
//    compiler entry point
//    ...
//    methodOop
//    compiler entry point
//    -- vtable for interface 2 ---
//    ...
class JavaITable : public AllStatic {
 public:
  static int compute_itable_size(TypeArray* local_interfaces, 
                                 InstanceClass* base_klass, 
                                 int& itable_length);
  static void compute_interface_size(InstanceClass* intf, 
                                     InstanceClass* base_klass,
                                     int& nof_interfaces, int& nof_methods);

};
