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

class JavaNear: public Near {
 public:
  HANDLE_DEFINITION_CHECK(JavaNear, Near);

  // To avoid endless lists of friends the static offset computation routines
  // are all public. 
  static int raw_value_offset()  { 
    return FIELD_OFFSET(JavaNearDesc, _value); 
  }
  static int class_info_offset() {
    return FIELD_OFFSET(JavaNearDesc, _class_info); 
  }
 private:
  jint raw_value() const         { 
    return int_field(raw_value_offset());          
  }
  void set_raw_value(jint value) { 
     int_field_put(raw_value_offset(), value);      
  }

  ReturnOop class_info() {
    return obj_field(class_info_offset());
  }

  void set_class_info(ClassInfo* value) {
    obj_field_put(class_info_offset(), value);
  }

  jint hash_value() const { 
    return mask_bits(raw_value(), hash_mask_in_place) >> hash_shift;
  }

  jint lock_value() const { 
    return mask_bits(raw_value(), lock_mask_in_place) >> lock_shift; 
  }

 public:

  // Test operations
  bool is_locked()   const { return lock_value() != unlocked_value; }
  bool has_hash()    const { return hash_value() != no_hash_value;  }
  bool has_waiters() const { 
    // is locked & has waiters
    return lock_value() == waiters_value;  
  }  

  // Accessors
  jint hash() const {
    GUARANTEE(has_hash(), "must have hash");
    return hash_value();
  }

  void set_hash(jint hash_value);

  void set_lock(jint lock_value);

  enum { hash_bits  = 30,
         lock_bits  =  2,

         lock_shift =  0,
         hash_shift = lock_bits,

         // Mask for field

#if UNDER_ADS
         hash_mask          = 0x3fffffff,
#else
         hash_mask          = right_n_bits(hash_bits),
#endif
#if UNDER_ADS
         hash_mask_in_place = 0xfffffffc,
#else
         hash_mask_in_place = hash_mask << hash_shift,
#endif

         lock_mask          = right_n_bits(lock_bits),
         lock_mask_in_place = lock_mask << lock_shift,

         // Hash constants
         no_hash_value  = 0,

         // Lock constants
         unlocked_value = 0,
         locked_value   = 1,
         waiters_value  = 3 
       };

#ifndef PRODUCT
  void iterate(OopVisitor* visitor);
  void print_value_on(Stream* st);
#endif

  // Used for setting the class when the object is on the stack.
  // The default set_klass uses the write barrier!!!!!
  void set_raw_klass(Oop* value) { 
     // please do not use write barrier here
     *obj()->obj_field_addr(klass_offset()) = value->obj();
  }

#if ENABLE_ROM_GENERATOR
  int generate_fieldmap(TypeArray* field_map);
#endif
 
  friend class StackLock;
  friend class Synchronizer;
  friend class Scheduler;
  friend class Universe;
  friend class BinaryObjectWriter;
  friend class ROM;
};
