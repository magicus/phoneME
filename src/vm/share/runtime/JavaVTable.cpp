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

// CopyrightVersion 1.2

#include "incls/_precompiled.incl"
#include "incls/_JavaVTable.cpp.incl"

// this function computes the vtable size.
void JavaVTable::compute_vtable_size(int& vtable_length,
                              InstanceClass*    super,
                              ObjArray*         methods, 
                              const AccessFlags class_flags, 
                              Symbol*           classname) {
  // set up default result values
  vtable_length = 0;

  // start off with super's vtable length
  if (super->is_null()) {
    vtable_length = 0;
  } else {
    ClassInfo::Raw info = super->class_info();
    vtable_length = info().vtable_length();
  }
  
  // go thru each method in the methods table to see if it needs a new entry
  Method::Raw m;
  int len = methods->length();
  for (int i = 0; i < len; i++) {
    int override_index;             // int& for return value that we don't use
    (void)override_index;
    m = methods->obj_at(i);
    bool needs_new =
      InstanceClass::needs_new_vtable_entry(&m, super, classname, 
                                            class_flags, NULL, false);
    if (needs_new) {
      vtable_length++;
    }
  }  

  if (vtable_length == 0) {
    // array classes don't have their superclass set correctly during 
    // bootstrapping
    vtable_length = base_vtable_size();
  }
}

/*
 * Computes the interface table size needed by <intf> in
 * a subclass of <base_klass>.
 */
void
JavaITable::compute_interface_size(InstanceClass* intf,
                                   InstanceClass* base_klass,
                                   int& nof_interfaces, int& nof_methods) {
  if (base_klass->not_null() && base_klass->itable_contains(intf)) {
    // This interface is already contained by <base_klass>: no need
    // to reserve any space for it in a subclass of <base_klass>
    return;
  }

  // Reserve space needed bu this interface
  ObjArray::Raw methods = intf->methods();
  nof_interfaces++;
  nof_methods += methods().length();

  // Calculate the spaces needed by super-interfaces of <intf>
  TypeArray::Raw local_inter = intf->local_interfaces();
  InstanceClass::Raw super_intf;
  for (int index = 0; index < local_inter().length(); index++) {
    super_intf = Universe::class_from_id(local_inter().ushort_at(index));
    compute_interface_size(&super_intf, base_klass, nof_interfaces, 
                           nof_methods);
  }
}

/*
 * We have a subclass X of <base_klass>. <local_interfaces> contains
 * all the interfaces that X declares to implement.
 *
 * Return the size needed by the itable of X.
 */
int JavaITable::compute_itable_size(TypeArray* local_interfaces, 
                                    InstanceClass* base_klass, 
                                    int& itable_length)
{  
  int base_klass_size;

  if (base_klass->is_null()) {
    base_klass_size = 0;
    itable_length = 0;
  } else {
    ClassInfo::Raw info = base_klass->class_info();
    base_klass_size     = info().itable_size();
    itable_length       = info().itable_length();
  }

  int nof_interfaces = 0;
  int nof_methods = 0;

  InstanceClass::Raw intf;
  for (int index = 0; index < local_interfaces->length(); index++) {
    intf = Universe::class_from_id(local_interfaces->ushort_at(index));
    compute_interface_size(&intf, base_klass, nof_interfaces, nof_methods);
  }
  itable_length += nof_interfaces;

  return base_klass_size + ClassInfo::itable_size(nof_interfaces, nof_methods);
}
