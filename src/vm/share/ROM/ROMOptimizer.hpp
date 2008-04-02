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

#if ENABLE_ROM_GENERATOR

class ROMTableInfo {
protected:
  ObjArray *_heap_table;
  int _count;
public:
  ROMTableInfo(ObjArray *array);
  ObjArray * heap_table() {
    return _heap_table;
  }
  int num_buckets();
  int count() {
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
  int string_count() {
    return _string_count;
  }
  // Total number of symbols in the symbol table
  int symbol_count() {
    return _symbol_count;
  }

  ReturnOop string_table() {
    return _string_table.obj();
  }
  ReturnOop symbol_table() {
    return _symbol_table.obj();
  }

  void set_embedded_hashtables(ConstantPool *holder, int strings_offset, 
                               int symbols_offset) {
    _embedded_table_holder = holder->obj();
    _embedded_strings_offset = strings_offset;
    _embedded_symbols_offset = symbols_offset;
  }

  ReturnOop embedded_table_holder() {
    return _embedded_table_holder.obj();
  }

  int embedded_strings_offset() {
    return _embedded_strings_offset;
  }
  int embedded_symbols_offset() {
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

#if ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR
#define MPS_ROMOPTIMIZER_OOP_FIELDS_DO(template) \
  template(ROMVector,profiles_vector, "") \
  template(ROMProfile,current_profile, "") \
  template(TypeArray,profile_hidden_bitmap, "")
#else 
#define MPS_ROMOPTIMIZER_OOP_FIELDS_DO(template)
#endif

#define ROMOPTIMIZER_OOP_FIELDS_DO(template) \
  MPS_ROMOPTIMIZER_OOP_FIELDS_DO(template) \
  template(TypeArray,empty_short_array, "") \
  template(ObjArray, empty_obj_array, "") \
  template(ObjArray, init_at_build_classes, "Classes that should be " \
                                            "initialized at build time") \
  template(ObjArray, init_at_load_classes,  "Classes that should be " \
                                            "initialized VM load time") \
  template(ObjArray, dont_rename_fields_classes, "Don't rename private" \
                                            "fields in these classes") \
  template(ObjArray, dont_rename_methods_classes,"Don't rename private" \
                                            "methods in these classes") \
  template(ObjArray, dont_rename_classes,   "Don't rename these classes," \
                                            "even if they belong to a hidden" \
                                            "package") \
  template(ROMVector, hidden_packages, "") \
  template(ROMVector, restricted_packages, "") \
  template(ObjArray ,romizer_original_class_name_list, "Original names of" \
                                            "classes we've renamed.") \
  template(ObjArray, romizer_original_method_info, "Original names/signatures"\
                                            "of methods that we've renamed") \
  template(ObjArray, romizer_original_fields_list, "Original names/signatures"\
                                            "of fields that we've renamed") \
  template(ConstantPool,romizer_alternate_constant_pool, "") \
  template(ObjArray, kvm_native_methods_table, "Methods that use KVM-style " \
                                            "native interface") \
  template(ROMVector,precompile_method_list, "") \
  template(ObjArray,     string_table, "") \
  template(ObjArray,     symbol_table, "") \
  template(ConstantPool, embedded_table_holder, "") \
  template(ObjArray,     subclasses_array, "")  \
  template(ROMVector,    reserved_words, "")    \
  template(TypeArray,    direct_interface_implementation_cache, \
                                     "for each interface contains class_id of \
                                      implementing class, or -1 in case of \
                                      multiple classes") \
  template(TypeArray,    interface_implementation_cache, \
                                     "for each interface contains class_id of \
                                      directly implementing class, or -1 in case of \
                                      multiple classes")

#if USE_SOURCE_IMAGE_GENERATOR

#if ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR
#define MPS_SOURCE_ROMOPTIMIZER_INT_FIELDS_DO(template) \
  template(bool,         config_parsing_in_profile, "")
#else
#define MPS_SOURCE_ROMOPTIMIZER_INT_FIELDS_DO(template)
#endif

#define SOURCE_ROMOPTIMIZER_INT_FIELDS_DO(template) \
  MPS_SOURCE_ROMOPTIMIZER_INT_FIELDS_DO(template) \
  template(bool,                config_parsing_active, "") \
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
  template(ROMVector*, kvm_natives_log, "") \
  template(ROMVector*, kvm_native_methods_vector, "")


#define ROMOPTIMIZER_DECLARE_OOP_GETTER(type, name, comment) \
  static type * name() { \
    return (type*)&_romoptimizer_oops[name ## _index]; \
  }

#define ROMOPTIMIZER_DECLARE_OOP_SETTER(type, name, comment) \
  static void set_ ## name(type* value) { \
    _romoptimizer_oops[name ## _index] = value->obj(); \
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

class ROMOptimizer {
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

  enum {
    // The order is important. Some optimizations depend on
    // the results of earlier optimizations. Do not change.
    STATE_MAKE_RESTRICTED_PACKAGES_FINAL =  1,
    STATE_INITIALIZE_CLASSES             =  2,
    STATE_QUICKEN_METHODS                =  3,
    STATE_RESOLVE_CONSTANT_POOL          =  4,
    STATE_REMOVE_REDUNDATE_STACKMAPS     =  5,
    STATE_MERGE_STRING_BODIES            =  6,
    STATE_RESIZE_CLASS_LIST              =  7,
    STATE_REPLACE_EMPTY_ARRAYS           =  8,
    STATE_INLINE_METHODS                 =  9,
    STATE_OPTIMIZE_FAST_ACCESSORS        = 10,
    STATE_REMOVE_DEAD_METHODS            = 11,
    STATE_RENAME_NON_PUBLIC_SYMBOLS      = 12,
    STATE_REMOVE_UNUSED_STATIC_FIELDS    = 13,
    STATE_COMPACT_FIELD_TABLES           = 14,
    STATE_REMOVE_UNUSED_SYMBOLS          = 15,
    STATE_REWRITE_CONSTANT_POOLS         = 16,
    STATE_COMPACT_TABLES                 = 17,
    STATE_PRECOMPILE_METHODS             = 18,
    STATE_REMOVE_DUPLICATED_OBJECTS      = 19,
    STATE_MARK_HIDDEN_CLASSES            = 20,
    STATE_DONE                           = 21,
    STATE_COUNT,
    STATE_FIRST_STATE                    = STATE_MAKE_RESTRICTED_PACKAGES_FINAL
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

  static void set_next_state() {
    set_state(state() + 1);
  }
  static bool is_active() {
    return (STATE_FIRST_STATE <= state() && state() < STATE_DONE);
  }
  static bool is_done() {
    return (state() >= STATE_DONE);
  }
  static int number_of_states() {
    return STATE_DONE;
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

  void initialize_hashtables(ObjArray* symbol_table, ObjArray* string_table 
                             JVM_TRAPS);
  static void trace_failed_quicken(Method *method, 
                                   JavaClass *dependency JVM_TRAPS);
  static void process_quickening_failure(Method *method);
  bool may_be_initialized(InstanceClass *klass);
  bool is_in_restricted_package(InstanceClass *klass);
  bool is_in_hidden_package(InstanceClass *klass  JVM_TRAPS);
  ReturnOop original_fields(InstanceClass *klass, bool &is_orig);
  void set_classes_as_romized();
  bool is_overridden(InstanceClass *ic, Method *method);
#if USE_SOURCE_IMAGE_GENERATOR
  bool class_matches_classes_list(InstanceClass *klass, ROMVector *patterns);
  bool class_matches_packages_list(InstanceClass *klass, ROMVector *patterns
                                   JVM_TRAPS);
#endif

#if USE_SOURCE_IMAGE_GENERATOR || (ENABLE_MONET && !ENABLE_LIB_IMAGES)
  void fill_interface_implementation_cache(JVM_SINGLE_ARG_TRAPS);
  void forbid_invoke_interface_optimization(InstanceClass* cls, bool indirect_only);
  void set_implementing_class(int interface_id, int class_id, bool only_childs, bool direct);  
  enum {
    NOT_IMPLEMENTED = -1, 
    FORBID_TO_IMPLEMENT = -2
  };
#endif

private:
  void log_time_counters();
  int  get_max_alternate_constant_pool_count();

#if ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR
  void create_profiles_hidden_bitmap(JVM_SINGLE_ARG_TRAPS);
  int find_profile(char * name);
#endif

#if USE_SOURCE_IMAGE_GENERATOR
  void read_config_file(JVM_SINGLE_ARG_TRAPS);
  void read_config_file(const JvmPathChar* config_file JVM_TRAPS);
  void read_hardcoded_config(JVM_SINGLE_ARG_TRAPS);
  void process_config_line(char * config_line JVM_TRAPS);
  void include_config_file(char* config_file JVM_TRAPS);
  bool parse_config(char *line, char**name, char **value);
  void add_class_to_list(ObjArray *list, char *flag, char *classname
                         JVM_TRAPS);
  void add_package_to_list(ROMVector *vector, char *pkgname JVM_TRAPS);
  bool class_list_contains(ObjArray *list, InstanceClass *klass);
  void enable_quick_natives(char* pattern JVM_TRAPS);
  void write_quick_natives_log();
  void enable_kvm_natives(char* pattern JVM_TRAPS);
  void update_kvm_natives_table(JVM_SINGLE_ARG_TRAPS);
  void write_kvm_natives_log();
#if USE_AOT_COMPILATION
  void enable_precompile(char* pattern JVM_TRAPS);
#endif

  bool dont_rename_class(InstanceClass *klass) {
    return class_list_contains(dont_rename_classes(), klass);
  }
  bool dont_rename_fields_in_class(InstanceClass *klass) {
    return class_list_contains(dont_rename_fields_classes(), klass);
  }
  bool dont_rename_methods_in_class(InstanceClass *klass) {
    return class_list_contains(dont_rename_methods_classes(), klass);
  }  

  bool is_init_at_build(InstanceClass *klass);

#else
  bool dont_rename_class(InstanceClass* /*klass*/)            {return false;}
  bool dont_rename_fields_in_class(InstanceClass* /*klass*/)  {return false;}
  bool dont_rename_methods_in_class(InstanceClass* /*klass*/) {return false;}
#endif

  void disable_compilation(char* pattern JVM_TRAPS);
  void write_disable_compilation_log();
  void allocate_empty_arrays(JVM_SINGLE_ARG_TRAPS);
  void make_restricted_packages_final(JVM_SINGLE_ARG_TRAPS);
  void make_restricted_methods_final(JVM_SINGLE_ARG_TRAPS);
  void make_virtual_methods_final(InstanceClass *ic, ROMVector *log_vector
                                  JVM_TRAPS);
#if USE_SOURCE_IMAGE_GENERATOR
  bool name_matches_patterns_list(Symbol* checking_name, 
                                  ROMVector *patterns_list);
#endif

  bool has_subclasses(InstanceClass *klass);
  void log_non_restricted_packages();
  void initialize_classes(JVM_SINGLE_ARG_TRAPS);
  void print_class_initialization_log(JVM_SINGLE_ARG_TRAPS);
  void quicken_methods(JVM_SINGLE_ARG_TRAPS);
  bool quicken_one_method(Method *method JVM_TRAPS);
  void optimize_fast_accessors(JVM_SINGLE_ARG_TRAPS);
  void merge_string_bodies(JVM_SINGLE_ARG_TRAPS);
  int  compress_and_merge_strings(ROMVector *all_strings, TypeArray* body);
  void replace_string_bodies(ROMVector *all_strings, TypeArray* body);
#if !USE_PRODUCT_BINARY_IMAGE_GENERATOR
  jint find_duplicate_chars(jchar *pool, jint pool_size,
                            jchar *match, jint num_chars);
#endif
  void resize_class_list(JVM_SINGLE_ARG_TRAPS);
  void rename_non_public_symbols(JVM_SINGLE_ARG_TRAPS);
  int  rename_non_public_class(InstanceClass *klass JVM_TRAPS);
  int  rename_non_public_fields(InstanceClass *klass JVM_TRAPS);
  int  rename_non_public_methods(InstanceClass *klass JVM_TRAPS);
#if USE_SOURCE_IMAGE_GENERATOR
  void record_original_class_info(InstanceClass *klass, Symbol *name);
  void record_original_method_info(Method *method JVM_TRAPS);
  void record_original_field_info(InstanceClass *klass, int name_index 
                                  JVM_TRAPS);
  void record_original_fields(InstanceClass *klass JVM_TRAPS);
  jushort get_index_from_alternate_constant_pool(InstanceClass *klass,
                                                 jushort symbol_index);
#else
  void record_original_class_info(InstanceClass* /*klass*/, Symbol* /*name*/)
       {}
  void record_original_method_info(Method* /*method*/ JVM_TRAPS)
       {JVM_IGNORE_TRAPS;}
  void record_original_field_info(InstanceClass* /*klass*/, int /*name_index*/
                                  JVM_TRAPS) {JVM_IGNORE_TRAPS;}
  void record_original_fields(InstanceClass* /*klass*/ JVM_TRAPS) 
       {JVM_IGNORE_TRAPS;}
#endif

  void replace_empty_arrays();
  void remove_unused_static_fields(JVM_SINGLE_ARG_TRAPS);
  void mark_static_fieldrefs(ObjArray *directory);
  void fix_static_fieldrefs(ObjArray *directory);
  void mark_unremoveable_static_fields(ObjArray *directory JVM_TRAPS);
  void compact_static_field_containers(ObjArray *directory);
  void fix_field_tables_after_static_field_removal(ObjArray *directory);
  void fix_one_field_table(InstanceClass *klass, TypeArray *fields, 
                           TypeArray *reloc_info);
  void compact_field_tables(JVM_SINGLE_ARG_TRAPS);
  void compact_method_tablses(JVM_SINGLE_ARG_TRAPS);
  int compact_one_interface(InstanceClass* ic);
  void compact_interface_classes(JVM_SINGLE_ARG_TRAPS);
  bool is_field_removable(InstanceClass *ic, int field_index, bool from_table JVM_TRAPS);
  void compact_method_tables(JVM_SINGLE_ARG_TRAPS);
  int  compact_method_table(InstanceClass *klass JVM_TRAPS);
  bool is_method_removable_from_table(Method *method);
  bool is_member_reachable_by_apps(jint package_flags, 
                                   AccessFlags class_flags,
                                   AccessFlags member_flags);
  bool field_may_be_renamed(jint package_flags, AccessFlags class_flags,
                            AccessFlags member_flags, Symbol *name);
  bool method_may_be_renamed(InstanceClass *ic, Method *method JVM_TRAPS);
  bool is_method_reachable_by_apps(InstanceClass *ic, Method *method JVM_TRAPS);
  bool is_invocation_closure_root(InstanceClass *ic, Method *method JVM_TRAPS);
  bool is_in_public_itable(InstanceClass *ic, Method *method JVM_TRAPS);
  bool is_in_public_vtable(InstanceClass *ic, Method *method JVM_TRAPS);
  void remove_unused_symbols(JVM_SINGLE_ARG_TRAPS);
  bool is_symbol_alive(ObjArray *live_symbols, Symbol* symbol);
  ReturnOop get_live_symbols(JVM_SINGLE_ARG_TRAPS);
  void record_live_symbol(ObjArray *live_symbols, Symbol* s);
  void scan_live_symbols_in_class(ObjArray *live_symbols, JavaClass *klass);
#if ENABLE_ISOLATES
  void scan_all_symbols_in_class(ObjArray *live_symbols, JavaClass *klass);
#endif
  void scan_live_symbols_in_fields(ObjArray *live_symbols, 
                                   InstanceClass *klass);
  void scan_live_symbols_in_methods(ObjArray *live_symbols, 
                                    InstanceClass *klass);

  void remove_dead_methods(JVM_SINGLE_ARG_TRAPS);
  void inline_exception_constructors();
  void inline_short_methods(JVM_SINGLE_ARG_TRAPS);
  bool is_inlineable_exception_constructor(Method *method);
  bool is_special_method(Method* method);
  void clean_vtables(InstanceClass* klass, Method* method, int vindex);
  void clean_itables(InstanceClass* klass, int iindex);
  void remove_duplicated_objects(JVM_SINGLE_ARG_TRAPS);
  void remove_duplicated_short_arrays(JVM_SINGLE_ARG_TRAPS);
  void remove_duplicated_short_arrays(Method *method, void *param JVM_TRAPS);
  void remove_duplicated_stackmaps(JVM_SINGLE_ARG_TRAPS);
  void remove_duplicated_stackmaps(Method *method, void *param JVM_TRAPS);
  void remove_redundant_stackmaps(JVM_SINGLE_ARG_TRAPS);
  void remove_redundant_stackmaps(Method *method, void *param JVM_TRAPS);
  void use_unique_object_at(Oop *owner, int offset, ROMLookupTable *table
                            JVM_TRAPS);

#if ENABLE_REFLECTION
  void resolve_constant_pool(JVM_SINGLE_ARG_TRAPS);
#else
  void resolve_constant_pool(JVM_SINGLE_ARG_TRAPS) {JVM_IGNORE_TRAPS;}
#endif

#if USE_AOT_COMPILATION
  void precompile_methods(JVM_SINGLE_ARG_TRAPS);
#else
  void precompile_methods(JVM_SINGLE_ARG_TRAPS) {JVM_IGNORE_TRAPS;}
#endif
  void mark_hidden_classes(JVM_SINGLE_ARG_TRAPS);

  //SUBCLASS CACHE ZONE
  enum {
    NEXT = 0,
    CLASS, 
    SIZE
  };
  void initialize_subclasses_cache(JVM_SINGLE_ARG_TRAPS);
  ReturnOop get_subclass_list(jushort klass_id);
  //ENDOF SUBCLASS CACHE ZONE

  jint get_package_flags(InstanceClass *ic JVM_TRAPS) {
    bool hidden = is_in_hidden_package(ic JVM_CHECK_0);
    if (hidden) {
      return HIDDEN_PACKAGE;
    } else if (is_in_restricted_package(ic)) {
      return RESTRICTED_PACKAGE;
    } else {
      return UNRESTRICTED_PACKAGE;
    }
  }

  enum {
    UNRESTRICTED_PACKAGE = 0,
    RESTRICTED_PACKAGE   = 1,
    HIDDEN_PACKAGE       = 2
  };
  // used by ROMOptimizer::remove_unused_static_fields
  enum {
    DEAD_FIELD = 0x10000
  };

  class MethodIterator : public ObjectHeapVisitor
  {
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

  friend class MethodIterator;
};

class JavaClassPatternMatcher {
  Symbol _class, _method, _signature;
  bool   _as_package;
  bool   _has_wildcards;
private:
  bool match(Symbol* pattern, Symbol* symbol);

  void initialize(char* pattern JVM_TRAPS);
  void initialize_as_package(Symbol* class_name JVM_TRAPS);

  bool match_class(Symbol* symbol);
  bool match_method(Symbol* name, Symbol* signature);

  void quick_match(JVM_SINGLE_ARG_TRAPS);
  void wildcard_match(JVM_SINGLE_ARG_TRAPS);
public:
  /**
   * Override this method to handle all matching methods.
   */
  virtual void handle_matching_method(Method* /*method*/ JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    SHOULD_NOT_REACH_HERE();
  }

  void run(char *pattern JVM_TRAPS);
};

#endif // ENABLE_ROM_GENERATOR
