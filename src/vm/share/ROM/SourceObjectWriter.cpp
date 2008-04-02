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
#include "incls/_SourceObjectWriter.cpp.incl"


#if USE_SOURCE_IMAGE_GENERATOR


/* OBJECTWRITER CODE STARTS HERE ======================================== */

void SourceObjectWriter::start_block_comments(char *block_name) {
  _stream->cr();
  _stream->cr();
  _stream->print   ("/* ========================   ");
  _stream->print   ("Start block: %s", block_name);
  _stream->print_cr("   ======================== */");
  _stream->cr();
  _stream->cr();
}

void SourceObjectWriter::start_block(ROMWriter::BlockType type, int preset_count
                               JVM_TRAPS) {
  _current_type = type;
  _preset_count = preset_count;
#if !USE_SEGMENTED_TEXT_BLOCK_WRITER
  _offset = 0;
#else
  // Maintain _offset as the global TEXT offset
  if(type != ROMWriter::TEXT_BLOCK) _offset = 0;
#endif

  _word_position = 0;
  bool use_preset_count = true;

   /* 
    * When using the binary romizer, we don't want the variable parts 
    * info to be reset each time, because otherwise we cannot generate
    * variable parts info in the end of the ROM image generation process.
    */
#if USE_SEGMENTED_TEXT_BLOCK_WRITER
    if(_num_passes_completed == 0) {
      _variable_parts_offset = 0;
      _method_variable_parts.set_null();
    }
#else
    _variable_parts_offset = 0;
    _method_variable_parts.set_null();
#endif

  switch (_current_type) {
  case ROMWriter::TEXT_BLOCK:
    start_block_comments("text");
#if USE_SEGMENTED_TEXT_BLOCK_WRITER
    if(_num_passes_completed == 0) {
      _method_variable_parts =
        Universe::new_obj_array(_writer->count_methods() JVM_CHECK);
    }
#else
    _method_variable_parts =
      Universe::new_obj_array(_writer->count_methods() JVM_CHECK);
#endif

#if USE_SEGMENTED_TEXT_BLOCK_WRITER
    _stream->print   ("const int _rom_text_block%d", _num_passes_completed);
#else
    _stream->print   ("const int _rom_text_block");
#endif
    break;
  case ROMWriter::DATA_BLOCK:
    use_preset_count = false;
    start_block_comments("data");
    if (GenerateRelaunchableROM) {
      _stream->print_cr("const int _rom_is_relaunchable = 1;");
      _stream->print   ("const int _rom_data_block_src");
    } else {
      _stream->print_cr("const int _rom_is_relaunchable = 0;");
      _stream->print_cr("const int _rom_data_block_src[] = { 0 };");
      _stream->print   ("      int _rom_data_block");
    }
    break;
  case ROMWriter::HEAP_BLOCK:
    start_block_comments("heap");
    _stream->print("const int _rom_heap_block");
    break;
  case ROMWriter::PERSISTENT_HANDLES_BLOCK:
    start_block_comments("persistent_handles");
    _stream->print("const int _rom_persistent_handles");
    break;
#if ENABLE_HEAP_NEARS_IN_HEAP
  case ROMWriter::ROM_DUPLICATE_HANDLES_BLOCK:
    start_block_comments("rom duplicated handles");
    _stream->print("const int _rom_rom_duplicated_handles");
    break;
#endif
  case ROMWriter::SYSTEM_SYMBOLS_BLOCK:
    start_block_comments("system_symbols");
    _stream->print_cr("#if defined(PRODUCT) || ENABLE_MINIMAL_ASSERT_BUILD");
    _stream->print_cr("extern \"C\" const int system_symbols");
    _stream->print_cr("#else");
    _stream->print_cr("const int _rom_system_symbols_src");
    _stream->print_cr("#endif");
    break;
#if ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES 
  case ROMWriter::TASK_MIRRORS_BLOCK:
    start_block_comments("task_mirrors");
    _stream->print("const int _rom_task_mirrors");
    break;
#endif  
  default:
    SHOULD_NOT_REACH_HERE();
  }

  if (use_preset_count && _preset_count != 0) {
    _stream->print("[%d] = {", _preset_count);
  } else {
    _stream->print("[] = {");
  }
}

void SourceObjectWriter::end_block(JVM_SINGLE_ARG_TRAPS) {
  if (_offset == 0) {
    _stream->print_cr("0};");
  } else {
    _stream->print_cr("");
    _stream->print_cr("};");
  }

  switch (_current_type) {
  case ROMWriter::TEXT_BLOCK:
#if USE_SEGMENTED_TEXT_BLOCK_WRITER
    _stream->print_cr("const int _rom_text_block%d_size = %d;", 
                       _num_passes_completed, 
                       writer()->_pass_sizes[_num_passes_completed]);
    if(_num_passes_completed++ == 9) {
      print_method_variable_parts(JVM_SINGLE_ARG_CHECK);
    }
#else 
    _stream->print_cr("const int _rom_text_block_size = %d;", _offset);

    /* When using the binary romizer, we generate method variable
     * parts in the end of the ROM image generation process
     */
      print_method_variable_parts(JVM_SINGLE_ARG_CHECK);
#endif
    break;
  case ROMWriter::DATA_BLOCK:
    if (GenerateRelaunchableROM) {
      _stream->print_cr("int _rom_data_block[%d];", _offset/sizeof(int));
    }
    _stream->print_cr("const int _rom_data_block_size = %d;", _offset);
    _stream->print_cr("const int _rom_data_block_scanned_size = %d;",
                      writer()->_data_block_scanned_count * sizeof(int));
    break;
  case ROMWriter::HEAP_BLOCK:
    _stream->print_cr("const int _rom_heap_block_size = %d;", _offset);
    _stream->print_cr("const int _rom_heap_block_permanent_size = %d;",
                      writer()->_heap_block_permanent_count * sizeof(int));
    break;
  case ROMWriter::PERSISTENT_HANDLES_BLOCK:
    _stream->print_cr("const int _rom_persistent_handles_size = %d;", _offset);
    break;
#if ENABLE_HEAP_NEARS_IN_HEAP
  case ROMWriter::ROM_DUPLICATE_HANDLES_BLOCK:
    _stream->print_cr("const int _rom_rom_duplicated_handles_size = %d;", _offset);
    break;
#endif
#if ENABLE_PREINITED_TASK_MIRRORS && USE_SOURCE_IMAGE_GENERATOR && ENABLE_ISOLATES 
  case ROMWriter::TASK_MIRRORS_BLOCK:
    _stream->print_cr("const int _rom_task_mirrors_size = %d;", _offset);
    write_rom_tm_bitmap();
    break;
#endif
  case ROMWriter::SYSTEM_SYMBOLS_BLOCK:
    /* Nothing to do */
    break;
  default:
    SHOULD_NOT_REACH_HERE();
  }
#if !USE_SEGMENTED_TEXT_BLOCK_WRITER
  if (_preset_count != 0) {
    GUARANTEE(_preset_count == (_offset/(int)sizeof(int)), "sanity");
  }
#endif
}

#if ENABLE_PREINITED_TASK_MIRRORS && USE_SOURCE_IMAGE_GENERATOR && ENABLE_ISOLATES 
void SourceObjectWriter::write_rom_tm_bitmap() {
  TypeArray::Raw bitmap = writer()->rom_tm_bitmap()->obj();
  int length = bitmap().length();
  _stream->print_cr("const int _rom_task_mirrors_bitmap[%d] = {", length);
  for (int i = 0; i < length; i++) {
    writer()->write_plain_int(bitmap().int_at(i), _stream);
    if (i != length - 1) {
      put_separator();
    }
  }
  _stream->print_cr("\n};\n");
}
#endif

void SourceObjectWriter::begin_object(Oop *object JVM_TRAPS) {
  ROMWriter::BlockType type = writer()->block_type_of(object JVM_CHECK);
  if (type != _current_type) {
    return;
  }
  if (ROMWriter::write_by_reference(object)) {
    return;
  }
  int pass = writer()->pass_of(object JVM_CHECK);
  int skip_words = writer()->skip_words_of(object JVM_CHECK);
  int skip_bytes = sizeof(jobject) * skip_words;

  FarClass blueprint = object->blueprint();
  InstanceSize instance_size = blueprint.instance_size();

  if (GenerateROMComments) {
    _stream->cr();
    _stream->cr();
    _stream->print("/%s ", "*");
    int header_offset = (type == ROMWriter::DATA_BLOCK) ? sizeof(jint) : 0;
    header_offset -= skip_bytes;
#if ENABLE_PREINITED_TASK_MIRRORS && USE_SOURCE_IMAGE_GENERATOR && ENABLE_ISOLATES
    if (_current_type == ROMWriter::TASK_MIRRORS_BLOCK) {
       //do nothing at the time
    } else  
#endif
    {
#if ENABLE_HEAP_NEARS_IN_HEAP
      /*
        When writing reference to a cloned object we must reference the correct instance. 
       Links from HEAP must point to HEAP(this is the idea of optimization) and references from 
       TEXT or DATA must point to TEXT or DATA instance.
      */
      if (type != ROMWriter::TEXT_AND_HEAP_BLOCK && type != ROMWriter::DATA_AND_HEAP_BLOCK) {
        writer()->write_reference(type, _offset + header_offset, _current_type, _stream);
      } else if (_current_type == ROMWriter::HEAP_BLOCK) {
        writer()->write_reference(ROMWriter::HEAP_BLOCK, _offset + header_offset, _current_type, _stream);
      } else if (type == ROMWriter::TEXT_AND_HEAP_BLOCK) {
        writer()->write_reference(ROMWriter::TEXT_BLOCK, _offset + header_offset, _current_type, _stream);
      } else {
        writer()->write_reference(ROMWriter::DATA_BLOCK, _offset + header_offset, _current_type, _stream);
      }
#else
      writer()->write_reference(type, _offset + header_offset, _current_type, _stream);
#endif
    }
    _stream->print(" [%5d] ", (object->object_size() / sizeof(jint)));
    if (skip_words > 0) {
      _stream->print(" skip=%d ", skip_words);
    }
    Verbose ++;
    object->print_rom_definition_on(_stream);
    Verbose --;
    _stream->print(" */\n\t");
  } else {
    _stream->print("\n\t");
  }

  count(object, -skip_bytes);

  _word_position = 0;
  int my_calculated_offset = writer()->offset_of(object JVM_CHECK);
#if ENABLE_HEAP_NEARS_IN_HEAP && USE_SOURCE_IMAGE_GENERATOR
  /*
    for objects with type *_AND_HEAP_BLOCK, offset contains offset for the copy located in * block, 
    while heap_offset contains offset of the copy located in HEAP block
  */
  if (type == ROMWriter::TEXT_AND_HEAP_BLOCK && _current_type == ROMWriter::HEAP_BLOCK) {
    GUARANTEE(object->obj()->is_near(), "TEXT_AND_HEAP_BLOCK  usage sanity");
    my_calculated_offset = writer()->heap_offset_of(object JVM_CHECK);
  }
  if (type == ROMWriter::DATA_AND_HEAP_BLOCK && _current_type == ROMWriter::HEAP_BLOCK) {
    GUARANTEE(object->obj()->is_near(), "DATA_AND_HEAP_BLOCK  usage sanity");
    my_calculated_offset = writer()->heap_offset_of(object JVM_CHECK);
  }
#endif
  int real_offset = _offset - skip_bytes;
  GUARANTEE(my_calculated_offset == real_offset,
            "calculated address != real address");
}

void SourceObjectWriter::end_object(Oop *object JVM_TRAPS) {
  ROMWriter::BlockType type = writer()->block_type_of(object JVM_CHECK);
  if (!is_subtype(type, _current_type)) {  
    return;
  }
}

void SourceObjectWriter::put_separator() {
  if ((_word_position % WORDS_PER_LINE) == (WORDS_PER_LINE-1)) {
    _stream->print(",\n\t");
  } else {
    _stream->print(", ");
  }
  _word_position ++;
}

void SourceObjectWriter::put_reference(Oop *owner, int offset, Oop *object JVM_TRAPS) {
  if (!owner->is_null()) {
    ROMWriter::BlockType owner_type = writer()->block_type_of(owner JVM_CHECK);
    if (!is_subtype(owner_type, _current_type)) {  
      return;
    }
  }

#if !USE_SEGMENTED_TEXT_BLOCK_WRITER
  if (ROMWriter::write_by_reference(owner)) {
    return;
  }
#endif

  if (object->is_null()) {
    writer()->write_null(_stream);
    put_separator();

  } else {
    if (GenerateROMComments && _word_position > 0) {
      // This must be the first item on the line
      _stream->print("\n\t");
      _word_position = 0;
    }
#if ENABLE_PREINITED_TASK_MIRRORS && USE_SOURCE_IMAGE_GENERATOR && ENABLE_ISOLATES         
    if (_current_type == ROMWriter::TASK_MIRRORS_BLOCK) { 
      writer()->write_tm_reference(owner, offset, object, _stream); 
    } else  
#endif 
    { 
      writer()->write_reference(object, _current_type, _stream JVM_CHECK);
    }
    put_separator();

    if (GenerateROMComments) {
      _stream->print(" /* ");
      Verbose++;
      object->print_value_on(_stream);
      Verbose--;
      _stream->print(" */\n\t");
      _word_position = 0;
    }
  }
  _offset += sizeof(int);
}

void SourceObjectWriter::put_int(Oop *owner, jint value JVM_TRAPS) {
  ROMWriter::BlockType owner_type = writer()->block_type_of(owner JVM_CHECK);
  if (!is_subtype(owner_type, _current_type)) {  
    return;
  }
  writer()->write_int(value, _stream);

  if (VerboseROMComments && owner->is_constant_pool()) {
    int owner_offset = writer()->offset_of(owner JVM_CHECK);
    int cp_index = _offset - owner_offset - sizeof(ConstantPoolDesc);
    cp_index /= 4;
    _stream->print(" /* [%4d] ", cp_index);
    ConstantPool cp = owner->obj();
    cp.print_entry_on(_stream, cp_index JVM_CHECK);
    _stream->print(" */\n\t", cp_index);
    _word_position = 0;
  } if (VerboseROMComments && owner->is_method()) {
    int owner_offset = writer()->offset_of(owner JVM_CHECK);
    Method *method = (Method*)owner;
    if (_offset - owner_offset == Method::stackmaps_offset()) {
      _stream->print(" /* inlined StackmapList:");
      StackmapList::Raw sm = method->stackmaps();
      _stream->print(" 0x%08x", sm().entry_count());
      for (int i=0; i<sm().entry_count(); i++) {
        int n = sm().int_field(sm().entry_stat_offset(i));
        _stream->print(", 0x%08x", n);
      }
      _stream->print(" */");
    }
    put_separator();
  } else {
    put_separator();
  }

  _offset += sizeof(int);
}

void SourceObjectWriter::put_int_by_mask(Oop *owner, jint be_word, jint le_word,
                                   jint typemask JVM_TRAPS) {
  ROMWriter::BlockType owner_type = writer()->block_type_of(owner JVM_CHECK);
  if (!is_subtype(owner_type, _current_type)) {  
    return;
  }

  if (_word_position > 0) {
    _stream->print("\n\t");
  }
  _stream->print("ROM_BL(");
  writer()->write_plain_int(be_word, _stream);
  _stream->print(",");
  writer()->write_plain_int(le_word, _stream);
  _offset += sizeof(int);
  _stream->print(",");

  switch (typemask) {
  case ROMWriter::BYTE_BYTE_BYTE_BYTE:
    _stream->print("BBBB");
    break;
  case ROMWriter::SHORT_BYTE_BYTE:
    _stream->print("SBB");
    break;
  case ROMWriter::BYTE_BYTE_SHORT:
    _stream->print("BBS");
    break;
  case ROMWriter::SHORT_SHORT:
    _stream->print("SS");
    break;
  default:
    SHOULD_NOT_REACH_HERE();
  }
  _stream->print("),\n\t");
  _word_position = 0;
}

void SourceObjectWriter::put_long(Oop *owner, jint msw, jint lsw JVM_TRAPS) {
  ROMWriter::BlockType owner_type = writer()->block_type_of(owner JVM_CHECK);
  if (!is_subtype(owner_type, _current_type)) {  
    return;
  }
  if (GenerateROMComments && _word_position > 0) {
    // This must be the first item on the line
    _stream->print("\n\t");
    _word_position = 0;
  }
  writer()->write_long(msw, lsw, _stream);
  _offset += sizeof(int) * 2;
  put_separator();

  if (GenerateROMComments) {
    _stream->print(" /* ");
    _stream->print(OsMisc_jlong_format_specifier(),
                   jlong_from_msw_lsw(msw, lsw));
    _stream->print(" */\n\t");
    _word_position = 0;
  }
}

void SourceObjectWriter::put_double(Oop *owner, jint msw, jint lsw JVM_TRAPS) {
  ROMWriter::BlockType owner_type = writer()->block_type_of(owner JVM_CHECK);
  if (!is_subtype(owner_type, _current_type)) {  
    return;
  }
  if (GenerateROMComments && _word_position > 0) {
    // This must be the first item on the line
    _stream->print("\n\t");
    _word_position = 0;
  }
  writer()->write_double(msw, lsw, _stream);
  _offset += sizeof(int) * 2;
  put_separator();

  if (GenerateROMComments) {
    _stream->print(" /* ");
    _stream->print_double(jdouble_from_msw_lsw(msw, lsw));
    _stream->print(" */\n\t");
    _word_position = 0;
  }
}

void SourceObjectWriter::put_symbolic(Oop *owner, int offset JVM_TRAPS) {
  ROMWriter::BlockType owner_type = writer()->block_type_of(owner JVM_CHECK);
  if (!is_subtype(owner_type, _current_type)) {  
    return;
  }

  address addr = (address)owner->int_field(offset);

  if (_word_position != 0) {
    _stream->print("\n\t");
  }

  FarClass blueprint = owner->blueprint();
  InstanceSize instance_size = blueprint.instance_size();
  switch (instance_size.value()) {
  case InstanceSize::size_method:
    put_method_symbolic((Method*)owner, offset JVM_CHECK);
    break;

  case InstanceSize::size_obj_array_class:
  case InstanceSize::size_type_array_class:
  case InstanceSize::size_far_class:
    put_oopmap(owner, addr);
    break;

  case InstanceSize::size_compiled_method:
    put_compiled_method_symbolic((CompiledMethod*)owner, offset JVM_CHECK);
    break;

  default:
    SHOULD_NOT_REACH_HERE();
  }

  _stream->print(",\n\t");
  _offset += sizeof(int);
  _word_position = 0;
}

void SourceObjectWriter::put_method_symbolic(Method *method, int offset
                                             JVM_TRAPS) {
  if ((method->is_native() || method->is_abstract()) &&
    offset == Method::native_code_offset()) {
    put_c_function(method, method->get_native_code() JVM_CHECK);
  } else if (method->is_quick_native() &&
    offset == Method::quick_native_code_offset()) {
    put_c_function(method, method->get_quick_native_code() JVM_CHECK);
  } else if (offset == Method::variable_part_offset()) {
    put_method_variable_part(method JVM_CHECK);
  } else if (offset == Method::heap_execution_entry_offset()) {
    if (method->has_compiled_code()) {
      CompiledMethod cm = method->compiled_code();
      writer()->write_compiled_code_reference(&cm, _stream, true JVM_CHECK);
    } else {
      put_c_function(method, method->execution_entry() JVM_CHECK);
    }
  } else {
    // We are outputing the code.
    // Generate the alternate endian form of the code, if we haven't already
    if (!method->equals(&_saved_current_method)) {
      _saved_current_method = method;
      _saved_alt_method = method->create_other_endianness(JVM_SINGLE_ARG_CHECK);
    }
    Method *be_method, *le_method;
    if (HARDWARE_LITTLE_ENDIAN && ENABLE_NATIVE_ORDER_REWRITING) {
      // Original code has tableswitch, etc in little endian format
      le_method = method; be_method = &_saved_alt_method;
    } else { 
      // Original code has tableswitch, etc in big endian format
      be_method = method; le_method = &_saved_alt_method;
    }
    // There are probably shortcuts, but we want to be absolutely careful
    // to get the bytes just right.
    int be_byte0 = be_method->ubyte_field(offset);
    int be_byte1 = be_method->ubyte_field(offset+1);
    int be_byte2 = be_method->ubyte_field(offset+2);
    int be_byte3 = be_method->ubyte_field(offset+3);

    int le_byte0 = le_method->ubyte_field(offset);
    int le_byte1 = le_method->ubyte_field(offset+1);
    int le_byte2 = le_method->ubyte_field(offset+2);
    int le_byte3 = le_method->ubyte_field(offset+3);

    // Big endian version is independent of ENABLE_NATIVE_ORDER_REWRITING
    int be_word  = (be_byte0<<24) + (be_byte1<<16) + (be_byte2<<8) + be_byte3;
    // Little endian, !ENABLE_NATIVE_ORDER_REWRITING
    int le_word  = (be_byte3<<24) + (be_byte2<<16) + (be_byte1<<8) + be_byte0;
    // Little endian, ENABLE_NATIVE_ORDER_REWRITING
    int le_wordx = (le_byte3<<24) + (le_byte2<<16) + (le_byte1<<8) + le_byte0;

    // Output either ROM_BL or ROM_BLX, depending on whether the little-endian
    // version depends on ENABLE_NATIVE_ORDER_REWRITING or not
    _stream->print("%s(", le_word == le_wordx ? "ROM_BL" : "ROM_BLX");
    writer()->write_plain_int(be_word, _stream);
    _stream->print(",");
    writer()->write_plain_int(le_word, _stream);
    if (le_word != le_wordx) { 
      _stream->print(",");
      writer()->write_plain_int(le_wordx, _stream);
    }
    _stream->print(",code)");
  }
}


JvmExecutionEntry SourceObjectWriter::jvm_core_entries[] = {
  EXECUTION_ENTRIES_DO(ROM_DEFINE_ENTRY)
  {(unsigned char*)NULL, (char*)NULL}
};

void SourceObjectWriter::put_compiled_method_symbolic(CompiledMethod *cm,
                                                      int offset JVM_TRAPS) {
  Method method = cm->method();
  JavaClass holder = method.holder();

  const address value = (address)cm->int_field(offset);

  if (value == (address)Java_void_unimplemented) {
    // For midp and the like, we occassionally compile things that are
    // not yet defined.
    if ((method.is_native() || method.is_abstract()) &&
            value == method.get_native_code()) {
      put_c_function(&method, method.get_native_code() JVM_CHECK);
    } else if (method.is_quick_native() &&
                 value == method.get_quick_native_code()) {
      put_c_function(&method, method.get_quick_native_code() JVM_CHECK);
    } else {
      SHOULD_NOT_REACH_HERE();
    }
    return;
  }

  if (method.is_native()) {
    // This should be 99% of all cases. 
    GUARANTEE(!method.is_quick_native(), "Shouldn't compile quick natives");
    GUARANTEE(method.get_native_code() == value, "Must be correct address");
    put_c_function(&method, method.get_native_code() JVM_CHECK);   
    return;
  }
    
  ConstantPool cp = method.constants();
  for (int i = 1; i < cp.length(); i++) {
    ConstantTag tag = cp.tag_at(i);
    Method m;
    if (tag.is_resolved_static_method()) { 
      m = cp.resolved_static_method_at(i);
    } else if (tag.is_resolved_virtual_method()) {
      int vtable_index;
      int class_id;
      cp.resolved_virtual_method_at(i, vtable_index, class_id);

      // Get the class from the constant pool.
      JavaClass::Raw klass = Universe::class_from_id(class_id);
      ClassInfo::Raw info = klass().class_info();
      m = info().vtable_method_at(vtable_index);
    }

    if (!m.is_null() && m.is_native()) {
      if (value == (m.is_quick_native() 
                          ? m.get_quick_native_code()
                          : m.get_native_code())) {
        TypeArray native_name = Natives::get_native_function_name(&m JVM_CHECK);
        if (GenerateROMComments) {
          _stream->print("/* Native= */ ");
        }
        _stream->print("(int)%s", (char*)native_name.data());
        return;
      }
    }
  }

  // It could be an entry invoked from this method.
  for (int n=0; n<2; n++) {
    const JvmExecutionEntry *p = (n) ? jvm_api_entries : jvm_core_entries;
    for (; p->name; p++) {
      if (value == p->addr) {
        const char * const entry_comment = 
          GenerateROMComments ? "/* Invoked entry= */" : "";
        _stream->print("%s (int)%s", entry_comment, p->name);
        return;
      }
    }
  }

  // Currently, the only inline addresses are of native code.  We may need to
  // eventually change the next line to be something machine-dependent.
  //
  // If you come to here, you need to put the name of <value> into
  // gp_internal_constants[] in CompilerStubs_arm.cpp, near
  // "fixed_interpreter_method_entry".
  SHOULD_NOT_REACH_HERE()
}


void SourceObjectWriter::put_method_variable_part(Method *method JVM_TRAPS) {
  // Note: if you want to put method in heap, you have to add code to
  // relocate its variable_part
  GUARANTEE(_current_type != ROMWriter::HEAP_BLOCK,"can't put method in heap");

  _stream->print("/* variable_part= */");
    
  if (has_split_variable_part(method)) { 
    _stream->print("(int) &(_rom_method_variable_parts[%d])", 
                   _variable_parts_offset);
    _method_variable_parts.obj_at_put(_variable_parts_offset, method);

    _variable_parts_offset ++;
  } else { 
    const int delta = 
        Method::heap_execution_entry_offset() - Method::variable_part_offset();
    writer()->write_reference(_current_type, _offset + delta,
                             _current_type, _stream);
  }
}

bool SourceObjectWriter::has_split_variable_part(Method *method) {
  GUARANTEE(_current_type == ROMWriter::TEXT_BLOCK, "Sanity");
#if ENABLE_ROM_JAVA_DEBUGGER
  if (MakeROMDebuggable) {
    return true;
  } else
#endif
  {
    return !method->is_impossible_to_compile();
  }
}

const char* SourceObjectWriter::get_native_function_return_type(Method *method)
{
  Signature sig =  method->signature();
  switch (sig.return_type(true)) {
  case T_BOOLEAN:
    return "jboolean";
  case T_CHAR:
    return "jchar";
  case T_FLOAT:
    return "jfloat";
  case T_DOUBLE:
    return "jdouble";
  case T_BYTE:
    return "jbyte";
  case T_SHORT:
    return "jshort";
  case T_INT:
    return "jint";
  case T_LONG:
    return "jlong";
  case T_OBJECT:
  case T_ARRAY:
    return "jobject";
  case T_VOID:
    return "void";
  default:
    SHOULD_NOT_REACH_HERE();
    return NULL;
  }
}

void SourceObjectWriter::put_c_function(Method *method, address addr,
                                  Stream *output_stream JVM_TRAPS) {
  char *name = NULL;
  char *prefix = "";
  bool is_entry = (addr == method->execution_entry());
  TypeArray native_name;

  if (GenerateROMComments) {
    if (is_entry) {
      output_stream->print("%s", "/* entry =*/ ");
    } else {
      GUARANTEE(method->is_native() || method->is_abstract(), "sanity");
      output_stream->print("%s", "/* native=*/ ");
    }
  }

  if (is_entry) {
    for (int n=0; n<2; n++) {
      const JvmExecutionEntry *p = (n) ? jvm_api_entries : jvm_core_entries;
      for (; p->name; p++) {
        if (addr == p->addr) {
          name = p->name;
          goto done;
        }
      }
    }
    done:
    GUARANTEE(name != NULL, "sanity");
  } else {
    if (addr == (address)Java_void_unimplemented) {
      address entry_addr = method->execution_entry();
      bool is_native_entry = false;

      const JvmExecutionEntry *p = jvm_api_entries;
      for (; p->name; p++) {
        if (entry_addr == p->addr) {
          is_native_entry = true;
          break;
        }
      }

      if (is_native_entry) {
        // This is a class built in this VM that has a fixed entry
        name = "Java_void_unimplemented";
      } else {
        // We're romizing a class that's not part of the VM (e.g.,
        // we're romizing MIDP using a CLDC-only VM.) We failed to
        // find a native function for this method, because this
        // class is not part of the VM's NativesTable. We'll
        // find a name for it below using get_native_function_name().
        name = NULL;
      }
    }
    else if (addr == (address)Java_abstract_method_execution) {
      name = "Java_abstract_method_execution";
    }
    else if (addr == (address)Java_illegal_method_execution) {
      name = "Java_illegal_method_execution";
    }
    else if (addr == (address)Java_incompatible_method_execution) {
      name = "Java_incompatible_method_execution";
    }

    if (name == NULL) {
      native_name = Natives::get_native_function_name(method JVM_CHECK);
      GUARANTEE(!native_name.is_null(), "sanity");
      name = (char*) native_name.data();
      if (is_kvm_native(method)) {
        prefix = "__kvm_";
        write_kvm_method_stub(method, name);
      } else {
        _declare_stream->print_cr("extern \"C\" %s %s();", 
                                  get_native_function_return_type(method),
                                  name);
      }
    }
  }

  output_stream->print("(int) %s%s", prefix, name);
}

bool SourceObjectWriter::is_kvm_native(Method *method) {
  ObjArray::Raw array = _writer->_optimizer.kvm_native_methods_table()->obj();
  if (array.not_null()) {
    int size = array().length();
    if (size > 0) {
      bool dummy;

      InstanceClass::Raw klass = method->holder();
      Symbol::Raw class_name = klass().original_name();
      Symbol::Raw method_name = method->get_original_name(dummy);
      Symbol::Raw method_sig = method->signature();

      for (int i=0; i<size; i+=3) {
        if ((class_name.obj()  == array().obj_at(i+0)) &&
            (method_name.obj() == array().obj_at(i+1)) &&
            (method_sig.obj()  == array().obj_at(i+2))) {
          return true;
        }
      }
    }
  }
  return false;
}

void SourceObjectWriter::write_kvm_method_stub(Method *method, char *name) {
  char *ret_type_string = NULL;
  Signature::Raw sig = method->signature();
  BasicType ret_type = sig().return_type();

  switch (ret_type) {
  case T_BOOLEAN:
  case T_CHAR:
  case T_BYTE:
  case T_SHORT:
  case T_INT:
  case T_OBJECT:
  case T_ARRAY:
    // Don't use jboolean, etc, which are not defined in ROMImage.cpp
    ret_type_string = "int";
    break;
  case T_FLOAT:
    ret_type_string = "jfloat";
    break;
  case T_VOID:
    ret_type_string = "void";
    break;
  case T_DOUBLE:
    ret_type_string = "jdouble";
    break;
  case T_LONG:
    ret_type_string = "jlong";
    break;
  case T_SYMBOLIC:
  case T_ILLEGAL:
  default:
    SHOULD_NOT_REACH_HERE();
  }

  Stream *s = &((SourceROMWriter*)_writer)->_declare_stream;
  s->print_cr("extern \"C\" void __kvm_%s();", name);

  s = &((SourceROMWriter*)_writer)->_kvm_stream;
  s->print_cr("extern \"C\" {");
  s->print_cr("extern void _kvm_check_stack();");
  s->print_cr("extern void %s();", name);

  if (method->is_static()) {
    s->print("/* static */ ");
  }

  s->print_cr("%s __kvm_%s() {", ret_type_string, name);
  s->print_cr("    extern unsigned char * _kni_parameter_base;");
  s->print_cr("    extern int * _kvm_stack_top;");
  s->print_cr("    extern int   _kvm_return_value32;");
  s->print_cr("    extern jlong _kvm_return_value64;");
  s->print_cr("    extern int   _kvm_return_type;");
  s->print_cr("    extern int   _kvm_pushed;");
  s->print_cr("    extern int   _in_kvm_native_method;");
  s->print_cr("    extern int * _kvm_stack_bottom;");

  // See comments at the top of kvmcompat.c for the stack layout.
  //
  // Note that _kni_parameter_base has an "dummy slot" for static methods,
  // so the calculation of _kvm_stack_top does not depend on static-ness.
  // For example, for method foo(int,byte), regardless of static-ness,
  // parameter_word_size would be 3 words.
  int parameter_word_size = sig().parameter_word_size(false);

  // We support only JavaStackDirection == -1. Note that
  // _kvm_stack_top is an "full stack pointer": it points to the
  // space occupied by the current top of stack.
  s->print_cr("    _kvm_return_type = %d;", ret_type);
  if (ret_type == T_OBJECT || ret_type == T_ARRAY) {
    // Must NULL return value, since it would be scanned by GC.
    s->print_cr("    _kvm_return_value32 = 0;");
    s->print_cr("#ifdef AZZERT");
    s->print_cr("    _in_kvm_native_method = 1;");
    s->print_cr("#endif");
  } else {
    s->print_cr("#ifdef AZZERT");
    s->print_cr("    _kvm_return_value32 = 0xdeadbeef;");
    s->print_cr("    _in_kvm_native_method = 1;");
    s->print_cr("#endif");
  }
  s->print_cr("#ifdef AZZERT");
  s->print_cr("    _kvm_pushed = 0;");
  if (method->is_static()) {
    s->print_cr("    _kvm_stack_bottom = (int*)(_kni_parameter_base);");
  } else {
    s->print_cr("    _kvm_stack_bottom = (int*)(_kni_parameter_base + 4);");
  }
  s->print_cr("#endif");

  s->print   ("    _kvm_stack_top = (int*)(_kni_parameter_base - sizeof(int)");
  s->print_cr(" * (%d-1));", parameter_word_size);
  s->print_cr("    %s();", name);
  s->print_cr("#ifdef AZZERT");
  s->print_cr("    _kvm_check_stack();");
  s->print_cr("    _in_kvm_native_method = 0;");
  s->print_cr("#endif");

  switch (ret_type) {
  case T_VOID:
    // do nothing
    break;
  case T_FLOAT:
    s->print_cr("    return (jfloat)_kvm_return_value32;");
    break;
  case T_DOUBLE:
    s->print_cr("    return *((jdouble*)&_kvm_return_value64);");
    break;
  case T_LONG:
    s->print_cr("    return (jlong)_kvm_return_value64;");
    break;
  default:
    s->print_cr("    return _kvm_return_value32;");
  }
  s->print_cr("}");
  s->print_cr("}");
}

void SourceObjectWriter::print_entry_declarations() {
  for (int n=0; n<2; n++) {
    const JvmExecutionEntry *p = (n) ? jvm_api_entries : jvm_core_entries;
    for (; p->name; p++) {
      _declare_stream->print_cr("extern \"C\" void %s();",
                                p->name);
    }
  }
  _declare_stream->print_cr("extern \"C\" void %s();",
                            "Java_void_unimplemented");
  _declare_stream->print_cr("extern \"C\" void %s();",
                            "Java_abstract_method_execution");
  _declare_stream->print_cr("extern \"C\" void %s();",
                            "Java_illegal_method_execution");
  _declare_stream->print_cr("extern \"C\" void %s();",
                            "Java_incompatible_method_execution");
}

struct OopMapInfo {
  address addr;
  char *  name;
};

#define DEFINE_OOPMAP_INFO(x)  {(address)& oopmap_ ## x, STR(oopmap_ ## x)},

static OopMapInfo oopmap_info[] = {
  {(address)&oopmap_Empty, "oopmap_Empty"},
  OOPMAP_CLASSES_DO(DEFINE_OOPMAP_INFO)
};

void SourceObjectWriter::put_oopmap(Oop *owner, address addr) {
  const int count = sizeof(oopmap_info)/sizeof(oopmap_info[0]);

  for (int i=0; i<count; i++) {
    if (addr == oopmap_info[i].addr) {
      _stream->print("(int) %s", oopmap_info[i].name);
      return;
    }
  }
  tty->print_cr("Failed to resolve oopmap 0x%x", addr);
  BREAKPOINT;
}

void SourceObjectWriter::print_oopmap_declarations() {
  const int count = sizeof(oopmap_info)/sizeof(oopmap_info[0]);

  for (int i=0; i<count; i++) {
    _declare_stream->print_cr("extern \"C\" {extern unsigned char %s[];}",
                              oopmap_info[i].name);
  }
}

void SourceObjectWriter::print_method_variable_parts(JVM_SINGLE_ARG_TRAPS) {
  int variable_parts_count = _variable_parts_offset;
  if (GenerateRelaunchableROM) {
    _reloc_stream->print_cr("const int _rom_method_variable_parts_src[] = {");
  } else {
    _reloc_stream->print_cr("const int _rom_method_variable_parts_src[]={0};");
    _reloc_stream->print_cr("int _rom_method_variable_parts[] = {");
  }
  if (variable_parts_count <= 0) {
    _reloc_stream->print_cr("0");
  } else {
    for (int i=0; i<variable_parts_count; i++) {
      Method method = _method_variable_parts.obj_at(i);
      _reloc_stream->print("/*%4d*/", i);

      if (method.has_compiled_code()) {
          CompiledMethod cm = method.compiled_code();
          writer()->write_compiled_code_reference(&cm, _reloc_stream, true
                                                    JVM_CHECK);
      } else { 
          put_c_function(&method, method.execution_entry(),
                         _reloc_stream JVM_CHECK);
      }
      _reloc_stream->print_cr(",");

    }
  }
  _reloc_stream->print_cr("};");

  _reloc_stream->print_cr("const int _rom_method_variable_parts_size = %d;",
                          variable_parts_count * sizeof(int));

  if (GenerateRelaunchableROM) {
    if (variable_parts_count < 1) {
      // some compilers don't like array size of 0
      variable_parts_count = 1;
    }
    _reloc_stream->print_cr("int _rom_method_variable_parts[%d];",
                            variable_parts_count);
  }
}

void SourceObjectWriter::count(Oop *object, int adjustment) {
  FarClass blueprint = object->blueprint();
  InstanceSize instance_size = blueprint.instance_size();
  int num_bytes = object->object_size() + adjustment;

  switch (instance_size.value()) {
    case InstanceSize::size_symbol:
      count(mc_symbol, num_bytes);
      break;
    case InstanceSize::size_generic_near:
    case InstanceSize::size_java_near:
    case InstanceSize::size_obj_near:
      count(mc_meta, num_bytes);
      break;

    case InstanceSize::size_type_array_1:
      count(mc_array1, num_bytes);
      break;
    case InstanceSize::size_type_array_2:
      if (object->is_char_array()) {
        count(mc_array2c, num_bytes);
      } else {
        count(mc_array2s, num_bytes);
      }
      break;
    case InstanceSize::size_type_array_4:
      count(mc_array4, num_bytes);
      break;
    case InstanceSize::size_type_array_8:
      count(mc_array8, num_bytes);
      break;
    case InstanceSize::size_obj_array:
      count(mc_obj_array, num_bytes);
      break;

    case InstanceSize::size_method: {
        Method *method = (Method*)object;
        int body_size = method->code_size();
        int header_size = num_bytes - body_size;

        if (has_split_variable_part(method)) {
          num_bytes = method->object_size() + adjustment;
          mc_variable_parts.add_data_bytes(sizeof(MethodVariablePart));
          mc_total.add_data_bytes(sizeof(MethodVariablePart));
        }

        count(mc_method, num_bytes);
        count(mc_method_header, header_size);
        if (body_size > 0) {
          count(mc_method_body, body_size);
        }
        if (method->is_native()) {
          count(mc_native_method, num_bytes);
        }
        if (method->is_abstract()) {
          count(mc_abstract_method, num_bytes);
        }
        if (!method->is_static()) {
          count(mc_virtual_method, num_bytes);
        }
        Symbol name = method->name();
        if (name.equals(Symbols::class_initializer_name())) {
          count(mc_clinit_method, num_bytes);
        }
        if (name.equals(Symbols::unknown())) {
          count(mc_renamed_method, num_bytes);
          if (method->is_abstract()) {
            count(mc_renamed_abstract_method, num_bytes);
          }
        }
        TypeArray::Raw exception_table = method->exception_table();
        if (exception_table().length() > 0) {
          int bytes = exception_table().length() * 2 + 8;
          count(mc_exception_table, bytes);
        }
#if ENABLE_ROM_JAVA_DEBUGGER
        LineVarTable::Raw lvt = method->line_var_table();
        LineNumberTable::Raw lnt;
        if (!lvt.is_null()) {
          lnt = lvt().line_number_table();
        }
        if (!lvt.is_null() && !lnt.is_null() && lnt().count() > 0) {
          int bytes = lnt().length() * (lnt().is_compressed() ?
                                        sizeof(jubyte) : sizeof(jshort));
          count(mc_line_number_tables, bytes);
        }
#endif
      }
      break;

    case InstanceSize::size_compiled_method:
      count(mc_compiled_method, num_bytes);
      break;

    case InstanceSize::size_constant_pool:
      count(mc_constant_pool, num_bytes);
      break;

    case InstanceSize::size_obj_array_class:
    case InstanceSize::size_type_array_class:
      count(mc_array_class, num_bytes);
      break;

    case InstanceSize::size_class_info:
      {
        ClassInfo::Raw info = object;
        count(mc_class_info, num_bytes);

        if (info().vtable_length() > 0) {
          count(mc_vtable, info().vtable_length() * sizeof(jobject));
        }
        if (info().itable_length() > 0) {
          count(mc_itable, info().itable_size());
        }
      }
      break;

    case InstanceSize::size_stackmap_list:
      {
        count(mc_stackmap, num_bytes);
        StackmapList entry = object;
        int entry_count = entry.entry_count();
        for (int i = 0; i < entry_count; i++) {
          if(!entry.is_short_map(i)) {
            TypeArray longmap = entry.get_long_map(i);
            SETUP_ERROR_CHECKER_ARG;
            int skip = writer()->skip_words_of(&longmap JVM_NO_CHECK);
            GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION, "sanity");
            count(mc_longmaps, longmap.object_size() - skip* sizeof(int));
          }
        }
      }
      break;

    case InstanceSize::size_far_class:
      count(mc_meta, num_bytes);
      break;

    case InstanceSize::size_instance_class: {
        count(mc_instance_class, num_bytes);
        InstanceClass *ic = (InstanceClass *)object;
        if (!ic->is_fake_class() && ic->is_initialized()) {
          count(mc_inited_class, num_bytes);
        }
        Symbol name = ic->name();
        if (name.equals(Symbols::unknown())) {
          count(mc_renamed_class, num_bytes);
        }
        if (ic->has_embedded_static_oops()) {
          // Classes with embedded oop static fields must live in heap so that
          // write barriers by byte codes can be done efficiently
          GUARANTEE(_current_type == ROMWriter::HEAP_BLOCK,
                   "Classes with embedded static oop fields must live in heap");
        }
        if (ic->static_field_size() > 0) {
          count(mc_static_fields, ic->static_field_size());
        }
      }
      break;
#if ENABLE_ISOLATES
    case InstanceSize::size_task_mirror:
      { 
        count(mc_task_mirror, num_bytes);
        break;
      }
#endif
    default:
      if (object->is_string()) {
        count(mc_string, num_bytes);
      } else {
        count(mc_other, num_bytes);
      }
      break;
  }

  count(mc_total, num_bytes);
}

void SourceObjectWriter::count(MemCounter& counter, int bytes) {
  switch (_current_type) {
  case ROMWriter::TEXT_BLOCK:
    counter.add_text(bytes);
    break;
  case ROMWriter::DATA_BLOCK:
    counter.add_data(bytes);
    break;
  case ROMWriter::HEAP_BLOCK:
    counter.add_heap(bytes);
    break;
#if ENABLE_PREINITED_TASK_MIRRORS && USE_SOURCE_IMAGE_GENERATOR && ENABLE_ISOLATES 
  case ROMWriter::TASK_MIRRORS_BLOCK:
     counter.add_tm(bytes);
     break;
#endif  
  default:
      SHOULD_NOT_REACH_HERE();
  }
}

bool SourceObjectWriter::is_subtype(ROMWriter::BlockType type_to_check, ROMWriter::BlockType type) {
  
  if (type_to_check == type) {
    return true;
  } 
#if ENABLE_HEAP_NEARS_IN_HEAP  
  else if (type_to_check == ROMWriter::TEXT_AND_HEAP_BLOCK ) {
    return (type == ROMWriter::TEXT_BLOCK || type == ROMWriter::HEAP_BLOCK);
  } else if (type_to_check == ROMWriter::DATA_AND_HEAP_BLOCK ) {
    return (type == ROMWriter::DATA_BLOCK || type == ROMWriter::HEAP_BLOCK);
  }
#endif
  return false;
}

#endif //USE_SOURCE_IMAGE_GENERATOR
