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

#include "incls/_precompiled.incl"
#include "incls/_BytecodeCompileClosure.cpp.incl"

#if ENABLE_COMPILER
BytecodeCompileClosure::PoppedValue::PoppedValue(BasicType type)
  : Value(type)
{
  Compiler::frame()->pop(*this);
}

inline VirtualStackFrame* BytecodeCompileClosure::frame() const {
  return _compiler->frame();
}

inline CodeGenerator* BytecodeCompileClosure::code_generator() const {
  return _code_generator;
}

void BytecodeCompileClosure::initialize(Compiler* compiler, Method* method,
                                        int active_bci) {
  BytecodeClosure::initialize(method);
  set_compiler(compiler);
  set_active_bci(active_bci);
#if ENABLE_ISOLATES
  set_has_clinit(false);
  InstanceClass::Raw k = method->holder();
  while(k.not_null()){
    Method::Raw init =
      k().find_local_method(Symbols::class_initializer_name(), 
                            Symbols::void_signature());
    if (!init.is_null()) {
      _has_clinit = true;
      break;
    }
    k = k().super();
  }
#endif
  TypeArray::Raw exception_handlers = method->exception_table();
  _has_exception_handlers = (exception_handlers().length() != 0);
  _code_generator = Compiler::code_generator();
}

void BytecodeCompileClosure::frame_push(Value &value) {
  frame()->push(value);
}

#define __ code_generator()->

void BytecodeCompileClosure::osr_entry(JVM_SINGLE_ARG_TRAPS) {
  __ osr_entry( JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM );
}

// Push operations.
void BytecodeCompileClosure::push_int(jint value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(push_int);

  Value v(T_INT);
  v.set_int(value);

  // Push int
  frame_push(v);
}

void BytecodeCompileClosure::push_long(jlong value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(push_long);

  Value v(T_LONG);
  v.set_long(value);

  // Push long
  frame_push(v);

}

void BytecodeCompileClosure::push_obj(Oop* value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(push_obj);

  Value v(T_OBJECT);
  v.set_obj(value);

  // Push object
  frame_push(v);
}

void BytecodeCompileClosure::push_float(jfloat value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
#if ENABLE_FLOAT
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(push_float);

  Value v(T_FLOAT);
  v.set_float(value);

  // Push float
  frame_push(v);
#endif
}

void BytecodeCompileClosure::push_double(jdouble value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
#if ENABLE_FLOAT
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(push_double);

  Value v(T_DOUBLE);
  v.set_double(value);

  // Push double
  frame_push(v);
#endif
}

// Local operations.
void BytecodeCompileClosure::load_local(BasicType kind, int index JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(load_local);
  // Load the local
  Value value(kind);
  frame()->value_at(value, Compiler::current_local_base() + index);

  // Push the result
  frame_push(value);
}

void BytecodeCompileClosure::store_local(BasicType kind, int index JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(store_local);

  // Pop the argument
  PoppedValue value(kind);

  // Store the value
  frame()->value_at_put(Compiler::current_local_base() + index, value);
}

// Increment local integer operation.
void BytecodeCompileClosure::increment_local_int(int index, jint offset JVM_TRAPS)
{
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(increment_local_int);
  const int real_index = Compiler::current_local_base() + index;

  Value index_value(T_INT);
  frame()->value_at(index_value, real_index);

  // Clear the local to avoid unnecessary spilling. This would not be
  // necessary if we had dead store elimination.
  frame()->clear(real_index);

  // Give the addend the offset as immediate value, and add it to the local
  // with the given index.
  Value offset_value(T_INT);
  offset_value.set_int(offset);

  Value result(T_INT);
  __ int_binary(result, index_value, offset_value, bin_add JVM_CHECK);
  frame()->value_at_put(real_index, result);
}

// Array operations.
void BytecodeCompileClosure::array_length(JVM_SINGLE_ARG_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(array_length);

  // Pop argument
  PoppedValue array(T_OBJECT);

  if (array.must_be_null()) {
      throw_null_pointer_exception(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  } else {
    Value result(T_INT);

    __ load_from_object(result, array, Array::length_offset(),true JVM_CHECK);

    // Push the result
    frame_push(result);
  }
}

void BytecodeCompileClosure::load_array(BasicType kind JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(load_array);

  // Pop arguments
  PoppedValue index(T_INT);
  PoppedValue array(T_OBJECT);
  Value result(kind);

  if (array.must_be_null()) {
    throw_null_pointer_exception(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  } else {
    // Index check
    array_check(array, index JVM_CHECK);

    // Push array element
    __ load_from_array(result, array, index);
    frame_push(result);
  }
}

void BytecodeCompileClosure::store_array(BasicType kind JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(store_array);

  // Pop arguments
  PoppedValue value(kind);
  PoppedValue index(T_INT);
  PoppedValue array(T_OBJECT);

  if (array.must_be_null()) {
    throw_null_pointer_exception(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  } else {
    // Null and index check
    array_check(array, index JVM_CHECK);

    // Type check
    if (kind == T_OBJECT && !value.must_be_null()) {
      bool skip_type_check = false;
#if ENABLE_COMPILER_TYPE_INFO
      const jushort array_class_id = array.class_id();
      // If there is type info available for the array.
      if (array_class_id > 0) {
        // NOTE: array must be an object array at this point
        ObjArrayClass::Raw array_class = 
          Universe::class_from_id(array_class_id);
        if (array.is_exact_type() || array_class().is_final_type()) {
          const jushort value_class_id = value.class_id();
          JavaClass::Raw value_class = Universe::class_from_id(value_class_id);
          JavaClass::Raw array_element_class = array_class().element_class();
          if (value_class().is_subtype_of(&array_element_class)) {
            skip_type_check = true;
          }
        }
      }
#else
      skip_type_check = array.is_object_array() ||
        (value.is_string() && array.is_string_array());
#endif

      if (skip_type_check) {
#ifndef PRODUCT
        __ comment("Eliding type check");
#endif
      } else { 
        __ type_check(array, index, value JVM_CHECK);
      }
    }

    // Store array element
    __ store_to_array(value, array, index);
  }
}

#if ENABLE_CONDITIONAL_BRANCH_OPTIMIZATIONS
void BytecodeCompileClosure::branch_if_flags(JVM_SINGLE_ARG_TRAPS) {
  const int bci = get_next_bci();
  if( bci > 0 ) {
    const Bytecodes::Code code = bytecode_at(bci);
    const unsigned offset = unsigned(code) - Bytecodes::_ifeq;
    if( offset <= unsigned(Bytecodes::_ifle - Bytecodes::_ifeq) ) {
      const cond_op cond = cond_op(offset + BytecodeClosure::eq);
      const int dest = branch_destination(bci);
      set_bytecode(bci);
      set_default_next_bytecode_index(bci);
      Compiler::set_bci(bci); // Is it really necessary?

      // Insert OSR entries for backward branches.
      if( dest < bci || is_active_bci() ) {
        osr_entry(JVM_SINGLE_ARG_CHECK);
      }

      // Pop argument
      PoppedValue op1(T_INT);
      Value op2(T_INT);
      op2.set_int(0);

      __ branch_if(cond, dest, op1, op2, true JVM_NO_CHECK_AT_BOTTOM);
      set_jump_from_current_bci( dest );
    }
  }
}
#endif

void BytecodeCompileClosure::binary(BasicType kind, binary_op op JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(binary);

  // Pop arguments
  PoppedValue op2((op==bin_shl || op==bin_shr || op==bin_ushr) ? T_INT : kind);
  PoppedValue op1(kind);

  // Operate
  Value result(kind);
  switch (kind) {
  case T_INT    : 
    __ int_binary(result, op1, op2, op  JVM_NO_CHECK); 
    break;
  case T_LONG   :
    __ long_binary(result, op1, op2, op JVM_NO_CHECK);
    break;
#if ENABLE_FLOAT
  case T_FLOAT  : 
    __ float_binary(result, op1, op2, op JVM_NO_CHECK);
    break;
  case T_DOUBLE : 
    __ double_binary(result, op1, op2, op JVM_NO_CHECK);
    break;
#endif
  default       : SHOULD_NOT_REACH_HERE();
  }

  if (!CURRENT_HAS_PENDING_EXCEPTION) {
    frame_push(result);
#if ENABLE_CONDITIONAL_BRANCH_OPTIMIZATIONS
    #define DEF( name ) | (1 << bin_##name)
    enum { ops = 0
      DEF( add ) DEF( sub )
      DEF( shl ) DEF( shr ) DEF( ushr )
      DEF( and ) DEF( or  ) DEF( xor  )      
    };
    #undef DEF
    if( kind == T_INT && !result.is_immediate() && ((ops >> op) & 1) ) {
      branch_if_flags(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
    }
#endif
  }
}

void BytecodeCompileClosure::unary(BasicType kind, unary_op op JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(unary);

  // Pop arguments
  PoppedValue op1(kind);

  // Operate
  Value result(kind);
  switch (kind) {
  case T_INT    : 
    __ int_unary(result, op1, op  JVM_NO_CHECK); 
    break;
  case T_LONG   : 
    __ long_unary(result, op1, op JVM_NO_CHECK);
    break;
#if ENABLE_FLOAT
  case T_FLOAT  : 
    __ float_unary(result, op1, op JVM_NO_CHECK);
    break;
  case T_DOUBLE : 
    __ double_unary(result, op1, op JVM_NO_CHECK);
    break;
#endif
  default       :
    SHOULD_NOT_REACH_HERE();
  }

  if (!CURRENT_HAS_PENDING_EXCEPTION) {
    frame_push(result);
#if ENABLE_CONDITIONAL_BRANCH_OPTIMIZATIONS
    if( kind == T_INT && !result.is_immediate() && op == una_neg ) {
      branch_if_flags(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
    }
#endif
  }
}

// Conversion operations.
void BytecodeCompileClosure::convert(BasicType from, BasicType to JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(convert);

  PoppedValue value(from);
  bool is_immediate = value.is_immediate();
  Value result(to);

  CodeGenerator* gen = code_generator();

  // If this is an i2{bsc}, and the next instruction is
  // a {bsc}astore, we can treat the current instruction as an no-op
  const Bytecodes::Code bc = current_bytecode();
  switch (bc) {
  case Bytecodes::_i2b:
  case Bytecodes::_i2s:
  case Bytecodes::_i2c:
    {
      const int nextbci = get_next_bci();
      if( nextbci > 0 ) {
        const Bytecodes::Code nextbc = bytecode_at(nextbci);
        if ((bc - nextbc) == (Bytecodes::_i2b - Bytecodes::_bastore)) {
          if (GenerateCompilerComments) {
            gen->comment("narrowing opcode is safely skipped");
            value.copy(result);
            frame_push(result);
            return;
          }
        }
      }
    }
    break;
  }

  switch( current_bytecode() ) {
  case Bytecodes::_i2b:
    if (is_immediate) {
      result.set_int((jbyte)value.as_int());
    } else {
      gen->i2b(result, value JVM_NO_CHECK); 
    }
    break;
  case Bytecodes::_i2c:
    if (is_immediate) {
      result.set_int((jchar)value.as_int());
    } else { 
      gen->i2c(result, value JVM_NO_CHECK);
    }
    break;
  case Bytecodes::_i2s:
    if (is_immediate) {
      result.set_int((jshort)value.as_int());
    } else { 
      gen->i2s(result, value JVM_NO_CHECK);
    }
    break;
  case Bytecodes::_i2l:
    if (is_immediate) {
      result.set_long((jlong)value.as_int());
    } else {
      gen->i2l(result, value JVM_NO_CHECK);
    }
    break;
  case Bytecodes::_l2i:
    if (is_immediate) {
      result.set_int(value.lsw_bits());
    } else {
      Assembler::Register r = value.lsw_register();
      RegisterAllocator::reference(r);
      result.set_register(r);
    }
    break;

#if ENABLE_FLOAT
  case Bytecodes::_i2f:
    gen->i2f(result, value JVM_NO_CHECK);
    break;
  case Bytecodes::_i2d:
    gen->i2d(result, value JVM_NO_CHECK);
    break;
  case Bytecodes::_l2f:
    gen->l2f(result, value JVM_NO_CHECK);
    break;
  case Bytecodes::_l2d:
    gen->l2d(result, value JVM_NO_CHECK);
    break;
  case Bytecodes::_f2i:
    gen->f2i(result, value JVM_NO_CHECK);
    break;
  case Bytecodes::_f2l:
    gen->f2l(result, value JVM_NO_CHECK);
    break;
  case Bytecodes::_f2d:
    gen->f2d(result, value JVM_NO_CHECK);
    break;
  case Bytecodes::_d2i:
    gen->d2i(result, value JVM_NO_CHECK);
    break;
  case Bytecodes::_d2l:
    gen->d2l(result, value JVM_NO_CHECK);
    break;
  case Bytecodes::_d2f:
    gen->d2f(result, value JVM_NO_CHECK);
    break;
#endif
  default:
    SHOULD_NOT_REACH_HERE();
    break;
  }

  if (!CURRENT_HAS_PENDING_EXCEPTION) {
    // Some compiled code calls the interpreter.  The
    // check is to ensure we don't push garbage.
    if (!Compiler::closure()->is_compilation_done()) {
      frame_push(result);
    }
  }
}

void BytecodeCompileClosure::neg(BasicType kind JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  unary(kind, una_neg JVM_NO_CHECK_AT_BOTTOM);
}

void BytecodeCompileClosure::pop(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(pop);
  frame()->pop();
}

void BytecodeCompileClosure::pop_and_npe_if_null(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(pop_and_npe_if_null);

  // Pop argument
  PoppedValue value(T_OBJECT);
  if (value.must_be_null()) {
    throw_null_pointer_exception(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  } else {
    // Null check
#if ENABLE_NPCE
    __ maybe_null_check_by_npce(value, true, false, T_INT JVM_NO_CHECK_AT_BOTTOM);
#else 
    __ maybe_null_check(value JVM_NO_CHECK_AT_BOTTOM);
#endif
  }
}
void BytecodeCompileClosure::pop2(JVM_SINGLE_ARG_TRAPS)    { 
  JVM_IGNORE_TRAPS;
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(pop2);
  frame()->pop2();    
}
void BytecodeCompileClosure::dup(JVM_SINGLE_ARG_TRAPS)     { 
  JVM_IGNORE_TRAPS;
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(dup);
  frame()->dup();     
}
void BytecodeCompileClosure::dup2(JVM_SINGLE_ARG_TRAPS)    { 
  JVM_IGNORE_TRAPS;
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(dup2);
  frame()->dup2();    
}
void BytecodeCompileClosure::dup_x1(JVM_SINGLE_ARG_TRAPS)  { 
  JVM_IGNORE_TRAPS;
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(dup_x1);
  frame()->dup_x1();  
}
void BytecodeCompileClosure::dup2_x1(JVM_SINGLE_ARG_TRAPS) { 
  JVM_IGNORE_TRAPS;
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(dup2_x1);
  frame()->dup2_x1(); 
}
void BytecodeCompileClosure::dup_x2(JVM_SINGLE_ARG_TRAPS)  { 
  JVM_IGNORE_TRAPS;
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(dup_x2);
  frame()->dup_x2();  
}
void BytecodeCompileClosure::dup2_x2(JVM_SINGLE_ARG_TRAPS) { 
  JVM_IGNORE_TRAPS;
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(dup2_x2);
  frame()->dup2_x2(); 
}
void BytecodeCompileClosure::swap(JVM_SINGLE_ARG_TRAPS)    { 
  JVM_IGNORE_TRAPS;
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(swap);
  frame()->swap();    
}

void BytecodeCompileClosure::branch(int dest JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(branch);
  
  // Insert OSR entries for backward branches.
  if (dest < bci()) {
    osr_entry(JVM_SINGLE_ARG_CHECK);
  }
  __ branch(dest JVM_NO_CHECK_AT_BOTTOM);
  set_jump_from_current_bci( dest );
}

void BytecodeCompileClosure::branch_if(cond_op cond, int dest JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(branch_if);

  // Insert OSR entries for backward branches.
  if (dest < bci() || is_active_bci()) {
    osr_entry(JVM_SINGLE_ARG_CHECK);
  }
  BasicType kind( T_INT );
  switch (cond) {
    case null: case nonnull :
      kind = T_OBJECT; break;
  }

  // Pop argument
  PoppedValue op1(kind);
  Value op2(T_INT);             // branch_if doesn't check types
  op2.set_int(0);

  __ branch_if(cond, dest, op1, op2 JVM_NO_CHECK_AT_BOTTOM);
  set_jump_from_current_bci( dest );
}

void BytecodeCompileClosure::branch_if_icmp(cond_op cond, int dest JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(branch_if_icmp);

  // Insert OSR entries for backward branches.
  if (dest < bci() || is_active_bci()) {
      
    //cse     
    record_passable_statue_of_osr_entry(dest);      
    osr_entry(JVM_SINGLE_ARG_CHECK);
  }

  // Pop arguments
  PoppedValue op2(T_INT);
  PoppedValue op1(T_INT);

  // Branch
  __ branch_if(cond, dest, op1, op2 JVM_NO_CHECK_AT_BOTTOM);
  set_jump_from_current_bci( dest );
}

void BytecodeCompileClosure::branch_if_acmp(cond_op cond, int dest JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(branch_if_acmp);

  // Insert OSR entries for backward branches.
  if (dest < bci() || is_active_bci()) {
    osr_entry(JVM_SINGLE_ARG_CHECK);
  }

  // Pop arguments
  PoppedValue op2(T_OBJECT);
  PoppedValue op1(T_OBJECT);

  // Branch
  __ branch_if(cond, dest, op1, op2 JVM_NO_CHECK_AT_BOTTOM);
  set_jump_from_current_bci( dest );
}

void BytecodeCompileClosure::compare(BasicType kind, cond_op cond JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(compare);

  // Pop arguments
  PoppedValue op2(kind);
  PoppedValue op1(kind);

  // Compare longs
  Value result(T_INT);
  switch(kind) {
  case T_LONG:
    if (op1.is_immediate() && op2.is_immediate()) {
      jlong arg1 = op1.as_long();
      jlong arg2 = op2.as_long();
      int result_imm = (arg1 < arg2 ?  -1 : arg1 == arg2 ? 0 : 1);
      result.set_int(result_imm);
    } else { 
      __ long_cmp(result, op1, op2 JVM_NO_CHECK);
    }
    break;
#if ENABLE_FLOAT
  case T_FLOAT:
    __ float_cmp(result, cond, op1, op2 JVM_NO_CHECK);
    break;

  case T_DOUBLE:
   __ double_cmp(result, cond, op1, op2 JVM_NO_CHECK);
   break;
#endif

  default:
    SHOULD_NOT_REACH_HERE();
  }

  if (!CURRENT_HAS_PENDING_EXCEPTION) {
    frame_push(result);
  }
}

void BytecodeCompileClosure::check_cast(int index JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(check_cast);

  UsingFastOops fast_oops;
  // Retrieve the initialized class or generate an uncommon trap
  JavaClass::Fast klass = get_klass_or_null(index, false JVM_CHECK);
  if (klass.is_null()) {
    __ uncommon_trap(JVM_SINGLE_ARG_CHECK);
    return;
  }

  // Pop arguments
  PoppedValue object(T_OBJECT);

  if (!object.must_be_null()) {
#if ENABLE_COMPILER_TYPE_INFO
    const jushort class_id = object.class_id();
    JavaClass::Raw object_class = Universe::class_from_id(class_id);
    GUARANTEE(object_class.not_null(), "Sanity");
    if (!object_class().is_subtype_of(&klass)) {
#endif
      // Do the checking.
      Value klass_value(T_OBJECT);
      klass_value.set_obj(&klass);
      __ check_cast(object, klass_value, klass().class_id() JVM_CHECK);
#if ENABLE_COMPILER_TYPE_INFO
      frame()->set_value_class(object, &klass);
    }
#endif
  }
  // Push result
  frame_push(object);
}

void BytecodeCompileClosure::instance_of(int index JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(instance_of);

  UsingFastOops fast_oops;
  // Retrieve the initialized class or generate an uncommon trap
  JavaClass::Fast klass = get_klass_or_null(index, false JVM_CHECK);
  if (klass.is_null()) {
    __ uncommon_trap(JVM_SINGLE_ARG_CHECK);
    return;
  }

  // Pop arguments
  PoppedValue object(T_OBJECT);

  Value result(T_INT);

  if (object.must_be_null()) {
    result.set_int(0);
  } else {
#if ENABLE_COMPILER_TYPE_INFO
    const jushort class_id = object.class_id();
    JavaClass::Raw object_class = Universe::class_from_id(class_id);
    GUARANTEE(object_class.not_null(), "Sanity");
    if (!object_class().is_subtype_of(&klass)) {
#endif
      // Do the checking.
      Value klass_value(T_OBJECT);
      klass_value.set_obj(&klass);
      __ instance_of(result, object, klass_value, 
                     klass().class_id() JVM_CHECK);
#if ENABLE_COMPILER_TYPE_INFO
    } else if (object.must_be_nonnull()) {
      result.set_int(1);
    } else {
      result.assign_register();
      __ move(result.lo_register(), object.lo_register());
    }
#endif
  }
  // Push result
  frame_push(result);

#if ENABLE_COMPILER_TYPE_INFO
  // Try to optimize for a frequent pattern:
  //   if (anObject instanceof aClass) {
  //      ...
  //      ((aClass)anObject).aMethod();
  //      ...
  const int nextbci = get_next_bci();
  if( nextbci > 0 ) {
    const Bytecodes::Code nextbc = bytecode_at(nextbci);
    if (nextbc == Bytecodes::_ifeq) {
      const int fallthrough_bci = next_bci(nextbci);
      Compiler::set_bci(nextbci);
      // Recursive invocation to find out if the compiler takes the branch
      compile(JVM_SINGLE_ARG_CHECK);
      if (next_bytecode_index() == fallthrough_bci) {
        frame()->set_value_class(object, &klass);
      }
    }
  }
#endif
}

// Returns.
void BytecodeCompileClosure::return_op(BasicType kind JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(return_op);
#if ENABLE_INLINE
  if (Compiler::is_inlining()) {
    Compiler * compiler = Compiler::current();

    {
      Signature::Raw signature = method()->signature();
      BasicType type  = signature().return_type();
      
      if (type == T_VOID) {
        frame()->set_virtual_stack_pointer(Compiler::current_local_base() - 1);
      } else {
        //return value from callee
        Value temp(type);
        frame()->pop(temp);
        frame()->set_virtual_stack_pointer(Compiler::current_local_base() - 1);
        frame()->push(temp);
      }

      VirtualStackFrame* return_frame = compiler->parent_frame();
      if( !return_frame ) {
        frame()->conformance_entry(true);
        return_frame = frame()->clone(JVM_SINGLE_ARG_CHECK);
        compiler->set_parent_frame( return_frame );
      } else {
        frame()->conform_to( return_frame );
      }
    }

    BinaryAssembler::Label return_label = compiler->inline_return_label();
    if( compiler->compilation_queue() != NULL ) {
      code_generator()->jmp( return_label );    
    } 
    compiler->set_inline_return_label( return_label );
    return;    
  }
#endif // ENABLE_INLINE

  // In case of a synchronized method unlock the activation.
  if (method()->access_flags().is_synchronized()) {
    __ unlock_activation(JVM_SINGLE_ARG_CHECK);
  }

  // If the method has monitor bytecodes check that all monitors are unlocked.
  if (method()->access_flags().has_monitor_bytecodes()) {
    __ check_monitors(JVM_SINGLE_ARG_CHECK);
  }

  // Do the return.
  if (kind == T_VOID) {
    // Return
    __ return_void(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  } else {
    // Pop the return value
    PoppedValue value(kind);

    // Return
    __ return_result(value JVM_NO_CHECK_AT_BOTTOM);
  }
}

void BytecodeCompileClosure::throw_exception(JVM_SINGLE_ARG_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(throw_exception);

  UsingFastOops fast_oops;
  jint handler_bci;

  // Get the value to throw, which may be NULL!
  PoppedValue value(T_OBJECT);
  frame()->clear_stack();

  if (value.must_be_null()) {
    throw_null_pointer_exception(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
    return;
  } 
  __ maybe_null_check(value JVM_CHECK);

  if (_debugger_active) {
    // deoptimize and let interpreter code handle it
    frame()->flush(JVM_SINGLE_ARG_CHECK);
    __ call_vm_extra_arg(value.lo_register());
    // Make sure the caller can use whatever registers it wants
    value.destroy();
    __ call_vm((address)0, T_ILLEGAL JVM_NO_CHECK_AT_BOTTOM);
    return;
  }

  handler_bci = method()->exception_handler_if_first_is_any(bci());
  if (handler_bci != -1) {
    // It doesn't matter what the exception is.  There is an "any"
    // handler.  We can just go there
    frame_push(value);
    __ branch(handler_bci JVM_NO_CHECK_AT_BOTTOM);
    return;
  }

  if (!method()->exception_handler_exists_for(bci())
      && !method()->access_flags().is_synchronized()
      && !method()->access_flags().has_monitor_bytecodes()) {
    // This method has no handler for the given bci, and there are no
    // monitors to clear.  So just return
    frame()->clear();
    // IMPL_NOTE: Is the call to frame()->clear() necessary, since we're about
    // to call return_error?
    __ return_error(value JVM_NO_CHECK_AT_BOTTOM);
    return;
  }

  // If this method has exactly one exception handler, and it convers the
  // current bci, the program may be using the athrow bytecode as a goto.
  // For simplicity, We only handle the case where the expression stack has
  // only the exception object just before the athrow bytecode is
  // executed.
  TypeArray::Fast et = method()->exception_table();
  ConstantPool::Fast cp = method()->constants();
  JavaClass::Fast catch_type;
  if (frame()->virtual_stack_pointer() == method()->max_locals()-1 &&
      et().length() == 4 &&
      bci() >= et().ushort_at(0) && bci() < et().ushort_at(1)) {
    catch_type = cp().klass_at(et().ushort_at(3) JVM_CHECK);
    handler_bci = et().ushort_at(2);
    __ quick_catch_exception(value, &catch_type, handler_bci JVM_CHECK);
  }

  // We don't know what to do.  Deoptimize and let the interpreter handle it.
  frame()->flush(JVM_SINGLE_ARG_CHECK);
  __ call_vm_extra_arg(value.lo_register());
  // Make sure the caller can use whatever registers it wants
  value.destroy();
  __ call_vm((address)0, T_ILLEGAL JVM_NO_CHECK_AT_BOTTOM);
}

void BytecodeCompileClosure::table_switch(jint table_index,
                                          jint default_dest, 
                                          jint low, jint high JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(table_switch);

  // Pop argument
  PoppedValue index(T_INT);

  if (index.is_immediate()) {
    int jump_dest = default_dest;
    if (index.as_int() >= low && index.as_int() <= high) {
      int jump_index = index.as_int() - low;
      jump_dest = bci() +
          method()->get_java_switch_int(4 * jump_index + table_index + 12);
    }
    branch(jump_dest JVM_NO_CHECK_AT_BOTTOM);
  } else {
    __ table_switch(index, table_index, default_dest, low, high 
                    JVM_NO_CHECK_AT_BOTTOM);
  }
}

void BytecodeCompileClosure::lookup_switch(jint table_index, jint default_dest,
                                           jint num_of_pairs JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(lookup_switch);

  // Pop argument
  PoppedValue index(T_INT);

  if (index.is_immediate()) {
    int jump_dest = default_dest;
    for (int i = 0; i < num_of_pairs; i++) {
      if (index.as_int() ==
              method()->get_java_switch_int(8 * i + table_index + 8)) {
        jump_dest =
            bci() + method()->get_java_switch_int(8 * i + table_index + 12);
      }
    }
    __ branch(jump_dest JVM_NO_CHECK_AT_BOTTOM);
  } else { 
    __ lookup_switch(index, table_index, default_dest, num_of_pairs
                     JVM_NO_CHECK_AT_BOTTOM);
  }
}

void BytecodeCompileClosure::get_field(int index JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(get_field);

  non_fast_field_access(index, /*is_get=*/true JVM_NO_CHECK_AT_BOTTOM);
}

void BytecodeCompileClosure::put_field(int index JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(put_field);

  non_fast_field_access(index, /*is_get=*/false JVM_NO_CHECK_AT_BOTTOM);
}

void
BytecodeCompileClosure::non_fast_field_access(int index, bool is_get JVM_TRAPS) {
  BasicType field_type;
  int field_offset;

  if (ResolveConstantPoolInCompiler) {
    bool resolved = method()->try_resolve_field_access(index, field_type,
                                                       field_offset,
                                                       /*static=*/false,
                                                       is_get JVM_NO_CHECK);
    GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION, "sanity");

    if (resolved) {
      if (is_get) {
        fast_get_field(field_type, field_offset JVM_NO_CHECK_AT_BOTTOM);
      } else {
        fast_put_field(field_type, field_offset JVM_NO_CHECK_AT_BOTTOM);
      }
      return;
    }
  }

  __ uncommon_trap(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

void BytecodeCompileClosure::fast_get_field(BasicType field_type,
                                            int field_offset JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(fast_get_field);

  // Pop argument
  PoppedValue obj(T_OBJECT);

  if (obj.must_be_null()) {
    throw_null_pointer_exception(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  } else {
    // Load the field, doing NULL check
    Value result(field_type);
    __ load_from_object(result, obj, field_offset, 
                        /*null check*/true JVM_CHECK);

#if ENABLE_COMPILER_TYPE_INFO
    if (field_type == T_OBJECT || field_type == T_ARRAY) {
      const jushort obj_class_id = obj.class_id();
      // Zero class id is java.lang.Object, no fields there.
      if (obj_class_id > 0) {
        InstanceClass::Raw obj_class = Universe::class_from_id(obj_class_id);
        set_field_class_id(result, &obj_class, 
                           field_offset, /*non-static*/false);
      }
    }
#endif
    // Push the result
    frame_push(result);
  }
}

void BytecodeCompileClosure::fast_put_field(BasicType field_type,
                                            int field_offset JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(fast_put_field);

  // Pop arguments
  PoppedValue value(field_type);
  PoppedValue obj(T_OBJECT);

  if (obj.must_be_null()) {
    throw_null_pointer_exception(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  } else {
    // Store the value in local, doing null check
    __ store_to_object(value, obj, field_offset, true JVM_NO_CHECK_AT_BOTTOM);
  }
}


#if ENABLE_ISOLATES

// Determine the requirement for a class initialization barrier
// when using a class. The access_static_var flag indicate that
// the requirement are for accessing an static variable.
void BytecodeCompileClosure::cib_requirements(int class_id,
                                              bool &needs_cib,
                                              bool access_static_var) {
  InstanceClass::Raw klass = Universe::class_from_id(class_id);
  needs_cib = false;
  if (UseROM && class_id >= ROM::number_of_system_classes() &&
      klass().is_initialized()) {
    // If the referenced class is already initialized in this task and it
    // is not a system class it will always be initialized from this
    // time onward so we can skip the barrier test.
    return;
  }
  int method_holder_id = method()->holder_id();
  InstanceClass::Raw k;

  // Simple class initialization barrier elimination.
  // We do not need a barrier if the holder of the static var is also
  // the compiled method's class, or one of its super class.

  // IMPL_NOTE:
  //   - redundant barrier elimination.
  //   - not-initializer barrier elimination.
  needs_cib =  method_holder_id != class_id;
  if (needs_cib){
    // Not accessing a static var of the compiled method's class.
    // Check if the var belongs to a super class.
    k = Universe::class_from_id(method_holder_id);
    k = k().super();
    while(k.not_null()){
      if (k==klass) {
        needs_cib=false;
        break;
      }
      k = k().super();
    }
  }

  if (access_static_var && !needs_cib) {

    // Since needs_cib == false we know that the method being compiled
    // either is the declaring class or a subclass.  We may be compiling this
    // method because it was called from a super class's static initializer.
    // If so, when we OSR to this method the task_mirror for this class
    // will still be the task_class_init_marker.  Therefore, we will need
    // to have the barrier check.  We used to just check this method being
    // compiled to see if it had a clinit method, but we must check all
    // super classes as well.

    if (has_clinit()) {
      needs_cib = true;
    }
  }
}
#endif // ENABLE_ISOLATES

bool BytecodeCompileClosure::try_resolve_static_field(int index,
                                                      bool is_get
                                                      JVM_TRAPS)
{
  if (!ResolveConstantPoolInCompiler) {
    return false;
  }
  BasicType field_type_dummy;
  int field_offset_dummy;
  return method()->try_resolve_field_access(index, field_type_dummy,
                                            field_offset_dummy,
                                            /*static=*/true,
                                            is_get
                                            JVM_NO_CHECK_AT_BOTTOM);
}

void BytecodeCompileClosure::get_static(int index JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(get_static);

  UsingFastOops fast_oops;

  const Bytecodes::Code bc = current_bytecode();
  if (bc == Bytecodes::_getstatic) {
    // Always need to call ConstantPool::field_type_at(). See
    // src/tests/vm/share/handles/ConstantPool/field_type_at7 
    bool resolved = try_resolve_static_field(index, /*is_get=*/true
                                             JVM_MUST_SUCCEED);
    if (!resolved) {
      __ uncommon_trap(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
      return;
    }
  }

  // Get the class id, the offset and the kind.
  int class_id, offset;
  BasicType kind = cp()->resolved_field_type_at(index, offset, class_id);
  if (byte_size_for(kind) < T_INT_byte_size) {
    // 8- and 16-bit static fields are always stored in a 32-bit
    // slot.
    kind = T_INT;
  }

#if ENABLE_ISOLATES
  // We never issue an uncommon trap here because class initialization
  // must always be tested because of code sharing across isolates.
  InstanceClass::Fast klass = Universe::class_from_id(class_id);
#else

  InstanceClass::Fast klass;

#if USE_AOT_COMPILATION
  if (GenerateROMImage) {
    // Retrieve the class. For an uninitialized class generate 
    // initialization code.  
    klass = get_klass_from_id_and_initialize(class_id JVM_CHECK);
  } else
#endif
  {
    // Retrieve the initialized class or generate an uncommon trap
    klass = get_klass_or_null_from_id(class_id);
    if (klass.is_null()) {
      __ uncommon_trap(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
      return;
    }
  }

#endif //ENABLE_ISOLATES

  // Create a value for the result
  Value result(kind);
#if ENABLE_ISOLATES
  // Have to disable setting immediate for multi-tasking.
  // First, the compiled code may be shared among multiple task
  // so we must anyway add a class initialization barrier, even
  // if we only access a final static variable.
  // Second, final static var are currently stored with all other
  // static vars in the task mirror object. So we pay an indirection
  // for them currently and we cannot set them as immediate -- unless
  // they are true constant value, and not final value computed at
  // static initialization time.
  // In the future, we should change that so that true constant final
  // variable are stored in a shared area. This however will be problematic for
  // interpreted code. We may have to introduce a different bytecode
  // for final static to distinguish between true constant and other static
  // vars.

  // Meanwhile, we disable the following... [Laurent]
#else
  // Check for static final immediates
  if (klass().is_initialized() && !TestCompiler) {
    // We don't do it if TestCompiler -- in this test mode, classes
    // are marked as initialized, without executing <clinit>. As a result
    // many final static object fields will have null values. If we use
    // these immediate null values, the compiler will generate throw-null
    // sequences.
    result.set_immediate_from_static_field(&klass, offset);
  }
#endif
  // If the result isn't present we have to load it from the class
  if (!result.is_present()) {
    // Create the obj value
    Value klass_value(T_OBJECT);
#if ENABLE_ISOLATES
    bool needs_cib;
    cib_requirements(class_id, needs_cib, true);
    __ load_task_mirror(&klass, klass_value, needs_cib JVM_CHECK);
#else
    klass_value.set_obj(&klass);
#endif //ENABLE_ISOLATES
    // Here: the mirror is now loaded in klass_value.

    // Load the static field
    __ load_from_object(result, klass_value, offset, false JVM_CHECK);

#if ENABLE_COMPILER_TYPE_INFO
    if (kind == T_OBJECT || kind == T_ARRAY) {
      set_field_class_id(result, &klass, offset, /*static*/true);
    }
#endif
  }

  // Push the result
  frame_push(result);
}

void BytecodeCompileClosure::put_static(int index JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(put_static);

  const Bytecodes::Code bc = current_bytecode();
  if (bc == Bytecodes::_putstatic) {
    // Always need to call ConstantPool::field_type_at().
    bool resolved = try_resolve_static_field(index, /*is_get=*/false
                                             JVM_NO_CHECK);
    GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION,
              "May GC but will not throw");
    if (!resolved) {
      __ uncommon_trap(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
      return;
    }
  }

  // Get the class id, the offset and the kind.
  int class_id, offset;
  BasicType kind = cp()->resolved_field_type_at(index, offset, class_id);
  if (byte_size_for(kind) < T_INT_byte_size) {
    // 8- and 16-bit static fields are always stored in a 32-bit
    // slot.
    kind = T_INT;
  }

  UsingFastOops fast_oops;
#if ENABLE_ISOLATES
  // We never issue an uncommon trap here because class initialization
  // must always be tested because of code sharing across isolates.
  InstanceClass::Fast klass = Universe::class_from_id(class_id);
#else

  InstanceClass::Fast klass;

#if USE_AOT_COMPILATION
  if (GenerateROMImage) {
    // Retrieve the class. For an uninitialized class generate 
    // initialization code.
    klass = get_klass_from_id_and_initialize(class_id JVM_CHECK);
  } else 
#endif
  {
    // Retrieve the initialized class or generate an uncommon trap
    klass = get_klass_or_null_from_id(class_id);
    if (klass.is_null()) {
      __ uncommon_trap(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
      return;
    }
  }

#endif //ENABLE_ISOLATES

  // Create the obj value
  Value klass_value(T_OBJECT);
#if ENABLE_ISOLATES
  bool needs_cib;
  cib_requirements(class_id, needs_cib, true);
  __ load_task_mirror(&klass, klass_value, needs_cib JVM_CHECK);
#else
  klass_value.set_obj(&klass);
#endif //ENABLE_ISOLATES
  // Here: the mirror is now loaded in klass_value.
  // Pop argument
  PoppedValue value(kind);

  // store the static field
  __ store_to_object(value, klass_value, offset, false JVM_NO_CHECK_AT_BOTTOM);
}

#if ENABLE_COMPILER_TYPE_INFO
// Use available information about value type, try to extract field type info.
void BytecodeCompileClosure::set_field_class_id(Value& field_value,
                                                InstanceClass * ic, 
                                                int field_offset,
                                                bool is_static) {
  GUARANTEE(field_value.type() == T_OBJECT || field_value.type() == T_ARRAY, 
            "Should only be invoked for object and array fields");
  // We have original field information if ROM generator is enabled
  TypeArray::Raw fields = 
    ENABLE_ROM_GENERATOR ? ic->original_fields() : ic->fields();
  const int fields_count = fields().length();

  for (int index = 0; index < fields_count; index += 5) {
#if ENABLE_ROM_GENERATOR
    OriginalField field(ic, index);
#else
    Field field(ic, index);
#endif

    if (field.offset() == field_offset) {
      const BasicType basic_type = field.type();
      // Note: there may be static and non-static fields with the same offset
      if (is_static == field.is_static() &&
          (basic_type == T_OBJECT || basic_type == T_ARRAY)) {
        FieldType::Raw field_type = field.signature();
        JavaClass::Raw field_class = field_type().object_type();
        field_value.set_class_id(field_class().class_id());
        if (field_class().is_final_type()) {
          field_value.set_is_exact_type();
        }
        break;
      }
    }
  }
}
#endif

void BytecodeCompileClosure::new_object(int index JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(new_object);

  UsingFastOops fast_oops;

  JavaClass::Fast klass;

#if ENABLE_ISOLATES
  // It is not required that the class be initialized, since this
  // is always tested because of code sharing across isolates.
  klass = get_klass_or_null(index, false JVM_CHECK);
#else

#if USE_AOT_COMPILATION
  if (GenerateROMImage) {
    // Retrieve the initialized class or generate an uncommon trap
    klass = get_klass_or_null(index, false JVM_CHECK);
    if (klass.not_null()) {
      InstanceClass holder = method()->holder();
      if (klass.is_instance_class() && !holder.is_subclass_of(&klass)) {
        InstanceClass ic = klass.obj();
        if (!ic.is_initialized()) {
          __ initialize_class(&ic JVM_CHECK);
        }
      }
    }
  } else
#endif
  {
    // Retrieve the initialized class or generate an uncommon trap
    klass = get_klass_or_null(index, true JVM_CHECK);
  }

#endif

  if (klass.is_null()
      || (!klass().is_instance_class()) 
      || klass().is_abstract()
      || klass().is_interface()) {
    // same checks as in newobject() in InterpreterRuntime.cpp
    __ uncommon_trap(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
    return;
  }

  // IMPL_NOTE: for cross-generator case we skip security check for now
  // as we work on trused classes
#if !CROSS_GENERATOR
  // <sender_class> trying to access <instance_class>
  InstanceClass::Fast instance_class = klass.obj();
  InstanceClass::Fast sender_class = method()->holder();
  instance_class().check_access_by(&sender_class, ErrorOnFailure JVM_NO_CHECK);
  if (CURRENT_HAS_PENDING_EXCEPTION) {
    Thread::clear_current_pending_exception();
    __ uncommon_trap(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
    return;
  }
#endif

#if ENABLE_ISOLATES
  bool needs_cib;
  cib_requirements(klass().class_id(), needs_cib, false);
  if (needs_cib) {
      __ check_cib(&klass JVM_CHECK);
      // note: check_cib is destroying klass_value.
  }
#endif // ENABLE_ISOLATES

  // Allocate
  Value result(T_OBJECT);
  __ new_object(result, &klass JVM_CHECK);

  result.set_must_be_nonnull();
#if ENABLE_COMPILER_TYPE_INFO
  result.set_class_id(klass().class_id());
  result.set_is_exact_type();
#endif

  // Push result
  frame_push(result);
}

ReturnOop BytecodeCompileClosure::get_klass_or_null(int cp_index,
                                                    bool must_be_initialized
                                                    JVM_TRAPS) {
  if (cp()->tag_at(cp_index).is_unresolved_klass()) {
    if (!ResolveConstantPoolInCompiler ||
        cp()->try_resolve_klass_at(cp_index) == NULL) {
      return (ReturnOop)NULL;
    }
  }

  // Get the class from the constant pool.
  JavaClass::Raw klass = cp()->klass_at(cp_index JVM_CHECK_0);

  if (must_be_initialized && klass.is_instance_class()) {
    InstanceClass::Raw ic = klass.obj();
    if (!ic().is_initialized()) {
      return (ReturnOop)NULL;
    }
  }
  return klass;
}

ReturnOop BytecodeCompileClosure::get_klass_or_null_from_id(int class_id) {
  JavaClass::Raw klass = Universe::class_from_id(class_id);
  if (klass.is_instance_class()) {
    InstanceClass::Raw ic = klass.obj();
    if (!ic().is_initialized()) {
      return (ReturnOop)NULL;
    }
  }

  return klass;
}

#if USE_AOT_COMPILATION && !ENABLE_ISOLATES
ReturnOop 
BytecodeCompileClosure::get_klass_from_id_and_initialize(int class_id 
                                                         JVM_TRAPS) {
  GUARANTEE(GenerateROMImage, 
            "Should be called only for AOT compilation");
  JavaClass klass = Universe::class_from_id(class_id);
  InstanceClass holder = method()->holder();
  if (klass.is_instance_class() && !holder.is_subclass_of(&klass)) {
    InstanceClass ic = klass.obj();
    if (!ic.is_initialized()) {
      __ initialize_class(&ic JVM_CHECK_(NULL));
    }
  }

  return klass;
}
#endif // USE_AOT_COMPILATION

void BytecodeCompileClosure::new_object_array(int index JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(new_object_array);

  UsingFastOops fast_oops;

  JavaClass::Fast klass = get_klass_or_null(index, false JVM_CHECK);
  if (klass.is_null()) {
    __ uncommon_trap(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
    return;
  }

  // IMPL_NOTE: see comment for new_object
#if !CROSS_GENERATOR
  // <sender_class> trying to access <klass>
  InstanceClass::Fast sender_class = method()->holder();
  klass().check_access_by(&sender_class, ErrorOnFailure JVM_NO_CHECK);
  if (CURRENT_HAS_PENDING_EXCEPTION) {
    Thread::clear_current_pending_exception();
    __ uncommon_trap(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
    return;
  }
#endif

  // Pop argument
  PoppedValue length(T_INT);

  int known_length = length.is_immediate() ? length.as_int() : -1;

  // Allocate
  Value result(T_ARRAY);
  __ new_object_array(result, &klass, length JVM_CHECK);
  // Push result
  result.set_must_be_nonnull();
  if (known_length > 0) {
    result.set_has_known_min_length(known_length);
  }
#if ENABLE_COMPILER_TYPE_INFO
  ArrayClass::Raw array_class = klass().array_class();
  if (array_class.not_null()) {
    result.set_class_id(array_class().class_id());
    result.set_is_exact_type();
  }
#else
  if (klass.equals(Universe::object_class())) {
    result.set_is_object_array();
  } else if (klass.equals(Universe::string_class())) {
    result.set_is_string_array();
  }
#endif
  frame_push(result);
}

void BytecodeCompileClosure::new_basic_array(int type JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(new_basic_array);
  const BasicType basic_type = (BasicType)type;

  // Pop argument
  PoppedValue length(T_INT);

  int known_length = length.is_immediate() ? length.as_int() : -1;

  // Allocate
  Value result(T_ARRAY);
  __ new_basic_array(result, basic_type, length JVM_CHECK);

  // Push result
  result.set_must_be_nonnull();
  if (known_length > 0) {
    result.set_has_known_min_length(known_length);
  }
#if ENABLE_COMPILER_TYPE_INFO
  result.set_class_id(Universe::as_TypeArrayClass(basic_type)->class_id());
  result.set_is_exact_type();
#endif
  frame_push(result);
}

void BytecodeCompileClosure::new_multi_array(int /*index*/, int num_of_dims
                                             JVM_TRAPS)
{
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(new_multi_array);

  // Allocate
  Value result(T_ARRAY);
  __ new_multi_array(result JVM_CHECK);

  // Remove arguments
  for (int i = 0; i < num_of_dims; i++) {
    PoppedValue tmp(T_INT);
  }

  // Push result
  result.set_must_be_nonnull();
  frame_push(result);
}

void BytecodeCompileClosure::monitor_enter(JVM_SINGLE_ARG_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(monitor_enter);

  // Pop argument
  PoppedValue object(T_OBJECT);

  if (object.must_be_null()) {
    throw_null_pointer_exception(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  } else {
    // Null check
    __ maybe_null_check(object JVM_CHECK);

    // Monitor operation
    __ monitor_enter(object JVM_NO_CHECK_AT_BOTTOM);
  }
}

void BytecodeCompileClosure::monitor_exit(JVM_SINGLE_ARG_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(monitor_exit);

  // Pop argument
  PoppedValue object(T_OBJECT);

  if (object.must_be_null()) {
    throw_null_pointer_exception(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  } else {
    // Null check
    __ maybe_null_check(object JVM_CHECK);

    // Monitor operation
    __ monitor_exit(object JVM_NO_CHECK_AT_BOTTOM);
  }
}

void BytecodeCompileClosure::invoke_static(int index JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(invoke_static);

  jubyte tag = get_invoker_tag(index, Bytecodes::_invokestatic 
                               JVM_MUST_SUCCEED);

  if (ConstantTag::is_resolved_static_method(tag)) {
    direct_invoke(index, false JVM_NO_CHECK_AT_BOTTOM);
  } else {
    __ uncommon_trap(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  }
}

void BytecodeCompileClosure::do_direct_invoke(Method * callee, 
                                              bool must_do_null_check
                                              JVM_TRAPS) {
  const int size_of_parameters = callee->size_of_parameters();

  if (!callee->is_static() && 
      frame()->receiver_must_be_null(size_of_parameters)) {

    throw_null_pointer_exception(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
    return;
  }

  if (must_do_null_check && 
      frame()->receiver_must_be_nonnull(size_of_parameters)) {
    must_do_null_check = false;
  }

  if (callee->is_fast_get_accessor()) {
    if (TraceMethodInlining) {
      tty->print("Method ");
      callee->print_name_on_tty();
      tty->print(" inlined in ");
      method()->print_name_on_tty();
      tty->cr();
    }
    // Pop argument
    PoppedValue receiver(T_OBJECT);

    // Load field
    Value result(callee->fast_accessor_type());

    __ load_from_object(result, receiver, callee->fast_accessor_offset(),
                        must_do_null_check JVM_CHECK);
    // Push result
    frame_push(result);
    return;
  }
  
#if ENABLE_INLINE
  //we cannot call trace_bytecode from inlined method, 
  //so we must prohibit method inlining in case TraceBytecodesCompiler
  if (!TraceBytecodesCompiler) { 
    Method::Attributes method_attributes;
    bool can_be_inline = !callee->is_impossible_to_compile() &&
      callee->bytecode_inline_prepass(method_attributes JVM_CHECK); 
    if (can_be_inline) {
      UsingFastOops fast_oops;

      RegisterAllocator::guarantee_all_free();

      InstanceClass::Fast caller_holder = method()->holder();
      InstanceClass::Fast callee_holder = callee->holder();
      int needed_virtual_frame_space = callee->max_execution_stack_count() 
          - size_of_parameters;
      // 3 more locations is allocated 
      // please refer to VirtualStackFrame::create(Method* method JVM_TRAPS)
      // 5 + max_execution_stack_count should be
      // max rawlocation space reserved in caller VF.
      //- method()->size_of_parameters(), local variable part 1
      //- frame()->virtual_stack_pointer()
      int remaining_virtual_frame_space = 
        (5 + method()->max_execution_stack_count() -
         (1 + frame()->virtual_stack_pointer())) ;
      bool is_virtual_frame_space_enough = 
        remaining_virtual_frame_space >= needed_virtual_frame_space;
        
      if (is_virtual_frame_space_enough) {
        if (TraceMethodInlining) {
          tty->print("Method ");
          callee->print_name_on_tty();
          tty->print(" inlined in ");
          method()->print_name_on_tty();
          tty->cr();
        }
          
        if (must_do_null_check) {
          GUARANTEE(!frame()->receiver_must_be_nonnull(size_of_parameters),
                    "Null check not needed");
          Value receiver(T_OBJECT);
          frame()->receiver(receiver, size_of_parameters);
          // IMPL_NOTE: use maybe_null_check_1/2 on ARM
          code_generator()->maybe_null_check(receiver JVM_CHECK);
        } else {
          //do nothing
        }

        //create a new compiler to compile the inlined method
        Compiler compiler(callee, 0);
            
        compiler.internal_compile_inlined(method_attributes 
                                          JVM_NO_CHECK_AT_BOTTOM);

        return;        
      }
    }
  }
#endif

  __ invoke(callee, must_do_null_check JVM_NO_CHECK_AT_BOTTOM);
}

void BytecodeCompileClosure::direct_invoke(int index, bool must_do_null_check
                                           JVM_TRAPS)
{
  UsingFastOops fast_oops;

  GUARANTEE(cp()->tag_at(index).is_resolved_static_method() || 
            cp()->tag_at(index).is_resolved_final_uncommon_interface_method(),
            "sanity");

  // Get the target method.
  Method::Fast callee = cp()->resolved_static_method_at(index);
  InstanceClass::Fast holder = callee().holder();

  if (is_active_bci()) {
    osr_entry(JVM_SINGLE_ARG_CHECK);
  }

#if ENABLE_ISOLATES
  if (callee().is_static()){
    bool needs_cib;
    cib_requirements(callee().holder_id(), needs_cib, false);
    if (needs_cib){
        __ check_cib(&holder JVM_CHECK);
    }
  }
#else
  // !ENABLE_ISOLATES
  if (!holder().is_initialized()) {
#if USE_AOT_COMPILATION
    if (GenerateROMImage) {
      InstanceClass caller_holder = method()->holder();
      if (!caller_holder.is_subclass_of(&holder)) {
        __ initialize_class(&holder JVM_CHECK);
      }
    } else
#endif
    {
      // This is probably a method in ROM -- We always resolve the CP
      // entry, but the class may still be not yet initialized.
      __ uncommon_trap(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
      return;
    }
  }
#endif

  if (callee().match(Symbols::object_initializer_name(),
                     Symbols::void_signature())) {
    InstanceClass::Raw c = callee().holder();
    if (c().has_vanilla_constructor()) {
      // Pop the receiver.
      PoppedValue receiver(T_OBJECT);
      return;
    }
  }

  Symbol::Fast    name;
  Signature::Fast signature;
  BasicType       type;
  if (holder.equals(Universe::math_class())) {
    // We've got something in java.lang.Math.  Find out its name
    // and its return type
    name   = callee().name();
    signature = callee().signature();
    type  = signature().return_type();
    if (name.equals(Symbols::abs_name())) {
      // abs() is implemented for all four primitive types
      unary(type, una_abs JVM_NO_CHECK_AT_BOTTOM); 
      return;
    } else if (type == T_INT || type == T_LONG) {
      // min and max are implemented only for INT and LONG
      if (name.equals(Symbols::min_name())) { 
          binary(type, bin_min JVM_NO_CHECK_AT_BOTTOM); 
          return;
      } else if (name.equals(Symbols::max_name())) { 
          binary(type, bin_max JVM_NO_CHECK_AT_BOTTOM); 
          return;
      }
    }
  }

#if ENABLE_INLINED_ARRAYCOPY
  // Check for System.arraycopy()
  if (holder.equals(Universe::system_class())) {
    name = callee().name();
    if (name.equals(Symbols::arraycopy_name())) {
#ifdef AZZERT
      const int virtual_stack_pointer = frame()->virtual_stack_pointer();
#endif

      const bool done = __ arraycopy(JVM_SINGLE_ARG_CHECK);
      if (done) {
        return;
      }

      GUARANTEE(frame()->virtual_stack_pointer() == virtual_stack_pointer,
                "Arguments must remain on the stack");
    }
  }

  // Check for unchecked arraycopy entries
  if (holder.equals(Universe::jvm_class())) {
    name = callee().name();
    BasicType array_element_type = T_ILLEGAL;
    if (name.equals(Symbols::unchecked_byte_arraycopy_name())) {
      array_element_type = T_BYTE;
    }
    if (name.equals(Symbols::unchecked_char_arraycopy_name())) {
      array_element_type = T_CHAR;
    }
    if (name.equals(Symbols::unchecked_int_arraycopy_name())) {
      array_element_type = T_INT;
    }
    if (name.equals(Symbols::unchecked_obj_arraycopy_name())) {
      array_element_type = T_OBJECT;
    }

    if (array_element_type != T_ILLEGAL) {
#ifdef AZZERT
      const int virtual_stack_pointer = frame()->virtual_stack_pointer();
#endif

      const bool done = __ unchecked_arraycopy(array_element_type JVM_CHECK);
      if (done) {
        return;
      }

      GUARANTEE(frame()->virtual_stack_pointer() == virtual_stack_pointer,
                "Arguments must remain on the stack");
    }
  }
#endif

  do_direct_invoke(&callee, must_do_null_check JVM_NO_CHECK_AT_BOTTOM);
}

void BytecodeCompileClosure::invoke_interface(int index, int num_of_args 
                                              JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(invoke_interface);

  UsingFastOops fast_oops;

  jubyte tag = get_invoker_tag(index, Bytecodes::_invokeinterface 
                               JVM_MUST_SUCCEED);
  if (!ConstantTag::is_resolved_interface_method(tag)) {
    __ uncommon_trap(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
    return;
  }

  // Get the class index from the resolved constant pool entry.
  int class_id, itable_index;
  cp()->resolved_interface_method_at(index, itable_index, class_id);

  // Get the class from the constant pool.
  JavaClass::Fast klass = Universe::class_from_id(class_id);
  Value receiver(T_OBJECT);
  frame()->receiver(receiver, num_of_args);

  if (receiver.must_be_null()) {
    throw_null_pointer_exception(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  } else {
    // Make sure that invoke_interface can use whatever registers it
    // wants to
    receiver.destroy();
    // Retrieve the result type from the constant pool tag
    BasicType result_type= cp()->tag_at(index).resolved_interface_method_type();

    if (is_active_bci()) {
      osr_entry(JVM_SINGLE_ARG_CHECK);
    }

    // Call the method.
    __ invoke_interface(&klass, itable_index, num_of_args, result_type 
                        JVM_NO_CHECK_AT_BOTTOM);
  }
}

void BytecodeCompileClosure::fast_invoke_virtual(int index JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(fast_invoke_virtual);

  UsingFastOops fast_oops;

  int vtable_index;
  int class_id;
  BasicType return_type = cp()->resolved_virtual_method_at(index, vtable_index,
                                                          class_id);

  if (is_active_bci()) {
    osr_entry(JVM_SINGLE_ARG_CHECK);
  }

  // Get the class from the constant pool.
  JavaClass::Fast klass = Universe::class_from_id(class_id);
  ClassInfo::Fast info = klass().class_info();
  Method::Fast callee = info().vtable_method_at(vtable_index);
  
#if ENABLE_COMPILER_TYPE_INFO
  Value receiver(T_OBJECT);

  frame()->receiver(receiver, callee().size_of_parameters());

  const jushort receiver_class_id = receiver.class_id();
  JavaClass::Fast receiver_class = Universe::class_from_id(class_id);

  const bool can_devirtualize = 
    receiver.is_exact_type() || receiver_class().is_final_type();

  receiver.destroy();

  if (can_devirtualize) {
    if (receiver_class_id != class_id) {
      klass = Universe::class_from_id(receiver_class_id);
      info = klass().class_info();
      GUARANTEE(0 <= vtable_index && vtable_index < info().vtable_length(), 
                "Vtable index out of bounds");
      callee = info().vtable_method_at(vtable_index);
    }
    if (TraceMethodInlining) {
      tty->print("Method ");
      callee().print_name_on_tty();
      tty->print(" devirtualized in ");
      method()->print_name_on_tty();
      tty->cr();
    }

    do_direct_invoke(&callee, true/*need null check*/ JVM_NO_CHECK_AT_BOTTOM);

    return;
  }
#endif

#if ENABLE_INLINE
  GUARANTEE(klass().is_instance_class(), "Sanity");
  InstanceClass::Fast ic = klass.obj();
  // Can devirtualize call only if 
  //  - callee is not overridden and
  //  - caller is not precompiled and
  //  - caller cannot be shared between tasks
  if (!ic().is_method_overridden(vtable_index) &&
      !GenerateROMImage && !method()->is_shared()) {
    if (TraceMethodInlining) {
      tty->print("Method ");
      callee().print_name_on_tty();
      tty->print(" devirtualized in ");
      method()->print_name_on_tty();
      tty->cr();
    }

    do_direct_invoke(&callee, true/*need null check*/ JVM_CHECK);
    callee().add_direct_caller(method() JVM_NO_CHECK_AT_BOTTOM);
  } else
#endif
  {
    // Call the method.
    __ invoke_virtual(&callee, vtable_index, return_type 
                      JVM_NO_CHECK_AT_BOTTOM);
  }
}

void BytecodeCompileClosure::fast_invoke_special(int index JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(fast_invoke_special);
  if (is_active_bci()) {
    osr_entry(JVM_SINGLE_ARG_CHECK);
  }

  UsingFastOops fast_oops;
  int vtable_index;
  int class_id;
  cp()->resolved_virtual_method_at(index, vtable_index, class_id);

  // Get the class from the constant pool.
  JavaClass::Fast klass = Universe::class_from_id(class_id);
  ClassInfo::Fast info = klass().class_info();
  Method::Fast method = info().vtable_method_at(vtable_index);

  __ invoke(&method, true JVM_NO_CHECK_AT_BOTTOM);
}

void BytecodeCompileClosure::invoke_special(int index JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(invoke_special);

  jubyte tag = get_invoker_tag(index, Bytecodes::_invokespecial 
                               JVM_MUST_SUCCEED);

  if (ConstantTag::is_resolved_static_method(tag)) {
    // CR 4862713/6324543, If method was resolved by some good reference but
    // this one is some hacked class file with an invokespecial opcode
    // but the index of a static method then we must check this case.
    Method::Raw m = cp()->resolved_static_method_at(index);
    GUARANTEE(!m.is_null(), "Resolved method is null");
    if (m().is_static()) {
      __ uncommon_trap(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
      return;
    } 
    direct_invoke(index, true JVM_NO_CHECK_AT_BOTTOM);
  } else if (ConstantTag::is_resolved_virtual_method(tag)) {
    fast_invoke_special(index JVM_NO_CHECK_AT_BOTTOM);
  } else {
    __ uncommon_trap(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  }
}

void BytecodeCompileClosure::invoke_virtual(int index JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(invoke_virtual);

  jubyte tag = get_invoker_tag(index, Bytecodes::_invokevirtual 
                               JVM_MUST_SUCCEED);
  if (ConstantTag::is_resolved_static_method(tag)) {
    // CR 4862713/6324540, If method was resolved by some good reference but
    // this one is some hacked class file with an invokevirtual opcode
    // but the index of a static method then we must check this case.
    Method::Raw m = cp()->resolved_static_method_at(index);
    GUARANTEE(!m.is_null(), "Resolved method is null");
    if (m().is_static()) {
      __ uncommon_trap(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
      return;
    }
    direct_invoke(index, true JVM_NO_CHECK_AT_BOTTOM);
  } else if (ConstantTag::is_resolved_virtual_method(tag)) {
    fast_invoke_virtual(index JVM_NO_CHECK_AT_BOTTOM);
  } else {
    __ uncommon_trap(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  }
}

void BytecodeCompileClosure::fast_invoke_virtual_final(int index JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(fast_invoke_virtual_final);
  direct_invoke(index, true JVM_NO_CHECK_AT_BOTTOM);
}

void BytecodeCompileClosure::invoke_native(BasicType return_kind, 
                                           address entry JVM_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(invoke_native);
  __ invoke_native(return_kind, entry JVM_NO_CHECK_AT_BOTTOM);
}

jubyte BytecodeCompileClosure::get_invoker_tag(int index, 
                                               Bytecodes::Code bytecode
                                               JVM_TRAPS) {
  jubyte tag = cp()->tag_value_at(index);

  if( ConstantTag::is_method(tag) ) {
    const bool status = try_resolve_invoker(index, bytecode JVM_MUST_SUCCEED);
    if( status || bytecode != Bytecodes::_invokestatic ) {
      tag = cp()->tag_value_at(index);
    }
  }

  return tag;
}

bool BytecodeCompileClosure::try_resolve_invoker(int index,
                                                 Bytecodes::Code bytecode
                                                 JVM_TRAPS) {
  if (!ResolveConstantPoolInCompiler) {
    return false;
  }

  UsingFastOops fast_oops;
  InstanceClass::Fast caller_class = method()->holder();
  bool ok = true, callee_class_inited;

  ConstantPool::suspend_class_loading();
  {
    switch (bytecode) {
    case Bytecodes::_invokestatic:
      callee_class_inited =
        cp()->resolve_invoke_static_at(&caller_class, index, false JVM_NO_CHECK);
      if (!callee_class_inited) {
        ok = false;
      }
      break;
    case Bytecodes::_invokespecial:
      cp()->resolve_invoke_special_at(&caller_class, index JVM_NO_CHECK);
      break;
    case Bytecodes::_invokevirtual:
      cp()->resolve_invoke_virtual_at(&caller_class, index JVM_NO_CHECK);
      break;
    case Bytecodes::_invokeinterface:
      cp()->resolve_invoke_interface_at(&caller_class, index JVM_NO_CHECK);
      break;
    default:
      SHOULD_NOT_REACH_HERE();
    }
  }
  ConstantPool::resume_class_loading();

  if (CURRENT_HAS_PENDING_EXCEPTION) {
    Thread::clear_current_pending_exception();
    ok = false;
  }

  return ok;
}

void BytecodeCompileClosure::bytecode_prolog(JVM_SINGLE_ARG_TRAPS) {
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(bytecode_prolog);

  bool overflown = __ has_overflown_compiled_method();
  bool has_handlers = _has_exception_handlers;

  if (overflown) {
    GUARANTEE( __ has_overflown_compiled_method(), "sanity");
    Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW);
  } else {
    GUARANTEE( !(__ has_overflown_compiled_method()), "sanity");
  }

  __ bytecode_prolog();

#if !defined(PRODUCT) || USE_COMPILER_COMMENTS
  if (GenerateCompilerComments) {
    FixedArrayOutputStream output;
    Verbose++; // force callee names to be printed out, etc.
    method()->print_bytecodes(&output, bci());
    Verbose--;
    __ comment(output.array());
  }
  if (TraceBytecodesCompiler) {
    PreserveVirtualStackFrameState preserve(frame() JVM_CHECK);
    __ call_vm((address)trace_bytecode, T_VOID JVM_NO_CHECK_AT_BOTTOM);
  }
#endif
  if (Deterministic) {
    __ check_bytecode_counter();
  }
  if (has_handlers) {
    check_exception_handler_start(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  }
}

void 
BytecodeCompileClosure::check_exception_handler_start(JVM_SINGLE_ARG_TRAPS) {
  // Look to see if this is the start of an exception handler
  UsingFastOops fast_oops;
  TypeArray::Fast exception_table = method()->exception_table();
  GUARANTEE((exception_table().length() % 4) == 0, "Sanity check");
  for (int i = exception_table().length() - 4; i >= 0; i-=4) {
    const int start_bci = exception_table().ushort_at(i);
    if( bci() == start_bci &&
            !Compiler::current()->exception_has_osr_entry(start_bci) ) {
      Compiler::current()->set_exception_has_osr_entry(start_bci);
      const int handler_bci = exception_table().ushort_at(i + 2);

      VirtualStackFrame* exception_frame = 
        frame()->clone_for_exception(handler_bci JVM_ZCHECK(exception_frame));

      VirtualStackFrame* old_frame = frame();
      Compiler::set_frame( exception_frame );
      {
        BinaryAssembler::Label unused;

        // Compiler at this location, and make it an OSR entry
        CompilationContinuation* stub =
          CompilationContinuation::insert( handler_bci, unused JVM_NO_CHECK );
        if( stub ) {
          stub->set_need_osr_entry();
          stub->set_is_exception_handler();
        }
      }
      Compiler::set_frame( old_frame );
    }
  }
}

void BytecodeCompileClosure::bytecode_epilog(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(bytecode_epilog);

#ifndef PRODUCT
  if (GenerateCompilerComments && DumpVSFInComments) {
    // a frame->dump() is potentially quite expensive to compute, so we
    // avoid computing it unless asked to generate compiler comments.
    frame()->dump(true);
    frame()->dump_fp_registers(true);
  }
#endif
}

void BytecodeCompileClosure::throw_null_pointer_exception(JVM_SINGLE_ARG_TRAPS) {
  NullCheckStub* error =
    NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_NO_CHECK);
  if( error ) {
    __ jmp( error );
    terminate_compilation();
  }
}

void BytecodeCompileClosure::array_check(Value& array, Value& index JVM_TRAPS) {
  if (index.is_immediate()) {
    int length;
    if (array.has_known_min_length(length) &&
        index.as_int() < length  &&
        index.as_int() >= 0) {
#ifndef PRODUCT
      __ comment("Elide index check");
#endif
#if ENABLE_NPCE
      __ maybe_null_check_by_npce(array, false, false, T_INT JVM_CHECK);
#else 
      __ maybe_null_check(array JVM_CHECK);
#endif
    } else { 
      __ array_check(array, index JVM_NO_CHECK_AT_BOTTOM);
      array.set_has_known_min_length(index.as_int() + 1);
      frame()->set_value_has_known_min_length(array, index.as_int() + 1);
    }
  } else { 
   __ array_check(array, index JVM_NO_CHECK_AT_BOTTOM);
  }
}

#if !ENABLE_CPU_VARIANT
void BytecodeCompileClosure::init_static_array(JVM_SINGLE_ARG_TRAPS) {  
  COMPILER_PERFORMANCE_COUNTER_IN_BLOCK(init_static_array);

  Value result(T_ARRAY);
  frame()->value_at(result, frame()->virtual_stack_pointer());

  __ init_static_array(result JVM_CHECK);
}
#endif //!ENABLE_CPU_VARIANT
#undef __

inline void
BytecodeCompileClosure::set_default_next_bytecode_index(const jint bci) {
  const Bytecodes::Code bc = bytecode_at(bci);
  if (Bytecodes::can_fall_through(bc)) {
    set_next_bytecode_index( next_bci(bci) );
  } else {
    terminate_compilation();
  }
}

bool BytecodeCompileClosure::compile(JVM_SINGLE_ARG_TRAPS) {
  const int bci = Compiler::bci();
  GUARANTEE( bci >= 0 && bci < method_size(),
            "Bytecode index must be within bounds");
    code_generator()->ensure_compiled_method_space();

  // Set the next bytecode index to be the default one - this can be
  // overwritten during the compilation.
  set_default_next_bytecode_index(bci);

  const Bytecodes::Code code = bytecode_at(bci);
  if( !code_eliminate_prologue(code) ) {
    // Compile current bytecode
    // Must use Compiler::bci() because bci could change
    method()->iterate_bytecode(Compiler::bci(), this, code JVM_CHECK_(false));
    code_eliminate_epilogue(code);

    // Update current bytecode index
    Compiler::set_bci(next_bytecode_index());
  }

  // Return whether or not the compilation should continue.
  return !is_compilation_done();
}

// If next bytecode can be read ahead, returns its index, otherwise -1
int BytecodeCompileClosure::get_next_bci( void ) const {
  const int nextbci = next_bci( bci() );
  return nextbci < method_size() &&
         Compiler::current()->entry_count_for(nextbci) == 1
         ? nextbci : -1;
}

#if USE_DEBUG_PRINTING
void BytecodeCompileClosure::print_on(Stream *st) {
  st->print_cr("bci                    = %d", bci());
  st->print_cr("active_bci             = %d", _active_bci);
  st->print_cr("has_exception_handlers = %d", _has_exception_handlers);
  st->print_cr("has_clinit             = %d", _has_clinit);
}
void BytecodeCompileClosure::p()  {
  print_on(tty);
}
#endif

#if ENABLE_CSE
void BytecodeCompileClosure::record_passable_statue_of_osr_entry(int dest) {
   if (dest >= bci() && is_active_bci()) {
     //for non loop osr
     //we should try to keep the cse values
     VERBOSE_CSE(("mask osr passable bci = %d", Compiler::current()->bci()));
     VirtualStackFrame::mark_as_passable();
   }
} 

bool BytecodeCompileClosure::code_eliminate_prologue(Bytecodes::Code code) {
  // IMPL_NOTE: need to revisit for inlining
  if (!Compiler::is_inlining()) {
    jint bci_after_elimination = not_found;
    //can_decrease_stack mean the code will
    //consume the items of the operation stack
    //since we only target the byte code related to
    //memory acces and arithmetic
    if (Bytecodes::can_decrease_stack(code) && 
        is_following_codes_can_be_eliminated(bci_after_elimination)) {
  
      GUARANTEE(bci_after_elimination != not_found, "cse match error")
      //next bci to be compiled, cse will skip some byte code here.
      set_default_next_bytecode_index(bci_after_elimination);
      Compiler::set_bci(next_bytecode_index());
      //skip the compilation of current bci
      return true;
    }
    
    method()->wipe_out_dirty_recorded_snippet(Compiler::bci(), code);
  }

  //if we aborted in previous bci, we reset and start a new code snippet tracking.
  VirtualStackFrame::mark_as_not_aborted();
  return false;
}

void BytecodeCompileClosure::code_eliminate_epilogue(Bytecodes::Code code) {
  // IMPL_NOTE: need to revisit for inlining
  if (!Compiler::is_inlining()) {
    if (Bytecodes::is_kind_of_eliminable(code)) {
      record_current_code_snippet();       
    }
  }
}

void BytecodeCompileClosure::record_current_code_snippet(void) {
  jint begin;
  jint end = bci();

  // IMPL_NOTE: need to revisit for inlining
  GUARANTEE(!Compiler::is_inlining(), "Unsupported for inlining");
  Method * method = Compiler::root()->method();

  //track the dependency between 
  //eliminatable byte codes string 
  //and java local variables(field, array)
  jint local_mask = 0;
  jint constant_mask = 0;
  jint array_type = 0;
                                           //leftest bci
  begin = VirtualStackFrame::first_bci_of_current_snippet();

  //current byte code can't be CSE passable.
  //the tracked status of tag stack should be cleared
  //For example:
  //        o o
  //        |/
  //        o  <-- tag(field modified by abort() method) status must be cleaned here(multipule entries) 
  //        |
  //        o  
  if (VirtualStackFrame::is_aborted_from_tracking()) {
    return ;
  }

  //reach the end of byte code, 
  //no more CSE needed
  if (end >= method->code_size()) {
    return;
  }

  //if the operation stack is empty
  //we don't do CSE, because we 
  //have no register information 
  //in this cases
  if (method->is_local(frame()->virtual_stack_pointer())) {
    return;
  }

  //The bci should start from zero. So if there's a pop happen and we still 
  //get -1 here,we should abort the notation of current byte codes.
  if (begin == -1 ) {
    //is a pop action really happen on execution stack.       
    if (VirtualStackFrame::is_popped()) {
      return;
    } else {
      //The result is caused by one byte code without poping value from stack. 
      // like new bci like aload_0_fast_get_field_1...
      begin = end;
    }
  }

  // fall through back, 
  // so we abandon this 
  //We only record the offset and length of a snippet.
  //so we don't handle  this case if begin > end, since the length of the notation will be negative.
  if (begin > end) {
    VirtualStackFrame::abort_tracking_of_current_snippet();
    return;
  }
   if (!method->is_snippet_can_be_elminate(begin, end, local_mask, constant_mask, 
                                       array_type)) {
    return;
  }

  //found the register holding the result
  Value cached(frame()->expression_stack_type(0));
  if (cached.is_two_word()) {
    return;
  }
  int old_code_size = code_generator()->code_size();
  frame()->value_at(cached, frame()->virtual_stack_pointer());
  if (!cached.in_register()) {
    return;
  }

  int new_code_size = code_generator()->code_size();
  GUARANTEE(new_code_size == old_code_size, "extra load instruction is emitted");
  
  //generate notation
  RegisterAllocator::Register reg = cached.lo_register();
  RegisterAllocator::set_array_element_type(reg, array_type);
  RegisterAllocator::set_notation(reg, begin, end - begin + 1);
  RegisterAllocator::set_locals(reg, local_mask);
  RegisterAllocator::set_constants(reg, constant_mask);
  RegisterAllocator::set_type(reg, cached.type());
  VERBOSE_CSE(("mark byte code snippet [%d, %d]",begin, end));
  RegisterAllocator::dump_notation(reg);
}

bool BytecodeCompileClosure::is_following_codes_can_be_eliminated(jint& bci_after_elimination) {
   jint begin_bci;
   jint end_bci;
   jint longest_next_bci = 0;
   jint longest_length = 0;
   jint bci = Compiler::bci();
   Assembler::Register longest = Assembler::first_register;
   Assembler::Register reg = Assembler::first_register;
   if ( !RegisterAllocator::is_notated()) {
      return false;
   }

   //visit each notation in the register notation table.
   //for each notation: 1. get its byte code sequence.
   // 2. compare the byte codes start from current bci with the ones of the notation
   //if a same one is found, we may skip the compilation of those byte codes by 
   //reuse the value in the register.
   //the byte code sequence in the notation start from the 
   //leftest bci that the value is computed from. For example, ((3+5)-7), 
   //we record the bci of  iconst 3.
   for (reg ;
        reg <= Assembler::last_register;
        reg = (Assembler::Register) ((int) reg + 1)) {
     if ( RegisterAllocator::is_notated(reg)) {
           RegisterAllocator::get_notation(reg, begin_bci, end_bci);
       if( Compiler::current()->method()->compare_bytecode(
             begin_bci, end_bci, bci, bci_after_elimination) ) { 
        //one match;
        if ( (end_bci - begin_bci) > longest_length) {
           if ( longest_length != 0 ) {
             VERBOSE_CSE(("found one more code snippet\n"));
           } 
           //try to find the longest matching sequence 
           //if (1+2) and  (1+2)+3 are both cached, 
           //and current byte codes are (1+2)+3 
           //we will try to match (1+2)+3 here.
           longest_length = end_bci - begin_bci;
           longest = reg;
           longest_next_bci = bci_after_elimination;
         }
       }
     }
   }

   if ( longest_length == 0) {
    return false;
   }
   
   reg = longest;
   RegisterAllocator::get_notation(reg, begin_bci, end_bci);
   bci_after_elimination = longest_next_bci;
   //wrapper the register into a value and push on the stack.
   {
     BasicType type = (BasicType)RegisterAllocator::get_type(reg);
     Value matched_value( type ); 
     GUARANTEE(type >= T_BOOLEAN && type <=T_OBJECT, "Error register type");
     //increase the reference
     RegisterAllocator::reference(reg);
     matched_value.set_register(reg);
     frame()->push(matched_value);
     if(VerboseByteCodeEliminate) { 
       VERBOSE_CSE(("hit cache"));
       RegisterAllocator::dump_notation(reg);
     }
     return true;
   }
}
#endif

#endif //ENABLE_COMPILER
