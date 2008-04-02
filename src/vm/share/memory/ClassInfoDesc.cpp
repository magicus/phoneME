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

# include "incls/_precompiled.incl"
# include "incls/_ClassInfoDesc.cpp.incl"

void ClassInfoDesc::variable_oops_do(void do_oop(OopDesc**)) {
  //
  // Reminder:: remember to change ROMWriter::stream_class_info()
  // whenever you change this function!
  //

  if (_access_flags.is_array_class()) {
    //do_oop((OopDesc**)&array._name);
  } else {
    do_oop((OopDesc**)&instance._methods);
    do_oop((OopDesc**)&instance._local_interfaces);
    do_oop((OopDesc**)&instance._fields);
    do_oop((OopDesc**)&instance._constants);
#if ENABLE_REFLECTION
    do_oop((OopDesc**)&instance._inner_classes);
#endif
  }

  // vtable
  OopDesc** vtable = (OopDesc**) ((address) this + header_size());
  int i;
  for (i = 0; i < _vtable_length; i++) {
    do_oop(vtable++);  // visit virtual method
  }

  // itable
  OopDesc** itable = vtable;
  OopDesc** itable_end = (OopDesc**) ((address) this + _object_size);
  for (i = 0; i < _itable_length; i++) {
    itable++;          // skip interface klass_index
    itable++;          // skip integer method index
  }
  while (itable < itable_end) {
    do_oop(itable++);  // visit interface method
  }    
}
