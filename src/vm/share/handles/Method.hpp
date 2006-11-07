/*
 *
 * Portions Copyright  2003-2006 Sun Microsystems, Inc. All Rights Reserved.
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

class Method: public Oop {
 public:
  HANDLE_DEFINITION_CHECK(Method, Oop);
  ~Method() {}

 private:
  MethodDesc* method() const {
    return ((MethodDesc*) obj());
  }

 public:
  // To avoid endless lists of friends the static offset computation routines
  // are all public. 
  static int code_size_offset() {
    return FIELD_OFFSET(MethodDesc, _code_size);
  }
  static int max_execution_stack_count_offset() {
    return FIELD_OFFSET(MethodDesc, x._max_execution_stack_count);
  }
  static int max_locals_offset() {
    return FIELD_OFFSET(MethodDesc, x._max_locals);
  }
  static int size_of_parameters_and_return_type_offset() {
    return FIELD_OFFSET(MethodDesc, _size_of_parameters);
  }
  static int name_index_offset() {
    return FIELD_OFFSET(MethodDesc, _name_index);
  }
  static int signature_index_offset() {
    return FIELD_OFFSET(MethodDesc, _signature_index);
  }
  static int constants_offset() {
    return FIELD_OFFSET(MethodDesc, _constants);
  }
  static int exception_table_offset() {
    return FIELD_OFFSET(MethodDesc, _exception_table);
  }
  static int stackmaps_offset() {
    return FIELD_OFFSET(MethodDesc, _stackmaps);
  }
  enum {
    NUMBER_OF_RETURN_TYPE_BITS = 2,
    NUMBER_OF_PARAMETERS_BITS = 14,
    SIZE_OF_PARAMETERS_MASK = 0x3fff
  };

#if ENABLE_REFLECTION
  static int thrown_exceptions_offset() {
    return FIELD_OFFSET(MethodDesc, _thrown_exceptions);
  }
#endif

#if  ENABLE_JVMPI_PROFILE 
  // get the method id offset
  static int method_id_offset() {
    return FIELD_OFFSET(MethodDesc, _method_id);
  }
#endif 

  static int holder_id_offset() {
    return FIELD_OFFSET(MethodDesc, _holder_id);
  }
  static int access_flags_offset() {
    return FIELD_OFFSET(MethodDesc, _access_flags);
  }
  static int variable_part_offset() {
    return FIELD_OFFSET(MethodDesc, _variable_part);      
  }
  static int heap_execution_entry_offset() { 
    return FIELD_OFFSET(MethodDesc, _heap_execution_entry); 
  }
  static int quick_native_code_offset() {
    return FIELD_OFFSET(MethodDesc, y._quick_native_code);
  }
  static int fast_accessor_offset_offset() {
    // We're stealing the _max_locals to store the offset of
    // fast getfield accessors, as the bytecodes of 
    // these method have are never compiled or interpreted.
    return FIELD_OFFSET(MethodDesc, x._max_locals);
  }
  static int fast_accessor_type_offset() {
    // We're stealing the _max_execution_stack_count to store the type of
    // fast getfield accessors, as the bytecodes of 
    // these method have are never compiled or interpreted.
    return FIELD_OFFSET(MethodDesc, x._max_execution_stack_count);
  }

#if ENABLE_ROM_GENERATOR
  int generate_fieldmap(TypeArray* field_map);
#endif

  static int base_offset() { 
    return MethodDesc::header_size();                     
  } 
  static int native_code_offset() { 
    return align_size_up(base_offset() + 1, BytesPerWord);
  }
  static int native_code_offset_from_bcp() { 
    return native_code_offset() - base_offset();         
  }

#if ENABLE_ROM_JAVA_DEBUGGER
  static int line_var_table_offset() {
    return FIELD_OFFSET(MethodDesc, _line_var_table);
  }
#endif

 private:
  int bc_offset_for(const int bci) const {
    GUARANTEE(bci < code_size(), "Cannot get bytecode outside method");
    return base_offset() + bci;
  }

 public:
  // method size
  jushort code_size() const { 
    return ushort_field(code_size_offset());
  }
  void set_code_size(jushort value) { 
    ushort_field_put(code_size_offset(), value);
  }

  address code_base() const {
    return ((address) method()) + base_offset();
  }
  address code_end() const {
    return code_base() + code_size();
  }

  // max locals
  jushort max_locals() const {
    GUARANTEE(GenerateROMImage || 
              (!is_quick_native() && !is_fast_get_accessor()),
              "Misuse of overloaded field");
    return ushort_field(max_locals_offset());
  }

  void set_max_locals(jushort size) {
    GUARANTEE(GenerateROMImage ||
                         (!is_quick_native() && !is_fast_get_accessor()),
              "misuse of overloaded field");
    ushort_field_put(max_locals_offset(), size);
  }

    // size of parameters
  jushort size_of_parameters() const {
    return ushort_field(size_of_parameters_and_return_type_offset()) & 
           SIZE_OF_PARAMETERS_MASK;
  }
  jushort size_of_return_type() const {
    return ushort_field(size_of_parameters_and_return_type_offset()) >>
           NUMBER_OF_PARAMETERS_BITS;
  }
  jushort size_of_parameters_and_return_type() const {
    return ushort_field(size_of_parameters_and_return_type_offset());
  }
  void set_size_of_parameters_and_return_type(jushort value) {
    ushort_field_put(size_of_parameters_and_return_type_offset(), value);
  }

  // max stack locations needed
  jushort max_execution_stack_count() const {
    GUARANTEE(GenerateROMImage ||
                         (!is_quick_native() && !is_fast_get_accessor()),
              "misuse of overloaded field");
    return ushort_field(max_execution_stack_count_offset());
  }
  void set_max_execution_stack_count(jushort size) {
    GUARANTEE(GenerateROMImage ||
                         (!is_quick_native() && !is_fast_get_accessor()),
              "misuse of overloaded field");
    ushort_field_put(max_execution_stack_count_offset(), size);
  }

  // max stack
  jushort max_stack() const {
    return (jushort)(max_execution_stack_count() - max_locals());
  }

  // name index
  jushort name_index() const {
    return ushort_field(name_index_offset());
  }
  void set_name_index(jushort index) {
    ushort_field_put(name_index_offset(), index);
  }

  // signature index
  jushort signature_index() const {
    return ushort_field(signature_index_offset());
  }
  void set_signature_index(jushort index) {
    ushort_field_put(signature_index_offset(), index);
  }

  // vtable index
  int vtable_index();

  // access flag
  AccessFlags access_flags() const {
    AccessFlags af;
    af.set_flags(ushort_field(access_flags_offset()));
    return af;
  }
  void set_access_flags(AccessFlags flags) {
    GUARANTEE((flags.as_int() & 0xffff0000) == 0, "16-bit flags only!");
    short_field_put(access_flags_offset(), flags.as_short());
  }

  // Tells whether the method has compressed headers
  bool has_compressed_header() const {
    return access_flags().has_compressed_header();
  }
  bool has_no_stackmaps() const {
    return access_flags().has_no_stackmaps();
  }
  bool has_no_exception_table() const {
    return access_flags().has_no_exception_table();
  }

  // constant pool of this method
  ReturnOop constants() const;

private:
  void set_interpreter_entry_with_table(const address *table);

  // Returns the base address of the CP, for accessing constants
  // like raw_constants_base()[i]
  OopDesc** raw_constants_base() const {
    OopDesc *c = constants();
#ifdef AZZERT
    ConstantPool::Raw checkit = c;
#endif
    return DERIVED(OopDesc**, c, ConstantPool::base_offset());
  }

public:
  void set_constants(ConstantPool* value) {
    obj_field_put(constants_offset(), (Oop*)value);
  }

  // (verifier or GC) stackmaps of this method
  ReturnOop stackmaps() const;

  void set_stackmaps(Oop* value) {
    obj_field_put(stackmaps_offset(), value);
  }
  void clear_stackmaps() {
    obj_field_clear(stackmaps_offset());
  }

  // exception table of this method
  ReturnOop exception_table() const;
  void set_exception_table(Oop* value) {
    obj_field_put(exception_table_offset(), value);
  }

#if ENABLE_REFLECTION
  ReturnOop thrown_exceptions() const {
    return obj_field(thrown_exceptions_offset());
  }
  void set_thrown_exceptions(Oop* value) {
    obj_field_put(thrown_exceptions_offset(), value);
  }
#endif

  // holder class of this method
  ReturnOop holder() const {
    GUARANTEE(ushort_field(holder_id_offset()) != 0xFFFF,
      "Attempt to access uninitialized holder id");
    return Universe::class_from_id(ushort_field(holder_id_offset()));
  }

  ReturnOop holder(Thread *thread) {
#if ENABLE_ISOLATES
    GUARANTEE(holder_id() != 0xFFFF,
              "Attempt to access uninitialized holder id");
    GUARANTEE(thread->task_id() != -1, "Dead task");
    Task::Raw t = Task::get_task(thread->task_id());
    GUARANTEE(!t.is_null(), "Null task");
    ObjArray::Raw cl = t().class_list();
    GUARANTEE(!cl.is_null() && cl().length() > holder_id(),
              "Illegal class id");
    return cl().obj_at(ushort_field(holder_id_offset()));
#else
    (void)thread;
    return holder();
#endif
  }

  jushort holder_id() const {
    return ushort_field(holder_id_offset());
  }
  void set_holder_id(jushort index) {
    ushort_field_put(holder_id_offset(), index);
  }

#if  ENABLE_JVMPI_PROFILE 
   // get the method id
   jint method_id() {
     return uint_field(method_id_offset());
   }
   
   // set the method id
   void set_method_id(juint index){
     uint_field_put(method_id_offset(), index);
   }
#endif

  MethodVariablePart* variable_part() const {
    return method()->variable_part();
  }

  // execution entry accessors:
  address execution_entry() const {    
    return method()->execution_entry();
  }

  void set_execution_entry(address entry) {
    method()->set_execution_entry(entry);
  }
  void set_compiled_execution_entry(address compiled_entry) {
#if ENABLE_THUMB_COMPILER
    // The low bit is set to 0x1 so that BX will automatically switch into
    // THUMB mode.
    compiled_entry += 1;
#endif
    method()->set_execution_entry(compiled_entry);
    GUARANTEE(has_compiled_code(), "sanity check");
  }

#if ENABLE_ROM_JAVA_DEBUGGER
  ReturnOop line_var_table() {
    return obj_field(line_var_table_offset());
  }

  void set_line_var_table(Oop *value) {
    obj_field_put(line_var_table_offset(), value);
  }
#endif

#if ENABLE_METHOD_TRAPS
  inline MethodTrapDesc* get_trap() const {
    return variable_part()->get_trap();
  }
#endif

  // native code accessors:
  void set_native_code(address code) { 
    GUARANTEE(GenerateROMImage || (code != NULL), "Sanity check"); 
    // The native code is guaranteed to be BytesPerWord aligned
    GUARANTEE(native_code_offset() % BytesPerWord == 0, "bad native alignment");
    *(address *)field_base(native_code_offset()) = code;
  }
  address get_native_code() { 
    return *(address *)field_base(native_code_offset());
  }

  void set_quick_native_code(address code) { 
    GUARANTEE(!uses_monitors(), "must not be synchronized method");
    GUARANTEE(GenerateROMImage || (code != NULL), "Sanity check"); 
    GUARANTEE(GenerateROMImage || is_quick_native(),
                  "misuse of overloaded field");
    *(address *)field_base(quick_native_code_offset()) = code;
  }
  address get_quick_native_code() { 
    GUARANTEE(GenerateROMImage || is_quick_native(),
              "misuse of overloaded field");
    return *(address *)field_base(quick_native_code_offset());
  }

  void bytecode_at_put_raw(jint bci, Bytecodes::Code bc) {
    ubyte_field_put(bc_offset_for(bci), (jubyte) bc);
  }

  // byte code accessors:
  void bytecode_at_put(jint bci, Bytecodes::Code bc) {
    if (_debugger_active) {
      if ((Bytecodes::Code)ubyte_field(bc_offset_for(bci)) == Bytecodes::_breakpoint) {
        VMEvent::replace_event_opcode(this, bc, bci);
      } else {
        ubyte_field_put(bc_offset_for(bci), (jubyte) bc);
      }
    } else {
      ubyte_field_put(bc_offset_for(bci), (jubyte) bc);
    }
  }

  void byte_at_put(jint bci, jint value) {
    ubyte_field_put(bc_offset_for(bci), (jubyte) value);
  }

  Bytecodes::Code bytecode_at_raw(jint bci)
  {
    return (Bytecodes::Code) ubyte_field(bc_offset_for(bci));
  }

  Bytecodes::Code bytecode_at(const jint bci) const {
    if (_debugger_active) {
      if ((Bytecodes::Code)ubyte_field(bc_offset_for(bci)) == Bytecodes::_breakpoint) {
        return VMEvent::get_verifier_breakpoint_opcode(this, bci);
      } else {
        return (Bytecodes::Code) ubyte_field(bc_offset_for(bci));
      }
    } else {
      return (Bytecodes::Code) ubyte_field(bc_offset_for(bci));
    }
  }

  // Returns the handler bci for a given exception class and a given bytecode
  // index. Returns -1 if no handler is available.
  jint exception_handler_bci_for(JavaClass* exception_class,
                                 jint bci JVM_TRAPS);

  // Does any exception handle cover bci for *any* exception type?
  bool exception_handler_exists_for(jint bci);

  // If the first exception handler for the indicated bci is an "any", then
  // return the handler bci.  Otherwise, return -1
  jint exception_handler_if_first_is_any(jint bci);

  bool is_local(int index) const {
    return (0 <= index && index < max_locals());
  }

  // Accessors for indices

  jubyte  ubyte_at(jint bci) {
    return ubyte_field(bc_offset_for(bci));
  }

  void ubyte_at_put(jint bci, jubyte value) {
    ubyte_field_put(bc_offset_for(bci), value);
  }
    
  // cannot inline, must check for unaligned access
  jushort ushort_at(jint bci);

  // Inlined index resolving.
  jushort get_Java_u2_index_at(jint bci);
  jushort resolve_u2_index_at(jint bci);
  juint   resolve_u4_index_at(jint bci);

  void write_u2_index_at(jint bci, jushort index);

  int itable_index() const;

#if ENABLE_ROM_GENERATOR
  void set_has_compressed_header() {
    AccessFlags af = access_flags();
    af.set_has_compressed_header();
    set_access_flags(af);
  }
  void set_has_no_stackmaps() {
    AccessFlags af = access_flags();
    af.set_has_no_stackmaps();
    set_access_flags(af);
  }
  void set_has_no_exception_table() {
    AccessFlags af = access_flags();
    af.set_has_no_exception_table();
    set_access_flags(af);
  }

  ReturnOop create_other_endianness(JVM_SINGLE_ARG_TRAPS);
#endif

  bool is_leaf() const {
    return !access_flags().has_invoke_bytecodes();
  }
  bool is_double_size( void ) const {
    return access_flags().is_double_size();
  }

  void set_double_size( void ) {
    if( !JavaClass::Raw( holder() )().access_flags().is_romized() ) {
      AccessFlags af = access_flags();
      af.set_double_size();
      set_access_flags(af);
    }
  }

  // Returns the compiled code for this method.
  ReturnOop compiled_code() const;

  // Tells whether it is impossible to compile this method
  bool is_impossible_to_compile (void) const;
  void set_impossible_to_compile(void);
  
  // Tells whether the method has been compiled
  bool has_compiled_code(void) const {
    return method()->has_compiled_code();
  }

  bool can_be_compiled( void ) const {
    return !is_impossible_to_compile() && !has_compiled_code();
  }

  void set_fixed_entry(address /*entry*/) ROMIZED_PRODUCT_RETURN;

  void unlink_compiled_code();

  // Tells whether the method uses monitors during execution
  bool uses_monitors() const {
    return access_flags().is_synchronized()
        || access_flags().has_monitor_bytecodes();
  }

  jushort compute_size_of_parameters();

  // Tells whether you can change the bytecodes of this method
  bool may_be_quickened() const {
    return !(ROM::in_any_loaded_readonly_bundle(obj()));
  }
  // Sets the default entry (used by ClassFileParser)
  void set_default_entry(bool);

  // Updates default entries if -comp is specified in command-line
  static void update_rom_default_entries();

  // Sets the default entry for executing this method in
  // interpreter mode.
  void set_default_interpreter_entry();

  // Sets the fixed entry for executing this method in interpreter
  // mode, which also means this method should never be compiled.
  void set_fixed_interpreter_entry();

  // If possible, set the execution entry to corresponding
  // shared_fast_getXXX_accessor
  void set_fast_accessor_entry(JVM_SINGLE_ARG_TRAPS);
#if ENABLE_INLINE && ARM
  bool is_fast_get_accessor(BasicType& type, int& offset) const;
#endif
  // Is this method a converted fast accessor? If so, compiler can do inlining
  bool is_fast_get_accessor() const;

  // Check if this method can be converted to a fast get
  // accessor. This is called immediately after a class is verified
  // but before any bytecodes in this method is quickened.
  bool may_convert_to_fast_get_accessor(BasicType& type, int& offset JVM_TRAPS);
#if ENABLE_INLINE && ARM
  bool bytecode_inline_filter(bool& has_field_get, int& index JVM_TRAPS);
  bool bytecode_inline_prepass(JVM_SINGLE_ARG_TRAPS);
#endif
  // Try to resolve a getfield/setfield bytecode, whose <index> operand
  // is located at <index_bci>. This is done without causing any class
  // loading or initialization.
  bool try_resolve_field_access(int index_bci, BasicType& type, int& offset,
                                bool is_static, bool is_get JVM_TRAPS);

  int fast_accessor_offset() const {
    return ushort_field(fast_accessor_offset_offset());
  }

  void set_fast_accessor_offset(jushort value) {
    ushort_field_put(fast_accessor_offset_offset(), value);
  }

  BasicType fast_accessor_type() const {
    return (BasicType)ushort_field(fast_accessor_type_offset());
  }

  void set_fast_accessor_type(BasicType value) {
    short_field_put(fast_accessor_type_offset(), (short) value);
  }

  // Returns the name of the method
  ReturnOop name() const;

  // Returns the name of the method
  ReturnOop signature() const;

  bool is_abstract()        const { return access_flags().is_abstract();     }
  bool is_native()          const { return access_flags().is_native();       }
  bool is_private()         const { return access_flags().is_private();      }
  bool is_final()           const { return access_flags().is_final();        }
  bool is_static()          const { return access_flags().is_static();       }
  bool is_public()          const { return access_flags().is_public();       }
  bool is_protected()       const { return access_flags().is_protected();    }
  bool is_package_private() const {
    return access_flags().is_package_private();
  }

  bool is_quick_native() const {
    return ((code_size() == 0) && is_native());
  }

  void check_access_by(InstanceClass* sender_class, 
                       InstanceClass* static_receiver_class, 
                       FailureMode fail_mode JVM_TRAPS);

  bool can_access_by(InstanceClass* sender_class, 
             InstanceClass* static_receiver_class);

  bool match(Symbol* name, Symbol* signature);
  void check_bytecodes(JVM_SINGLE_ARG_TRAPS);
  bool is_object_initializer();

  // Computes whether this method is a vanilla constructor. 
  // Used for setting the has_vanilla_constructor flag in the class.
  bool is_vanilla_constructor();

  // Iterators for invoking the BytecodeClosure
  void iterate(int begin, int end, BytecodeClosure* blk JVM_TRAPS);
  void iterate_bytecode(int bci, BytecodeClosure* blk, Bytecodes::Code code
                        JVM_TRAPS);
  void iterate_uncommon_bytecode(int bci, BytecodeClosure* blk JVM_TRAPS);

#if !defined(PRODUCT) || USE_PRODUCT_BINARY_IMAGE_GENERATOR || \
       ENABLE_PERFORMANCE_COUNTERS || ENABLE_JVMPI_PROFILE
  ReturnOop get_original_name(bool& renamed);
  // Print the name of the method in <class>.<name>() format. The
  // method signature is printed only if necessary (to distinguish between
  // overloaded methods).
#else
  ReturnOop get_original_name(bool& renamed) {
    renamed = false;
    return name();
  }
#endif

  inline ReturnOop get_original_name() {
    bool dummy;
    return get_original_name(dummy);
  }

  bool is_overloaded();

#ifdef PRODUCT
  void print_name_on_tty() PRODUCT_RETURN;
#else
  void print_name_on_tty() {
    print_name_on(tty);
  }
  void print_name_on(Stream* st) {
    print_name_on(st, false);
  }
  void print_name_on(Stream* st, bool long_output);
  void iterate(OopVisitor* visitor);
  void print_bytecodes(Stream* st) {
    print_bytecodes(st, 0, code_size());
  }

  void print_bytecodes(Stream* st, int bci) {
    int end = bci + Bytecodes::length_for(this, bci);
    print_bytecodes(st, bci, end, false, false);
  }

  void print_bytecodes(Stream* st, int start, int end, bool include_nl = true,
                       bool verbose = true);
  static void iterate_oopmaps(oopmaps_doer do_map, void* param);
#endif

#if !defined(PRODUCT) || ENABLE_PERFORMANCE_COUNTERS
  void print_name_to(char *buffer, int length);
#endif


#if ENABLE_COMPILER
  // Compiled the method and installs the compile code.
  // Returns whether the compilation succeeded.
  bool compile(int active_bci, bool resume JVM_TRAPS);
private:
  inline bool resume_compilation(JVM_SINGLE_ARG_TRAPS);
public:
#endif

  // Accessors for byte values
  jubyte  get_ubyte (const int bci) const {
    return ubyte_field(bc_offset_for(bci));
  }
  jbyte   get_byte (const int bci) const {
    return byte_field (bc_offset_for(bci));
  }

  // Accessors for java-ordered shorts and ints
  jushort get_java_ushort(const int bci) const {
    return (jushort)Bytes::get_Java_u2((address)field_base(bc_offset_for(bci)));
  }
  jshort  get_java_short (const int bci) const {
    return (jshort)Bytes::get_Java_u2((address)field_base(bc_offset_for(bci)));
  }
  juint   get_java_uint  (const int bci) const {
    return (juint)Bytes::get_Java_u4((address)field_base(bc_offset_for(bci)));
  }
  jint    get_java_int   (const int bci) const {
    return (jint)Bytes::get_Java_u4((address)field_base(bc_offset_for(bci)));
  }

  jint    get_native_aligned_int(const int bci) const {
    return int_field(bc_offset_for(bci));
  }
      
  void    put_native_aligned_int(int bci, int value) {
    int_field_put(bc_offset_for(bci), value);
  }

  jint    get_java_switch_int(int bci) const { 
    int value = get_native_aligned_int(bci);
    if (!ENABLE_NATIVE_ORDER_REWRITING
            && Bytes::is_Java_byte_ordering_different()) {
      // int has been left in its original wrong ordering
      return Bytes::swap_u4(value);
    } else { 
      // int was either already in native ordering, or we have swapped it.
      return value;
    } 
  }

  void    put_java_switch_int(int bci, int value) {
    if (!ENABLE_NATIVE_ORDER_REWRITING &&
           Bytes::is_Java_byte_ordering_different()) {
      // int must be put into the wrong order
      put_native_aligned_int(bci, Bytes::swap_u4(value));
    } else { 
      put_native_aligned_int(bci, value);
    } 
  }

  void put_java_ushort(int bci, jushort value) {
    Bytes::put_Java_u2((address)field_base(bc_offset_for(bci)), value);
  }
  void put_java_short(int bci, jshort value) {
    Bytes::put_Java_u2((address)field_base(bc_offset_for(bci)), (jushort) value);
  }
  void put_java_uint(int bci, juint value) {
    Bytes::put_Java_u4((address)field_base(bc_offset_for(bci)), value);
  }
  void put_java_int(int bci, jint value) {
    Bytes::put_Java_u4((address)field_base(bc_offset_for(bci)), value);
  }

  jushort get_native_ushort(const int bci) const {
   return (jushort)Bytes::get_native_u2((address)field_base(bc_offset_for(bci)));
  }
  jshort get_native_short(const int bci) const {
    return (jshort)Bytes::get_native_u2((address)field_base(bc_offset_for(bci)));
  }
  juint get_native_uint(const int bci) const {
    return (juint)Bytes::get_native_u4((address)field_base(bc_offset_for(bci)));
  }
  jint get_native_int(const int bci) const {
    return (jint)Bytes::get_native_u4((address)field_base(bc_offset_for(bci)));
  }

  void put_native_ushort(int bci, jushort value) {
    Bytes::put_native_u2((address)field_base(bc_offset_for(bci)), value);
  }
  void put_native_short(int bci, jshort value) {
    Bytes::put_native_u2((address)field_base(bc_offset_for(bci)), (jushort)value);
  }
  void put_native_uint(int bci, juint value) {
    Bytes::put_native_u4((address)field_base(bc_offset_for(bci)), value);
  }
  void put_native_int(int bci, jint value) {
    Bytes::put_native_u4((address)field_base(bc_offset_for(bci)), value);
  }

  void bytecode_at_flush_icache(jint bci, Bytecodes::Code bc) {
    if (ENABLE_BYTECODE_FLUSHING) {
      OsMisc_flush_icache((address) obj()->ubyte_field_addr(bc_offset_for(bci)),
                          Bytecodes::length_for(bc) );
    }
  }
 
  void code_flush_icache(void) {
    if (ENABLE_BYTECODE_FLUSHING) {
      OsMisc_flush_icache((address) code_base(), code_size());
    }
  }
#if ENABLE_CSE
  bool check_codes(jint begin_bci, jint end_bci, int& local_mask, int& constant_mask, int& array_type);
  bool compare_bytecode(const jint begin_bci, const jint end_bci, jint cur_bci, jint& next_bci);
  void kill_changed_values(int bci, Bytecodes::Code code) ;
#endif


 private:

  void iterate_push_constant_1(int i, BytecodeClosure* blk JVM_TRAPS);
  void iterate_push_constant_2(int i, BytecodeClosure* blk JVM_TRAPS);

  friend class ClassFileParser;
  friend class Bytecodes;
  friend class BasicBlock;
  friend class JavaFrame;
  friend class VMEvent;
};
