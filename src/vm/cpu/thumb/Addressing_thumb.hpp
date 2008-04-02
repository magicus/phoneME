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

/*
 * Warning: ENABLE_THUMB_COMPILER=true is not a supported feature
 * This code is included only for reference purposes.
 */

#if ENABLE_THUMB_COMPILER

#if ENABLE_COMPILER

class MemoryAddress: public GenericAddress {
 protected:
  void clear_address_register() { 
    _address_register = Assembler::no_reg;
    _register_includes_base_offset = false;
  }

 public:
  MemoryAddress(BasicType type) : GenericAddress(type) { 
    clear_address_register(); 
  } 

  // the destructor deallocates any registers used in the address computation
 ~MemoryAddress();

  // get the platform dependant address for this address

  void get_preindexed_address(bool lo,
                              Assembler::Register& reg,
                              jint& offset) {
    if (lo) {
      prepare_preindexed_address(lo_offset(), reg, offset);
    } else {
      prepare_preindexed_address(hi_offset(), reg, offset);
    }      
    return;
  }

  void get_indexed_address(bool lo,
                           Assembler::Register& reg,
                           jint& offset) {
    if (lo) {
      prepare_indexed_address(lo_offset(), reg, offset);
    } else {
      prepare_indexed_address(hi_offset(), reg, offset);
    }      
    return;
  }

  private:
    void prepare_preindexed_address(jint address_offset,
                                    Assembler::Register& reg,
                                    jint& offset);
    void prepare_indexed_address(jint address_offset,
                                 Assembler::Register& reg,
                                 jint& offset);
  
 protected:
   // has_fixed_offset returns true iff the field we want is a fixed distance
   // from the start of the object (excluding the base_offset().  If true,
   // it also sets fixed_offset to that fixed offset value.  
  virtual bool has_fixed_offset(jint& /*fixed_offset*/) JVM_PURE_VIRTUAL_0;
 
  // Returns the register from which we're calculating the offset
  virtual Assembler::Register fixed_register() 
      JVM_PURE_VIRTUAL_((Assembler::Register)0);

  // Allocate an address register and initialize its value.
  void    create_and_initialize_address_register();

  // fill in the address register.  This implementation assumes that
  // has_fixed_offset(jint&) returns true.  If has_fixed_offset() can return
  // false, then this definition must be overridden
  virtual void fill_in_address_register();

  // After we create the address register, all other registers are unused.
  virtual void destroy_nonaddress_registers() {}

  // get the base offset for this memory address
  virtual jint base_offset() const { return 0; }

  // accessors for the address register
  Assembler::Register  address_register() const { 
    return _address_register;  
  }

  void set_address_register(Assembler::Register value) { 
    _address_register = value; 
    _register_includes_base_offset = false;
  }

  bool has_address_register() const { 
    return address_register() != Assembler::no_reg; 
  }

  bool address_register_includes_base_offset() const {
    return _register_includes_base_offset;
  }

  void set_address_register_includes_base_offset() {
    _register_includes_base_offset = true;
  }

 private:
  Assembler::Register _address_register;
  bool _register_includes_base_offset;
};

class HeapAddress: public MemoryAddress {
 public:
  HeapAddress(BasicType type) : MemoryAddress(type) { }

  // override the default write barrier implementation (which does nothing)
  virtual void write_barrier_prolog();
  virtual void write_barrier_epilog();
};

class FieldAddress: public HeapAddress {
 public:
  FieldAddress(Value& object, jint offset, BasicType type) : 
      HeapAddress(type), _object(&object), _offset(offset) { }

 protected:
  virtual Assembler::Register fixed_register();
  virtual bool                has_fixed_offset(jint& fixed_offset);
  virtual void                destroy_nonaddress_registers();

 private:
  Value* _object;
  jint   _offset;

  // accessors for the object and offset
  Value*  object() const { return _object; }
  jint    offset() const { return _offset; }
};

class IndexedAddress: public HeapAddress {
 public:
  IndexedAddress(Value& array, Value& index, BasicType type) : 
    HeapAddress(type), _array(&array), _index(&index) { } 

 protected:
  // Determine register and offset for access with address register
  virtual Assembler::Register fixed_register();
  virtual bool                has_fixed_offset(jint& fixed_offset);

  // fill in the address register
  virtual void fill_in_address_register();

  virtual void destroy_nonaddress_registers();

  // override the base offset defined in memory address
  virtual jint base_offset() const { return Array::base_offset(); }

 private:
  Value* _array;
  Value* _index;

  // accessors for the array and index
  Value*  array()       const { return _array; }
  Value*  index()       const { return _index; }

  // get the number of bits to shift the index with 
  jint    index_shift() const { return jvm_log2(byte_size_for(type())); }
};

class LocationAddress: public MemoryAddress {
 public:
  // construct a location address for the given index and type
  LocationAddress(jint index, BasicType type) : 
     MemoryAddress(type), _index(index) {}

  int get_fixed_offset();
      
 protected:
   // Determine register and offset for access with address register
  virtual Assembler::Register fixed_register();
  virtual bool                has_fixed_offset(jint& fixed_offset);

  virtual jint lo_offset()   const { 
    return JavaStackDirection < 0 ? (is_two_word() ? -BytesPerStackElement : 0)
                                  : 0;
  }
  virtual jint hi_offset()   const { 
    GUARANTEE(is_two_word(), "sanity"); 
    return JavaStackDirection < 0 ? 0 : BytesPerStackElement;
  }

 private:
  jint   _index;
  jint    index()       const { return _index; }

  // check if the location address is the address of a local (as opposed to
  // an expression stack element) 
  bool       is_local()    const { return method()->is_local(index()); }
};

#endif

#endif /* #if ENABLE_THUMB_COMPILER*/
