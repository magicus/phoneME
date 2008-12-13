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

#if ENABLE_ROM_GENERATOR && USE_BINARY_IMAGE_GENERATOR

class BinaryObjectWriter;
class BinaryROMWriter : public ROMWriter {
private:
  enum {
    // The value and meaning of the first 4 states are values that are
    // well-defined in both SourceROMWriter and ROMWriter.
    STATE_SUCCEEDED            = -4, // a previous call to the romizer has
                                     // successfully completed.
    STATE_CANCELLED            = -3, // a previous call to the romizer has
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
    STATE_CLEANUP              = 4,
    STATE_DONE                 = 5   // This is a transient state,
                                     // will go to STATE_SUCCEEDED immediately.

  };

public:
  BinaryROMWriter();

  ~BinaryROMWriter() {}

  static void initialize();
  void start(FilePath *input, FilePath* output, int flags JVM_TRAPS);
  void start0(FilePath *input, FilePath* output, int flags JVM_TRAPS);
  void add_input_jar_to_classpath(FilePath *input JVM_TRAPS);
  virtual void load_all_classes(JVM_SINGLE_ARG_TRAPS);
  bool execute(JVM_SINGLE_ARG_TRAPS);
  bool execute0(JVM_SINGLE_ARG_TRAPS);
  virtual void write_image(JVM_SINGLE_ARG_TRAPS);
  virtual void visit_persistent_handles(JVM_SINGLE_ARG_TRAPS);

  void copy_persistent_handles(JVM_SINGLE_ARG_TRAPS);

  void write_subtype_ranges();
  static void generate_fast_fieldmap_by_oops_do(OopDesc**p);
  void write_all_objects_of_type(BlockType type JVM_TRAPS);

  // Returns: 0 ... 100 to indicate the progress of the binary
  // romization process. Or -1 if the last binary romization
  // had failed.
  static int get_progress();

  // Has no effect if image creation is not underway.
  static void cancel();

  void open_output_file();
  void remove_output_file();

public:
  BufferedFileStream _binary_stream;

  virtual void save_file_streams();
  virtual void restore_file_streams();
  virtual void init_streams();
  virtual void close_streams();
  virtual void stream_object(Oop* object JVM_TRAPS);
  
  virtual void calculate_layout(JVM_SINGLE_ARG_TRAPS);
  virtual void write_objects(JVM_SINGLE_ARG_TRAPS);
  virtual int  write_rom_hashtable(const char *table_name,
                                   const char *element_name,
                                   ObjArray *table,
                                   ConstantPool *embedded_holder,
                                   int embedded_offset JVM_TRAPS);
#if ENABLE_LIB_IMAGES
  void write_referenced_bundles();
#endif
  
#if USE_AOT_COMPILATION
  void write_compiled_method_table(JVM_SINGLE_ARG_TRAPS);
#endif
  virtual int  print_rom_hashtable_header(const char *table_name, 
                                          const char *element_name,
                                          ObjArray *table, 
                                          ConstantPool *embedded_holder,
                                          int embedded_offset JVM_TRAPS);
  virtual int  print_rom_hashtable_content(const char *element_name,
                                           ObjArray *table JVM_TRAPS);
  virtual void combine_log_files();

  void write_persistent_handles(BinaryObjectWriter &obj_writer JVM_TRAPS);

  void set_bit_for_current_offset() {
    GUARANTEE(((_binary_stream.get_counter() >> LogBytesPerWord) / 32)
                < (juint)_relocation_bit_map.length(), 
                "setting bit out of range");
    bit_set(_binary_stream.get_counter() >> LogBytesPerWord,
           (unsigned int *)_relocation_bit_map.base_address());
  }

#if ENABLE_MONET_DEBUG_DUMP
#define DEBUG_DUMP_FUNCTION
  FileStream   _dump_stream;
#define DUMP_COMMENT(x) flush_eol_comment(); _dump_stream.print x
#else
#define DEBUG_DUMP_FUNCTION {}
#define DUMP_COMMENT(x)
#endif

  void binary_dump_counter(bool /*check*/)                DEBUG_DUMP_FUNCTION;
  void binary_dump_const_int_ref(jint /*value*/)          DEBUG_DUMP_FUNCTION;
  void binary_dump_int_ref(jint /*value*/)                DEBUG_DUMP_FUNCTION;
  void binary_dump_int(jint /*value*/, jint /*offset*/)   DEBUG_DUMP_FUNCTION;
  void binary_dump_char(char /*value*/)                   DEBUG_DUMP_FUNCTION;
  void binary_dump_long(jint /*hi*/, jint /*low*/)        DEBUG_DUMP_FUNCTION;
  void binary_dump_double(jint /*hi*/, jint /*low*/)      DEBUG_DUMP_FUNCTION;
  void set_eol_comment(const char* /*cmt*/)               DEBUG_DUMP_FUNCTION;
  void set_eol_comment_object(Oop* /*object*/)            DEBUG_DUMP_FUNCTION;
  void flush_eol_comment()                                DEBUG_DUMP_FUNCTION;

  void binary_dump_int(jint /*value*/, const char* /*cmt*/)
                                                          DEBUG_DUMP_FUNCTION;
  void binary_dump_int(jint value) {
    binary_dump_int(value, (const char*)NULL);
  }

  inline int binary_stream_counter() {
    return _binary_stream.get_counter();
  }

  inline void writebinary_char(char value) {
    binary_dump_char(value);
    _binary_stream.print_char(value);
  }

  inline void writebinary_int(jint value) {
    binary_dump_int(value);
    _binary_stream.print_int(value);
  }

  inline void writebinary_int_ref(jint value) {
    binary_dump_int_ref(value);
    set_bit_for_current_offset();
    _binary_stream.print_int_ref(value);
  }

  // A reference to an object in the system TEXT/DATA blocks.
  inline void writebinary_const_int_ref(OopDesc *value) {
    binary_dump_const_int_ref((int)value);
    _binary_stream.print_int_ref((int)value);
  }

#if ENABLE_LIB_IMAGES
  // A reference to an object in the library TEXT block.
  inline void writebinary_lib_int_ref(int value) {
    binary_dump_const_int_ref(value | (0x3 << ROM::flag_start));
    set_bit_for_current_offset();
    _binary_stream.print_int_ref(value | (0x3 << ROM::flag_start));
  }
#endif

  inline void writebinary_null() {
    binary_dump_int(0, "reference");
    _binary_stream.print_int(0);
  }

  inline void writebinary_symbolic(address addr) {
    writebinary_int((jint)addr);
#if ENABLE_MONET_DEBUG_DUMP
    set_eol_comment(ROM::getNameForAddress((address)addr));
#endif
  }

#if USE_AOT_COMPILATION
  inline void writebinary_compiled_code_reference(CompiledMethod * cm 
                                                  JVM_TRAPS) {
    AZZERT_ONLY(BlockType type = block_type_of(cm JVM_CHECK));
    GUARANTEE(type == ROMWriter::TEXT_BLOCK, 
              "All compiled methods must be in TEXT block");

    int offset = offset_of(cm JVM_CHECK);      
    int delta = CompiledMethod::entry_offset();

#if ENABLE_THUMB_COMPILER
    // The low bit is set to 0x1 so that BX will automatically switch into
    // THUMB mode.
    delta += 1;
#endif

    writebinary_int_ref(binary_text_block_addr() + offset + delta);
#if ENABLE_MONET_DEBUG_DUMP
    set_eol_comment("compiled_method");
#endif
  }
#endif

  /* The number of header fields in the binary file before the actual ROM
   * image starts */
  #define BINARY_HEADER_SIZE (ROMBundle::HEADER_SIZE * sizeof(int))

  /* VM-specific number of persistent handles */
  #define PERSISTENT_HANDLES_SIZE  (Universe::__number_of_persistent_handles - Universe::NUM_HANDLES_SKIP) * sizeof(int)

  int binary_text_block_addr();
  int binary_text_block_size();
  int binary_heap_block_addr();
  int binary_heap_block_size();
  int binary_persistent_handles_addr();
  int binary_persistent_handles_size();
  int binary_symbol_table_addr();
  int binary_symbol_table_size();
  int binary_symbol_table_num_buckets();
  int binary_string_table_addr();
  int binary_string_table_size();
  int binary_string_table_num_buckets();
  int binary_method_variable_parts_addr();
  int binary_method_variable_parts_size();
  int binary_relocation_bitmap_addr();
  int binary_relocation_bitmap_size();
#if ENABLE_LIB_IMAGES
  int binary_referenced_bundles_addr();
  int binary_referenced_bundles_size();
#endif

#if USE_AOT_COMPILATION
  int binary_compiled_method_table_addr();
  int binary_compiled_method_table_size();
#endif

  int binary_total_size();

  void write_map(TypeArray *bitmap);
  TypeArray _relocation_bit_map;
  int calculate_bit_map_size();
  friend class ROM;
};

class BinaryObjectWriter : public ObjectWriter {
public:
  BinaryObjectWriter(ROMOptimizer *optimizer)
       : ObjectWriter() {
    this->optimizer = optimizer;
    _string_started = false;
    _offset = 0;
    _variable_parts_offset = 0;
  }
  void set_writer(BinaryROMWriter *writer) {
    _writer = (ROMWriter *)writer;
  }
  BinaryROMWriter * writer() {
    return (BinaryROMWriter *)_writer;
  }

  virtual void start_block(ROMWriter::BlockType type, int preset_count 
                           JVM_TRAPS);
  virtual void begin_object(Oop *object JVM_TRAPS);
  virtual void put_reference(Oop *owner, int offset, Oop *object JVM_TRAPS);
  virtual void put_int(Oop *owner, jint value JVM_TRAPS);
  virtual void put_int_by_mask(Oop *owner, jint be_word, jint le_word,
                               jint typemask JVM_TRAPS);
  virtual void put_long(Oop *owner, jint hi, jint low JVM_TRAPS);
  virtual void put_double(Oop *owner, jint hi, jint low JVM_TRAPS);
  virtual void put_symbolic(Oop *owner, int offset JVM_TRAPS);
  virtual void end_object(Oop *object JVM_TRAPS);
#if USE_ROM_LOGGING 
  void count(Oop* object, int adjustment);
  void count(class MemCounter& counter, int bytes);
#else
  void count(Oop* /*object*/, int /*adjustment*/) {}
  void count(class MemCounter& /*counter*/, int /*bytes*/) {}
#endif

  void put_method_symbolic(Method *method, int offset JVM_TRAPS);
  void put_compiled_method_symbolic(CompiledMethod *method, int offset
                                    JVM_TRAPS);
  void put_method_variable_part(Method *method JVM_TRAPS);
  void print_method_variable_parts(JVM_SINGLE_ARG_TRAPS);
  static bool has_split_variable_part(Method *method);
  void encode_heap_reference(Oop *object);

  void advance_offset() {
    _offset += sizeof(int);
  }
#if ENABLE_MONET_DEBUG_DUMP
  void dump_object(Oop *object, int skip_words);
#endif

  friend class ROMWriter;
};

#endif // ENABLE_ROM_GENERATOR && USE_BINARY_IMAGE_GENERATOR
