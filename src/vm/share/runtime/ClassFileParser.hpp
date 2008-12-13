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

/** \class ClassFileParser
    Parser for class files.
    The bytes describing a class file structure are read from a Buffer object
    and corresponding internal structures describing a class are constructed.
    Many class file format checks are performed.

*/

class ClassFileParser: public StackObj {
 public:
  // Constructor
  ClassFileParser(Buffer* buffer, LoaderContext *loader_ctx) {
    _buffer            = buffer;
    set_buffer_position(0);
    _loader_ctx        = loader_ctx;
    _name              = loader_ctx->class_name();
#ifdef USE_CLASS_LOADER
    _class_loader      = loader_ctx->_class_loader;
    _protection_domain = loader_ctx->_protection_domain;
#endif

    // Push this on the static class file parser list
    _previous          = _head;
    _head              = this;
  }

  ~ClassFileParser( void ) {
    // Pop this from the static class file parser list
    _head = _previous;
  }

  // Parse class file and return new InstanceClass. The InstanceClass
  // is not hooked up to the system dictionary or any other
  // structures, so a .class file can be loaded several times if
  // desired. The system dictionary hookup is done by ClassLoader.
  ReturnOop parse_class(ClassParserState *state JVM_TRAPS) {
    EventLogger::start( EventLogger::LOAD_CLASS );
#if ENABLE_MONET
    const int class_id = Universe::number_of_java_classes();
#endif
    OopDesc* klass = parse_class_internal(state JVM_NO_CHECK);
#if ENABLE_MONET
    // If converting, remove this class from the class_list if there was
    // an exception.  Otherwise when we iterate through the list of
    // system classes we will get a NULL InstanceClass
    if (CURRENT_HAS_PENDING_EXCEPTION) {
      if (GenerateROMImage && Universe::number_of_java_classes() == class_id + 1){
        Universe::unregister_last_java_class();
      }
    }
#endif
    EventLogger::end( EventLogger::LOAD_CLASS );
    return klass;
  }

  static bool is_package_restricted(Symbol *class_name);
  // Needed for lazy error throwing
  static ReturnOop new_lazy_error_method(Method* method,
                                         address native_function JVM_TRAPS);
  static void gc_prologue( void );
  static void gc_epilogue( void );
 private:
  TypeArray*       _buffer;
  jubyte*          _bufptr;
  jubyte*          _bufend;
  LoaderContext*   _loader_ctx;
  Symbol*          _name;
  ClassFileParser* _previous;
  static ClassFileParser* _head;

  Symbol* name()              { return _name;              }
#ifdef USE_CLASS_LOADER
  Oop*             _class_loader;
  Oop*             _protection_domain;
  Oop*    class_loader()      { return _class_loader;      }
  Oop*    protection_domain() { return _protection_domain; }
#endif
  ClassFileParser* previous() { return _previous;          }

  // Throws an exception if we have circular class parsing
  void check_for_circular_class_parsing(ClassParserState *stack JVM_TRAPS);
  void check_for_duplicate_methods(ConstantPool *cp, const ObjArray& methods 
                                   JVM_TRAPS);
  void check_for_duplicate_fields(ConstantPool* cp, const TypeArray& fields
                                  JVM_TRAPS);

  // Accessors to buffer stream.
  // with buffer bounds checks
  jubyte  get_u1(JVM_SINGLE_ARG_TRAPS);
  jushort get_u2(JVM_SINGLE_ARG_TRAPS);
  juint   get_u4(JVM_SINGLE_ARG_TRAPS);

  // Skip length jubyte or jushort elements from stream
  // checking buffer bounds
  void skip_u1(int length JVM_TRAPS);
  void skip_u2(int length JVM_TRAPS);

  jint get_buffer_position() {
    return _bufptr - (jubyte*)_buffer->base_address();
  }
  jint available_bytes() {
    return _bufend - _bufptr;
  }
  void set_buffer_position(int pos);

  // This can only be used when executing memcpy
  // Make sure that buffer bounds have been checked before using this
  jubyte* raw_u1_buffer(jint position) {
    return (_buffer->base_address() + position );
  }

  static bool are_valid_field_access_flags(const AccessFlags field_access_flags,
                                           const AccessFlags class_access_flags);
  static bool are_valid_method_access_flags(ClassParserState *state, Symbol *name, 
                                            const AccessFlags method_access_flags,
                                            const AccessFlags class_access_flags);

  static inline bool is_circular(InstanceClass* this_class);
  static inline void check_local_interfaces(InstanceClass* this_class JVM_TRAPS);

  // Constant pool parsing
  bool is_valid_utf8_in_buffer(jint utf8_length);
  void parse_constant_pool_utf8_entry(ConstantPool* cp, int index JVM_TRAPS);
  void parse_constant_pool_integer_entry(ConstantPool* cp, int index JVM_TRAPS);
  void parse_constant_pool_float_entry(ConstantPool* cp, int index JVM_TRAPS);
  void parse_constant_pool_long_entry(ConstantPool* cp, int index JVM_TRAPS);
  void parse_constant_pool_double_entry(ConstantPool* cp, int index JVM_TRAPS);
  void parse_constant_pool_class_entry(ConstantPool* cp, int index JVM_TRAPS);
  void parse_constant_pool_string_entry(ConstantPool* cp, int index JVM_TRAPS);
  void parse_constant_pool_fieldref_entry(ConstantPool* cp, int index JVM_TRAPS);
  void parse_constant_pool_methodref_entry(ConstantPool* cp, int index JVM_TRAPS);
  void parse_constant_pool_interfacemethodref_entry(ConstantPool* cp, 
                                                    int index JVM_TRAPS);
  void parse_constant_pool_nameandtype_entry(ConstantPool* cp, int index
                                             JVM_TRAPS);
  void parse_constant_pool_entry(ConstantPool* cp, int& index JVM_TRAPS);
  void parse_constant_pool_entries(ConstantPool* cp JVM_TRAPS);
  void validate_and_fixup_constant_pool_entries(ConstantPool* cp JVM_TRAPS);
  void remove_unused_utf8_entries(ConstantPool* cp);
  ReturnOop parse_constant_pool(JVM_SINGLE_ARG_TRAPS);

  // Interface parsing
  ReturnOop parse_interface_indices(ClassParserState *stack,
                                    ConstantPool* cp, bool& resolved JVM_TRAPS);
  ReturnOop parse_interfaces(ConstantPool* cp, TypeArray* interface_indices
                             JVM_TRAPS);
  static int count_interfaces(OopDesc* local_interface_indices, OopDesc* bitmap);
  static int found_all_interfaces(OopDesc* local_interface_indices, OopDesc* all_interface_indices, 
                                  int already_in_table, OopDesc* bitmap);

  // Field parsing
  void parse_field_attributes(ConstantPool* cp, 
                              jushort& constantvalue_index_addr,
                              FieldType* type, bool& is_synthetic_addr JVM_TRAPS);
  ReturnOop parse_fields(ConstantPool* cp, int& nonstatic_field_size, 
                         int& static_field_size, 
                         AccessFlags& class_access_flags JVM_TRAPS);

  // Method parsing
  ReturnOop parse_method(ClassParserState *state, ConstantPool* cp, 
                         const AccessFlags class_access_flags JVM_TRAPS);
  ReturnOop parse_methods(ClassParserState *state, ConstantPool* cp, 
                          const AccessFlags class_access_flags, 
                          bool& has_native_methods JVM_TRAPS);

  ReturnOop parse_exception_table(int code_length, ConstantPool* cp JVM_TRAPS);
  ReturnOop parse_code_attributes(ConstantPool* cp, jushort max_stack,
                                  jushort max_locals, juint code_length,
                                  LineVarTable *line_var_table JVM_TRAPS);
  static void init_native_method(Method *method, address native_function);

  // Stackmap parsing
  juint parse_stackmaps(ConstantPool* cp, 
                        jushort num_stackmaps, jushort max_stack, 
                        jushort frame_size, juint code_length, 
                        ObjArray* stackmaps JVM_TRAPS);

  // Classfile attribute parsing
  void parse_classfile_sourcefile_attribute(ConstantPool* cp JVM_TRAPS);
  void parse_classfile_inner_classes_attribute(ConstantPool* cp,
                                               InstanceClass* c JVM_TRAPS);
  void parse_classfile_attributes(ConstantPool* cp, InstanceClass* c JVM_TRAPS);
  void parse_classfile_synthetic_attribute(InstanceClass* c) {
    c->set_is_synthetic();
  }
  
  ReturnOop parse_class_internal(ClassParserState *state JVM_TRAPS);
  bool parse_class_0(ClassParserState *state JVM_TRAPS);

  static void clone_invoke_special_virtual_conflicts(InstanceClass* this_klass,
    ConstantPool* old_cp, ConstantPool* new_cp,
    TypeArray* relocation_map, int delta);  
  static void fill_in_invoke_indexes(InstanceClass* this_class,
    TypeArray* invoke_sp_ids, TypeArray* invoke_vi_ids);
  static void resolve_invoke_special_virtual_conflicts(InstanceClass* this_class
    JVM_TRAPS);

  // Field and oopmap offset computation
  void update_fields(ConstantPool* cp, TypeArray* fields, bool is_static,
                     int next_offset, int prev_oop_offset, TypeArray* map,
                     int& map_size, int map_index);

  void print_on(Stream *) PRODUCT_RETURN;
  void p() PRODUCT_RETURN;

#if ENABLE_ROM_GENERATOR
private:
  static int _total_classfile_bytes;
  static int _total_bytecode_bytes;
  static int _total_stackmap_bytes;

public:
  /**
   * Returns the size of all the classfiles that have been loaded for
   * the entire lifetime of the VM executable. This is mainly used by
   * the Romizer to calculate statistics.
   */
  static int total_classfile_bytes() {
    return _total_classfile_bytes;
  }

  /**
   * Returns the size of all the bytecodes in loaded classfiles for
   * the entire lifetime of the VM executable. This is mainly used by
   * the Romizer to calculate statistics.
   */
  static int total_bytecode_bytes() {
    return _total_bytecode_bytes;
  }

  /**
   * Returns the size of all the verifier stackmaps in loaded
   * classfiles for the entire lifetime of the VM executable. This is
   * mainly used by the Romizer to calculate statistics.
   */
  static int total_stackmap_bytes() {
    return _total_stackmap_bytes;
  }
#endif // ENABLE_ROM_GENERATOR

friend class SystemDictionary;
};
