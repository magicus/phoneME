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

/*
 * The following macros declares the states stored in the ROMWriter
 * object, so that it can be saved/restored across invocations of
 * {Source,Binary}ROMWriter::execute().
 *
 * This isn't too pretty. If I were to rewrite everything from scratch
 * I would implement ROMWriter as a MixedOop rather than a
 * StackObj. However, doing so would mean a lot of changes from the
 * old ROMWriter code.
 *
 * One new feature  is ROMWriter can be
 * suspended and resumed (so that other activities in the VM can carry on
 * concurrently -- this is similar to the suspend/resume feature in the
 * Compiler). During suspension, we save all ROMWriter states into
 * a side buffer, and restore it when the ROMWriter is resumed later.
 */
#define ROMWRITER_OOP_FIELDS_DO_COMMON(template) \
 template(ObjArray,     names_of_bad_classes_array, "") \
 template(ObjArray,     info_table, "") \
 template(PendingLink,  pending_links_head, "") \
 template(PendingLink,  pending_links_tail, "") \
 template(PendingLink,  pending_links_cache, "") \
 template(Oop,          info_table_last_obj, "") \
 template(ROMizerHashEntry, info_table_last_entry, "") \
 template(TypeArray,    string_chars, "") \
 template(TypeArray,    current_fieldmap, "") \
 template(Oop,          visiting_object, "The object that's currently being" \
                                         "visited by visit_object()") \
 template(ROMVector,    visited_objects, "") \
 template(ROMVector,    visited_object_infos, "") \
 template(ROMVector,    names_of_bad_classes_vector, "Classes that throw " \
                                     "Error during attempt to load all " \
                                     "classes in JAR file.")

#define ROMWRITER_OOP_FIELDS_DO_SOURCE(template) \
 template(ConstantPool, skipped_constant_pool, "") \
 template(TypeArray,    rom_tm_bitmap, "bitmap of references inside TASK_MIRRORS_BLOCK") \
 template(ROMLookupTable, constant_string_table, "")

#define ROMWRITER_OOP_FIELDS_DO_BINARY(template) \
 template(ROMLookupTable, constant_string_table, "") \
 template(ObjArray,  binary_image_current_dictionary, "") \
 template(ObjArray,  binary_image_class_list, "") \
 template(ObjArray,  binary_image_mirror_list, "") \
 template(Oop,       eol_comment_object, "") \
 template(FilePath,  binary_input_file, "") \
 template(FilePath,  binary_output_file, "")

#define ROMWRITER_INT_FIELDS_DO_COMMON(template) \
 template(int,    state,                "Current state of the romizer") \
 template(int,    flags,                "flags") \
 template(bool,   must_suspend,          "") \
 template(int,    number_of_hashed_objects, "") \
 template(int,    symbols_start,        "offset of the first Symbol in TEXT") \
 template(int,    strings_start,        "offset of the first String in DATA") \
 template(int,    num_methods_in_image, "Number of methods in the ROM image") \
 template(int,    method_start_skip,    "") \
 template(jlong,  starting_ms,       "") \
 template(size_t, starting_free_heap,"") \
 template(size_t, ending_free_heap,  "") \
 template(ROMWriter*, singleton,     "") \
 template(jlong,  work_timer_start,  "Time when the work timer was reset") \
 template(RomOopVisitor*, visitor, "") \
 template(int, visiting_pass, "") \
 template(int, visited_all_objects_once, "") \
 template(int, gc_stackmap_size, "") \
 template(int, text_block_count, "") \
 template(int, data_block_count, "") \
 template(int, data_block_scanned_count, "") \
 template(int, heap_block_count, "") \
 template(int, heap_block_permanent_count, "") \
 template(int, symbolic_offset1, "") \
 template(int, symbolic_offset2, "") \
 template(int, symbolic_offset3, "") \
 template(int, romizer_class_count, "") \
 template(bool, visiting_object_is_current_subtype, "") \
 template(FileStreamState, summary_log_stream_state, "") \
 template(FileStreamState, optimizer_log_stream_state, "") \
 template(FileStream*, comment_stream, "")

#define ROMWRITER_INT_FIELDS_DO_SOURCE(template) \
 template(int, constant_string_count, "") \
 template(int, tm_block_count, "") \
 template(FileStreamState, declare_stream_state, "") \
 template(FileStreamState, main_stream_state, "") \
 template(FileStreamState, reloc_stream_state, "") \
 template(FileStreamState, kvm_stream_state, "")

#define ROMWRITER_INT_FIELDS_DO_BINARY(template) \
 template(BufferedFileStreamState, binary_stream_state, "") \
 template(FileStreamState, dump_stream_state, "") \
 template(int, image_target_location, "The requested location in which the" \
                                      "binary file will ideally be loaded" \
                                      "Actual loading location may vary") \
 template(ObjectWriter*, obj_writer, "") \
 template(int, dump_count, "") \
 template(int, variable_parts_count, "Number of words used by method " \
                                     "variable parts") \
 template(int, cancelled, "") \
 template(const char *, eol_comment, "")

#if ENABLE_ISOLATES
#define ROMWRITER_INT_FIELDS_DO_ISOLATE(template) \
 template(int,   romizer_task_id, "")
#else
#define ROMWRITER_INT_FIELDS_DO_ISOLATE(template)
#endif

#if ENABLE_COMPILER
#define ROMWRITER_OOP_FIELDS_DO_AOT_COMMON(template) \
 template(OffsetVector, compiled_method_list, \
          "List of precompiled methods")
#else
#define ROMWRITER_OOP_FIELDS_DO_AOT_COMMON(template)
#endif

#if USE_SOURCE_IMAGE_GENERATOR
#define ROMWRITER_OOP_FIELDS_DO(template) \
        ROMWRITER_OOP_FIELDS_DO_COMMON(template) \
        ROMWRITER_OOP_FIELDS_DO_AOT_COMMON(template) \
        ROMWRITER_OOP_FIELDS_DO_SOURCE(template)
#define ROMWRITER_INT_FIELDS_DO(template) \
        ROMWRITER_INT_FIELDS_DO_COMMON(template) \
        ROMWRITER_INT_FIELDS_DO_SOURCE(template)
#else
#define ROMWRITER_OOP_FIELDS_DO(template) \
        ROMWRITER_OOP_FIELDS_DO_COMMON(template) \
        ROMWRITER_OOP_FIELDS_DO_AOT_COMMON(template) \
        ROMWRITER_OOP_FIELDS_DO_BINARY(template)
#define ROMWRITER_INT_FIELDS_DO(template) \
        ROMWRITER_INT_FIELDS_DO_COMMON(template) \
        ROMWRITER_INT_FIELDS_DO_BINARY(template) \
        ROMWRITER_INT_FIELDS_DO_ISOLATE(template)
#endif

#define ROMWRITER_DECLARE_OOP_GETTER(type, name, comment) \
  static type * name() { \
    return (type*)&_romwriter_oops[name ## _index]; \
  }

#define ROMWRITER_DECLARE_OOP_SETTER(type, name, comment) \
  static void set_ ## name(type* value) { \
    _romwriter_oops[name ## _index] = value->obj(); \
  }

#define ROMWRITER_DECLARE_INT(type, name, comment) \
  static type _ ## name;

#define ROMWRITER_DECLARE_INT_GETTER(type, name, comment) \
  static type name() { \
    return _ ## name; \
  }

#define ROMWRITER_DECLARE_INT_SETTER(type, name, comment) \
  static void set_ ## name(type value) { \
    _ ## name = value; \
  }

#define ROMWRITER_DEFINE_INT(type, name, comment) \
  type ROMWriter::_ ## name;

#define ROMWRITER_COUNT_FIELDS(type, name, comment) \
  name ## _index,

class ROMWriter;
class ObjectWriter;

class PendingLinkDesc : public MixedOopDesc {
public:
  OopDesc *_pending_obj;
  PendingLinkDesc *_next;
};

class PendingLink: public MixedOop {
public:
  HANDLE_DEFINITION(PendingLink, MixedOop);

  static size_t allocation_size() {
    return align_allocation_size(sizeof(PendingLinkDesc));
  }
  static size_t pointer_count() {
    return 2;
  }
  static jint pending_obj_offset() {
    return FIELD_OFFSET(PendingLinkDesc, _pending_obj);
  }
  static jint next_offset() {
    return FIELD_OFFSET(PendingLinkDesc, _next);
  }

  ReturnOop pending_obj() const  {
    return obj_field(pending_obj_offset());
  }
  void set_pending_obj(Oop* oop) {
    obj_field_put(pending_obj_offset(), oop);
  }
  void clear_pending_obj() {
    obj_field_clear(pending_obj_offset());
  }

  ReturnOop next() const  {
    return obj_field(next_offset());
  }
  void set_next(Oop* oop) {
    obj_field_put(next_offset(), oop);
  }
  void clear_next() {
    obj_field_clear(next_offset());
  }
};


// This is an abstract base class for iterating over all the objects
// that appear in the ROM image. The subclasses are BlockTypeFinder,
// OffsetFinder, and ObjectWriter.
class RomOopVisitor {
protected:
  ROMWriter  *_writer;

public:
  void set_writer(ROMWriter *writer) {
    _writer = writer;
  }
  ROMWriter * writer() {
    return _writer;
  }
  virtual void begin_object(Oop* /*object*/ JVM_TRAPS)
    JVM_PURE_VIRTUAL_WITH_TRAPS;
  virtual void put_reference(Oop* /*owner*/, int /*offset*/,
                             Oop* /*object*/ JVM_TRAPS)
    JVM_PURE_VIRTUAL_WITH_TRAPS;
  virtual void put_int(Oop* /*owner*/, jint /*value*/ JVM_TRAPS)
    JVM_PURE_VIRTUAL_WITH_TRAPS;
  virtual void put_long(Oop* /*owner*/, jint /*hi*/, jint /*low*/ JVM_TRAPS)
    JVM_PURE_VIRTUAL_WITH_TRAPS;
  virtual void put_double(Oop* /*owner*/, jint /*hi*/, jint /*low*/ JVM_TRAPS)
    JVM_PURE_VIRTUAL_WITH_TRAPS;
  virtual void put_symbolic(Oop* /*owner*/, int /*offset*/ JVM_TRAPS)
    JVM_PURE_VIRTUAL_WITH_TRAPS;
  virtual void end_object(Oop* /*object*/ JVM_TRAPS)
    JVM_PURE_VIRTUAL_WITH_TRAPS;
  virtual void put_int_by_mask(Oop* owner, jint /*be_word*/, jint /*le_word*/,
                               jint /*typemask*/ JVM_TRAPS) {
    put_int(owner, 0 JVM_NO_CHECK_AT_BOTTOM);
  }
};

class OffsetVector: public ROMVector {
public:
  HANDLE_DEFINITION(OffsetVector, ROMVector);

  void initialize(int count JVM_TRAPS);
  void initialize(JVM_SINGLE_ARG_TRAPS);
  jint _compare_to(Oop *obj1, Oop* obj2);
};

class ROMWriter: public StackObj {
public:
  // Count the number of integer and oop fields in the ROMWriter class
  enum {
    ROMWRITER_OOP_FIELDS_DO(ROMWRITER_COUNT_FIELDS)
    _number_of_oop_fields
  };
  enum {
    ROMWRITER_INT_FIELDS_DO(ROMWRITER_COUNT_FIELDS)
    _number_of_int_fields
  };
  static OopDesc* _romwriter_oops[_number_of_oop_fields];

public:
  ROMWriter() {
     GUARANTEE(_singleton == NULL, "Only one ROMWriter instance should be "
               "active at any given time.");
     _singleton = this;
  }

  // Returns false on failure
  bool write(JVM_SINGLE_ARG_TRAPS);

  ~ROMWriter() {
    _singleton = NULL;
  }
  // These two functions are used to check if the romizer has used
  // more than a cetrain amount of time. If so, it will be suspended 
  // and invoked later. This allows the romizer to run concurrently with
  // other activities in the VM.
  static void start_work_timer();
  static bool work_timer_has_expired();

  static void oops_do(void do_oop(OopDesc**));
  static bool is_active() {
    return (state() >= 0);
  }
protected:

  void remove_unfinished_files() {
    OsFile_remove(Arguments::rom_output_file());
  };
  static void initialize();
  void start(JVM_SINGLE_ARG_TRAPS);
  virtual void load_all_classes(JVM_SINGLE_ARG_TRAPS) {JVM_IGNORE_TRAPS;}
  void fixup_image(JVM_SINGLE_ARG_TRAPS);
  virtual void write_image(JVM_SINGLE_ARG_TRAPS){JVM_IGNORE_TRAPS;}
  void write_reports(JVM_SINGLE_ARG_TRAPS);
  virtual void save_file_streams();
  virtual void restore_file_streams();

  virtual void visit_persistent_handles(JVM_SINGLE_ARG_TRAPS) 
           {JVM_IGNORE_TRAPS;};

public:
  FileStream  _summary_log_stream;      // used to generate ROMLog.txt
  FileStream  _optimizer_log_stream;    // used to generate ROMLog.txt

  // Declare functions such as 
  // static ConstantPool * skipped_constant_pool();
  ROMWRITER_OOP_FIELDS_DO(ROMWRITER_DECLARE_OOP_GETTER)

  // Declare functions such as 
  // static void set_skipped_constant_pool(ConstantPool * value);
  ROMWRITER_OOP_FIELDS_DO(ROMWRITER_DECLARE_OOP_SETTER)

  // Declare fields such as the following (for old code to work)
  // static int _state;  
  ROMWRITER_INT_FIELDS_DO(ROMWRITER_DECLARE_INT)

  // Declare fields accessor such as the following (should be used by new code)
  // static int state()
  // static void set_state(int value)
  ROMWRITER_INT_FIELDS_DO(ROMWRITER_DECLARE_INT_GETTER)
  ROMWRITER_INT_FIELDS_DO(ROMWRITER_DECLARE_INT_SETTER)

  static void set_next_state() {
    set_state(state() + 1);
  }
  static void suspend() {
    set_must_suspend(true);
  }

  static void rehash();
  void rehash_info_table();

#if USE_SEGMENTED_TEXT_BLOCK_WRITER
  // Useful for calculating relative TEXT block offsets
  static int         _pass_sizes[ROM::TEXT_BLOCK_SEGMENTS_COUNT];
  static FileStreamState 
    _main_segment_stream_states[ROM::SEGMENTS_STREAMS_COUNT];
  static FileStreamState _rom_generated_header_state;
  static int         _main_stream_ind_state;
#endif 

  ROMOptimizer _optimizer;

  static int _last_oop_streaming_offset;// used in streaming fields using
                                        // OopDesc::oops_do()
  static OopDesc *_streaming_oop;       // The object that's being streamed
                                        // using OopDesc::oops_do().
  static int _streaming_index;
  static TypeArray *_streaming_fieldmap;

  enum BlockType {
    UNKNOWN_BLOCK,
    TEXT_BLOCK,  // objects that stay in .text segment
    DATA_BLOCK,  // objects that stay in .data segment
    HEAP_BLOCK,  // objects that get copied into heap before they're used
    PERSISTENT_HANDLES_BLOCK,
    SYSTEM_SYMBOLS_BLOCK,
#if ENABLE_HEAP_NEARS_IN_HEAP  
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
    INFO_TABLE_SIZE        = 8192,
    DEFAULT_FIELD_MAP_SIZE = 4096
  };

  enum {
    // valid values for typemask for streaming 8- and 16-bit fields
    BYTE_BYTE_BYTE_BYTE  = 0x0f,   /* 1:1:1:1 */
    SHORT_BYTE_BYTE      = 0x03,   /* 0:0:1:1 */
    BYTE_BYTE_SHORT      = 0x0c,   /* 1:1:0:0 */
    SHORT_SHORT         = 0x00     /* 0:0:0:0 */
  };

#define NUM_TEXT_KLASS_BUCKETS 128

  void visit_already_seen(RomOopVisitor *visitor, int npass JVM_TRAPS);
  void visit_all_objects(RomOopVisitor *visitor, int npass JVM_TRAPS);
  void visit_rom_hashtable(ObjArray *table JVM_TRAPS);
  void visit_object_and_friends(Oop *object JVM_TRAPS);
  void visit_object(Oop *object, OopDesc *info JVM_TRAPS);
  inline ReturnOop info_for(Oop* object JVM_TRAPS) {
    if (info_table_last_obj()->equals(object)) {
      return info_table_last_entry()->obj();
    } else {
      *info_table_last_obj() = object->obj();
    }

    int bucket = info_hashcode(object);

    {
      AllocationDisabler raw_pointers_used_in_this_block;
      ROMizerHashEntryDesc *entry_ptr =
          (ROMizerHashEntryDesc*)info_table()->obj_at(bucket);
      OopDesc *p = object->obj();

      while (entry_ptr) {
        ROMizerHashEntryDesc *next = entry_ptr->_next;
        if (entry_ptr->_referent == p) {
          *info_table_last_entry() = entry_ptr;
          return entry_ptr;
        } else {
          entry_ptr = next;
        }
      }
    }

    // We can't find the object, create a new entry for it
    UsingFastOops level1;
    ROMizerHashEntry::Fast entry = ROMizerHashEntry::allocate(JVM_SINGLE_ARG_CHECK_0);
    // The above may have caused a GC, we need to recalc the object hash and
    // bucket
    bucket = info_hashcode(object);
    ROMizerHashEntry::Fast next = info_table()->obj_at(bucket);
    entry().set_referent(object);
    entry().set_next(&next);
    _number_of_hashed_objects ++;
    entry().set_seen(0);
    entry().set_type(UNKNOWN_BLOCK);
    entry().set_skip_words(0);
    entry().set_pass(0);
    entry().set_offset(-1);
    info_table()->obj_at_put(bucket, &entry);
    
    *info_table_last_entry() = entry.obj();
    return entry.obj();
  }

  virtual void stream_object(Oop* object JVM_TRAPS);

  bool is_seen(Oop* object JVM_TRAPS);
  void set_seen(Oop* object JVM_TRAPS);

  int  offset_of(Oop* object JVM_TRAPS);
  void set_offset_of(Oop* object, int offset JVM_TRAPS);

#if ENABLE_HEAP_NEARS_IN_HEAP    
  int  heap_offset_of(Oop* object JVM_TRAPS);
  void set_heap_offset_of(Oop* object, int offset JVM_TRAPS);
#endif

  void set_block_type_of(Oop* object, BlockType type JVM_TRAPS);  
  BlockType block_type_of(Oop* object JVM_TRAPS);
  
  void set_pass_of(Oop* object, int pass JVM_TRAPS);
  
  void set_skip_words_of(Oop* object, int skip JVM_TRAPS);
  int pass_of(Oop* object JVM_TRAPS);
  
  int skip_words_of(Oop* object JVM_TRAPS);

#if USE_SEGMENTED_TEXT_BLOCK_WRITER
  void set_loc_offset_of(Oop* object, int loc_offset JVM_TRAPS);
  int loc_offset_of(Oop* object JVM_TRAPS);
#endif

  static inline int info_hashcode(Oop *object) {
    // Note: hash code will be recomputed after each GC.
    return (((int)(object->obj())) & 0x7fffffff) % INFO_TABLE_SIZE;
  }

  static bool write_by_reference(OopDesc *obj) {
    // In Monet: if an object is already included in a loaded (system
    // or binary) image, we don't write the whole object again into
    // the output. Instead, we write only a reference to it.
#if USE_SOURCE_IMAGE_GENERATOR
    return false;
#else
    if (ROM::in_any_loaded_bundle_of_current_task(obj)) {
      return true;
    }
    // Classes in the current task should never refer to romized objects
    // in any other task!
    GUARANTEE(!ROM::in_any_loaded_bundle(obj), 
              "intra-isolate reference forbidden");
    return false;
#endif
  }
  static bool write_by_reference(Oop *oop) {
    return write_by_reference(oop->obj());
  }

  // the opposite of write_by_reference
  static bool write_by_value(OopDesc *obj) {
     return !write_by_reference(obj);
  }
  static bool write_by_value(Oop *oop) {
     return !write_by_reference(oop);
  }


  virtual void init_streams(){}
  virtual void write_copyright(Stream* /*stream*/, bool /*c_style_comments*/)
               {}
  
  void find_types(JVM_SINGLE_ARG_TRAPS);
  virtual void find_offsets(JVM_SINGLE_ARG_TRAPS) {JVM_IGNORE_TRAPS;}
  void reorder_string_offsets(JVM_SINGLE_ARG_TRAPS);

  virtual void write_objects(JVM_SINGLE_ARG_TRAPS) {JVM_IGNORE_TRAPS;}
  void write_symbol_table(JVM_SINGLE_ARG_TRAPS);
  void write_string_table(JVM_SINGLE_ARG_TRAPS);
  virtual int  write_rom_hashtable(const char* /*table_name*/,
                                   const char* /*element_name*/,
                                   ObjArray* /*table*/,
                                   ConstantPool* /*embedded_holder*/,
                                   int /*embedded_offset*/ JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    return 0;
  }
  virtual int  print_rom_hashtable_header(const char* /*table_name*/,
                                          const char* /*element_name*/,
                                          ObjArray* /*table*/, 
                                          ConstantPool* /*embedded_holder*/,
                                          int /*embedded_offset*/ JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    return 0;
  }
  virtual int  print_rom_hashtable_content(const char* /*element_name*/,
                                           ObjArray* /*table*/ JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    return 0;
  }
  
  void write_persistent_handles(ObjectWriter *obj_writer JVM_TRAPS);
  void write_system_symbols(ObjectWriter *obj_writer JVM_TRAPS);
  virtual void write_constant_string(Symbol* /*s*/ JVM_TRAPS) 
               {JVM_IGNORE_TRAPS;}
  virtual void write_constant_string_ref(Symbol* /*s*/) {}
  virtual void write_restricted_packages(JVM_SINGLE_ARG_TRAPS)
               {JVM_IGNORE_TRAPS;}
#if ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR
  virtual void write_hidden_classes(JVM_SINGLE_ARG_TRAPS) {JVM_IGNORE_TRAPS;}
  virtual void write_restricted_in_profiles() {}
  virtual void write_hidden_in_profiles() {}
#endif
  virtual void write_global_singletons(JVM_SINGLE_ARG_TRAPS) {JVM_IGNORE_TRAPS;}
#if ENABLE_PREINITED_TASK_MIRRORS && USE_SOURCE_IMAGE_GENERATOR && ENABLE_ISOLATES        
  virtual void write_tm_reference(Oop* /*owner*/, int owner_offset, Oop* /*oop*/, FileStream* /*stream*/) {}
#endif
  virtual void write_reference(Oop* /*oop*/, BlockType /*current_type*/,
                               FileStream* /*stream*/ JVM_TRAPS)
               {JVM_IGNORE_TRAPS;}
  virtual void write_reference(BlockType /*type*/, int /*offset*/,
                               BlockType /*current_type*/,
                               FileStream* /*stream*/) {}
  virtual void write_int(jint /*value*/, FileStream* /*stream*/) {}
  virtual void write_plain_int(jint /*value*/, FileStream* /*stream*/) {}
  virtual void write_double(jint /*hi*/, jint /*low*/, FileStream* /*stream*/)
          {}
  virtual void write_long(jint /*hi*/, jint /*low*/, FileStream* /*stream*/) {}
  virtual void write_null(FileStream* /*stream*/) {}

  void write_report(Stream *st, jlong elapsed);
  virtual void combine_output_files() {}
  void append_file_to(OsFile_Handle dst, const JvmPathChar *src_name);

  static void record_class_loading_failure(Symbol *class_name,
                                           Symbol *exception_class_name
                                           JVM_TRAPS);

  // Count number of methods to be written into the image.
  int count_methods();
  void print_hierarchy(Stream *st, InstanceClass *klass, int indent);
  void write_class_summary(Stream *st);
  void write_method_summary(Stream *st JVM_TRAPS);
  void write_compiled_method_summary(Stream *st JVM_TRAPS);  


  bool has_pending_object() {
    return pending_links_head()->not_null();
  }
  void add_pending_object(Oop *object JVM_TRAPS);
  ReturnOop remove_pending_object();

  static int number_of_romized_java_classes() {
#if !ENABLE_MONET
    return Universe::number_of_java_classes();
#else 
    return _romizer_class_count;
#endif
  }

  static void set_number_of_romized_java_classes() {
    _romizer_class_count = 0;
    for (SystemClassStream st; st.has_next();) {
      _romizer_class_count++;
    }
  }

private:

#if ENABLE_ISOLATES
  int stream_task_mirror(TaskMirror* tm JVM_TRAPS);
  void stream_static_fields_oopmap(InstanceClass *ic JVM_TRAPS);
#endif

#if USE_SOURCE_IMAGE_GENERATOR
  void stream_method(Method* method JVM_TRAPS);
#endif

  int  stream_instance_fields(InstanceClass* klass, Oop* object,
                              jint start_offset JVM_TRAPS);
  int  stream_static_fields(InstanceClass* klass, 
                            Oop* object, 
                            jint start_offset JVM_TRAPS);
  int  stream_java_fields(InstanceClass* klass, 
                          Oop* object, bool is_static,
                          jint start_offset JVM_TRAPS);
  int  generate_java_fieldmap(InstanceClass* klass, bool is_static JVM_TRAPS);
  int  get_java_fieldmap_size(InstanceClass* klass, bool is_static);
  int  generate_static_java_fieldmap(InstanceClass* klass);
  int  generate_instance_java_fieldmap(InstanceClass* klass);
  int  generate_instance_java_fieldmap(InstanceClass* klass, int start_index, 
                                       size_t &instance_size);
  static void generate_fieldmap_by_oops_do(OopDesc**p);

  // Field map related functions
  void alloc_field_map(int size JVM_TRAPS);
  int  get_next_field_map_item_index(int index, int skip_bytes, 
                                     int end_offset);

  int  gen_and_stream_fieldmap(Oop* object, int force_skip_words JVM_TRAPS);
  int  gen_and_stream_fieldmap(Oop* object JVM_TRAPS) {
       return gen_and_stream_fieldmap(object, 0 JVM_NO_CHECK_AT_BOTTOM);
  }
  int  stream_fields_by_map(Oop* object, int obj_pos,
                            int start_field, int field_count JVM_TRAPS);
  int  stream_field_by_type(Oop* object, int obj_pos,
                            int field_type, int field_count JVM_TRAPS);
  int  stream_small_fields(Oop * object, int obj_pos, 
                           int field, int field_count JVM_TRAPS);
  //////////////////////////////////////////////////////////////////  
  void put_int_field(Oop* object, int offset JVM_TRAPS);
  void put_int_field(Oop* object, int offset, int typemask JVM_TRAPS);
  void put_int_block(Oop* object, int offset, int size JVM_TRAPS);
  void put_short_pair_field(Oop* object, int offset JVM_TRAPS);
  void put_double_field(Oop* object, int offset JVM_TRAPS);
  void put_oop_field(Oop* object, int offset JVM_TRAPS);
  void put_long_field(Oop* object, int offset JVM_TRAPS);
  void put_symbolic_field(Oop* object, int offset JVM_TRAPS);
  
  void follow_oop_field(Oop* object, int offset JVM_TRAPS);

  void add_to_bucket(ObjArray *rom_table, int index, Oop *object);
  void put_block_element(BlockType type, int index, BlockType current_type);
  bool is_current_subtype(Oop *object);

  friend class ROM;
  friend class ROMizerHashEntry;
  friend class SourceROMWriter;
  friend class SourceObjectWriter;
  friend class BinaryROMWriter;
};

const ROMWriter::BlockType _T_ = ROMWriter::TEXT_BLOCK;
const ROMWriter::BlockType _D_ = ROMWriter::DATA_BLOCK;
const ROMWriter::BlockType _H_ = ROMWriter::HEAP_BLOCK;
const ROMWriter::BlockType _U_ = ROMWriter::UNKNOWN_BLOCK;

class BlockTypeFinder : public RomOopVisitor {
  ObjArray _meta_obj_array;

  void add_meta_type(int &n, ROMWriter::BlockType type,
                     ROMWriter::BlockType near_type, Oop *object JVM_TRAPS);
  ROMWriter::BlockType find_meta_type(Oop *object);
  void find_array_type(Oop *owner, Oop *object JVM_TRAPS);
  bool _seen_methods;

  enum {
    PASS_DEFAULT                   = 0,

    PASS_FOR_STRING_BODY           = 0,
    PASS_FOR_METHODS               = 1,
    PASS_FOR_OTHER_SYMBOLS         = 2,
    PASS_FOR_FIELDTYPE_SYMBOLS     = 3,
    PASS_FOR_SIGNATURE_SYMBOLS     = 4,
    PASS_FOR_STACKMAPS             = 5,
    PASS_FOR_STRINGS               = 6,
    PASS_FOR_CONSTANT_POOLS        = 7,
    PASS_FOR_OTHER_TEXT_OBJECTS    = 8,
    PASS_FOR_ONE_WORD_TEXT_OBJECTS = 9,

    PASS_FOR_PERMANEBT_HEAP_OBJECTS= 0,
    PASS_FOR_OTHER_HEAP_OBJECTS    = 1,

    PASS_FOR_NORMAL_DATA_OBJECTS   = 0,
    PASS_FOR_UNSCANNED_DATA_OBJECTS= 1
  };

public:
  BlockTypeFinder(JVM_SINGLE_ARG_TRAPS);
  virtual void begin_object(Oop* object JVM_TRAPS);
  virtual void put_reference(Oop* owner, int offset, Oop* object JVM_TRAPS);
  virtual void put_int(Oop* /*owner*/, jint /*value*/ JVM_TRAPS)
           {JVM_IGNORE_TRAPS;}
  virtual void put_long(Oop* /*owner*/, jint /*hi*/, jint /*low*/ JVM_TRAPS)
           {JVM_IGNORE_TRAPS;}
  virtual void put_double(Oop* /*owner*/, jint /*hi*/, jint /*low*/ JVM_TRAPS)
           {JVM_IGNORE_TRAPS;}
  virtual void put_symbolic(Oop* /*owner*/, int /*offset*/ JVM_TRAPS)
           {JVM_IGNORE_TRAPS;}
  virtual void end_object(Oop* /*object*/ JVM_TRAPS)
           {JVM_IGNORE_TRAPS;}
  void find_type(Oop *owner, Oop *object JVM_TRAPS);
  void do_method(Method* method, ROMWriter::BlockType &my_type, int &my_pass,
                 int &my_skip_words);
  void do_compiled_method(CompiledMethod* method,
                          ROMWriter::BlockType &my_type,
                          int &my_pass, int &my_skip_words JVM_TRAPS);
  void finish();
};

class ObjectWriter : public RomOopVisitor {
protected:
  int _offset;
  int _variable_parts_offset;
  bool _string_started;
  ROMWriter::BlockType _current_type;
  ObjArray _method_variable_parts;
  ROMOptimizer *optimizer;
  Method _saved_current_method;
  Method _saved_alt_method;

public:
  ObjectWriter() : _method_variable_parts() {
  }

  virtual void start_block(ROMWriter::BlockType /*type*/, int /*preset_count*/
                           JVM_TRAPS)          {JVM_IGNORE_TRAPS;}
  virtual void end_block(JVM_SINGLE_ARG_TRAPS) {JVM_IGNORE_TRAPS;}
  int variable_parts_offset() { return _variable_parts_offset;}

#define INTERPRETER_ENTRIES_DO(template)          \
  template(interpreter_method_entry)              \
  template(interpreter_fast_method_entry_0)       \
  template(interpreter_fast_method_entry_1)       \
  template(interpreter_fast_method_entry_2)       \
  template(interpreter_fast_method_entry_3)       \
  template(interpreter_fast_method_entry_4)       \
  \
  template(fixed_interpreter_fast_method_entry_0) \
  template(fixed_interpreter_fast_method_entry_1) \
  template(fixed_interpreter_fast_method_entry_2) \
  template(fixed_interpreter_fast_method_entry_3) \
  template(fixed_interpreter_fast_method_entry_4) \
  \
  template(quick_void_native_method_entry)        \
  template(quick_int_native_method_entry)         \
  template(quick_obj_native_method_entry)         \
  \
  template(shared_fast_getbyte_accessor)          \
  template(shared_fast_getshort_accessor)         \
  template(shared_fast_getchar_accessor)          \
  template(shared_fast_getint_accessor)           \
  template(shared_fast_getlong_accessor)          \
  template(shared_fast_getbyte_static_accessor)   \
  template(shared_fast_getshort_static_accessor)  \
  template(shared_fast_getchar_static_accessor)   \
  template(shared_fast_getint_static_accessor)    \
  template(shared_fast_getlong_static_accessor)   \
  \
  template(shared_fast_getfloat_accessor)         \
  template(shared_fast_getdouble_accessor)        \
  template(shared_fast_getfloat_static_accessor)  \
  template(shared_fast_getdouble_static_accessor)


#if ENABLE_COMPILER
#define COMPILER_ENTRIES_DO(template)      \
  template(fixed_interpreter_method_entry)
#else
#define COMPILER_ENTRIES_DO(template)
#endif

#if ENABLE_THUMB_COMPILER
#define THUMB_COMPILER_INTERPRETER_ENTRIES_DO(template) \
  template(jvm_ladd)  \
  template(jvm_lsub)  \
  template(jvm_land)  \
  template(jvm_lor)   \
  template(jvm_lxor)  \
  template(jvm_lcmp)  \
  template(jvm_lmin)  \
  template(jvm_lmax)  \
  template(jvm_lmul)  \
  template(jvm_lshl)  \
  template(jvm_lshr)  \
  template(jvm_lushr)
#else
#define THUMB_COMPILER_INTERPRETER_ENTRIES_DO(template)
#endif

#if defined(ARM) && ENABLE_FLOAT && !ENABLE_SOFT_FLOAT
#define COMPILER_FLOAT_ENTRIES_DO(template) \
  template(jvm_fadd)  \
  template(jvm_fsub)  \
  template(jvm_fmul)  \
  template(jvm_fdiv)  \
  template(jvm_frem)  \
  template(jvm_dadd)  \
  template(jvm_dsub)  \
  template(jvm_dmul)  \
  template(jvm_ddiv)  \
  template(jvm_drem)  \
  template(jvm_fcmpl)  \
  template(jvm_fcmpg)  \
  template(jvm_dcmpl)  \
  template(jvm_dcmpg)  \
  template(jvm_i2d)  \
  template(jvm_d2i)  \
  template(jvm_f2i)  \
  template(jvm_i2f)  \
  template(jvm_d2l)  \
  template(jvm_l2d)  \
  template(jvm_f2l)  \
  template(jvm_l2f)  \
  template(jvm_d2f)  \
  template(jvm_f2d)  
#else
#define COMPILER_FLOAT_ENTRIES_DO(template)
#endif 

#define EXECUTION_ENTRIES_DO(template)            \
  INTERPRETER_ENTRIES_DO(template)                \
  COMPILER_ENTRIES_DO(template)                   \
  THUMB_COMPILER_INTERPRETER_ENTRIES_DO(template) \
  COMPILER_FLOAT_ENTRIES_DO(template) 
  
#define ROM_DEFINE_ENTRY(x)             {(address) &x, STR(x)},

  friend class SourceROMWriter;
};

class MemCounter {
  static MemCounter* all_counters[41];
  static int counter_number;  

public:
  const char * name;
  int text_bytes, text_objects;
  int data_bytes, data_objects;
  int heap_bytes, heap_objects;  
#if ENABLE_PREINITED_TASK_MIRRORS && USE_SOURCE_IMAGE_GENERATOR && ENABLE_ISOLATES 
  int tm_bytes, tm_objects;  
#endif
  MemCounter(const char *n);

  int all_bytes() {
    return text_bytes + data_bytes + heap_bytes;
  }

  int all_objects() {
    return text_objects + data_objects + heap_objects;
  }

  int dynamic_bytes() {
    return data_bytes + heap_bytes;
  }

  int dynamic_objects() {
    return data_objects + heap_objects;
  }

  void add_text(int bytes) {
    text_objects ++;
    text_bytes += bytes;
  }
  void add_text_bytes(int bytes) {
    text_bytes += bytes;
  }
  void add_data(int bytes) {
    data_objects ++;
    data_bytes += bytes;
  }
  void add_data_bytes(int bytes) {
    data_bytes += bytes;
  }
  void add_heap(int bytes) {
    heap_objects ++;
    heap_bytes += bytes;
  }
  void add_heap_bytes(int bytes) {
    heap_bytes += bytes;
  }
#if ENABLE_PREINITED_TASK_MIRRORS && USE_SOURCE_IMAGE_GENERATOR && ENABLE_ISOLATES  
  void add_tm(int bytes) { 
    tm_objects ++; 
    tm_bytes += bytes; 
  } 
  void add_tm_bytes(int bytes) { 
    tm_bytes += bytes; 
  } 
#endif 
  void reset() {
    text_objects = 0;
    data_objects = 0;
    heap_objects = 0;
    text_bytes   = 0;
    data_bytes   = 0;
    heap_bytes   = 0;
#if ENABLE_PREINITED_TASK_MIRRORS && USE_SOURCE_IMAGE_GENERATOR && ENABLE_ISOLATES 
    tm_bytes = tm_objects = 0;
#endif  
  }

  void print(Stream *stream);
  static void print_percent(Stream *stream, const char *name, 
                            int objs, int bytes, int all_bytes);
  static void print_percent(Stream *stream, int n, int total);
  static void print_header(Stream *stream);
  static void print_separator(Stream *stream);

  static void reset_counters();
};

extern MemCounter mc_instance_class;
extern MemCounter mc_inited_class;
extern MemCounter mc_renamed_class;
extern MemCounter mc_static_fields;
extern MemCounter mc_vtable;
extern MemCounter mc_itable;
extern MemCounter mc_array_class;
extern MemCounter mc_class_info;
extern MemCounter mc_method;
extern MemCounter mc_method_header;
extern MemCounter mc_method_body;
extern MemCounter mc_compiled_method;
extern MemCounter mc_native_method;
extern MemCounter mc_abstract_method;
extern MemCounter mc_virtual_method;
extern MemCounter mc_renamed_method;
extern MemCounter mc_renamed_abstract_method;
extern MemCounter mc_clinit_method;
extern MemCounter mc_exception_table;
extern MemCounter mc_constant_pool;
extern MemCounter mc_stackmap;
extern MemCounter mc_longmaps;
extern MemCounter mc_symbol;
extern MemCounter mc_encoded_symbol;
extern MemCounter mc_string;
extern MemCounter mc_array1;
extern MemCounter mc_array2s;
extern MemCounter mc_array2c;
extern MemCounter mc_array4;
extern MemCounter mc_array8;
extern MemCounter mc_obj_array;
extern MemCounter mc_meta;
extern MemCounter mc_other;
extern MemCounter mc_pers_handles;
extern MemCounter mc_symbol_table;
extern MemCounter mc_string_table;
extern MemCounter mc_variable_parts;
extern MemCounter mc_restricted_pkgs;
extern MemCounter mc_task_mirror;
extern MemCounter mc_line_number_tables;
extern MemCounter mc_total;

/*
 * Macros for quickly looping through the romizer hashtable
 */
#define START_HASHTABLE_LOOP_WITH_RAW_POINTERS(entry_ptr) \
  { \
    AllocationDisabler raw_pointers_used_in_this_block; \
    \
    ROMizerHashEntryDesc *entry_ptr; \
    for (int bucket=0; bucket<INFO_TABLE_SIZE; bucket++) { \
      for (entry_ptr = (ROMizerHashEntryDesc*)info_table()->obj_at(bucket); \
           entry_ptr != NULL; \
           entry_ptr = entry_ptr->_next) {

#define END_HASHTABLE_LOOP_WITH_RAW_POINTERS \
      } \
    } \
  }

#endif // ENABLE_ROM_GENERATOR
