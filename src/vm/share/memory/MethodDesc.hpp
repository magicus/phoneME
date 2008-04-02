/*
 *
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 *
 *!c<
 * Copyright 2006 Intel Corporation. All rights reserved.
 *!c>
 */

class MethodVariablePart {
public:
  address execution_entry() const;
  void set_execution_entry(address entry) {
    GUARANTEE(!ROM::system_text_contains((OopDesc*)this), "sanity");
#if ENABLE_METHOD_TRAPS
    // If the software tries to overwrite method's execution entry that was
    // already modified by our trap, we won't rewrite the real entry
    // saving new address in the table instead.
    MethodTrapDesc* trap = get_trap();
    if (trap != NULL) {   
      trap->set_old_entry(entry);
    } else
#endif
    {
      _execution_entry = entry;
    }
  }

#if ENABLE_METHOD_TRAPS
  MethodTrapDesc* get_trap() const;
#endif

private:
  address       _execution_entry;

friend class MethodDesc;
friend class Method;
friend class ROM;
friend class ROMBundle;
};

class MethodDesc: public OopDesc { 
 private:
  static int header_size() { return sizeof (MethodDesc); }

  // Computes the object size 
  static size_t allocation_size(int byte_code_size) {
    return align_allocation_size(header_size() + 
                                 byte_code_size * sizeof(jbyte));
  }

  address execution_entry(void) const {
    return _variable_part->execution_entry();
  }

 public:
  // Returns the object size
  size_t object_size() const {
    return allocation_size(_code_size);
  }

  // Used for computing index in profiler table
  int profile_hash() { 
      return _code_size ^ x._max_execution_stack_count
                     ^ _signature_index ^ _name_index; 
  }

  bool has_compiled_code(void) const {
    return ObjectHeap::contains((OopDesc*)execution_entry());
  }

  ReturnOop compiled_code(void) const;

  // Does this method use the specified compiled method for execution?
  bool uses_compiled_code(const CompiledMethodDesc *cm) const { 
    return ((void*)compiled_code()) == ((void*)cm);
  }

  // Call this when speed is not critical, to save space
  void set_execution_entry(address entry);

  // Call this when speed is critical
  inline void set_execution_entry_inline(address entry) {
    variable_part()->set_execution_entry(entry);
  }

  MethodVariablePart *variable_part() {
    return _variable_part;
  }
  void relocate_variable_part(int delta);

  jushort holder_id() {
    return _holder_id;
  }

#if  ENABLE_JVMPI_PROFILE 
  // get the method id
  juint method_id() {
   return _method_id;
  }
#endif

  jushort max_locals() {
    return x._max_locals;
  }

  // This method is called a lot during class loading.
  bool match(OopDesc* name, OopDesc* signature) {
    AllocationDisabler raw_pointers_used_in_this_function;

    address cp;
    if (_access_flags & JVM_ACC_HAS_COMPRESSED_HEADER && UseROM) {
      cp = (address)_rom_constant_pool;
    } else {
      cp = (address)_constants;
    }
    OopDesc **cpbase = (OopDesc**)(cp + ConstantPoolDesc::header_size());

    OopDesc *my_name = cpbase[_name_index];
    if (my_name != name) {
      return false;
    }

    OopDesc *my_sig = cpbase[_signature_index];
    if (my_sig != signature) {
      // Empty signature means that any method with the same name will match
      return signature == NULL;
    }

    return true;
  }

 private:
  // Initializes the object after allocation
  void initialize(OopDesc* klass, jint code_size) {
#if ENABLE_JVMPI_PROFILE 
    // for the Romized method to set ID.
    static int rom_method_id = 0;

    _method_id = rom_method_id ++;
#endif  
  
    OopDesc::initialize(klass);
    GUARANTEE(0 <= code_size && code_size < (1 << 16), "invalid method size");
    _code_size = (jushort)code_size;
    _variable_part = &_heap_execution_entry;
  }

  /*
   * This particular ordering is important for ROMizer:
   * 1 _constants
   * 2 _exception_table
   * 3 _stackmaps
   * 4 _heap_execution_entry
   * ... 
   * as ROMizer uses optimization that omits _constants and sometimes
   * _exception_table, _stackmaps and _heap_execution_entry. This particular
   * order yields the max ROMImage footprint saving. Do not the order
   * change without consider footprint impact!
   */
  ConstantPoolDesc* _constants;    // Constant pool
  OopDesc * _exception_table;      // Exception table (a short TypeArrayDesc*)
                                   // or GC stackmaps (a StackmapListDesc*)
  OopDesc * _stackmaps;            // Verifier stackmaps (an ObjArrayDesc*)
                                   // or GC stackmaps (a StackmapListDesc*)
  MethodVariablePart  _heap_execution_entry;
#if ENABLE_ROM_JAVA_DEBUGGER
 OopDesc * _line_var_table;
#endif
  MethodVariablePart *_variable_part;  
#if ENABLE_REFLECTION
  TypeArrayDesc* _thrown_exceptions;
#endif

  jushort     _access_flags;       // Access flags

  jushort     _holder_id;          // InstanceClass that owns this method

#if defined(UNDER_ADS) && (__ARMCC_VERSION < 200000)
  // ADS 1.2 wants this
  public:
#endif

  struct _x {
    // max_stack + max_locals + max_locks < 0xffff
    jushort _max_execution_stack_count; 

    jushort _max_locals;         // Number of local variables used by this
                                 // method
  };
  struct _y {
    address _quick_native_code;
  };

#if defined(UNDER_ADS) && (__ARMCC_VERSION < 200000)
  // ADS 1.2 wants this
  private:
#endif

  union {
   _x x;
   _y y;
  };

  jushort     _method_attributes; // size of the parameter block (receiver +
                                   // arguments) in words 

  jushort     _name_index;         // Method name (index in constant pool)

  jushort     _signature_index;    // Method signature (index in constant pool)

  jushort     _code_size;          // Size of Java bytecodes allocated
                                   // immediately after methodOop 

#if ENABLE_JVMPI_PROFILE 
public:
  juint  _method_id;               // unique ID identify this method
#endif                                     

  friend class Method;
  friend class Universe;
  //friend class JavaFrame;
#if ENABLE_COMPILER
  friend class CompiledMethodDesc;
#endif
};
