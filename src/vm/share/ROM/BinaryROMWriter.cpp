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

#include "incls/_precompiled.incl"
#include "incls/_ROMWriter.cpp.incl"

#if ENABLE_ROM_GENERATOR

#define CHECK_COUNTER(x) \
  GUARANTEE(binary_stream_counter() + _image_target_location== x, \
            "calculated offset must match actual offset")

#define WRITE_HEADER_FIELD_INT(x) \
  writebinary_int((jint)(x)); \
  set_eol_comment(#x)

#define WRITE_HEADER_FIELD_REF(x) \
  writebinary_int_ref((jint)(x)); \
  set_eol_comment(#x)

// This method is called for 1 time after JVM_Start() is called.
void BinaryROMWriter::initialize() {
  ROMWriter::initialize();
  set_state(STATE_VIRGIN);
  set_cancelled(0);
}

BinaryROMWriter::BinaryROMWriter() : ROMWriter() {
  // This constructor is NOT just called to initialize the romizer.
  // It's called every time when the romizer is resumed.
  //
  // Initialization the romizer should be done in the STATE_INITIALIZE
  // arm of BinaryROMWriter::execute()
  restore_file_streams();
}

void BinaryROMWriter::save_file_streams() {
  ROMWriter::save_file_streams();

  _binary_stream.save(&_binary_stream_state);
#if ENABLE_MONET_DEBUG_DUMP
  _dump_stream.save(&_dump_stream_state);
#endif
}

void BinaryROMWriter::restore_file_streams() {
  ROMWriter::restore_file_streams();

  _binary_stream.restore(&_binary_stream_state);
#if ENABLE_MONET_DEBUG_DUMP
  _dump_stream.restore(&_dump_stream_state);
#endif
}

// If another instance of romizer is already running, an Error is thrown
// with no side effect on the BinaryROMWriter states.
//
// Otherwise, start the romizer. If an error is encountered, a Java exception
// will be thrown (to be handled by the Java code that invokes this
// function) and the state is set to STATE_FAILED.
void BinaryROMWriter::start(FilePath *input, FilePath* output, int flags
                            JVM_TRAPS) {
  if (is_active()) {
    Throw::error(too_many_romizations JVM_THROW);
  }

  int number_of_romized_classes = ROM::number_of_system_classes();
#if ENABLE_LIB_IMAGES
  ObjArray::Raw binary_images = Task::current()->binary_images();
  if (binary_images.not_null()) {    
    for(int i = 0; i< binary_images().length(); i++) {
      ROMBundle* bun = (ROMBundle*)binary_images().obj_at(i);
      if (bun == NULL) {
        break; 
      }
      GUARANTEE(number_of_romized_classes < bun->number_of_java_classes(), 
        "bundles in the list are in order");
      number_of_romized_classes = bun->number_of_java_classes();
    }    
  }
#endif
  if (Universe::number_of_java_classes() != number_of_romized_classes) {
    // We cannot load any user classes (in this task) before running the
    // binary romizer.
    Throw::error(romization_requires_fresh_vm JVM_THROW);
  }

  if (!cancelled()) {
    start0(input, output, flags JVM_NO_CHECK);
  }

  if (CURRENT_HAS_PENDING_EXCEPTION || cancelled()) {
    // The exception is not cleared here, so it will be thrown
    // and will cause the loop in System.createRomImage() to terminate.
    remove_output_file();
    if (cancelled()) {
      set_state(STATE_CANCELLED);
    } else {
      set_state(STATE_FAILED);
    }
    set_cancelled(0); // you can cancel at most one creation process.
  }
}

void BinaryROMWriter::start0(FilePath *input, FilePath* output, int flags
                             JVM_TRAPS) {
  // All of these handles must be cleared before we set the state to be
  // active.
  jvm_memset(_romwriter_oops, 0, sizeof(_romwriter_oops));
  _optimizer.init_handles();
  set_variable_parts_count(0);  

  // Binary ROM image does not work with EmbeddedROMHashTables
  EmbeddedROMHashTables = false;

  /*
   * WARNING: DO NOT REMOVE THIS MESSAGE UNLESS YOU HAVE READ AND
   * UNDERSTOOD THE SECURITY IMPLICATIONS IN THE FILE
   * SecurityConsiderations.html, AND HAVE CORRECTLY IMPLEMENTED THE
   * SECURITY MEASURES AS DESCRIBED THEREIN.
   *
   * If your system does not meet the security requirements as
   * stipulated in SecurityConsiderations.html, DO
   * NOT USE the BinaryROMWriter and Binary ROM images.
   */
  tty->print_cr("");
  tty->print_cr("****warning***");
  tty->print_cr("****Binary ROM Images must be created in secured file"
                " system.");
  tty->print_cr("****Please refer to"
                " src/vm/share/ROM/SecurityConsiderations.html for more"
                " information***");
  tty->print_cr("****warning***");
  tty->print_cr("");

  add_input_jar_to_classpath(input JVM_CHECK);

  set_state(STATE_START);
  set_binary_input_file(input);
  set_binary_output_file(output);
  set_flags(flags);

  // Note: we don't clear cancel here -- otherwise if the user tries to cancel
  // even before start0(0 is called (due to thread timing), the attempt
  // to cancel will be ignored.
  // DONT-> set_cancelled(0);
#if ENABLE_ISOLATES
  set_romizer_task_id(TaskContext::current_task_id());
#endif

  ROMWriter::start(JVM_SINGLE_ARG_CHECK);
  _optimizer.initialize(&_optimizer_log_stream JVM_CHECK);

  set_next_state();
  save_file_streams();
  //tty->print_cr("My sizes = %d, %d", sizeof(*this), sizeof(ROMOptimizer));
}

void BinaryROMWriter::add_input_jar_to_classpath(FilePath *input JVM_TRAPS) {
  UsingFastOops fast_oops;
  ObjArray::Fast old_classpath = Task::current()->app_classpath();

  int new_length = old_classpath().length() + 1;
  ObjArray::Fast new_classpath = Universe::new_obj_array(new_length JVM_CHECK);
  
  ObjArray::array_copy(&old_classpath, 0,  // src
                       &new_classpath, 0,  // dst
                       old_classpath().length() JVM_MUST_SUCCEED);
  new_classpath().obj_at_put(new_length-1, input);

  Task::current()->set_app_classpath(&new_classpath);
}

void BinaryROMWriter::cancel() {
  if (is_active()) {
    set_cancelled(1);
  }
}

int BinaryROMWriter::get_progress() {
  int s = state();
  switch (s) {
  case STATE_VIRGIN:
  case STATE_FAILED:
  case STATE_CANCELLED:
    return s;
  case STATE_SUCCEEDED:
    return 100; // 100% finished
  }

  int slots = STATE_DONE - STATE_START + ROMOptimizer::number_of_states();
  GUARANTEE(slots > 0, "sanity");

  if (s == STATE_OPTIMIZE) {
    s += ROMOptimizer::state();
  } else if (s > STATE_OPTIMIZE) {
    s += ROMOptimizer::number_of_states();
  }

  int percent = (s * 100) / slots;
  if (percent > 100) {
    percent = 100;
  }
  tty->print_cr("%d:%d=%d", s, slots, percent);
  return percent;
}

void BinaryROMWriter::load_all_classes(JVM_SINGLE_ARG_TRAPS) {
  Universe::load_all_in_classpath_segment(binary_input_file() 
                                          JVM_NO_CHECK_AT_BOTTOM);
}

// Returns true iff romization is suspended and needs to be resumed later..
bool BinaryROMWriter::execute(JVM_SINGLE_ARG_TRAPS) {
#if ENABLE_PERFORMANCE_COUNTERS
  jlong start_time = Os::elapsed_counter();
  if (RomizerSleepTime > 0) {
    Os::sleep(RomizerSleepTime);
  }
#endif

  bool suspended = execute0(JVM_SINGLE_ARG_NO_CHECK);

  if (CURRENT_HAS_PENDING_EXCEPTION) {
    // The exception is not cleared here, so it will be thrown
    // and will cause the loop in System.createRomImage() to terminate.
    remove_output_file();
    set_state(STATE_FAILED);
  } else if (!suspended) {
    // Romization has completed. Reset romizer to inactive
    // to disable ROMWritet::oops_do().
    set_state(STATE_SUCCEEDED);
  }

#if ENABLE_PERFORMANCE_COUNTERS
  jlong elapsed = Os::elapsed_counter() - start_time;
  jvm_perf_count.num_of_romizer_steps ++;
  jvm_perf_count.total_romizer_hrticks += elapsed;
  if (jvm_perf_count.max_romizer_hrticks < elapsed) {
      jvm_perf_count.max_romizer_hrticks = elapsed;
  }
#endif

  return suspended;
}

bool BinaryROMWriter::execute0(JVM_SINGLE_ARG_TRAPS) {
#if 0
  static int count = 0;
  jlong ms = JVM_JavaMilliSeconds();
  tty->print_cr("doing work %d: %d.%d %dms", ++count, state(),
                ROMOptimizer::state(), int(ms) % 10000);
#endif
  start_work_timer();

  GUARANTEE(state() > STATE_START, "romizer must have been started");

  do {
    switch (state()) {
    case STATE_OPTIMIZE:
      _optimizer.optimize(&_optimizer_log_stream JVM_CHECK_0);
      if (_optimizer.is_done()) {
        set_next_state();
      }
      break;

    case STATE_FIXUP_IMAGE:
      // Fix up persistent handles, etc
      fixup_image(JVM_SINGLE_ARG_CHECK_0);
      set_next_state();
      break;

    case STATE_WRITE_IMAGE:
      // Write the ROM image
      write_image(JVM_SINGLE_ARG_CHECK_0);
      set_next_state();
      break;

    case STATE_CLEANUP:
#if USE_ROM_LOGGING
      write_reports(JVM_SINGLE_ARG_CHECK_0);
      combine_log_files();
#endif

      close_streams();
      if (_binary_stream.has_error()) {
        Throw::error(binary_file_error JVM_THROW_0);
      }

      if ((flags() & JVM_REMOVE_CLASSES_FROM_JAR) != 0) {
        bool success = 
          JarFileParser::remove_class_entries(binary_input_file() JVM_CHECK_0);
        if (!success) {
          TTY_TRACE_CR(("Failed to remove .class entries from JAR files!"));
          return false;
        }
      }
      set_next_state();
      break;
    }
  }  while (!work_timer_has_expired() && state() < STATE_DONE);

  save_file_streams();
  return (state() < STATE_DONE);
}

void BinaryROMWriter::write_persistent_handles(BinaryObjectWriter &obj_writer
                                               JVM_TRAPS) {
  Oop null_owner;
  Oop* object;
  obj_writer.start_block(PERSISTENT_HANDLES_BLOCK, 0 JVM_CHECK);
  object = binary_image_current_dictionary();
  obj_writer.put_reference(&null_owner, -1, object JVM_CHECK);           
  object = binary_image_class_list();
  obj_writer.put_reference(&null_owner, -1, object JVM_CHECK);
  object = names_of_bad_classes_array();
  obj_writer.put_reference(&null_owner, -1, object JVM_CHECK);
#if ENABLE_ISOLATES
  object = binary_image_mirror_list();
  obj_writer.put_reference(&null_owner, -1, object JVM_CHECK);
#endif
  obj_writer.end_block(JVM_SINGLE_ARG_CHECK);
}

void BinaryROMWriter::write_all_objects_of_type(BlockType type 
                                                JVM_TRAPS) {
  UsingFastOops fast_oops;
  Oop::Fast object;

  _visitor = _obj_writer;
  int size = visited_objects()->size();
  ObjArray::Fast raw_objects = visited_objects()->raw_array();
  ObjArray::Fast raw_infos   = visited_object_infos()->raw_array();

  for (int i=0; i<size; i++) {
    object = raw_objects().obj_at(i);
    ROMizerHashEntry::Raw entry = raw_infos().obj_at(i);

#if ENABLE_LIB_IMAGES
    //IMPL_NOTE: may be shall look over all bundles?
    if (ROM::in_any_loaded_bundle_of_current_task(object().obj())) {
      continue;
    }
#endif

    if (ROM::system_contains(object().obj())) {
      continue;
    }

    if (entry().type() != type) {
      continue;
    }

    if ((type == ROMWriter::HEAP_BLOCK) &&
        object.obj() <= ROM::romized_heap_marker()) {
#if ENABLE_ISOLATES && !ENABLE_LIB_IMAGES
      GUARANTEE(ObjectHeap::owner_task_id(object.obj()) != romizer_task_id(),
                "Missed heap object");
      GUARANTEE(ObjectHeap::owner_task_id(object.obj()) == SYSTEM_TASK,
                "Non-system, non-convert task heap object");
#endif
      continue;
    }
#if ENABLE_ISOLATES
    GUARANTEE(type != ROMWriter::HEAP_BLOCK ||
              ObjectHeap::owner_task_id(object.obj()) == romizer_task_id(),
              "Streaming heap object from different task");
#endif

    _obj_writer->begin_object(&object JVM_CHECK);
    stream_object(&object JVM_CHECK);

    _symbolic_offset1 = 0;
    _symbolic_offset2 = 0;
    _symbolic_offset3 = 0;
  }
}

// This function is used in Monet mode (or for any objects that contains
// either oops or ints, nothing else).
void BinaryROMWriter::stream_object(Oop* object JVM_TRAPS) {
  if (!is_current_subtype(object) || ROM::system_contains(object->obj())) {
    return;
  }
  int field_count = object->object_size() / sizeof(OopDesc*);
  if (field_count > current_fieldmap()->length()) {
    alloc_field_map(field_count JVM_CHECK);
  }
  _streaming_fieldmap = current_fieldmap();
  _streaming_oop = object->obj();

  int *base_addr = (int*)current_fieldmap()->base_address();
  memset(base_addr, 0, field_count * sizeof(jubyte));
  current_fieldmap()->byte_at_put(0, 1); // the _klass pointer

  object->obj()->oops_do(generate_fast_fieldmap_by_oops_do);
  if (object->is_method()) {
    AZZERT_ONLY(Method::Raw method = object->obj());
    GUARANTEE(!method().is_quick_native(), "not supported in Monet");

    current_fieldmap()->byte_at_put(Method::heap_execution_entry_offset()/4, 2);
    current_fieldmap()->byte_at_put(Method::variable_part_offset()/4, 2);
  }

  int skip_words = skip_words_of(object JVM_CHECK);
  if (visitor() == _obj_writer) {
    UsingFastOops fast_oops;
    Oop::Fast value;
    for (int i=skip_words; i<field_count; i++) {
      switch (current_fieldmap()->byte_at(i)) {
      case 0:
        {
          jint value = object->int_field(i * sizeof(OopDesc*));
          writebinary_int(value);
          ((BinaryObjectWriter*)_obj_writer)->advance_offset();
        }
        break;
      case 1: {
          int offset = i * sizeof(OopDesc*);
          value = object->obj_field(offset);
          _obj_writer->put_reference(object, offset, &value JVM_CHECK);
        }
        break;
      case 2:
        put_symbolic_field(object, i * sizeof(OopDesc*) JVM_CHECK);
        break;
      default:
        SHOULD_NOT_REACH_HERE();
      }
    }
  } else {
    for (int i=skip_words; i<field_count; i++) {
      switch (current_fieldmap()->byte_at(i)) {
      case 0:
        {
          //put_int_field(object, i * sizeof(OopDesc*) JVM_CHECK);
          jint value = object->int_field(i * sizeof(OopDesc*));
          visitor()->put_int(object, value JVM_CHECK);
        }
        break;
      case 1:
        put_oop_field(object, i * sizeof(OopDesc*) JVM_CHECK);
        break;
      case 2:
        put_symbolic_field(object, i * sizeof(OopDesc*) JVM_CHECK);
        break;
      default:
        SHOULD_NOT_REACH_HERE();
      }
    }
  }
}

void BinaryROMWriter::generate_fast_fieldmap_by_oops_do(OopDesc**p) {
  int i = ((int)p - (int)_streaming_oop) / sizeof(OopDesc*);
  _streaming_fieldmap->byte_at_put(i, 1);
}


void BinaryROMWriter::write_image(JVM_SINGLE_ARG_TRAPS) {
  copy_persistent_handles(JVM_SINGLE_ARG_CHECK);

  // Find out where all the objects should live (TEXT or HEAP)
  // and at what offset). Then write them to the output files.
  find_types(JVM_SINGLE_ARG_CHECK);
  calculate_layout(JVM_SINGLE_ARG_CHECK);
  _optimizer.set_classes_as_romized();

  int relocation_bit_map_size = calculate_bit_map_size();
  _relocation_bit_map =
      Universe::new_int_array(relocation_bit_map_size JVM_CHECK);

  write_objects(JVM_SINGLE_ARG_CHECK);
  _ending_free_heap = ObjectHeap::free_memory();
}

void BinaryROMWriter::visit_persistent_handles(JVM_SINGLE_ARG_TRAPS) {
  visit_object(binary_image_current_dictionary(), NULL JVM_CHECK);
  visit_object(binary_image_class_list() , NULL JVM_CHECK);
  visit_object(names_of_bad_classes_array(), NULL JVM_CHECK);
#if ENABLE_ISOLATES
  visit_object(binary_image_mirror_list() , NULL JVM_CHECK);
#endif
}

void BinaryROMWriter::copy_persistent_handles(JVM_SINGLE_ARG_TRAPS) {
#if ENABLE_LIB_IMAGES  
  int offset = Task::current()->classes_in_images();
#else
  int offset = ROM::number_of_system_classes();
#endif
  int shrunk_size = Universe::number_of_java_classes() - offset;
  if (shrunk_size < 0) {
    shrunk_size = 0;
  }

  *binary_image_class_list() = 
      Universe::new_obj_array(shrunk_size JVM_CHECK);
  ObjArray::array_copy(Universe::class_list(), offset, // src
                       binary_image_class_list(), 0,   // dst
                       shrunk_size JVM_MUST_SUCCEED);

#if ENABLE_ISOLATES
  *binary_image_mirror_list() = 
      Universe::new_obj_array(shrunk_size JVM_CHECK); 
  ObjArray::array_copy(Universe::mirror_list(), offset, // src
                       binary_image_mirror_list(), 0,   // dst
                       shrunk_size JVM_MUST_SUCCEED);

#endif

  // Copy the dictionaty, since it might be below ROM::_romized_heap_marker
  int dict_size = Universe::current_dictionary()->length();
  *binary_image_current_dictionary() =
      Universe::new_obj_array(dict_size JVM_CHECK);
  ObjArray::array_copy(Universe::current_dictionary(), 0,     // src
                       binary_image_current_dictionary(), 0,  // dst
                       dict_size JVM_MUST_SUCCEED);  
}

/*
 * Create the streams used for generating ROMImage.cpp and ROMLog.txt.
 * Note we use several streams for ROMImage.cpp. That's because we
 * sometimes need to generate two blocks of code at the same time.
 *
 * We also use two streams for ROMLog.txt so that we can put the summary at
 * the top of the file.
 */
void BinaryROMWriter::init_streams() {
#if USE_ROM_LOGGING
  GUARANTEE(_summary_log_stream_state._file == NULL,   "must be clean state");
  GUARANTEE(_optimizer_log_stream_state._file == NULL, "must be clean state");
  _summary_log_stream.open(FilePath::rom_summary_file);
  _optimizer_log_stream.open(FilePath::rom_optimizer_file);
#endif

#if ENABLE_MONET_DEBUG_DUMP
  GUARANTEE(_dump_stream_state._file == NULL,           "must be clean state");
  const JvmPathChar* rom_dump_file = FilePath::rom_dump_file;
  const int buffer_length = 1024;
  JvmPathChar name[buffer_length];

  if (SaveSerialROMLogs) {
    static int serial = 0;
    const int name_length = fn_strlen(rom_dump_file);
    const int num_length = 6;
    JvmPathChar num[num_length + 1];

    if (name_length + num_length < buffer_length) {
      jvm_memcpy(name, rom_dump_file, 
                 (name_length+1) * sizeof(JvmPathChar));
  
      tty->print_cr("ROM dump serial = %d", serial);
      num[0] = '.';
      num[1] = '0' + ((serial / 10000) % 10);
      num[2] = '0' + ((serial / 1000)  % 10);
      num[3] = '0' + ((serial / 100)   % 10);
      num[4] = '0' + ((serial / 10)    % 10);
      num[5] = '0' + ((serial / 1)     % 10);
      num[6] = 0;
      fn_strcat(name, num);
      serial ++;

      rom_dump_file = name;
    }
  }
  _dump_stream.open(rom_dump_file);
  _dump_count = 0;
  _eol_comment = NULL;
#endif

  GUARANTEE(_binary_stream_state._file == NULL, "must be clean state");

  /* Open the binary ROM image file */
  open_output_file();

  /* Print the magic number and target location of the binary ROM image */
  OopDesc *header_tag = Universe::int_array_class()->prototypical_near();
  int header_word_count = ROMBundle::HEADER_SIZE -
                          sizeof(ArrayDesc)/BytesPerWord;

  WRITE_HEADER_FIELD_INT(header_tag);
  WRITE_HEADER_FIELD_INT(header_word_count);
  WRITE_HEADER_FIELD_INT(ROM_BINARY_MAGIC);
  WRITE_HEADER_FIELD_INT(JVM_GetVersionID());    
  int rom_flags = _image_target_location;
  WRITE_HEADER_FIELD_INT(rom_flags); // must not be WRITE_HEADER_FIELD_REF!
  
  int rom_bundle_id = Os::java_time_millis();
  WRITE_HEADER_FIELD_INT(rom_bundle_id); // ROM_BUNDLE_ID
#if ENABLE_LIB_IMAGES
  WRITE_HEADER_FIELD_INT(flags() & JVM_GENERATE_SHARED_IMAGE);
  if (flags() & JVM_GENERATE_SHARED_IMAGE) printf("Generate shared!\n");
#else
  WRITE_HEADER_FIELD_INT(false/*no Shared images supported*/);
#endif
}

void BinaryROMWriter::close_streams() {
#if ENABLE_MONET_DEBUG_DUMP
   flush_eol_comment();
   _dump_stream.close(&_dump_stream_state);
#endif

  _binary_stream.close(&_binary_stream_state);
}

void BinaryROMWriter::open_output_file() {
  const JvmPathChar *name = Arguments::rom_output_file();

  const int buffer_size = 512;
  DECLARE_STATIC_BUFFER(JvmPathChar, file_name, buffer_size);
  if (binary_output_file()->not_null()) {
    binary_output_file()->string_copy(file_name, buffer_size);
    name = file_name;
  }

  _binary_stream.open(name);

  // Pick a value for the requested location in which the binary file 
  // will ideally be loaded.
#if USE_IMAGE_MAPPING
  _image_target_location = (int)OsFile_ImagePreferredAddress(name);
#else
  // If we use _heap_start, the image's TEXT will block will require
  // no relocation when loaded into an SVM mode VM. See
  // ROM::preload_bundle()
  _image_target_location = (int)_heap_start;
#endif
}

void BinaryROMWriter::remove_output_file() {
  UsingFastOops fast_oops;
  Oop::Fast saved_exception = Thread::current_pending_exception();
  Thread::clear_current_pending_exception();

#if USE_ROM_LOGGING
  combine_log_files();
#endif

  close_streams();

  // Delete the binary output file.
  const JvmPathChar *name = Arguments::rom_output_file();

  const int buffer_size = 512;
  DECLARE_STATIC_BUFFER(JvmPathChar, file_name, buffer_size);
  if (binary_output_file()->not_null()) {
    binary_output_file()->string_copy(file_name, buffer_size);
    name = file_name;
  }

  OsFile_remove(name);
  Thread::set_current_pending_exception(&saved_exception);
}

// This is a fast implementation that does not use OffsetFinder, because
// all blocks in the BinaryROM contains a single pass. It does the following
//
//    Determine the size of text and heap blocks
//    Determine the size of symbol and string table
//    Determine the size of method variable part
//
// The only thing not determined here is the size of the bitmap. This
// is OK because it's located at the very end of the image.
void BinaryROMWriter::calculate_layout(JVM_SINGLE_ARG_TRAPS) {
  int size = visited_objects()->size();
  UsingFastOops fast_oops;
  ObjArray::Fast raw_objects = visited_objects()->raw_array();
  ObjArray::Fast raw_infos   = visited_object_infos()->raw_array();
  int text_offset = 0;
  int heap_offset = 0;
  OopDesc *marker = ROM::romized_heap_marker();

  // We create separate symbol and string tables that contain only symbols and
  // strings referenced from the application. This is to avoid writing all the
  // other symbols and strings to the bundle.
  int rom_symbol_count = 0;
  int rom_string_count = 0;
  ObjArray::Fast rom_symbol_table;
  ObjArray::Fast rom_string_table;
  ObjArray::Fast table;
  {
    // To save space we use one array for strings and symbols:
    // put symbols from the beginning and strings from the end.
    // Can't use an expanding vector: allocation is not allowed in the loop.
    table = Universe::new_obj_array(size JVM_CHECK);

    {
      AllocationDisabler raw_pointers_used_in_this_block;

      for (int i=0; i<size; i++) {
        ROMizerHashEntryDesc *entry_ptr =
          (ROMizerHashEntryDesc *)raw_infos().obj_at(i);
        OopDesc *obj = raw_objects().obj_at(i);
        int type       = entry_ptr->_type;
        int byte_size  = obj->object_size(); 
        
        if (ROM::system_contains(obj)) {
          GUARANTEE(type == ROMWriter::TEXT_BLOCK, "sanity");
          entry_ptr->_offset = (int)obj;
        } else {
          if (type == ROMWriter::TEXT_BLOCK) {
            int skip_bytes = entry_ptr->_skip_words * BytesPerWord;
            byte_size -= skip_bytes;
            entry_ptr->_offset = text_offset - skip_bytes;
            text_offset += byte_size;

            if (obj->is_symbol()) {
              table().obj_at_put(rom_symbol_count++, obj);
            } else if (obj->is_string()) {
              table().obj_at_put(size - (++rom_string_count), obj);
            } else if (obj->is_method()) {
              if (BinaryObjectWriter::has_split_variable_part((Method*)&obj)) {
                _variable_parts_count ++;
              }
            }
          }
          else if (type == ROMWriter::HEAP_BLOCK) {
            if (obj > marker) {
#if ENABLE_ISOLATES && !ENABLE_LIB_IMAGES
              GUARANTEE(ObjectHeap::owner_task_id(obj) == romizer_task_id(),
                        "Streaming heap object from wrong task");
#endif
              GUARANTEE(entry_ptr->_skip_words == 0, 
                        "heap objects cannot have skipped header");
              entry_ptr->_offset = heap_offset;
              heap_offset += byte_size;
            } else {
#if ENABLE_ISOLATES && !ENABLE_LIB_IMAGES //object from another bundle!
              GUARANTEE(ObjectHeap::owner_task_id(obj) != romizer_task_id(),
                        "Missed heap object");
              GUARANTEE(ObjectHeap::owner_task_id(obj) == SYSTEM_TASK,
                        "Non-system, non-convert task heap object");
#endif
            }
          }
          else {
            // No objects should be in DATA block
            SHOULD_NOT_REACH_HERE();
          }
        }
      }
    }

    _text_block_count = text_offset / BytesPerWord;
    _heap_block_count = heap_offset / BytesPerWord;

    rom_symbol_table = 
      Universe::new_obj_array(rom_symbol_count JVM_CHECK);
    rom_string_table = 
      Universe::new_obj_array(rom_string_count JVM_CHECK);
    ObjArray::array_copy(&table, 0, &rom_symbol_table, 0, 
                         rom_symbol_count JVM_CHECK);
    ObjArray::array_copy(&table, size - rom_string_count, &rom_string_table, 0, 
                         rom_string_count JVM_CHECK);
  }

  _optimizer.initialize_hashtables(&rom_symbol_table, &rom_string_table 
                                    JVM_NO_CHECK_AT_BOTTOM);  
}


void BinaryROMWriter::write_objects(JVM_SINGLE_ARG_TRAPS) {
  Oop null_obj;
  ObjArray table;
  BinaryObjectWriter obj_writer(&_optimizer);
  obj_writer.set_writer(this);
  this->_obj_writer = &obj_writer;
  const int variable_parts_size = variable_parts_count() * sizeof(int);

  // In the imagem we adjust the text_block so that it covers the header
  // as well (and the header appears as a Java integer array. This makes
  // it easy to relocate references to ROMBundle's in the case
  // of USE_LARGE_OBJECT_AREA.
  int adj_text_block_offset = binary_text_block_addr() - BINARY_HEADER_SIZE;
  int adj_text_block_size   = binary_text_block_size()   + BINARY_HEADER_SIZE;

  // (1) Write the binary ROM image header data
#if ENABLE_LIB_IMAGES
  WRITE_HEADER_FIELD_REF(binary_referenced_bundles_addr()); // ROM_BUNDLE_OFFSET
#else 
  writebinary_int(0); //NOT USED
#endif
  WRITE_HEADER_FIELD_REF(adj_text_block_offset);
  WRITE_HEADER_FIELD_REF(binary_symbol_table_addr());
  WRITE_HEADER_FIELD_REF(binary_string_table_addr());
#if USE_AOT_COMPILATION
  WRITE_HEADER_FIELD_REF(binary_compiled_method_table_addr());
#endif
  WRITE_HEADER_FIELD_REF(binary_method_variable_parts_addr());
  WRITE_HEADER_FIELD_REF(binary_persistent_handles_addr());
  WRITE_HEADER_FIELD_REF(binary_heap_block_addr());

  int bitmap_offset = binary_relocation_bitmap_addr() - _image_target_location;
  WRITE_HEADER_FIELD_INT(bitmap_offset);

  WRITE_HEADER_FIELD_INT(adj_text_block_size);
  WRITE_HEADER_FIELD_INT(binary_symbol_table_num_buckets());
  WRITE_HEADER_FIELD_INT(binary_string_table_num_buckets());
#if USE_AOT_COMPILATION
  WRITE_HEADER_FIELD_INT(binary_compiled_method_table_size());
#endif
  WRITE_HEADER_FIELD_INT(variable_parts_size);
  WRITE_HEADER_FIELD_INT(binary_persistent_handles_size());
  WRITE_HEADER_FIELD_INT(binary_heap_block_size());

  WRITE_HEADER_FIELD_INT(gc_stackmap_size());

  /* system classes + app classes */
  WRITE_HEADER_FIELD_INT(Universe::number_of_java_classes());

  CHECK_COUNTER(binary_text_block_addr());
  DUMP_COMMENT(("=============TEXT Block==============="));
  obj_writer.start_block(TEXT_BLOCK, _text_block_count JVM_CHECK);
  write_all_objects_of_type(TEXT_BLOCK JVM_CHECK);
  obj_writer.end_block(JVM_SINGLE_ARG_CHECK);

#if USE_AOT_COMPILATION
  CHECK_COUNTER(binary_compiled_method_table_addr());
  DUMP_COMMENT(("=============Compiled Methods======================="));
  write_compiled_method_table(JVM_SINGLE_ARG_CHECK);
#endif

  CHECK_COUNTER(binary_symbol_table_addr());
  write_symbol_table(JVM_SINGLE_ARG_CHECK);

  CHECK_COUNTER(binary_string_table_addr());
  write_string_table(JVM_SINGLE_ARG_CHECK);

#if ENABLE_LIB_IMAGES
  CHECK_COUNTER(binary_referenced_bundles_addr());
  DUMP_COMMENT(("=============referenced bundles=================="));
  write_referenced_bundles();
#endif

  CHECK_COUNTER(binary_method_variable_parts_addr());
  DUMP_COMMENT(("=============Method Variable Parts==============="));
  obj_writer.print_method_variable_parts(JVM_SINGLE_ARG_CHECK);

  CHECK_COUNTER(binary_persistent_handles_addr());
  DUMP_COMMENT(("=============Persistent Handles==============="));
  write_persistent_handles(obj_writer JVM_CHECK);

  CHECK_COUNTER(binary_heap_block_addr());
  DUMP_COMMENT(("=============HEAP Block==============="));
  obj_writer.start_block(HEAP_BLOCK, _heap_block_count JVM_CHECK);
  write_all_objects_of_type(HEAP_BLOCK JVM_CHECK);
  obj_writer.end_block(JVM_SINGLE_ARG_CHECK);

  DUMP_COMMENT(("=============relocation bitmap==================="));
  write_map(&_relocation_bit_map);

  CHECK_COUNTER(binary_total_size());
}

int BinaryROMWriter::binary_text_block_addr() {
  return  _image_target_location + BINARY_HEADER_SIZE;
}

int BinaryROMWriter::binary_text_block_size() {
  return  _text_block_count * sizeof(int);
}

#if USE_AOT_COMPILATION

int BinaryROMWriter::binary_compiled_method_table_addr() {
  return binary_text_block_addr() + binary_text_block_size();
}

int BinaryROMWriter::binary_compiled_method_table_size() {
  return compiled_method_list()->size() * sizeof(OopDesc*);
}

int BinaryROMWriter::binary_symbol_table_addr() {
  return binary_compiled_method_table_addr() + 
    binary_compiled_method_table_size();
}

#else

int BinaryROMWriter::binary_symbol_table_addr() {
  return binary_text_block_addr() + binary_text_block_size();
}

#endif

int BinaryROMWriter::binary_symbol_table_num_buckets() {
  ObjArray::Raw table = _optimizer.symbol_table();
  if (table.is_null()) {
    return 0;
  } else {
    return table().length();
  }
}

int BinaryROMWriter::binary_string_table_num_buckets() {
  ObjArray::Raw table = _optimizer.string_table();
  if (table.is_null()) {
    return 0;
  } else {
    return table().length();
  }
}

int BinaryROMWriter::binary_symbol_table_size() {
  ObjArray::Raw table = _optimizer.symbol_table();
  ConstantPool::Raw embedded_holder = _optimizer.embedded_table_holder();
  if (table.is_null()) {
    return 0;
  } else {
    if (embedded_holder.is_null()) {
      int count = 0;
      for (int i = 0; i < table().length(); i++) {
        ObjArray::Raw bucket = table().obj_at(i);
        count += bucket().length();
      }
      return (table().length() + count + 1) * sizeof(int);
    } else {
      return (table().length() + 1) * sizeof(int);
    }
  }
}

int BinaryROMWriter::binary_string_table_addr() {
  return binary_symbol_table_addr() + binary_symbol_table_size();
}

int BinaryROMWriter::binary_string_table_size() {
  ObjArray::Raw table = _optimizer.string_table();
  ConstantPool::Raw embedded_holder = _optimizer.embedded_table_holder();
  if (table.is_null()) {
    return 0;
  } else {
    if (embedded_holder.is_null()) {
      int count = 0;
      for (int i = 0; i < table().length(); i++) {
        ObjArray::Raw bucket = table().obj_at(i);
        count += bucket().length();
      }
      return (table().length() + count + 1) * sizeof(int);
    } else {
      return (table().length() + 1) * sizeof(int);
    }
  }
}

/* IMPL_NOTE: Temporarily overrides the variable declaration in ObjectWriter */
/* We don't want this field to be reset back to 0, because otherwise */
/* we cannot generate variable parts info in the end of the binary ROM */
/* image generation process */
#if ENABLE_LIB_IMAGES
int BinaryROMWriter::binary_referenced_bundles_addr() { 
  return binary_string_table_addr() + binary_string_table_size();
}

int BinaryROMWriter::binary_referenced_bundles_size() {
  return 4*(1 + Task::current()->get_shared_libs_num());
}

int BinaryROMWriter::binary_method_variable_parts_addr() {
  return binary_referenced_bundles_addr() + binary_referenced_bundles_size();
}
#else
int BinaryROMWriter::binary_method_variable_parts_addr() {
  return binary_string_table_addr() + binary_string_table_size();
}
#endif //ENABLE_LIB_IMAGES

int BinaryROMWriter::binary_method_variable_parts_size() {
  return _variable_parts_count * sizeof(int);
}

int BinaryROMWriter::binary_persistent_handles_addr() {
  return binary_method_variable_parts_addr() + 
         binary_method_variable_parts_size();
}

int BinaryROMWriter::binary_persistent_handles_size() {
  // we only print out the current_dictionary handle,
  // class_list and mirror_list
#if ENABLE_ISOLATES
  return 4 * sizeof(int);
#else
  // just dictionary_list and class_list
  return 3 * sizeof(int);
#endif
}

int BinaryROMWriter::binary_heap_block_addr() {
  return binary_persistent_handles_addr() + 
      binary_persistent_handles_size();
}

int BinaryROMWriter::binary_heap_block_size() {
  return _heap_block_count * sizeof(int);
}


void BinaryROMWriter::write_map(TypeArray *bitmap) {
  int i;

  int len = bitmap->length();

  /* Write the size of the bit map */
  writebinary_int(len * sizeof(int));

  /* Write the bit map of pointers that need relocation */
  for (i = 0; i < len; i++) {
    writebinary_int(bitmap->int_at(i));
  }

#if ENABLE_MONET_DEBUG_DUMP
  _dump_stream.cr();
  _dump_stream.print_cr("=============END OF IMAGE===============");
  _dump_stream.cr();
  _dump_stream.print_cr("The following is details of the bitmaps");
  _dump_stream.print_cr("---------------------------------------");
  int addr   = _image_target_location;
  int offset = 0x0;
  for (i = 0; i < len; i++) {
    juint n = (juint)bitmap->int_at(i);
    for (int j=0; j<32; j++, addr+=4, offset+=4) {
      if (((n >> j) & 0x01) == 0x01) {
        _dump_stream.print_cr("%8d 0x%08x", offset, addr);
      }
    }
  }
  _dump_stream.print_cr("---end of bitmap details---------------");
#endif
}

int BinaryROMWriter::binary_relocation_bitmap_addr() {
  return binary_heap_block_addr() + 
         binary_heap_block_size();
}

int BinaryROMWriter::binary_relocation_bitmap_size() {
  return _relocation_bit_map.length() * sizeof(int) + 
         /* size field */ sizeof(int);
}

int BinaryROMWriter::binary_total_size() {
  return binary_relocation_bitmap_addr() + binary_relocation_bitmap_size();
}



// Return the number of bytes written
int BinaryROMWriter::write_rom_hashtable(const char *table_name,
                                   const char *element_name,
                                   ObjArray *table, 
                                   ConstantPool *embedded_holder,
                                   int embedded_offset JVM_TRAPS)
{
  DUMP_COMMENT(("=============%s index============", table_name));
  int written_bytes;
  written_bytes = print_rom_hashtable_header(table_name, element_name, table, 
                                             embedded_holder, embedded_offset
                                             JVM_CHECK_0);
  
  if (embedded_holder->is_null()) {
    DUMP_COMMENT(("=============%s contents=======", table_name));
    written_bytes = print_rom_hashtable_content(element_name, table 
                                                JVM_CHECK_0);
  }

  return written_bytes;
}

int BinaryROMWriter::print_rom_hashtable_header(const char *table_name, 
                                          const char *element_name,
                                          ObjArray *table, 
                                          ConstantPool *embedded_holder,
                                          int embedded_offset JVM_TRAPS)
{
  (void)table_name;
  (void)element_name;
  int num_buckets = table->length();

  // Print the indices to the buckets
  int bytes_written = 0;
  int bucket_start;

  if (embedded_holder->not_null()) {
    bucket_start = 0;
  } else {
    bucket_start = num_buckets + 1; // plus marker for end of the last bucket
  }

  int base_offset;
  if (embedded_holder->not_null()) {
    // The table is embedded inside a constant pool
    int text_offset = offset_of(embedded_holder JVM_CHECK_0);
    GUARANTEE(text_offset >= 0, "embedded_holder must be included in bundle");
    text_offset /= BytesPerWord;
    text_offset += embedded_holder->base_offset() / BytesPerWord;
    text_offset += embedded_offset;
    base_offset = (text_offset * BytesPerWord) + binary_text_block_addr();
  } else {
    base_offset = binary_stream_counter() + _image_target_location;
  }
  
  ObjArray bucket;
  for (int b=0; b<num_buckets; b++) {
    bucket = table->obj_at(b);

    writebinary_int_ref(base_offset + bucket_start*sizeof(int));

    bucket_start += bucket.length();
    bytes_written += BytesPerWord;
  }

  // marker for the end of the last bucket
  //
  // Note that you'd see one more "BUCKET()" than the value of
  // _rom_{string,symbol}_table_num_buckets.  See comments in
  // SymbolTable.cpp for how the BUCKET() items are used in searching
  // for ROMized symbols/strings.

  writebinary_int_ref(base_offset + bucket_start*sizeof(int));

  bytes_written += BytesPerWord;

  return bytes_written;
}

int
BinaryROMWriter::print_rom_hashtable_content(const char *element_name,
                                             ObjArray *table
                                             JVM_TRAPS)
{
  (void)element_name;
  int num_buckets = table->length();
  int bytes_written = 0;
  UsingFastOops level1;
  ObjArray::Fast bucket;
  Oop::Fast oop;
  for (int b = 0; b<num_buckets; b++) {
    bucket = table->obj_at(b);
    if (bucket().length() == 0) {
      continue;
    }

    int bucket_size = bucket().length();
    for (int index=0; index<bucket_size; index++) {
      oop = bucket().obj_at(index);
      GUARANTEE(!oop.is_null(), "sanity");
      int oop_offset = offset_of(&oop JVM_CHECK_0);
      writebinary_int_ref(binary_text_block_addr() + oop_offset);

      bytes_written += 4;
    }
  }

  return bytes_written;
}

void BinaryROMWriter::combine_log_files() {
#if USE_ROM_LOGGING
  _summary_log_stream.print_cr("Binary file counter in the end of file: %lx\n",
                          binary_stream_counter() + _image_target_location);
  _summary_log_stream.close(&_summary_log_stream_state);
  _optimizer_log_stream.close(&_optimizer_log_stream_state);

  const JvmPathChar* rom_log_file = FilePath::rom_log_file;
  const int buffer_length = 1024;
  JvmPathChar name[buffer_length];

  if (SaveSerialROMLogs) {
    static int serial = 0;
    const int name_length = fn_strlen(rom_log_file);
    const int num_length = 6;
    JvmPathChar num[num_length + 1];

    if (name_length + num_length < buffer_length) {
      jvm_memcpy(name, rom_log_file, 
                 (name_length+1) * sizeof(JvmPathChar));
  
      tty->print_cr("ROM dump serial = %d", serial);
      num[0] = '.';
      num[1] = '0' + ((serial / 10000) % 10);
      num[2] = '0' + ((serial / 1000)  % 10);
      num[3] = '0' + ((serial / 100)   % 10);
      num[4] = '0' + ((serial / 10)    % 10);
      num[5] = '0' + ((serial / 1)     % 10);
      num[6] = 0;
      fn_strcat(name, num);
      serial ++;

      rom_log_file = name;
    }
  }

  OsFile_Handle dst = OsFile_open(rom_log_file, "w");
  if (dst) {
    append_file_to(dst, FilePath::rom_summary_file);
    append_file_to(dst, FilePath::rom_optimizer_file);
    OsFile_flush(dst);
    OsFile_close(dst);
  }

  OsFile_remove(FilePath::rom_summary_file);
  OsFile_remove(FilePath::rom_optimizer_file);
#endif
}

int BinaryROMWriter::calculate_bit_map_size() {
  // number of bytes
  int size = binary_relocation_bitmap_addr() - _image_target_location;

  // number of words
  size /= sizeof(int);

  // number of bitmap words
  size /= BitsPerWord;

  return size + 1;
}

#if ENABLE_LIB_IMAGES
void BinaryROMWriter::write_referenced_bundles() {  
  int i = 0;  
  writebinary_int(Task::current()->get_shared_libs_num());   
  ObjArray::Raw bundles = Task::current()->binary_images();
  if (bundles.is_null()) return;
  for (; i < bundles().length(); i++) {    
    ROMBundle* bundle = (ROMBundle*)bundles().obj_at(i);
    if (bundle == NULL) {
      break;
    }
    writebinary_int(bundle->int_at(ROMBundle::ROM_BUNDLE_ID));
  }
}
#endif

#if USE_AOT_COMPILATION
void BinaryROMWriter::write_compiled_method_table(JVM_SINGLE_ARG_TRAPS) {
  const jint text_block_addr = binary_text_block_addr();
  ROMVector * const compiled_methods = compiled_method_list();
  const int compiled_methods_count = compiled_methods->size();

  if (compiled_methods_count > 0) {
    compiled_methods->sort();
    CompiledMethod cm;
    for (int i = 0; i < compiled_methods_count; i++) {
      cm = compiled_methods->element_at(i);

      AZZERT_ONLY(BlockType type = block_type_of(&cm JVM_CHECK));
      GUARANTEE(type == ROMWriter::TEXT_BLOCK, 
                "All compiled methods should be in TEXT block");

      int offset = offset_of(&cm JVM_CHECK);      
      writebinary_int_ref(text_block_addr + offset);
    }
  }
}
#endif

#if ENABLE_MONET_DEBUG_DUMP
void BinaryROMWriter::binary_dump_counter(bool check) {
  flush_eol_comment();
  int counter = binary_stream_counter();
  if (check) {
    GUARANTEE(counter >= _dump_count, "sanity");
    if (counter > _dump_count) {
      _dump_stream.print_cr("<*** dumping of %d byte(s) skipped ***>", 
                            counter - _dump_count);
      _dump_count = counter;
    }
  }

  _dump_stream.print("%8d 0x%08x: ", counter,
                     counter + _image_target_location);
}

void BinaryROMWriter::binary_dump_int(jint value, const char *comment) {
  binary_dump_counter(true);
  _dump_stream.print("0x%08x", value);
  _dump_count += 4;
  _eol_comment = (char*)comment;
}

void BinaryROMWriter::binary_dump_int_ref(jint value) {
  binary_dump_counter(true);
  _dump_stream.print("0x%08x", value);
  _dump_count += 4;
  set_eol_comment("reference (app)");
}

void BinaryROMWriter::binary_dump_const_int_ref(jint value) {
  binary_dump_counter(true);
  _dump_stream.print("0x%08x", value);
  _dump_count += 4;
  set_eol_comment("reference (sys)");
}

void BinaryROMWriter::binary_dump_char(char value) {
  binary_dump_counter(false);
  _dump_stream.print("      0x%02x", juint(value));
  if (isprint(value)) {
    _dump_stream.print(" '%c'", value);
  }
  _dump_count += 1;
}

void BinaryROMWriter::set_eol_comment(const char *s) {
  _eol_comment = s;
}
void BinaryROMWriter::set_eol_comment_object(Oop *value) {
  *eol_comment_object() = value->obj();
}

void BinaryROMWriter::flush_eol_comment() {
  if (_eol_comment != NULL || eol_comment_object()->not_null()) {
    if (_eol_comment == NULL) {
      _eol_comment = "reference (0x0)";
    }
    _dump_stream.print(" %s", _eol_comment);
    _eol_comment = NULL;

    if (eol_comment_object()->not_null()) {
      _dump_stream.print(" 0x%08x ", (int)(eol_comment_object()->obj()));
#ifndef PRODUCT
      eol_comment_object()->print_value_on(&_dump_stream);
#if ENABLE_ISOLATES
      if (eol_comment_object()->is_task_mirror()) {
        TaskMirror tm = eol_comment_object()->obj();
        JavaClass c = tm.containing_class();
        _dump_stream.print(" for ");
        c.print_value_on(&_dump_stream);
      }
#endif
#endif
    }
    eol_comment_object()->set_null();
  }
  _dump_stream.cr();
}
#endif // ENABLE_MONET_DEBUG_DUMP

#endif // ENABLE_ROM_GENERATOR
