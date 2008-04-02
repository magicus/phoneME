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

class Universe: public AllStatic {
 public:
  // Create the initial universe.
  static bool bootstrap(const JvmPathChar* classpath);
  static void create_main_thread_mirror(JVM_SINGLE_ARG_TRAPS);

#if USE_JAR_ENTRY_ENUMERATOR
  static void load_all_in_classpath(JVM_SINGLE_ARG_TRAPS);
  static void load_all_in_classpath_segment(FilePath* path JVM_TRAPS) {
    load_next_in_classpath_segment(path, 0, max_jint JVM_NO_CHECK_AT_BOTTOM);
  }
  /*
   * Loads a chunk of entries from the specified path.
   * entry_id specifies the next entry to load.
   * entry_id of the first entry in the file is 0.
   * chunk_size defines the total compressed size of entries
   * to load from the path within this chunk.
   * Entries are loaded one-by-one until either all the entries are loaded in
   * this path or the total compressed size of loaded entries is greater than
   * chunk_size.
   *
   * Returns 0 if all entries in this path have been successfully loaded. 
   * Returns -1 if failed to load some entry in this path.
   * Otherwise, returns entry_id of the next entry.
   */
  static int load_next_in_classpath_segment(FilePath* path, 
    int entry_id, int chunk_size JVM_TRAPS);
  static void invoke_pending_entries(JVM_SINGLE_ARG_TRAPS);
#endif

#if !defined(PRODUCT) || ENABLE_ROM_GENERATOR
  static void check_rom_layout();
  static void record_inited_at_build(InstanceClass* klass JVM_TRAPS);
#endif

  // Destroy the universe
  static void apocalypse();

#if ENABLE_COMPILER
  // Allocates a new compiled method. Please note the code_size will
  // be word aligned in the process
  // ^CompiledMethod
  static ReturnOop new_compiled_method(int code_size JVM_TRAPS);
#endif
  static ReturnOop new_method(const int code_length,
                              const AccessFlags access_flags JVM_TRAPS);
  static ReturnOop new_constant_pool(int length JVM_TRAPS);
  static ReturnOop new_instance(InstanceClass* klass JVM_TRAPS);
  static ReturnOop new_instance_class(int vtable_size,
                                      size_t itable_size, int itable_length,
                                      size_t static_field_size,
                                      size_t oop_map_size,
                                      size_t instance_size JVM_TRAPS);
  static ReturnOop new_obj_array(int length JVM_TRAPS);
  static ReturnOop new_obj_array(JavaClass* klass, int length JVM_TRAPS);

  // Allocate a new java.lang.String
  static ReturnOop new_string(CharacterStream* stream JVM_TRAPS) {
    return new_string(stream, 0, 0 JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop new_string(const char* name, int length JVM_TRAPS);
  static ReturnOop new_string(Symbol* symbol JVM_TRAPS);
  static ReturnOop new_string(CharacterStream* stream, int lead_spaces,
                              int trail_spaces JVM_TRAPS);
  static ReturnOop new_string(TypeArray *char_array, int offset, int length
                              JVM_TRAPS);
  static ReturnOop interned_string_for(CharacterStream *stream JVM_TRAPS);
  static ReturnOop interned_string_for(String *string JVM_TRAPS);
  static ReturnOop interned_string_from_utf8(Oop *oop JVM_TRAPS);

  // Allocate a new java near
  static ReturnOop new_java_near(JavaClass* java_class JVM_TRAPS) {
    return allocate_java_near(java_class JVM_NO_CHECK_AT_BOTTOM);
  }

  // ^TypeArray, Support for allocation of multi arrays
  static ReturnOop new_type_array(TypeArrayClass* klass, jint length JVM_TRAPS);

  // Support for shrinking objects
  static void fill_heap_gap(address ptr, size_t size_to_fill);
  static ReturnOop shrink_object(Oop* object, size_t new_size,
                                 bool down = true);

  static ReturnOop new_entry_activation(Method* method, jint length JVM_TRAPS);
  static bool flush_caches();
  static ReturnOop new_thread(JVM_SINGLE_ARG_TRAPS) {
    return new_mixed_oop(MixedOopDesc::Type_Thread,
                         ThreadDesc::allocation_size(),
                         ThreadDesc::pointer_count()
                         JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop new_mixed_oop(int type, size_t size, int pointer_count
                                 JVM_TRAPS);
  static ReturnOop new_execution_stack(jint length JVM_TRAPS);

  // Support for stackmaps
  static ReturnOop new_stackmap_list(jint length JVM_TRAPS);

  // Java level debugger support
#if ENABLE_JAVA_DEBUGGER
  static ReturnOop new_refnode(JVM_SINGLE_ARG_TRAPS);
#endif

#if ENABLE_ISOLATES
  // Points to Universe::task_list(), but is not updated directly
  // by the GC. This variable is used in crucial sections during the GC
  // where Universe::task_list() is temporarily clobbered.
  static OopDesc* _raw_task_list;

  static void setup_isolate_list(JVM_SINGLE_ARG_TRAPS);
  static ReturnOop setup_mirror_list(int i JVM_TRAPS);
  static ReturnOop new_task(int id JVM_TRAPS);
  static ReturnOop new_task_mirror( const jint statics_size, 
                                    const jint vtable_length JVM_TRAPS) {
    return allocate_task_mirror(statics_size, vtable_length JVM_NO_CHECK);
  }
  static ReturnOop task_from_id(int task_id);

  static inline void set_task_list(ObjArray *tl);
  static void allocate_boundary_near_list(JVM_SINGLE_ARG_TRAPS);
  static void unload_unused_classes();
private:
  static bool class_is_in_dictionary(InstanceClass *cl, juint hash_value, 
                                     ObjArray *d);
  static inline ReturnOop new_task_list(JVM_SINGLE_ARG_TRAPS);
public:
#endif //ENABLE_ISOLATES

  static void set_current_task(const int task_id) {
#if !ENABLE_ISOLATES
    GUARANTEE( task_id == 1, "Unexpected task_id" );
#endif
    TaskContext::set_current_task(task_id);
  }

#if ENABLE_DYNAMIC_NATIVE_METHODS
  static int dynamic_lib_count;
#endif

  // Unconditionally allocates a new symbol.
  static ReturnOop new_symbol(TypeArray *byte_array, utf8 s, int len JVM_TRAPS);

  static void add_new_dictionary(ObjArray *);

private:
  static void init_task_list(JVM_SINGLE_ARG_TRAPS);
  static void create_first_task(const JvmPathChar* classpath JVM_TRAPS);
  static void allocate_bootstrap_thread(void);
  static bool bootstrap_with_rom(const JvmPathChar* classpath);

#if ROMIZED_PRODUCT
  static bool bootstrap_without_rom(const JvmPathChar* /*classpath*/) { return false; }
#else
  static bool bootstrap_without_rom(const JvmPathChar* classpath);
#endif

#if USE_BINARY_IMAGE_LOADER
  static void bootstrap_dynamic_rom(JVM_SINGLE_ARG_TRAPS);
#endif
#if ENABLE_ISOLATES
#ifndef PRODUCT
  static void check_romized_obj_array_classes(void);
#endif
#endif

  // Allocates some object. This can in return force a garbage collection.
  static ReturnOop allocate_instance_class(int vtable_length,
                                           size_t itable_size,
                                           int itable_length,
                                           size_t static_field_size,
                                           size_t oop_map_size,
                                           size_t instance_size JVM_TRAPS);
  static ReturnOop allocate_method        (int length JVM_TRAPS);
  static ReturnOop allocate_constant_pool (int length JVM_TRAPS);

  // Allocate near classes.  This can in return force a garbage collection.
  static ReturnOop allocate_java_near(JavaClass* klass JVM_TRAPS);

  static ReturnOop allocate_near(FarClass* klass JVM_TRAPS);
  static ReturnOop allocate_near_class(FarClass* klass JVM_TRAPS);

  static ReturnOop allocate_entry_activation(jint length JVM_TRAPS);
  static ReturnOop allocate_class_info(int vtable_length, int itable_length,
                                       int itable_size,
                                       ISOLATES_PARAM(int static_field_size)
                                       bool is_array JVM_TRAPS);
  static ReturnOop allocate_task(JVM_SINGLE_ARG_TRAPS);

#if ENABLE_ISOLATES
  static ReturnOop allocate_task_mirror(jint statics_size,
                                        int vtable_length JVM_TRAPS);
#endif

 public:
  static ReturnOop allocate_array_raw(FarClass* klass, int length, int scale
                                  JVM_TRAPS);
  static ReturnOop allocate_array(FarClass* klass, int length, int scale
                                  JVM_TRAPS);
  // These are temporarily made public. Used by Method.cpp.
  static ReturnOop allocate_obj_near(FarClass* klass JVM_TRAPS);

  // This is public. Used by FarClass::array_class().
  static ReturnOop new_obj_array_class(JavaClass* element_class JVM_TRAPS);

  static bool is_bootstrapping()       { return _is_bootstrapping; }
  static bool before_main()            { return _before_main; }
  static void set_stopping()           { _is_stopping = true; }
  static bool is_stopping()            { return _is_stopping; }

  // Allocation of persistent handle (not thread-local)
  static bool is_persistent_handle(Oop* /*obj*/) PRODUCT_RETURN0;

  // Support for GC of persistent handles
  static void oops_do(void do_oop(OopDesc**), const bool young_only = false);
  static void update_relative_pointers();

  // GC testing support
  static void allocate_gc_dummies(JVM_SINGLE_ARG_TRAPS) PRODUCT_RETURN;
  static void release_gc_dummy() PRODUCT_RETURN;

#if USE_DEBUG_PRINTING 
  // Dumping of the GC roots
  static void print_values_on(Stream*);
  static void print_persistent_handle_definitions();
#endif

  static int number_of_java_classes() {
    return TaskContext::number_of_java_classes();
  }

  static void set_number_of_java_classes(int number) {
    TaskContext::set_number_of_java_classes(number);
  }

#if  ENABLE_JVMPI_PROFILE 
   // Used to set the method ID
   static int number_of_java_methods(){
   return _number_of_java_methods;
   }
   
   // Increase the method ID
   static void inc_number_of_java_methods() {
     _number_of_java_methods++;
   }

   static void set_number_of_java_methods(int num_methods) {
     _number_of_java_methods = num_methods;
   }
#endif  

  static void pop_class_id(InstanceClass *new_cls, InstanceClass *old_cls);

  static int   _compilation_abstinence_ticks;
  static bool is_compilation_allowed ( void ) { return _is_compilation_allowed; }

#if ENABLE_INTERPRETATION_LOG
  static void reset_interpretation_log ( void ) {
  GUARANTEE_R(is_power_of_2(INTERP_LOG_SIZE-1), "sanity");
  _interpretation_log_idx  = 0;
  jvm_memset(_interpretation_log, 0, 
             INTERP_LOG_SIZE * sizeof(_interpretation_log[0]));
}

  static void set_compilation_allowed( const bool enable );
#else
  static void set_compilation_allowed( const bool enable ) {
    _is_compilation_allowed = enable;
  }
#endif

  static ReturnOop setup_classpath(TypeArray* classpath JVM_TRAPS);

#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  static void set_profile_id(const int id); 
  static int profile_id_by_name(const char * profile);

  static int current_profile_id();

  static int profile_id() { 
    return _profile_id;   
  }
#if USE_SOURCE_IMAGE_GENERATOR
  static ReturnOop new_profile(JVM_SINGLE_ARG_TRAPS);
  static ReturnOop new_vector(JVM_SINGLE_ARG_TRAPS);  
#endif // USE_SOURCE_IMAGE_GENERATOR
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT
  static bool name_matches_pattern(const char* name, int name_len, 
                                   const char* pattern, int pattern_len);  
 
 private:
  typedef OopDesc* Allocator(size_t size JVM_TRAPS);

  static void report_bootstrap_failure();
  static void grow_symbol_table(JVM_SINGLE_ARG_TRAPS);

  // Generic allocation routines
  static ReturnOop generic_allocate_near(FarClass* klass, size_t size
                                         JVM_TRAPS);
  static ReturnOop generic_allocate_near_class(FarClass* klass,
                                               size_t class_size,
                                               jint instance_size,
                                               jubyte* extern_oop_map
                                               JVM_TRAPS);
  static ReturnOop generic_allocate_oop(FarClass *klass,
                                        size_t allocation_size
                                        JVM_TRAPS);

  static ReturnOop generic_allocate_array(Allocator* allocate, 
                                          FarClass* klass, 
                                          int length,
                                          int scale JVM_TRAPS);

  static void check_class_list_size(JVM_SINGLE_ARG_TRAPS);
  static void resize_class_list(int delta JVM_TRAPS);
  static void register_java_class(JavaClass *klass);
  static void unregister_last_java_class();

  static bool  _is_bootstrapping;
  static bool  _before_main;
  static bool  _is_stopping;
  static bool  _is_compilation_allowed;

#if ENABLE_JVMPI_PROFILE 
  // for set method id
  static jint  _number_of_java_methods;   
#endif  


#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  static int   _profile_id;
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT

#define UNIVERSE_GENERIC_HANDLES_DO(template)                         \
  template(meta_class,                           FarClass)            \
  template(type_array_class_class,               FarClass)            \
  template(obj_array_class_class,                FarClass)            \
  template(instance_class_class,                 FarClass)            \
   /* see as_TypeArrayClass().  Order of array classes is important */ \
  template(bool_array_class,                     TypeArrayClass)      \
  template(char_array_class,                     TypeArrayClass)      \
  template(float_array_class,                    TypeArrayClass)      \
  template(double_array_class,                   TypeArrayClass)      \
  template(byte_array_class,                     TypeArrayClass)      \
  template(short_array_class,                    TypeArrayClass)      \
  template(int_array_class,                      TypeArrayClass)      \
  template(long_array_class,                     TypeArrayClass)      \
   /* end of type array.  Order no longer important */                \
  template(method_class,                         FarClass)            \
  template(constant_pool_class,                  FarClass)            \
  template(symbol_class,                         FarClass)            \
  template(compiled_method_class,                FarClass)            \
  template(entry_activation_class,               FarClass)            \
  template(stackmap_list_class,                  FarClass)            \
  template(class_info_class,                     FarClass)            \
  template(execution_stack_class,                FarClass)            \
  template(system_dictionary,                    ObjArray)            \
  template(field_type_near,                      Near)                \
  template(method_signature_near,                Near)                \
  template(object_class,                         InstanceClass)       \
  template(system_class,                         InstanceClass)       \
  template(jvm_class,                            InstanceClass)       \
  template(file_descriptor_class,                InstanceClass)       \
  template(thread_class,                         InstanceClass)       \
  template(throwable_class,                      InstanceClass)       \
  template(error_class,                          InstanceClass)       \
  template(math_class,                           InstanceClass)       \
  template(object_array_class,                   ObjArrayClass)       \
  template(string_class,                         InstanceClass)       \
  template(java_lang_Class_class,                InstanceClass)       \
  /* The following 5 classes must be in this order. See */            \
  /* ThrowExceptionStub::exception_class() */                         \
  template(null_pointer_exception_class,         InstanceClass)       \
  template(array_index_out_of_bounds_exception_class, InstanceClass)  \
  template(illegal_monitor_state_exception_class, InstanceClass)      \
  template(arithmetic_exception_class,            InstanceClass)      \
  template(incompatible_class_change_error_class, InstanceClass)      \
  /* End of fixed ordering */                                         \
  template(dynamic_lib_handles,                  TypeArray)           \
  template(mixed_oop_class,                      FarClass)            \
  template(rom_text_empty_obj_array,             ObjArray)            \
  template(empty_obj_array,                      ObjArray)            \
  template(empty_short_array,                    TypeArray)           \
  template(out_of_memory_error_instance,         Oop)                 \
  template(gc_dummies,                           ObjArray)            \
  template(global_refs_array,                    RefArray)            \
  template(throw_null_pointer_exception_method,  Method)              \
  template(throw_array_index_exception_method,   Method)              \
  template(quick_native_throw_method,            Method)              \
  template(string_table,                         StringTable)         \
  template(symbol_table,                         SymbolTable)         \
  template(system_class_list,                    ObjArray)            \
  template(class_list,                           ObjArray)            \
  template(mirror_list,                          ObjArray)            \
  template(blocked_threads_buffer,               TypeArray)           \
  template(gc_block_stackmap,                    TypeArray)           \
  template(inlined_stackmaps,                    StackmapList)        \
  template(verifier_stackmap_cache,              TypeArray)           \
  template(verifier_instruction_starts_cache,    TypeArray)           \
  template(verifier_vstack_tags_cache,           TypeArray)           \
  template(verifier_vstack_classes_cache,        ObjArray)            \
  template(verifier_vlocals_tags_cache,          TypeArray)           \
  template(verifier_vlocals_classes_cache,       ObjArray)            \
  template(async_watcher,                        Thread)              \
  template(scheduler_async,                      Thread)              \
  template(special_thread,                       Thread)              \
  template(current_dictionary,                   ObjArray)            \
  template(lock_obj_table,                       ObjArray)            \
  template(interned_string_near,                 Near)                \
  template(resource_names,                       ObjArray)            \
  template(resource_data,                        ObjArray)            \
  template(resource_size,                        TypeArray)

// These handles are skipped during source romization
#define UNIVERSE_GENERIC_HANDLES_SKIP_DO(template)                    \
  template(rom_image_head,                       Oop)                 \
  template(scheduler_priority_queues,            ObjArray)            \
  template(system_mirror_list,                   ObjArray)            \
  template(current_task_obj,                     Task)                \
  template(scheduler_waiting,                    Thread)              \
  template(global_threadlist,                    Thread)

  // ^^^^^^^NOTE: last NUM_HANDLES_SKIP handles must be
  // global_threadlist, Universe knows this ugly secret

#if ENABLE_HEAP_NEARS_IN_HEAP
#define ROM_DUPLICATE_CLASS_HANDLES_DO(template)                          \
  template(rom_meta_class,                           FarClass)            \
  template(rom_type_array_class_class,               FarClass)            \
  template(rom_obj_array_class_class,                FarClass)            \
  template(rom_instance_class_class,                 FarClass)            \
   /* see as_TypeArrayClass().  Order of array classes is important */ \
  template(rom_bool_array_class,                     TypeArrayClass)      \
  template(rom_char_array_class,                     TypeArrayClass)      \
  template(rom_float_array_class,                    TypeArrayClass)      \
  template(rom_double_array_class,                   TypeArrayClass)      \
  template(rom_byte_array_class,                     TypeArrayClass)      \
  template(rom_short_array_class,                    TypeArrayClass)      \
  template(rom_int_array_class,                      TypeArrayClass)      \
  template(rom_long_array_class,                     TypeArrayClass)      
#else
#define ROM_DUPLICATE_CLASS_HANDLES_DO(template)                          
#endif

 public:

#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  enum {
    DEFAULT_PROFILE_ID = -1
  };
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT

#if ENABLE_JAVA_DEBUGGER
#define UNIVERSE_DEBUGGER_HANDLES_DO(template)                        \
  template(vmevent_request_head,                 Oop)                 \
  template(packet_buffer_list,                   ObjArray)            \
  template(transport_head,                       Oop)                 \
  template(refnode_class,                        FarClass)            \
  template(objects_by_id_map,                    ObjArray)            \
  template(dbg_class,                            InstanceClass)       \
  template(objects_by_ref_map,                   ObjArray)            \
  template(mp_stack_list,                        ObjArray)

#else
#define UNIVERSE_DEBUGGER_HANDLES_DO(template)
#endif

#if ENABLE_REFLECTION
#define UNIVERSE_REFLECTION_HANDLES_DO(template)                      \
  /* Order does matter. See get_primitive_type_class() */             \
  template(boolean_class,                        InstanceClass)       \
  template(character_class,                      InstanceClass)       \
  template(float_class,                          InstanceClass)       \
  template(double_class,                         InstanceClass)       \
  template(byte_class,                           InstanceClass)       \
  template(short_class,                          InstanceClass)       \
  template(integer_class,                        InstanceClass)       \
  template(long_class,                           InstanceClass)       \
  template(void_class,                           InstanceClass)       \
  /* End of type classes */

#else
#define UNIVERSE_REFLECTION_HANDLES_DO(template)
#endif

#if ENABLE_ISOLATES
#define UNIVERSE_ISOLATES_HANDLES_DO(template)                        \
  template(boundary_class,                       FarClass)            \
  template(boundary_near_list,                   ObjArray)            \
  template(task_class,                           FarClass)            \
  template(task_mirror_class,                    FarClass)            \
  template(isolate_class,                        InstanceClass)       \
  template(isolate_termination_signal,           Oop)                 \
  template(suspend_task_queue,                   Thread)              \
  template(inited_at_build,                      ObjArray)            \
  template(global_binary_images,                 ObjArray)            \
  template(global_binary_persistante_handles,    ObjArray)            \
  template(global_image_handles,                 TypeArray)           \
  template(task_class_init_marker,               Oop)

// These handles are skipped during source romization
#define UNIVERSE_ISOLATES_HANDLES_SKIP_DO(template)                   \
  template(boundary_list,                        Oop)                 \
  template(task_list,                            TaskList)

#else
#define UNIVERSE_ISOLATES_HANDLES_DO(template)
#define UNIVERSE_ISOLATES_HANDLES_SKIP_DO(template)
#endif

#define UNIVERSE_HANDLES_DO(template)          \
   UNIVERSE_DEBUGGER_HANDLES_DO(template)      \
   UNIVERSE_ISOLATES_HANDLES_DO(template)      \
   UNIVERSE_REFLECTION_HANDLES_DO(template)    \
   UNIVERSE_GENERIC_HANDLES_DO(template)       \
   ROM_DUPLICATE_CLASS_HANDLES_DO(template)    \
   UNIVERSE_ISOLATES_HANDLES_SKIP_DO(template) \
   UNIVERSE_GENERIC_HANDLES_SKIP_DO(template)

#define UNIVERSE_HANDLES_DECLARE(name, type) \
  name##_index,

#define UNIVERSE_HANDLES_COUNT_SKIP(name, type) \
  name##_skip_##suffix,

#define UNIVERSE_HANDLES_ACCESS(name, type) \
  static type* name() { return (type*) &persistent_handles[name##_index]; }

  // Count the number of all persistent handles
  enum {
   UNIVERSE_HANDLES_DO(UNIVERSE_HANDLES_DECLARE)
   __number_of_persistent_handles
  };

  // Count how many persistent handles should be skipped by source romizer
  enum {
   UNIVERSE_ISOLATES_HANDLES_SKIP_DO(UNIVERSE_HANDLES_COUNT_SKIP)
   UNIVERSE_GENERIC_HANDLES_SKIP_DO(UNIVERSE_HANDLES_COUNT_SKIP)
   NUM_HANDLES_SKIP
  };

  // Count how many duplicated ROM persistent handles we have
  enum {  
   ROM_DUPLICATE_CLASS_HANDLES_DO(UNIVERSE_HANDLES_COUNT_SKIP)
   NUM_DUPLICATE_ROM_HANDLES
  };

  static const jubyte* oopmaps[];

  UNIVERSE_HANDLES_DO(UNIVERSE_HANDLES_ACCESS)

#if ENABLE_VERIFY_ONLY
  /*
   * Loads and verifies a chunk of entries from the specified path.
   * entry_id specifies the next entry to load and verify.
   * entry_id of the first entry in the file is 0.
   * chunk_size defines the total compressed size of entries
   * to load from the path within this chunk.
   * Entries are loaded one-by-one until either all the entries are loaded in
   * this path or the total compressed size of loaded entries is greater than
   * chunk_size.
   *
   * Returns 0 if all entries in this path have been successfully loaded and
   * verified. 
   * Returns -1 if failed to load or verify some entry in this path.
   * Otherwise, returns entry_id of the next entry.
   */
  static int load_next_and_verify(FilePath* path, 
                                  int chunk_id, int chunk_size JVM_TRAPS);
#endif

private:
#if USE_JAR_ENTRY_ENUMERATOR
  static void load_jar_entry(char* name, int length, 
                             JarFileParser* jf_parser JVM_TRAPS);
#endif
  static void create_meta(JVM_SINGLE_ARG_TRAPS);
  static void create_type_array_classes(JVM_SINGLE_ARG_TRAPS);
  static void setup_super_and_vtables(JVM_SINGLE_ARG_TRAPS);
  static void setup_mirrors(JVM_SINGLE_ARG_TRAPS);
  static void setup_thread_priority_list(JVM_SINGLE_ARG_TRAPS);
  static void setup_thread(Thread *thread);

  friend class SymbolTable;
  friend class Symbols;
  friend class ClassFileParser;
  friend class ConstantPool;
  friend class ObjectHeap;
  friend class ROMWriter;
  friend class SourceROMWriter;
  friend class BinaryROMWriter;
  friend class ROMOptimizer;
  friend class ROM;

  static void load_root_class(InstanceClass* ic, Symbol* class_name);

 public:
  // These functions use persistent handles. Gcc can generate better
  // code if their inlineed definitions appear after the declaration
  // of the persistent_handle accessor functions.
  static TypeArrayClass* as_TypeArrayClass(BasicType index) {
    // This code makes use of the fact that the order of the array classes
    // in the persistent_handles array is precisely the same as their
    // numeric order.
    GUARANTEE(index >= T_BOOLEAN && index <= T_LONG, "bad BasicType");
    return (TypeArrayClass*)
      &persistent_handles[index + (bool_array_class_index - T_BOOLEAN)];
  }
  static ReturnOop class_from_id(jint class_id) {
    ReturnOop cls = ((ReturnOop*)_class_list_base)[class_id];
    GUARANTEE(cls != NULL, "sanity");
    GUARANTEE(TaskContext::number_of_java_classes() > class_id, "sanity");
    return cls;
  }
#if ENABLE_ISOLATES
  static ReturnOop class_from_id_or_null(jint class_id) {
    return ((ReturnOop*)_class_list_base)[class_id];
  }

  static ReturnOop task_mirror_from_id(jint class_id) {
    return ((ReturnOop*)_mirror_list_base)[class_id];
  }
#endif
  // Factory member functions.
  static ReturnOop new_bool_array(int length JVM_TRAPS) {
    return allocate_array((FarClass *)((void*)bool_array_class()),
                          length, sizeof(jboolean)
                          JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop new_bool_array_raw(int length JVM_TRAPS) {
    return allocate_array_raw((FarClass *)((void*)bool_array_class()),
                          length, sizeof(jboolean)
                          JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop new_byte_array(int length JVM_TRAPS) {
    return allocate_array((FarClass *)((void*)byte_array_class()),
                          length, sizeof(jbyte) JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop new_byte_array_raw(int length JVM_TRAPS) {
    return allocate_array_raw((FarClass *)((void*)byte_array_class()),
                          length, sizeof(jbyte) JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop new_char_array(int length JVM_TRAPS) {
    return allocate_array((FarClass *)((void*)char_array_class()),
                          length, sizeof(jchar) JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop new_char_array_raw(int length JVM_TRAPS) {
    return allocate_array_raw((FarClass *)((void*)char_array_class()),
                          length, sizeof(jchar) JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop new_int_array(int length JVM_TRAPS) {
    return allocate_array((FarClass *)((void*)int_array_class()),
                          length, sizeof(jint) JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop new_int_array_raw(int length JVM_TRAPS) {
    return allocate_array_raw((FarClass *)((void*)int_array_class()),
                          length, sizeof(jint) JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop new_short_array(int length JVM_TRAPS) {
    return allocate_array((FarClass *)((void*)short_array_class()),
                          length, sizeof(jshort) JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop new_short_array_raw(int length JVM_TRAPS) {
    return allocate_array_raw((FarClass *)((void*)short_array_class()),
                          length, sizeof(jshort) JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop new_long_array(int length JVM_TRAPS) {
    return allocate_array((FarClass *)((void*)long_array_class()),
                          length, sizeof(jlong) JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop new_long_array_raw(int length JVM_TRAPS) {
    return allocate_array_raw((FarClass *)((void*)long_array_class()),
                          length, sizeof(jlong) JVM_NO_CHECK_AT_BOTTOM);
  }
#if ENABLE_FLOAT
  static ReturnOop new_float_array(int length JVM_TRAPS) {
    return allocate_array((FarClass *)((void*)float_array_class()),
                          length, sizeof(jfloat) JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop new_float_array_raw(int length JVM_TRAPS) {
    return allocate_array_raw((FarClass *)((void*)float_array_class()),
                          length, sizeof(jfloat) JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop new_double_array(int length JVM_TRAPS) {
    return allocate_array((FarClass *)((void*)double_array_class()),
                          length, sizeof(jdouble) JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop new_double_array_raw(int length JVM_TRAPS) {
    return allocate_array_raw((FarClass *)((void*)double_array_class()),
                          length, sizeof(jdouble) JVM_NO_CHECK_AT_BOTTOM);
  }
#endif

#if ENABLE_ISOLATES
  // IMPL_NOTE: maybe create a special iterator for the functions below?
  static ReturnOop copy_strings_to_byte_arrays(OopDesc* string_array JVM_TRAPS);
  static ReturnOop copy_strings_to_char_arrays(OopDesc* string_array JVM_TRAPS);
  static ReturnOop make_strings_from_char_arrays(OopDesc* string_array JVM_TRAPS);
  static ReturnOop deep_copy(OopDesc* obj JVM_TRAPS);
#endif
};
