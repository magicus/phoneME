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

void OopDesc::map_oops_do(jubyte* map, void do_oop(OopDesc**)) {
  // Note: each value in the map is between [0, 0x80] (inclusive).
  // This is a little waste of space, but rarely we have more than 127
  // words between pointers inside an object. This restriction makes
  // it possible to complete the following loop with 1 comparison per
  // iteration in most cases.
  for( OopDesc** base = (OopDesc**) this;;) {
    const jint entry = (jint)(*((jbyte*)map)); map++;
    if (entry > 0) {
      base += entry; 
      do_oop(base); 
    } else if (entry == 0) {
      break;
    } else {
      GUARANTEE((entry & 0xff) == OopMapEscape, "sanity")
      base += (OopMapEscape - 1);
    }
  }
}

void OopDesc::near_do(void do_oop(OopDesc**)) {
  // No oop map for near pointer
  do_oop((OopDesc**)&_klass);
}

// The basic Java types
bool OopDesc::is_instance(void) const {
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size > 0;
}
bool OopDesc::is_bool_array(void) const {
  OopDesc* p = blueprint();
#if ENABLE_HEAP_NEARS_IN_HEAP
  if (p == Universe::rom_bool_array_class()->obj()) {
    return true;
  }
#endif
  return p == Universe::bool_array_class()->obj();
}
bool OopDesc::is_char_array(void) const {
  OopDesc* p = blueprint();
#if ENABLE_HEAP_NEARS_IN_HEAP
  if (p == Universe::rom_char_array_class()->obj()) {
    return true;
  }
#endif
  return p == Universe::char_array_class()->obj();
}
bool OopDesc::is_int_array(void) const {
  OopDesc* p = blueprint();
#if ENABLE_HEAP_NEARS_IN_HEAP
  if (p == Universe::rom_int_array_class()->obj()) {
    return true;
  }
#endif
  return p == Universe::int_array_class()->obj();
}
bool OopDesc::is_byte_array(void) const {
  OopDesc* p = blueprint();
#if ENABLE_HEAP_NEARS_IN_HEAP
  if (p == Universe::rom_byte_array_class()->obj()) {
    return true;
  }
#endif
  return p == Universe::byte_array_class()->obj();
}
bool OopDesc::is_short_array(void) const {
  OopDesc* p = blueprint();
#if ENABLE_HEAP_NEARS_IN_HEAP
  if (p == Universe::rom_short_array_class()->obj()) {
    return true;
  }
#endif
  return p == Universe::short_array_class()->obj();
}
bool OopDesc::is_long_array(void) const {
  OopDesc* p = blueprint();
#if ENABLE_HEAP_NEARS_IN_HEAP
  if (p == Universe::rom_long_array_class()->obj()) {
    return true;
  }
#endif
  return p == Universe::long_array_class()->obj();
}
bool OopDesc::is_float_array(void) const {
  OopDesc* p = blueprint();
#if ENABLE_HEAP_NEARS_IN_HEAP
  if (p == Universe::rom_float_array_class()->obj()) {
    return true;
  }
#endif
  return p == Universe::float_array_class()->obj();
}
bool OopDesc::is_double_array(void) const {
  OopDesc* p = blueprint();
#if ENABLE_HEAP_NEARS_IN_HEAP
  if (p == Universe::rom_double_array_class()->obj()) {
    return true;
  }
#endif
  return p == Universe::double_array_class()->obj();
}
bool OopDesc::is_obj_array(void) const {
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size == InstanceSize::size_obj_array;
}
bool OopDesc::is_type_array(void) const { 
  const jint instance_size = blueprint()->instance_size_as_jint();
  GUARANTEE(InstanceSize::size_type_array_8 < InstanceSize::size_type_array_1, 
            "sanity check");
  return instance_size >= InstanceSize::size_type_array_8
      && instance_size <= InstanceSize::size_type_array_1;
}
  

// Specific instance types
bool OopDesc::is_string(void) const {
  return blueprint() == Universe::string_class()->obj();          
}
bool OopDesc::is_jvm_thread(void) const {
  const jint instance_size = blueprint()->instance_size_as_jint();
  return (instance_size == InstanceSize::size_mixed_oop && 
          ((MixedOopDesc *)this)->_type == MixedOopDesc::Type_Thread);
}

bool OopDesc::is_throwable(void) const {
  const InstanceClassDesc* const klass = (InstanceClassDesc*) blueprint();
  return ((InstanceClass*)&klass)->is_subclass_of(Universe::throwable_class());
}

// Other basic types in the system
bool OopDesc::is_compiled_method(void) const {
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size== InstanceSize::size_compiled_method;
} 
bool OopDesc::is_symbol(void) const {
  if (is_rom_symbol()) {
    return true;
  } else {
    const jint instance_size = blueprint()->instance_size_as_jint();
    return instance_size== InstanceSize::size_symbol;
  }
} 
bool OopDesc::is_method(void) const {
  if (is_rom_method()) {
    return true;
  } else {
    const jint instance_size = blueprint()->instance_size_as_jint();
    return instance_size== InstanceSize::size_method;
  }
}
bool OopDesc::is_constant_pool(void) const {
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size== InstanceSize::size_constant_pool;
}
bool OopDesc::is_entry_activation(void) const {
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size== InstanceSize::size_entry_activation;
}
bool OopDesc::is_execution_stack(void) const {
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size== InstanceSize::size_execution_stack;
}
bool OopDesc::is_condition(void) const {
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size== InstanceSize::size_mixed_oop; // IMPL_NOTE: 
                         // consider whether it should be fixed. 
}
bool OopDesc::is_mixed_oop(void) const {
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size == InstanceSize::size_mixed_oop;
}
bool OopDesc::is_task(void) const {
  return is_mixed_oop() && ((MixedOopDesc*)this)->is_task();
}
bool OopDesc::is_boundary(void) const {
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size == InstanceSize::size_boundary;
}
bool OopDesc::is_stackmap_list(void) const {
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size== InstanceSize::size_stackmap_list;
}
bool OopDesc::is_instance_class(void) const {
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size == InstanceSize::size_instance_class;
}
bool OopDesc::is_class_info(void) const {
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size == InstanceSize::size_class_info;
}
bool OopDesc::is_type_array_class(void) const {
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size == InstanceSize::size_type_array_class;
}
bool OopDesc::is_obj_array_class(void) const {
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size == InstanceSize::size_obj_array_class;
}

bool OopDesc::is_method_class(void) const {
  return  this == Universe::method_class()->obj();
}

bool OopDesc::is_obj_near(void) const {  
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size == InstanceSize::size_obj_near;
}

bool OopDesc::is_java_near(void) const { 
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size == InstanceSize::size_java_near;
}

#if ENABLE_HEAP_NEARS_IN_HEAP && USE_SOURCE_IMAGE_GENERATOR
bool OopDesc::is_generic_near(void) const {  
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size == InstanceSize::size_generic_near;
}

bool OopDesc::is_far(void) const {    
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size == InstanceSize::size_far_class;
}

bool OopDesc::is_near(void) const {    
  return is_java_near() || is_obj_near() || is_generic_near() || is_far() ||
    is_instance_class() || is_type_array_class() || is_obj_array_class();
}
#endif 
bool OopDesc::is_task_mirror(void) const {
  const jint instance_size = blueprint()->instance_size_as_jint();
  return instance_size == InstanceSize::size_task_mirror;
}

bool OopDesc::is_thread(void) const {
  return blueprint() == Universe::thread_class()->obj();          
}

bool OopDesc::is_file_decoder(void) const {
  return is_mixed_oop() && (((MixedOopDesc*)this)->is_file_decoder() ||
                            ((MixedOopDesc*)this)->is_inflater());
}

bool OopDesc::is_inflater(void) const {
  return is_mixed_oop() && ((MixedOopDesc*)this)->is_inflater();
}

bool OopDesc::is_jar_file_parser(void) const {
  return is_mixed_oop() && ((MixedOopDesc*)this)->is_jar_file_parser();
}
