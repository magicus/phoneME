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

#if USE_BINARY_IMAGE_LOADER
class ROMBundle {
public:
#if USE_AOT_COMPILATION
#define ROMBUNDLE_COMPILED_METHOD_TABLE_ADDR_DO(template) \
    template(COMPILED_METHOD_TABLE_ADDR)
#define ROMBUNDLE_COMPILED_METHOD_TABLE_SIZE_DO(template) \
    template(COMPILED_METHOD_TABLE_SIZE)
#else
#define ROMBUNDLE_COMPILED_METHOD_TABLE_ADDR_DO(template)
#define ROMBUNDLE_COMPILED_METHOD_TABLE_SIZE_DO(template)
#endif

#define ROMBUNDLE_FIELDS_DO(template) \
    template(HEADER_TAG) \
    template(HEADER_WORD_COUNT) \
    template(MAGIC) \
    template(VERSION_ID) \
    template(BASE) \
    template(ROM_BUNDLE_ID) /*required only by shared_libs_monet support*/ \
    template(IS_SHARABLE) /*required only by shared_libs_monet support*/ \
    /* -------- here fields that need relocation starts -------- */ \
    template(ROM_LINKED_BUNDLES_OFFSET) /*required only by shared_libs_monet support*/\
    template(TEXT_BLOCK) \
    template(SYMBOL_TABLE) \
    template(STRING_TABLE) \
    ROMBUNDLE_COMPILED_METHOD_TABLE_ADDR_DO(template) \
    template(METHOD_VARIABLE_PARTS) \
    template(PERSISTENT_HANDLES) \
    template(HEAP_BLOCK) \
    /* -------- end of fields requiring relocation -------- */ \
    template(RELOCATION_BITMAP_OFFSET) \
    template(TEXT_BLOCK_SIZE) \
    template(SYMBOL_TABLE_NUM_BUCKETS) \
    template(STRING_TABLE_NUM_BUCKETS) \
    ROMBUNDLE_COMPILED_METHOD_TABLE_SIZE_DO(template) \
    template(METHOD_VARIABLE_PARTS_SIZE) \
    template(ROM_PERSISTENT_HANDLES_SIZE) \
    template(HEAP_BLOCK_SIZE) \
    template(GC_STACKMAP_SIZE) \
    template(NUMBER_OF_JAVA_CLASSES)

#define ROMBUNDLE_ENUM_DECLARE(x) x,

  enum {
    ROMBUNDLE_FIELDS_DO(ROMBUNDLE_ENUM_DECLARE)
    HEADER_SIZE
  };
private:
  juint array[HEADER_SIZE];

  bool heap_src_block_contains(const address target) const;
  static ROMBundle* _current;
  static int  heap_relocation_offset;
  static void relocate_pointer_to_heap(OopDesc** p);
  static void update_system_array_class(JVM_SINGLE_ARG_TRAPS);
#if ENABLE_COMPILER && ENABLE_INLINE
  void update_vtable_bitmaps(const int sys_class_count JVM_TRAPS) const;
#endif
public:
  int int_at(const int index) const {
    return int( array[index] );
  }
  void int_at_put(const int index, const int value) {
    array[index] = value;
  }
  juint uint_at(const int index) const {
    return (juint)(array[index]);
  }
  int* ptr_at( const int index ) const {
    return (int*)(array[index]);
  }
  const int* base( void ) const {
    return ptr_at( BASE );
  }
#if USE_AOT_COMPILATION
  int* compiled_method_table_addr( void ) const {
    return ptr_at( COMPILED_METHOD_TABLE_ADDR );
  }
  int compiled_method_table_size( void ) const {
    return int_at( COMPILED_METHOD_TABLE_SIZE );
  }
#endif
  int* method_variable_parts( void ) const {
    return ptr_at( METHOD_VARIABLE_PARTS );
  }
  int method_variable_parts_size( void ) const {
    return int_at( METHOD_VARIABLE_PARTS_SIZE );
  }
  int* heap_block( void ) const {
    return ptr_at( HEAP_BLOCK );
  }
  juint heap_block_size( void ) const {
    return uint_at( HEAP_BLOCK_SIZE );
  }
  int* text_block( void ) const {
    return ptr_at( TEXT_BLOCK );
  }
  juint text_block_size( void ) const {
    return uint_at( TEXT_BLOCK_SIZE );
  }
  int number_of_java_classes( void ) const {
    return uint_at( NUMBER_OF_JAVA_CLASSES );
  }
  juint symbol_table_num_buckets( void ) const {
    return uint_at(SYMBOL_TABLE_NUM_BUCKETS);
  }
  juint string_table_num_buckets( void ) const {
    return uint_at(STRING_TABLE_NUM_BUCKETS);
  }
  OopDesc** symbol_table( void ) const {
    return (OopDesc**)ptr_at( SYMBOL_TABLE );
  }
  OopDesc** string_table( void ) const {
    return (OopDesc**)ptr_at( STRING_TABLE );
  }
  int stackmap_size( void ) const {
    return uint_at( GC_STACKMAP_SIZE );
  }
  OopDesc** persistent_handles( void ) const {
    return (OopDesc**) ptr_at(PERSISTENT_HANDLES);
  }

  bool text_contains(const OopDesc* target) const {
    const juint offset = DISTANCE(text_block(), target);
    return offset < (juint)text_block_size();
  }

  unsigned relocation_bitmap_offset( void ) const {
    return int_at( RELOCATION_BITMAP_OFFSET );
  }
  int* relocation_bit_map( void ) const {
    return DERIVED(int*, &array, relocation_bitmap_offset() );
  }

  bool contains(const OopDesc* target) const {
    const unsigned offset = DISTANCE( array, target );
    return offset < relocation_bitmap_offset();
  }

#if USE_LARGE_OBJECT_AREA
  const unsigned heap_used( void ) const {
    return LargeObject::head( this )->size();  
  }
#endif

  void fixup( void );
  void setup_persistent_handles ( void ) const;
  ReturnOop copy_heap_block( JVM_SINGLE_ARG_TRAPS ) ;
  int bundle_id( void ) const {
    return int_at( ROM_BUNDLE_ID );
  }  

  bool is_sharable( void ) const {
    return (int_at( IS_SHARABLE ) != 0);
  }
#if ENABLE_LIB_IMAGES && ENABLE_ISOLATES
  static void restore_names_of_bad_classes(ObjArray* old_bad_classes_list JVM_TRAPS);
  void restore_vm_structures( int global_bundle_id, bool already_loaded, bool sharable JVM_TRAPS );
#else
  void restore_vm_structures( JVM_SINGLE_ARG_TRAPS );
#endif

  void free ( void ) {
#if USE_LARGE_OBJECT_AREA
    LargeObject::head( this )->free();
#endif
  }  
 
#if USE_AOT_COMPILATION
  static ReturnOop compiled_method_from_address(const address addr);
#endif
  
  static OsFile_Handle open(const JvmPathChar* file, int& length, int* bundle_id);
  static ROMBundle* load(const int task_id, const JvmPathChar path_name[],
                                            void** map_handle, bool* already_loaded);
  void method_variable_parts_oops_do(void do_oop(OopDesc**));
  void update_rom_default_entries( void )
#if ENABLE_COMPILER
   ;
#else
   {}
#endif

#if ENABLE_ISOLATES
#if ENABLE_LIB_IMAGES
 int add_to_global_binary_images(void* image_handle JVM_TRAPS);
 bool remove_if_not_currently_shared(); 
 bool is_shared(const Task* const task);
#ifdef AZZERT
 bool is_shared();
#endif
#else
  void add_to_global_binary_images();
#endif
  bool remove_from_global_binary_images();
#else
  void add_to_global_binary_images() {}
  void remove_from_global_binary_images() {}
#endif  
  static ROMBundle* current( void ) {
    return _current;
  }
  static void set_current( ROMBundle* bundle ) {
    _current = bundle;
  }
#ifndef PRODUCT
  void print_on(Stream *st);
  void p();
  static void print_all();
#endif

#if USE_IMAGE_PRELOADING
  static void preload( const JvmPathChar class_path[] );
protected:
#if ENABLE_LIB_IMAGES
#define MAX_LIB_IMAGES 8
  static OsFile_Handle _preloaded_handles[MAX_LIB_IMAGES];
  static int           _preloaded_lengths[MAX_LIB_IMAGES];
  static int           _loaded_bundle_count; 
  static int           _loaded_bundle_size; 
  static unsigned align_size  ( const unsigned size ) {
    return align_size_up( size, BitsPerWord * BytesPerWord );
  }
#else
  static OsFile_Handle _preloaded_handle;
  static int           _preloaded_length;
#endif
#endif
};
#endif /* USE_BINARY_IMAGE_LOADER */

#if ENABLE_PERFORMANCE_COUNTERS
struct ROM_PerformanceCounters {
  julong oops_do_hrticks;
#if ENABLE_DETAILED_PERFORMANCE_COUNTERS
  julong valid_method_hrticks;
  julong valid_field_hrticks;
  julong is_rom_symbol_hrticks;
  julong is_rom_method_hrticks;
  julong has_compact_method_hrticks;
  julong text_contains_hrticks;
  julong data_contains_hrticks;
  julong heap_contains_hrticks;
  julong get_max_offset_hrticks;
  julong string_from_table_hrticks;
  julong symbol_for_hrticks;
#endif
};

extern ROM_PerformanceCounters rom_perf_counts;
#endif


#if ENABLE_DETAILED_PERFORMANCE_COUNTERS
  #define ROM_DETAILED_PERFORMANCE_COUNTER_START() \
    jlong __start_time = Os::elapsed_counter() 
  #define ROM_DETAILED_PERFORMANCE_COUNTER_END(x)  \
    rom_perf_counts.x  += Os::elapsed_counter() - __start_time
#else
  #define ROM_DETAILED_PERFORMANCE_COUNTER_START()
  #define ROM_DETAILED_PERFORMANCE_COUNTER_END(x)
#endif


class ROM {
public:
  static ReturnOop decode_heap_reference(int value);
  static int encode_heap_reference(Oop* object);
  static void initialize(const JvmPathChar* class_path);
  static bool link_static(OopDesc** persistent_handles,
                          int number_of_persistent_handles);
#if ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES
  static ReturnOop link_static_mirror_list(JVM_SINGLE_ARG_TRAPS);
#endif
  static void init_debug_symbols(JVM_SINGLE_ARG_TRAPS);
  static void check_consistency() PRODUCT_RETURN;

  // Check if an object is in the system ROM
  static bool system_contains(const OopDesc* target)  {
    return system_text_contains(target) || system_data_contains(target);
  }
  static bool system_contains(OopDesc** target) {
    return system_contains((OopDesc*)target);
  }

  static bool in_any_loaded_bundle_of_current_task(const OopDesc* target);
  static bool in_any_loaded_readonly_bundle(const OopDesc* target);

  static bool in_any_loaded_bundle(const OopDesc* target)
#if !ENABLE_ISOLATES
    {return in_any_loaded_bundle_of_current_task(target);}
#else
    ;
#endif

#ifndef PRODUCT
  static void system_method_variable_parts_oops_do(void do_oop(OopDesc**));
#endif
  static void oops_do(void do_oop(OopDesc**), bool do_all_data_objects,
                      bool do_method_variable_parts);
  static size_t get_max_offset();

#if !defined(PRODUCT) || USE_PRODUCT_BINARY_IMAGE_GENERATOR
  static bool is_synchronized_method_allowed(Method *method);
#else
  static bool is_synchronized_method_allowed(Method* /*method*/)
               PRODUCT_RETURN0;
#endif

#if ENABLE_JVMPI_PROFILE || ENABLE_TTY_TRACE
  static ReturnOop get_original_class_name(ClassInfo* /*clsinfo*/);
  static ReturnOop get_original_method_name(const Method* /*method*/);
  static ReturnOop get_original_fields(InstanceClass* /*ic*/);
  static ReturnOop alternate_constant_pool(InstanceClass* /*ic*/);
#else
  static ReturnOop get_original_class_name(ClassInfo* /*clsinfo*/)
               PRODUCT_RETURN0;
  static ReturnOop get_original_method_name(const Method* /*method*/)
               PRODUCT_RETURN0;
  static ReturnOop get_original_fields(InstanceClass* /*ic*/)
               PRODUCT_RETURN0;
  static ReturnOop alternate_constant_pool(InstanceClass* /*ic*/)
               PRODUCT_RETURN0;
#endif

  static void dispose();

#if USE_IMAGE_MAPPING
  static void dispose_binary_images();
#else
  static void dispose_binary_images() {}
#endif
#if ENABLE_LIB_IMAGES
  static void accumulate_task_memory_usage();
#endif
  /**
   * The OopDesc::_klass field is required for debugging operations in
   * non-product modes (used by Oop::p(), Oop::type_check(), pp(), etc).
   * However, many objects in the TEXT_BLOCK have the _klass field skipped
   * to save footprint. The ROM::text_klass_of() reconstructs the
   * _klass field of such objects from a hash table, _rom_text_klass_table[].
   * This makes debugging operations possible for objects in the TEXT_BLOCK.
   */
  static OopDesc* text_klass_of(const OopDesc* /*obj*/) PRODUCT_RETURN0;

  static bool is_valid_text_object(const OopDesc* /*obj*/) PRODUCT_RETURN0;

  static bool is_restricted_package(const char* name, int len);
  static ReturnOop string_from_table(String *string, juint hash_value);
  static ReturnOop symbol_for(utf8 s, juint hash_value, int len);

  static int number_of_system_classes() { return _rom_number_of_java_classes; }

#if ENABLE_PERFORMANCE_COUNTERS
  static void ROM_print_hrticks(void print_hrticks(const char *, julong));
#endif

#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  static bool class_is_hidden_in_profile(const JavaClass* const jc);
  static bool is_restricted_package_in_profile(const char *name, 
                                               int name_len);
  static int profiles_count() { return _rom_profiles_count; }
  static const char **profiles_names() { return _rom_profiles_names; }
#endif

private:
  // Used in ROM initialization only -- is the address inside the
  // source data used to initialize the heap?
  inline static bool heap_src_block_contains(address target);
  static void relocate_heap_block();
  static void relocate_data_block();
public:
#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
  enum {
    MAIN_SEGMENT_INDEX         = -1, // Index of ROMImage.cpp stream;
    TEXT_BLOCK_SEGMENTS_COUNT  = 10, // Number of TEXT block parts;
    DATA_STREAM_INDEX = TEXT_BLOCK_SEGMENTS_COUNT, // DATA stream index
    HEAP_STREAM_INDEX,               // HEAP stream index;
    STUFF_STREAM_INDEX,              // Other ROM objects;
    SEGMENTS_STREAMS_COUNT           // Number of pieces, ROMImage.cpp 
                                     // is divided into. {13 files} =
                                     //   {10 files, TEXT block parts} +
                                     //   {DATA block file}  +
                                     //   {HEAP block file}  +
                                     //   {All other stuff};
  };
  enum {
    MAX_SECTIONS_GAP           = 512 // Maximum size of summary gap beetween
                                     // TEXT sections
  };
#endif

#if USE_BINARY_IMAGE_GENERATOR || USE_BINARY_IMAGE_LOADER || (ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES)
  // define bits in romized_reference

  // bits 31 - 30 block type
  // bits 29 - 0 offset in block
  //   00 = normal relocation
  //   01 = in ROM text section, offset is in next word
  //   10 = in ROM data section, offset is in next word
  //   11 = in ROM heap section, offset is in next word
  // for HEAP section:
  //    bits 29-26 instance_size (negative of the InstanceSize value)
  //    bits 25-0  type specific info (class id, etc)
  enum {
    offset_width        = 30,
    offset_start        = 0,
    block_type_start    = offset_start + offset_width,
    block_type_width    = 2,
    flag_start          = 0, //whether it is heap, library or system ref.
    flag_width          = 2,
    type_start          = flag_start + flag_width,
    type_width          = 24,
    type_mask           = (1 << type_width)-1,
    instance_size_start = type_start + type_width,
    instance_size_width = 4,
    instance_size_mask  = (1 << instance_size_width)-1
  };

  // copied from ROMWriter.hpp to avoid circular dependencies
  enum {
    TEXT_BLOCK = 1,
    DATA_BLOCK = 2,
    HEAP_BLOCK = 3,
    PERSISTENT_HANDLES_BLOCK,
    SYSTEM_SYMBOLS_BLOCK,
#if ENABLE_HEAP_NEARS_IN_HEAP && USE_SOURCE_IMAGE_GENERATOR  
    ROM_DUPLICATE_HANDLES_BLOCK, 
    TEXT_AND_HEAP_BLOCK, 
    DATA_AND_HEAP_BLOCK, 
#endif 
#if ENABLE_PREINITED_TASK_MIRRORS && USE_SOURCE_IMAGE_GENERATOR && ENABLE_ISOLATES  
    TASK_MIRRORS_BLOCK, 
#endif
    BLOCK_COUNT
  };

  enum {
    NAME_BUFFER_SIZE = 256
  };
#endif //USE_BINARY_IMAGE_GENERATOR || USE_BINARY_IMAGE_LOADER || (ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES)

  inline static bool system_text_contains(const OopDesc* target) {
#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
    GUARANTEE(_text_total_size > 0, "Sanity");
    const juint offset = ((juint)target) - _min_text_seg_addr;
    if (offset < _text_total_size) {
      return true;
    }
#else
    juint offset = ((juint)target) - ((juint)&_rom_text_block[0]);
    if (offset < (juint)_rom_text_block_size_fast) {
      return true;
    }
#endif  
    return false;
  }

  static bool system_data_contains(const OopDesc* target) {
    juint offset = ((juint)target) - ((juint)&_rom_data_block[0]);
    if (offset < (juint)_rom_data_block_size_fast) {
      return true;
    } else {
      return false;
    }
  }

#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
  static int text_segment_of(const OopDesc* obj);
#endif

#if ENABLE_COMPILER && ENABLE_APPENDED_CALLINFO
  static ReturnOop compiled_method_from_address(const address addr);
#endif

#if USE_BINARY_IMAGE_LOADER
  // SVM mode: is the VM currently using a binary image?
  // MVM mode: is the current isolate using a binary image?
#if !ENABLE_LIB_IMAGES
  static bool binary_image_currently_enabled() {
    return ROMBundle::current() != NULL;
  }
#endif //!ENABLE_LIB_IMAGES
#else //USE_BINARY_IMAGE_LOADER
  static bool binary_image_currently_enabled( void ) { return false; }
#endif //USE_BINARY_IMAGE_LOADER

  static bool is_rom_method(const OopDesc *target) {
    if (((OopDesc*)_rom_methods_start) <= target &&
        ((OopDesc*)_rom_methods_end) >= target) {
      return true;
    }
    return false;
  }

  static bool is_rom_symbol(const OopDesc *symbol) {
    // Note: I am using <= and >= here, in the hope that the C++ compiler
    // can generate better code for ARM.
    bool in_symbols =         
      (((OopDesc*)_rom_symbols_start) <= symbol) &&
      (((OopDesc*)_rom_symbols_end)   >= symbol);

#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
    const bool in_fieldtype_symbols = 
      (((OopDesc*)_rom_fieldtype_symbols_start) <= symbol) &&
      (((OopDesc*)_rom_fieldtype_symbols_end)   >= symbol);
    const bool in_signature_symbols = 
      (((OopDesc*)_rom_signature_symbols_start) <= symbol) &&
      (((OopDesc*)_rom_signature_symbols_end)   >= symbol);
    in_symbols |= in_fieldtype_symbols || in_signature_symbols;
#endif

    if (in_symbols) {
      // IMPL_NOTE: <this> is in the range of possible ROM symbol
      // pointers, but it may point to the interior of a symbol. If
      // we're truely paranoid, we can use a GUARANTEE to check <this>
      // against a list of all possible true ROM symbol pointers. This
      // list can be written by the Romizer.
      return true;
    }
    return false;
  }

  static bool is_valid_field_type(const OopDesc *target) {
    if (is_rom_symbol(target)) {
      if ((((OopDesc*)_rom_fieldtype_symbols_start) <= target) &&
          (((OopDesc*)_rom_fieldtype_symbols_end)   >= target)) {
        return true;
      }
    }
    return false;
  }

  static bool is_valid_method_signature(const OopDesc *target) {
    if (is_rom_symbol(target)) {
      if ((((OopDesc*)_rom_signature_symbols_start) <= target) &&
          (((OopDesc*)_rom_signature_symbols_end)   >= target)) {
        return true;
      }
    }
    return false;
  }

  static int _heap_relocation_offset;
private:
#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
  static void sort_text_segments();
  static void arrange_text_block();

  static juint _rom_text_block_segment_sizes[];
  static juint _rom_text_block_segments[];
  static juint _min_text_seg_addr;
  static juint _text_total_size;

public:
  static juint min_text_seg_addr() {
    return _min_text_seg_addr;
  }
  static juint text_total_size() {
    return _text_total_size;
  }
private:
#endif

  static OopDesc* raw_text_klass_of(const OopDesc* /*obj*/) PRODUCT_RETURN0;
  static void relocate_pointer_to_heap(OopDesc** p);
  static void decompress_strings();

#if ENABLE_JVMPI_PROFILE || ENABLE_TTY_TRACE
  static void initialize_original_class_name_list(JVM_SINGLE_ARG_TRAPS);

  static void initialize_original_method_info_list(JVM_SINGLE_ARG_TRAPS);

  static void initialize_original_fields_list(JVM_SINGLE_ARG_TRAPS);

  static void initialize_alternate_constant_pool(JVM_SINGLE_ARG_TRAPS);
#else

  static void initialize_original_class_name_list(JVM_SINGLE_ARG_TRAPS)
    PRODUCT_RETURN;
  static void initialize_original_method_info_list(JVM_SINGLE_ARG_TRAPS)
    PRODUCT_RETURN;
  static void initialize_original_fields_list(JVM_SINGLE_ARG_TRAPS)
    PRODUCT_RETURN;
  static void initialize_alternate_constant_pool(JVM_SINGLE_ARG_TRAPS)
    PRODUCT_RETURN;
#endif

/*====================================================================
 * The following definitions are related to the dynamic ROM image
 * loader.  See ROM.cpp for comments.
 *===================================================================*/

#if USE_BINARY_IMAGE_LOADER
public:
  static bool link_dynamic(Task* task, FilePath* unicode_file JVM_TRAPS);

#if ENABLE_ISOLATES
  static void on_task_switch(int tid);
#endif

private:
#if ENABLE_LIB_IMAGES 
  static bool check_bundle_references(ROMBundle *bundle, bool already_loaded);
public:
#if ENABLE_ISOLATES
  static ROMBundle* already_loaded(const int bundle_id, void** map_handle);
#endif // ENABLE_ISOLATES
#endif // ENABLE_LIB_IMAGES
#endif /* USE_BINARY_IMAGE_LOADER */

#if USE_BINARY_IMAGE_GENERATOR || USE_BINARY_IMAGE_LOADER
private:
  static OopDesc* _romized_heap_marker;

public:
  static OopDesc* romized_heap_marker() {
    return _romized_heap_marker;
  }

  static void set_romized_heap_marker(JVM_SINGLE_ARG_TRAPS) {
    const int task = ObjectHeap::start_system_allocation();
    _romized_heap_marker =
      Universe::new_instance(Universe::object_class() JVM_NO_CHECK);
    ObjectHeap::finish_system_allocation(task);
  }

#if ENABLE_MONET_DEBUG_DUMP
  static char * getNameForAddress(address addr);
#endif
#endif // USE_BINARY_IMAGE_GENERATOR || USE_BINARY_IMAGE_LOADER

/*====================================================================
 * Debug (non-product) operations start here.
 *===================================================================*/

#if !defined(PRODUCT) || USE_PRODUCT_BINARY_IMAGE_GENERATOR \
       || ENABLE_JVMPI_PROFILE || ENABLE_TTY_TRACE
  /*
   * [Original Class Names]
   *
   * This is an ObjArray whose length equals the number of romized classes.
   * Element [i] is the Symbol of the original name for the class whose
   * class_id is i.
   *
   * In GenerateROMImage mode, this array is created by
   * ROMOptimizer. Otherwise, this array is created by
   * initialize_original_class_name_list.
   */
  static OopDesc* _original_class_name_list;

  static void set_original_class_name_list(ObjArray *list) {
    _original_class_name_list = list->obj();
  }
  static ReturnOop original_class_name_list() {
    return _original_class_name_list;
  }

  /*
   * [Original Method Names]
   *
   * This is an ObjArray whose length equals the number of romized classes.
   * Element [i] corresponds to the romized class whose klass_index is i.
   *
   * Each element [i] is in turn an ObjArray of length=3. Its format is
   *  [0] = Method
   *  [1] = Original name of the method.
   *  [2] = link to next ObjArray[3].
   *
   * In GenerateROMImage mode, this array is initialized by
   * ROMOptimizer. Otherwise, this array is created by
   * initialize_original_field_method_list.
   */
  static OopDesc* _original_method_info_list;

  // Returns an ObjArray[3] as described above for the given method.
  static ReturnOop get_original_method_info(const Method *method);

  enum {
    INFO_OFFSET_METHOD    = 0,
    INFO_OFFSET_NAME      = 1,
    INFO_OFFSET_NEXT      = 2
  };

  static void set_original_method_info_list(ObjArray *list) {
    _original_method_info_list = list->obj();
  }
  static ReturnOop original_method_info_list() {
    return _original_method_info_list;
  }

  /*
   * [Original Field Information]
   *
   * This is an ObjArray whose length equals the number of romized
   * classes.  Element [i] is the a TypeArray (in the same format as
   * InstanceClass::fields()) that contains information about the
   * original fields of the class whose class_id is i.
   *
   * In GenerateROMImage mode, this array is created by
   * ROMOptimizer. Otherwise, this array is created by
   * initialize_original_fields_list.
   */
  static OopDesc* _original_fields_list;

  static void set_original_fields_list(ObjArray *list) {
    _original_fields_list = list->obj();
  }
  static ReturnOop original_fields_list() {
    return _original_fields_list;
  }

  static OopDesc* _alternate_constant_pool;
  static void set_alternate_constant_pool(Oop *cp) {
    _alternate_constant_pool = cp->obj();
  }


public:
  static bool create_rom_image(const JvmPathChar *class_path);

#if ENABLE_ISOLATES
  static int mvm_create_rom_image(String* jar, String* bin, int flags);
#endif

  friend class ROMOptimizer;
  friend class SourceROMWriter;
  friend class ROMWriter;
  friend class ConstantPoolRewriter;

#endif // PRODUCT || USE_PRODUCT_BINARY_IMAGE_GENERATOR || ENABLE_JVMPI_PROFILE

private:
#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
  static void init_rom_text_constants();
#endif
};

#if ENABLE_LIB_IMAGES 

#define  FOREACH_BINARY_IMAGE_IN_CURRENT_TASK(bundle)            \
  do {                                                           \
    ObjArray images = Task::current()->binary_images();          \
    if (images.not_null()) {                                     \
      const int len = images.length();                           \
      for (int i = 0; i < len; i++) {                            \
        const ROMBundle * bundle = (ROMBundle*)images.obj_at(i); \
        if (bundle != NULL) {

#define  ENDEACH_BINARY_IMAGE_IN_CURRENT_TASK \
        }                                     \
      }                                       \
    }                                         \
  } while (0)

#else

#define  FOREACH_BINARY_IMAGE_IN_CURRENT_TASK(bundle) \
  do {                                                \
    const ROMBundle * bundle = ROMBundle::current();  \
    if (bundle != NULL) {

#define  ENDEACH_BINARY_IMAGE_IN_CURRENT_TASK \
    }                                         \
  } while (0) 

#endif
