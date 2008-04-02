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

#if ENABLE_ROM_GENERATOR && USE_SOURCE_IMAGE_GENERATOR

class SourceObjectWriter;  // Forward reference

class SourceROMWriter : public ROMWriter {
public:
  SourceROMWriter();

  ~SourceROMWriter() {
  }

private:
  enum {
    // The value and meaning of the first 4 states are values that are
    // well-defined in both SourceROMWriter and ROMWriter.
    STATE_SUCCEEDED            = -3, // a previous call to the romizer has
                                     // successfully completed.
    STATE_FAILED               = -2, // a previous call to the romizer has 
                                     // failed
    STATE_VIRGIN               = -1, // the romizer has never been
                                     // executed since JVM_Start time
    STATE_START                = 0,  // the romizer is in the starting phase.

    // The value and meaning of the following states are specific only
    // to SourceROMWriter.
    STATE_OPTIMIZE             = 1,
    STATE_FIXUP_IMAGE          = 2,
    STATE_WRITE_IMAGE          = 3,
    STATE_WRITE_STRUCT         = 4,
    STATE_WRITE_REPORTS        = 5,
    STATE_COMBINE_OUTPUT_FILES = 6,
    STATE_DONE                 = 7   // This is a transient state,
                                     // will go to STATE_SUCCEEDED immediately.
  };

  virtual void write_image(JVM_SINGLE_ARG_TRAPS);
  virtual void visit_persistent_handles(JVM_SINGLE_ARG_TRAPS);
  virtual void load_all_classes(JVM_SINGLE_ARG_TRAPS);
  bool execute0(JVM_SINGLE_ARG_TRAPS);

#if ENABLE_COMPILER && ENABLE_APPENDED_CALLINFO
  void write_compiled_method_table(JVM_SINGLE_ARG_TRAPS);
#endif
#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  void print_packages_list(ROMVector* patterns);
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT
  static void handle_jar_entry(char* name, int length, JarFileParser *jf
                               JVM_TRAPS);
  void get_all_names_in_jar(FilePath* path, 
    int classpath_index, bool classes JVM_TRAPS);
  void sort_and_load_all_in_classpath(JVM_SINGLE_ARG_TRAPS);
  ROMVector *_sorted_class_names;

public:
  static void initialize();
  void start(JVM_SINGLE_ARG_TRAPS);

  bool execute(JVM_SINGLE_ARG_TRAPS);

  FileStream  _declare_stream;          // used to generate ROMImage.cpp
  FileStream  _main_stream;             // used to generate ROMImage.cpp
  FileStream  _reloc_stream;            // used to generate ROMImage.cpp
  FileStream  _kvm_stream;              // used to generate KvmNatives.cpp

private:
  virtual FileStream* main_stream() {
    return &_main_stream;
  }

  virtual void write_text_defines(FileStream* stream);
  virtual void write_text_undefines(FileStream* stream);
  virtual void write_includes() {};
  virtual void write_text_block(SourceObjectWriter* obj_writer JVM_TRAPS);
  virtual void link_output_rom_image();

  virtual void finalize_streams(); 

  virtual void write_segment_header() {}
  virtual void write_segment_footer() {}

  void write_general_reference(FileStream* stream, int offset, 
                               int delta, const char* block_name);
  void write_method_comments(FileStream* stream, CompiledMethod* cm);
  virtual void write_text_reference(FileStream* stream, int offset);
  virtual void write_compiled_text_reference(FileStream* stream,int offset, 
                                             int delta);
  bool is_text_subtype(int type);

protected:
  virtual void write_data_body(SourceObjectWriter* obj_writer JVM_TRAPS);
  virtual void write_heap_body(SourceObjectWriter* obj_writer JVM_TRAPS);
  virtual void write_stuff_body(SourceObjectWriter* obj_writer JVM_TRAPS);

  virtual void write_data_block(SourceObjectWriter* obj_writer JVM_TRAPS);
  virtual void write_heap_block(SourceObjectWriter* obj_writer JVM_TRAPS);
  virtual void write_stuff_block(SourceObjectWriter* obj_writer JVM_TRAPS);

#if ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES 
  virtual void write_tm_body(SourceObjectWriter* obj_writer JVM_TRAPS);
  virtual void write_tm_block(SourceObjectWriter* obj_writer JVM_TRAPS);
#endif  
  virtual void init_declare_stream();
  
public:
  void save_file_streams();
  void restore_file_streams();
  virtual void init_streams();
  virtual void write_copyright(Stream *stream, bool c_style_comments);

  void write_text_klass_table(JVM_SINGLE_ARG_TRAPS);

  virtual void write_objects(JVM_SINGLE_ARG_TRAPS);
  virtual void find_offsets(JVM_SINGLE_ARG_TRAPS);
  virtual void write_subtype_range(char *name, int skip_header_words, 
                           int start, int end);
  virtual int  write_rom_hashtable(const char *table_name, 
                                   const char *element_name,
                                   ObjArray *table,
                                   ConstantPool *embedded_holder,
                                   int embedded_offset JVM_TRAPS);
  
  virtual int  print_rom_hashtable_header(const char *table_name,
                                   const char *element_name,
                                   ObjArray *table, 
                                   ConstantPool *embedded_holder,
                                   int embedded_offset JVM_TRAPS);
  virtual int  print_rom_hashtable_content(const char *element_name,
                                           ObjArray *table JVM_TRAPS);

  void print_separator(char * section);
  void write_original_class_info_table(JVM_SINGLE_ARG_TRAPS);
  void write_original_info_strings(JVM_SINGLE_ARG_TRAPS);
  void write_constant_string(Symbol* s JVM_TRAPS);
  void write_constant_string_ref(Symbol* s);
  virtual void write_restricted_packages(JVM_SINGLE_ARG_TRAPS);
#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  virtual void write_hidden_classes(JVM_SINGLE_ARG_TRAPS);
  virtual void write_restricted_in_profiles();
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT
  virtual void write_global_singletons(JVM_SINGLE_ARG_TRAPS);
  virtual void write_link_checks();
#if ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES         
  virtual void write_tm_reference(Oop* /*owner*/, int owner_offset, Oop* /*oop*/, FileStream* /*stream*/); 
#endif 
  virtual void write_reference(Oop* oop, BlockType current_type,
                               FileStream* stream JVM_TRAPS);
  virtual void write_reference(BlockType type, int offset,
                               BlockType current_type,
                               FileStream *stream);
  void write_compiled_code_reference(CompiledMethod* cm,
                               FileStream* stream,
                               bool as_execution_entry
                               JVM_TRAPS);
  void write_compiled_method_reference(CompiledMethod* cm, 
                                       BlockType current_type,
                                       FileStream* stream JVM_TRAPS);
  virtual void write_int(jint value, FileStream* stream);
  virtual void write_plain_int(jint value, FileStream* stream);
  virtual void write_double(jint hi, jint low, FileStream* stream);
  virtual void write_long(jint hi, jint low, FileStream* stream);
  virtual void write_null(FileStream* stream);
  virtual void combine_output_files();

  void write_consistency_checks();
  void write_aot_symbol_table(JVM_SINGLE_ARG_TRAPS);

  bool may_skip_constant_pool(Method *m);
  void fixup_image(JVM_SINGLE_ARG_TRAPS);
  friend class ROM;
};

// Note: this class is usded only by source ROM generator.
// We have a much faster way to find offsets for Binary ROM generator
class OffsetFinder : public RomOopVisitor {
private:
  int _text_offset;
  int _data_offset;
  int _heap_offset;
#if ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES 
  int _task_mirrors_offset;
#endif
  int _last_text_offset;
  bool _string_started;
  ROMWriter::BlockType _current_type;
  int _romized_reference_count;
public:
  OffsetFinder() {
    _text_offset = 0;
    _data_offset = 0;
    _heap_offset = 0;
#if ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES 
    _task_mirrors_offset = 0;
#endif
    _romized_reference_count = 0;
    _last_text_offset = -4;
    _string_started = false;
  }
  virtual void begin_object(Oop *object JVM_TRAPS);
  virtual void put_reference(Oop *owner, int offset, Oop *object JVM_TRAPS);
  virtual void put_int(Oop *object, jint value JVM_TRAPS);
  virtual void put_long(Oop *object, jint hi, jint low JVM_TRAPS);
  virtual void put_double(Oop *object, jint hi, jint low JVM_TRAPS);
  virtual void put_symbolic(Oop *owner, int offset JVM_TRAPS);
  virtual void end_object(Oop *object JVM_TRAPS)                           {}

  // Number of ints in the text, data and heap blocks
  int get_text_count() {return _text_offset / sizeof(int);}
  int get_data_count() {return _data_offset / sizeof(int);}
  int get_heap_count() {return _heap_offset / sizeof(int);}
#if ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES 
  int get_task_mirrors_count() {return _task_mirrors_offset / sizeof(int);}
#endif
  int romized_reference_count() { return _romized_reference_count;}
};

class SourceObjectWriter : public ObjectWriter {
  int _word_position;
  int _preset_count;
#if USE_SEGMENTED_TEXT_BLOCK_WRITER
  int _num_passes_completed;
#endif
  FileStream *_declare_stream;
  FileStream *_stream;
  FileStream *_reloc_stream;

  enum {
    // number of words per line in the output .cpp files
    WORDS_PER_LINE = 4
  };

public:
  SourceObjectWriter(FileStream *declare, FileStream *stream,
                     FileStream *reloc,
                     ROMOptimizer *optimizer)
    : ObjectWriter() {
    _declare_stream = declare;
    _stream = stream;
    _reloc_stream = reloc;
    _word_position = 0;
    this->optimizer = optimizer;
    _string_started = false;
    _offset = 0;
    _variable_parts_offset = 0;
#if USE_SEGMENTED_TEXT_BLOCK_WRITER
    _num_passes_completed = 0;
#endif
  }
  void reset() {
    _word_position = 0;
  }
  void set_writer(SourceROMWriter *writer) {
    _writer = (ROMWriter *)writer;
  }
  SourceROMWriter * writer() {
    return (SourceROMWriter *)_writer;
  }
  void set_main_stream(FileStream* stream) {
    _stream = stream;
  }

  void put_separator();
  void start_block_comments(char *block_name);
  virtual void start_block(ROMWriter::BlockType type, int preset_count JVM_TRAPS);
  virtual void end_block(JVM_SINGLE_ARG_TRAPS);

  virtual void begin_object(Oop *object JVM_TRAPS);
  virtual void put_reference(Oop *owner, int offset, Oop *object JVM_TRAPS);
  virtual void put_int(Oop *owner, jint value JVM_TRAPS);
  virtual void put_int_by_mask(Oop *owner, jint be_word, jint le_word, jint typemask
                       JVM_TRAPS);
  virtual void put_long(Oop *owner, jint hi, jint low JVM_TRAPS);
  virtual void put_double(Oop *owner, jint hi, jint low JVM_TRAPS);
  virtual void put_symbolic(Oop *owner, int offset JVM_TRAPS);
  virtual void end_object(Oop *object JVM_TRAPS);
  void print_entry_declarations();
  void print_oopmap_declarations();
  void count(Oop *object, int adjustment);
  void count(class MemCounter& counter, int bytes) PRODUCT_RETURN;
  bool is_kvm_native(Method *method);
  void write_kvm_method_stub(Method *method, char *name);
  void put_c_function(Method *owner, address addr, Stream *stream JVM_TRAPS);
  void put_oopmap(Oop *owner, address addr);
  void put_method_symbolic(Method *method, int offset JVM_TRAPS);
  void put_compiled_method_symbolic(CompiledMethod *method,
                                    int offset JVM_TRAPS);
  void put_method_variable_part(Method *method JVM_TRAPS);
  void print_method_variable_parts(JVM_SINGLE_ARG_TRAPS);
  bool has_split_variable_part(Method *method);
  bool is_subtype(ROMWriter::BlockType type_to_check, ROMWriter::BlockType type);
#if ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES  
  void write_rom_tm_bitmap(); 
#endif 
  void put_c_function(Method *owner, address addr JVM_TRAPS) {
    put_c_function(owner, addr, _stream JVM_CHECK);
  }
  const char* get_native_function_return_type(Method *method);

  static JvmExecutionEntry jvm_core_entries[];

  friend class ROMWriter;
};

#endif // ENABLE_ROM_GENERATOR && USE_SOURCE_IMAGE_GENERATOR
