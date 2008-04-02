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

/**
 * This file contains C++ routines that are most frequently used
 * during compilation and GC. On low-end devices (such as ARM7TDMI),
 * you should compile this file in ARM mode and place it into
 * fast memory.
 */


#ifdef USE_HOT_ROUTINES
// This file is compiled only if USE_HOT_ROUTINES is defined 
// (inside HotRoutines0.cpp or HotRoutines1.cpp). Since this file contains
// function/variable definitions, there must be no effect if this file is
// accidentally included by other .cpp files
//
//
// The contents of this file are ordered roughly by importance. If you don't
// have enough fast memory, change the "#if USE_HOT_ROUTINES" to
// "#if !USE_HOT_ROUTINES" at each function that you want to exclude from
// fast memory.

#if USE_HOT_ROUTINES
_KNI_HandleInfo*  last_kni_handle_info = NULL;
int               _rom_text_block_size_fast = 0;
int               _rom_data_block_size_fast = 0;

int       Verifier::_stackmap_cache_max;

ObjectHeap::QuickVars ObjectHeap::_quick_vars;

#if ENABLE_COMPILER
#if ENABLE_INTERNAL_CODE_OPTIMIZER
InternalCodeOptimizer* InternalCodeOptimizer::_current = NULL;
int InternalCodeOptimizer::_start_code_offset = 0;
CompiledMethod* InternalCodeOptimizer::_start_method = NULL;
int OptimizerInstruction::latency_dty[] ={ 0, 1, 0, 3, 1, 2, 2, 5, 3, 
    1, 0, 0, 0}; 
#endif 
#if ENABLE_CSE
jint VirtualStackFrame::_pop_bci = -1;
jint VirtualStackFrame::_cse_tag = 0;
bool VirtualStackFrame::_abort = false;
jint VirtualStackFrame::_passable_entry = 0;
jint RegisterAllocator::_notation_map = 0;
jint RegisterAllocator::_status_checked = 0;
RegisterNotation RegisterAllocator::_register_notation_table[Assembler::number_of_registers] 
  = {};
#endif


int RegisterAllocator::_register_references[Assembler::number_of_registers]
    = {0, };
#endif

#endif // USE_HOT_ROUTINES



#if USE_HOT_ROUTINES
// Write barrier for individual pointer store.
void oop_write_barrier(OopDesc** addr, OopDesc* value) {
  // prefetch _heap_start and _heap_top to avoid stalls on ARM
  OopDesc ** heap_start = _heap_start;
  OopDesc ** old_generation_end = _old_generation_end;
  *addr = value;

  // Note the order of the comparison. In most cases the first comparison 
  // will fail because addr is in the young space
  if (addr < old_generation_end && ((OopDesc*)addr) < value && heap_start <= addr) {
    ObjectHeap::set_bit_for(addr);
    GUARANTEE(ObjectHeap::test_bit_for(addr), "sanity check");
  }
}
#endif


#if USE_HOT_ROUTINES
int Bytecodes::length_for(const Method* method, const int bci) {
  const Code code = method->bytecode_at(bci);
  check(code);
  const int size = length_for(code);
  return size ? size : wide_length_for(method, bci, code);
}
#endif


#if USE_HOT_ROUTINES
jushort ClassFileParser::get_u2(JVM_SINGLE_ARG_TRAPS) {
  jubyte *ptr = _bufptr;
  jubyte *end = _bufend;
  jushort val = Bytes::get_Java_u2((address)ptr);
  ptr += 2;
  _bufptr = ptr;

  if (ptr <= end) {
    return val;
  } else {
    Throw::class_format_error(truncated_class_file JVM_NO_CHECK_AT_BOTTOM);
    return 0;
  }
}
#endif



#if USE_HOT_ROUTINES
#if ENABLE_COMPILER
void RawLocation::read_value(Value& v, int index) {
  AllocationDisabler shouldnt_allocate_in_this_function;

  v.destroy();

  if (is_flushed()) {
    Compiler::code_generator()->load_from_location(v, index);
    v.set_flags(flags());
    v.set_length(length());
#if ENABLE_COMPILER_TYPE_INFO
    v.set_class_id(class_id());
#endif
    write_value(v);
    mark_as_cached();
  } else { 
    v.set_where(where());
    v.set_flags(flags());
    v.set_length(length());
#if ENABLE_COMPILER_TYPE_INFO
    v.set_class_id(class_id());
#endif

    GUARANTEE(type() == v.stack_type(), "Types must match");
    v.set_low_word(value());

    if (v.is_two_word()) {
      RawLocation *next_location = this + 1;
      GUARANTEE(next_location->type() == T_ILLEGAL,
                "The type of the high word must be T_ILLEGAL");
      v.set_high_word(next_location->value());
    }

    if (v.in_register()) {
      RegisterAllocator::reference(v.lo_register());
      if (v.use_two_registers()) {
          RegisterAllocator::reference(v.hi_register());
      }
    }
  }
}
#endif // ENABLE_COMPILER
#endif



#if USE_HOT_ROUTINES
// return the CRC for a given array of data
juint Inflater::crc32(const unsigned char *data, const juint length) {
  juint crc = 0xFFFFFFFF;
  const unsigned char* end = data + length;

#if ENABLE_FAST_CRC32
  const juint *table = _fast_crc32_table;

  switch (length & 3) {
    do {
      crc = table[ ( crc ^ *data++ ) & 0xFF ] ^ (crc >> 8);
    case 3:
      crc = table[ ( crc ^ *data++ ) & 0xFF ] ^ (crc >> 8);
    case 2:
      crc = table[ ( crc ^ *data++ ) & 0xFF ] ^ (crc >> 8);
    case 1:
      crc = table[ ( crc ^ *data++ ) & 0xFF ] ^ (crc >> 8);
    default:;
    } while( data < end );
  }

  GUARANTEE(data == end, "loop must be unrolled correctly");

#else
  unsigned int j;

  for ( ; data < end; data++) {
    crc ^= *data;
    for (j = 8; j > 0; --j) {
      crc = (crc & 1) ? ((crc >> 1) ^ 0xedb88320) : (crc >> 1);
    }
  }
#endif

  return ~crc;
}
#endif



#if USE_HOT_ROUTINES
int VerifierFrame::get_stackmap_index_for_offset(int target_bci) {
  int len = stackmaps()->length();
  address *p = (address*)stackmaps()->base_address();
  int stackmap_index = 0;

  // WAS: for (stackmap_index = 0; stackmap_index < len; stackmap_index+=2)
  while (stackmap_index < len) {
    address scalars = *p;

    // WAS: stackmap_scalars = stackmaps()->obj_at(stackmap_index);
    // WAS: if (target_bci == stackmap_scalars().int_at(0)) {
    // WAS:   return stackmap_index;
    // WAS: }

    scalars += Array::base_offset();
    if (target_bci == ((jint*)scalars)[0]) {
      return stackmap_index;
    }
    p += 2;
    stackmap_index += 2;
  }

  return -1;
}
#endif



#if USE_HOT_ROUTINES
int Field::find_field_index(InstanceClass* ic, Symbol* name, Symbol* signature)
{
  AllocationDisabler shouldnt_allocate_in_this_function;

  ConstantPool::Raw cp = get_constants_for(ic);
  TypeArray::Raw fields = get_fields_for(ic);
  int fields_length = fields().length();

  OopDesc *name_obj = name->obj();
  OopDesc *sig_obj  = signature->obj();

  address field_base = fields().base_address();
  address cp_base = ((address)cp.obj()) + ConstantPool::base_offset();

  for (int index = 0; index < fields_length; index += 5) {
    int name_index      = ((jushort*)field_base)[NAME_OFFSET];
    int signature_index = ((jushort*)field_base)[SIGNATURE_OFFSET];

    OopDesc *n = ((OopDesc**)cp_base)[name_index];
    OopDesc *s = ((OopDesc**)cp_base)[signature_index];

    if (n == name_obj && s == sig_obj) {
      return index;
    }

    field_base += 5 * sizeof(jushort);
  }
  return -1;
}
#endif



#if USE_HOT_ROUTINES
ConstantTag ConstantPool::tag_at(int index) const  {
  jubyte* ptr = (jubyte*)tags();
  ptr += Array::base_offset();
  return ConstantTag(ptr[index]);
}
#endif

#if USE_HOT_ROUTINES
void ObjectHeap::do_nothing(OopDesc**) {
}
#endif

#if USE_HOT_ROUTINES
void ObjectHeap::mark_pointer_to_young_generation(OopDesc** p) {
  if( p < _collection_area_start ) {
    OopDesc** const obj = (OopDesc**)*p;
    if( _collection_area_start <= obj && obj < _inline_allocation_top ) {
      set_bit_for( p );
    }
  }
}
#endif

#if USE_HOT_ROUTINES
void ObjectHeap::mark_and_push(OopDesc** p) {
  OopDesc* obj = *p;
  // Is object pointed to in collection target area? This does null check as
  // well. 
  if (_collection_area_start <= (OopDesc**)obj &&
      (OopDesc**)obj < mark_area_end() ) { // _heap_top for separate compiler area
    // Is object already marked?
    if (!test_and_set_bit_for((OopDesc**) obj)) {
#if ENABLE_REMOTE_TRACER
      if (RemoteTracePort > 0) {
        RemoteTracer::update_stats(obj);
      }
#endif
      // Object now marked, push on marking stack
      if (_marking_stack_top == _marking_stack_end) {
        _marking_stack_overflow = true;
        if (TraceGC) {
          TTY_TRACE_CR(("TraceGC: 0x%x marked, overflow", obj));
        }
      } else {
        *_marking_stack_top++ = obj;
        if (TraceGC) {
          TTY_TRACE_CR(("TraceGC: 0x%x marked", obj));
        }
      }
    }
  }
#ifdef AZZERT
  else {
    GUARANTEE( !in_dead_bundles( (OopDesc*) obj ),
               "Unexpected reference to dead bundle" );
  }
#endif
}
#endif

#if USE_HOT_ROUTINES
void ObjectHeap::continue_marking(void) {
  // Cache in local registers
  OopDesc** const collection_area_start = _collection_area_start;
  OopDesc** const heap_top              = mark_area_end();
  OopDesc** const marking_stack_end     = _marking_stack_end;
  address   const bitvector_base        = _bitvector_base;

  while (_marking_stack_top > _marking_stack_start) {
    // Pop top marking stack element
    OopDesc* obj = *--_marking_stack_top;
    GUARANTEE(test_bit_for((OopDesc**) obj), "Pushed objects should be marked");
    // Follow near pointer
    mark_and_push(&(obj->_klass));

    FarClassDesc* const blueprint = obj->blueprint();
    if (blueprint->instance_size_as_jint() > 0) {
      // This is a common case: (non-array) Java object instance. In-line
      // OopDesc::oops_do_for() to make it run faster.
      const jbyte* map = (jbyte*)blueprint->embedded_oop_map();
      OopDesc** p = (OopDesc**)obj;
      for (;;) {
        const jint entry = (jint)(*map++);
        if (entry > 0) {
          p += entry;
          OopDesc** const o = (OopDesc**)*p;
          // Is object pointed to in collection target area? This
          // does null check as well. 
          if (collection_area_start <= o && o < heap_top) {
            // Is object already marked?
            if (!test_and_set_bit_for(o, bitvector_base)) {
              // Object now marked, push on marking stack
              if (_marking_stack_top == marking_stack_end) {
                _marking_stack_overflow = true;
                if (TraceGC) {
                  TTY_TRACE_CR(("TraceGC: 0x%x marked, overflow", o));
                }
              } else {
                *_marking_stack_top++ = (OopDesc*)o;
                if (TraceGC) {
                  TTY_TRACE_CR(("TraceGC: obj 0x%x, 0x%x marked", obj, o));
                }
              }
            }
          }
#ifdef AZZERT
          else {
            GUARANTEE( !in_dead_bundles( (OopDesc*) obj ),
                       "Unexpected reference to dead bundle" );
          }
#endif
        } else if (entry == 0) {
          break;
        } else {
          GUARANTEE((entry & 0xff) == OopMapEscape, "sanity")
          p += (OopMapEscape - 1);
        }
      }
    } else {
      if (TraceGC) {
        TTY_TRACE_CR(("TraceGC: obj 0x%x, oops_do_for", obj));
#ifndef PRODUCT
        Oop *q =(Oop*)&obj;
        if (!q->is_string() && !q->is_short_array() &&
            !q->is_byte_array()  && !q->is_symbol() &&
            !q->is_char_array()) {
          q->print_value_on(tty);
          tty->cr();
        }
#endif
      }
      obj->oops_do_for(blueprint, mark_and_push);
    }
  }
}
#endif


#if USE_HOT_ROUTINES
void ObjectHeap::mark_root_and_stack(OopDesc** p) {
  OopDesc** const obj = (OopDesc**) *p;
  if( _collection_area_start <= obj && obj < mark_area_end()
      && !test_and_set_bit_for(obj) ) {
#if ENABLE_REMOTE_TRACER
    if (RemoteTracePort > 0) {
      RemoteTracer::update_stats((OopDesc*)obj);
    }
#endif
    // No marking stack overflow is possible here
    *_marking_stack_top++ = (OopDesc*)obj;
    if (TraceGC) {
      TTY_TRACE_CR(("TraceGC: 0x%x marked", obj));
    }
    continue_marking();
  }
#ifdef AZZERT
  else {
    GUARANTEE( !in_dead_bundles( (OopDesc*) obj ),
               "Unexpected reference to dead bundle" );
  }
#endif
}
#endif

#if USE_HOT_ROUTINES
void
OopDesc::oops_do_for(const FarClassDesc* blueprint, void do_oop(OopDesc**)) {
  jint instance_size = blueprint->instance_size_as_jint();
  switch(instance_size) {
  default:
    GUARANTEE(instance_size > 0, "bad instance size");
    map_oops_do(blueprint->embedded_oop_map(), do_oop);
    return;
    
  case InstanceSize::size_type_array_1:
  case InstanceSize::size_type_array_2:
  case InstanceSize::size_type_array_4:
  case InstanceSize::size_type_array_8:
  case InstanceSize::size_generic_near:
  case InstanceSize::size_symbol:
    // These have no embedded pointers
    GUARANTEE(blueprint->extern_oop_map()[0] == OopMapSentinel, 
              "No fixed pointers");
    return;
    
  case InstanceSize::size_far_class:
  case InstanceSize::size_obj_near: 
  case InstanceSize::size_java_near:
  case InstanceSize::size_boundary:
  case InstanceSize::size_method:
    // The only pointers in these are fixed
    break;

  case InstanceSize::size_obj_array_class:
  case InstanceSize::size_type_array_class: 
    // Because these have a fixed vtable size, their pointers are fixed
    break;
    
  case InstanceSize::size_execution_stack:
    ((ExecutionStackDesc*) this)->variable_oops_do(do_oop); 
    GUARANTEE(blueprint->extern_oop_map()[0] == OopMapSentinel, 
              "No fixed pointers");
    return;

  case InstanceSize::size_obj_array:
    ((ObjArrayDesc*) this)->variable_oops_do(do_oop); 
    GUARANTEE(blueprint->extern_oop_map()[0] == OopMapSentinel, 
              "No fixed pointers");
    return;

  case InstanceSize::size_refnode:
#if ENABLE_JAVA_DEBUGGER
    ((RefNodeDesc *) this)->variable_oops_do(do_oop); 
    GUARANTEE(blueprint->extern_oop_map()[0] == OopMapSentinel, 
              "No fixed pointers");
#endif
    return;

  case InstanceSize::size_mixed_oop:
    ((MixedOopDesc*) this)->variable_oops_do(do_oop); 
    break;
#if USE_COMPILER_STRUCTURES
  case InstanceSize::size_compiled_method:
    ((CompiledMethodDesc*) this)->variable_oops_do(do_oop); 
    break;
#endif
  case InstanceSize::size_constant_pool: 
    ((ConstantPoolDesc*) this)->variable_oops_do(do_oop);  
    break;
  case InstanceSize::size_entry_activation:  
    ((EntryActivationDesc*)this)->variable_oops_do(do_oop);
    break;
  case InstanceSize::size_instance_class:
    ((InstanceClassDesc*) this)->variable_oops_do(do_oop);
    break;
  case InstanceSize::size_class_info:
    ((ClassInfoDesc*) this)->variable_oops_do(do_oop);  
    break;
  case InstanceSize::size_stackmap_list:
    ((StackmapListDesc*) this)->variable_oops_do(do_oop);  
    break;
#if ENABLE_ISOLATES
  case InstanceSize::size_task_mirror:
    ((TaskMirrorDesc*) this)->variable_oops_do(do_oop);  
    break;
#endif
  } 
  map_oops_do(blueprint->extern_oop_map(), do_oop);
}
#endif



#if USE_HOT_ROUTINES
#if ENABLE_COMPILER
void RawLocation::write_value(const Value& v) {
  set_where(v.where());
  set_type(v.stack_type());
  set_flags(v.flags());
  set_length(v.length());
#if ENABLE_COMPILER_TYPE_INFO
  set_class_id(v.class_id());
#endif
  set_value(v.low_word());

  mark_as_changed();

  if (v.is_two_word()) {
    RawLocation *next_location = this + 1;

    next_location->mark_as_illegal();
    next_location->set_value(v.high_word());
  }
}
#endif // ENABLE_COMPILER
#endif



#if USE_HOT_ROUTINES
// This function is frequently called during class loading and verification.
// We hand-code all the checkings to get max performance. Otherwise,
// if we call into functions like ConstantPool::name_and_type_ref_index_at(),
// etc, many of the bounds check would be duplicated, and we would
// have too many function calls.
//
// tag[index]   = a MethodRef, InterfaceMethodRef or FieldRef
// value[index] = [high: name_and_type_index, low: class_index]
//
void ConstantPool::resolve_helper_0(int index, Symbol* name, Symbol* signature,
                                    InstanceClass* klass, Symbol* klass_name
                                    JVM_TRAPS) {
  int value, name_and_type_value;
  jushort name_and_type_index, name_index, signature_index, class_index;
  jushort len = length();

  {
    AllocationDisabler shouldnt_allocate_in_this_block;
    TypeArray::Raw ta = tags();
    jubyte *tag = (jubyte*)ta().base_address();

    // (1) tag[index] must be MethodRef, InterfaceMethodRef or FieldRef
    if (!is_within_bounds(index, len) ||
        !ConstantTag::is_field_or_method(tag[index])) {
      goto error;
    }

    value = int_field(offset_from_index(index));
    name_and_type_index = extract_high_jushort_from_jint(value);
    class_index         = extract_low_jushort_from_jint (value);

    // tag[name_and_type_index] must be NameAndType
    if (!is_within_bounds(name_and_type_index, len) ||
        !ConstantTag::is_name_and_type(tag[name_and_type_index])) {
      goto error;
    }

    // tag[class_index] must be Class or UnresolvedClass
    if (!is_within_bounds(class_index, len) || 
        !ConstantTag::is_klass(tag[class_index])) {
      goto error;
    }

    name_and_type_value = int_field(offset_from_index(name_and_type_index));
    name_index      = extract_low_jushort_from_jint (name_and_type_value);
    signature_index = extract_high_jushort_from_jint(name_and_type_value);

    // tag[name_index] must be UTF8
    if (!is_within_bounds(name_index, len) ||
        !ConstantTag::is_utf8(tag[name_index])) {
      goto error;
    }
    // tag[signature_index] must be UTF8
    if (!is_within_bounds(signature_index, len) ||
        !ConstantTag::is_utf8(tag[signature_index])) {
      goto error;
    }

    *name = symbol_at(name_index);
    *signature = symbol_at(signature_index);
  }


  if (ConstantTag::is_unresolved_klass(tag_value_at(class_index))) {
    if (klass != NULL) {
      *klass = klass_at(class_index JVM_NO_CHECK_AT_BOTTOM);
    } else {
      *klass_name = unchecked_unresolved_klass_at(class_index);
    }
  } else {
    GUARANTEE(ConstantTag::is_resolved_klass(tag_value_at(class_index)), 
              "sanity");
    jint class_id = int_field(offset_from_index(class_index));
    JavaClass::Raw k = Universe::class_from_id(class_id);
    if (klass != NULL) {
      *klass = k.obj();
    } else {
      *klass_name = k().name();
    }
  }
  return;

error:
  Throw::error(invalid_constant JVM_NO_CHECK_AT_BOTTOM);
}
#endif



#if USE_HOT_ROUTINES
int ConstantPool::name_and_type_ref_index_at(int index JVM_TRAPS) {
  //WAS: jint ref_index = field_or_method_at(index JVM_ZCHECK(ref_index));
  TypeArray::Raw ta = tags();
  jubyte *tag_base = (jubyte*)ta().base_address();
  jint   *val_base = (jint*) ( (int)(obj()) + base_offset() );
  int len = length();
  jint ref_index, name_and_type_index;

  if ((juint)index >= (juint)len) {
    goto error;
  }
  ref_index = val_base[index];

  {
    ConstantTag tag1(tag_base[index]);
    if (!tag1.is_field_or_method()) {
      goto error;
    }
  }

  name_and_type_index = extract_high_jshort_from_jint(ref_index);

  //WAS: cp_check_0(is_within_bounds(name_and_type_index));
  //WAS: cp_check_0(tag_at(name_and_type_index).is_name_and_type());
  if ((juint)name_and_type_index >= (juint)len) {
    goto error;
  }

  {
    ConstantTag tag2(tag_base[name_and_type_index]);
    if (!tag2.is_name_and_type()) {
      goto error;
    }
  }

  GUARANTEE(name_and_type_index != 0, "sanity for JVM_ZCHECK");
  return name_and_type_index;

error:
  Throw::error(invalid_constant JVM_NO_CHECK_AT_BOTTOM);
  return 0;
}
#endif

#endif // defined( USE_HOT_ROUTINES)
