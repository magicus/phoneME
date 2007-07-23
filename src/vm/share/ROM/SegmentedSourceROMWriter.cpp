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

#include "incls/_precompiled.incl"
#include "incls/_SegmentedSourceROMWriter.cpp.incl"

   
#if ENABLE_ROM_GENERATOR && USE_SEGMENTED_TEXT_BLOCK_WRITER

SegmentedSourceROMWriter::SegmentedSourceROMWriter() : 
  SourceROMWriter() 
{
  restore_file_streams();
}

void SegmentedSourceROMWriter::save_file_streams() {  
  SourceROMWriter::save_file_streams();

  for (int s = 0; s < ROM::SEGMENTS_STREAMS_COUNT; s++) {
    _main_segment_streams[s].save(&_main_segment_stream_states[s]);
  }
  _main_stream_ind_state = _stream_ind;
}

void SegmentedSourceROMWriter::restore_file_streams() {
  for (int s = 0; s < ROM::SEGMENTS_STREAMS_COUNT; s++) {
    _main_segment_streams[s].restore(&_main_segment_stream_states[s]);
  }
  _stream_ind = _main_stream_ind_state;
}

void SegmentedSourceROMWriter::write_text_undefines(FileStream* stream) {
  for (int i = 0; i < ROM::TEXT_BLOCK_SEGMENTS_COUNT; i++) {
    stream->print_cr("#undef TEXT%i", i);
    stream->print_cr("#undef TEXTb%i", i);
  }
}

void SegmentedSourceROMWriter::write_text_defines(FileStream* stream) {
  for (int i = 0; i < ROM::TEXT_BLOCK_SEGMENTS_COUNT; i++) {
    stream->print_cr("#define TEXT%i(x)  (int)&_rom_text_block%i[x]", i, i);
    stream->print_cr(
      "#define TEXTb%i(x) (int)&(((char*)_rom_text_block%i)[x])", i, i);
  }
}

void SegmentedSourceROMWriter::write_forward_declarations(FileStream* stream) {
  stream->cr();
  for (int i = 0; i < ROM::TEXT_BLOCK_SEGMENTS_COUNT; i++) {
    stream->print_cr("extern const int _rom_text_block%i[];", i);
  }
  stream->cr();

  for (int i = 0; i < NUM_TEXT_KLASS_BUCKETS; i++) {
    stream->print_cr("extern const int klass_table_%i[];", i);
  }
  stream->cr();
}

void SegmentedSourceROMWriter::init_declare_stream() {
  _declare_stream.print_cr("#ifndef ROM_IMAGE_GENERATED_HEADER");
  _declare_stream.print_cr("#define ROM_IMAGE_GENERATED_HEADER");
  
  SourceROMWriter::init_declare_stream();

  write_forward_declarations(&_declare_stream);
}

PathChar* SegmentedSourceROMWriter::rom_tmp_segment_file(int index) {
  const int n = index % 1000;
  // printf("  [ ] SegmentedSourceROMWriter::rom_tmp_segment_file index = %d (%d, %d, %d)\n", index, n / 100, (n % 100) / 10, n % 10);
  FilePath::rom_tmp_segment_file[9] = (JvmPathChar)((n / 100) + '0');
  FilePath::rom_tmp_segment_file[10] = (JvmPathChar)((n % 100) / 10 + '0');
  FilePath::rom_tmp_segment_file[11] = (JvmPathChar)((n % 10) + '0');
//  for (int i = 0; i < 16; i++) {
//  	printf("%c", FilePath::rom_tmp_segment_file[i]);
//  }
//  printf("\n");
  return FilePath::rom_tmp_segment_file;
}

PathChar* SegmentedSourceROMWriter::rom_segment_file(int index) {
  const int n = index % 1000;
  FilePath::rom_segment_file[9] = (JvmPathChar)((n / 100) + '0');
  FilePath::rom_segment_file[10] = (JvmPathChar)((n % 100) / 10 + '0');
  FilePath::rom_segment_file[11] = (JvmPathChar)((n % 10) + '0');
//  for (int i = 0; i < 16; i++) {
//  	printf("%c", FilePath::rom_segment_file[i]);
//  }
//  printf("\n");
  return FilePath::rom_segment_file;
}

void SegmentedSourceROMWriter::init_streams() {
  SourceROMWriter::init_streams();

  write_copyright(&_main_stream, true);
  write_segment_header();

  for (int s = 0; s < ROM::SEGMENTS_STREAMS_COUNT; s++) {    
    _main_segment_streams[s].open(rom_tmp_segment_file(s));
    write_copyright(&_main_segment_streams[s], true);    
  }

  _stuff_stream = &_main_segment_streams[ROM::STUFF_STREAM_INDEX];    
}

FileStream* SegmentedSourceROMWriter::set_stream(int index) {
  GUARANTEE(index >= ROM::MAIN_SEGMENT_INDEX &&
            index <= ROM::SEGMENTS_STREAMS_COUNT, "Sanity");
  _stream_ind = index;
  return main_stream();
}

FileStream* SegmentedSourceROMWriter::main_stream() {
  if (_stream_ind >= 0 && 
      _stream_ind < ROM::SEGMENTS_STREAMS_COUNT) {
    return &_main_segment_streams[_stream_ind];
  }
  return &_main_stream;
}

void SegmentedSourceROMWriter::partial_write_objects(
  int pass_num, int block_size, SourceObjectWriter* obj_writer JVM_TRAPS) {

  set_stream(pass_num);
  obj_writer->set_main_stream(main_stream());

  write_segment_header();

  // Writing Block.
  tty->print_cr("Writing TEXT block %i ...", pass_num);
  obj_writer->start_block(TEXT_BLOCK, block_size JVM_CHECK);
  char separator[20] = "";
  jvm_sprintf(separator, "TEXT pass %i", pass_num);
  print_separator(separator);

  visit_all_objects(obj_writer, pass_num JVM_CHECK);

  obj_writer->end_block(JVM_SINGLE_ARG_CHECK);

  write_segment_footer();
}

void SegmentedSourceROMWriter::write_includes() {
  // Include all the components of ROMImage.cpp.
  main_stream()->cr();
  main_stream()->cr();
  for (int s = 0; s < ROM::SEGMENTS_STREAMS_COUNT; s++) {
    main_stream()->print_cr("#include \"ROMImage_%02i.cpp\"", s);
  }
  main_stream()->cr();
  main_stream()->cr();
}

void SegmentedSourceROMWriter::write_text_block(SourceObjectWriter* writer
                                                JVM_TRAPS) {
  for (int i = 0; i < ROM::TEXT_BLOCK_SEGMENTS_COUNT; i++) {
    partial_write_objects(i, _pass_sizes[i] / sizeof(jint), writer JVM_CHECK);
  }
}

void SegmentedSourceROMWriter::copy_from_tmp(int pass_ind) {
  GUARANTEE(pass_ind != ROM::MAIN_SEGMENT_INDEX, "Sanity");
  // Form src temporary file name.
  OsFile_Handle seg = OsFile_open(rom_segment_file(pass_ind), "w");
  append_file_to(seg, rom_tmp_segment_file(pass_ind));

  OsFile_flush(seg);
  OsFile_close(seg);

  // Remove remporary file.
  OsFile_remove(FilePath::rom_tmp_segment_file);
}

void SegmentedSourceROMWriter::finalize_streams() {
  _declare_stream.print_cr("#endif // ROMIZING");
  _declare_stream.print_cr("#endif // ROM_IMAGE_GENERATED_HEADER");
  _main_stream.print_cr("#endif // ROMIZING");
  _reloc_stream.print_cr("#endif // ROMIZING");
}

void SegmentedSourceROMWriter::link_output_rom_image() {

  for (int s = 0; s < ROM::SEGMENTS_STREAMS_COUNT; s++) {    
    _main_segment_streams[s].close();
    copy_from_tmp(s);
  }

  OsFile_Handle dst = 
    OsFile_open(rom_segment_file(ROM::STUFF_STREAM_INDEX), "a");
  append_file_to(dst, FilePath::rom_reloc_file);
  OsFile_close(dst);

  dst = OsFile_open(Arguments::rom_output_file(), "w");
  append_file_to(dst, FilePath::rom_main_file);
  OsFile_close(dst);

  dst = OsFile_open(FilePath::rom_image_generated_file, "w");
  append_file_to(dst, FilePath::rom_declare_file);
  OsFile_close(dst);

  dst = OsFile_open(FilePath::rom_log_file, "w");
  append_file_to(dst, FilePath::rom_summary_file);
  append_file_to(dst, FilePath::rom_optimizer_file);
  OsFile_close(dst);
}

void SegmentedSourceROMWriter::write_segment_header() {
  main_stream()->print_cr("#ifdef ROMIZING");
  main_stream()->print_cr("#include \"ROMImageGenerated.hpp\"");
}

void SegmentedSourceROMWriter::write_segment_footer() {
  main_stream()->print_cr("#endif // ROMIZING");
}

void SegmentedSourceROMWriter::write_data_block(SourceObjectWriter* obj_writer 
                                                JVM_TRAPS) {
  obj_writer->set_main_stream(set_stream(ROM::DATA_STREAM_INDEX));
  SourceROMWriter::write_data_block(obj_writer JVM_CHECK);
}

void SegmentedSourceROMWriter::write_heap_block(SourceObjectWriter* obj_writer 
                                                JVM_TRAPS) {
  obj_writer->set_main_stream(set_stream(ROM::HEAP_STREAM_INDEX));
  SourceROMWriter::write_heap_block(obj_writer JVM_CHECK);
}

void SegmentedSourceROMWriter::write_stuff_block(SourceObjectWriter* obj_writer 
                                                 JVM_TRAPS) {
  obj_writer->set_main_stream(set_stream(ROM::STUFF_STREAM_INDEX));
  write_segment_header();
  write_stuff_body(obj_writer JVM_CHECK);
  write_consistency_checks();
  // We don't need to write footer for stuff here.
}
#if ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES 
void SegmentedSourceROMWriter::write_tm_body(SourceObjectWriter* obj_writer 
                                      JVM_TRAPS) {
  tty->print_cr("Writing TASK MIRRORS block ...");
  obj_writer->start_block(TASK_MIRRORS_BLOCK, _tm_block_count JVM_CHECK);
  visit_all_objects(obj_writer, 0 JVM_CHECK);
  obj_writer->end_block(JVM_SINGLE_ARG_CHECK);
}

void SegmentedSourceROMWriter::write_tm_block(SourceObjectWriter* obj_writer 
                                      JVM_TRAPS) {
  write_tm_body(obj_writer JVM_CHECK);
}
#endif
// Returns the number of pass which corresponds to the given global offset.
int SegmentedSourceROMWriter::pass_of(int global_offset) {
  GUARANTEE(global_offset >= 0, "Sanity");
  int pass = 0;
  for (int offset_chk = global_offset; offset_chk >= 0; pass++) {
    offset_chk -= _pass_sizes[pass];  
  }
  return pass - 1;
}

int SegmentedSourceROMWriter::loc_offset(int global_offset) {
  const int pass = pass_of(global_offset);
  int loc_offset = global_offset;
  for (int i = 0; i < pass; i++) {
    loc_offset -= _pass_sizes[i];
  }
  GUARANTEE(loc_offset >= 0, "Sanity");
  return loc_offset;
}

void SegmentedSourceROMWriter::write_text_reference(FileStream* stream, 
                                                    int offset) {
  unsigned int local_offset = (unsigned int) loc_offset(offset) / sizeof(int);
  stream->print("TEXT%d(0x%04x)", pass_of(offset), local_offset);
}

void SegmentedSourceROMWriter::write_compiled_text_reference(FileStream* stream,
                                                    int offset, 
                                                    int delta) {
  int pass = pass_of(offset);
  unsigned int local_offset = (unsigned int)loc_offset(offset)/sizeof(int);
  if (delta & 0x03) {
    stream->print("TEXTb%d(0x%08x * 4 + %d)", pass, local_offset, delta);
  } else {
    stream->print("TEXT%d(0x%08x + %d)", pass, local_offset, delta/4);      
  }  
}

/**
 * Writes _rom_text_klass_table, which is used by non-PRODUCT modes
 * to get the "klass" field of objects in the TEXT block. Most
 * of these objects have their "klass" field skipped to save
 * footprint.
 */
void SegmentedSourceROMWriter::write_text_klass_table(JVM_SINGLE_ARG_TRAPS) {
  tty->print_cr("Writing text_klass_table ...");

  int i;

  UsingFastOops fast_oops;  
  ROMizerHashEntry::Fast info;
  Oop::Fast oop;
  Oop::Fast klass;
  Oop::Fast record;
  int count = 0;
  TextKlassLookupTable::Fast table;
  table().initialize(NUM_TEXT_KLASS_BUCKETS, 0 JVM_CHECK);

  //
  // (1) Iterate over all objects in the _info_table, and add them into
  //     TextKlassLookupTable
  //
  for (int bucket=0; bucket<INFO_TABLE_SIZE; bucket++) {
    for (info = info_table()->obj_at(bucket); !info.is_null(); ) {
      oop = info().referent(); // get the object
      if (oop.not_null()) {
        // Why would it be NULL?
        if (is_text_subtype(info().type())) 
        {
          table().put(&oop JVM_CHECK);
          count++;
        }
      }
      info = info().next(); // move onto the next link
    }
  }

  main_stream()->print_cr("#ifndef PRODUCT");
  OffsetVector sorter;
  sorter.initialize(JVM_SINGLE_ARG_CHECK);

  tty->print_cr("  [ ] current stream index = %d", stream_index());
  //
  // (2) Print out the individual buckets
  //

  const int saved_stream = stream_index();
  for (i=0; i < NUM_TEXT_KLASS_BUCKETS; i++) {
    set_stream(stream_index() + 1);
    write_segment_header();

    int num_written = 0;
    sorter.flush();
    main_stream()->print("\nconst int klass_table_%d[] = {\n\t", i);
    record = table().get_record_at(i);

    // Sort the content of each bucket, so that we have the same output
    // when romizing on different hosts.
    while (!record.is_null()) {
      oop = table().get_key_from_record(&record);
      record = table().get_next_record(&record);
      sorter.add_element(&oop JVM_CHECK);
    }

    sorter.sort();

    for (int n=0; n<sorter.size(); n++) {
      oop = sorter.element_at(n);
      klass = oop.klass();
      if (GenerateROMComments && VerbosePointers) {
        main_stream()->print("/* (0x%x)->klass = 0x%x*/ ", 
                      (int)(oop.obj()), (int)(klass.obj()));
      }
      num_written ++;
      write_reference(&oop, TEXT_BLOCK, main_stream() JVM_CHECK);
      main_stream()->print(", ");
      write_reference(&klass, TEXT_BLOCK, main_stream() JVM_CHECK);
      if (GenerateROMComments || ((num_written % 2) == 0)) {
        main_stream()->print(",\n\t");
      } else {
        main_stream()->print(", ");
      }
    }
    main_stream()->print_cr("0, 0 };\n");
    write_segment_footer();
  }
  set_stream(saved_stream);

  // Print the table, which points to all the buckets.
  main_stream()->print_cr("const int  _rom_text_klass_table_size = %d;", 
                           NUM_TEXT_KLASS_BUCKETS);
  main_stream()->print_cr("const int* _rom_text_klass_table[] = {");
  for (i=0; i < NUM_TEXT_KLASS_BUCKETS; i++) {
    main_stream()->print_cr("\t(const int*)klass_table_%d, ", i);
  }
  main_stream()->print_cr("};");

  main_stream()->print_cr("#endif /*  PRODUCT */");
}

#endif // ENABLE_ROM_GENERATOR && USE_SEGMENTED_TEXT_BLOCK_WRITER
