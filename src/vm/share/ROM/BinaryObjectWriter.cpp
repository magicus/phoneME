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
#include "incls/_BinaryObjectWriter.cpp.incl"


#if ENABLE_ROM_GENERATOR

void BinaryObjectWriter::start_block(ROMWriter::BlockType type,
                                     int preset_count JVM_TRAPS) {
  _current_type = type;
  _offset = 0;
  (void)preset_count;

  switch (_current_type) {
  case ROMWriter::TEXT_BLOCK:
    _method_variable_parts =
      Universe::new_obj_array(_writer->count_methods() JVM_CHECK);
    break;
  case ROMWriter::DATA_BLOCK:
  case ROMWriter::HEAP_BLOCK:
  case ROMWriter::PERSISTENT_HANDLES_BLOCK:
  case ROMWriter::SYSTEM_SYMBOLS_BLOCK:
    break;
  default:
    SHOULD_NOT_REACH_HERE();
  }
}

void BinaryObjectWriter::begin_object(Oop *object JVM_TRAPS) {
  (void)object;
#if !defined(PRODUCT) || ENABLE_MONET_DEBUG_DUMP
  const ROMWriter::BlockType type = writer()->block_type_of(object JVM_CHECK);

  GUARANTEE(type == _current_type, "sanity");
  GUARANTEE(!ROM::system_contains(object->obj()), "sanity");

  int skip_words = writer()->skip_words_of(object JVM_CHECK);
  int skip_bytes = sizeof(jobject) * skip_words;

  count(object, -skip_bytes);
#endif

#if ENABLE_MONET_DEBUG_DUMP
  bool dumpit = true;
  if (ROM::system_text_contains(object->obj())) {
    // IMPL_NOTE: why are we here anyway?
    dumpit = false;
  }
  if (type == ROMWriter::HEAP_BLOCK &&
      object->obj() <= ROM::romized_heap_marker()) {
    dumpit = false;
  }
  if (dumpit) {
    dump_object(object, skip_words);
  }
#endif

#ifdef AZZERT
  int my_calculated_offset = writer()->offset_of(object JVM_CHECK);
  int real_offset = _offset - skip_bytes;
  GUARANTEE(my_calculated_offset == real_offset,
            "calculated address != real address");
#endif
}

void BinaryObjectWriter::end_object(Oop *object JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  (void)object;
}

void BinaryObjectWriter::put_reference(Oop *owner, int offset, Oop *object
                                       JVM_TRAPS) {
  (void)offset;
  (void)owner;
#ifdef AZZERT
  if (owner->not_null()) {
    ROMWriter::BlockType owner_type = writer()->block_type_of(owner JVM_CHECK);
    GUARANTEE(owner_type == _current_type, "sanity");
  }
  GUARANTEE(ROMWriter::write_by_value(owner), "sanity");
#endif

  if (object->is_null()) {
    writer()->writebinary_null();
  } else {
    ROMizerHashEntry::Raw entry = writer()->info_for(object JVM_CHECK);
    int oop_offset = entry().offset();
    ROMWriter::BlockType type = (ROMWriter::BlockType)entry().type();

    switch (type) {
      case ROMWriter::TEXT_BLOCK: {
        if (!ROM::system_text_contains(object->obj())) {
#if ENABLE_LIB_IMAGES
          if (ROM::in_any_loaded_bundle_of_current_task(object->obj())) {
            writer()->writebinary_lib_int_ref(
                 Task::current()->encode_reference((int)object->obj()));
          } else {
          // A reference to another objects in the binary ROM TEXT block
          GUARANTEE(oop_offset != -1, "Offset not set");
          writer()->writebinary_int_ref(writer()->binary_text_block_addr() +
                                        oop_offset);
          }          
#else
          // A reference to another objects in the binary ROM TEXT block
          GUARANTEE(oop_offset != -1, "Offset not set");
          writer()->writebinary_int_ref(writer()->binary_text_block_addr() +
                                        oop_offset);
#endif
        } else {
          // A reference to the system ROM TEXT block
          writer()->writebinary_const_int_ref(object->obj());
        }
      }
      break;

    case ROMWriter::DATA_BLOCK: {
        GUARANTEE(ROM::system_data_contains(object->obj()), 
                  "Binary ROM contains no DATA objects");
        writer()->writebinary_const_int_ref(object->obj());
      }
      break;

    case ROMWriter::HEAP_BLOCK: {
        if (object->obj() > ROM::romized_heap_marker()) {
          // A reference to another objects in the binary ROM HEAP block
          GUARANTEE(oop_offset != -1, "Offset not set");
          writer()->writebinary_int_ref(writer()->binary_heap_block_addr() +
                                        oop_offset);
        } else {
#ifndef PRODUCT
          if (Verbose) {
            tty->print("Heap ref to: 0x%x ",
                       ((int)object->obj() - (int)_heap_start));
            object->print_value_on(tty);
            tty->print_cr("");
          }
#endif
          if (object->equals(Universe::empty_obj_array())) {
            int encoded_value = (int)Universe::rom_text_empty_obj_array()->obj();
            GUARANTEE(ROM::system_text_contains((OopDesc*)encoded_value), "sanity");
            writer()->writebinary_const_int_ref((OopDesc*)encoded_value);
          } else { 
            GUARANTEE(_current_type != ROMWriter::TEXT_BLOCK,
              "Heap references are not allowed in TEXT block");
            writer()->writebinary_int_ref(ROM::encode_heap_reference(object));
          }
        }
      }
      break;

    default:
      SHOULD_NOT_REACH_HERE();
    }
    writer()->set_eol_comment_object(object);
  }
  _offset += sizeof(int);
}

void BinaryObjectWriter::put_int(Oop *owner, jint value JVM_TRAPS) {
#ifdef AZZERT
  ROMWriter::BlockType owner_type = writer()->block_type_of(owner JVM_CHECK);
  GUARANTEE(owner_type == _current_type, "sanity");
#else
  JVM_IGNORE_TRAPS;
  (void)owner;
#endif

  writer()->writebinary_int(value);
  _offset += sizeof(int);
}

void BinaryObjectWriter::put_int_by_mask(Oop *owner, jint be_word, 
                                         jint le_word,
                                         jint typemask JVM_TRAPS) {
  // This is not done in BinaryObjectWriter since there's no cross-endian
  // issue.
  (void)owner;
  (void)be_word;
  (void)le_word;
  (void)typemask;
  JVM_IGNORE_TRAPS;
  SHOULD_NOT_REACH_HERE();
}

void BinaryObjectWriter::put_long(Oop *owner, jint msw, jint lsw JVM_TRAPS) {
  // This is not done in BinaryObjectWriter since there's no cross-endian
  // issue.
  (void)owner;
  (void)msw;
  (void)lsw;
  JVM_IGNORE_TRAPS;
  SHOULD_NOT_REACH_HERE();
}

void BinaryObjectWriter::put_double(Oop *owner, jint msw, jint lsw JVM_TRAPS) {
  // This is not done in BinaryObjectWriter since there's no cross-endian
  // issue.
  (void)owner;
  (void)msw;
  (void)lsw;
  JVM_IGNORE_TRAPS;
  SHOULD_NOT_REACH_HERE();
}

void BinaryObjectWriter::put_symbolic(Oop *owner, int offset JVM_TRAPS) {
#ifdef AZZERT
  ROMWriter::BlockType owner_type = writer()->block_type_of(owner JVM_CHECK);
  GUARANTEE(owner_type == _current_type, "sanity");
#endif

  FarClass blueprint = owner->blueprint();
  InstanceSize instance_size = blueprint.instance_size();
  switch (instance_size.value()) {
  case InstanceSize::size_method:
    put_method_symbolic((Method*)owner, offset JVM_CHECK);
    break;

  case InstanceSize::size_obj_array_class:
  case InstanceSize::size_type_array_class:
  case InstanceSize::size_far_class:
    writer()->writebinary_int(owner->int_field(offset));
    break;

  default:
    SHOULD_NOT_REACH_HERE();
  }

  _offset += sizeof(int);
}

void BinaryObjectWriter::put_method_symbolic(Method *method, int offset 
                                             JVM_TRAPS) {
  GUARANTEE(!method->is_quick_native(), 
            "loaded classes cannot have quick natives");

  if ((method->is_native() || method->is_abstract()) &&
    offset == Method::native_code_offset()) {
    writer()->writebinary_symbolic((address)method->int_field(offset));
  } else if (offset == Method::variable_part_offset()) {
    put_method_variable_part(method JVM_CHECK);
  } else if (offset == Method::heap_execution_entry_offset()) {
#if USE_AOT_COMPILATION
    if (method->has_compiled_code()) {
      CompiledMethod cm = method->compiled_code();
      writer()->writebinary_compiled_code_reference(&cm JVM_CHECK);
    } else 
#endif
    {
      writer()->writebinary_symbolic((address)method->int_field(offset));
    }
  } else {
    // We're writing the bytecodes
    writer()->writebinary_int(method->int_field(offset));
  }
}

void BinaryObjectWriter::put_method_variable_part(Method *method JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  // Note: if you want to put method in heap, you have to add code to
  // relocate its variable_part
  GUARANTEE(_current_type != ROMWriter::HEAP_BLOCK,"can't put method in heap");

  if (has_split_variable_part(method)) {
    _method_variable_parts.obj_at_put(_variable_parts_offset, method);
    int vpart_start_offset = writer()->binary_method_variable_parts_addr();
    int vpart_offset = vpart_start_offset + _variable_parts_offset*sizeof(int);
    writer()->writebinary_int_ref(vpart_offset);
    _variable_parts_offset ++;
  } else { 
    const int delta = 
        Method::heap_execution_entry_offset() - Method::variable_part_offset();

    int ref = _offset + delta;
    switch (_current_type) {
      case ROMWriter::TEXT_BLOCK:
        ref += writer()->binary_text_block_addr();
        break;
      case ROMWriter::HEAP_BLOCK:                 
        ref += writer()->binary_heap_block_addr();
        break;
      case ROMWriter::DATA_BLOCK:
      default:
        SHOULD_NOT_REACH_HERE();
    }
    writer()->writebinary_int_ref(ref);
  }
}

bool BinaryObjectWriter::has_split_variable_part(Method *method) {
#if USE_IMAGE_MAPPING
#if USE_AOT_COMPILATION
  if (GenerateROMImage && method->has_compiled_code()) {
    return false;
  }
#endif

  // The method may be in a read-only mmap'ed region, so we must put the
  // variable part in a separate, writeable block
  return !method->is_impossible_to_compile();
#else
  (void)method;
  // The method is either in the preloaded heap region, or in a LargeObject,
  // so it's always writeable. No need to split variable part
  return false;
#endif
}


void BinaryObjectWriter::print_method_variable_parts(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  int variable_parts_count = _variable_parts_offset;
  GUARANTEE(variable_parts_count == writer()->variable_parts_count(),"sanity");

  for (int i=0; i<variable_parts_count; i++) {
    Method::Raw method = _method_variable_parts.obj_at(i);
    jint addr = method().int_field(Method::heap_execution_entry_offset());
    writer()->writebinary_symbolic((address)addr);
  }
}

#if USE_ROM_LOGGING
void BinaryObjectWriter::count(Oop *object, int adjustment) {
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
        if (!ic->is_romized() && ic->is_initialized()) {
          count(mc_inited_class, num_bytes);
        }
        Symbol name = ic->name();
        if (name.equals(Symbols::unknown())) {
          count(mc_renamed_class, num_bytes);
        }
        if (ic->static_field_size() > 0) {
          // Classes with static fields must live in heap so that
          // write barriers by byte codes can be done efficiently
          GUARANTEE(_current_type == ROMWriter::HEAP_BLOCK,
                    "Classes with static fields must live in heap");
          count(mc_static_fields, ic->static_field_size());
        }
      }
      break;
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

void BinaryObjectWriter::count(MemCounter& counter, int bytes) {
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
  default:
      SHOULD_NOT_REACH_HERE();
  }
}
#endif // USE_ROM_LOGGING

#if ENABLE_MONET_DEBUG_DUMP
void BinaryObjectWriter::dump_object(Oop *object, int skip_words) {
  UsingFastOops fast_oops;
  FarClass::Fast blueprint = object->blueprint();
  InstanceSize i = blueprint().instance_size();

  writer()->flush_eol_comment();
  Stream *st = &writer()->_dump_stream;
  switch (i.value()) {
  case InstanceSize::size_obj_array:         
    st->print("obj_array");        break;
  case InstanceSize::size_type_array_1:
    st->print("type_array_1");     break;
  case InstanceSize::size_type_array_2:  
    st->print("type_array_2");     break;
  case InstanceSize::size_type_array_4:
    st->print("type_array_4");     break;
  case InstanceSize::size_type_array_8:
    st->print("type_array_8");     break;
  case InstanceSize::size_instance_class:
#ifndef PRODUCT
    ((InstanceClass*)object)->print_value_on(st);
#else
    st->print("instance_class");
#endif
    break;
  case InstanceSize::size_obj_array_class:  
    st->print("obj_array_class");  break;
  case InstanceSize::size_type_array_class:
    st->print("type_array_class"); break;
  case InstanceSize::size_generic_near:
    st->print("type_array_class"); break;
  case InstanceSize::size_java_near:
#ifndef PRODUCT
    ((JavaNear*)object)->print_value_on(st);
#else
    st->print("java_near");
#endif
    break;
  case InstanceSize::size_obj_near:          
    st->print("obj_array_class");  break;
  case InstanceSize::size_far_class:
    st->print("far_class");        break;
  case InstanceSize::size_mixed_oop:
    st->print("mixed_oop");        break;
  case InstanceSize::size_task_mirror:
    st->print("task_mirror");      break;
  case InstanceSize::size_boundary:          
    st->print("boundary");         break;
  case InstanceSize::size_entry_activation:
    st->print("entry_activation"); break;
  case InstanceSize::size_execution_stack: 
    st->print("execution_stack");  break;
  case InstanceSize::size_constant_pool:
    st->print("constant_pool");    break;
  case InstanceSize::size_class_info: 
    st->print("class_info");       break;
  case InstanceSize::size_compiled_method:   
    st->print("compiled_method");  break;
  case InstanceSize::size_stackmap_list:
    st->print("stackmap_list");    break;
  case InstanceSize::size_refnode:           
    st->print("refnode");          break;
  case InstanceSize::size_symbol:
    {
      st->print("symbol: #");
      Symbol::Raw symbol = object->obj();
      symbol().print_symbol_on(st, true);
    }
    break;
  case InstanceSize::size_method:
    {
      st->print("method ");
      UsingFastOops fast_oops_inside;
      Method::Fast method = object->obj();
      InstanceClass::Fast ic = method().holder();
      Symbol::Fast symbol = ic().name();
      symbol().print_symbol_on(st, true);
      st->print(".");
      symbol = method().name();
      symbol().print_symbol_on(st, true);
    }
    break;

  default:
    {
      GUARANTEE(i.value() > 0, "sanity");
      if (blueprint.equals(Universe::string_class())) {
        st->print("String \"");
        String::Raw str = object->obj();
        str().print_string_on(st);
        st->print("\"");
      } else {
        st->print("java_object");
      }
    }
    break;
  }
  int word_size = object->object_size() / 4;
  st->print(" %d ", word_size);
  if (skip_words > 0) {
    st->print("- %d = %d ", skip_words, word_size - skip_words);
  }
  st->print("words");
}
#endif
#endif //ENABLE_ROM_GENERATOR
