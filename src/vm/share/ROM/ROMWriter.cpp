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

#if USE_SEGMENTED_TEXT_BLOCK_WRITER
//Useful for calculating relative TEXT block offsets
int ROMWriter::_pass_sizes[ROM::TEXT_BLOCK_SEGMENTS_COUNT];
int ROMWriter::_main_stream_ind_state = ROM::MAIN_SEGMENT_INDEX;

FileStreamState ROMWriter::_main_segment_stream_states[ROM::SEGMENTS_STREAMS_COUNT];
FileStreamState ROMWriter::_rom_generated_header_state;
#endif

#define ALL_SUBTYPES -1

#if USE_ROM_LOGGING
MemCounter mc_instance_class         ("InstanceClass");
MemCounter mc_inited_class           (" inited");
MemCounter mc_renamed_class          (" renamed");
MemCounter mc_static_fields          (" static fields");
MemCounter mc_vtable                 (" vtables");
MemCounter mc_itable                 (" itables");
MemCounter mc_array_class            ("ArrayClass");
MemCounter mc_class_info             ("ClassInfo");
MemCounter mc_method                 ("Method");
MemCounter mc_method_header          (" header");
MemCounter mc_method_body            (" body");
MemCounter mc_native_method          (" native");
MemCounter mc_abstract_method        (" abstract");
MemCounter mc_virtual_method         (" virtual");
MemCounter mc_compiled_method        (" compiled");
MemCounter mc_renamed_method         (" renamed");
MemCounter mc_renamed_abstract_method("  abstract");
MemCounter mc_clinit_method          (" <clinit>");
MemCounter mc_exception_table        (" except. tab");
MemCounter mc_constant_pool          ("Constant Pool");
MemCounter mc_stackmap               ("Stackmaps");
MemCounter mc_longmaps               (" longmaps");
MemCounter mc_symbol                 ("Symbol");
MemCounter mc_encoded_symbol         (" encoded");
MemCounter mc_string                 ("String");
MemCounter mc_array1                 ("Array_1");
MemCounter mc_array2s                ("Array_2(short)");
MemCounter mc_array2c                ("Array_2(char)");
MemCounter mc_array4                 ("Array_4");
MemCounter mc_array8                 ("Array_8");
MemCounter mc_obj_array              ("Object Array");
MemCounter mc_meta                   ("Meta objects");
MemCounter mc_other                  ("Other objects");
MemCounter mc_pers_handles           ("Pers. Handles");
MemCounter mc_symbol_table           ("Symbol Table");
MemCounter mc_string_table           ("String Table");
MemCounter mc_variable_parts         ("Method vars");
MemCounter mc_restricted_pkgs        ("Restricted pkg");
#if ENABLE_ISOLATES
MemCounter mc_task_mirror            (" task mirrors");
#endif
MemCounter mc_line_number_tables     ("Linenum tables");
MemCounter mc_total                  ("Total");

MemCounter* MemCounter::all_counters[41];
int MemCounter::counter_number = 0;

void MemCounter::reset_counters() {
  for (int i=0; i<counter_number; i++) {    
    all_counters[i]->reset();
  }
}

MemCounter::MemCounter(const char *n) {
  name = n;
  reset();
  if (counter_number >= ARRAY_SIZE(all_counters)) {
    BREAKPOINT;
  }
  all_counters[counter_number++] = this;  
}
#endif

// Define all static fields of ROMWriter
ROMWRITER_INT_FIELDS_DO(ROMWRITER_DEFINE_INT)
OopDesc*   ROMWriter::_romwriter_oops[ROMWriter::_number_of_oop_fields];
int        ROMWriter::_last_oop_streaming_offset = 0;
int        ROMWriter::_streaming_index = 0;
OopDesc *  ROMWriter::_streaming_oop = NULL;
TypeArray* ROMWriter::_streaming_fieldmap = NULL;

void ROMWriter::oops_do(void do_oop(OopDesc**)) {
  if (is_active()) {
    for (int i=_number_of_oop_fields-1; i>=0; i--) {
      do_oop(&_romwriter_oops[i]);
    }
    ROMOptimizer::oops_do(do_oop);
  }
}

void ROMWriter::save_file_streams() {
#if USE_ROM_LOGGING
  _summary_log_stream.save(&_summary_log_stream_state);
  _optimizer_log_stream.save(&_optimizer_log_stream_state);
#endif
}

void ROMWriter::restore_file_streams() {
#if USE_ROM_LOGGING
  _summary_log_stream.restore(&_summary_log_stream_state);
  _optimizer_log_stream.restore(&_optimizer_log_stream_state);
#endif
}

void ROMWriter::initialize() {

}

void ROMWriter::start(JVM_SINGLE_ARG_TRAPS) {
  _number_of_hashed_objects = 0;
  _symbolic_offset1 = 0;
  _symbolic_offset2 = 0;
  _symbolic_offset3 = 0;

#if ENABLE_CPU_VARIANT
  // Do not optimize bytecodes for a Jazelle-enabled VM, or else we will
  // introduce fast bytecodes that cannot be executed in hardware
  OptimizeBytecodes = false;
#endif

  _starting_ms = Os::java_time_millis();
#if ENABLE_ISOLATES
  // Flush verifier caches since there may be items in there from other
  // tasks
  Verifier::flush_cache();
#endif
  ObjectHeap::full_collect(JVM_SINGLE_ARG_CHECK);

  _method_start_skip = sizeof(OopDesc)/BytesPerWord + 4;
  _starting_free_heap = ObjectHeap::free_memory();
#if USE_ROM_LOGGING
  MemCounter::reset_counters();
#endif

  TTY_TRACE_CR(("Starting free heap = %d KB", _starting_free_heap / 1024));

  // Init streams for writing the ROM output files
  init_streams();

  // Allocate hash table used by romizer
  *info_table() = Universe::new_obj_array(INFO_TABLE_SIZE JVM_CHECK);

  // Allocate the field map used during object streaming
  alloc_field_map(DEFAULT_FIELD_MAP_SIZE JVM_CHECK);

  visited_objects()->initialize(JVM_SINGLE_ARG_CHECK);
  visited_object_infos()->initialize(JVM_SINGLE_ARG_CHECK);
  names_of_bad_classes_vector()->initialize(JVM_SINGLE_ARG_CHECK);
  _visited_all_objects_once = 0;

#if ENABLE_COMPILER
  compiled_method_list()->initialize(JVM_SINGLE_ARG_CHECK);
#endif

  _num_methods_in_image = -1;

#if ENABLE_TTY_TRACE
  TTY_TRACE(("Loading classes..."));
  jlong load_start_ms = Os::java_time_millis();
#endif

  load_all_classes(JVM_SINGLE_ARG_CHECK);

#if ENABLE_TTY_TRACE
  long diff = (long)(Os::java_time_millis() - load_start_ms);
  TTY_TRACE_CR(("Done! in %ld.%ld seconds", diff/1000, diff % 1000));
#endif

#if USE_SOURCE_IMAGE_GENERATOR
  {
    Oop null_obj;
    Task::current()->set_app_classpath(&null_obj);
  }
#endif
  _gc_stackmap_size = Universe::gc_block_stackmap()->length();
}

void ROMWriter::record_class_loading_failure(Symbol *class_name,
                                             Symbol *exception_class_name
                                             JVM_TRAPS) {
  ROMVector * const bad_classes = _singleton->names_of_bad_classes_vector();

  bad_classes->add_element(class_name JVM_CHECK);
  bad_classes->add_element(exception_class_name JVM_NO_CHECK_AT_BOTTOM);
}

// IMPL_NOTE: why do we use int array? byte array is good enough.
void ROMWriter::alloc_field_map(int size JVM_TRAPS) {
  // Allocate field map used during object streaming
  *current_fieldmap() = Universe::new_byte_array(size JVM_CHECK);  
}

void ROMWriter::fixup_image(JVM_SINGLE_ARG_TRAPS) {
  // symbol_table() and string_table() will be explicitly allocated when
  // during VM initialization.

  int i;
  int size = names_of_bad_classes_vector()->size();
  *names_of_bad_classes_array() = Universe::new_obj_array(size JVM_CHECK);
  for (i=0; i<size; i++) {
    OopDesc* obj = names_of_bad_classes_vector()->element_at(i);
    names_of_bad_classes_array()->obj_at_put(i, obj);
  }

#if ENABLE_ISOLATES
  // The mirror_list should not have any entries pointing to the
  // task_class_init_marker. We null out any of these entries.
  // They will get re-created correctly when the image is loaded
  UsingFastOops fast_oops;
  ObjArray::Fast list = Universe::mirror_list();
  for (i = 0; i < list().length(); i++) {
    TaskMirror::Raw tm = list().obj_at(i);
    GUARANTEE(!tm.is_null(), "null taskmirror in mirror list");
    if (tm().is_being_initialized_mirror()) {
      GUARANTEE(tm().array_class() == NULL, "non-null array class in mirror");
    }
  }
#endif
}

#if USE_ROM_LOGGING
void ROMWriter::write_reports(JVM_SINGLE_ARG_TRAPS) {
  if (info_table()->is_null()) {
    // We probably ran out of memory while initializing the romwriter.
    return;
  }
  _summary_log_stream.print_cr("[Input Classfiles Summary]");
  _summary_log_stream.cr();
  _summary_log_stream.print_cr("Total classfile size = %7d bytes",
                               ClassFileParser::total_classfile_bytes());
  _summary_log_stream.print_cr("           bytecodes = %7d bytes",
                               ClassFileParser::total_bytecode_bytes());
  _summary_log_stream.print_cr("           stackmaps = %7d bytes",
                               ClassFileParser::total_stackmap_bytes());
  _summary_log_stream.cr();

  // Write a summary report of how the objects are arranged inside
  // the ROM image.
  jlong elapsed_ms = Os::java_time_millis() - _starting_ms;

  _summary_log_stream.print_cr("[ROM Image Summary]");
  _summary_log_stream.cr();

  write_report(&_summary_log_stream, elapsed_ms);
  write_report(tty, elapsed_ms);

  _summary_log_stream.cr();

#if USE_SOURCE_IMAGE_GENERATOR
  _summary_log_stream.print_cr("\n[Romizer run-time flags]\n");
  Globals::print_flags(&_summary_log_stream);

  _summary_log_stream.print_cr("\n[Romizer build-time options]\n");
  Globals::print_build_options(&_summary_log_stream);
  _summary_log_stream.cr();
#endif

  write_class_summary(&_summary_log_stream);
  write_method_summary(&_summary_log_stream JVM_CHECK);
  write_compiled_method_summary(&_summary_log_stream JVM_CHECK);

  _summary_log_stream.print_cr("Ending free heap = %d KB (romizer used %d KB)",
                _ending_free_heap / 1024,
                (_starting_free_heap - _ending_free_heap) / 1024);
}
#endif

#if USE_ROM_LOGGING || USE_SOURCE_IMAGE_GENERATOR

void ROMWriter::append_file_to(OsFile_Handle dst, const JvmPathChar *src_name)
{
  char buff[512];
  int read_count;

  OsFile_Handle src = OsFile_open(src_name, "r");
  if (src) {
    while ((read_count = OsFile_read(src, buff, 1, sizeof(buff))) > 0) {
#ifdef AZZERT
      int write_count =
#endif
          OsFile_write(dst, buff, 1, read_count);
      GUARANTEE(read_count == write_count, "sanity");
    }

    OsFile_close(src);
  }
}
#endif

void ROMWriter::find_types(JVM_SINGLE_ARG_TRAPS) {
  TTY_TRACE_CR(("Categorizing objects ..."));

  BlockTypeFinder type_finder = BlockTypeFinder(JVM_SINGLE_ARG_CHECK);
  visit_all_objects(&type_finder, ALL_SUBTYPES JVM_CHECK);
  type_finder.finish();
}

void ROMWriter::visit_already_seen(RomOopVisitor *visitor, 
                                   int npass JVM_TRAPS) {
  UsingFastOops fast_oops;
  ROMizerHashEntry::Raw entry;
  Oop::Fast object;

  START_HASHTABLE_LOOP_WITH_RAW_POINTERS(entry_ptr) {
    entry_ptr->_seen = false;
  }
  END_HASHTABLE_LOOP_WITH_RAW_POINTERS;

  _visitor = visitor;
  _visiting_pass = npass;
  _visitor->set_writer(this);

  int size = visited_objects()->size();
  ObjArray::Fast raw_objects = visited_objects()->raw_array();
  ObjArray::Fast raw_infos   = visited_object_infos()->raw_array();
  for (int i=0; i<size; i++) {
    object = raw_objects().obj_at(i);
    visit_object(&object, raw_infos().obj_at(i) JVM_CHECK);
  }
}

void ROMWriter::visit_all_objects(RomOopVisitor *visitor, int npass JVM_TRAPS)
{
  if (visited_all_objects_once()) {
    visit_already_seen(visitor, npass JVM_NO_CHECK_AT_BOTTOM);
    return;
  }

  _visitor = visitor;
  _visiting_pass = npass;
  _visitor->set_writer(this);
 
  // Reset the seen flag of all the objects
  ROMizerHashEntry entry;

  START_HASHTABLE_LOOP_WITH_RAW_POINTERS(entry_ptr) {
    entry_ptr->_seen = false;
  }
  END_HASHTABLE_LOOP_WITH_RAW_POINTERS;

  UsingFastOops level1;
  Oop::Fast visiting_object;

#if !ENABLE_MONET
  int index;

  // Visit all the symbols and strings. Note that we have already set
  // Universe::symbol_table and  Universe::string_table to NULL.
  ObjArray::Fast symbol_table = _optimizer.symbol_table();
  ObjArray::Fast string_table = _optimizer.string_table();

  if (!symbol_table.is_null()) {
    visit_rom_hashtable(&symbol_table JVM_CHECK);
  }
  if (!string_table.is_null()) {
    visit_rom_hashtable(&string_table JVM_CHECK);
  }

  // we're building an image of the system libs, step through
  // the persistent handles
  // Visit all entries of system symbols[].
  for (index = 0; index < Symbols::number_of_system_symbols(); index++) {
    visiting_object = ((OopDesc*)system_symbols[index]);
    visit_object(&visiting_object, NULL JVM_CHECK);
  }
#endif 

#if USE_AOT_COMPILATION
  // AOT methods
  OopDesc *cm  = (OopDesc*)jvm_fast_globals.compiler_area_start;
  OopDesc *end = (OopDesc*)jvm_fast_globals.compiler_area_top;
  while (cm < end) {
    const size_t size = cm->object_size();
    visiting_object = cm;
    visit_object(&visiting_object, NULL JVM_CHECK);
    cm = DERIVED(OopDesc*, cm, size);
  }
#endif 
    
  visit_persistent_handles(JVM_SINGLE_ARG_CHECK);

  while (has_pending_object()) {
    visiting_object = remove_pending_object();
    visit_object(&visiting_object, NULL JVM_CHECK);
  }

  GUARANTEE(pending_links_head()->is_null(),
            "All objects should have been traversed");
  _visitor = NULL;
  _visited_all_objects_once = 1;
}

void ROMWriter::add_pending_object(Oop *object JVM_TRAPS) {
  if (write_by_reference(object)) {
    return;
  }
  UsingFastOops fast_oops;
  PendingLink::Fast link;

#ifdef AZZERT
  if (object->is_thread()) {
    BREAKPOINT;
  }
#endif
  if (pending_links_cache()->not_null()) {
    link = pending_links_cache()->obj();
    *pending_links_cache() = pending_links_cache()->next();
    link().clear_next();
  } else {
    link = Universe::new_mixed_oop(MixedOopDesc::Type_PendingLink,
                                   PendingLink::allocation_size(), 
                                   PendingLink::pointer_count()
                                   JVM_CHECK);
  }

  link().set_pending_obj(object);
  
  if (pending_links_head()->is_null()) {
    *pending_links_head() = link.obj();
    *pending_links_tail() = link.obj();
  } else {
    pending_links_tail()->set_next(&link);
    *pending_links_tail() = link.obj();
  }
}

ReturnOop ROMWriter::remove_pending_object() {
  PendingLink::Raw link = pending_links_head()->obj();
  *pending_links_head() = link().next();

  if (pending_links_head()->is_null()) {
    pending_links_tail()->set_null();
  }

  // Save this link into the links cache to reduce allocations
  link().set_next(pending_links_cache());
  *pending_links_cache() = link.obj();

  ReturnOop result = link().pending_obj();
  link().clear_pending_obj();
  return result;
}

void ROMWriter::visit_rom_hashtable(ObjArray *table JVM_TRAPS) {
  int num_buckets = table->length();
  UsingFastOops level1;
  ObjArray::Fast bucket;
  Oop::Fast object;
  for (int b=0; b<num_buckets; b++) {
    bucket = table->obj_at(b);
    int bucket_size = bucket().length();

    for (int i=0; i<bucket_size; i++) {
      object = bucket().obj_at(i);
      visit_object(&object, NULL JVM_CHECK);
    }
  }
}

#if USE_ROM_LOGGING
void ROMWriter::write_report(Stream *st, jlong elapsed) {
  if (st != tty) {
    MemCounter::print_header(st);

    mc_instance_class.print(st);
#if ENABLE_ISOLATES
    mc_task_mirror.print(st);
#endif
    mc_inited_class.print(st);
    mc_renamed_class.print(st);
    mc_static_fields.print(st);
    mc_array_class.print(st);
    mc_class_info.print(st);
    mc_vtable.print(st);
    mc_itable.print(st);
    mc_method.print(st);
    mc_method_header.print(st);
    mc_method_body.print(st);
    mc_native_method.print(st);
    mc_abstract_method.print(st);
    mc_virtual_method.print(st);
    mc_renamed_method.print(st);
    mc_renamed_abstract_method.print(st);
    mc_compiled_method.print(st);
    mc_clinit_method.print(st);
    mc_exception_table.print(st);
    mc_stackmap.print(st);
    mc_longmaps.print(st);
    mc_constant_pool.print(st);
    mc_symbol.print(st);
    mc_encoded_symbol.print(st);
    mc_string.print(st);
    mc_array1.print(st);
    mc_array2s.print(st);
    mc_array2c.print(st);
    mc_array4.print(st);
    mc_array8.print(st);
    mc_obj_array.print(st);
    mc_meta.print(st);
    mc_other.print(st);
    mc_pers_handles.print(st);
    mc_symbol_table.print(st);
    mc_string_table.print(st);
    mc_restricted_pkgs.print(st);
    mc_variable_parts.print(st);
    mc_line_number_tables.print(st);
    MemCounter::print_separator(st);
    mc_total.print(st);
  }
  MemCounter::print_separator(st);

  MemCounter::print_percent(st, "TEXT:",
                            mc_total.text_objects,
                            mc_total.text_bytes,
                            mc_total.all_bytes());
  MemCounter::print_percent(st, "DATA:",
                            mc_total.data_objects,
                            mc_total.data_bytes,
                            mc_total.all_bytes());
  MemCounter::print_percent(st, "HEAP:",
                            mc_total.heap_objects,
                            mc_total.heap_bytes,
                            mc_total.all_bytes());
  MemCounter::print_percent(st, "DATA+HEAP:",
                            mc_total.dynamic_objects(),
                            mc_total.dynamic_bytes(),
                            mc_total.all_bytes());

  st->print("Total:     %7d objects = %7d bytes", mc_total.all_objects(),
                                                  mc_total.all_bytes());
  st->cr();

  MemCounter::print_separator(st);

  int ielapsed = (int)elapsed;

  st->print_cr("ROM image generated in %.2f seconds",
               jvm_f2d(jvm_fdiv(jvm_i2f(ielapsed), 1000.0f))
               /* ((double)ielapsed / 1000.0) */);

}
#endif

// Count how many methods should be in the output image.
int ROMWriter::count_methods() {
  if (_num_methods_in_image < 0) {
    int n = 0;

    START_HASHTABLE_LOOP_WITH_RAW_POINTERS(entry_ptr) {
      OopDesc *oopdesc = entry_ptr->_referent;
      if (oopdesc && !write_by_reference(oopdesc) && oopdesc->is_method()) {
        n++;
      }
    }
    END_HASHTABLE_LOOP_WITH_RAW_POINTERS;
    _num_methods_in_image = n;
  }

  return _num_methods_in_image;
}

#if USE_ROM_LOGGING
void ROMWriter::write_class_summary(Stream *st) {
  // The information written here will be useful when we implement
  // future optimizations, such as:
  // - If only one implementation of an abstract/interface method is known
  //   to exist (for closed packages), we can change 
  //   invokevirtual/invokeinterface to invokevirtual_final.
  // - If an interface or abstract class in a closed package is never 
  //   implemented, it can be removed automatically.
  st->print_cr("[Class Hierarchy]");
  InstanceClass::Raw klass = Universe::object_class();
  print_hierarchy(st, &klass, 0);
  st->cr();

  st->print_cr("[Interface Summary]");
  InstanceClass::Raw c1;
  InstanceClass::Raw c2;
  for (SystemClassStream scs1; scs1.has_next();) {
    c1 = scs1.next();
    if (c1().is_interface()) {
      c1().print_name_on(st);
      st->cr();
      for (SystemClassStream scs2; scs2.has_next();) {
        c2 = scs2.next();
        if (!c2.equals(&c1) && c2().is_subtype_of(&c1)) {
          st->print("    ");
          c2().print_name_on(st);
          st->cr();
        }
      }
    }
  }
  st->cr();
}

void ROMWriter::print_hierarchy(Stream *st, InstanceClass *klass, int indent) {
  if (klass->is_interface()) {
    // Interfaces are printed in a different chart.
    return;
  }
  for (int i=0; i<indent; i++) {
    st->print(" ");
  }
  klass->print_name_on(st);
  st->cr();

  InstanceClass::Raw child;
  for (SystemClassStream scs; scs.has_next();) {
    child = scs.next();
    if (child().super() == klass->obj()) {
      print_hierarchy(st, &child, indent+4);
    }
  }
}

void ROMWriter::write_method_summary(Stream *st JVM_TRAPS) {
  ROMVector log_vector;
  log_vector.initialize(JVM_SINGLE_ARG_CHECK);

  st->print_cr("[Method size summary (bytes of bytecode)]");
  st->cr();

  ROMizerHashEntry entry;

  for (int bucket=0; bucket<INFO_TABLE_SIZE; bucket++) {
    for (entry = info_table()->obj_at(bucket); !entry.is_null(); ) {
      Oop obj = entry.referent();
      entry = entry.next(); // advance to next object

      if (obj.not_null() && !write_by_reference(obj) && obj.is_method()) {
        Method m = obj.obj();
        log_vector.add_element(&m JVM_CHECK);
      }
    }
  }

  log_vector.sort();
  for (int i=0; i<log_vector.size(); i++) {
    Method::Raw m = log_vector.element_at(i);
    st->print("%5d ", m().code_size());
#if USE_PRODUCT_BINARY_IMAGE_GENERATOR
    // Don't have method->print_name_on so do it "by hand"
    Symbol::Raw name = m().name();
    if (name.equals(Symbols::unknown())) {
      name = ROM::get_original_method_name(&m);
    }
    if (name.not_null()) {
      name().print_symbol_on(st);
    } else {
      st->print_cr("NULL");
    }
#else
    m().print_name_on(st);
#endif
    st->cr();
  }
  st->cr();
}

void ROMWriter::write_compiled_method_summary(Stream *st JVM_TRAPS) {
#if ENABLE_COMPILER
  ROMVector log_vector;
  log_vector.initialize(JVM_SINGLE_ARG_CHECK);

  ROMizerHashEntry entry;

  Oop obj;
  for (int bucket=0; bucket<INFO_TABLE_SIZE; bucket++) {
    for (entry = info_table()->obj_at(bucket); !entry.is_null(); ) {
      obj = entry.referent();
      entry = entry.next(); // advance to next object

      if (obj.not_null() && !write_by_reference(obj) && 
          obj.is_compiled_method()) {
        CompiledMethod cm = obj.obj();
        log_vector.add_element(&cm JVM_CHECK);
      }
    }
  }

  if (log_vector.size() <= 0) {
    return;
  }

  st->print_cr("[Compiled Method size summary (%d compiled methods)]",
               log_vector.size());
  st->cr();

  log_vector.sort();
  int total_bytecode_size = 0;
  int total_compiled_size = 0;

  for (int i=0; i<log_vector.size(); i++) {
    CompiledMethod::Raw cm = log_vector.element_at(i);
    Method::Raw m = cm().method();

    int bytecode_size = m().code_size();
    int compiled_size = cm().object_size();
    st->print("%5d ", bytecode_size);
    st->print("%5d ", compiled_size);
    total_bytecode_size += bytecode_size;
    total_compiled_size += compiled_size;

    if (bytecode_size == 0) {
      bytecode_size = 1;
    }
    st->print("%5d.%d ", compiled_size/bytecode_size,
              ((compiled_size*10)/bytecode_size)%10);

#if USE_PRODUCT_BINARY_IMAGE_GENERATOR
    // Don't have method->print_name_on so do it "by hand"
    Symbol::Raw name = m().name();
    if (name.equals(Symbols::unknown())) {
      name = ROM::get_original_method_name(&m);
    }
    if (name.not_null()) {
      name().print_symbol_on(st);
    } else {
      st->print_cr("NULL");
    }
#else
    m().print_name_on(st);
#endif
    st->cr();
  }
  st->print("%5d ", total_bytecode_size);
  st->print("%5d ", total_compiled_size);
  if (total_bytecode_size == 0) {
    total_bytecode_size = 1;
  }
  st->print_cr("%5d.%d ", total_compiled_size/total_bytecode_size,
               ((total_compiled_size*10)/total_bytecode_size)%10);
  st->cr();
#endif
}
#endif

void ROMWriter::rehash() {
  if (_singleton != NULL && is_active()) {
    _singleton->rehash_info_table();
  }
}

void ROMWriter::rehash_info_table() {
  if (info_table()->is_null()) {
    return;
  }
  UsingFastOops fast_oops;

  Oop::Fast o;
  ROMizerHashEntry::Fast p; 
  ROMizerHashEntry::Fast prev; 
  ROMizerHashEntry::Fast curr; 
  ROMizerHashEntry::Fast next;   

  int new_index, in_chain; 

  for (int i=0; i < INFO_TABLE_SIZE; i++) {
    in_chain = false;
    curr = info_table()->obj_at(i);

    while(!(curr.is_null())) {
      o = curr().referent();
      new_index = info_hashcode(&o);
      if (new_index == i) {
        prev = curr;
        curr = curr().next();
        in_chain = true;
      } else  {
        if (in_chain && prev != curr) { 
          p = curr().next();
          prev().set_next(&p);
          p = info_table()->obj_at(new_index);
          curr().set_next(&p);
          info_table()->obj_at_put(new_index, &curr);
          curr = prev().next();
        } else {
          next = curr().next();
          p = info_table()->obj_at(new_index);
          curr().set_next(&p);
          info_table()->obj_at_put(new_index, &curr);
          info_table()->obj_at_put(i, &next); 
          curr = next;
        }
      }
    }
  }
}

bool ROMWriter::is_seen(Oop* object JVM_TRAPS) {
  ROMizerHashEntry::Raw entry = info_for(object JVM_CHECK_0);
  return entry().seen();
}

void ROMWriter::set_seen(Oop* object JVM_TRAPS) {
  ROMizerHashEntry::Raw entry = info_for(object JVM_CHECK);
  entry().set_seen(true);
}

ROMWriter::BlockType ROMWriter::block_type_of(Oop* object JVM_TRAPS) {
  ROMizerHashEntry::Raw entry = info_for(object JVM_CHECK_(UNKNOWN_BLOCK));
  return (ROMWriter::BlockType)entry().type();
}

void ROMWriter::set_block_type_of(Oop* object, BlockType type JVM_TRAPS) {
  ROMizerHashEntry::Raw entry = info_for(object JVM_CHECK);
  entry().set_type(type);
}

int ROMWriter::pass_of(Oop* object JVM_TRAPS) {
  ROMizerHashEntry::Raw entry = info_for(object JVM_CHECK_(UNKNOWN_BLOCK));
  return entry().pass();
}

void ROMWriter::set_pass_of(Oop* object, int pass JVM_TRAPS) {
  ROMizerHashEntry::Raw entry = info_for(object JVM_CHECK);
  entry().set_pass(pass); 
}

int ROMWriter::skip_words_of(Oop* object JVM_TRAPS) {
  ROMizerHashEntry::Raw entry = info_for(object JVM_CHECK_(UNKNOWN_BLOCK));
  return entry().skip_words();
}

void ROMWriter::set_skip_words_of(Oop* object, int skip JVM_TRAPS) {
  ROMizerHashEntry::Raw entry = info_for(object JVM_CHECK);
  entry().set_skip_words(skip);
}

int ROMWriter::offset_of(Oop* object JVM_TRAPS) {
  if (write_by_reference(object)) {
    return (int)(object->obj());
  }
  ROMizerHashEntry::Raw entry = info_for(object JVM_CHECK_(-1));
  return entry().offset();
}

void ROMWriter::set_offset_of(Oop* object, int offset JVM_TRAPS) {
  ROMizerHashEntry::Raw entry = info_for(object JVM_CHECK);
  entry().set_offset(offset);
}

#if ENABLE_HEAP_NEARS_IN_HEAP && USE_SOURCE_IMAGE_GENERATOR
int ROMWriter::heap_offset_of(Oop* object JVM_TRAPS) {
  ROMizerHashEntry::Raw entry = info_for(object JVM_CHECK_0);
  return entry().heap_offset();
}

void ROMWriter::set_heap_offset_of(Oop* object, int offset JVM_TRAPS) {
  ROMizerHashEntry::Raw entry = info_for(object JVM_CHECK);
  entry().set_heap_offset(offset);
}
#endif

#if USE_SEGMENTED_TEXT_BLOCK_WRITER

int ROMWriter::loc_offset_of(Oop* object JVM_TRAPS) {
  ROMizerHashEntry::Raw entry = info_for(object JVM_CHECK_(UNKNOWN_BLOCK));
  return entry().loc_offset();
}

void ROMWriter::set_loc_offset_of(Oop* object, int loc_offset JVM_TRAPS) {
  ROMizerHashEntry::Raw entry = info_for(object JVM_CHECK);
  entry().set_loc_offset(loc_offset);
}

#endif

void ROMWriter::visit_object(Oop *object, OopDesc *info JVM_TRAPS) {
  if (object->is_null()) {
    return;
  }

  ROMizerHashEntry entry;
  if (info != NULL) {
    entry = info;
  } else { 
    entry = info_for(object JVM_CHECK);
  }
  if (entry.seen()) {
    return;
  }
  entry.set_seen(true);

  if (visited_all_objects_once() == 0) {
    GUARANTEE(info == NULL, "sanity");
    GUARANTEE(visited_objects()->size() ==  visited_object_infos()->size(), 
              "must be of same length");
    visited_objects()->add_element(object JVM_CHECK);
    visited_object_infos()->add_element(&entry JVM_CHECK);
  }

  // Cache the value of is_current_subtype()
  *visiting_object() = object->obj();
  if (visiting_pass() == ALL_SUBTYPES) {
    _visiting_object_is_current_subtype = true;
  } else {
    _visiting_object_is_current_subtype = (entry.pass() == visiting_pass());
  }

#if USE_BINARY_IMAGE_GENERATOR
 {
   BlockType type = (BlockType)entry.type();
   if ((type == ROMWriter::HEAP_BLOCK) &&
       object->obj() <= ROM::romized_heap_marker()) {
#if ENABLE_ISOLATES && !ENABLE_LIB_IMAGES
      GUARANTEE(ObjectHeap::owner_task_id(object->obj()) != romizer_task_id(),
                "Missed heap object");
      GUARANTEE(ObjectHeap::owner_task_id(object->obj()) == SYSTEM_TASK,
                "Non-system, non-convert task heap object");
#endif
     return;
   }
#if ENABLE_ISOLATES
   GUARANTEE(type != ROMWriter::HEAP_BLOCK ||
             ObjectHeap::owner_task_id(object->obj()) == romizer_task_id(),
             "Streaming heap object from different task");
#endif
 }
#endif


  // prologue
  if (visiting_object_is_current_subtype()) {
    visitor()->begin_object(object JVM_CHECK);
  }

#if USE_BINARY_IMAGE_GENERATOR
  if (write_by_reference(object)) {
    // All the remaining can be skipped to speed up binary romization.
    return;
  }
#endif
  stream_object(object JVM_CHECK);

  // epilogue
  if (visiting_object_is_current_subtype()) {
    visitor()->end_object(object JVM_CHECK);
  }

  _symbolic_offset1 = 0;
  _symbolic_offset2 = 0;
  _symbolic_offset3 = 0;
}


void ROMWriter::stream_object(Oop* object JVM_TRAPS) {  
  FarClass blueprint = object->blueprint();
  InstanceSize instance_size = blueprint.instance_size();

  // Save object to special representation in ROM that can be read by ROMReader
  switch (instance_size.value()) {
    default:
    { // object is an instance.
      GUARANTEE(instance_size.value() > 0, "bad instance size");
      UsingFastOops level2;
      Instance::Fast it = object;
      InstanceClass::Fast klass = it().blueprint();

      // stream the klass pointer
      stream_field_by_type(object, 0, T_OBJECT, 1 JVM_CHECK);
#if ENABLE_OOP_TAG
      // task id and seq num.
      if (sizeof(OopDesc) > BytesPerWord) {
        for (int i = 4; i < sizeof(OopDesc); i+=4) {
          // IMPL_NOTE: T_INT only!
          stream_field_by_type(object, i, T_INT, 1 JVM_CHECK);
        }
      }
#endif
      // stream the instance fields
      stream_instance_fields(&klass, &it, Instance::header_size() JVM_CHECK);
    }
    break;

    case InstanceSize::size_compiled_method:
      // SHOULD_NOT_REACH_HERE();

    case InstanceSize::size_type_array_1:
    case InstanceSize::size_type_array_2:
    case InstanceSize::size_type_array_4:
    case InstanceSize::size_type_array_8:
    case InstanceSize::size_obj_array:
    {
      gen_and_stream_fieldmap(object JVM_CHECK);
    }
    break;

    case InstanceSize::size_obj_array_class:
    case InstanceSize::size_type_array_class:
    {
      JavaClass it = object;
      it.clear_subtype_caches();
      gen_and_stream_fieldmap(object JVM_CHECK);
    }
    break;

    case InstanceSize::size_far_class:
    {
      FarClass it = object;
      gen_and_stream_fieldmap(object JVM_CHECK);
    }
    break;

    case InstanceSize::size_instance_class:
      {
      InstanceClass ic = object;
      ic.clear_subtype_caches();
      gen_and_stream_fieldmap(object JVM_CHECK);

#if ENABLE_ISOLATES
      stream_static_fields_oopmap(&ic JVM_CHECK);
#else
      // stream the static fields for the Java class as well as
      // the associated instance and stack fields oopmap
      stream_static_fields(&ic, &ic, ic.static_field_start() JVM_CHECK);
#endif
    }
    break;

    case InstanceSize::size_method:
      if (((Method*)object)->has_compiled_code() && 
          !visited_all_objects_once()) {
        CompiledMethod cm = ((Method*)object)->compiled_code();
        bool seen = is_seen(&cm JVM_CHECK);
        if (!seen) { 
          add_pending_object(&cm JVM_CHECK);
        }
      }
#if USE_SOURCE_IMAGE_GENERATOR
      stream_method((Method*)object JVM_CHECK);
#else
      gen_and_stream_fieldmap(object JVM_CHECK);
#endif
      break;    

      // All objects of the types below are streamed
      // solely based on the field maps that they generate
    case InstanceSize::size_constant_pool:
    case InstanceSize::size_stackmap_list:
    case InstanceSize::size_class_info:
    case InstanceSize::size_symbol:
    case InstanceSize::size_generic_near:
    case InstanceSize::size_java_near:
    case InstanceSize::size_obj_near:
    case InstanceSize::size_mixed_oop:
      gen_and_stream_fieldmap(object JVM_CHECK);
      break;    

#if ENABLE_ISOLATES
    case InstanceSize::size_task_mirror:
      { 
        TaskMirror tm = object;
        gen_and_stream_fieldmap(object JVM_CHECK);
        JavaClassObj::Raw m = tm.real_java_mirror();
        // task_class_init_marker has null real_java_mirror
        if (m.not_null()) {
          JavaClass::Raw jc = m().java_class();
          // stream out the static fields
          if (jc().is_instance_class()) {
            InstanceClass::Raw ic = jc.obj();
            stream_static_fields(&ic, &tm, tm.static_field_start() JVM_CHECK);
          }
        }
      }
      break;
#endif

  } /* switch (instance_size.value()) */
}

#if USE_SOURCE_IMAGE_GENERATOR
// This optimization embeds short StackmapList infomation directly at
// ((int)MethodDesc::_stackmaps). It's disabled in Monet since the pay off
// is small unless you have thousands of methods, such as a MIDP system ROM.
void ROMWriter::stream_method(Method* method JVM_TRAPS) {
  GUARANTEE(Method::constants_offset()      == 1*sizeof(OopDesc),"assumption");
  GUARANTEE(Method::exception_table_offset()== 2*sizeof(OopDesc),"assumption");
  GUARANTEE(Method::stackmaps_offset()      == 3*sizeof(OopDesc),"assumption");

  int skip_words = skip_words_of(method JVM_CHECK);

  if (skip_words <= 0) { // near
    put_oop_field(method, 0*sizeof(OopDesc) JVM_CHECK);
  }
  if (skip_words <= 1) { // constants
    put_oop_field(method, 1*sizeof(OopDesc) JVM_CHECK);
  }
  if (skip_words <= 2) { // exception_table
    put_oop_field(method, 2*sizeof(OopDesc) JVM_CHECK);
  }
  if (skip_words <= 3) { // stackmaps
    StackmapList::Raw sm = method->stackmaps();
    bool streamed = false;

    if (is_current_subtype(method) && 
        sm.not_null() && sm().entry_count() == 1 && sm().is_short_map(0)) {
      juint n = sm().uint_field(sm().entry_stat_offset(0));
      if (n < 0x80000000) {
        n = (juint)((n << 1) | 0x01); // to be decoded in Method::stackmaps().
        visitor()->put_int(method, (int)n JVM_CHECK);
        streamed = true;
      }
    }
    if (!streamed) {
      put_oop_field(method, 3*sizeof(OopDesc) JVM_CHECK);
    }
  }

  gen_and_stream_fieldmap(method, 4 JVM_CHECK);
}
#endif

#if ENABLE_ISOLATES
void ROMWriter::stream_static_fields_oopmap(InstanceClass* ic JVM_TRAPS) {

  // Put the instance oop map, static oopmap and vtable bitmap;
  // they are just a series of bytes.
  int start_offset = ic->header_size();
  int limit = ic->object_size();
  while (start_offset < limit) {
    put_int_field(ic, start_offset, BYTE_BYTE_BYTE_BYTE JVM_CHECK);
    start_offset += sizeof(jint);
  }
}
#endif

// Generate a FieldMap (for the static or non-static fields) of 
// the given Java class
int ROMWriter::generate_java_fieldmap(InstanceClass* klass, bool is_static
                                      JVM_TRAPS)
{
  int max_fields = get_java_fieldmap_size(klass, is_static);

  if (max_fields > current_fieldmap()->length()) {
    alloc_field_map(max_fields JVM_CHECK_0);
  }

  if (is_static) {
    return generate_static_java_fieldmap(klass);
  } else {
    return generate_instance_java_fieldmap(klass);
  }
}

int ROMWriter::get_java_fieldmap_size(InstanceClass* klass, bool is_static) {
  int max_fields = 0;

  // (1) Find the total number of fields
  if (is_static) {
    TypeArray::Raw fields = klass->original_fields();

    for (int i = 0; i < fields().length(); i += 5) {
      OriginalField f(klass, i);
      if (f.is_static()) {
        max_fields ++;
      }
    }
  } else {
    // In the worst case, every instance field is 8-bit long, so we will
    // have at most this many instance fields.
    max_fields = klass->instance_size().fixed_value() + 3;
  }

  return max_fields;
}

// Returns the number of fields stored in the fieldmap
int ROMWriter::generate_static_java_fieldmap(InstanceClass* klass) {
#ifdef AZZERT
  int current_offset = -1;
#endif
  int map_index = 0;
  TypeArray::Raw fields = klass->original_fields();

  // Each static field is word-aligned and occupies at least 4 bytes.
  // 8- and 16-bit fields are promoted to 32-bit values. E.g., the jchar
  // 0xabcd is stored as 0x0000abcd, and the jbyte -1 is stored
  // as 0xffffffff

  for (int i = 0; i < fields().length(); i += 5) {
    OriginalField f(klass, i);
    if (f.is_static()) {
      GUARANTEE((f.offset() > current_offset), 
                "static fields offsets must be ascending");
      BasicType type = f.type();

      if (byte_size_for(type) < 4) {
        type = T_INT;
      }
      current_fieldmap()->byte_at_put(map_index++, type);

#ifdef AZZERT
      current_offset = f.offset();
#endif
    }
  }

  return map_index;
}

int ROMWriter::generate_instance_java_fieldmap(InstanceClass* klass) {
  int map_index = 0;
  size_t instance_size = sizeof(OopDesc); // the first X bytes are for the object header
  int field_count = generate_instance_java_fieldmap(klass, map_index, 
                                                    instance_size);

  //tty->print_cr("%d\n", klass->instance_size().fixed_value());
  GUARANTEE(instance_size == klass->instance_size().fixed_value(), "sanity");

  return field_count;
}

int ROMWriter::generate_instance_java_fieldmap(InstanceClass* klass, 
                                               int start_index, 
                                               size_t &instance_size) {
  int map_index = start_index;
  InstanceClass::Raw super_klass = klass->super();
  if (super_klass.not_null()) {
    map_index = generate_instance_java_fieldmap(&super_klass, map_index, 
                                                instance_size); 
  }

  // Instance fields are packed. Pad bytes are added such that
  //   - 16-bit values are 2-byte aligned
  //   - 32- and 64-bit values are 4-byte aligned.
  // Also, the first field of a sub-class always starts at a 4-byte aligned
  // address, regardless of the amount of left over space from its super
  // class. E.g., if you have:
  //     class Super { char s1; byte b1; }
  //     class Sub   { byte b2; }
  // The offsets of the fields are:
  //     s1         = 0
  //     b1         = 2
  //     <pad byte> = 3
  //     b2         = 4
  TypeArray::Raw fields = klass->original_fields();
  int i;

  int num_instance_fields = 0;
  for (i = 0; i < fields().length(); i += 5) {
    OriginalField f(klass, i);
    if (!f.is_static()) {
      num_instance_fields ++;
    }
  }

  int num_instance_fields_done = 0;
  while (num_instance_fields_done < num_instance_fields) {
    int found = -1;
    int min = 0x7fffffff;
    for (i = 0; i < fields().length(); i += 5) {
      // The fields array is not sorted by address, so we need to find
      // the address with the lowest field that has not been processed
      // yet.
      int offset = fields().ushort_at(i + Field::OFFSET_OFFSET);
      int flags  = fields().ushort_at(i + Field::ACCESS_FLAGS_OFFSET);
      if (!(flags & JVM_ACC_STATIC)) {
        if (offset >= (jint)instance_size && offset < min) {
          found = i;
          min = offset;
        }
      }
    }

    GUARANTEE(found >= 0, "sanity");
    OriginalField f(klass, found);

    // Put dummy fieldmap entries for the pad bytes
    int pad = (f.offset() - instance_size);
    GUARANTEE(pad >= 0, "instance field offset must be ascending");
    GUARANTEE(pad <= 3,  "padding must be at most 3 bytes");
    for (int p=0; p<pad; p++) {
      current_fieldmap()->byte_at_put(map_index++, T_BYTE); // padding
      instance_size ++;
    }

    // Put the fieldmap entry for the current field
    BasicType type = f.type();
    current_fieldmap()->byte_at_put(map_index++, type);
    instance_size += byte_size_for(type);
    num_instance_fields_done ++;
  }

  if ((instance_size % 4) != 0) {
    int pad = 4 - (instance_size % 4);
    for (int p=0; p<pad; p++) {
      current_fieldmap()->byte_at_put(map_index++, T_BYTE); // padding
      instance_size ++;
    }
  }

  return map_index;
}

// Write the static fields that resides in an InstanceClass.
int ROMWriter::stream_static_fields(InstanceClass* klass,
                                    Oop* object, jint start_offset JVM_TRAPS) {
  jint field_count = generate_java_fieldmap(klass, true JVM_CHECK_0);
  int current_pos = stream_fields_by_map(object, start_offset, 0, field_count
                                         JVM_CHECK_0);

  // Static fields are followed by:
  //  - for SVM: instance oopmap, static oopmap and vtable bitmap 
  //  - for MVM: vtable bitmap 
  // -- they are just a bunch of bytes.
  int limit = object->object_size();
  while (current_pos < limit) {
    put_int_field(object, current_pos, BYTE_BYTE_BYTE_BYTE JVM_CHECK_0);
    current_pos += sizeof(jint);
  }
  return current_pos;
}

#if !USE_BINARY_IMAGE_GENERATOR
// Write the *non-static* fields in an Instance.
int ROMWriter::stream_instance_fields(InstanceClass* klass,
                                      Oop* object,
                                      jint start_offset
                                      JVM_TRAPS) {   
  jint field_count = generate_java_fieldmap(klass, false JVM_CHECK_0);
  return stream_fields_by_map(object,start_offset, 0, field_count JVM_CHECK_0);
}
#else
// IMPL_NOTE: this function can be used to stream all objects, since
// we don't have to worry about small fields in Monet (there's no
// cross-endianness generation).
int ROMWriter::stream_instance_fields(InstanceClass* /*klass*/,
                                      Oop* object,
                                      jint start_offset
                                      JVM_TRAPS) {
  int field_count = object->object_size() / sizeof(OopDesc*);
  if (field_count > current_fieldmap()->length()) {
    alloc_field_map(field_count JVM_CHECK_0);
  }
  _streaming_fieldmap = current_fieldmap();
  _streaming_oop = object->obj();
  _streaming_index = 0;

  object->obj()->oops_do(generate_fieldmap_by_oops_do);

  // Copy any remaining int fields at the end of the object
  int size = (int)object->object_size();
  for (int i = _last_oop_streaming_offset + sizeof(OopDesc*); i < size;
       i += sizeof(OopDesc*)) {
    current_fieldmap()->byte_at_put(_streaming_index++, T_INT);
  }
  return stream_fields_by_map(object, start_offset, 0, _streaming_index
                              JVM_CHECK_0);
}
#endif

void ROMWriter::generate_fieldmap_by_oops_do(OopDesc**p) {
  int offset = (int)p - (int)_streaming_oop;
  for (int i = _last_oop_streaming_offset + sizeof(OopDesc*); i < offset;
       i += sizeof(OopDesc*)) {
    _streaming_fieldmap->byte_at_put(_streaming_index++, T_INT);
  }

  _streaming_fieldmap->byte_at_put(_streaming_index++, T_OBJECT);
  _last_oop_streaming_offset = offset;
}

int ROMWriter::gen_and_stream_fieldmap(Oop* object, int force_skip_words
                                       JVM_TRAPS) {
  int field_count = 0;
  
  // Have the object generate's its field map
  field_count = object->generate_fieldmap(current_fieldmap());

  // in case the field map space was insufficient, realloc
  // and retry field map generation
  if (field_count > current_fieldmap()->length()) {
    alloc_field_map(field_count JVM_CHECK_0);
    field_count = object->generate_fieldmap(current_fieldmap());
  }

  GUARANTEE(field_count <= current_fieldmap()->length(), 
            "PANIC: Out of field map space! Please increase VM heap size");

  // determine the number of bytes that needs to be skipped
  int skip_words = skip_words_of(object JVM_CHECK_0);
  if (skip_words < force_skip_words) {
      skip_words = force_skip_words;
  }
  int skip_bytes = skip_words * sizeof(jint);

  // Determine the offset in the field map from which we can
  // start streaming. I.e., find the type of the first field in the object
  // that we have to start streaming from
  int start_offset = get_next_field_map_item_index(0, skip_bytes, field_count);

  // Stream the fields
  return stream_fields_by_map(object, skip_bytes, start_offset, 
                              field_count JVM_CHECK_0);
}

int ROMWriter::get_next_field_map_item_index(int index,
                                             int skip_bytes,
                                             int field_count) {
  // Sanity check
  GUARANTEE(skip_bytes >= 0 && !current_fieldmap()->is_null() && index >= 0, 
            "Invalid get_next_field_map_item_index  parameter");

  if (skip_bytes <= 0) {
    return index;
  }

  int bytes_touched = 0;
  
  // The fields of type boolean, byte, char & short
  // forces streaming of a word of data to adhere to
  // ROMizer requirements.
  while (bytes_touched < skip_bytes && index < field_count) {
    switch (current_fieldmap()->byte_at(index++)) {
      case T_BOOLEAN:
      case T_BYTE:
        bytes_touched++;
        break;
      case T_CHAR:
      case T_SHORT:
        bytes_touched += 2;
        break;
      case T_LONG:
      case T_DOUBLE:
        bytes_touched += 8;
        break;
      case T_INT:
      case T_FLOAT:
      case T_OBJECT:
      case T_ARRAY:
      case T_SYMBOLIC:
        bytes_touched += 4;
        break;
      default:
        SHOULD_NOT_REACH_HERE();
    }
  }
  // Make sure we are aligned correctly
  GUARANTEE(bytes_touched == skip_bytes || index >= field_count,
            "ROMWriter: Mismatch between field map and skip bytes");
  return index;
}

// stream the fields of an objects by its field map
int ROMWriter::stream_fields_by_map(Oop* object,
                                    int obj_pos,
                                    int start_field,
                                    int field_count JVM_TRAPS) {
  int current_pos = obj_pos;

  for (int i = start_field; i < field_count;) {
    GUARANTEE((current_pos % 4) == 0, "must be 32-bit word aligned");
    BasicType type = (BasicType)current_fieldmap()->byte_at(i);

    if (byte_size_for(type) < 4) {
      i = stream_small_fields(object, current_pos, i, field_count JVM_CHECK_0);
      current_pos += 4;
    } else {
      current_pos = stream_field_by_type(object, current_pos, 
                                         type, 1 JVM_CHECK_0);
      i++;
    }
  }

  return current_pos;
}

// Stream 32-bits of fields in the object.
int ROMWriter::stream_small_fields(Oop * object, int obj_pos, 
                                   int field_index, int field_count JVM_TRAPS) {
  GUARANTEE((obj_pos % 4) == 0, "must be 32-bit word aligned");
  int num_bytes = 0;
  int typemask = 0;

  while (num_bytes < 4 && field_index < field_count) {
    BasicType type = (BasicType)current_fieldmap()->byte_at(field_index);

    if ((byte_size_for(type) + num_bytes) > 4) {
      // The next field is already outside of the current 4-byte word.
      break;
    }

    switch (byte_size_for(type)) {
    case 1:
      num_bytes += 1;
      typemask <<= 1;
      typemask |= 1;
      break;
    case 2:
      if (num_bytes == 1) {
        // 1 byte of padding
        num_bytes += 1;
        typemask <<= 1;
        typemask |= 1;
      }
      num_bytes += 2;
      typemask <<= 2;
      break;
    default:
      SHOULD_NOT_REACH_HERE();
    }
    field_index ++;
  }

  // Pad the remaining bytes, if necessary.
  while (num_bytes < 4) {
    num_bytes ++;
    typemask <<= 1;
    typemask |= 1;
  }

  put_int_field(object, obj_pos, typemask JVM_CHECK_0);

  return field_index;
}

// stream a single type of the object 
int ROMWriter::stream_field_by_type(Oop* object,
                                    int obj_pos,
                                    int field_type,
                                    int field_count JVM_TRAPS) {
  // NOTE: This implementation works on the assumption
  // that fields of a unique type is streamed
  // might stream more than 1 field of the given type
  // in a single iteration
  for (int i = 0; i < field_count;) {
    switch (field_type){
      case T_BOOLEAN:
      case T_BYTE:
      case T_CHAR:
      case T_SHORT:
        SHOULD_NOT_REACH_HERE()
        break;

      case T_FLOAT:
      case T_INT:
        // Plain raw integer data
        put_int_field(object, obj_pos JVM_CHECK_0);
        obj_pos += sizeof(jint);
        i++;
        break;

      case T_LONG:
        put_long_field(object, obj_pos JVM_CHECK_0);
        obj_pos += 2 * sizeof(jint);
        i++;
        break;

      case T_DOUBLE:
        put_double_field(object, obj_pos JVM_CHECK_0);
        obj_pos += 2 * sizeof(jint);
        i++;
        break;

      case T_OBJECT:
      case T_ARRAY:
        put_oop_field(object, obj_pos JVM_CHECK_0);
        obj_pos += sizeof(jint);
        i++;
        break;

      case T_SYMBOLIC:
        put_symbolic_field(object, obj_pos JVM_CHECK_0);
        obj_pos += sizeof(jint);
        i++;
        break;

      default:
        SHOULD_NOT_REACH_HERE();
      }
  }  
  return obj_pos;
}

void ROMWriter::put_int_field(Oop* object, int offset JVM_TRAPS) {
  if (!is_current_subtype(object) || write_by_reference(object)) {
    return;
  }

  jint value = object->int_field(offset);
  visitor()->put_int(object, value JVM_CHECK);
}

void ROMWriter::put_int_field(Oop* object, int offset,
                              int typemask JVM_TRAPS) {
  if (!is_current_subtype(object) || write_by_reference(object)) {
    return;
  }

  jint value = object->int_field(offset);

  union {
    jint word;
    jbyte bytes[4];
  } be, le;

  if (HARDWARE_LITTLE_ENDIAN) {
    // Host   = little-endian
    // Target = big-endian
    le.word = value;
    jbyte byte_at_host_addr0 = le.bytes[0];
    jbyte byte_at_host_addr1 = le.bytes[1];
    jbyte byte_at_host_addr2 = le.bytes[2];
    jbyte byte_at_host_addr3 = le.bytes[3];

    jbyte byte_at_target_addr0;
    jbyte byte_at_target_addr1;
    jbyte byte_at_target_addr2;
    jbyte byte_at_target_addr3;

    switch (typemask) {
    case BYTE_BYTE_BYTE_BYTE:
      byte_at_target_addr0 = byte_at_host_addr0;
      byte_at_target_addr1 = byte_at_host_addr1;
      byte_at_target_addr2 = byte_at_host_addr2;
      byte_at_target_addr3 = byte_at_host_addr3;
      break;
    case SHORT_BYTE_BYTE:
      byte_at_target_addr0 = byte_at_host_addr1;
      byte_at_target_addr1 = byte_at_host_addr0;
      byte_at_target_addr2 = byte_at_host_addr2;
      byte_at_target_addr3 = byte_at_host_addr3;
      break;
    case BYTE_BYTE_SHORT:
      byte_at_target_addr0 = byte_at_host_addr0;
      byte_at_target_addr1 = byte_at_host_addr1;
      byte_at_target_addr2 = byte_at_host_addr3;
      byte_at_target_addr3 = byte_at_host_addr2;
      break;
    case SHORT_SHORT:
      byte_at_target_addr0 = byte_at_host_addr1;
      byte_at_target_addr1 = byte_at_host_addr0;
      byte_at_target_addr2 = byte_at_host_addr3;
      byte_at_target_addr3 = byte_at_host_addr2;
      break;
    default:
      SHOULD_NOT_REACH_HERE();
      return;
    }

    // convert this to a value to be printed by little-endian host.
    be.bytes[0] = byte_at_target_addr3;
    be.bytes[1] = byte_at_target_addr2;
    be.bytes[2] = byte_at_target_addr1;
    be.bytes[3] = byte_at_target_addr0;
  } else {
    // Host   = big-endian
    // Target = little-endian
    be.word = value;
    jbyte byte_at_host_addr0 = be.bytes[0];
    jbyte byte_at_host_addr1 = be.bytes[1];
    jbyte byte_at_host_addr2 = be.bytes[2];
    jbyte byte_at_host_addr3 = be.bytes[3];

    jbyte byte_at_target_addr0;
    jbyte byte_at_target_addr1;
    jbyte byte_at_target_addr2;
    jbyte byte_at_target_addr3;

    switch (typemask) {
    case BYTE_BYTE_BYTE_BYTE:
      byte_at_target_addr0 = byte_at_host_addr0;
      byte_at_target_addr1 = byte_at_host_addr1;
      byte_at_target_addr2 = byte_at_host_addr2;
      byte_at_target_addr3 = byte_at_host_addr3;
      break;
    case SHORT_BYTE_BYTE:
      byte_at_target_addr0 = byte_at_host_addr1;
      byte_at_target_addr1 = byte_at_host_addr0;
      byte_at_target_addr2 = byte_at_host_addr2;
      byte_at_target_addr3 = byte_at_host_addr3;
      break;
    case BYTE_BYTE_SHORT:
      byte_at_target_addr0 = byte_at_host_addr0;
      byte_at_target_addr1 = byte_at_host_addr1;
      byte_at_target_addr2 = byte_at_host_addr3;
      byte_at_target_addr3 = byte_at_host_addr2;
      break;
    case SHORT_SHORT:
      byte_at_target_addr0 = byte_at_host_addr1;
      byte_at_target_addr1 = byte_at_host_addr0;
      byte_at_target_addr2 = byte_at_host_addr3;
      byte_at_target_addr3 = byte_at_host_addr2;
      break;
    default:
      SHOULD_NOT_REACH_HERE();
      return;
    }

    // convert this to a value to be printed by big-endian host.
    le.bytes[0] = byte_at_target_addr3;
    le.bytes[1] = byte_at_target_addr2;
    le.bytes[2] = byte_at_target_addr1;
    le.bytes[3] = byte_at_target_addr0;
  }

  visitor()->put_int_by_mask(object, be.word, le.word, typemask JVM_CHECK);
}

void ROMWriter::put_symbolic_field(Oop* object, int offset JVM_TRAPS) {
  if (!is_current_subtype(object) || write_by_reference(object)) {
    return;
  }
  visitor()->put_symbolic(object, offset JVM_CHECK);
}

void ROMWriter::put_int_block(Oop* object, int offset, int size JVM_TRAPS) {
  if (!is_current_subtype(object) || write_by_reference(object)) {
    return;
  }

  // Plain raw data
  for (int current= offset; current < offset + size; current += sizeof(jint)) {
    put_int_field(object, current JVM_CHECK);
  }
}

void ROMWriter::put_double_field(Oop* object, int offset JVM_TRAPS) {
  if (!is_current_subtype(object) || write_by_reference(object)) {
    return;
  }

  jint msw, lsw;
  if (MSW_FIRST_FOR_DOUBLE) {
    msw  = object->int_field(offset);
    lsw = object->int_field(offset + 4);
  } else {
    lsw = object->int_field(offset);
    msw  = object->int_field(offset + 4);
  }

  visitor()->put_double(object, msw, lsw JVM_CHECK);
}

void ROMWriter::put_long_field(Oop* object, int offset JVM_TRAPS) {
  if (!is_current_subtype(object) || write_by_reference(object)) {
    return;
  }

  jint msw, lsw;
  if (MSW_FIRST_FOR_LONG) {
    msw  = object->int_field(offset);
    lsw = object->int_field(offset + 4);
  } else {
    lsw = object->int_field(offset);
    msw  = object->int_field(offset + 4);
  }

  visitor()->put_long(object, msw, lsw JVM_CHECK);
}

void ROMWriter::put_oop_field(Oop* object, int offset JVM_TRAPS) {
  Oop value = object->obj_field(offset);

  if (is_current_subtype(object)) {
    visitor()->put_reference(object, offset, &value JVM_CHECK);
  }

  if (ROM::system_text_contains(value.obj())) {
    return;
  }

  if (!visited_all_objects_once()) {
    // We're still building the transitive closure, so add the object if
    // it's not yet seen.
    bool seen = is_seen(&value JVM_CHECK);
    if (!seen && !value.is_null()) {
      add_pending_object(&value JVM_CHECK);

      // For tags arrays associated with constant pools we skip the
      // tags for all starting Symbols. If a tags array with many
      // starting Symbols is appended immediatelly after an object of
      // a small size, it could happen that the offset of the small
      // object will be greater than that of the tags array.  This
      // should never happen - see comments for the assertion for
      // "illegal overlapping objects in TEXT".  So we explicitly
      // append the tags array after its associated constant pool. The
      // number of Symbols in the tags array cannot be greater than
      // the size of associated constant pool, so the assertion will
      // succeed.
      if (value.is_constant_pool() && is_current_subtype(&value)) {
        UsingFastOops fast_oops;
        ConstantPool::Fast cp = value.obj();
        TypeArray::Fast tags = cp().tags();
        if (tags.not_null()) {
          bool tags_seen = is_seen(&tags JVM_CHECK);
          if (!tags_seen) {
            add_pending_object(&tags JVM_CHECK);
          }
        }
      }
    }                                  
  }
}

bool ROMWriter::is_current_subtype(Oop* object) {
  if (visiting_object()->equals(object)) {
    return _visiting_object_is_current_subtype;
  }

  if (visiting_pass() == ALL_SUBTYPES) {
    return true;
  }
  // No need to JVM_CHECK -- we have passed the BlockTypeFinder stage, so
  // the object already has an info. No allocation will happen here
  SETUP_ERROR_CHECKER_ARG;
  int pass = pass_of(object JVM_MUST_SUCCEED);
#if ENABLE_HEAP_NEARS_IN_HEAP && USE_SOURCE_IMAGE_GENERATOR
  BlockType tp = block_type_of(object JVM_MUST_SUCCEED);
  if (tp == TEXT_AND_HEAP_BLOCK || tp == DATA_AND_HEAP_BLOCK) 
    return 0 == visiting_pass();
#endif
  
  return pass == visiting_pass();
}

void ROMWriter::write_symbol_table(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops level1;
  ObjArray::Fast table               = _optimizer.symbol_table();
  ConstantPool::Fast embedded_holder = _optimizer.embedded_table_holder();
  int embedded_offset          = _optimizer.embedded_symbols_offset();

  int num_bytes = write_rom_hashtable("symbol_table", "SYMB", &table,
                                      &embedded_holder, embedded_offset 
                                      JVM_CHECK);

#if USE_ROM_LOGGING
  mc_symbol_table.add_text_bytes(num_bytes);
  mc_total.add_text_bytes       (num_bytes);
#else
  (void)num_bytes;
#endif
}

void ROMWriter::write_string_table(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops level1;
  ObjArray::Fast table               = _optimizer.string_table();
  ConstantPool::Fast embedded_holder = _optimizer.embedded_table_holder();
  int embedded_offset          = _optimizer.embedded_strings_offset();

  int num_bytes = write_rom_hashtable("string_table", "STRG", &table,
                                      &embedded_holder, embedded_offset
                                      JVM_CHECK);
#if USE_ROM_LOGGING
  mc_string_table.add_text_bytes(num_bytes);
  mc_total.add_text_bytes       (num_bytes);
#else
  (void)num_bytes;
#endif
}


#define HANDLE_NAME(name, type) STR(name),

void ROMWriter::write_persistent_handles(ObjectWriter *obj_writer JVM_TRAPS) {
#ifndef PRODUCT
  static const char *handle_names[] = {
    UNIVERSE_HANDLES_DO(HANDLE_NAME)
  };
#endif

  Oop null_owner;

  obj_writer->start_block(PERSISTENT_HANDLES_BLOCK, 0 JVM_CHECK);
  if (GenerateROMComments) {
    _comment_stream->cr();
    _comment_stream->print("\t");
  }
  // we don't write the last NUM_HANDLES_SKIP handles
  // we should write the last ROM_DUPLICATED_HANDLES other way!
  int count = Universe::__number_of_persistent_handles -
              Universe::NUM_HANDLES_SKIP - Universe::NUM_DUPLICATE_ROM_HANDLES;
  for (int index= 0; index < count; index++) {
#ifndef PRODUCT
    if (GenerateROMComments) {
      _comment_stream->cr();
      _comment_stream->print("\t");
      _comment_stream->print("/* [%d] %s */\n\t", index, handle_names[index]);
    }
#endif
    Oop object(persistent_handles[index]);
    obj_writer->put_reference(&null_owner, -1, &object JVM_CHECK);
  }
  obj_writer->end_block(JVM_SINGLE_ARG_CHECK);
#if ENABLE_HEAP_NEARS_IN_HEAP
  obj_writer->start_block(ROM_DUPLICATE_HANDLES_BLOCK, 0 JVM_CHECK);
  for (int index= 0; index < Universe::NUM_DUPLICATE_ROM_HANDLES; index++) {
#ifndef PRODUCT
    if (GenerateROMComments) {
      _comment_stream->cr();
      _comment_stream->print("\t");
      _comment_stream->print("/* [%d] %s */\n\t", index, handle_names[index]);
    }
#endif
    Oop object(persistent_handles[index+count]);
    obj_writer->put_reference(&null_owner, -1, &object JVM_CHECK);
  }
  obj_writer->end_block(JVM_SINGLE_ARG_CHECK);
#endif

#if USE_ROM_LOGGING
  mc_pers_handles.add_text_bytes(count * sizeof(int));
  mc_total.add_text_bytes(count * sizeof(int));
#endif
}

void ROMWriter::write_system_symbols(ObjectWriter *obj_writer JVM_TRAPS) {
  Oop null_owner;

  obj_writer->start_block(SYSTEM_SYMBOLS_BLOCK, 0 JVM_CHECK);
  int count = Symbols::number_of_system_symbols();
  for (int index= 0; index < count; index++) {
    Oop object((OopDesc*)system_symbols[index]);
    obj_writer->put_reference(&null_owner, -1, &object JVM_CHECK);
  }
  obj_writer->end_block(JVM_SINGLE_ARG_CHECK);

#if USE_ROM_LOGGING
  mc_pers_handles.add_text_bytes(count * sizeof(int));
  mc_total.add_text_bytes(count * sizeof(int));
#endif
}

BlockTypeFinder::BlockTypeFinder(JVM_SINGLE_ARG_TRAPS)
  :RomOopVisitor() {
  
  int n = 0;

  _meta_obj_array = Universe::new_obj_array(100 JVM_CHECK);

  // This table defines where we should put the meta objects.  The
  // first column defines the block type of the meta object. The second
  // column is applicable only if the meta object is a class -- it
  // defines the block type of a near object of the given class.
  //
  //      meta obj        near obj (if applicable)
  //      ----------+    +--------
  //                |    |
  //                v    v
  add_meta_type(n, _T_, _T_, Universe::meta_class()             JVM_CHECK);
  add_meta_type(n, _T_, _T_, Universe::method_class()           JVM_CHECK);
  add_meta_type(n, _T_, _T_, Universe::constant_pool_class()    JVM_CHECK);
  add_meta_type(n, _T_, _T_, Universe::type_array_class_class() JVM_CHECK);
  add_meta_type(n, _T_, _T_, Universe::obj_array_class_class()  JVM_CHECK);
  add_meta_type(n, _T_, _T_, Universe::entry_activation_class() JVM_CHECK);
  add_meta_type(n, _T_, _T_, Universe::execution_stack_class()  JVM_CHECK);
  add_meta_type(n, _T_, _T_, Universe::symbol_class()           JVM_CHECK);
  add_meta_type(n, _T_, _T_, Universe::instance_class_class()   JVM_CHECK);
  add_meta_type(n, _T_, _T_, Universe::compiled_method_class()  JVM_CHECK);
  add_meta_type(n, _T_, _T_, Universe::class_info_class()       JVM_CHECK);
  add_meta_type(n, _T_, _T_, Universe::stackmap_list_class()    JVM_CHECK);
  add_meta_type(n, _T_, _T_, Universe::mixed_oop_class()        JVM_CHECK);

  add_meta_type(n, _D_, _T_, Universe::bool_array_class()       JVM_CHECK);
  add_meta_type(n, _D_, _T_, Universe::byte_array_class()       JVM_CHECK);
  add_meta_type(n, _D_, _T_, Universe::char_array_class()       JVM_CHECK);
  add_meta_type(n, _D_, _T_, Universe::short_array_class()      JVM_CHECK);
  add_meta_type(n, _D_, _T_, Universe::int_array_class()        JVM_CHECK);
  add_meta_type(n, _D_, _T_, Universe::float_array_class()      JVM_CHECK);
  add_meta_type(n, _D_, _T_, Universe::long_array_class()       JVM_CHECK);
  add_meta_type(n, _D_, _T_, Universe::double_array_class()     JVM_CHECK);
  add_meta_type(n, _D_, _T_, Universe::object_array_class()     JVM_CHECK);
  add_meta_type(n, _D_, _T_, Universe::string_class()           JVM_CHECK);

#if ENABLE_ISOLATES
  add_meta_type(n, _T_, _T_, Universe::boundary_class()        JVM_CHECK);
  add_meta_type(n, _D_, _D_, Universe::task_mirror_class()     JVM_CHECK);
#endif
}

void BlockTypeFinder::add_meta_type(int &n, ROMWriter::BlockType type,
                                    ROMWriter::BlockType near_type,
                                    Oop * object JVM_TRAPS) {

  MetaObjType mt = MetaObjType::allocate(JVM_SINGLE_ARG_CHECK);
  mt.set_objref(object);
  mt.set_type(type);
  mt.set_near_type(near_type);
  _meta_obj_array.obj_at_put(n, &mt);
  n++;

  GUARANTEE(n < _meta_obj_array.length(), "Must leave zero in last entry");
}

void BlockTypeFinder::begin_object(Oop *object JVM_TRAPS) {
#if USE_SOURCE_IMAGE_GENERATOR
  if (object->is_string()) {
    String::Raw string = object->obj();
    TypeArray chars = string().value();
    GUARANTEE(writer()->string_chars()->is_null() || 
              writer()->string_chars()->equals(&chars), 
              "all romized strings must share the same chars array");
    *writer()->string_chars() = chars.obj();
  }
#endif

  if (!object->is_null()) {
    find_type(NULL, object JVM_NO_CHECK_AT_BOTTOM);
  }
}

void BlockTypeFinder::put_reference(Oop *owner, int /*offset*/, Oop *object 
                                    JVM_TRAPS){

  if (ROM::system_text_contains(object->obj())) {
    writer()->set_block_type_of(object, ROMWriter::TEXT_BLOCK JVM_CHECK);
    writer()->set_pass_of(object, PASS_DEFAULT JVM_CHECK);
    writer()->set_skip_words_of(object, 0 JVM_CHECK);
    return;
  }

  if (ROM::system_data_contains(object->obj())) {
    writer()->set_block_type_of(object, ROMWriter::DATA_BLOCK JVM_CHECK);
    writer()->set_pass_of(object, PASS_DEFAULT JVM_CHECK);
    writer()->set_skip_words_of(object, 0 JVM_CHECK);
    return;
  }

  if (!object->is_null()) {
    find_type(owner, object JVM_NO_CHECK_AT_BOTTOM);
  }
}

ROMWriter::BlockType BlockTypeFinder::find_meta_type(Oop *object) {
  OopDesc *obj   = object->obj();
  OopDesc *klass = object->klass();

  ROMWriter::BlockType meta_type = ROMWriter::UNKNOWN_BLOCK;

  MetaObjType metot;
  for(int i=0; _meta_obj_array.obj_at(i); i++) {
    metot = _meta_obj_array.obj_at(i); 
    if(metot.objref() == obj) {
      meta_type = (ROMWriter::BlockType)metot.type();
      break;
    }
    if(metot.objref() == klass) {
      meta_type = (ROMWriter::BlockType)metot.near_type();
      break;
    }
  }

  return meta_type;
}

void ROMWriter::start_work_timer() {
  set_must_suspend(false);
  set_work_timer_start(Os::java_time_millis());
}

bool ROMWriter::work_timer_has_expired() {
  if (must_suspend()) {
    return true;
  }
  jlong elapsed = Os::java_time_millis() - work_timer_start();
  return ((int)elapsed) > MaxRomizationTime;
}

void BlockTypeFinder::find_array_type(Oop *owner, Oop *object JVM_TRAPS) {
  ROMWriter::BlockType old_type = writer()->block_type_of(object JVM_CHECK);
  ROMWriter::BlockType type = old_type;
  int my_skip_words = -1;
  int my_pass = -1;

  if (old_type == ROMWriter::HEAP_BLOCK) {
    // At least one owner requires that this array be in HEAP block, then
    // it must be in heap block.
    //
    // If another owner requires that this array be in TEXT block, it will
    // be discovered by the GUARANTEE in ObjectWriter::put_reference()
    return;
  }

  if (owner == NULL) {
    if (old_type == ROMWriter::UNKNOWN_BLOCK) {
      // object must be in persistent_handles[].
      if (object->equals(Universe::rom_text_empty_obj_array()) ||
          object->equals(Universe::resource_names()) ||
          object->equals(Universe::resource_data())  ||
          object->equals(Universe::resource_size())) {
        type = ROMWriter::TEXT_BLOCK;
      } else if (object->equals(writer()->names_of_bad_classes_array())) {
        type = ROMWriter::TEXT_BLOCK;
#if ENABLE_PREINITED_TASK_MIRRORS && USE_SOURCE_IMAGE_GENERATOR && ENABLE_ISOLATES
      } else if (object->equals(Universe::mirror_list())) {
        type = ROMWriter::TASK_MIRRORS_BLOCK;
#endif      
      } else {
        type = ROMWriter::HEAP_BLOCK;
      }
    }
  }
  else {
    ROMWriter::BlockType owner_type = writer()->block_type_of(owner JVM_CHECK);
    type = ROMWriter::HEAP_BLOCK;

    if (owner_type == ROMWriter::TEXT_BLOCK) {
      type = ROMWriter::TEXT_BLOCK;
#if 0
      GUARANTEE(!object->equals(Universe::empty_short_array()),
                "Changing type of empty_short_array");
#endif
      if (owner->is_string()) {
        // obj->klass() is used at run-time by Java code, so it can't
        // be skipped.
        my_skip_words = 0;
        my_pass = PASS_FOR_STRING_BODY;
      }
    }
    else if (object->is_char_array()) {
      if (owner->is_string()) {
        // obj->klass() is used at run-time by Java code, so it can't
        // be skipped.
        type = ROMWriter::TEXT_BLOCK;
        my_skip_words = 0;
        my_pass = PASS_FOR_STRING_BODY;
      }
    }
    else if (object->is_short_array()) {
      if (object->equals(Universe::empty_short_array())) {
        type = ROMWriter::HEAP_BLOCK;
      }
      // THis was an 'else if' before.  I changed it to just 'if' and
      // added the GUARANTEE to catch the situation where you might
      // have a classinfo in text with a pointer to HEAP.  
      // "Should never happen"
      if (owner->is_class_info()) {
        ClassInfo *info = (ClassInfo*)owner;
        GUARANTEE(!object->equals(Universe::empty_short_array()),
                  "Changing type of empty_short_array");

        if (object->obj() == info->fields()) {
          type = ROMWriter::TEXT_BLOCK;

        }
        else if (object->obj() == info->local_interfaces()) {
          type = ROMWriter::TEXT_BLOCK;
        }
      }
    }
    else if (object->is_int_array()) {
      if (owner->is_obj_near()) {
        ObjNear * on = (ObjNear*)owner;
        if (on->klass()->is_method_class()) {
          type = ROMWriter::TEXT_BLOCK;
        }
      } else if (owner->is_stackmap_list()) {
        type = ROMWriter::TEXT_BLOCK;
      }
    }
    else if (object->is_obj_array()) {
      if (owner->is_class_info()) {
        ClassInfo *info = (ClassInfo*)owner;

        if (object->obj() == info->methods()) {
          type = ROMWriter::TEXT_BLOCK;
        } 
      }
    }
    else if (object->is_byte_array()) {
      if (owner->equals(Universe::resource_data())) {
        type = ROMWriter::TEXT_BLOCK;
      }
    }
  }

#if !ENABLE_MONET
  if (object->is_byte_array() && owner != NULL && owner->is_constant_pool()) {
    type = ROMWriter::TEXT_BLOCK;
    my_pass = PASS_FOR_CONSTANT_POOLS;

    // Skip the tags for the Symbols in the CP. In PRODUCT
    // mode, we never ask if an entry in a romized constant pool
    // is a symbol.
    TypeArray::Raw byte_array = object->obj();
    int len = byte_array().length();
    my_skip_words = sizeof(OopDesc)/BytesPerWord + 1; // _klass and _length
    int i;
    for (i=0; i<len; i++) {
      jint tag = byte_array().ubyte_at(i);
      if ((tag != JVM_CONSTANT_Invalid) && (tag != JVM_CONSTANT_Utf8)) {
        break;
      }
    }
    my_skip_words += (i / 4);
  }
#endif

  if (my_skip_words == -1) {
    // By default if an array is placed in TEXT its header can be safely
    // skipped.
    if (type == ROMWriter::TEXT_BLOCK && 
        !object->equals(Universe::empty_obj_array()) &&
        !object->equals(Universe::empty_short_array()) &&
        !object->equals(Universe::rom_text_empty_obj_array())) {
      my_skip_words = sizeof(OopDesc)/BytesPerWord;
    } else {
      my_skip_words = 0;
    }
  }

#if USE_BINARY_IMAGE_GENERATOR && (!defined(PRODUCT) || USE_LARGE_OBJECT_AREA)
  // We don't skip any headers in Monet-debug modes, so that we can
  // do run-time type checking (based on object->_obj->_klass) without
  // (a) having multiple passes in TEXT and (b) using a text_klass table
  // for each loaded binary image.
  my_skip_words = 0;
#endif
#if ENABLE_PREINITED_TASK_MIRRORS && USE_SOURCE_IMAGE_GENERATOR && ENABLE_ISOLATES         
  if (owner != NULL) { 
    ROMWriter::BlockType owner_type = writer()->block_type_of(owner JVM_CHECK); 
    if (type == ROMWriter::HEAP_BLOCK && owner_type == ROMWriter::TASK_MIRRORS_BLOCK) { 
      type = ROMWriter::TASK_MIRRORS_BLOCK; 
      my_skip_words = 0; 
    } 
  } 
#endif 
  if (type != old_type) {
    writer()->set_block_type_of(object, type JVM_CHECK);
    writer()->set_skip_words_of(object, my_skip_words JVM_CHECK);
    if (type == ROMWriter::TEXT_BLOCK) {
      if (my_pass == -1) {
        my_pass = PASS_FOR_OTHER_TEXT_OBJECTS;
      }
    } else if (USE_SOURCE_IMAGE_GENERATOR && type == ROMWriter::HEAP_BLOCK) {
      my_pass = PASS_FOR_OTHER_HEAP_OBJECTS;
    }

#if ENABLE_MONET 
    // In Monet mode we have a single pass for TEXT objects to reduce
    // conversion time.
    my_pass = 0;
#endif    

    if (my_pass != -1) {
      writer()->set_pass_of(object, my_pass JVM_CHECK);
    }
  }
}

void BlockTypeFinder::find_type(Oop *owner, Oop *object JVM_TRAPS) {
  ROMWriter::BlockType my_type = writer()->block_type_of(object JVM_CHECK);
  int my_pass = PASS_DEFAULT;
  int my_skip_words = 0;

  if (object->is_type_array() || object->is_obj_array()) {
    find_array_type(owner, object JVM_NO_CHECK);
    return;
  }

  if (my_type != ROMWriter::UNKNOWN_BLOCK) {
    return;
  }

#ifdef AZZERT
  if (owner != NULL) {
    ROMWriter::BlockType owner_type = writer()->block_type_of(owner JVM_CHECK);
    GUARANTEE(owner_type != ROMWriter::UNKNOWN_BLOCK, "sanity");
  }
#endif

  ROMWriter::BlockType meta_type;
  if (!ENABLE_MONET &&
      (meta_type = find_meta_type(object)) != ROMWriter::UNKNOWN_BLOCK) {
    my_type = meta_type;
  }
  else if (object->is_symbol()) {
    my_type = ROMWriter::TEXT_BLOCK;
    if (USE_SOURCE_IMAGE_GENERATOR) {
      Symbol::Raw sym = object->obj();
      my_skip_words = sizeof(OopDesc)/BytesPerWord;

      // Layout of Symbols in ROM:
      // [pass = 2] regular symbols
      // [pass = 3] fieldtype symbols
      // [pass = 4] signature symbols
      if (sym().is_valid_field_type()) {
        my_pass = PASS_FOR_FIELDTYPE_SYMBOLS;
#if USE_ROM_LOGGING
        mc_encoded_symbol.add_text(object->object_size());
#endif
      } else if (sym().is_valid_method_signature()) {
        my_pass = PASS_FOR_SIGNATURE_SYMBOLS;
#if USE_ROM_LOGGING
        mc_encoded_symbol.add_text(object->object_size());
#endif
      } else {
        my_pass = PASS_FOR_OTHER_SYMBOLS;
      }
    } else {
      // This is necessary because VerifyMethodCodes::is_array_name(), etc
      // need to check if a symbol is valid field type or not. We don't want
      // to split the binary ROM into multiple passes, so we must save
      // the _klass pointer.
      my_skip_words = 0;
    }
  }
  else if (object->is_string()) {
    my_type = ROMWriter::TEXT_BLOCK;
    my_pass = PASS_FOR_STRINGS;
  }
  else if (object->is_constant_pool()) {
    my_type = ROMWriter::TEXT_BLOCK;
    my_pass = PASS_FOR_CONSTANT_POOLS;
  }
  else if (object->is_class_info()) {
    my_type = ROMWriter::TEXT_BLOCK;
    my_skip_words = sizeof(OopDesc)/BytesPerWord;
  }
  else if (object->is_stackmap_list()) {
    if (owner != NULL) {
      my_type = ROMWriter::TEXT_BLOCK;
      my_skip_words = sizeof(OopDesc)/BytesPerWord;
      my_pass = PASS_FOR_STACKMAPS;
    } else {
      GUARANTEE(object->equals(Universe::inlined_stackmaps()), "sanity");
      my_type = ROMWriter::HEAP_BLOCK;
    }
  }
  else if (object->is_method()) {
    do_method((Method*)object, my_type, my_pass, my_skip_words);
  }
#if USE_AOT_COMPILATION
  else if (object->is_compiled_method()) {
    do_compiled_method((CompiledMethod*)object, my_type, my_pass,
                       my_skip_words JVM_CHECK);
  }
#endif
#if ENABLE_ISOLATES
  else if (object->is_task_mirror()) {
    if (object->obj() == Universe::task_class_init_marker()->obj()){
      my_type = ROMWriter::TEXT_BLOCK;
    } else {
      my_type = ROMWriter::HEAP_BLOCK;
    }
  }
#endif
  else {
    my_type = ROMWriter::HEAP_BLOCK;
  }

  if (object->object_size() == sizeof(OopDesc)
      && my_type == ROMWriter::TEXT_BLOCK) {
    // This is a special case to make sure two objects don't have the
    // illegal overlapping objects due to header skipping. See
    // OffsetFinder::begin_object() below.
    my_pass = PASS_FOR_ONE_WORD_TEXT_OBJECTS;
  }
  if (my_type == ROMWriter::TEXT_BLOCK && my_pass == PASS_DEFAULT) {
    my_pass = PASS_FOR_OTHER_TEXT_OBJECTS;
  }

#if USE_SOURCE_IMAGE_GENERATOR
  if (my_type == ROMWriter::HEAP_BLOCK) {
    if (object->is_java_class() ||
        object->blueprint() == Universe::java_lang_Class_class()->obj()) {
      my_pass = PASS_FOR_PERMANEBT_HEAP_OBJECTS;
    } else {
      my_pass = PASS_FOR_OTHER_HEAP_OBJECTS;
    }
  }
#endif

#if USE_BINARY_IMAGE_GENERATOR && (!defined(PRODUCT) || USE_LARGE_OBJECT_AREA)
  // We don't skip any headers in Monet-debug modes, so that we can
  // do run-time type checking (based on object->_obj->_klass) without
  // (a) having multiple passes in TEXT and (b) using a text_klass table
  // for each loaded binary image.
  my_skip_words = 0;
#endif

#if ENABLE_MONET
  // In Monet mode we have a single pass for TEXT objects to reduce
  // conversion time.
  my_pass = 0;
#endif
#if ENABLE_PREINITED_TASK_MIRRORS && USE_SOURCE_IMAGE_GENERATOR && ENABLE_ISOLATES
  if (owner != NULL) {
    ROMWriter::BlockType owner_type = writer()->block_type_of(owner JVM_CHECK);
    if (owner_type == ROMWriter::TASK_MIRRORS_BLOCK) {
      if (object->is_java_class() || object->is_java_near() || object->is_string()) {
         //do nothing
         //they shall stay in their blocks
      } else {
        my_type = ROMWriter::TASK_MIRRORS_BLOCK;
        my_pass = 0;
        my_skip_words = 0;
      }
    }
  }
#endif
  {
    ROMizerHashEntryDesc *entry_ptr = 
      (ROMizerHashEntryDesc *)writer()->info_for(object JVM_CHECK);
    AllocationDisabler raw_pointers_used_in_this_block;
    entry_ptr->_type = my_type;
    entry_ptr->_pass = my_pass;
    entry_ptr->_skip_words = my_skip_words;
  }
}

void BlockTypeFinder::do_method(Method* method, ROMWriter::BlockType &my_type,
                                int &my_pass, int &my_skip_words)
{
  my_type = ROMWriter::TEXT_BLOCK;
  my_pass = PASS_FOR_METHODS;

#if USE_BINARY_IMAGE_GENERATOR
  // IMPL_NOTE: re-think this later to reduce space
  my_skip_words = 0;
  (void)method;
#else
  // This also checks if constant pool merging took place at all
  if (!ROMWriter::write_by_reference(method) && 
      ((SourceROMWriter*)writer())->may_skip_constant_pool(method)) {
    const int objsize = sizeof(jobject);
    GUARANTEE(Method::constants_offset() ==
              (int)((sizeof(OopDesc)/BytesPerWord) * objsize), "Sanity");
    GUARANTEE(Method::exception_table_offset() ==
              (int)((sizeof(OopDesc)/BytesPerWord + 1) * objsize), "Sanity");
    GUARANTEE(Method::stackmaps_offset() ==
              (int)((sizeof(OopDesc)/BytesPerWord + 2) * objsize), "Sanity");
    GUARANTEE(Method::heap_execution_entry_offset()==
              (int)((sizeof(OopDesc)/BytesPerWord + 3) * objsize), "Sanity");
    
    my_skip_words = sizeof(OopDesc)/BytesPerWord + 1; // skip near, constants
    method->set_has_compressed_header();
    
    // Skip the exception_table, stackmaps, and heap_execution_entry
    // if possible. Note the nested conditions -- you can't skip
    // a field unless you also skip the field before!
    TypeArray::Raw exception_table = method->exception_table();
    if (exception_table().length() == 0) {
      method->set_has_no_exception_table();
      my_skip_words ++; // skip exception table
    
      if (method->stackmaps() == NULL) {
        method->set_has_no_stackmaps();
        my_skip_words ++; // skip stackmaps
    
        if (!method->is_impossible_to_compile()) {
          my_skip_words ++; // skip heap_execution_entry
#if ENABLE_ROM_JAVA_DEBUGGER
          if (!MakeROMDebuggable) {
            // ROM methods are not debuggable so skip entry
            my_skip_words ++;
          }
#endif
        }
      }
    }
    // update method header
    GUARANTEE(method->has_compressed_header(), "sanity");
  }
#endif
}

#if USE_AOT_COMPILATION

void BlockTypeFinder::do_compiled_method(CompiledMethod* cm, 
                                         ROMWriter::BlockType &my_type,
                                         int &my_pass, int &my_skip_words
                                         JVM_TRAPS) {
  my_type = ROMWriter::TEXT_BLOCK;

#ifdef AZZERT
  // Verify that compiled method doesn't contain references to heap objects.

  // So the recursive call below works.
  writer()->set_block_type_of(cm, my_type JVM_CHECK);

  GUARANTEE(cm->size() > 0, "No half baked Compiled Methods");
  for (RelocationReader stream(cm); !stream.at_end(); stream.advance()) {
    if (stream.kind() == Relocation::oop_type) {
      Oop value = *(OopDesc**) (cm->entry() + stream.code_offset());
      if (value.is_method() || value.is_string()) { 
        // ignore these.  We know they're okay
      } else { 
        // we can't put the compiled method into text if any objects
        // it is pointing at are in the heap
        find_type(cm, &value JVM_CHECK);
        ROMWriter::BlockType value_type =
            writer()->block_type_of(&value JVM_CHECK);
        if (value_type == ROMWriter::HEAP_BLOCK) {
          my_type = ROMWriter::DATA_BLOCK;
          cm->print_value_on(tty);
          tty->print(" is in data because of ");
          value.print_value_on(tty);
          tty->cr();
          SHOULD_NOT_REACH_HERE();
          break;
        }
      }
    }
  }
#endif

  ROMVector * vector = writer()->compiled_method_list();

  GUARANTEE(!vector->contains(cm), "Already visited");
  vector->add_element(cm JVM_NO_CHECK_AT_BOTTOM);
}

#endif

void BlockTypeFinder::finish() {
#if USE_SOURCE_IMAGE_GENERATOR
  AllocationDisabler allocation_not_allowed_in_this_function;
  SETUP_ERROR_CHECKER_ARG;
  ROMizerHashEntry::Raw entry;
  int bucket;


  // (1) Place InstanceClass in UNSCANNED_DATA if possible
  for (bucket=0; bucket<ROMWriter::INFO_TABLE_SIZE; bucket++) {
    for (entry = writer()->info_table()->obj_at(bucket); !entry.is_null(); 
         entry = entry().next()) {
      Oop::Raw object = entry().referent();
      if (object.not_null() && object.is_instance_class()) {
        InstanceClass::Raw klass = object.obj();
        Symbol::Raw name = klass().name();

        if (klass().has_embedded_static_oops()) {
          continue;
        }

        if (name.equals(Symbols::unknown())) {
          //
        } else if (klass().is_final()) {
          if (!ENABLE_ISOLATES && klass().array_class() == NULL) {
            // Array class may be created for this class -- array_class need to
            // be scanned.
            continue;
          }
        } else {
          // This class may be subclasses by application -- subtype
          // cache need to be scanned. (NOTE: subclassing by system
          // classes is OK -- all system InstanceClasses are either in DATA or
          // in permanent heap area, which means any pointers from
          // the subtype-cache to system classes do NOT need to be scanned
          // for relocation.
          //
          // Array class may be created for this class -- array_class need to
          // be scanned.
          continue;
        }

        ROMWriter::BlockType my_type = ROMWriter::DATA_BLOCK;
        int my_pass = PASS_FOR_UNSCANNED_DATA_OBJECTS;

        writer()->set_block_type_of(&object, my_type JVM_MUST_SUCCEED);
        writer()->set_pass_of(&object, my_pass JVM_MUST_SUCCEED);
      }
    }
  }

  for (bucket=0; bucket<ROMWriter::INFO_TABLE_SIZE; bucket++) {
    for (entry = writer()->info_table()->obj_at(bucket); !entry.is_null(); 
         entry = entry().next()) {
      Oop::Raw object = entry().referent();

      if (object.not_null() && object.is_java_near()) {
        // A prototypical JavaNear contains 3 fields
        //     JavaClassDesc *_klass
        //     ClassInfoDesc *_class_info -> always point to TEXT
        //     int           *_value      -> always 0
        // The _klass field could point to DATA or HEAP:
        //     - If it points to DATA, this pointer never changes and 
        //       can be determined at build time, so we can place this near
        //       object to TEXT
        //     - If it points to HEAP, the pointer is within the permanent
        //       heap area. Its value never changes at run time, but can be
        //       determined only after VM has started. So we place this
        //       near object in DATA
        JavaNear jn = object.obj();
        JavaClass klass = jn.klass();
        if (jn.obj() == klass.prototypical_near()) {
          ROMWriter::BlockType type =
              writer()->block_type_of(&klass JVM_MUST_SUCCEED);
          ROMWriter::BlockType my_type;
          int my_pass;
          if (type == ROMWriter::DATA_BLOCK) {
            my_type = ROMWriter::TEXT_BLOCK;
            my_pass = PASS_FOR_OTHER_TEXT_OBJECTS;
          } else {
            my_type = ROMWriter::DATA_BLOCK;
            my_pass = PASS_FOR_UNSCANNED_DATA_OBJECTS;
          }
          writer()->set_block_type_of(&object, my_type JVM_MUST_SUCCEED);
          writer()->set_pass_of(&object, my_pass JVM_MUST_SUCCEED);
        }
      }
    }
  }
#if ENABLE_HEAP_NEARS_IN_HEAP
  for (bucket=0; bucket<ROMWriter::INFO_TABLE_SIZE; bucket++) {
    for (entry = writer()->info_table()->obj_at(bucket); !entry.is_null(); 
         entry = entry().next()) {
      Oop::Raw object = entry().referent();
      if (object.not_null() && object.obj()->is_near()) {
        ROMWriter::BlockType type =
            writer()->block_type_of(&object JVM_MUST_SUCCEED);
        int my_pass = writer()->pass_of(&object JVM_MUST_SUCCEED);
        if (type == ROMWriter::TEXT_BLOCK) {
          type = ROMWriter::TEXT_AND_HEAP_BLOCK;
          my_pass = 0;
        } else if (type == ROMWriter::DATA_BLOCK) {
          type = ROMWriter::DATA_AND_HEAP_BLOCK;          
          my_pass = 0;
        } else {
          GUARANTEE(type == ROMWriter::HEAP_BLOCK, 
            "cloned types shalln't appear at this time");
        }
        writer()->set_block_type_of(&object, type JVM_MUST_SUCCEED);  
        writer()->set_pass_of(&object, my_pass JVM_MUST_SUCCEED);
      }
    }
  }
#endif
#endif //USE_SOURCE_IMAGE_GENERATOR
}

void OffsetVector::initialize(int count JVM_TRAPS) {
  ROMVector::initialize(count JVM_CHECK);
  set_compare_to_func((compare_to_func_type)&OffsetVector::_compare_to);
}

void OffsetVector::initialize(JVM_SINGLE_ARG_TRAPS) {
  ROMVector::initialize(JVM_SINGLE_ARG_CHECK);
  set_compare_to_func((compare_to_func_type)&OffsetVector::_compare_to);
}

jint OffsetVector::_compare_to(Oop *obj1, Oop* obj2) {
  SETUP_ERROR_CHECKER_ARG;
  ROMWriter* romwriter = ROMWriter::singleton();
  juint offset_1 = romwriter->offset_of(obj1 JVM_NO_CHECK);
  juint offset_2 = romwriter->offset_of(obj2 JVM_NO_CHECK);
  GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION, "sanity");

  if (offset_1 < offset_2) {
    return -1;
  } else {
    return 1;
  }
}

#if USE_ROM_LOGGING
void MemCounter::print_header(Stream *stream) {
  print_separator(stream);
  stream->print("               TEXT# DATA# HEAP#  TEXTb  DATAb  HEAPb");
  stream->print(" TEXT%% DATA%% HEAP%%  ALL%%");
  stream->cr();
  print_separator(stream);
}

void MemCounter::print_separator(Stream *stream) {
  stream->print("-----------------------------------------------------");
  stream->print("------------------------");
  stream->cr();
}

void MemCounter::print(Stream* stream) {
  stream->print("%-14s", name);
  stream->print(" %5d %5d %5d", text_objects, data_objects, heap_objects);
  stream->print(" %6d %6d %6d", text_bytes,   data_bytes,   heap_bytes);

  if (this != &mc_total && LogROMPercentages) {
    print_percent(stream, text_bytes,  mc_total.text_bytes);
    print_percent(stream, data_bytes,  mc_total.data_bytes);
    print_percent(stream, heap_bytes,  mc_total.heap_bytes);
    print_percent(stream, all_bytes(), mc_total.all_bytes());
  }
  stream->cr();
}

void MemCounter::print_percent(Stream *stream, int n, int total) {
  if (total == 0) {
    stream->print("     -");
  } else if (n == total) {
    stream->print("100.0%%");
  } else {
    double pect = /* 100.0 * (double)n / (double)total*/
      jvm_ddiv(jvm_dmul(jvm_i2d(n), 100.0), jvm_i2d(total));
    char buff[100];
    jvm_sprintf(buff, "%2.1f%%", pect);
    if (jvm_strlen(buff) == 4) {
      stream->print(" ");
    }
    stream->print(" %s", buff);
  }
}

void MemCounter::print_percent(Stream *stream, const char *name,
                               int objs, int bytes,
                               int all_bytes) {
  stream->print("%-10s %7d objects = %7d bytes = ", name, objs, bytes);
  print_percent(stream, bytes, all_bytes);
  stream->cr();
}
#endif // USE_ROM_LOGGING

#ifndef PRODUCT
/*
 * The following macro makes it easy to print out the values of the
 * ROMWriter and ROMOptimizer handles inside a C++ debugger. Some
 * debuggers, such as VC++, have problems showing values like
 * ROMWriter::info_table(). To work around this problem, use
 * ROMWriter_info_table instead.
 */

#define ROMWRITER_HANDLES_PEEK(type, name, xx) \
 type& ROMWriter_##name = \
       *((type*)&(ROMWriter::_romwriter_oops[ROMWriter::name##_index]));

ROMWRITER_OOP_FIELDS_DO(ROMWRITER_HANDLES_PEEK)

#endif
#endif // ENABLE_ROM_GENERATOR
