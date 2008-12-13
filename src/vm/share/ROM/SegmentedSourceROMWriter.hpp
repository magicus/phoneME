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

#if ENABLE_ROM_GENERATOR && USE_SEGMENTED_TEXT_BLOCK_WRITER

class SegmentedSourceROMWriter: public SourceROMWriter {
public:
  SegmentedSourceROMWriter();

  ~SegmentedSourceROMWriter() {
  }

  virtual void init_streams();

private: 
  virtual FileStream* main_stream();
  virtual void write_text_defines(FileStream* stream);
  virtual void write_text_undefines(FileStream* stream);
  virtual void write_includes();
  virtual void write_text_block(SourceObjectWriter* obj_writer JVM_TRAPS);
  virtual void link_output_rom_image();
  virtual void finalize_streams(); 

  virtual void write_segment_header();
  virtual void write_segment_footer();

  virtual void write_text_reference(FileStream* stream, int offset);
  virtual void write_compiled_text_reference(FileStream* stream,int offset, 
                                             int delta);

protected:
  virtual void write_data_block(SourceObjectWriter* obj_writer JVM_TRAPS);
  virtual void write_heap_block(SourceObjectWriter* obj_writer JVM_TRAPS);
  virtual void write_stuff_block(SourceObjectWriter* obj_writer JVM_TRAPS);
#if ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES 
  virtual void write_tm_body(SourceObjectWriter* obj_writer JVM_TRAPS);
  virtual void write_tm_block(SourceObjectWriter* obj_writer JVM_TRAPS);
#endif  
  virtual void init_declare_stream();

private:
  int pass_of(int global_offset);
  int loc_offset(int global_offset);

  void write_forward_declarations(FileStream* stream);
  PathChar* rom_tmp_segment_file(int index);
  PathChar* rom_segment_file(int index);
  void save_file_streams();
  void restore_file_streams();
  FileStream* set_stream(int index);
  void copy_from_tmp(int pass_ind);
  void partial_write_objects(int pass_num, int block_size, 
    SourceObjectWriter* writer JVM_TRAPS);

private:
  int _stream_ind;
  // We use _main_segment_streams[ROM::STUFF_STREAM_INDEX]
  // to write ROM objects that are not from TEXT, DATA or HEAP blocks.
  // _stuff_stream is just a shorter synonym for
  // &_main_segment_streams[ROM::STUFF_STREAM_INDEX].
  FileStream* _stuff_stream;
  FileStream _main_segment_streams[ROM::SEGMENTS_STREAMS_COUNT]; 
};

#endif // ENABLE_ROM_GENERATOR && USE_SEGMENTED_TEXT_BLOCK_WRITER
