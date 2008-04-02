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

/**
   \class VerifyMethodCodes
   Used by the verifier to perform operations on Java bytecodes
   during bytecode verification.
  
   This VM has a two-phase class file verifier.  
   In order to run this VM, class files must 
   first be processed with a special "pre-verifier" tool. 
   This phase is typically done on the development workstation.  
   During execution, the runtime verifier (defined in 
   VerifyClassFile.cpp) performs the actual class file 
   verification based on both runtime information and 
   pre-verification information.
 */

#include "incls/_precompiled.incl"
#include "incls/_VerifyMethodCodes.cpp.incl"

class GetInstructionStarts { 
public:
  static ReturnOop run(Method *method JVM_TRAPS) {
    UsingFastOops fast_oops;
    TypeArray::Fast array;

    // (1) Allocate the array
    int len = method->code_size();
    if (len <= Verifier::INSTRUCTION_STARTS_CACHE_SIZE) {
      array = Universe::verifier_instruction_starts_cache()->obj();
      jvm_memset(array().base_address(), 0x0, sizeof(jbyte) * len);
    } else {
      array = Universe::new_byte_array(len JVM_CHECK_0);
    }

    {
      AllocationDisabler raw_pointers_used_in_this_block;

      // (2) Fast iterate over the bytecodes
      jbyte *array_base = (jbyte*)array().base_address();
      jubyte *bcptr = (jubyte*)method->code_base();
      int bci;

      // This is a hot loop used during verification. The conditions checked
      // inside this loop are carefully ordered so that most bytecodes can
      // be processed quickly.
      for (bci = 0; bci < len;) {
        Bytecodes::Code code = (Bytecodes::Code)(*bcptr);
        GUARANTEE(code >= 0, "sanity: unsigned value");

#if ENABLE_JAVA_DEBUGGER
        // handle breakpoint opcode
        if (code == Bytecodes::_breakpoint) {
          code = VMEvent::get_verifier_breakpoint_opcode(method, bci);
          GUARANTEE(code != Bytecodes::_illegal, "Cannot find original opcode");
        }
#endif

        array_base[bci] = 1;
        if (code == Bytecodes::_new) {
          array_base[bci] = (jbyte)(array_base[bci] | 2);
        }

        int simple_len = Bytecodes::length_for(code);
        if (simple_len > 0) {
          bci += simple_len;
          bcptr += simple_len;
        }
        else if (simple_len == 0) {
          int wide_len = Bytecodes::wide_length_for(method, bci, code);
          if (wide_len > 0) {
            bci += wide_len;
            bcptr += wide_len;
          } else {
            goto error;
          }
        } else {
          goto error;
        }
      }

      if (bci > len) {
        goto error;
      }
    }
    return array.obj();

  error:
    VFY_ERROR_0(ve_bad_instr);
  }
};

VerifyMethodCodes::VerifyMethodCodes() {
  // Do not remove: The existence of this destructor helps GCC 2.9 generate
  // smaller code
}

void VerifyMethodCodes::populate_stackmap_cache(Method *method) {
  ObjArray::Raw stackmaps = method->stackmaps();
  if (stackmaps.is_null()) {
    Verifier::_stackmap_cache_max = 0;
    return;
  }
  int max = method->code_size();
  if (max > Verifier::STACKMAP_CACHE_SIZE) {
      max = Verifier::STACKMAP_CACHE_SIZE;
  }

  TypeArray::Raw cache = Universe::verifier_stackmap_cache()->obj();
  jvm_memset(cache().base_address(), 0xff, sizeof(jint) * max);

  int i, len = stackmaps().length();
  TypeArray::Raw stackmap_scalars;
  for (i = 0; i < len; i += 2) {
    stackmap_scalars = stackmaps().obj_at(i);
    int target_bci = stackmap_scalars().int_at(0);
    if (target_bci < max) {
      if (cache().int_at(target_bci) != -1) {
        // oops - duplicated stack maps. This is a bad classfile. Don't
        // use the cache, and the error will eventually be sorted out
        max = 0;
        break;
      } else {
        cache().int_at_put(target_bci, i);
      }
    }
  }

  Verifier::_stackmap_cache_max = max;
}

void VerifyMethodCodes::verify(Method *method, ObjArray *stackmaps JVM_TRAPS) {
  BytecodeClosure::initialize(method);

  populate_stackmap_cache(method);
  check_not_final_override(method JVM_CHECK);

  {
    UsingFastOops fast_oops;
    TypeArray::Fast instruction_starts =
      GetInstructionStarts::run(method JVM_CHECK);

    TypeArray::Raw exception_handlers = method->exception_table();
    bool has_exception_handlers = (exception_handlers().length() != 0);
    if (has_exception_handlers) {
      check_handlers(method, &instruction_starts JVM_CHECK);
    } else {
      _min_exception_start_bci = 0xffff;
      _max_exception_end_bci = 0;
    }
    check_news(stackmaps, &instruction_starts JVM_CHECK);
  }

  frame()->initialize(method, stackmaps JVM_CHECK);
  initialize_locals(method JVM_CHECK);

  set_no_control_flow(false);

  _num_stackmaps = stackmaps->length();
  _current_stackmap_index = 0;

  method->iterate(0, method->code_size(), this JVM_CHECK);

  // If we're not at the end of the stackmaps, there are extras
  if (_current_stackmap_index != _num_stackmaps) {
    VFY_ERROR(ve_bad_stackmap);   
  }

  // We must not be able to fall off the end
  if (!no_control_flow()) {
    VFY_ERROR(ve_fall_through);
  }
}

void VerifyMethodCodes::check_not_final_override(Method *method JVM_TRAPS) {
  UsingFastOops fast_oops;
  InstanceClass::Fast klass = method->holder();
  InstanceClass::Fast super_class = klass().super();

  if (method->access_flags().is_static()
    || klass().equals(Universe::object_class())){
    return;
  }
  // Lookup the method being verified in the superclass
  Symbol::Fast name = method->name();
  Signature::Fast signature = method->signature();
  Method::Fast superMethod = super_class().lookup_method(&name, &signature);
  if (superMethod.not_null() && 
      superMethod().is_final()) {
    InstanceClass::Raw holder = method->holder();
    InstanceClass::Raw superHolder = superMethod().holder();
    if (superMethod().can_access_by(&holder, &superHolder)) {
      VFY_ERROR(ve_final_method_override);
    }
  }
}

void VerifyMethodCodes::check_handlers(Method *method,
                                       TypeArray* instruction_starts JVM_TRAPS)
{
  UsingFastOops fast_oops;
  int code_size = method->code_size();
  ConstantPool::Fast cp = method->constants();

  TypeArray::Fast exception_handlers = method->exception_table();
  JavaClass::Fast catch_type;

  int exception_table_length = exception_handlers().length();
  int min_start = 0xffff;
  int max_end   = 0;

  // Iterate through the handler table
  for (int i = 0; i < exception_table_length; i+=4) {
    int start_index   = exception_handlers().ushort_at(i);
    int end_index     = exception_handlers().ushort_at(i+1);
    int handler_index = exception_handlers().ushort_at(i+2);
    int class_index   = exception_handlers().ushort_at(i+3);

    if (min_start > start_index) {
      min_start = start_index;
    }
    if (max_end < end_index) {
      max_end = end_index;
    }
    if (   (start_index > end_index)
        || (start_index < 0 || start_index >= code_size) 
        || (handler_index < 0 || handler_index >= code_size)
        || (end_index <= 0 || end_index > code_size)
        || (instruction_starts->byte_at(start_index) == 0)
        || (instruction_starts->byte_at(handler_index) == 0)
        || (end_index < code_size &&
                  instruction_starts->byte_at(end_index) == 0)
       ) { 
        VFY_ERROR(corrupted_exception_handler);
    }
    if (class_index != 0) {
      if (!cp().is_within_bounds(class_index)) {
        VFY_ERROR(ve_cp_index_out_of_bounds);
      }
      if (!ConstantTag::is_klass(cp().tag_value_at(class_index))) {
        VFY_ERROR(ve_expect_class);
      }
      // check that the entry is there and that the class of the exception
      // is a subclass of the catch type

      catch_type = cp().klass_at(class_index JVM_CHECK);
      // Check that it is a subclass of java.lang.Throwable
      if (!catch_type().is_subtype_of(Universe::throwable_class())) {
        VFY_ERROR(ve_expect_throwable);
      }
    }
  }

  _min_exception_start_bci = (jushort)((juint)min_start);
  _max_exception_end_bci   = (jushort)((juint)max_end);
}

void VerifyMethodCodes::check_news(ObjArray *stackmaps, 
                                   TypeArray* instruction_starts JVM_TRAPS) {
  int stackmap_index;
  TypeArray::Raw stackmap_scalars;
  for (stackmap_index = 0; stackmap_index < stackmaps->length();
            stackmap_index += 2) {
    stackmap_scalars = stackmaps->obj_at(stackmap_index);
    int nlocals = stackmap_scalars().int_at(1);
    int nstack  = stackmap_scalars().int_at(nlocals+2);
    for (int i = 0; i < nlocals + nstack; i++) {
      int offset = (i < nlocals) ? i + 2 : i + 3;
      StackMapKind slocal_tag =
          (StackMapKind)stackmap_scalars().int_at(offset);
      if (IS_TAG_TYPE_FOR_NEW_OBJECT(slocal_tag)) {
        int new_bci = DECODE_NEWOBJECT(slocal_tag);
        // A "2" indicates a "new" instruction
        if ((instruction_starts->byte_at(new_bci) & 2) == 0) { 
          VFY_ERROR(ve_bad_stackmap);     // bci should have met offset.
        }
      }
    }
  }
}

void VerifyMethodCodes::bytecode_prolog(JVM_SINGLE_ARG_TRAPS) {
  // Check that stackmaps are ordered according to offset and that every
  // offset in stackmaps points to the beginning of an instruction.
  const int mybci = bci();
  if (_current_stackmap_index < _num_stackmaps) {
    int next_stackmap_bci =
      frame()->get_stackmap_entry_offset(_current_stackmap_index);
    if (next_stackmap_bci == mybci) {
      _current_stackmap_index += 2;             // This offset is good.
    } else if (next_stackmap_bci < mybci) {
      VFY_ERROR(ve_bad_stackmap);               // bci should have met offset.
    }
  }

  // Make sure that we have an appropriate stack map for this instruction
  frame()->check_current_target(method(), mybci, no_control_flow() JVM_CHECK);

#ifndef PRODUCT
  if (TraceVerifier || TraceVerifierByteCodes) {
    const Bytecodes::Code opcode = current_bytecode();
    const char* bytecode = Bytecodes::name(opcode);
    tty->print_cr("Verifying bci==%d, %s", bci(), bytecode);
  }
#endif

  int min = _min_exception_start_bci; // inclusive
  int max = _max_exception_end_bci;   // exclusive

  if (mybci >= min && mybci < max) {
    check_exception_handlers_for_bci(JVM_SINGLE_ARG_CHECK);
  }
  set_no_control_flow(false);
}

void VerifyMethodCodes::bytecode_epilog(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  if (TraceVerifier && !no_control_flow()) {
    print_frame();
  }
}

void VerifyMethodCodes::check_exception_handlers_for_bci(JVM_SINGLE_ARG_TRAPS)
{
  UsingFastOops fast_oops;
  TypeArray::Fast exception_handlers = method()->exception_table();
  Symbol::Fast exception_class;

  int exception_table_length = exception_handlers().length();
  const int mybci = bci();
  for (int i = 0 ; i < exception_table_length ; i += 4) {
    jushort *base = ((jushort*)exception_handlers().base_address()) + i;
    if (mybci >= base[0] && mybci < base[1]) {
      // get the handler PC offset
      int handlerPC = base[2];
      // get the catch type index
      int catchTypeIndex = base[3];
      if (catchTypeIndex != 0) {
        exception_class = cp()->name_of_klass_at(catchTypeIndex JVM_CHECK);
      } else {
        exception_class = Symbols::java_lang_Throwable();
      }

      // Modify the stack to indicate that it has only this single
      // exception on it, and check the target.
      frame()->save_stack_state();
      frame()->push_object(&exception_class JVM_CHECK);
      frame()->check_handler_target(method(), handlerPC JVM_CHECK);
      frame()->restore_stack_state();
    }
  }
}

void VerifyMethodCodes::push_int(jint /*value*/ JVM_TRAPS) {
  push_integer(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::push_long(jlong /*value*/ JVM_TRAPS) {
  push_long(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::push_float(jfloat /*value*/ JVM_TRAPS) {
  push_float(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::push_double(jdouble /*value*/ JVM_TRAPS) {
  push_double(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::push_obj(Oop* value JVM_TRAPS) {
  if (value->is_null()) {
    // aconst_null
    frame()->push(ITEM_Null JVM_NO_CHECK_AT_BOTTOM);
  } else {
    // ldc <String>
    frame()->push_object(Symbols::java_lang_String() JVM_NO_CHECK_AT_BOTTOM);
  }
}

void VerifyMethodCodes::load_local(BasicType kind, int index JVM_TRAPS)  {
  if (kind == T_OBJECT) {
    UsingFastOops fast_oops;
    Symbol::Fast name;
    StackMapKind kind;
    frame()->check_local_ref(index, kind, &name JVM_CHECK);
    frame()->push_category1(kind, &name JVM_NO_CHECK_AT_BOTTOM);
  } else {
    check_local(index, kind JVM_CHECK);
    push(kind JVM_NO_CHECK_AT_BOTTOM);
  }
}

void VerifyMethodCodes::store_local(BasicType kind, int index JVM_TRAPS) {
  if (kind == T_OBJECT) {
    UsingFastOops fast_oops;
    Symbol::Fast name;
    StackMapKind kind;
    frame()->pop_ref(kind, &name JVM_CHECK);
    frame()->set_local_ref(index, kind, &name JVM_NO_CHECK_AT_BOTTOM);
  } else {
    pop(kind JVM_CHECK);
    set_local(index, kind JVM_NO_CHECK_AT_BOTTOM);
  }
}

void VerifyMethodCodes::increment_local_int(int index, jint /*offset*/  
                                            JVM_TRAPS) {
  check_local(index, T_INT JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::array_length(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  Symbol::Fast name;
  StackMapKind kind;
  frame()->pop_ref(kind, &name JVM_CHECK);
  // Top item on the stack must be NULL or an array klass
  if (kind == ITEM_Null || (kind == ITEM_Object && is_array_name(&name))) {
    // success
  } else {
    VFY_ERROR(ve_expect_array);
  }
  push_integer(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::load_array(BasicType kind JVM_TRAPS) {
  pop_integer(JVM_SINGLE_ARG_CHECK);
  if (kind == T_OBJECT) {
    UsingFastOops fast_oops;
    // aaload
    Symbol::Fast array_name, element_name;
    StackMapKind kind;
    frame()->pop_ref(kind, &array_name JVM_CHECK);
    if (kind == ITEM_Null) {
      frame()->push(ITEM_Null JVM_NO_CHECK_AT_BOTTOM);
    } else {
      element_name = reference_array_element_name(&array_name);
      if (!element_name.is_null()) {
        frame()->push_object(&element_name JVM_NO_CHECK_AT_BOTTOM);
      } else {
        VFY_ERROR(ve_stack_bad_type);
      }
    }
  } else {
    pop_array_of(kind JVM_CHECK);
    push(kind JVM_NO_CHECK_AT_BOTTOM);
  }
}

void VerifyMethodCodes::store_array(BasicType kind JVM_TRAPS) {
  if (kind == T_OBJECT) {
    UsingFastOops fast_oops;
    Symbol::Fast element_name, array_name, array_element_name;
    StackMapKind element_kind, array_kind;
    frame()->pop_ref(element_kind, &element_name JVM_CHECK);
    pop_integer(JVM_SINGLE_ARG_CHECK);
    frame()->pop_ref(array_kind, &array_name JVM_CHECK);

    // Both must be actual objects, and not just references.
    if (   (element_kind != ITEM_Null && element_kind != ITEM_Object)
        || (array_kind != ITEM_Null && array_kind != ITEM_Object)) {
      VFY_ERROR(ve_aastore_bad_type);
    }
    /*
     * According to the CLDC Byte Code Typechecker Specification,
     * the only constraints on aastore are:
     *  - the type of a value stored into an array by an aastore instruction
     *    must be a reference type;
     *  - the component type of the array being stored into must be
     *    a reference type.
     */
    if (array_kind != ITEM_Null) {
      GUARANTEE(array_name.not_null(), "Must not be null");
      array_element_name = reference_array_element_name(&array_name);
      if (array_element_name.is_null()) {
        VFY_ERROR(ve_aastore_bad_type);
      }
    }
  } else {
    pop(kind JVM_CHECK);
    pop_integer(JVM_SINGLE_ARG_CHECK);
    pop_array_of(kind JVM_NO_CHECK_AT_BOTTOM);
  }
}

void VerifyMethodCodes::pop(JVM_SINGLE_ARG_TRAPS) {
  StackMapKind kind1;
  frame()->pop_category1(kind1, &_fast_name1 JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::pop2(JVM_SINGLE_ARG_TRAPS) {
  StackMapKind kind1, kind2;
  frame()->pop_category2(kind1, kind2, &_fast_name1, &_fast_name2
                         JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::dup(JVM_SINGLE_ARG_TRAPS) {   
  StackMapKind kind1;
  frame()->pop_category1(kind1,  &_fast_name1 JVM_CHECK);
  frame()->push_category1(kind1, &_fast_name1 JVM_CHECK);
  frame()->push_category1(kind1, &_fast_name1 JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::dup2(JVM_SINGLE_ARG_TRAPS) {
  StackMapKind kind1, kind2;
  frame()->pop_category2(kind1, kind2,  &_fast_name1, &_fast_name2 JVM_CHECK);
  frame()->push_category2(kind1, kind2, &_fast_name1, &_fast_name2 JVM_CHECK);
  frame()->push_category2(kind1, kind2, &_fast_name1, &_fast_name2 
                          JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::dup_x1(JVM_SINGLE_ARG_TRAPS) {
  StackMapKind kind1, kind2;

  frame()->pop_category1(kind1, &_fast_name1 JVM_CHECK);
  frame()->pop_category1(kind2, &_fast_name2 JVM_CHECK);

  frame()->push_category1(kind1, &_fast_name1 JVM_CHECK);
  frame()->push_category1(kind2, &_fast_name2 JVM_CHECK);
  frame()->push_category1(kind1, &_fast_name1 JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::dup2_x1(JVM_SINGLE_ARG_TRAPS) {
  StackMapKind kind1, kind2, kind3;

  frame()->pop_category2(kind1, kind2, &_fast_name1, &_fast_name2 JVM_CHECK);
  frame()->pop_category1(kind3,        &_fast_name3 JVM_CHECK);

  frame()->push_category2(kind1, kind2, &_fast_name1, &_fast_name2 JVM_CHECK);
  frame()->push_category1(kind3,        &_fast_name3 JVM_CHECK);
  frame()->push_category2(kind1, kind2, &_fast_name1, &_fast_name2 
                                        JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::dup_x2(JVM_SINGLE_ARG_TRAPS) {
  StackMapKind kind1, kind2, kind3;

  frame()->pop_category1(kind1,        &_fast_name1 JVM_CHECK);
  frame()->pop_category2(kind2, kind3, &_fast_name2, &_fast_name3 JVM_CHECK);

  frame()->push_category1(kind1,        &_fast_name1 JVM_CHECK);
  frame()->push_category2(kind2, kind3, &_fast_name2, &_fast_name3 JVM_CHECK);
  frame()->push_category1(kind1,        &_fast_name1 JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::dup2_x2(JVM_SINGLE_ARG_TRAPS) {
  StackMapKind kind1, kind2, kind3, kind4;

  frame()->pop_category2(kind1, kind2, &_fast_name1, &_fast_name2 JVM_CHECK);
  frame()->pop_category2(kind3, kind4, &_fast_name3, &_fast_name4 JVM_CHECK);
  frame()->push_category2(kind1, kind2, &_fast_name1, &_fast_name2 JVM_CHECK);
  frame()->push_category2(kind3, kind4, &_fast_name3, &_fast_name4 JVM_CHECK);
  frame()->push_category2(kind1, kind2, &_fast_name1, &_fast_name2 
                          JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::swap(JVM_SINGLE_ARG_TRAPS) {
  StackMapKind kind1, kind2;

  frame()->pop_category1(kind1, &_fast_name1 JVM_CHECK);
  frame()->pop_category1(kind2, &_fast_name2 JVM_CHECK);
  frame()->push_category1(kind1, &_fast_name1 JVM_CHECK);
  frame()->push_category1(kind2, &_fast_name2 JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::neg(BasicType kind JVM_TRAPS) {
  pop(kind JVM_CHECK);
  push(kind JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::binary(BasicType kind, binary_op op JVM_TRAPS) {
  switch(op) {
    case BytecodeClosure::bin_shl:
    case BytecodeClosure::bin_shr:
    case BytecodeClosure::bin_ushr:
      GUARANTEE(kind == T_INT || kind == T_LONG, "Shift operation");
      pop(T_INT JVM_CHECK); break;
    default:
      pop(kind JVM_CHECK); break;
  }
  pop(kind JVM_CHECK);
  push(kind JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::convert(BasicType from, BasicType to JVM_TRAPS)  {
  pop(from JVM_CHECK);
  push(to JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::branch_if(cond_op cond, int dest JVM_TRAPS) {
  switch(cond) {
    case BytecodeClosure::null:
    case BytecodeClosure::nonnull:
      frame()->pop_ref(JVM_SINGLE_ARG_CHECK);
      break;
    default:
      pop_integer(JVM_SINGLE_ARG_CHECK);
      break;
  }
  frame()->check_branch_target(method(), dest JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::branch_if_icmp(cond_op /*cond*/, int dest JVM_TRAPS) {
  pop_integer(JVM_SINGLE_ARG_CHECK);
  pop_integer(JVM_SINGLE_ARG_CHECK);
  frame()->check_branch_target(method(), dest JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::branch_if_acmp(cond_op /*cond*/, int dest JVM_TRAPS) {
  frame()->pop_ref(JVM_SINGLE_ARG_CHECK);
  frame()->pop_ref(JVM_SINGLE_ARG_CHECK);
  frame()->check_branch_target(method(), dest JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::compare(BasicType kind, cond_op /*cond*/ JVM_TRAPS) { 
  pop(kind JVM_CHECK);
  pop(kind JVM_CHECK);
  push(T_INT JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::branch(int dest JVM_TRAPS) {
  frame()->check_branch_target(method(), dest JVM_CHECK);
  set_no_control_flow(true);
}

void VerifyMethodCodes::return_op(BasicType return_kind JVM_TRAPS) {
  UsingFastOops fast_oops;

  Signature::Fast method_signature = method()->signature();
  BasicType expected_kind;
  Symbol::Fast    expected_klass;

  method_return_type(&method_signature, expected_kind, &expected_klass JVM_CHECK);
  switch (expected_kind) {
    case T_BYTE: case T_BOOLEAN: case T_CHAR: case T_SHORT:
       expected_kind = T_INT; break;
    case T_ARRAY:
      expected_kind = T_OBJECT; break;
    default:
      /* Leave result type as it is */
      break;
  }
  if (frame()->need_initialization()) {
    VFY_ERROR(ve_return_uninit_this);
  }
  if (expected_kind != return_kind) {
    VFY_ERROR(ve_retval_bad_type);
  }
  if (return_kind == T_OBJECT) {
    frame()->pop_object(&expected_klass JVM_CHECK);
  } else {
    pop(expected_kind JVM_CHECK);
  }
  set_no_control_flow(true);
}

void VerifyMethodCodes::table_switch(jint table_index, jint default_dest,
                                     jint low, jint high JVM_TRAPS) {
  pop_integer(JVM_SINGLE_ARG_CHECK);

  int range = high - low;
  frame()->check_branch_target(method(), default_dest JVM_CHECK);
  for (int i = 0, location = table_index + 12; i <= range; i++, location += 4){
    int offset = method()->get_java_switch_int(location);
    frame()->check_branch_target(method(), bci() + offset JVM_CHECK);
  }
  set_no_control_flow(true);
}

void VerifyMethodCodes::lookup_switch(jint table_index, jint default_dest,
                                      jint num_of_pairs JVM_TRAPS) {
  pop_integer(JVM_SINGLE_ARG_CHECK);
  frame()->check_branch_target(method(), default_dest JVM_CHECK);
  int last_key = 0;
  for (int i=0, location=table_index+8; i < num_of_pairs; i++, location += 8) {
    int this_key = method()->get_java_switch_int(location);
    int offset = method()->get_java_switch_int(location + 4);
    if (i > 0 && this_key <= last_key) {
      VFY_ERROR(ve_bad_lookupswitch);
    }
    frame()->check_branch_target(method(), bci() + offset JVM_CHECK);
    last_key = this_key;
  }
  set_no_control_flow(true);
}

  // get/set for dynamic class loading
void VerifyMethodCodes::get_field(int index JVM_TRAPS) {
  handle_field_reference(index, false, false JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::put_field(int index JVM_TRAPS) {
  handle_field_reference(index, false, true JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::get_static(int index JVM_TRAPS) {
  handle_field_reference(index, true, false JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::put_static(int index JVM_TRAPS) {
  handle_field_reference(index, true, true JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::handle_field_reference(int index,
                                               bool is_static,
                                               bool is_put JVM_TRAPS) {
  int field_index = index;

  if (!cp()->is_within_bounds(field_index)) {
    VFY_ERROR(ve_cp_index_out_of_bounds);
  }

  // Check that the tag is correct
  jubyte tag = cp()->tag_value_at(field_index);
  if (!ConstantTag::is_field(tag)) {
    VFY_ERROR(ve_expect_fieldref);
  }

  // get the field name, type and the signature
  UsingFastOops fast_oops;
  Symbol::Fast field_name, field_klass_name, field_type_name;
  FieldType::Fast field_type;
  cp()->resolve_helper(field_index, &field_name, &field_type,
                    &field_klass_name JVM_CHECK);
  BasicType field_kind = field_type().basic_type();

  if (field_kind == T_OBJECT) {
    field_type_name = field_type().object_type_symbol();
  } else if (field_kind == T_ARRAY) {
    field_kind = T_OBJECT;
    field_type_name = field_type; // array class names are same as their sig
  }

  Symbol::Fast target_name;
  bool is_protected = is_protected_access(field_index, false JVM_CHECK);
  if (is_protected) {
    // get the class this method belongs to
    InstanceClass::Raw me = method()->holder();
    target_name = me().name();
  } else {
    target_name = field_klass_name;
  }

  if (is_put) {
    if (field_kind == T_OBJECT) {
      frame()->pop_object(&field_type_name JVM_CHECK);
    } else {
      pop(field_kind JVM_CHECK);
    }
    if (!is_static) {
      StackMapKind kind;
      UsingFastOops fast_oops_inside;
      Symbol::Fast init_object_name;
      Symbol::Fast v_class_name;
      InstanceClass::Fast v_class;
      bool in_declaring_class = false;
      frame()->pop_ref(kind, &init_object_name JVM_CHECK);
      if (kind == ITEM_InitObject) {
        // Fix to emulate 1.4.1 verifier behavior
        // The 2nd edition VM of the specification allows field
        // initializations before the superclass initializer,
        // if the field is defined within the current class.

        //Class being verified
        v_class = method()->holder();
        v_class_name = v_class().name();
        GUARANTEE(init_object_name.is_null(), "init_object_name must be null");
        if (v_class_name.equals(&field_klass_name)) {
          TypeArray::Raw fields = v_class().fields();
          int fields_length = fields().length();
          for(int i = 0; i < fields_length; i += 5) {
            Symbol::Raw fname =
              cp()->symbol_at(fields().ushort_at(i + Field::NAME_OFFSET));
            if (fname().equals(&field_name)) {
              Symbol::Raw fsig =
                cp()->symbol_at(fields().ushort_at(i + Field::SIGNATURE_OFFSET));
              if (fsig().equals(&field_type)) {
                in_declaring_class = true;
                break;
              }
            }
          }
        }
      }
      if (in_declaring_class) {
        // We have a putfield opcode acting on an ITEM_InitObject.
        // init_object_name is null in this case.  We now must test
        // the reference type of the field with the type of the class
        // being verified.  Set kind to ITEM_Object and init_object_name
        // to this class name.
        kind = ITEM_Object; 
        init_object_name = v_class().name();
      }
      // push it back on so we can pop it
      frame()->push_category1(kind, &init_object_name JVM_CHECK);
      frame()->pop_object(&target_name JVM_NO_CHECK_AT_BOTTOM);
    }
  } else {
    if (!is_static) {
      frame()->pop_object(&target_name JVM_CHECK);
    }
    if (field_kind == T_OBJECT) {
      frame()->push_object(&field_type_name JVM_NO_CHECK_AT_BOTTOM);
    } else {
      push(field_kind JVM_NO_CHECK_AT_BOTTOM);
    }
  }
}

// Method invocation
void VerifyMethodCodes::invoke_interface(int index, int num_of_args JVM_TRAPS) {
  handle_invoker(index, num_of_args, Bytecodes::_invokeinterface JVM_NO_CHECK_AT_BOTTOM);
}
void VerifyMethodCodes::invoke_special(int index JVM_TRAPS) {
  handle_invoker(index, -1, Bytecodes::_invokespecial JVM_NO_CHECK_AT_BOTTOM);
}
void VerifyMethodCodes::invoke_static(int index JVM_TRAPS) {
  handle_invoker(index, -1, Bytecodes::_invokestatic JVM_NO_CHECK_AT_BOTTOM);
}
void VerifyMethodCodes::invoke_virtual(int index JVM_TRAPS) {
  handle_invoker(index, -1, Bytecodes::_invokevirtual JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::handle_invoker(int index, int /*num_of_args*/,
                                       Bytecodes::Code opcode JVM_TRAPS) {
  int method_index = index;

  if (!cp()->is_within_bounds(method_index)) {
    VFY_ERROR(ve_cp_index_out_of_bounds);
  }

  // Check that the tag is correct
  jubyte tag = cp()->tag_value_at(method_index);
  if (!ConstantTag::is_interface_method(tag) && !ConstantTag::is_method(tag)) {
    VFY_ERROR(ve_expect_methodref);
  }

  // get method name, signature and class
  UsingFastOops fast_oops;
  Symbol::Fast method_name, method_klass_name;
  Signature::Fast method_signature;
  InstanceClass::Fast verifying_class = method()->holder();

  cp()->resolve_helper(method_index, &method_name, &method_signature,
                      &method_klass_name JVM_CHECK);

    // Pop the arguments from the stack
  int nwords = pop_invoke_arguments(&method_signature JVM_CHECK);

  // Check that the only method called that starts with "<" is <init>
  // and that it is only called via an INVOKESPECIAL

  if (method_name().byte_at(0) == '<') {
    if (opcode != Bytecodes::_invokespecial
        || !(method_name.equals(Symbols::object_initializer_name()))) {
      VFY_ERROR(ve_expect_invokespecial);
    }
  }

  // Receiver type processing
  if (opcode != Bytecodes::_invokestatic) {
    // Special processing for calls to <init>
    if (method_name.equals((Oop *)Symbols::object_initializer_name())) {
      UsingFastOops inside;
      Symbol::Fast newobject_name;
      // Pop the receiver type
      StackMapKind receiver_kind;
      Symbol::Fast       receiver_klass;
      frame()->pop_category1(receiver_kind, &receiver_klass JVM_CHECK);
      if (IS_TAG_TYPE_FOR_NEW_OBJECT(receiver_kind)){
        // This is for is a call to <init> that is made to an object that
        // has just been created with a NEW bytecode.
        int new_bci = DECODE_NEWOBJECT(receiver_kind);
        const Bytecodes::Code new_code = bytecode_at(new_bci);
        if( new_bci > (method_size() - 3) || (new_code != Bytecodes::_new) ) {
          VFY_ERROR(ve_expect_new);
        }
        // Get the pool index after the NEW bytecode and check that
        // it is a CONSTANT_class
        int new_index = method()->ushort_at(new_bci + 1);
        if (!cp()->is_within_bounds(new_index)) {
          VFY_ERROR(ve_cp_index_out_of_bounds);
        }

        jubyte tag = cp()->tag_value_at(new_index);
        if (!ConstantTag::is_klass(tag)) {
          VFY_ERROR(ve_expect_class);
        }

        //  Get the target class corresponding to its entry
        newobject_name = cp()->name_of_klass_at(new_index JVM_CHECK);

        // The method must be an <init> method of the indicated class
        if (!newobject_name.equals(&method_klass_name)) {
          VFY_ERROR(ve_bad_init_call);
        }
      } else if (receiver_kind == ITEM_InitObject) {
        // This is for a call to <init> that is made as a result of
        // a constructor calling another constructor (e.g. this() or super())
        newobject_name = verifying_class().name();

        // The receiver class tag type must be the same as the one for the class
        // of the method being verified or its superclass
        if (!method_klass_name.equals(&newobject_name)) {
          JavaClass::Raw super = verifying_class().super();
          Symbol::Raw super_klass_name = super().name();
          if (!method_klass_name.equals(&super_klass_name)) {
            VFY_ERROR(ve_bad_init_call);
          }
        }

        if (!frame()-> need_initialization()) {
          VFY_ERROR(ve_bad_init_call);
        }

        {
          TypeArray::Raw exception_handlers = method()->exception_table();
          int exception_table_length = exception_handlers().length();

          // Iterate through the handler table
          for (int i = 0; i < exception_table_length; i+=4) {
            if (bci() >= exception_handlers().ushort_at(i)
                  && bci() < exception_handlers().ushort_at(i+1)) {
              VFY_ERROR(ve_bad_init_call);
            }
          }
        }

        frame()->set_need_initialization(false);

      } else {
        VFY_ERROR(ve_expect_uninit);
      }

      // Replace all the <init> receiver type with the real target type
      frame()->replace_stack_type_with_real_type(receiver_kind, ITEM_Object, &newobject_name);
    } else {
      // This is for the non INVOKESTATIC case where <init> is not being called.
      //
      // Check that an INVOKESPECIAL is either to the method being verified
      // or a superclass.
      if (opcode == Bytecodes::_invokespecial) {
        if (!is_subclass_of((JavaClass*)&verifying_class, &method_klass_name)){
          VFY_ERROR(ve_invokespecial);
        }

        // The type of the instance must be the same as or a subclass of the 
        // current class.
        // See instructionIsTypeSafe predicate in "CLDC 1.1 Appendix 1", p. 51
        method_klass_name = verifying_class().name();
      }
      // Verification of access to protected methods is done here because
      // the superclass must be loaded at this time (other checks are
      // done at runtime in order to implement lazy class loading.)
      //
      if (opcode == Bytecodes::_invokevirtual) {
        bool protected_method_access =
                   is_protected_access(method_index, true JVM_CHECK);
        if (protected_method_access) {
          method_klass_name = verifying_class().name();
        }
      }
      frame()->pop_object(&method_klass_name JVM_CHECK);
    }
  }
  push_invoke_result(&method_signature JVM_CHECK);

  if (opcode == Bytecodes::_invokeinterface) {
    if (method()->get_ubyte(bci() + 3) != nwords + 1) {
      VFY_ERROR(ve_nargs_mismatch);
    }
    if (method()->get_ubyte(bci() + 4) != 0) {
      VFY_ERROR(ve_expect_zero);
    }
  }
}

void VerifyMethodCodes::invoke_native(BasicType /*return_kind*/,
                                      address /*entry*/
                                      JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  SHOULD_NOT_REACH_HERE();
}

  // Exception handling
void VerifyMethodCodes::throw_exception(JVM_SINGLE_ARG_TRAPS) {
  frame()->pop_object(Symbols::java_lang_Throwable() JVM_CHECK);
  set_no_control_flow(true);
}

  // Object and array allocation
void VerifyMethodCodes::new_object(int index JVM_TRAPS) {
  if (!cp()->is_within_bounds(index)) {
    VFY_ERROR(ve_cp_index_out_of_bounds);
  }

  jubyte tag = cp()->tag_value_at(index);
  if (!ConstantTag::is_klass(tag)) {
    VFY_ERROR(ve_expect_class);
  }

  {
    Symbol::Raw new_name = cp()->name_of_klass_at(index JVM_CHECK);
    if (is_array_name(&new_name)) {
      VFY_ERROR(ve_expect_class);
    }
  }

  StackMapKind new_object_tag_type = (StackMapKind)ENCODE_NEWOBJECT(bci());
  frame()->push(new_object_tag_type JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::new_basic_array(int type JVM_TRAPS) {
  pop_integer(JVM_SINGLE_ARG_CHECK);
  if (T_BOOLEAN <= type && type <= T_LONG) {
    UsingFastOops fast_oops;

    int index = Universe::bool_array_class_index + type - T_BOOLEAN;
    TypeArrayClass* ac = (TypeArrayClass*)(&persistent_handles[index]);
    Symbol::Fast array_name = ac->name();
    frame()->push_object(&array_name JVM_NO_CHECK_AT_BOTTOM);
  } else {
    VFY_ERROR(ve_bad_instr);
  }
}

void VerifyMethodCodes::new_object_array(int index JVM_TRAPS) {
  if (!cp()->is_within_bounds(index)) {
    VFY_ERROR(ve_cp_index_out_of_bounds);
  }
  // use native ordering
  jubyte tag = cp()->tag_value_at(index);
  if (!ConstantTag::is_klass(tag)) {
    VFY_ERROR(ve_expect_class);
  }

  UsingFastOops fast_oops;
  Symbol::Fast name = cp()->name_of_klass_at(index JVM_CHECK);
  Symbol::Fast array_name = TypeSymbol::obj_array_class_name(&name JVM_CHECK);

  pop_integer(JVM_SINGLE_ARG_CHECK);
  frame()->push_object(&array_name JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::new_multi_array(int index, int dim JVM_TRAPS) {
  if (!cp()->is_within_bounds(index)) {
    VFY_ERROR(ve_cp_index_out_of_bounds);
  }

  jubyte tag = cp()->tag_value_at(index);
  if (!ConstantTag::is_klass(tag)) {
    VFY_ERROR(ve_expect_class);
  }

  UsingFastOops fast_oops;
  Symbol::Fast name = cp()->name_of_klass_at(index JVM_CHECK);
  FieldType::Fast ft;
  JavaClass::Fast klass;
  if (GenerateROMImage) {
    // Need to load this since we may be quickening bytecodes and we need
    // to have a resoved CP entry.  During normal VM runtime we don't do
    // this so as to force a NoClassDefFoundError to occur.
    klass = cp()->klass_at(index JVM_CHECK);
  }

  if (dim > 0 && name().is_valid_field_type()) {
    ft = name().obj();
    if (ft().basic_type() == T_ARRAY) {
      klass = ft().object_type();
      while (dim > 0) {
        dim --;
        pop_integer(JVM_SINGLE_ARG_CHECK);

        if (klass.is_type_array_class()) {
          break;
        } else {
          ObjArrayClass::Raw oac = klass.obj();
          klass = oac().element_class();
          if (!klass().is_array_class()) {
            break;
          }
        }
      }
      if (dim == 0) {
        frame()->push_object(&name JVM_NO_CHECK_AT_BOTTOM);
        return;
      }
    }
  }

  VFY_ERROR(ve_multianewarray);
}

void VerifyMethodCodes::check_cast(int index JVM_TRAPS) {
  if (!cp()->is_within_bounds(index)) {
    VFY_ERROR(ve_cp_index_out_of_bounds);
  }
  jubyte tag = cp()->tag_value_at(index);
  if (!ConstantTag::is_klass(tag)) {
    VFY_ERROR(ve_expect_class);
  }

  UsingFastOops fast_oops;
  Symbol::Fast name = cp()->name_of_klass_at(index JVM_CHECK);
  frame()->pop_object(Symbols::java_lang_Object() JVM_CHECK);
  frame()->push_object(&name JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::instance_of(int index JVM_TRAPS) {
  if (!cp()->is_within_bounds(index)) {
    VFY_ERROR(ve_cp_index_out_of_bounds);
  }
  jubyte tag = cp()->tag_value_at(index);
  if (!ConstantTag::is_klass(tag)) {
    VFY_ERROR(ve_expect_class);
  }

  /*Symbol::Fast name = */ cp()->name_of_klass_at(index JVM_CHECK);
  frame()->pop_object(Symbols::java_lang_Object() JVM_CHECK);
  push_integer(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::monitor_enter(JVM_SINGLE_ARG_TRAPS) {
  frame()->pop_ref(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::monitor_exit(JVM_SINGLE_ARG_TRAPS) {
  frame()->pop_ref(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

  // Extra methods for handling fast bytecodes that are hard to revert.
void VerifyMethodCodes::fast_invoke_virtual(int /*index*/ JVM_TRAPS) {
  VFY_ERROR(ve_bad_instr);
}

void VerifyMethodCodes::fast_invoke_virtual_final(int /*index*/ JVM_TRAPS) {
  VFY_ERROR(ve_bad_instr);
}

void VerifyMethodCodes::fast_get_field(BasicType /*field_type*/,
                                       int /*field_offset*/ JVM_TRAPS) {
  VFY_ERROR(ve_bad_instr);
}

void VerifyMethodCodes::fast_put_field(BasicType /*field_type*/,
                                       int /*field_offset*/ JVM_TRAPS) {
  VFY_ERROR(ve_bad_instr);
}

void VerifyMethodCodes::uncommon_trap(JVM_SINGLE_ARG_TRAPS) {
  VFY_ERROR(ve_bad_instr);
}

void VerifyMethodCodes::illegal_code(JVM_SINGLE_ARG_TRAPS) {
  VFY_ERROR(ve_bad_instr);
}

bool VerifyMethodCodes::is_protected_access(int index, bool is_method 
                                            JVM_TRAPS)
{
  UsingFastOops fast_oops;
  Symbol::Fast name;
  Signature::Fast signature;
  Symbol::Fast klass_name;

  InstanceClass::Fast this_class = method()->holder();

  cp()->resolve_helper(index, &name, &signature, &klass_name JVM_CHECK_0);
  if (!is_subclass_of(&this_class, &klass_name)) {
    // If the method's class isn't a superclass of our own, then we have
    // nothing to worry about. See passesProtectedCheck predicate in
    // "CLDC 1.1 Appendix 1", p. 44
    return false;
  }

  {
    AllocationDisabler raw_pointers_used_in_this_block;

    InstanceClass::Raw klass = SystemDictionary::find_class_or_null(&klass_name);
    GUARANTEE(klass.not_null(), "super class must have been loaded.");
    GUARANTEE(this_class().is_subclass_of(&klass), "must be super class");

    Symbol::Raw my_name = this_class().name();

    if (is_method) {
      for( ; klass.not_null(); klass = klass().super()) {
        ObjArray::Raw class_methods = klass().methods();
        if (!class_methods.is_null()) {
          Method::Raw m = InstanceClass::find_method(&class_methods,
                                                     &name, &signature);
          if (m.not_null()) {
            return (m().is_protected() &&
                   !klass().is_same_class_package(&my_name));
          }
        }
      } 
      return false;
    } else {
      // The constructor already looks up the tree to find the
      // the right class, and assigns that value to klass
      Field field(&klass, &name, &signature);
      return  field.is_valid()
           && field.is_protected() 
           && !klass().is_same_class_package(&my_name);
    }
  }
}

void VerifyMethodCodes::push_integer(JVM_SINGLE_ARG_TRAPS) {
  frame()->push(ITEM_Integer JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::push_long(JVM_SINGLE_ARG_TRAPS) {
  frame()->push(ITEM_Long JVM_CHECK);
  frame()->push(ITEM_Long_2 JVM_NO_CHECK_AT_BOTTOM);
}
void VerifyMethodCodes::push_float(JVM_SINGLE_ARG_TRAPS) {
  frame()->push(ITEM_Float JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::push_double(JVM_SINGLE_ARG_TRAPS) {
  frame()->push(ITEM_Double JVM_CHECK);
  frame()->push(ITEM_Double_2 JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::push(BasicType type JVM_TRAPS) {
  switch(type) {
    case T_INT:
    case T_BOOLEAN:
    case T_BYTE:
    case T_CHAR:
    case T_SHORT:   push_integer(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);  break;
    case T_LONG:    push_long(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM); break;
    case T_FLOAT:   push_float(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM); break;
    case T_DOUBLE:  push_double(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM); break;
    case T_VOID:    break;
    default:        SHOULD_NOT_REACH_HERE();
  }
}

void VerifyMethodCodes::pop_integer(JVM_SINGLE_ARG_TRAPS) {
  frame()->pop(ITEM_Integer JVM_NO_CHECK_AT_BOTTOM);
}

void VerifyMethodCodes::pop(BasicType type JVM_TRAPS) {
  switch(type) {
    case T_INT:
    case T_BOOLEAN:
    case T_BYTE:
    case T_CHAR:
    case T_SHORT:   pop_integer(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);  break;
    case T_FLOAT:   frame()->pop(ITEM_Float JVM_NO_CHECK_AT_BOTTOM); break;

    case T_LONG:    frame()->pop(ITEM_Long_2 JVM_CHECK);
                    frame()->pop(ITEM_Long JVM_NO_CHECK_AT_BOTTOM);
                    break;

    case T_DOUBLE:  frame()->pop(ITEM_Double_2 JVM_CHECK);
                    frame()->pop(ITEM_Double JVM_NO_CHECK_AT_BOTTOM);
                    break;

    case T_VOID:    break;      // used by return

    default:        SHOULD_NOT_REACH_HERE();
  }
}

void VerifyMethodCodes::pop_array_of(BasicType type JVM_TRAPS) {
  UsingFastOops fast_oops;
  Symbol::Fast name;
  StackMapKind kind;
  frame()->pop_ref(kind, &name JVM_CHECK);
  if (kind == ITEM_Null) {
    return;
  } else if (kind == ITEM_Object) {
    if (is_array_name(&name)) {
      FieldType::Raw ft = name.obj();
      ArrayClass::Raw ac = ft().object_type();

      if (ac().is_type_array_class()) {
        TypeArrayClass::Raw tac = ac.obj();
        BasicType element_type = (BasicType)tac().type();
        switch (type) {
        case T_BOOLEAN: // fall through
        case T_BYTE:  
          if (element_type == T_BOOLEAN || element_type == T_BYTE) {
            return;
          }
          break;
        default:
          if (type == element_type) {
            return;
          }
        }
      }
    }
  }

  VFY_ERROR(ve_stack_bad_type);
}

void VerifyMethodCodes::check_local(int index, BasicType type JVM_TRAPS) {
  switch(type) {
  case T_INT:
  case T_BOOLEAN:
  case T_BYTE:
  case T_CHAR:
  case T_SHORT:    frame()->check_local(index,  ITEM_Integer JVM_NO_CHECK_AT_BOTTOM);  break;
  case T_FLOAT:    frame()->check_local(index,  ITEM_Float JVM_NO_CHECK_AT_BOTTOM);    break;
  case T_LONG:     frame()->check_local2(index, ITEM_Long JVM_NO_CHECK_AT_BOTTOM);     break;
  case T_DOUBLE:   frame()->check_local2(index, ITEM_Double JVM_NO_CHECK_AT_BOTTOM);  break;
  default:         SHOULD_NOT_REACH_HERE();
  }
}

void VerifyMethodCodes::set_local(int index, BasicType kind JVM_TRAPS) {
  switch(kind) {
  case T_INT:
  case T_BOOLEAN:
  case T_BYTE:
  case T_CHAR:
  case T_SHORT:    frame()->set_local(index,  ITEM_Integer JVM_NO_CHECK_AT_BOTTOM);  break;
  case T_FLOAT:    frame()->set_local(index,  ITEM_Float JVM_NO_CHECK_AT_BOTTOM);  break;
  case T_LONG:     frame()->set_local2(index, ITEM_Long JVM_NO_CHECK_AT_BOTTOM);  break;
  case T_DOUBLE:   frame()->set_local2(index, ITEM_Double JVM_NO_CHECK_AT_BOTTOM);  break;
  default:         SHOULD_NOT_REACH_HERE();
  }
}

void VerifyMethodCodes::initialize_locals(Method* method JVM_TRAPS) {
  UsingFastOops fast_oops;
  /* Fill in the initial derived stack map with argument types. */
  Signature::Fast method_signature = method->signature();

  // force is_static to true
  int n = 0;
  frame()->set_need_initialization(false);
  if (!method->is_static()) {
    UsingFastOops in_if;
    JavaClass::Fast jc = method->holder();
    Symbol::Fast jc_name = jc().name();
    if (!jc.equals(Universe::object_class()) && method->is_object_initializer()) {
      frame()->set_need_initialization(true);
      frame()->set_local(0, ITEM_InitObject JVM_CHECK);
    } else {
      frame()->set_local_ref(0, ITEM_Object, &jc_name JVM_CHECK);
    }
    n = 1;
  }

  SignatureStream ss(&method_signature, true);
  for (; !ss.eos(); ss.next()) {
    BasicType param_type = ss.type();
    if (param_type == T_OBJECT || param_type == T_ARRAY) {
      UsingFastOops in_if;
      Symbol::Fast param_name = ss.type_symbol();
      frame()->set_local_ref(n, ITEM_Object, &param_name JVM_CHECK);
      n++;
    } else {
      set_local(n, param_type JVM_CHECK);
      n += (param_type == T_LONG || param_type == T_DOUBLE) ? 2 : 1;
    }
  }
  for ( ; n < method->max_locals() ; n++) {
    frame()->set_local(n, ITEM_Bogus JVM_CHECK);
  }
}

void VerifyMethodCodes::push_invoke_result(Signature* method_signature JVM_TRAPS){
  UsingFastOops fast_oops;
  BasicType kind;
  Symbol::Fast klass;
  method_return_type(method_signature, kind, &klass JVM_CHECK);
  if (kind == T_OBJECT || kind == T_ARRAY) {
    frame()->push_category1(ITEM_Object, &klass JVM_NO_CHECK_AT_BOTTOM);
  } else {
    push(kind JVM_NO_CHECK_AT_BOTTOM);
  }
}

void VerifyMethodCodes::method_return_type(Signature *method_signature,
                                           BasicType& kind, Symbol *klass
                                           JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  kind = method_signature->return_type();
  if (kind == T_OBJECT || kind == T_ARRAY) {
    *klass = method_signature->return_type_symbol();
  }
}

bool VerifyMethodCodes::is_subclass_of(JavaClass* from, Symbol* to_name) {
  if (to_name->equals(Symbols::java_lang_Object())) return true;
  JavaClass::Raw f = from;
  for(; !f.is_null(); f = f().super()) {
    if (to_name->equals(f().name())) {
      return true;
    }
  }
  return false;
}

bool VerifyMethodCodes::is_array_name(Symbol* name) {
  if (!name->is_valid_field_type()) {
    return false;
  } else {
    FieldType::Raw ft = name->obj();
    return (ft().basic_type() == T_ARRAY);
  }
}

ReturnOop VerifyMethodCodes::reference_array_element_name(Symbol *array_name) {
  if (is_array_name(array_name)) {
    FieldType::Raw ft = array_name->obj();
    ArrayClass::Raw ac = ft().object_type();
    if (ac().is_obj_array_class()) {
      ObjArrayClass::Raw oac = ac.obj();
      JavaClass::Raw element_class = oac().element_class();
      return element_class().name();
    }
  }
  return NULL;
}

// TO DO:
//  Check new
//  Make sure we don't land in the middle of an instruction
