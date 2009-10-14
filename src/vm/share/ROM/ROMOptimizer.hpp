/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#if ENABLE_ROM_GENERATOR

class ROMTableInfo {
protected:
  ObjArray* _heap_table;
  int _count;
public:
  ROMTableInfo(ObjArray *array);
  ObjArray* heap_table(void) const {
    return _heap_table;
  }
  int num_buckets(void) const;
  int count(void) const {
    return _count;
  }
  virtual juint hash(Oop * /*object*/) JVM_PURE_VIRTUAL_0;
};

/** \class ROMHashtableManager
 * Manages the contents of the hashtables of Strings and Symbols in
 * the ROM image. 
 */
class ROMHashtableManager {
public:
  ROMHashtableManager() {
    _string_count = 0;
    _symbol_count = 0;
  }
  void initialize(ObjArray* symbol_table, ObjArray* string_table JVM_TRAPS);
  ReturnOop init_symbols(ObjArray* symbol_table JVM_TRAPS);
  ReturnOop init_strings(ObjArray* string_table JVM_TRAPS);
  ReturnOop init_rom_hashtable(ROMTableInfo &info JVM_TRAPS);
  void add_to_bucket(ObjArray *rom_table, int index, Oop *object);

  // Total number of strings in the string table
  int string_count(void) const {
    return _string_count;
  }
  // Total number of symbols in the symbol table
  int symbol_count(void) const {
    return _symbol_count;
  }

  ReturnOop string_table(void) const {
    return _string_table.obj();
  }
  ReturnOop symbol_table() {
    return _symbol_table.obj();
  }

  void set_embedded_hashtables(const ConstantPool* holder, int strings_offset,
                               int symbols_offset) {
    _embedded_table_holder = holder->obj();
    _embedded_strings_offset = strings_offset;
    _embedded_symbols_offset = symbols_offset;
  }

  ReturnOop embedded_table_holder(void) const {
    return _embedded_table_holder.obj();
  }

  int embedded_strings_offset(void) const {
    return _embedded_strings_offset;
  }
  int embedded_symbols_offset(void) const {
    return _embedded_symbols_offset;
  }
private:
  ObjArray _string_table;
  ObjArray _symbol_table;

  int _string_count;
  int _symbol_count;

  ConstantPool _embedded_table_holder;
  int _embedded_symbols_offset;
  int _embedded_strings_offset;
};

class ROMOptimizer {
#if USE_SOURCE_IMAGE_GENERATOR
  #if ENABLE_MULTIPLE_PROFILES_SUPPORT
    #define MPS_ROMOPTIMIZER_OOP_FIELDS_DO(template)  \
      template(ROMProfile, global_profile,  "")       \
      template(ROMProfile, current_profile, "")       \
      template(ROMVector,  profiles_vector, "")
  #else 
    #define MPS_ROMOPTIMIZER_OOP_FIELDS_DO(template)
  #endif // ENABLE_MULTIPLE_PROFILES_SUPPORT

  #if ENABLE_MEMBER_HIDING
    #define MEMBER_HIDING_ROMOPTIMIZER_OOP_FIELDS_DO(template)  \
      template(ROMVector, hidden_field_classes,     "") \
      template(ROMVector, hidden_field_names,       "") \
      template(ROMVector, hidden_method_classes,    "") \
      template(ROMVector, hidden_method_names,      "") \
      template(ROMVector, hidden_method_signatures, "")
  #else
    #define MEMBER_HIDING_ROMOPTIMIZER_OOP_FIELDS_DO(template)
  #endif // ENABLE_MEMBER_HIDING

  #define SOURCE_ROMOPTIMIZER_OOP_FIELDS_DO(template) \
    MPS_ROMOPTIMIZER_OOP_FIELDS_DO(template)          \
    MEMBER_HIDING_ROMOPTIMIZER_OOP_FIELDS_DO(template)\
    template(ROMVector, hidden_classes, "")           \
    template(ROMVector, hidden_packages, "")          \
    template(ROMVector, restricted_packages, "")
#else 
  #define SOURCE_ROMOPTIMIZER_OOP_FIELDS_DO(template)
#endif // USE_SOURCE_IMAGE_GENERATOR

#define ROMOPTIMIZER_OOP_FIELDS_DO(template)                                  \
  SOURCE_ROMOPTIMIZER_OOP_FIELDS_DO(template)                                 \
  template(TypeArray,empty_short_array, "")                                   \
  template(ObjArray, empty_obj_array, "")                                     \
  template(ObjArray, init_at_build_classes, "Classes that should be "         \
                                            "initialized at build time")      \
  template(ObjArray, init_at_load_classes,  "Classes that should be "         \
                                            "initialized VM load time")       \
  template(ObjArray, dont_rename_fields_classes, "Don't rename private"       \
                                            "fields in these classes")        \
  template(ObjArray, dont_rename_methods_classes,"Don't rename private"       \
                                            "methods in these classes")       \
  template(ObjArray, dont_rename_classes,   "Don't rename these classes,"     \
                                            "even if they belong to a hidden" \
                                            "package")                        \
  template(ObjArray ,romizer_original_class_name_list, "Original names of"    \
                                            "classes we've renamed.")         \
  template(ObjArray, romizer_original_method_info, "Original names/signatures"\
                                            "of methods that we've renamed")  \
  template(ObjArray, romizer_original_fields_list, "Original names/signatures"\
                                            "of fields that we've renamed")   \
  template(ConstantPool,romizer_alternate_constant_pool, "")                  \
  template(ObjArray, kvm_native_methods_table, "Methods that use KVM-style "  \
                                            "native interface")               \
  template(ObjArray, jni_native_methods_table, "Methods that use JNI-style "  \
                                            "native interface")               \
  template(ROMVector,    precompile_method_list, "")                          \
  template(ObjArray,     string_table, "")                                    \
  template(ObjArray,     symbol_table, "")                                    \
  template(ConstantPool, embedded_table_holder, "")                           \
  template(ObjArray,     subclasses_array, "")                                \
  template(ROMVector,    reserved_words, "")                                  \
  template(TypeArray,    direct_interface_implementation_cache,               \
                                     "for each interface contains class_id of \
                                      implementing class, or -1 in case of    \
                                      multiple classes")                      \
  template(TypeArray,    interface_implementation_cache,                      \
                         "for each interface contains class_id of directly    \
                          implementing class, or -1 in case of                \
                          multiple classes")                                  \
  template(TypeArray,    extended_class_attributes,                           \
                         "Additional class attributes derived by ROMizer")

#if USE_SOURCE_IMAGE_GENERATOR
#define SOURCE_ROMOPTIMIZER_INT_FIELDS_DO(template) \
  template(int,                 if_level, "") \
  template(int,                 false_if_level, "") \
  template(int,                 profile_if_level, "") \
  template(int,                 config_parsing_line_number, "") \
  template(const JvmPathChar *, config_parsing_file, "")  
#else
#define SOURCE_ROMOPTIMIZER_INT_FIELDS_DO(template)
#endif

#define ROMOPTIMIZER_INT_FIELDS_DO(template) \
  SOURCE_ROMOPTIMIZER_INT_FIELDS_DO(template) \
  template(int, state, "") \
  template(int, embedded_symbols_offset, "") \
  template(int, embedded_strings_offset, "") \
  template(int, redundant_stackmap_count, "") \
  template(int, redundant_stackmap_bytes, "") \
  template(Stream*, log_stream, "") \
  template(ROMVector*, disable_compilation_log, "") \
  template(ROMVector*, quick_natives_log, "") \
  template(ROMVector*, jni_natives_log, "") \
  template(ROMVector*, kvm_natives_log, "") \
  template(ROMVector*, kvm_native_methods_vector, "")


#define ROMOPTIMIZER_DECLARE_OOP_GETTER(type, name, comment) \
  static type * name() { \
    return (type*)&_romoptimizer_oops[name ## _index]; \
  }

#define ROMOPTIMIZER_DECLARE_OOP_SETTER(type, name, comment) \
  static void set_ ## name(type* value) { \
    _romoptimizer_oops[name ## _index] = value->obj(); \
  } \
  static void set_ ## name(OopDesc* value) { \
    _romoptimizer_oops[name ## _index] = value; \
  }

#define ROMOPTIMIZER_DECLARE_INT(type, name, comment) \
  static type _ ## name;

#define ROMOPTIMIZER_DECLARE_INT_GETTER(type, name, comment) \
  static type name() { \
    return _ ## name; \
  }

#define ROMOPTIMIZER_DECLARE_INT_SETTER(type, name, comment) \
  static void set_ ## name(type value) { \
    _ ## name = value; \
  }

#define ROMOPTIMIZER_DEFINE_INT(type, name, comment) \
  type ROMOptimizer::_ ## name;

#define ROMOPTIMIZER_COUNT_FIELDS(type, name, comment) \
  name ## _index,

  // Count the number of integer and oop fields in the ROMOptimizer class
  enum {
    ROMOPTIMIZER_OOP_FIELDS_DO(ROMOPTIMIZER_COUNT_FIELDS)
    _number_of_oop_fields
  };
  enum {
    ROMOPTIMIZER_INT_FIELDS_DO(ROMOPTIMIZER_COUNT_FIELDS)
    _number_of_int_fields
  };
  static OopDesc* _romoptimizer_oops[_number_of_oop_fields];

// The order is important. Some optimizations depend on
// the results of earlier optimizations. Do not change.
#define ROMOPTIMIZER_STATES_DO(template)  \
  template(MAKE_RESTRICTED_PACKAGES_FINAL )\
  template(INITIALIZE_CLASSES             )\
  template(QUICKEN_METHODS                )\
  template(RESOLVE_CONSTANT_POOL          )\
  template(REMOVE_REDUNDATE_STACKMAPS     )\
  template(MERGE_STRING_BODIES            )\
  template(RESIZE_CLASS_LIST              )\
  template(REPLACE_EMPTY_ARRAYS           )\
  template(INLINE_METHODS                 )\
  template(OPTIMIZE_FAST_ACCESSORS        )\
  template(REMOVE_DEAD_METHODS            )\
  template(RENAME_NON_PUBLIC_SYMBOLS      )\
  template(REMOVE_UNUSED_STATIC_FIELDS    )\
  template(COMPACT_FIELD_TABLES           )\
  template(REMOVE_UNUSED_SYMBOLS          )\
  template(REWRITE_CONSTANT_POOLS         )\
  template(COMPACT_TABLES                 )\
  template(PRECOMPILE_METHODS             )\
  template(REMOVE_DUPLICATED_OBJECTS      )\
  template(MARK_HIDDEN_CLASSES            )

  enum {
    #define DEFINE_ROMOPTIMIZER_STATE(n) STATE_##n,
      ROMOPTIMIZER_STATES_DO(DEFINE_ROMOPTIMIZER_STATE)
    #undef DEFINE_ROMOPTIMIZER_STATE
    STATE_COUNT
  };

  static int _time_counters[STATE_COUNT];
public:
  ROMOptimizer() {

  }

  // Declare functions such as 
  // static ObjArray * dont_rename_classes();
  ROMOPTIMIZER_OOP_FIELDS_DO(ROMOPTIMIZER_DECLARE_OOP_GETTER)

  // Declare functions such as 
  // static void set_dont_rename_classes(ObjArray * value);
  ROMOPTIMIZER_OOP_FIELDS_DO(ROMOPTIMIZER_DECLARE_OOP_SETTER)

  // Declare fields such as the following (for old code to work)
  // static int _state;  
  ROMOPTIMIZER_INT_FIELDS_DO(ROMOPTIMIZER_DECLARE_INT)

  // Declare fields accessor such as the following (should be used by new code)
  // static int state()
  // static void set_state(int value)
  ROMOPTIMIZER_INT_FIELDS_DO(ROMOPTIMIZER_DECLARE_INT_GETTER)
  ROMOPTIMIZER_INT_FIELDS_DO(ROMOPTIMIZER_DECLARE_INT_SETTER)

  static void set_next_state(void) {
    set_state(state() + 1);
  }
  static bool is_active(void) {
    return state() < STATE_COUNT;
  }
  static bool is_done(void) {
    return state() >= STATE_COUNT;
  }
  static int number_of_states(void) {
    return STATE_COUNT;
  }
  
  static void init_handles();
  static void oops_do(void do_oop(OopDesc**));
  void initialize(Stream *log_stream JVM_TRAPS);
  void optimize(Stream *log_stream JVM_TRAPS);

  static ReturnOop original_name(int class_id) {
    if (romizer_original_class_name_list()->is_null() ||
        class_id >= romizer_original_class_name_list()->length()) {
      return NULL;
    }
    return romizer_original_class_name_list()->obj_at(class_id);
  }
  static ReturnOop original_method_info(int class_id) {
    if (romizer_original_method_info()->is_null() ||
        class_id >= romizer_original_method_info()->length()) {
      return NULL;
    }
    return romizer_original_method_info()->obj_at(class_id);
  }
  static ReturnOop original_fields(int class_id) {
    if (romizer_original_fields_list()->is_null() ||
        class_id >= romizer_original_fields_list()->length()) {
      return NULL;
    }
    return romizer_original_fields_list()->obj_at(class_id);
  }
  static ReturnOop alternate_constant_pool() {
    return romizer_alternate_constant_pool()->obj();
  }

  void initialize_hashtables(ObjArray* symbol_table_input,
                             ObjArray* string_table_input JVM_TRAPS) {
    ROMHashtableManager hashtab_mgr;
    hashtab_mgr.initialize(symbol_table_input, string_table_input JVM_CHECK);

    *string_table()          = hashtab_mgr.string_table();
    *symbol_table()          = hashtab_mgr.symbol_table();
    *embedded_table_holder() = hashtab_mgr.embedded_table_holder();
    _embedded_symbols_offset = hashtab_mgr.embedded_symbols_offset();
    _embedded_strings_offset = hashtab_mgr.embedded_strings_offset();
  }

  static void trace_failed_quicken(Method *method, 
                                   JavaClass *dependency JVM_TRAPS);
  static void process_quickening_failure(Method *method);
  bool may_be_initialized(const InstanceClass* klass) const;

#if USE_SOURCE_IMAGE_GENERATOR
  bool is_in_restricted_package (const InstanceClass* klass) const;
  bool is_in_hidden_package     (const InstanceClass* klass) const;
  bool is_hidden                (const InstanceClass* klass) const;
#else
  static bool is_in_restricted_package (const InstanceClass* ) {
    // IMPL_NOTE: Monet: all classes can be considered as restricted
    return false;
  }
  static bool is_in_hidden_package(const InstanceClass* ) {
    return false;
  }
  static bool is_hidden(const InstanceClass* ) {
    // IMPL_NOTE: Monet: all classes can be considered as restricted
    return false;
  }
#endif // USE_SOURCE_IMAGE_GENERATOR

  ReturnOop original_fields(const InstanceClass* klass, bool &is_orig);
  void set_classes_as_romized();
  bool is_overridden(const InstanceClass* ic, const Method *method) const;
#if USE_SOURCE_IMAGE_GENERATOR
  static bool class_matches_classes_list(const InstanceClass* klass,
                                         const ROMVector* patterns);
  static bool class_matches_packages_list(const InstanceClass* klass,
                                          const ROMVector* patterns);
#endif

#if USE_SOURCE_IMAGE_GENERATOR || (ENABLE_MONET && !ENABLE_LIB_IMAGES)
  void fill_interface_implementation_cache(void);
  void forbid_invoke_interface_optimization(const InstanceClass* cls,
                                            const bool indirect_only) const;
  void set_implementing_class(const int interface_id, const int class_id,
                              const bool only_childs, const bool direct) const;
  enum {
    NOT_IMPLEMENTED = -1, 
    FORBID_TO_IMPLEMENT = -2
  };
#endif

private:
  void log_time_counters();
  int  get_max_alternate_constant_pool_count();

#if ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR
  void set_profile( OopDesc* profile ) {
    set_current_profile( profile );
    #define ROMPROFILE_SET_VECTORS(type, name)\
      set_##name(current_profile()->name());
    ROMPROFILE_VECTORS_DO(ROMPROFILE_SET_VECTORS)
    #undef ROMPROFILE_SET_VECTORS
  }

  int find_profile(const char name[]);
#endif

#if USE_SOURCE_IMAGE_GENERATOR
  void set_global_profile( void ) {
#if ENABLE_MULTIPLE_PROFILES_SUPPORT
    set_profile( global_profile()->obj() );
#endif
  }

  bool is_in_profile( void ) const {
#if ENABLE_MULTIPLE_PROFILES_SUPPORT
    return current_profile()->obj() != global_profile()->obj();
#else
    return false;
#endif
  }

  void reset_false_if_level( void ) {
    set_false_if_level(max_jint);
  }
  int if_level_save( void ) {
    const int old_level = if_level();
    set_if_level(0);
    return old_level;
  }
  void if_level_restore( const int old_level ) {
    if( if_level() != 0 ) {
      config_error( "EndIf expected" );
    }
    set_if_level(old_level);
  }

  void read_config_file(JVM_SINGLE_ARG_TRAPS);
  void read_config_file(const JvmPathChar* config_file JVM_TRAPS);
  void read_hardcoded_config(JVM_SINGLE_ARG_TRAPS);
  void optimize_package_lists(JVM_SINGLE_ARG_TRAPS);
  static void abort( void );
  static void config_message_head( const char category[] );
  static void config_message( const char category[], const char msg[] );
  static void config_warning( const char msg[] );
  static void config_warning_not_found( const char name[] );
  static void config_error( const char msg[] );
  bool validate_class_pattern( const char pattern[] ) const;
  ReturnOop validate_package_pattern( const char pattern[] JVM_TRAPS ) const;
  bool validate_package_not_empty( const Symbol* package_name ) const;
  void process_config_line(char* config_line JVM_TRAPS);
  void include_config_file(const char* config_file JVM_TRAPS);
  static char parse_config(char* line, const char*& name, const char*& value);
  void add_class_to_list(ObjArray* list, const char classname[] JVM_TRAPS);
  void add_package_to_list(ROMVector* vector, Symbol* pkgname JVM_TRAPS);
  void remove_packages(ROMVector* from, const Symbol* pattern);
  void remove_packages(OopDesc* from_vector, const ROMVector* patterns);
  static bool class_list_contains(const ObjArray* list,
                                  const InstanceClass* klass);
  void write_methods_log(ROMVector* log, const char name[],
                                         const char prefix[]);
  void update_jni_natives_table(JVM_SINGLE_ARG_TRAPS) {
    *jni_native_methods_table() = build_method_table(_jni_natives_log 
                                                     JVM_NO_CHECK_AT_BOTTOM);
  }

  void update_kvm_natives_table(JVM_SINGLE_ARG_TRAPS) {
    *kvm_native_methods_table() = build_method_table(_kvm_natives_log 
                                                     JVM_NO_CHECK_AT_BOTTOM);
  }
  ReturnOop build_method_table(const ROMVector* methods JVM_TRAPS) const;

  bool dont_rename_class(const InstanceClass* klass) const {
    return class_list_contains(dont_rename_classes(), klass);
  }
  bool dont_rename_fields_in_class(const InstanceClass* klass) const {
    return class_list_contains(dont_rename_fields_classes(), klass);
  }
  bool dont_rename_methods_in_class(const InstanceClass* klass) const {
    return class_list_contains(dont_rename_methods_classes(), klass);
  }  

  bool is_init_at_build(const InstanceClass* klass) const;

#else
  bool dont_rename_class(InstanceClass* /*klass*/)            {return false;}
  bool dont_rename_fields_in_class(InstanceClass* /*klass*/)  {return false;}
  bool dont_rename_methods_in_class(InstanceClass* /*klass*/) {return false;}
#endif

  void disable_compilation(const char* pattern JVM_TRAPS);
  void allocate_empty_arrays(JVM_SINGLE_ARG_TRAPS);
  void make_restricted_packages_final(JVM_SINGLE_ARG_TRAPS);
  void make_restricted_methods_final(JVM_SINGLE_ARG_TRAPS) const;
  void make_virtual_methods_final(const InstanceClass* ic, ROMVector* log_vector
                                  JVM_TRAPS) const;
#if USE_SOURCE_IMAGE_GENERATOR
  static bool name_matches_pattern(const char name[], const int name_len,
                                   const ROMVector* patterns);
  static bool name_matches_pattern(const Symbol* s,
                                   const ROMVector* patterns);
#endif

  static bool has_subclasses(const InstanceClass* klass) ;
  void log_non_restricted_packages();
  void initialize_classes(JVM_SINGLE_ARG_TRAPS);
  void print_class_initialization_log(JVM_SINGLE_ARG_TRAPS);
  void quicken_methods(JVM_SINGLE_ARG_TRAPS);
  bool quicken_one_method(Method *method JVM_TRAPS);
  void optimize_fast_accessors(JVM_SINGLE_ARG_TRAPS);
  void merge_string_bodies(JVM_SINGLE_ARG_TRAPS);
  int  compress_and_merge_strings(ROMVector *all_strings, TypeArray* body);
  static void replace_string_bodies(const ROMVector* all_strings,
                                    const TypeArray* body);
#if !USE_PRODUCT_BINARY_IMAGE_GENERATOR
  jint find_duplicate_chars(jchar *pool, jint pool_size,
                            jchar *match, jint num_chars);
#endif
  void resize_class_list(JVM_SINGLE_ARG_TRAPS);

#if !USE_SOURCE_IMAGE_GENERATOR || !ENABLE_MEMBER_HIDING
  static bool is_hidden_method(const InstanceClass* /*ic*/,
                               const Method* /*method*/) {
    return false;
  }
  static bool is_hidden_field (const InstanceClass* /*ic*/,
                               const OopDesc* /*field*/) {
    return false;
  }
#endif

#if USE_SOURCE_IMAGE_GENERATOR
  void record_original_field_info(InstanceClass *klass, int name_index 
                                  JVM_TRAPS);
  void record_original_fields(InstanceClass *klass JVM_TRAPS);
  jushort get_index_from_alternate_constant_pool(const InstanceClass* klass,
                                                 const jushort symbol_index);
#else
  void record_original_field_info(InstanceClass* /*klass*/, int /*name_index*/
                                  JVM_TRAPS) {JVM_IGNORE_TRAPS;}
  void record_original_fields(InstanceClass* /*klass*/ JVM_TRAPS) 
       {JVM_IGNORE_TRAPS;}
#endif

  void replace_empty_arrays();
  void remove_unused_static_fields(JVM_SINGLE_ARG_TRAPS);

  void mark_static_fieldrefs                      (ObjArray *directory);
  void fix_static_fieldrefs                       (ObjArray *directory);
  void mark_unremoveable_static_fields            (ObjArray* directory);
  void compact_static_field_containers            (ObjArray *directory);
  void fix_field_tables_after_static_field_removal(ObjArray *directory);

  int compact_one_interface(InstanceClass* ic);
  void compact_interface_classes(JVM_SINGLE_ARG_TRAPS);
  void compact_method_tables(JVM_SINGLE_ARG_TRAPS);
  int  compact_method_table(InstanceClass *klass JVM_TRAPS);
  bool is_method_removable_from_table(const InstanceClass* klass,
                                      const Method* method);
  enum {
    MEMBERS_UNACCESSIBLE,
    PUBLIC_MEMBERS_ACCESSIBLE,
    PROTECTED_MEMBERS_ACCESSIBLE,
    PACKAGE_PRIVATE_MEMBERS_ACCESSIBLE
  };
  void create_extended_class_attributes(JVM_SINGLE_ARG_TRAPS);
  jbyte get_extended_class_attributes(const int i) const {
    GUARANTEE(extended_class_attributes()->not_null(), "Sanity");
    return extended_class_attributes()->byte_at(i);
  }
  void set_extended_class_attributes(const int i, const jbyte value) const {
    extended_class_attributes()->ubyte_at_put(i, value);
  }
  jbyte get_class_access_level(const InstanceClass* klass) const {
    return get_extended_class_attributes(klass->class_id());
  }
  void set_class_access_level(const InstanceClass* klass, const jbyte value) const {
    return set_extended_class_attributes(klass->class_id(), value);
  }
  bool is_member_reachable_by_apps(const jbyte class_access_level,
                                   const AccessFlags member_flags) const;
  bool is_member_reachable_by_apps(const InstanceClass* klass,
                                   const AccessFlags member_flags) const;
  static bool is_member_reachable_by_apps(const jint package_flags, 
                                          const AccessFlags class_flags,
                                          const AccessFlags member_flags);
  bool is_symbol_alive(ObjArray *live_symbols, Symbol* symbol);
  ReturnOop get_live_symbols(JVM_SINGLE_ARG_TRAPS);
  void record_live_symbol(ObjArray* live_symbols, OopDesc* symbol);
  void scan_live_symbols_in_class(ObjArray *live_symbols, JavaClass *klass);
#if ENABLE_ISOLATES
  void scan_all_symbols_in_class(ObjArray *live_symbols, JavaClass *klass);
#endif
  void scan_live_symbols_in_fields(ObjArray *live_symbols, 
                                   InstanceClass *klass);
  void scan_live_symbols_in_methods(ObjArray *live_symbols, 
                                    InstanceClass *klass);

  void inline_exception_constructors();
  void inline_short_methods(JVM_SINGLE_ARG_TRAPS);
  bool is_inlineable_exception_constructor(Method *method);
  static bool is_special_method(const Method* method);
  void clean_vtables(InstanceClass* klass, Method* method, int vindex);
  void clean_itables(InstanceClass* klass, int iindex);

#if USE_REFLECTION
  void resolve_constant_pool(JVM_SINGLE_ARG_TRAPS);
#else
  void resolve_constant_pool(JVM_SINGLE_ARG_TRAPS) {JVM_IGNORE_TRAPS;}
#endif

#if USE_AOT_COMPILATION
  void precompile_methods(JVM_SINGLE_ARG_TRAPS);
#else
  void precompile_methods(JVM_SINGLE_ARG_TRAPS) {JVM_IGNORE_TRAPS;}
#endif

#if USE_SOURCE_IMAGE_GENERATOR
  bool is_in_public_itable        (const InstanceClass* ic,
                                   const Method* method) const;
  bool is_in_public_vtable        (const InstanceClass* ic,
                                   const Method* method) const;
  bool is_method_reachable_by_apps(const InstanceClass* ic,
                                   const Method* method) const;
  bool is_invocation_closure_root (const InstanceClass* ic,
                                   const Method* method) const;
  void remove_dead_methods(JVM_SINGLE_ARG_TRAPS);
  bool is_field_removable(const InstanceClass* ic,
                          const int field_index, const bool from_table) const;
  void fix_one_field_table(const InstanceClass* klass, TypeArray* fields, 
                           const TypeArray* reloc_info) const;
  void compact_field_tables(JVM_SINGLE_ARG_TRAPS);
  void remove_unused_symbols(JVM_SINGLE_ARG_TRAPS);
  void remove_duplicated_short_arrays(JVM_SINGLE_ARG_TRAPS);
  void remove_duplicated_stackmaps(JVM_SINGLE_ARG_TRAPS);
  void remove_duplicated_objects(JVM_SINGLE_ARG_TRAPS);
  void remove_duplicated_short_arrays(Method *method, void *param JVM_TRAPS);
  void remove_duplicated_stackmaps(Method *method, void *param JVM_TRAPS);
  void remove_redundant_stackmaps(JVM_SINGLE_ARG_TRAPS);
  void remove_redundant_stackmaps(Method *method, void *param JVM_TRAPS);
  void use_unique_object_at(Oop *owner, int offset, ROMLookupTable *table
                            JVM_TRAPS);
  void record_original_class_info(const InstanceClass* klass,
                                  Symbol* name) const;
  void record_original_method_info(const Method* method JVM_TRAPS) const;

  void rename_non_public_symbols(JVM_SINGLE_ARG_TRAPS);
  void mark_hidden_classes(JVM_SINGLE_ARG_TRAPS);
#if ENABLE_MEMBER_HIDING
  static bool is_hidden_field (const InstanceClass* ic, const OopDesc* field);
  static bool is_hidden_method(const InstanceClass* ic, const Method* method);
#endif // ENABLE_MEMBER_HIDING


  class MethodIterator : public ObjectHeapVisitor {
    ROMOptimizer *_optimizer;
    int _mode;
    void *_param;
  public:
    MethodIterator(ROMOptimizer *optimizer) {
      _optimizer = optimizer;
    }
    void iterate(int mode, void *param JVM_TRAPS) {
      _mode = mode;
      _param = param;

      // Do this for two reasons:
      // - collect all removed methods that have become garbage
      // - ObjectHeap::iterate() doens't work too well if GC happens
      //   in the middle.
#if ENABLE_MONET //application image
      ObjectHeap::safe_collect(0 JVM_CHECK);
#else //system image
      ObjectHeap::full_collect(JVM_SINGLE_ARG_CHECK);
#endif

      GCDisabler disable_gc_for_the_rest_of_this_method;
      ObjectHeap::expand_young_generation();

      ObjectHeap::iterate(this);
    }

    virtual void do_obj(Oop* obj) {
      SETUP_ERROR_CHECKER_ARG;

      if (obj->klass() == NULL) {
        // Object hasn't been initialized yet
        return;
      }
      if (obj->blueprint() == NULL) {
        // Object hasn't been initialized yet
        return;
      }
      if (obj->is_method()) {
        UsingFastOops fast_oops;
        Method::Fast m = obj->obj();
        InstanceClass::Fast ic = m().holder();
        // Do not touch stackmaps if a class failed verification or was
        // marked non-optimizable because of some other error
        if (!ic().is_verified() || !ic().is_optimizable()) {
          return;
        }
        switch (_mode) {
        case REMOVE_DUPLICATED_SHORT_ARRAYS:
          _optimizer->remove_duplicated_short_arrays(&m, _param JVM_CHECK);
          break;
        case REMOVE_DUPLICATED_STACKMAPS:
          _optimizer->remove_duplicated_stackmaps(&m, _param JVM_CHECK);
          break;
        case REMOVE_REDUNDANT_STACKMAPS:
          _optimizer->remove_redundant_stackmaps(&m, _param JVM_CHECK);
          break;
        default:
          SHOULD_NOT_REACH_HERE();
        }
        GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION, "sanity");
      }
    }
    enum {
      REMOVE_DUPLICATED_SHORT_ARRAYS = 1,
      REMOVE_DUPLICATED_STACKMAPS    = 2,
      REMOVE_REDUNDANT_STACKMAPS     = 3
    };
  };
#endif // USE_SOURCE_IMAGE_GENERATOR

  //SUBCLASS CACHE ZONE
  enum {
    NEXT = 0,
    CLASS, 
    SIZE
  };
  void initialize_subclasses_cache(JVM_SINGLE_ARG_TRAPS);
  static ReturnOop get_subclass_list(jushort klass_id);
  //ENDOF SUBCLASS CACHE ZONE

  // used by ROMOptimizer::remove_unused_static_fields
  enum {
    DEAD_FIELD = 0x10000
  };

  friend class MethodIterator;
  friend class ROMClassPatternMatcher;
  friend class SourceROMWriter;
};

#endif // ENABLE_ROM_GENERATOR
