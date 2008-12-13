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
# include "incls/_OopDesc.cpp.incl"

size_t OopDesc::object_size_for(const FarClassDesc* blueprint) const {
  jint instance_size = blueprint->instance_size_as_jint();
  switch(instance_size) {
  default:
    // All positive values indicate an instance class, and the _value is
    // the size of the instance
    GUARANTEE(instance_size > 0, "bad instance size");
    return instance_size;
    
    // Arrays
  case InstanceSize::size_type_array_1:  
    return ((ArrayDesc*)this)->object_size(1);
  case InstanceSize::size_type_array_2:  
    return ((ArrayDesc*)this)->object_size(2);
  case InstanceSize::size_type_array_4:  
  case InstanceSize::size_obj_array:
    return ((ArrayDesc*)this)->object_size(4);
  case InstanceSize::size_type_array_8:  
    return ((ArrayDesc*)this)->object_size(8);
    
    // Other variable size objects
  case InstanceSize::size_method:
    return ((MethodDesc*)this)->object_size();
#if USE_COMPILER_STRUCTURES
  case InstanceSize::size_compiled_method:
    return ((CompiledMethodDesc*)this)->object_size();
#endif
  case InstanceSize::size_constant_pool:
    return ((ConstantPoolDesc*)this)->object_size();
  case InstanceSize::size_symbol:  
    return ((SymbolDesc*)this)->object_size();

  case InstanceSize::size_entry_activation:  
    return ((EntryActivationDesc*)this)->object_size();

  case InstanceSize::size_execution_stack:
    return ((ExecutionStackDesc*)this)->object_size();

  case InstanceSize::size_stackmap_list:
    return ((StackmapListDesc*)this)->object_size();
    
    // These are all a fixed size.
  case InstanceSize::size_generic_near:
    return NearDesc::allocation_size();
  case InstanceSize::size_java_near:
    return JavaNearDesc::allocation_size();
  case InstanceSize::size_obj_near:
    return ObjNearDesc::allocation_size();
  case InstanceSize::size_mixed_oop:
    return ((MixedOopDesc*)this)->object_size();
  case InstanceSize::size_boundary:
    return BoundaryDesc::allocation_size();
    
#if ENABLE_JAVA_DEBUGGER
  case InstanceSize::size_refnode:
    return RefNodeDesc::allocation_size();
#endif
#if ENABLE_ISOLATES
  case InstanceSize::size_task_mirror:
    return ((TaskMirrorDesc*)this)->object_size();
#endif
  case InstanceSize::size_far_class:
  case InstanceSize::size_instance_class:
  case InstanceSize::size_obj_array_class:
  case InstanceSize::size_type_array_class: 
    return ((FarClassDesc*)this)->object_size();
  case InstanceSize::size_class_info: 
    return ((ClassInfoDesc*)this)->object_size();
  }
}

bool OopDesc::is_rom_symbol() const {
  return ROM::is_rom_symbol(this);
}

bool OopDesc::is_rom_method() const {
  return ROM::is_rom_method(this);
}

#ifndef PRODUCT

OopDesc* OopDesc::klass() const {
  if (UseROM && ROM::system_text_contains(this)) {
    return ROM::text_klass_of(this);
  }
  GUARANTEE(this != NULL, "NULL oopdesc *");
  return this->_klass;
}

#endif // !defined(PRODUCT)
