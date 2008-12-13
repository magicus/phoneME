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

#if ENABLE_COMPILER

class MemoryAddress: public GenericAddress {
 public:
  MemoryAddress(BasicType type) : GenericAddress(type) { } 

  // get the platform dependant address for this address
  BinaryAssembler::Address lo_address()  {              
    return address_for(lo_offset());
  }
  BinaryAssembler::Address hi_address()  {
    GUARANTEE(is_two_word(), "sanity check");
    return address_for(hi_offset());
  }

 protected:
  // get the Intel x86 address of this memory address given the address offset
  virtual BinaryAssembler::Address address_for(jint address_offset) 
     JVM_PURE_VIRTUAL_((BinaryAssembler::Address)0);
};

class HeapAddress: public MemoryAddress {
 public:
  HeapAddress(BasicType type) : MemoryAddress(type) { clear_address_register(); }

  // the destructor for heap address makes sure that the write barrier has been updated if necessary
 ~HeapAddress();

  // override the default write barrier implementation (which does nothing)
  virtual void write_barrier_prolog();
  virtual void write_barrier_epilog();

 protected:
  // get the Intel x86 address of this heap address given the address offset
  virtual BinaryAssembler::Address address_for(jint address_offset);   

  // compute the Intel x86 address for this heap address given the address 
  // offset
  virtual BinaryAssembler::Address compute_address_for(jint address_offset)
     JVM_PURE_VIRTUAL_((BinaryAssembler::Address)0);

 private:
  BinaryAssembler::Register _address_register;

  // accessors for the address register
  BinaryAssembler::Register  address_register() const { 
    return _address_register;  
  }
  void set_address_register(BinaryAssembler::Register value) { 
    _address_register = value;
  }

  bool has_address_register() const {
    return address_register() != BinaryAssembler::no_reg; 
  }
  void clear_address_register() { 
    set_address_register(BinaryAssembler::no_reg);        
  }
};

class FieldAddress: public HeapAddress {
 public:
  FieldAddress(Value& object, jint offset, BasicType type) :
    HeapAddress(type), _object(&object), _offset(offset) { }

 protected:
  // compute the Intel x86 address of this field address given the address offset
  virtual BinaryAssembler::Address compute_address_for(jint address_offset);   

 private:
  Value* _object;
  jint   _offset;

  // accessors for the object and offset
  Value*  object() const { return _object; }
  jint    offset() const { return _offset; }
};

class IndexedAddress: public HeapAddress {
 public:
  IndexedAddress(Value& array, Value& index, BasicType type)
    : HeapAddress(type), _array(&array), _index(&index) { } 

 protected:
  // compute the Intel x86 address of this indexed address given the address offset
  virtual BinaryAssembler::Address compute_address_for(jint address_offset);   

 private:
  Value* _array;
  Value* _index;

  // accessors for the array and index
  Value*  array()       const { return _array; }
  Value*  index()       const { return _index; }

  // get the number of bits to shift the index with 
  jint    index_shift() const { return jvm_log2(byte_size_for(type())); }
};

class StackAddress: public MemoryAddress {
 public:
  StackAddress(BinaryAssembler::Register base, BasicType type) : MemoryAddress(type), _base(base) { } 
  BinaryAssembler::Address tag_address() { return address_for(tag_offset()); }
  BinaryAssembler::Address tag2_address() { return address_for(tag2_offset()); }
  
 protected:
  // get the Intel x86 address of this stack address given the address offset
  virtual BinaryAssembler::Address address_for(jint address_offset);

  // the base offset is overridden for location addresses
  virtual jint compute_base_offset() { 
    // We want to point at the value of the low word.
    return JavaFrame::arg_offset_from_sp(is_two_word() ? 1 : 0);
  }

  // offsets to the different parts of the stack address.
  // We point to the >>value<< of the of the time.
  // For two-word items, we are pointing at the higher addressed item.
  virtual jint tag_offset()  const { 
    GUARANTEE(TaggedJavaStack, "Shouldn't be getting tag_offset()");
    return -BytesPerWord;  
  }
  virtual jint tag2_offset() const { 
    GUARANTEE(is_two_word() && TaggedJavaStack, "sanity"); 
    return -(BytesPerStackElement + BytesPerWord);   
  }

  virtual jint lo_offset()   const {
    return (is_two_word() ? -BytesPerStackElement : 0);
  }
  virtual jint hi_offset()   const { 
    GUARANTEE(is_two_word(), "sanity"); 
    return 0;
  }

 private:
  BinaryAssembler::Register _base;

  // accessor for the base register
  BinaryAssembler::Register  base() const { return _base; }
};

class LocationAddress: public StackAddress {
 public:
  // construct a location address for the given index and type
  LocationAddress(jint index, BasicType type) : StackAddress(base_for(index), type), _index(index) { } 

 protected:
  // the base offset is computed from the index of this location address
  virtual jint compute_base_offset();

 private:
  jint      _index;

  // accessor for the index
  jint       index()       const { return _index; }

  bool       is_local() const { return is_local_index(index()); }

  // check if the location address is the address of a local 
  // (as opposed to an expression stack element) 
  static bool is_local_index(jint index);

  // compute the base register for a location address with the given index
  static BinaryAssembler::Register base_for(jint index) { 
    return is_local_index(index) ? 
      BinaryAssembler::ebp : BinaryAssembler::esp; 
  }
};

#endif
