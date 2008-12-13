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
# include "incls/_NearClass.cpp.incl"

#ifndef PRODUCT

void NearClass::print_value_on(Stream* st) {
#if USE_DEBUG_PRINTING
  // This is called for both Java Near classes and for generic near classes
  const char *super_type;
  switch(instance_size().value()) {
    case InstanceSize::size_java_near: {
      FarClass parent = klass();
      if (parent.equals(Universe::type_array_class_class())) {
          super_type = "TypeArray";
      } else if (parent.equals(Universe::obj_array_class_class())) {
          super_type = "ObjArray";
      } else if (parent.equals(Universe::instance_class_class())) {
          super_type = "Instance";
      } else {
          super_type = "??Java??";
      }
      break;
    }

    case InstanceSize::size_generic_near:
      super_type = "Generic";
      break;

    case InstanceSize::size_obj_near: // for methods
      super_type = "Object";
      break;

    default:
      super_type = "??";
      break;
  }
  st->print("The %s NearClass ", super_type);
#endif
}

void NearClass::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  FarClass::iterate(visitor);
#endif
}

#endif
