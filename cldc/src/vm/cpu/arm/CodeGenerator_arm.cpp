/*
 *
 * Portions Copyright  2003-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#if !ENABLE_THUMB_COMPILER
#if ENABLE_COMPILER
#include "incls/_CodeGenerator_arm.cpp.incl"

class TempRegister {
private:
    Assembler::Register _reg;
public:
    TempRegister() {
      _reg = RegisterAllocator::allocate();
    }
    TempRegister(Assembler::Register reg)  : _reg(reg) {
      RegisterAllocator::allocate(reg);
    }
    ~TempRegister() { RegisterAllocator::dereference(_reg); }

    operator Assembler::Register() { return _reg; }

    // Simple accessor as a workaround for above UDC
    Assembler::Register reg() { return _reg; }
};

class CompilerLiteralAccessor : public LiteralAccessor {
public:
  virtual bool has_literal(int imm32, Assembler::Address1& result);

  virtual Assembler::Register get_literal(int imm32) {
    return frame()->get_literal(imm32, *this);
  }
private:
  VirtualStackFrame* frame() {
    return Compiler::frame();
  }
};

void CodeGenerator::call_through_gp(address& target, bool speed JVM_TRAPS) {
  long offset = (long)&target - (long)&gp_base_label;
  call_from_compiled_code(gp, (int) offset, 0, /* indirect */true, speed
                          JVM_NO_CHECK_AT_BOTTOM);
}

#if ENABLE_ISOLATES

// Load task mirror, perform class initialization before hand if needed.
// Barrier is not necessary when the class that holds the static variables:
//  - is a ROMIZED class,
//  - has no static initializer
//  - has a static initializer but it does not invoke any methods (therefore
//  no compiled method can see the class in the being initialized state)
//  Methods that are used in static initializers and that may hit a barrier
//  may also be tagged so that only tagged methods have to clear a barrier.
//  This however is harder to achieve.
//
//
void CodeGenerator::load_task_mirror(Oop*klass, Value& statics_holder,
                                     bool needs_cib JVM_TRAPS)
{
  {
    JavaClass::Raw jc = klass;
    statics_holder.assign_register();
    get_mirror_list_base(statics_holder.lo_register());
    // FieldAddress might destroy the value, so we create a defensive copy.
    Value statics_holder_copy;
    statics_holder.copy(statics_holder_copy);
    FieldAddress address(statics_holder_copy,
                         jc().class_id() * sizeof(OopDesc *), T_OBJECT);
    ldr(statics_holder.lo_register(), address.lo_address_2());
  }
  if (needs_cib) {
    Label class_is_initialized, need_init;
    // Can we make the flush conditional for  get/put static ?
    //  see if register usage cross compiled bytecode.
    flush_frame();
    {
      // The marker cannot be treated as a constant value, as it would break 
      // cross-compilation. Thus we load it from GP table.
      TempRegister tmp;
      get_task_class_init_marker(tmp);
      cmp(statics_holder.lo_register(), reg(tmp));
    }
    b(class_is_initialized, ne);
bind(need_init);
    // Call the runtime system.
    // pass klass as extra args, move in correct register beforehand
    // if necessary
    // Passing the klass in parameter.
    {
      // KEEP these brackets: without them the klass_parameter's destructor
      // would not be called before call_vm and cause an error.
      Value klass_parameter(T_OBJECT);
      klass_parameter.set_obj(klass);
      if (klass_parameter.lo_register() != r1) {
        call_vm_extra_arg(klass_parameter.lo_register());
      }
    }
    call_vm((address) compiled_code_task_barrier, T_OBJECT JVM_CHECK);
    // Need to move mirror to expected register
    if (statics_holder.lo_register()!= r0) {
      mov(statics_holder.lo_register(), reg(r0));
    }
bind(class_is_initialized);
  }
}

void CodeGenerator::check_cib(Oop *klass JVM_TRAPS) {
  Label class_is_initialized, need_init;
  // IMPL_NOTE: Cannot make the flush conditionally.
  //  see how this can be made conditional!
  flush_frame();

  // add to the klass oop to get the address of the appropriate
  // task mirror table entry
  Value task_mirror(T_OBJECT);
  task_mirror.assign_register();
  Value klass_value(T_OBJECT);
  klass_value.set_obj(klass);
  {
    JavaClass::Raw jc = klass;
    get_mirror_list_base(task_mirror.lo_register());
    // FieldAddress might destroy the value, so we create a defensive copy.
    Value task_mirror_copy;
    task_mirror.copy(task_mirror_copy);
    FieldAddress address(task_mirror_copy, jc().class_id() * sizeof(OopDesc *), T_OBJECT);
    ldr(task_mirror.lo_register(), address.lo_address_2());
  }

  {
    // The marker cannot be treated as a constant value, as it would break 
    // cross-compilation. Thus we load it from GP table.
    TempRegister tmp;
    get_task_class_init_marker(tmp);
    cmp(task_mirror.lo_register(), reg(tmp));
    b(class_is_initialized, ne);
  }
bind(need_init);
  // Call the runtime system.
  // pass klass as extra args
  // flush_frame();
  task_mirror.destroy();
  if (klass_value.lo_register() != r1) {
    call_vm_extra_arg(klass_value.lo_register());
  }
  klass_value.destroy();
  call_vm((address) compiled_code_task_barrier, T_OBJECT JVM_CHECK);
  bind(class_is_initialized);
}

#endif

extern "C" {
  extern address gp_base_label;
}

bool CompilerLiteralAccessor::has_literal(int imm32,
                                          Assembler::Address1& result) {
  LiteralElementStream les(frame());
  for ( ; !les.eos() ; les.next()) {
    if (les.value() == imm32) {
      result = Assembler::reg(les.reg());
      return true;
    }
  }
  // IMPL_NOTE: 
  // This for loop should probably be deleted.  It's just not worth it.
  for(les.reset(); !les.eos(); les.next()) {
    int value = les.value();
    if (value == 0) { continue; }
    Assembler::Register rd = les.reg();

    for (int i = 1; i < 31; i++) {
      if ((value >> i) == imm32) {
        result = Assembler::imm_shift(rd, Assembler::asr, i);
        return true;
      } else if ((value << i) == imm32) {
        result = Assembler::imm_shift(rd, Assembler::lsl, i);
        return true;
      } else if (((unsigned)value >> i) == (unsigned)imm32) {
        result = Assembler::imm_shift(rd, Assembler::lsr, i);
        return true;
      } else if ((int)_rotr(value, i) == imm32) {
        result = Assembler::imm_shift(rd, Assembler::ror, i);
        return true;
      }
    }
  }
  return false;
}

void CodeGenerator::save_state(CompilerState *compiler_state) {
  BinaryAssembler::save_state(compiler_state);
}

#if ENABLE_INLINE && ARM
void CodeGenerator::restore_state(CompilerState *compiler_state) {
  BinaryAssembler::restore_state(compiler_state);
}
#endif
void CodeGenerator::load_from_address(Value& result, BasicType type,
                                      MemoryAddress& address,
                                      Assembler::Condition cond) {
  write_literals_if_desperate();

   // illegal types do not require any loading
  if (type == T_ILLEGAL) return;

  GUARANTEE(stack_type_for(type) == result.stack_type(),
            "types must match (taking stack types into account)");
  result.assign_register();
  const Register lo = result.lo_register();

  switch(type) {
  case T_BOOLEAN:
    ldrsb (lo, address.lo_address_3(), cond);
    break;

  case T_CHAR:
    ldrh(lo, address.lo_address_3(), cond);
    break;

  case T_SHORT:
    ldrsh (lo, address.lo_address_3(), cond);
    break;

  case T_INT:   // fall through
  case T_FLOAT: // fall through
  case T_ARRAY: // fall through
  case T_OBJECT:
    ldr(lo, address.lo_address_2(), cond);
    break;

  case T_LONG:  // fall through
  case T_DOUBLE:
    ldr   (lo, address.lo_address_2(), cond);
    ldr   (result.hi_register(), address.hi_address_2(), cond);
    break;

  case T_BYTE:
    {
      // T_BYTE doesn't get compiled very often, so let's put it here. Gcc generates
      // better code to make it faster to compile the common case (loading 4-bytes).
      bool is_signed = true;

      Bytecodes::Code bc = method()->bytecode_at(bci());
      int nextbci = bci() + Bytecodes::length_for(method(), bci());
      int len = method()->code_size();
      
      if (bc == Bytecodes::_baload && nextbci + 1 < len) {
        Bytecodes::Code bc1 = method()->bytecode_at(nextbci);
        Bytecodes::Code bc2 = method()->bytecode_at(nextbci+1);
        if ((bc1 == Bytecodes::_i2s  && bc2 == Bytecodes::_iand) ||
            (bc1 == Bytecodes::_iand && bc2 == Bytecodes::_i2s)) {
          // Detect a common code pattern:
          //     sipush 0xff
          //     aload array
          //     iload index
          //     baload array    <<< we are here
          //     iand
          //     i2s
          // We can safely skip the iand and i2s bytecodes, and change the load
          // into an "unsigned load byte". The only case we cannot do this is
          // when the iand or i2s bytecode sits at a branch target, so we check it
          // with the Compiler::entry_count_for() lines below.
          if (Compiler::entry_count_for(nextbci) == 1 &&
              Compiler::entry_count_for(nextbci+1) == 1) {
            // At this point, the operands to the baload bytecode have already
            // been popped. The top of stack is the operand to the iand.
            Value iand_operand(T_INT);
            int operand_index = frame()->virtual_stack_pointer();
            frame()->value_at(iand_operand, operand_index);
            if (iand_operand.is_immediate() && iand_operand.as_int() == 0xff) {
              frame()->pop();
              Compiler::closure()->set_next_bytecode_index(nextbci+2);
              is_signed = false;
            }
          }
        } else if (bc1 == Bytecodes::_sipush &&
                   method()->get_java_short(nextbci+1) == 0xff &&
                   Compiler::entry_count_for(nextbci) == 1 &&
                   nextbci + 3 < len && 
                   method()->bytecode_at(nextbci+3) == Bytecodes::_iand && 
                   Compiler::entry_count_for(nextbci+3) == 1) {
          // Detect a common code pattern:
          //     aload array
          //     iload index
          //     baload array    <<< we are here
          //     sipush 0xff
          //     iand
          Compiler::closure()->set_next_bytecode_index(nextbci+4);
          is_signed = false;
        }
      }
      
      if (is_signed) {
        ldrsb(lo, address.lo_address_3(), cond);
      } else {
        ldrb(lo,  address.lo_address_2(), cond);
      }
    }
    break;

  default        :
    SHOULD_NOT_REACH_HERE();
    break;
  }
}

void CodeGenerator::store_to_address(Value& value, BasicType type,
                                     MemoryAddress& address) {
  write_literals_if_desperate();

  // if the value to store isn't present there nothing left to do
  if (!value.is_present()) {
    return;
  }
  GUARANTEE(stack_type_for(type) == value.stack_type(),
             "types must match (taking stack types into account)");
  Register reg;
  CompilerLiteralAccessor cla;

  if (value.is_immediate()) {
    reg = cla.get_literal(value.lo_bits());
    // We must use ::reference, not ::allocate, since the latter flushes the
    // register from the frame!
    // We need to make sure that this register isn't flushed when doing the
    // address calculations.
    RegisterAllocator::reference(reg);
  } else {
    reg = value.lo_register();
  }

  switch(type) {
    case T_BOOLEAN : // fall through
    case T_BYTE    :
        strb(reg, address.lo_address_2());
        break;

    case T_CHAR    : // fall through
    case T_SHORT   :
        strh(reg, address.lo_address_3());
        break;

    case T_ARRAY   :
    case T_OBJECT  :
        GUARANTEE(value.in_register() || value.must_be_null(),
                  "Only NULLs can be immediate");
        if (!value.not_on_heap()) {
          // Need to do pointer setting
          address.write_barrier_prolog();
          str(reg, address.lo_address_2());

          // Free up register that's not needed any more.  Pointer setting
          // uses up lots of registers, and we want to minimize pressure
          value.destroy();

          address.write_barrier_epilog();
          break;
        }
        // Fall through

    case T_FLOAT   :
    case T_INT     :
        str(reg, address.lo_address_2());
        break;

    case T_DOUBLE  : // fall through
    case T_LONG    :
        str(reg, address.lo_address_2());
        if (value.is_immediate()) {
          // Unreference the old literal.  Get the new literal and reference it
          RegisterAllocator::dereference(reg);
          reg = cla.get_literal(value.hi_bits());
          RegisterAllocator::reference(reg);
        } else {
          reg = value.hi_register();
        }
        str(reg, address.hi_address_2());
        break;

    default        :
        SHOULD_NOT_REACH_HERE();
        break;
  }
  if (value.is_immediate()) {
    RegisterAllocator::dereference(reg);
  }
}

#if ENABLE_NPCE && ARM
void CodeGenerator::store_to_address_safe(Value& value, BasicType type,
                                     MemoryAddress& address) {
  bool need_npe_check=false;
  write_literals_if_desperate();

  // if the value to store isn't present there nothing left to do
  if (!value.is_present()) {
    return;
  }

  GUARANTEE(stack_type_for(type) == value.stack_type(),
             "types must match (taking stack types into account)");
  Register reg;
  CompilerLiteralAccessor cla;

  if (value.is_immediate()) {
    reg = cla.get_literal(value.lo_bits());
    // We must use ::reference, not ::allocate, since the latter flushes the
    // register from the frame!
    // We need to make sure that this register isn't flushed when doing the
    // address calculations.
    RegisterAllocator::reference(reg);
  } else {
    reg = value.lo_register();
  }

  jint old_code_size = 0;
  NullCheckStub::Raw last;
  CompilationQueueElementDesc* tmp = 
    Compiler::current()->get_unlinked_exception_stub(bci());
  if (tmp != NULL) {
      last = tmp;
      need_npe_check = true;
  }

  switch(type) {
    case T_BOOLEAN : // fall through
    case T_BYTE    :
        strb(reg, address.lo_address_2());
        break;

    case T_CHAR    : // fall through
    case T_SHORT   :
        strh(reg, address.lo_address_3());
        break;

    case T_ARRAY   :
    case T_OBJECT  :
        GUARANTEE(value.in_register() || value.must_be_null(),
                  "Only NULLs can be immediate");
        if (!value.not_on_heap()) {
          // Need to do pointer setting
          address.write_barrier_prolog();
          str(reg, address.lo_address_2());
          if (need_npe_check) {
            record_npe_point(&last, -1) ;
            //NPCE record of object has been handler in special case.
            //No need to handle it in general case.
            need_npe_check = false;
          }

          // Free up register that's not needed any more.  Pointer setting
          // uses up lots of registers, and we want to minimize pressure
          value.destroy();

          address.write_barrier_epilog();
          break;
        }
        // Fall through

    case T_FLOAT   :
    case T_INT     :
        str(reg, address.lo_address_2());
        break;

    case T_DOUBLE  : // fall through
    case T_LONG    :
        str(reg, address.lo_address_2());
        if (need_npe_check) {
          record_npe_point(&last, -1);
          old_code_size = code_size();
        }
        if (value.is_immediate()) {
          // Unreference the old literal.  Get the new literal and reference it
          RegisterAllocator::dereference(reg);
          reg = cla.get_literal(value.hi_bits());
          RegisterAllocator::reference(reg);
        } else {
          reg = value.hi_register();
        }
        str(reg, address.hi_address_2());
        if ( need_npe_check ) {
#if ENABLE_CODE_OPTIMIZER
          last().set_two_words((code_size()-old_code_size)>>2);
#endif
          record_npe_point(NULL, -1) ;
        }
        break;

    default        :
        SHOULD_NOT_REACH_HERE();
        break;
  }
  if (need_npe_check && (type !=T_DOUBLE && type != T_LONG )) {
          record_npe_point(&last,-1) ;
  }
  if (value.is_immediate()) {
    RegisterAllocator::dereference(reg);
  }
}
#endif //NPCE

void
CodeGenerator::move(const Value& dst, const Value& src, const Condition cond) {
  // if the source isn't present there's nothing left to do
  if (!src.is_present()) return;

  GUARANTEE(dst.type() == src.type(), "type check");
  GUARANTEE(dst.in_register(), "destination must be in register");
  if (src.is_immediate()) {
    CompilerLiteralAccessor cla;
    /* move always 1 word */
    mov_imm(dst.lo_register(), src.lo_bits(), &cla, cond);
    if (cond == al) {
      frame()->set_has_literal_value(dst.lo_register(), src.lo_bits());
    }
    if (dst.is_two_word()) {
      mov_imm(dst.hi_register(), src.hi_bits(), &cla, cond);
      if (cond == al) {
        frame()->set_has_literal_value(dst.hi_register(), src.hi_bits());
      }
    }
  } else {
    GUARANTEE(src.in_register(), "source must be in register");
    /* move always 1 word */
    mov_reg(dst.lo_register(), src.lo_register(), no_CC, cond);
    if (dst.is_two_word()) {
      mov_reg(dst.hi_register(), src.hi_register(), no_CC, cond);
    }
  }
}

void CodeGenerator::move(Value& dst, Oop* obj, Condition cond) {
  GUARANTEE(dst.type() == T_OBJECT || dst.type() == T_ARRAY, "type check");
  ldr_oop(dst.lo_register(), obj, cond);
}

void CodeGenerator::move(Assembler::Register dst, Assembler::Register src,
                         Condition cond) {
  mov_reg(dst, src, no_CC, cond);
}

#if ENABLE_REMEMBER_ARRAY_LENGTH & ARM
void CodeGenerator::preload_parameter (Method* method) {
  Signature::Raw signature = method->signature();
  for (SignatureStream ss(&signature, method->is_static()); !ss.eos(); ss.next())
  {
    if (ss.type()==T_ARRAY) {
      Value value(ss.type());
      frame()->value_at(value, ss.index());
      break;
    }
  }
}
#endif

void CodeGenerator::array_check(Value& array, Value& index JVM_TRAPS) {
  write_literals_if_desperate();
  UsingFastOops fast_oops;
  bool null_check = need_null_check(array);

  NullCheckStub::Fast null_check_stub;

#if ENABLE_REMEMBER_ARRAY_LENGTH
  Register length;
  bool first_time = !(array.is_not_first_time_access());
  if (null_check) {
    null_check_stub = NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_CHECK);
#if ENABLE_NPCE
    record_npe_point(&null_check_stub);
    frame()->set_value_must_be_nonnull(array);
    length = frame()->get_bound(array.lo_register(), 
                                    first_time, Assembler::al);
#else
    cmp(array.lo_register(), zero);
    frame()->set_value_must_be_nonnull(array);
    length = frame()->get_bound(array.lo_register(), 
                                    first_time, Assembler::ne);
    b(&null_check_stub, eq);
#endif
  } else {
    length = frame()->get_bound(array.lo_register(), 
                                    first_time, Assembler::al);
  }

#else
  // !ENABLE_REMEMBER_ARRAY_LENGTH
  TempRegister length;
  if (null_check) {
#if ENABLE_NPCE
    null_check_stub = NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_CHECK);
    record_npe_point(&null_check_stub);
    frame()->set_value_must_be_nonnull(array);
    ldr_imm_index(length, array.lo_register(), Array::length_offset());
#else
    cmp(array.lo_register(), zero);
    frame()->set_value_must_be_nonnull(array);
    ldr(length, imm_index(array.lo_register(), Array::length_offset()), ne);
    int offset = get_inline_thrower_gp_index(
            ThrowExceptionStub::rte_null_pointer JVM_CHECK);
    if (offset > 0) {
      ldr(pc, imm_index(gp, offset), eq);
    } else {
      null_check_stub = NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_CHECK);
      b(&null_check_stub, eq);
    }
#endif
  } else {
    ldr_imm_index(length, array.lo_register(), Array::length_offset());
  }
#endif
 
#if ENABLE_REMEMBER_ARRAY_CHECK && \
     ENABLE_REMEMBER_ARRAY_LENGTH && ENABLE_NPCE
  if (!index.is_immediate() &&  index.stack_type() == T_INT) {
    if ( frame()->is_value_must_be_index_checked( length, index)) {
#ifndef PRODUCT
      if(PrintCompiledCodeAsYouGo) { 
        TTY_TRACE_CR(("Omit a array length checking"));
      }
#endif 
      return;
    } else {
      frame()->set_value_must_be_index_checked( length, index);
    }
  }
#endif

  if (index.is_immediate()) {
    CompilerLiteralAccessor cla;
    cmp_imm(length, index.as_int(), &cla);
  } else {
    cmp(length, reg(index.lo_register()));
  }

  int offset = get_inline_thrower_gp_index(
            ThrowExceptionStub::rte_array_index_out_of_bounds JVM_CHECK);
  if (offset > 0) {
    ldr(pc, imm_index(gp, offset), ls);
  } else {  
    IndexCheckStub::Raw index_check_stub = 
      IndexCheckStub::allocate_or_share(JVM_SINGLE_ARG_CHECK);
    b(&index_check_stub, ls);
  }
}

int CodeGenerator::get_inline_thrower_gp_index(int rte JVM_TRAPS) {
#if ENABLE_XSCALE_WMMX_INSTRUCTIONS || ENABLE_ARM_V6
  // This optimization actually slows down XScale or ARM1136 because 
  // "ldr<cond> pc, [gp, #xx]" is very slow even if the condition is false.
  return -1;
#else
  bool allowed = is_inline_exception_allowed(rte JVM_CHECK_(-1));
  if (!allowed) {
    return -1;
  }

  int locals = method()->max_locals();
  if (locals < MAX_INLINE_THROWER_METHOD_LOCALS) {
    long offset;
    if (rte == ThrowExceptionStub::rte_null_pointer) {
      address &target = gp_compiler_throw_NullPointerException_0_ptr;
      offset = (long)&target - (long)&gp_base_label;
    } else {
      address &target = gp_compiler_throw_ArrayIndexOutOfBoundsException_0_ptr;
      offset = (long)&target - (long)&gp_base_label;
    }
    offset += long(locals) * BytesPerWord;
    return offset;
  }
  return -1;
#endif
}

// Note: We cannot bailout in null_check, as null_check is used in
// combination with other instructions.
// This means that we do not have enough data to reconstruct the virtual
// stack frame for the  uncommon trap
void CodeGenerator::null_check(const Value& object JVM_TRAPS) {
  cmp(object.lo_register(), zero);

  int offset = get_inline_thrower_gp_index(ThrowExceptionStub::rte_null_pointer
                                          JVM_CHECK);
  if (offset > 0) {
    ldr(pc, imm_index(gp, offset), eq);
  } else {
    NullCheckStub::Raw check_stub = 
        NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_NO_CHECK);
    if (check_stub.not_null()) {
      b(&check_stub, eq);
    }
  }
}

#if ENABLE_NPCE
void CodeGenerator::null_check_by_signal(Value& object, bool fakeldr JVM_TRAPS)
{
  NullCheckStub::Raw check_stub = 
      NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_NO_CHECK);
  if (check_stub.not_null()) {
    record_npe_point(&check_stub);
    if (fakeldr) {
      TempRegister dummy;
#ifndef PRODUCT
      if(PrintCompiledCodeAsYouGo) {
        TTY_TRACE_CR((" generate a faked ldr instruction =>\n"));
      }
#endif 
      ldr_imm_index(dummy, object.lo_register(), 0);
    }
  }
}

void CodeGenerator::null_check_by_signal_quick(Value& object, BasicType type 
                                               JVM_TRAPS) {
  NullCheckStub::Raw check_stub = 
      NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_NO_CHECK);
#if ENABLE_CODE_OPTIMIZER && !ENABLE_THUMB_COMPILER
  if (check_stub.not_null()) {
    if (!check_stub().is_persistent() && 
          (type == T_LONG || type == T_DOUBLE)) {
      check_stub().set_two_words();
    }
  }
#endif  
}
#endif //ENABLE_NPCE

/* In order to improve the pipeline on the ARM, we try to replace,
 *     maybe_null_check(op)
 *     code that uses op
 * with the code sequence
 *     cond = maybe_null_check_1(op)
 *     code that uses op, conditionalized on cond
 *     maybe_null_check_2(cond)
 *
 * In particular, if the second line is a "ldr" instruction, that improves
 * the chances that there will be an instruction separating the register
 * whose value is loaded and the instruction that uses it.
 */
Assembler::Condition CodeGenerator::maybe_null_check_1(Value& object) {
  if (object.must_be_null()) {
      // flag was set when we did a set_obj
    cmp(r0, reg(r0));   // set CC to "eq"
    return ne;
  } else if (need_null_check(object)) {
    cmp(object.lo_register(), zero);
    frame()->set_value_must_be_nonnull(object);
    return ne;
  } else {
    return al;
  }
}

void CodeGenerator::maybe_null_check_2(Assembler::Condition cond JVM_TRAPS) {
  if (cond == ne) {
    int offset = get_inline_thrower_gp_index(
            ThrowExceptionStub::rte_null_pointer JVM_CHECK);
    if (offset > 0) {
      ldr(pc, imm_index(gp, offset), eq);
    } else {
      NullCheckStub::Raw error = 
          NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_NO_CHECK);
      if (error.not_null()) {
        b(&error, eq);
      }
    }
  }
}

#if ENABLE_NPCE
Assembler::Condition CodeGenerator::maybe_null_check_1_signal(Value& object,
                                                              bool& is_npe) {
  if (object.must_be_null()) {
    is_npe = true;
    return al;
  } else if (need_null_check(object)) {
      is_npe = true;
      frame()->set_value_must_be_nonnull(object);
      return al;
  } else {
    is_npe = false;
    return al;
  }
}

void CodeGenerator::maybe_null_check_2_signal(Assembler::Condition cond,
                                              bool is_npe JVM_TRAPS) {
  if (is_npe) {
    NullCheckStub::Raw error = 
        NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_NO_CHECK);
    if (error.not_null()) {
      record_npe_point(&error,-1);
    }
   }
}

void CodeGenerator::maybe_null_check_3_signal(Assembler::Condition cond,
                                              bool is_npe, BasicType type
                                              JVM_TRAPS) {
  if (cond == ne || is_npe ) {
    NullCheckStub::Raw error = 
        NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_NO_CHECK);
    if (error.not_null()) {
      if (is_npe) {
        if (type == T_LONG || type == T_DOUBLE) {
          record_npe_point(&error,-2);
        } else {    
          record_npe_point(&error,-1);
        }
      } else {
        b(&error, eq);
      }
    }
  }
}
#endif //ENABLE_NPCE

void CodeGenerator::overflow(const Assembler::Register& stack_pointer,
                             const Assembler::Register& method_pointer) {
  if (USE_OVERFLOW_STUB) {
    if (method_pointer != Assembler::r0) {
      mov(r0, reg(method_pointer));
    }
    if (stack_pointer != Assembler::r1) {
      mov(r1, reg(stack_pointer));
    }
    int offset = (int)&gp_interpreter_method_entry_ptr - 
                 (int)&gp_base_label;
    ldr(pc, imm_index(gp, offset));
  } else {
    (void)stack_pointer;
    (void)method_pointer;
    SHOULD_NOT_REACH_HERE();
  }
}

void CodeGenerator::method_prolog(Method *method JVM_TRAPS) {
  int stack_bytes_needed =
      (method->max_execution_stack_count() * BytesPerStackElement) +
      JavaFrame::frame_desc_size();

  // We check timer tick only if check for stack overflow.
  bool need_stack_and_timer_checks = true;
  if (method->is_native() || !method->access_flags().has_invoke_bytecodes()) {
    if (stack_bytes_needed < LeafMethodStackPadding) {
      // We're sure this method won't cause a stack overflow.
      //
      // IMPL_NOTE: leaf methods do not check for timer ticks. We need to
      // add timer ticks checks in non-leaf methods that make long
      // series of method calls inside straight-line code.
      need_stack_and_timer_checks = false;
    }
  }

  if (Compiler::omit_stack_frame()) {
    need_stack_and_timer_checks = false;
  }

  if (!need_stack_and_timer_checks) {
    // Just need to update method execution sensor
    if (!GenerateROMImage) {
      strb(gp, imm_index(gp,
        address(_method_execution_sensor) - address(&gp_base_label)));
    }
  } else {
#if !ENABLE_TRAMPOLINE
    if (GenerateCompilerAssertions) {
      // Our calling convention guarantees method is in Register::callee.
      ldr_oop(r12, method);
      cmp(r12, reg(callee));
      breakpoint(ne);
    }
#endif

    COMPILER_COMMENT(("check for stack overflow and timer tick"));
    GUARANTEE(callee == r0 || callee == r1, "code assumption");
    TempRegister stack_limit(r4);
    TempRegister tmp(r3);


#if ENABLE_XSCALE_WMMX_TIMER_TICK && !ENABLE_TIMER_THREAD
    get_current_stack_limit(stack_limit);
#else
    TempRegister timer_ticks(r2);
    get_rt_timer_ticks(timer_ticks);
    get_current_stack_limit(stack_limit);
#endif

    if (!GenerateROMImage) {
      strb(gp, imm_index(gp,
        address(_method_execution_sensor) - address(&gp_base_label)));
    }

#if ENABLE_XSCALE_WMMX_TIMER_TICK && !ENABLE_TIMER_THREAD
    textrcb(0);
#else
    cmp(timer_ticks, imm(0));
#endif

    if (stack_bytes_needed < LeafMethodStackPadding &&
        method->max_execution_stack_count() < 20) {
      // Don't need to do an exact check -- if we overwrite slightly over
      // current_stack_limit, we will write into the StackPadding area, and
      // thus will not write outside of the legal stack area.
      if (JavaStackDirection < 0) {
        cmp(stack_limit, reg(jsp), ls);
      } else {
        cmp(jsp, reg(stack_limit), ls);
      }
    } else {
      add_imm(tmp, jsp, JavaStackDirection * stack_bytes_needed);
      if (JavaStackDirection < 0) {
        cmp(stack_limit, reg(tmp), ls);
      } else {
        cmp(tmp, reg(stack_limit), ls);
      }
    }

    // We trap back to interpreter if
    // (JavaStackDirection < 0) ->
    //      (timer_ticks > 0) || stack_limit > jsp)
    //
    // (JavaStackDirection > 0) ->
    //      (timer_ticks > 0) || jsp > stack_limit)
    if (USE_OVERFLOW_STUB) {
      // On xscale, a conditional branch is faster than a conditional ldr pc
      Label stack_overflow, done;
      b(stack_overflow, hi);
    bind(done); // Not actually used on ARM port

      StackOverflowStub::Raw stub =
        StackOverflowStub::allocate(stack_overflow, done, r1, r0 JVM_CHECK);
      stub().insert();
      // If we go to the stub, we can't be guaranteed it has preserved literals
      frame()->clear_literals();
    } else {
      int offset = (int)&gp_interpreter_method_entry_ptr - 
                   (int)&gp_base_label;
      ldr(pc, imm_index(gp, offset), hi);
    }
  }
}

void CodeGenerator::method_entry(Method* method JVM_TRAPS) {
  // prolog does some or all of the following
  //   - update execution sensor
  //   - check timer ticks
  //   - check stack overflow
  method_prolog(method JVM_CHECK);
 
  if (Compiler::omit_stack_frame()) {
    // The rest of method_entry deal with pushing the call frame, so
    // we can safely return here.
    GUARANTEE(!ENABLE_WTK_PROFILER, "Profiler always need call frame");
    return;
  }

  COMPILER_COMMENT(("reserve space for locals & frame descriptor"));
  int extra_locals = method->max_locals() - method->size_of_parameters();
  int jsp_shift = extra_locals*BytesPerStackElement +
                  JavaFrame::frame_desc_size();

  if (ENABLE_FULL_STACK && 
      (JavaFrame::empty_stack_offset() == 
       JavaFrame::return_address_offset()) &&
      (JavaStackDirection < 0) &&
      (has_room_for_imm(jsp_shift, 12))) {
    // We can save one instruction by using ARM pre-index addressing mode
    str(lr, imm_index(jsp, JavaStackDirection * jsp_shift, pre_indexed));
  } else {
    add_imm(jsp, jsp, JavaStackDirection * jsp_shift);
    str(lr, imm_index(jsp, JavaFrame::return_address_offset()
                                     - JavaFrame::empty_stack_offset()));
  }

  // The new fp will be at jsp - JavaFrame::empty_stack_offset().  We need to
  // save the old value of fp before setting the new one
  str(fp, imm_index(jsp, JavaFrame::caller_fp_offset()
                       - JavaFrame::empty_stack_offset()));
  sub_imm(fp, jsp, JavaFrame::empty_stack_offset());

  if (method->access_flags().is_synchronized()) {
    if (method->access_flags().is_static()) {
      UsingFastOops fast_oops;
      // Get the class mirror object.
#if ENABLE_ISOLATES
      TempRegister task_mirror(tmp0);
      InstanceClass::Fast klass = method->holder();
      Value klass_value(T_OBJECT);
      klass_value.set_obj(&klass);

      if (StopAtRealMirrorAccess) {
          breakpoint();
      }
      load_task_mirror(&klass, klass_value, true JVM_CHECK);
      // Now load the real mirror
      ldr_imm_index(r0, klass_value.lo_register(),
                        TaskMirror::real_java_mirror_offset());
#else
      JavaClass::Fast klass = method->holder();
      Instance::Fast mirror = klass().java_mirror();
      COMPILER_COMMENT(("Static method. Synchronize on the class "
                          "mirror object"));
      if (GenerateROMImage) {
        // ldr_oop handles classes correctly
        ldr_oop(r0, &klass);
        ldr_imm_index(r0, r0, JavaClass::java_mirror_offset());
      } else { 
        ldr_oop(r0, &mirror);
      }
#endif
    } else {
      COMPILER_COMMENT(("Non-static method. Synchronize on the receiver"));
      LocationAddress obj(0, T_OBJECT);
      ldr(r0, obj.lo_address_2());
    }
    call_through_gp(gp_shared_lock_synchronized_method_ptr JVM_CHECK);
  } else { // not synchronized
    if (method->access_flags().has_monitor_bytecodes()) {
      // Method isn't synchronized, but it has monitor bytecodes.
      COMPILER_COMMENT(("fill in the stack bottom pointer"));
      str(jsp, imm_index(fp, JavaFrame::stack_bottom_pointer_offset()));
    } else {
      if (GenerateCompilerAssertions) {
        COMPILER_COMMENT(("insert bogus stack bottom pointer"));
        mov(r0, imm_rotate(0xBA, 4)); // pretty bogus immediate
        str(r0, imm_index(fp, JavaFrame::stack_bottom_pointer_offset()));
      }
    }
  }

#if ENABLE_WTK_PROFILER
  // we always call this callback, as profiler can be later dynamically
  // enabled using C API (JVM_SendProfilerCommand)
  call_vm((address)jprof_record_method_transition, T_VOID JVM_CHECK);
#endif
}

void CodeGenerator::clear_stack() {
  if (method()->access_flags().is_synchronized()
             || method()->access_flags().has_monitor_bytecodes()) {
    // if the method is synchronized or has monitor bytecodes the
    // stack bottom pointer in the frame descriptor is filled in
    ldr_imm_index(jsp, fp, JavaFrame::stack_bottom_pointer_offset());
  } else {
    // Used a fixed offset from the fp
    add_imm(jsp, fp, JavaFrame::empty_stack_offset());
  }
}

void CodeGenerator::clear_object_location(jint index) {
  // The field is actual T_OBJECT, but T_INT is simpler to work with, and
  // the result is the same
  Value zero(T_INT);
  zero.set_int(0);
  LocationAddress address(index, T_INT);
  store_to_address(zero, T_INT, address);
}

void CodeGenerator::int_binary_do(Value& result, Value& op1, Value& op2,
                                  BytecodeClosure::binary_op op JVM_TRAPS) {
  write_literals_if_desperate();

  GUARANTEE(!result.is_present(), "result must not be present");
  GUARANTEE(op1.in_register(), "op1 must be in a register");
  GUARANTEE(op2.is_immediate() || op2.in_register(),
            "op2 must be in a register or an immediate");


  static const jubyte table[] = {
    /* bin_add */ _add,
    /* bin_sub */ _sub,
    /* bin_mul */ 0xff,
    /* bin_div */ 0xff,
    /* bin_rem */ 0xff,
    /* bin_shl */ 0xff,
    /* bin_shr */ 0xff,
    /* bin_ushr*/ 0xff,
    /* bin_and */ _andr,
    /* bin_or  */ _orr,
    /* bin_xor */ _eor,
    /* bin_min */ 0xff,
    /* bin_max */ 0xff,
    /* bin_rsb */ _rsb,
  };

  switch (op) {
    case BytecodeClosure::bin_sub  :
    case BytecodeClosure::bin_rsb  :
    case BytecodeClosure::bin_add  :
    case BytecodeClosure::bin_and  :
    case BytecodeClosure::bin_xor  :
    case BytecodeClosure::bin_or   :
      GUARANTEE(int(op) >= 0 && op < sizeof(table), "sanity");
      GUARANTEE(table[op] != 0xff, "sanity");
      arithmetic((Opcode)(table[op]), result, op1, op2);
      break;
    case BytecodeClosure::bin_shr  :
      shift (asr, result, op1, op2);
      break;
    case BytecodeClosure::bin_shl  :
      shift (lsl, result, op1, op2);
      break;
    case BytecodeClosure::bin_ushr :
      shift (lsr, result, op1, op2);
      break;
    case BytecodeClosure::bin_mul  :
      imul (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM);
      break;
    case BytecodeClosure::bin_min  :
    case BytecodeClosure::bin_max  :
      assign_register(result, op1);
      if (op2.is_immediate()) {
        op2.materialize();
      }
      cmp(op1.lo_register(), reg(op2.lo_register()));
      mov_reg(result.lo_register(), op1.lo_register());
      mov(result.lo_register(), reg(op2.lo_register()),
                      ((op == BytecodeClosure::bin_min) ? gt : lt));
      break;
    case BytecodeClosure::bin_div  :
      idiv_rem (result, op1, op2, false JVM_NO_CHECK_AT_BOTTOM);
      break;
    case BytecodeClosure::bin_rem  :
      idiv_rem (result, op1, op2, true JVM_NO_CHECK_AT_BOTTOM);
      break;
    default                    :
      SHOULD_NOT_REACH_HERE();
      break;
  }
}

void CodeGenerator::int_unary_do(Value& result, Value& op1,
                                  BytecodeClosure::unary_op op JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();

  GUARANTEE(!result.is_present(), "result must not be present");
  GUARANTEE(op1.in_register(), "op1 must be in a register");

  assign_register(result, op1);

  const Register resReg = result.lo_register();
  const Register opReg  = op1.lo_register();
  switch (op) {
    case BytecodeClosure::una_neg  :
      rsb(resReg, opReg, zero);
      break;
    case BytecodeClosure::una_abs  :
      add(resReg, opReg, zero, set_CC);
      rsb(resReg, opReg, zero, lt);
      break;
    default:
      SHOULD_NOT_REACH_HERE();
      break;
  }
}

void CodeGenerator::long_binary_do(Value& result, Value& op1, Value& op2,
                                   BytecodeClosure::binary_op op JVM_TRAPS) {
  write_literals_if_desperate();
  GUARANTEE(!result.is_present(), "result must not be present");
  GUARANTEE(op1.in_register(), "op1 must be in a register");
  GUARANTEE(op2.is_immediate() || op2.in_register(),
            "op2 must be in a register or an immediate");

  static const jubyte table[] = {
    /* bin_add */ _add, _adc,
    /* bin_sub */ _sub, _sbc,
    /* bin_mul */ 0xff, 0xff,
    /* bin_div */ 0xff, 0xff,
    /* bin_rem */ 0xff, 0xff,
    /* bin_shl */ 0xff, 0xff,
    /* bin_shr */ 0xff, 0xff,
    /* bin_ushr*/ 0xff, 0xff,
    /* bin_and */ _andr,_andr,
    /* bin_or  */ _orr, _orr,
    /* bin_xor */ _eor, _eor,
    /* bin_min */ 0xff, 0xff,
    /* bin_max */ 0xff, 0xff,
    /* bin_rsb */ _rsb, _rsc,
  };

  switch (op) {
    case BytecodeClosure::bin_sub:
    case BytecodeClosure::bin_rsb:
    case BytecodeClosure::bin_add:
    case BytecodeClosure::bin_and:
    case BytecodeClosure::bin_xor:
    case BytecodeClosure::bin_or :
      {
        int i = ((int)op) * 2;
        larithmetic((Opcode)table[i], (Opcode)table[i+1], result, op1, op2);
      }
      break;

    case BytecodeClosure::bin_mul:
      lmul (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM);
      break;
    case BytecodeClosure::bin_div:
      ldiv (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM);
      break;
    case BytecodeClosure::bin_rem:
      lrem (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM);
      break;

    case BytecodeClosure::bin_shr :
      lshift(asr, result, op1, op2);
      break;
    case BytecodeClosure::bin_shl :
      lshift(lsl, result, op1, op2);
      break;
    case BytecodeClosure::bin_ushr:
      lshift(lsr, result, op1, op2);
      break;

    case BytecodeClosure::bin_min:
    case BytecodeClosure::bin_max: {
      assign_register(result, op1);
      // This code isn't called very often, so we don't bother optimizing
      // the case that op2 is an immediate
      if (op2.is_immediate()) {
        op2.materialize();
      }
      Register A1 =  op1.lsw_register();
      Register A2 =  op1.msw_register();

      Register B1 =  op2.lsw_register();
      Register B2 =  op2.msw_register();

      Register R1 =  result.lsw_register();
      Register R2 =  result.msw_register();

      TempRegister tmp;
      // Compare op1 and op2.  Correctly set bits for lt, ge
      cmp(     A1, reg(B1));
      sbc(tmp, A2, reg(B2), set_CC);
      // Copy one of the results
      Assembler::Condition op1_is_result =
                      ((op == BytecodeClosure::bin_min) ? lt : ge);
      Assembler::Condition op2_is_result = not_cond(op1_is_result);

      if (A1 != R1) {
        mov(R1, reg(A1), op1_is_result);
        mov(R2, reg(A2), op1_is_result);
      }
      mov(  R1, reg(B1), op2_is_result);
      mov(  R2, reg(B2), op2_is_result);
      break;
    }

    default                    :
      SHOULD_NOT_REACH_HERE();        break;
  }
}

void CodeGenerator::long_unary_do(Value& result, Value& op1,
                                   BytecodeClosure::unary_op op JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();

  GUARANTEE(!result.is_present(), "result must not be present");
  GUARANTEE(op1.in_register(), "op1 must be in a register");
  Label done;

  assign_register(result, op1);

  Register A1 =  op1.lsw_register();
  Register A2 =  op1.msw_register();

  Register R1 =  result.lsw_register();
  Register R2 =  result.msw_register();

  switch (op) {
    case BytecodeClosure::una_neg:
      rsb(R1, A1, zero, set_CC);
      rsc(R2, A2, zero);
      break;

    case BytecodeClosure::una_abs:
      mov_reg(R1, A1);
      add(R2, A2, zero, set_CC);
      b(done, ge);              // If hi register >= 0, positive
      rsb(R1, A1, zero, set_CC);
      rsc(R2, A2, zero);
      bind(done);
      break;

    default:
      SHOULD_NOT_REACH_HERE();
      break;
  }
}

void CodeGenerator::arithmetic(Opcode opcode,
                               Value& result, Value& op1, Value& op2) {
  assign_register(result, op1);

  const Register resReg = result.lo_register();
  const Register op1Reg  = op1.lo_register();
  if (op2.is_immediate()) {
    CompilerLiteralAccessor cla;
    arith_imm(opcode, 
              resReg, op1Reg, op2.as_int(), &cla);
  } else {
    arith(opcode,
          resReg, op1Reg, reg(op2.lo_register()));
  }
}

void CodeGenerator::imul(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  // Check.  Can we reuse op1.lo_register() in result?
  result.assign_register();

  const Register resReg = result.lo_register();
  const Register op1Reg  = op1.lo_register();
  if (op2.is_immediate()) {
    TempRegister tmp;
    mul_imm(resReg, op1Reg, op2.as_int(), tmp);
  } else {
    mul(resReg, op1Reg, op2.lo_register());
  }
}

void CodeGenerator::idiv_rem(Value& result, Value& op1, Value& op2,
                             bool isRemainder JVM_TRAPS) {
  Register resReg, invReg;
  int divisor = op2.in_register() ? 0 : op2.as_int();
  bool negate = false;
  if (divisor < 0) {
    divisor = - divisor;
    // We only need to negate the result for division
    negate = !isRemainder;
  }

  if (op2.in_register()) {
    flush_frame();
    setup_c_args(2, &op1, &op2, NULL);
    // Call the compiler stub.
    call_through_gp(gp_compiler_idiv_irem_ptr JVM_CHECK);
    Register result_register = isRemainder ? r0 : r1;
    RegisterAllocator::reference(result_register);
    result.set_register(result_register);
  } else if (divisor == 0) {
    ZeroDivisorCheckStub::Raw zero =
      ZeroDivisorCheckStub::allocate_or_share(JVM_SINGLE_ARG_CHECK);
    b(&zero);
    Compiler::current()->closure()->terminate_compilation();
  } else if (divisor == 1) {
    if (isRemainder) {
      result.set_int(0);
    } else if (negate) {
      int_unary_do(result, op1, BytecodeClosure::una_neg JVM_NO_CHECK_AT_BOTTOM);
    } else {
      op1.copy(result);
    }
  } else if (divisor == 0x80000000 || is_power_of_2(divisor)) {
    int shift = (divisor == 0x80000000) ? 31 : exact_log2(divisor);
    assign_register(result, op1);
    resReg = result.lo_register();
    add(resReg, op1.lo_register(), zero, set_CC);
    rsb(resReg, resReg, zero, lt);
    if (isRemainder) {
      if (is_rotated_imm(divisor - 1) || is_rotated_imm(~(divisor - 1))) {
        andr_imm(resReg, resReg, divisor - 1);
      } else {
        mov(resReg, imm_shift(resReg, lsl, (32 - shift)));
        mov(resReg, imm_shift(resReg, lsr, (32 - shift)));
      }
    } else {
       int shift = (divisor == 0x80000000) ? 31 : exact_log2(divisor);
       mov(resReg, imm_shift(resReg, lsr, shift));
    }
    rsb(resReg, resReg, zero, negate ? ge : lt);
  } else {
    int shift = jvm_log2(divisor);
    // Calculate the (32+shift)-bit fixed-point inverse of the divisor
    jlong magic = (jlong) ((((julong)1) << (32 + shift)) - 1);
    jlong ldivisor = (jlong) ((julong) ((juint)divisor));
    jlong inverse = (magic / ldivisor) + 1;

    // See if we can use ceiling(inverse/2);
    jlong inverse2 = (inverse + 1) >> 1;
    jlong inverse2_error = inverse2 * ldivisor - (((julong)1) << (31 + shift));
    if (inverse2_error < (jlong) (((julong)1 << shift))) {
      inverse = inverse2;
      shift = shift - 1;
    }
    if (negate) {
      inverse = -inverse;
    }
    invReg = RegisterAllocator::allocate();
    mov_imm(invReg, (int)inverse);

    if (!isRemainder && (inverse == (int)inverse)) {
      assign_register(result, op1);
    } else {
      // We need to use op1 after the multiplication
      result.assign_register();
    }
    resReg = result.lo_register();
    TempRegister tmp;

    // Calculate op1 * inverse >> (32 + shift)
    // Note that inverse is in the range -FFFFFFFF <= inverse <= FFFFFFFF
    if (inverse == (int)inverse) {
      // inverse is a normal integer, so we can just do a signed multiply
      smull(tmp, resReg, invReg, op1.lo_register(), set_CC);
    } else {
      // inverse is outside the range of a normal integer, so we have to
      // adjust the result
      smull(tmp, resReg, invReg, op1.lo_register());
      arith(((inverse < 0) ? _sub : _add),
            resReg, resReg, reg(op1.lo_register()), set_CC);
    }
    mov(resReg, imm_shift(resReg, asr, shift));
    add(resReg, resReg, one, mi); // Don't use neg! since V is uncertain

    if (isRemainder) {
      mul_imm(invReg, resReg, divisor, tmp);
      rsb(resReg, invReg, reg(op1.lo_register()));
    }
    RegisterAllocator::dereference(invReg);
  }
}

void CodeGenerator::shift(Shift shifter, Value& result, Value& op1, Value& op2)
{
  if (op2.is_immediate()) {
    assign_register(result, op1);
    const Register resReg = result.lo_register();

    // We have to treat 0 as a special case since "asr 0" and "lsr 0"
    // don't actually mean "shift right by zero"
    int shift = (op2.as_int() & 0x1f);
    if (shift == 0) {
      mov_reg(resReg, op1.lo_register());
    } else {
      mov(resReg, imm_shift(op1.lo_register(), shifter, shift));
    }
  } else {
    assign_register(result, op2); // result & op1 can't be same
    const Register resReg = result.lo_register();
    andr(resReg, op2.lo_register(), imm(0x1f));
    mov(resReg, reg_shift(op1.lo_register(), shifter, resReg));
  }
}

#if ENABLE_FLOAT
void CodeGenerator::float_binary_do(Value& result, Value& op1, Value& op2,
                                    BytecodeClosure::binary_op op JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  typedef JVM_SOFTFP_LINKAGE float (*runtime_func_type)(float, float);

  static const runtime_func_type funcs[] = {
    /* bin_add = 0 */ jvm_fadd,
    /* bin_sub = 1 */ jvm_fsub,
    /* bin_mul = 2 */ jvm_fmul,
    /* bin_div = 3 */ jvm_fdiv,
    /* bin_rem = 4 */ jvm_frem,
  };

  GUARANTEE(int(op) >= int(BytecodeClosure::bin_add) &&
            int(op) <= int(BytecodeClosure::bin_rem), "sanity");
  runtime_func_type runtime_func = funcs[op];

  if (op1.is_immediate() && op2.is_immediate()) {
    float result_imm = runtime_func(op1.as_float(), op2.as_float());
    result.set_float(result_imm);
  } else {
    if ((op == BytecodeClosure::bin_add || op == BytecodeClosure::bin_mul)
        && (   (op1.in_register() && op1.lo_register() == r1)
            || (op2.in_register() && op2.lo_register() == r0))) {
      // Avoid register shuffling on the commutative operations.
      call_simple_c_runtime(result, (address)runtime_func, op2, op1);
    } else {
      call_simple_c_runtime(result, (address)runtime_func, op1, op2);
    }
  }
}

void CodeGenerator::float_unary_do(Value& result, Value& op1,
                                   BytecodeClosure::unary_op op JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();

  GUARANTEE(op == BytecodeClosure::una_neg || op == BytecodeClosure::una_abs,
            "Sanity")

  GUARANTEE(!result.is_present(), "result must not be present");
  GUARANTEE(op1.in_register(), "op1 must be in a register");

  assign_register(result, op1);
  Opcode opcode = (  op == BytecodeClosure::una_neg ? _eor : _bic);
  arith(opcode, result.lo_register(), op1.lo_register(), imm_rotate(2,2));
}

void CodeGenerator::float_cmp (Value& result, BytecodeClosure::cond_op cond,
                               Value& op1, Value& op2 JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  JVM_SOFTFP_LINKAGE int (*runtime_func)(float, float);
  switch (cond) {
    case BytecodeClosure::lt:
        runtime_func = jvm_fcmpl; break;
    case BytecodeClosure::gt:
        runtime_func = jvm_fcmpg; break;
    default                 :
        runtime_func = 0; SHOULD_NOT_REACH_HERE(); break;
  }
  if (op1.is_immediate() && op2.is_immediate()) {
    result.set_int(runtime_func(op1.as_float(), op2.as_float()));
  } else {
    call_simple_c_runtime(result, (address)runtime_func, op1, op2);
  }
}

void CodeGenerator::double_binary_do(Value& result, Value& op1, Value& op2,
                                     BytecodeClosure::binary_op op JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  typedef JVM_SOFTFP_LINKAGE double (*runtime_func_type)(double, double);

  static const runtime_func_type funcs[] = {
    /* bin_add = 0 */ jvm_dadd,
    /* bin_sub = 1 */ jvm_dsub,
    /* bin_mul = 2 */ jvm_dmul,
    /* bin_div = 3 */ jvm_ddiv,
    /* bin_rem = 4 */ jvm_drem,
  };

  GUARANTEE(int(op) >= int(BytecodeClosure::bin_add) &&
            int(op) <= int(BytecodeClosure::bin_rem), "sanity");
  runtime_func_type runtime_func = funcs[op];

  if (op1.is_immediate() && op2.is_immediate()) {
    jdouble result_imm = runtime_func(op1.as_double(), op2.as_double());
    result.set_double(result_imm);
  } else {
    call_simple_c_runtime(result, (address)runtime_func, op1, op2);
  }
}

void CodeGenerator::double_unary_do(Value& result, Value& op1,
                                    BytecodeClosure::unary_op op JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();

  GUARANTEE(!result.is_present(), "result must not be present");
  GUARANTEE(op1.in_register(), "op1 must be in a register");

  GUARANTEE(op == BytecodeClosure::una_neg || op == BytecodeClosure::una_abs,
            "Sanity")

  assign_register(result, op1);
  Opcode opcode =  (op == BytecodeClosure::una_neg) ? _eor  : _bic;
  if (TARGET_MSW_FIRST_FOR_DOUBLE) {
    // The first word contains the sign bit
    arith(opcode, result.lo_register(),  op1.lo_register(), imm_rotate(2,2));
    mov_reg(      result.hi_register(),  op1.hi_register());
  } else {
    // The second word contains the sign bit
    arith(opcode, result.hi_register(),  op1.hi_register(), imm_rotate(2,2));
    mov_reg(      result.lo_register(),  op1.lo_register());
  }
}

void CodeGenerator::double_cmp(Value& result, BytecodeClosure::cond_op cond,
                               Value& op1, Value& op2 JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  JVM_SOFTFP_LINKAGE int (*runtime_func)(double, double);
  switch (cond) {
    case BytecodeClosure::lt:
      runtime_func = jvm_dcmpl; break;
    case BytecodeClosure::gt:
      runtime_func = jvm_dcmpg; break;
    default                 :
      runtime_func = 0; SHOULD_NOT_REACH_HERE(); break;
  }
  if (op1.is_immediate() && op2.is_immediate()) {
    result.set_int(runtime_func(op1.as_double(), op2.as_double()));
  } else {
    call_simple_c_runtime(result, (address)runtime_func, op1, op2);
  }
}
#endif

// Currently, this function is only used for floating point.
// It is actually rather generic and can be used for any C function
// that is guaranteed to never call into the VM.

void CodeGenerator::vcall_simple_c_runtime(Value& result,
                                        address runtime_func, ...) {
  GUARANTEE(runtime_func != 0, "sanity check");
  GUARANTEE(!Compiler::omit_stack_frame(), 
            "cannot call runtime functions with omitted compiled frame");
  int i;
  static const Register ctemps[] = { r0, r1, r2, r3, r12, lr };

  for (i = 0; i < ARRAY_SIZE(ctemps); i++) {
    RegisterAllocator::reference(ctemps[i]);
#if ENABLE_CSE
    RegisterAllocator::clear_notation(ctemps[i]);
    COMPILER_COMMENT(("clear reg %s", Disassembler::reg_name(ctemps[i])));
#endif

  }
  for (i = 0; i < ARRAY_SIZE(ctemps); i++) {
    frame()->unuse_register(ctemps[i]);
  }
  for (i = 0; i < ARRAY_SIZE(ctemps); i++) {
    RegisterAllocator::dereference(ctemps[i]);
  }

  va_list ap;
  va_start(ap, runtime_func);
    vsetup_c_args(ap);
  va_end(ap);

  mov_imm(r12, runtime_func);

  if (JavaStackDirection > 0 && sp == jsp) {
    // IMPL_NOTE:  We don't have to move to the C stack for the functions written
    // in assembly language in Interpreter_arm.s.
    // fcmpl, fcmpg, dcmpl, dcmpg, jvm_i2f, jvm_i2d, jvm_f2i
    mov(lr, reg(jsp));
    ldr_using_gp(sp, (address)&_primordial_sp);
    str(lr, imm_index(sp, -BytesPerWord, pre_indexed));
  }

  int offset = code_size();  // offset of the next instruction
  add(lr, pc, imm_rotate(0,0));

#if ENABLE_THUMB_VM
  // IMPL_NOTE: use a macro or inlined function to make this #if block
  // go away.
  bx(r12);
#else
  mov(pc, reg(r12));
#endif

  write_literals();
  if (!has_overflown_compiled_method()) {
    *(int *)addr_at(offset) |= imm(code_size() - offset - 8);
  }
  if (JavaStackDirection > 0 && sp == jsp) {
    ldr_imm_index(jsp, sp);
  }

  // We use "reference" rather than "allocate" since the register allocator
  // might think these are still in use from arguments.
  RegisterAllocator::reference(r0);
  if (result.is_two_word()) {
    RegisterAllocator::reference(r1);
    result.set_registers(r0, r1);
  } else {
    result.set_register(r0);
  }
}

void CodeGenerator::i2b(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  GUARANTEE(value.in_register(), "Immediate case already handled");
  write_literals_if_desperate();

  assign_register(result, value);

  const Register v = value.lo_register();
  const Register r = result.lo_register();
  mov(r, imm_shift(v, lsl, BitsPerWord - BitsPerByte));
  mov(r, imm_shift(r, asr, BitsPerWord - BitsPerByte));
}

void CodeGenerator::i2c(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  GUARANTEE(value.in_register(), "Immediate case already handled");
  write_literals_if_desperate();

  assign_register(result, value);

  const Register v = value.lo_register();
  const Register r = result.lo_register();
  mov(r, imm_shift(v, lsl, BitsPerWord - BitsPerShort));
  mov(r, imm_shift(r, lsr, BitsPerWord - BitsPerShort));
}

void CodeGenerator::i2s(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  GUARANTEE(value.in_register(), "Immediate case already handled");
  write_literals_if_desperate();

  assign_register(result, value);

  const Register v = value.lo_register();
  const Register r = result.lo_register();
  mov(r, imm_shift(v, lsl, BitsPerWord - BitsPerShort));
  mov(r, imm_shift(r, asr, BitsPerWord - BitsPerShort));
}

void CodeGenerator::i2l(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  GUARANTEE(value.in_register(), "Immediate case already handled");
  write_literals_if_desperate();
  RegisterAllocator::reference(value.lo_register());
  if (TARGET_MSW_FIRST_FOR_LONG) {
    result.set_registers(RegisterAllocator::allocate(), value.lo_register());
    mov(result.lo_register(), imm_shift(result.hi_register(), asr, 31));
  } else {
    result.set_registers(value.lo_register(), RegisterAllocator::allocate());
    mov(result.hi_register(), imm_shift(result.lo_register(), asr, 31));
  }
}

#if ENABLE_FLOAT

void CodeGenerator::i2f(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_float(::jvm_i2f(value.as_int()));
  } else {
    call_simple_c_runtime(result, (address)::jvm_i2f, value);
  }
}

void CodeGenerator::i2d(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_double(::jvm_i2d(value.as_int()));
  } else {
    call_simple_c_runtime(result, (address)::jvm_i2d, value);
  }
}

void CodeGenerator::l2f(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_float(::jvm_l2f(value.as_long()));
  } else {
    call_simple_c_runtime(result, (address)::jvm_l2f, value);
  }
}

void CodeGenerator::l2d(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_double(::jvm_l2d(value.as_long()));
  } else {
    call_simple_c_runtime(result, (address)::jvm_l2d, value);
  }
}

void CodeGenerator::f2i(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_int(::jvm_f2i(value.as_float()));
  } else {
    call_simple_c_runtime(result, (address)::jvm_f2i, value);
  }
}

void CodeGenerator::f2l(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_long(::jvm_f2l(value.as_float()));
  } else {
    call_simple_c_runtime(result, (address)::jvm_f2l, value);
  }
}

void CodeGenerator::f2d(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_double(::jvm_f2d(value.as_float()));
  } else {
    call_simple_c_runtime(result, (address)::jvm_f2d, value);
  }
}

void CodeGenerator::d2i(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_int(::jvm_d2i(value.as_double()));
  } else {
    call_simple_c_runtime(result, (address)::jvm_d2i, value);
  }
}

void CodeGenerator::d2l(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_long(::jvm_d2l(value.as_double()));
  } else {
    call_simple_c_runtime(result, (address)::jvm_d2l, value);
  }
}

void CodeGenerator::d2f(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_float(::jvm_d2f(value.as_double()));
  } else {
    call_simple_c_runtime(result, (address)::jvm_d2f, value);
  }
}
#endif

void CodeGenerator::larithmetic(Opcode opcode1, Opcode opcode2,
                                Value& result, Value& op1, Value& op2) {
  write_literals_if_desperate();
  assign_register(result, op1);

  // Remember that "lo" and "hi" refer to the address in memory.
  // For big endian machines, these are counter-intuitive
  Register A1 = op1.lsw_register();
  Register A2 = op1.msw_register();

  Register R1 = result.lsw_register();
  Register R2 = result.msw_register();

  // Setting set_CC in all cases wouldn't be bad, but the instruction
  // scheduler generates better code if we use set_CC sparingly
  CCMode mode = (opcode1 == opcode2) ? no_CC : set_CC;

  if (op2.is_immediate()) {
    CompilerLiteralAccessor cla;
    arith_imm(opcode1, R1, A1, op2.lsw_bits(), &cla, mode);
    arith_imm(opcode2, R2, A2, op2.msw_bits(), &cla);
  } else {
    arith(opcode1, R1, A1, reg(op2.lsw_register()), mode);
    arith(opcode2, R2, A2, reg(op2.msw_register()));
  }
}

void CodeGenerator::lmul(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  // Should eventually detect multiplication by small (0..10) and
  // by 32bit constants and generate better code for those cases.
  write_literals_if_desperate();

  if (op1.is_immediate()) {
    op1.materialize();
  }
  if (op2.is_immediate()) {
    op2.materialize();
  }
  result.assign_register();

  // Remember that "lo" and "hi" refer to the address in memory.
  // For big endian machines, these are counter-intuitive
  Register A1 = op1.lsw_register();
  Register A2 = op1.msw_register();

  Register B1 = op2.lsw_register();
  Register B2 = op2.msw_register();

  Register R1 = result.lsw_register();
  Register R2 = result.msw_register();

  // (A2*(2^32) + A1)*(B2*(2^32) + B1) =
  //            A2*B2*(2^64) + A2*B1(2^32) + A1*B2*(2^32) + A1*B1
  // ignore 2^64 term
  umull(R1, R2, A1, B1    ); // r =                               A1*B1
  mla  (    R2, A1, B2, R2); // r =                A1*B2*(2^32) + A1*B1
  mla  (    R2, A2, B1, R2); // r = A2*B1*(2^32) + A1*B2*(2^32) + A1*B1
}

void CodeGenerator::runtime_long_op(Value& result, Value& op1, Value& op2,
                                    bool check_zero, address routine JVM_TRAPS)
{
  write_literals_if_desperate();
  if (check_zero) {
    if (op2.in_register() || (op2.is_immediate() && op2.as_long() == 0)) {
      ZeroDivisorCheckStub::Raw zero =
          ZeroDivisorCheckStub::allocate_or_share(JVM_SINGLE_ARG_CHECK);
      if (op2.is_immediate()) {
        jmp(&zero);
      } else {
        TempRegister tmp;
        orr(tmp, op2.lo_register(), reg(op2.hi_register()), set_CC);
        b(&zero, eq);
      }
    }
  }
  call_simple_c_runtime(result, routine, op1, op2);
}

void CodeGenerator::ldiv(Value& result, Value& op1, Value& op2 JVM_TRAPS) {

  runtime_long_op(result, op1, op2, true, (address)jvm_ldiv JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::lrem(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  runtime_long_op(result, op1, op2, true, (address)jvm_lrem JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::lshift(Shift type,  Value& result,
                           Value& op1, Value& op2) {
  write_literals_if_desperate();
  if (op2.is_immediate()) {
    lshift_imm(type, result, op1, op2.as_int() & 63);
  } else {
    lshift_reg(type, result, op1, op2);
  }
}

void CodeGenerator::lshift_reg(Shift type,
                               Value& result, Value& op1, Value& op2) {
  result.assign_register();

  if (op1.is_immediate()) {
    op1.materialize();
  }

  // Remember that "lo" and "hi" refer to the address in memory.
  // For big endian machines, these are counter-intuitive
  Register A1 = op1.lsw_register();
  Register A2 = op1.msw_register();

  Register R1 = result.lsw_register();
  Register R2 = result.msw_register();

  TempRegister shift;
  Register unshift = (type == lsl ? R1 : R2);

  andr(shift, op2.lo_register(), imm(63));

  // Calculate 32 - shift and see if shift is >= 32
  rsb(unshift, shift, imm(32),   set_CC);
  sub(shift, shift, imm(32),                     le);
  switch(type) {
  case lsl:
    mov(R2, reg_shift(A1, lsl, shift),           le);
    mov(R1, zero,                                le);

    mov(R2, reg_shift(A2, lsl, shift),           gt);
    orr(R2, R2, reg_shift(A1, lsr, unshift),     gt);
    mov(R1, reg_shift(A1, lsl, shift),           gt);
    break;

  case asr: case lsr:
    mov(R1, reg_shift(A2, type, shift),           le);
    mov(R2, type == lsr ? zero : imm_shift(A2, asr, 31), le);

    mov(R1, reg_shift(A1, lsr, shift),           gt);
    orr(R1, R1, reg_shift(A2, lsl, unshift),     gt);
    mov(R2, reg_shift(A2, type, shift),          gt);
    break;
  }
}

void CodeGenerator::lshift_imm(Shift type,
                               Value& result, Value& op1, int shift) {
  GUARANTEE(0 <= shift && shift <= 63, "Code guarantee");

  if (shift == 0) {
    op1.copy(result);
    return;
  }
  // We could use
  //    assign_register(result, op1)
  // if shift == 1 || shift >= 32, since in those cases, there is no problem
  // with both using the same location.  Not really worth it.
  result.assign_register();

  Register A1 = op1.lsw_register();
  Register A2 = op1.msw_register();

  Register R1 = result.lsw_register();
  Register R2 = result.msw_register();

  if (shift == 1) {
    if (type == lsl) {
      // multiply by two
      add(R1, A1, reg(A1), set_CC);
      adc(R2, A2, reg(A2));
    } else {
      mov(R2, imm_shift(A2, type, 1), set_CC); // C gets bit that falls off
      mov(R1, imm_shift(A1, ror, 0));          // That's A1, RRX
    }
  } else if (shift < 32) {
    if (type == lsl) {
      mov(R2, imm_shift(A2, lsl, shift));
      orr(R2, R2, imm_shift(A1, lsr, 32 - shift));
      mov(R1, imm_shift(A1, lsl, shift));
    } else {
      mov(R1, imm_shift(A1, lsr, shift));
      orr(R1, R1, imm_shift(A2, lsl, 32-shift));
      mov(R2, imm_shift(A2, type, shift));
    }
  } else {
    if (type == lsl) {
      mov(R2, imm_shift(A1, lsl, shift - 32));
      mov(R1, zero);
    } else {
      // We have to be slightly careful here for shift == 32.
      // "lsl 0" does the right, but "asr 0" and "lsr 0" don't.
      mov(R1, imm_shift(A2, shift == 32 ? lsl : type, shift - 32));
      mov(R2, type == lsr ? zero : imm_shift(A2, asr, 31));
    }
  }
}

void CodeGenerator::cmp_values(Value& op1, Value& op2) {
  GUARANTEE(op1.in_register(), "op1 must be in a register");
  GUARANTEE(op2.is_immediate() || op2.in_register(),
            "op2 must be in a register or an immediate");

  const Register op1_lo_reg = op1.lo_register();
  if (op2.is_immediate()) {
    CompilerLiteralAccessor cla;
    cmp_imm(op1_lo_reg, op2.as_int(), &cla);
  } else {
    cmp(op1_lo_reg, reg(op2.lo_register()));
  }
}

void CodeGenerator::branch_if_do(BytecodeClosure::cond_op condition,
                                 Value& op1, Value& op2, int destination JVM_TRAPS)
{
  cmp_values(op1, op2);
  conditional_jump(condition, destination, true JVM_NO_CHECK_AT_BOTTOM);
}


void CodeGenerator::long_cmp(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  write_literals_if_desperate();
  GUARANTEE(!op1.is_immediate() || !op2.is_immediate(),
            "Immediate case handled by generic code");
  result.assign_register();
  Register rreg = result.lo_register();
  Value* arg1 = &op1;
  Value* arg2 = &op2;
  Value* temp;
  // Technically, lcmp is supposed to generate one of -1, 0, or +1.
  // But if we know that the next byte code is ifXX, we can product a value
  // that isn't correct, but is good enough for that bytecode.
  //
  // In the future, we could produce even better code by treating the
  // the lcmp and ifXX as a single byte code, without producing an
  // intermediate result.  But the compiler is not yet set up to do that
  // easily
  int next_bci = Compiler::current()->closure()->next_bytecode_index();
  bool negate = false;
  switch(method()->bytecode_at(next_bci)) {
    case Bytecodes::_ifeq:
    case Bytecodes::_ifne:
      // We want a value that is 0 if arg1 == arg2 and non-zero otherwise
      if (arg1->is_immediate()) {
        // The order of the arguments is immaterial
        temp = arg1; arg1 = arg2; arg2 = temp;
      }
      if (!arg2->is_immediate()) {
        eor(rreg, arg1->lsw_register(), reg(arg2->lsw_register()), set_CC);
        eor(rreg, arg1->msw_register(), reg(arg2->msw_register()), eq);
      } else {
        jlong value = arg2->as_long();
        if (value == 0) {
          orr(rreg, arg1->lsw_register(), reg(arg1->msw_register()));
        } else {
          CompilerLiteralAccessor cla;
          eor_imm(rreg, arg1->lsw_register(), arg2->lsw_bits(), &cla, set_CC);
          eor_imm(rreg, arg1->msw_register(), arg2->msw_bits(), &cla, no_CC, eq);
        }
      }
      break;

    case Bytecodes::_ifle:
    case Bytecodes::_ifgt:
      negate = true;
      temp = arg1; arg1 = arg2; arg2 = temp;
      /* Fall through */

    case Bytecodes::_iflt:
    case Bytecodes::_ifge:
      // if arg1 >= arg2, return value >= 0 (!negate) or <= 0 (negate)
      // if arg1 <  arg2, return value <  0 (!negate) or >  0 (negate)
      // Note that in the !negate case, only the sign of the result is
      // important
      if (arg1->is_immediate()) {
        CompilerLiteralAccessor cla;
        // rreg is a temporary.  We're just setting the condition codes
        rsb_imm(rreg, arg2->lsw_register(), arg1->lsw_bits(), &cla, set_CC);
        rsc_imm(rreg, arg2->msw_register(), arg1->msw_bits(), &cla, set_CC);
      } else if (!arg2->is_immediate()) {
        // rreg is a temporary.  We're just setting the condition codes
        cmp(      arg1->lsw_register(), reg(arg2->lsw_register()));
        sbc(rreg, arg1->msw_register(), reg(arg2->msw_register()), set_CC);
      } else if (arg2->as_long() == 0) {
        // Just use the high word of arg1
        if (negate) {
          cmp(arg1->msw_register(), zero);
        } else {
          // We can use the high word of arg1 as result
          // Note, we have to copy arg1->msw_register() rather than using
          // it directly since an OSR might occur and bash arg1 with the
          // >>literally correct<< result of the comparison
          mov(rreg, reg(arg1->msw_register()));
          // Skip the code below
          break;
        }
      } else {
        CompilerLiteralAccessor cla;
        cmp_imm(      arg1->lsw_register(), arg2->lsw_bits(), &cla);
        sbc_imm(rreg, arg1->msw_register(), arg2->msw_bits(), &cla, set_CC);
      }
      if (!negate) {
        // rreg contains the high word of arg1 - arg2, and the condition
        // codes indicate the sign of this result.  The overflow bit indicates
        // that the sign bit of rreg is opposite the correct sign of arg1-arg2
        mvn(rreg, reg(rreg), vs);
      } else {
        mov(rreg, one, lt);
        mov(rreg, zero, ge);
      }
      break;

  default:
    // Only in the test suite would lcmp be followed by something other
    // than ifxx.  This just isn't worth worrying about.
    //
    // If in the future, we decide that this is worth compiling (hah!), then
    // use the code commented out below instead of the following three lines.
    frame()->push(op2);
    frame()->push(op1);
    go_to_interpreter(JVM_SINGLE_ARG_CHECK);
    break;

#if NOT_CURRENTLY_USED
    if (op1.is_immediate()) { op1.materialize(); }
    if (op2.is_immediate()) { op2.materialize(); }

    const Register xlo = op1.lsw_register();
    const Register xhi = op1.msw_register();
    const Register ylo = op2.lsw_register();
    const Register yhi = op2.msw_register();
    const Register res = result.lsw_register();
    TempRegister tmp;

    COMPILER_COMMENT(("subtract arguments and 'or' results"));
    sub(res , xlo, reg(ylo ), set_CC);
    sbc(tmp, xhi, reg(yhi ), set_CC);
    orr(tmp, res, reg(tmp), no_CC);
    COMPILER_COMMENT(("the condition codes are set correctly "
                        "*only* for < and >="));
    mvn(res, imm(0), lt);
    COMPILER_COMMENT(("wrong if ==, but fixed below"));
    mov(res, imm(1), ge);
    COMPILER_COMMENT(("correction for =="));
    // Note: If we had an extra register besides tmp, we could keep
    //       alive the result of the first subtraction and do the orr
    //       here instead of the cmp. This would save one instruction.
    cmp(tmp, imm(0));
    mov(res, imm(0), eq);
    break;
#endif
  }
}

void CodeGenerator::check_bytecode_counter() {
  if (Deterministic) {
    Label det_done;
    Register reg = RegisterAllocator::allocate();

    get_bytecode_counter(reg);
    sub(reg, reg, imm(1), set_CC);
    b(det_done, ne);
#if ENABLE_XSCALE_WMMX_TIMER_TICK && !ENABLE_TIMER_THREAD
    wcmpeqb(wR0, wR0, wR0);
#else
    get_rt_timer_ticks(reg);
    add(reg, reg, imm(1));
    set_rt_timer_ticks(reg);
#endif
    mov_imm(reg, RESCHEDULE_COUNT);
    bind(det_done);
    set_bytecode_counter(reg);
    RegisterAllocator::dereference(reg);
  }
}

void CodeGenerator::check_timer_tick(JVM_SINGLE_ARG_TRAPS) {
  Label timer_tick, done;

  COMPILER_COMMENT(("check for timer tick"));
  TempRegister tmp;
#if ENABLE_XSCALE_WMMX_TIMER_TICK && !ENABLE_TIMER_THREAD
  textrcb(0);
  b(timer_tick, ne);
#else  
  get_rt_timer_ticks(tmp);
  cmp(tmp, imm(0));
  b(timer_tick, ne);
#endif
  write_literals_if_desperate();
bind(done);

  TimerTickStub::Raw stub =
      TimerTickStub::allocate(Compiler::bci(),
                              timer_tick, done JVM_NO_CHECK);
  if (stub.not_null()) {
    stub().insert();

    // If we go to the stub, we can't be guaranteed it has preserved literals
    frame()->clear_literals();
  }
}

void CodeGenerator::check_cast(Value& object, Value& klass, int class_id
                               JVM_TRAPS) {
  Label slow_case, done_checking;

  COMPILER_COMMENT(("Typecast type check"));
  frame()->push(object);

  // Since register allocation might cause spilling we have to allocate *all*
  // registers before checking for null object.
  TempRegister tmp1;
  TempRegister tmp2;

  COMPILER_COMMENT(("Check for NULL object, get its class if not null"));
  cmp(object.lo_register(), zero);
  ldr(tmp2, imm_index(object.lo_register()), ne);
  b(done_checking, eq);
  ldr(tmp2, imm_index(tmp2), ne);

  COMPILER_COMMENT(("Check the subtype caches"));
  ldr_imm_index(tmp1, tmp2, JavaClass::subtype_cache_1_offset());
  ldr_imm_index(tmp2, tmp2, JavaClass::subtype_cache_2_offset());

  cmp(tmp1, reg(klass.lo_register()));
  cmp(tmp2, reg(klass.lo_register()), ne);
  if (need_to_force_literals()) {
    b(done_checking, eq);
    b(slow_case, ne);
    write_literals();
  } else {
    b(slow_case, ne);
  }

  bind(done_checking);
  CheckCastStub::insert(bci(), class_id, slow_case, done_checking JVM_CHECK);
  frame()->pop(object);

  // If we go to the stub, we can't be guaranteed it has preserved literals
  frame()->clear_literals();
}

bool CodeGenerator::quick_check_cast(int class_id, Label& stub_label, 
                                     Label& return_label JVM_TRAPS) {
  bind(stub_label);
  { 
    PreserveVirtualStackFrameState state(frame() JVM_CHECK_0);
    // Check that [top of stack] is castable to class_id
    mov_imm(tmp0, class_id);
    call_through_gp(gp_compiler_checkcast_ptr JVM_CHECK_0);
  }
  jmp(return_label);
  return true;
}

bool CodeGenerator::quick_instance_of(int class_id JVM_TRAPS) {
  // Check that [top of stack] is instanceof class_id. The return value
  // (true or false) is stored in BinaryAssembler::return_register
  mov_imm(r0, class_id);
  call_through_gp(gp_compiler_instanceof_ptr JVM_CHECK_0);
  return true;
}

void CodeGenerator::instance_of(Value& result, Value& object,
                                Value& klass, int class_id JVM_TRAPS) {
  Label slow_case, done_checking;
  result.assign_register();

  COMPILER_COMMENT(("Instance-of type check"));
  frame()->push(object);

  // Since register allocation might cause spilling we have to allocate *all*
  // registers before checking for null object.
  TempRegister tmp1;
  TempRegister tmp2;

  COMPILER_COMMENT(("Check for NULL object; Get the class for the object"));
  cmp(object.lo_register(), zero);
  ldr(tmp2, imm_index(object.lo_register()), ne);
  mov(result.lo_register(), zero, eq);
  ldr(tmp2, imm_index(tmp2), ne);
  b(done_checking, eq);

  COMPILER_COMMENT(("Check the subtype caches"));
  ldr_imm_index(tmp1, tmp2, JavaClass::subtype_cache_1_offset());
  ldr_imm_index(tmp2, tmp2, JavaClass::subtype_cache_2_offset());
  cmp(tmp1, reg(klass.lo_register()));
  cmp(tmp2, reg(klass.lo_register()), ne);
  mov(result.lo_register(), one, eq);
  if (need_to_force_literals()) {
    b(done_checking, eq);
    b(slow_case, ne);
    write_literals();
  } else {
    b(slow_case, ne);
  }

  bind(done_checking);
  InstanceOfStub::Raw stub =
      InstanceOfStub::allocate(bci(), class_id, slow_case, done_checking,
                               result.lo_register() JVM_NO_CHECK);
  if (stub.not_null()) {
    stub().insert();
    frame()->pop(object);

    // If we go to the stub, we can't be guaranteed it has preserved literals
    frame()->clear_literals();
  }
}

void CodeGenerator::if_then_else(Value& result,
                                 BytecodeClosure::cond_op condition,
                                 Value& op1, Value& op2,
                                 ExtendedValue& result_true,
                                 ExtendedValue& result_false JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  cmp_values(op1, op2);
  // Destroy op1, op2 since quite often result_true is using same register
  op1.destroy(); op2.destroy();

  if (result_true.is_value()) {
    assign_register(result, result_true.value());
  } else {
    result.assign_register();
  }
  Condition cond = convert_condition(condition);
  move(result, result_true,  cond);
  move(result, result_false, not_cond(cond));
}

void CodeGenerator::if_iinc(Value& result, BytecodeClosure::cond_op condition,
                            Value& op1, Value& op2,
                            Value& arg, int increment JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  cmp_values(op1, op2);
  // Destroy op1, op2 since quite often value is using the same register
  op1.destroy(); op2.destroy();

  Condition cond = convert_condition(condition);
  if (arg.is_immediate()) {
    arg.materialize();
  }
  assign_register(result, arg);
  // We hope that the following generates no code!
  move(result, arg);
  const Register reg = result.lo_register();
  add_imm(reg, reg, increment, no_CC, cond);
}


void CodeGenerator::new_object(Value& result, JavaClass* klass JVM_TRAPS) {
  COMPILER_COMMENT(("new_object"));
#ifdef AZZERT
  InstanceSize size = klass->instance_size();
  GUARANTEE(size.is_fixed(), "Size must be fixed in order to do allocation");
#endif
  // Do flushing, and remember to unmap.
  flush_frame();

  // Handle finalization by going slow-case for objects with finalizers.
  if (klass->has_finalizer()) {
    // _newobject(Thread&, raw_class);
    ldr_oop(r1, klass);
    call_vm((address) _newobject, T_OBJECT JVM_CHECK);
  } else {
    ldr_oop(r0, klass);
    call_through_gp(gp_compiler_new_object_ptr JVM_CHECK);
  }
  // The result is in r0
  RegisterAllocator::reference(r0);
  result.set_register(r0);
}

void CodeGenerator::new_object_array(Value& result, JavaClass* element_class,
                                     Value& length JVM_TRAPS) {
  UsingFastOops fast_oops;
  JavaClass::Fast array_class = element_class->get_array_class(1 JVM_CHECK);
  JavaNear::Fast java_near = array_class().prototypical_near();

  // Do flushing, and remember to unmap.
  flush_frame();

  // Call the allocation routine.
  Value save_reg_for_oop(T_ILLEGAL);
  setup_c_args(2, &save_reg_for_oop, &length, NULL);
  ldr_oop(r0, &java_near);
  call_through_gp(gp_compiler_new_obj_array_ptr JVM_CHECK);

  // The result is in r0
  RegisterAllocator::reference(r0);
  result.set_register(r0);
}

void CodeGenerator::new_basic_array(Value& result, BasicType type,
                                    Value& length JVM_TRAPS) {
  UsingFastOops fast_oops;
  // Do flushing, and remember to unmap.
  flush_frame();

  TypeArrayClass* array_class = Universe::as_TypeArrayClass(type);
  JavaNear::Fast java_near = array_class->prototypical_near();

  Value actual_length(T_INT);
  Value save_reg_for_oop(T_ILLEGAL);

  if (length.is_immediate()) {
    int value = length.as_int();
    if (value < 0) {
      actual_length.set_int(0);
    } else {
      actual_length.set_int(ArrayDesc::allocation_size(value,
                                                       array_class->scale()));
    }
  } else {
    // Try to allocate actual_length into r2, if possible
    if (length.lo_register() != r2) {
      actual_length.set_register(RegisterAllocator::allocate(r2));
    } else {
      actual_length.assign_register();
    }
    switch(array_class->scale()) {
      case 1:
        add(actual_length.lo_register(), length.lo_register(),
            imm(Array::base_offset() + BytesPerWord - 1));
        bic(actual_length.lo_register(),
            actual_length.lo_register(), imm(3));
        break;
      case 2:
        mov(actual_length.lo_register(),
            imm_shift(length.lo_register(), lsl, 1));
        add(actual_length.lo_register(), actual_length.lo_register(),
            imm(Array::base_offset() + BytesPerWord - 1));
        bic(actual_length.lo_register(), actual_length.lo_register(), imm(3));
        break;
      default:
        mov(actual_length.lo_register(),
            imm_shift(length.lo_register(), lsl, jvm_log2(array_class->scale())));
        add(actual_length.lo_register(), actual_length.lo_register(),
            imm(Array::base_offset()));
        break;
    }
  }
  setup_c_args(3, &save_reg_for_oop, &length, &actual_length, NULL);
  ldr_oop(r0, &java_near);
  call_through_gp(gp_compiler_new_type_array_ptr JVM_CHECK);

  // The result is in r0
  RegisterAllocator::reference(r0);
  result.set_register(r0);
}

void CodeGenerator::new_multi_array(Value& result JVM_TRAPS) {
  flush_frame();

  // Call the runtime system.
  call_vm((address) multianewarray, T_ARRAY JVM_CHECK);

  // The result is in r0
  RegisterAllocator::reference(r0);
  result.set_register(r0);
}

void CodeGenerator::monitor_enter(Value& object JVM_TRAPS) {
  // For now we flush before calling the compiler monitor enter stub.
  flush_frame();
  mov_reg(r0, object.lo_register());
  call_through_gp(gp_shared_monitor_enter_ptr JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::monitor_exit(Value& object JVM_TRAPS) {
  // For now we flush before calling the compiler monitor exit stub.
  flush_frame();
  // Make sure the object is in register r0 (tos_val).
  mov_reg(r0, object.lo_register());
  call_through_gp(gp_shared_monitor_exit_ptr JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::return_result(Value& result JVM_TRAPS) {
  // unlocking of synchronized methods occurs with unlock_activation
  // setup result

  // Put the result into r0 or r0/r1
  setup_c_args(1, &result, NULL);
  restore_last_frame(JVM_SINGLE_ARG_CHECK);

  int imm32;
  int tag;
  if (TaggedJavaStack) {
    tag = (int)::basic_type2tag(result.stack_type());
  } else {
    tag = ::word_size_for(result.stack_type());
  }

  if (TaggedJavaStack) {
    if (frame()->has_literal_value(method_return_type, imm32) && imm32 == tag){
      // We can elide setting the stack type.  This happens rather frequently!
      if (GenerateCompilerAssertions) {
        cmp(method_return_type, imm(MAKE_IMM(tag)));
        breakpoint(ne);
      }
    } else {
      mov(method_return_type, imm(MAKE_IMM(tag)));
    }
  }
  // We always return to Java code or an assembly language stub
  mov(pc, reg(lr));
  write_literals();
}

void CodeGenerator::return_void(JVM_SINGLE_ARG_TRAPS) {
  // unlocking of synchronized methods occurs with unlock_activation
  COMPILER_COMMENT(("return void"));
  restore_last_frame(JVM_SINGLE_ARG_CHECK);

  if (TaggedJavaStack) {
    GUARANTEE(uninitialized_tag == 0, "Same result Tagged Stack or Not");
    int imm32;
    if (frame()->has_literal_value(method_return_type, imm32) && imm32 == 0) {
      // We can elide setting the stack type.  This happens rather frequently!
      if (GenerateCompilerAssertions) {
        cmp(method_return_type, zero);
        breakpoint(ne);
      }
    } else {
      mov(method_return_type, zero);
    }
  }
  mov(pc, reg(lr));
  // An excellent place to write literals
  write_literals();
}

void CodeGenerator::return_error(Value& value JVM_TRAPS) {
  // This looks almost like return_void, except that we save
  // the return address in lr, and put the error into r0
  COMPILER_COMMENT(("return with error"));
  mov_reg(r1, value.lo_register());
  restore_last_frame(JVM_SINGLE_ARG_CHECK);
  long offset = (long)&gp_shared_call_vm_exception_ptr - (long)&gp_base_label;
  ldr(pc, imm_index(gp, offset));
  write_literals();
}

void CodeGenerator::restore_last_frame(JVM_SINGLE_ARG_TRAPS) {
  Method *m = method();
  if (Compiler::omit_stack_frame()) {
    int params = m->size_of_parameters();
    if (params > 0) {
      add_imm(jsp, jsp, - params * JavaStackDirection * BytesPerStackElement);
    }
  } else {
    jint locals = m->max_locals();
    jint caller_jsp_offset_from_jsp = locals * BytesPerStackElement + 
                                      JavaFrame::frame_desc_size();
    if (!method()->access_flags().is_synchronized() &&
        !method()->access_flags().has_monitor_bytecodes() &&
        ENABLE_FULL_STACK && 
        (frame()->stack_pointer() - locals) == -1 &&
        (JavaFrame::empty_stack_offset() == 
         JavaFrame::return_address_offset()) &&
        (JavaStackDirection < 0) &&
        (has_room_for_imm(caller_jsp_offset_from_jsp, 12))) {
      // Special case -- if jsp is already pointing to the return address,
      // we can save one instruction by using post-index addressing mode

      GUARANTEE(JavaFrame::caller_fp_offset() == 0, "Code assumption");
      int fp_offset_from_caller_jsp = - (locals * BytesPerStackElement +
                                         JavaFrame::end_of_locals_offset());
      // Fetch lr, and caller's jsp
      ldr(lr, imm_index(jsp, caller_jsp_offset_from_jsp, post_indexed));

      // Restore caller's fp
      ldr_imm_index(fp, jsp, fp_offset_from_caller_jsp);
    } else {
      jint offset = JavaFrame::end_of_locals_offset()
                 -  locals * JavaStackDirection * BytesPerStackElement;
      // We can avoid loading a memory location by dead reckoning the new value
      // of jsp off of fp, then by pulling it out of the frame.
      ldr_imm_index(lr, fp, JavaFrame::return_address_offset());

      mov(jsp, reg(fp));
      GUARANTEE(JavaFrame::caller_fp_offset() == 0, "Code assumption");
      if (!has_room_for_imm(abs(offset), 12)) {
        Compiler::abort_active_compilation(true JVM_THROW);
      }
      ldr(fp,  imm_index(jsp, offset, post_indexed));
    }
  }
}

// Throw exception in the simple case (the method is not synchronized,
// has no monitor bytecodes, and no handler in the current method covers
// this exception).
void CodeGenerator::throw_simple_exception(int rte JVM_TRAPS) {

  long offset = 0;

  if (rte == ThrowExceptionStub::rte_null_pointer) {
    int params = method()->size_of_parameters();
    if(Compiler::omit_stack_frame()) {
      address &target = gp_compiler_throw_NullPointerException_10_ptr;
      mov_imm(r0, - params * JavaStackDirection * BytesPerStackElement);
      offset = (long)&target - (long)&gp_base_label;
    } else {
      address &target = gp_compiler_throw_NullPointerException_ptr;
      mov_imm(r0, method()->max_locals());
      offset = (long)&target - (long)&gp_base_label;
    }
    ldr_imm_index(pc, gp, offset);
  } else if (rte == ThrowExceptionStub::rte_array_index_out_of_bounds) {
    int params = method()->size_of_parameters();

    if(Compiler::omit_stack_frame()) {
      address &target = gp_compiler_throw_ArrayIndexOutOfBoundsException_10_ptr;
      mov_imm(r0, - params * JavaStackDirection * BytesPerStackElement);
      offset = (long)&target - (long)&gp_base_label;
    } else {
      address &target = gp_compiler_throw_ArrayIndexOutOfBoundsException_ptr;
      mov_imm(r0, method()->max_locals());
      offset = (long)&target - (long)&gp_base_label;
    }
    ldr_imm_index(pc, gp, offset);
  } else {
    // IMPL_NOTE: old code
    frame()->clear();
    Value exception(T_OBJECT);
    call_vm(ThrowExceptionStub::exception_allocator(rte), T_OBJECT JVM_CHECK);
    exception.set_register(
                RegisterAllocator::allocate(Assembler::return_register));
    return_error(exception JVM_NO_CHECK_AT_BOTTOM);
  }
}

void CodeGenerator::unlock_activation(JVM_SINGLE_ARG_TRAPS) {
  GUARANTEE(method()->access_flags().is_synchronized(), "Sanity check");
  GUARANTEE(ROM::is_synchronized_method_allowed(method()), "sanity");

  flush_frame();
  call_through_gp(gp_shared_unlock_synchronized_method_ptr
                  JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::check_monitors(JVM_SINGLE_ARG_TRAPS) {
  // Make sure there are no locked monitors on the stack.
  Label unlocking_loop, unlocking_loop_entry;

  TempRegister lock;
  TempRegister object;
  TempRegister end;

  write_literals_if_desperate();

  // Add the stub for the unlock exception.
  UnlockExceptionStub::Raw unlock_exception =
      UnlockExceptionStub::allocate_or_share(JVM_SINGLE_ARG_CHECK);

  CompilerLiteralAccessor cla;
  COMPILER_COMMENT(("Point at the object of the topmost stack lock"));
  ldr_imm_index(lock, fp, JavaFrame::stack_bottom_pointer_offset());
  add_imm(end, fp,
          JavaFrame::pre_first_stack_lock_offset() + StackLock::size(), &cla);
  if (JavaStackDirection < 0) {
    add(lock, lock, imm(StackLock::size()));
  }
  // lock points at the object field of the first lock
  // end points to the object field of beyond the final lock

  b(unlocking_loop_entry);

bind(unlocking_loop);
  cmp(object, zero);
  b(&unlock_exception, ne);

bind(unlocking_loop_entry);
  cmp(lock, reg(end));
  if (GenerateCompilerAssertions) {
    breakpoint(JavaStackDirection < 0 ? hi : lo);
  }
  ldr(object,
      imm_index(lock,
                -JavaStackDirection * (BytesPerWord + StackLock::size()),
                post_indexed),
      ne);
  b(unlocking_loop, ne);
}

void CodeGenerator::table_switch(Value& index, jint table_index,
                                 jint default_dest, jint low, jint high JVM_TRAPS) {
  GUARANTEE(index.in_register(), "Immediates handled by caller");
  COMPILER_COMMENT(("tableswitch"));

  if (default_dest <= bci()) {
    // Negative offset in a branch table is not a usual case
    Compiler::abort_active_compilation(true JVM_THROW);
  }

  int table_size = high - low + 1;

  if (table_size >= MAX_TABLE_SWITCH_SIZE) {
    // Need to avoid overflow loading literals that have been used in preceding
    // bytecodes.
    Compiler::abort_active_compilation(true JVM_THROW);
  }

  int jump_table_bytes = table_size *sizeof(int);
  int total_bytes = jump_table_bytes  + 128; // be conservative and use 128 byte pad
  write_literals_if_desperate(total_bytes);

  Register entry = RegisterAllocator::allocate();
  sub_imm(entry, index.lo_register(), low);

  CompilerLiteralAccessor cla;
  cmp_imm(entry, high - low, &cla);

  add(pc, pc, imm_shift(entry, lsl, 2), ls);
  {
    // We fall through to here if not in the range low <= index <= high
    Label label;
    b(label, hi);
    CompilationContinuation::insert(default_dest, label JVM_CHECK);
  }
  for (int i = 0; i < (high - low + 1); i++) {
    // Create a branch for each target
    int jump_offset = method()->get_java_switch_int(4 * i + table_index + 12);
    if (jump_offset <= 0) {
      // Negative offset in a branch table is not a usual case
      Compiler::abort_active_compilation(true JVM_THROW);
    }
    Label label;
    b(label);
    CompilationContinuation::insert(bci() + jump_offset, label JVM_CHECK);
  }
  write_literals();
  RegisterAllocator::dereference(entry);
}

bool CodeGenerator::dense_lookup_switch(Value& index, jint table_index,
                                        jint default_dest, jint num_of_pairs
                                        JVM_TRAPS) {
  int i, offset;
  int last_dense = -1;

  // For now we handle only dense tables with a low boundary of 0.
  for (i = 0, offset = table_index + 8; i < num_of_pairs; i++, offset += 8) {
    int key = method()->get_java_switch_int(offset);
    if (key != last_dense + 1) {
      break;
    } else {
      last_dense = key;
    }
  }

  int table_size = last_dense+1;
  if (table_size < 3) {
    // Not worth optimizing
    return false;
  }
  if (table_size > MAX_TABLE_SWITCH_SIZE) {
    // Avoid same literal overflow problem as in lookup_switch()
    return false;
  }
  
  // Same issue as in lookup_switch() -- force preceding literals to be written
  // if they are close to overflow.
  int jump_table_bytes = table_size *sizeof(int);
  int total_bytes = jump_table_bytes  + 128; // be conservative and use 128 byte pad
  write_literals_if_desperate(total_bytes);

  CompilerLiteralAccessor cla;
  cmp_imm(index.lo_register(), last_dense, &cla);

  Label slow_lookup;

  add(pc, pc, imm_shift(index.lo_register(), lsl, 2), ls);
  b(slow_lookup, hi); // We fall through to here if !(0 <= index <= last_dense)

  for (i = 0, offset = table_index + 8; i <= last_dense; i++, offset += 8) {
    int jump_offset = method()->get_java_switch_int(offset + 4);
    Label label;
    b(label);
    CompilationContinuation::insert(bci() + jump_offset, label JVM_CHECK_0);
  }
  write_literals();

bind(slow_lookup);
  for (; i < num_of_pairs; i++, offset += 8) {
    int key = method()->get_java_switch_int(offset);
    int jump_offset = method()->get_java_switch_int(offset + 4);
    cmp_imm(index.lo_register(), key, &cla);
    conditional_jump(BytecodeClosure::eq, bci() + jump_offset, false JVM_CHECK_0);
  }
  branch(default_dest JVM_CHECK_0);

  return true;
}

void CodeGenerator::lookup_switch(Value& index, jint table_index,
                                  jint default_dest, jint num_of_pairs JVM_TRAPS) {
  // No need to do the same checking as in CodeGenerator::table_switch(), because
  // Literals are written inside conditional_jump().
  int i, offset;

  // (1) Check for negative offsets
  for (i = 0, offset = table_index + 8; i < num_of_pairs; i++, offset += 8) {
    int jump_offset = method()->get_java_switch_int(offset + 4);
    if (jump_offset <= 0) {
      // Negative offset in a branch table is not a usual case
      Compiler::abort_active_compilation(true JVM_THROW);
    }
  }

  // (2) Compile dense tables with a branch table.
  bool dense_ok = dense_lookup_switch(index, table_index, default_dest, 
                                      num_of_pairs JVM_CHECK);
  if (dense_ok) {
    return;
  }

  // (3) compile it the slow way
  CompilerLiteralAccessor cla;
  for (i = 0, offset = table_index + 8; i < num_of_pairs; i++, offset += 8) {
    int key = method()->get_java_switch_int(offset);
    int jump_offset = method()->get_java_switch_int(offset + 4);
    cmp_imm(index.lo_register(), key, &cla);
    conditional_jump(BytecodeClosure::eq, bci() + jump_offset, false JVM_CHECK);
  }
  branch(default_dest JVM_NO_CHECK_AT_BOTTOM);
}

#if NOT_CURRENTLY_USED

void CodeGenerator::lookup_switch(Value& index, jint table_index,
                                  jint default_dest, jint num_of_pairs JVM_TRAPS) {
  lookup_switch(index.lo_register(), table_index, 0, num_of_pairs - 1,
                default_dest JVM_CHECK);
  branch(default_dest JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::lookup_switch(Register index, jint table_index,
                                  jint start, jint end,
                                  jint default_dest JVM_TRAPS) {
  CompilerLiteralAccessor cla;
#if USE_COMPILER_COMMENTS
  char buffer[200];
  int num_of_pairs  = method()->get_java_switch_int(table_index + 4);
  int low = (start == 0)
        ? -0x80000000
        : method()->get_java_switch_int(8 * (start - 1) + table_index + 8) + 1;
  int high = (end + 1 == num_of_pairs)
        ? 0x7FFFFFFF
        : method()->get_java_switch_int(8 * (end + 1) + table_index + 8) - 1;
  jvm_sprintf(buffer, "0x%x <= r%d <= 0x%x", low, index, high);
  comment(buffer);
  frame()->dump(true);
#endif
  if (end - start <= 4) {
    for (int i = start; i <= end; i++) {
      int key = method()->get_java_switch_int(8 * i + table_index + 8);
      int jump_bci =
          bci() + method()->get_java_switch_int(8 * i + table_index + 12);
      cmp_imm(index, key, &cla);
      conditional_jump(BytecodeClosure::eq, jump_bci, false JVM_CHECK);
    }
    // Allowed to fall through on default
  } else {
    Label larger, smaller_default;
    int i = (start + end) >> 1;
    int key = method()->get_java_switch_int(8 * i + table_index + 8);
    int jump_bci =
             bci() + method()->get_java_switch_int(8 * i + table_index + 12);

    cmp_imm(index, key, &cla);
    conditional_jump(BytecodeClosure::eq, jump_bci, false JVM_CHECK);
    b(larger, gt);

    {
      PreserveVirtualStackFrameState state(frame() JVM_CHECK);
      // Handle start .. i - 1
      lookup_switch(index, table_index, start, i-1, default_dest JVM_CHECK);
      b(smaller_default);
      CompilationContinuation::insert(default_dest, smaller_default
                                      JVM_CHECK);
    }

  bind(larger);
    {
      PreserveVirtualStackFrameState state(frame() JVM_CHECK);
      // Handle start .. i - 1
      lookup_switch(index, table_index, i+1, end, default_dest JVM_CHECK);
      // Allowed to fall through
    }
  }
}

#endif

void CodeGenerator::invoke(Method* method, bool must_do_null_check JVM_TRAPS) {
  bool is_native = false;
  int size_of_parameters = method->size_of_parameters();

  Value hold_method(T_OBJECT);
  Value hold_tmp(T_INT);

  RegisterAllocator::reference(lr);     // tmp cannot be lr
  RegisterAllocator::reference(callee); // tmp cannot be Register::callee
  hold_tmp.assign_register();
  RegisterAllocator::dereference(callee);
  RegisterAllocator::dereference(lr);

  Register tmp = hold_tmp.lo_register();
  GUARANTEE(tmp != lr,     "Must not have lr as temporary register");
  GUARANTEE(tmp != callee, "Must not have callee as temporary register");

  frame()->commit_changes(callee);

  if (must_do_null_check) {
    if (frame()->reveiver_must_be_nonnull(size_of_parameters)) {
      ldr_oop(callee, method);
    } else {
      Value receiver(T_OBJECT);
      Assembler::Condition cond;
      frame()->receiver(receiver, size_of_parameters);
#if ENABLE_NPCE
      if (receiver.must_be_null()) {
        cond = maybe_null_check_1(receiver);
        ldr_oop(callee, method);
        maybe_null_check_2(cond JVM_CHECK);
      } else {
        bool is_npe;
        cond = maybe_null_check_1_signal(receiver, is_npe);
#ifndef PRODUCT         
        if (PrintCompiledCodeAsYouGo) {
          TTY_TRACE_CR((" generate a faked ldr instruction invoke=>\n"));
        }
#endif
        ldr(tmp, imm_index(receiver.lo_register()), cond);
        maybe_null_check_2_signal(cond,is_npe JVM_CHECK);
        ldr_oop(callee, method);
      }
#else
      cond = maybe_null_check_1(receiver);
      ldr_oop(callee, method);
      maybe_null_check_2(cond JVM_CHECK);
#endif
    }
  } else {
    ldr_oop(callee, method);
  }

  // The method must be in callee when we enter it
  hold_method.set_register(RegisterAllocator::allocate(callee));
  GUARANTEE(!frame()->is_mapping_something(callee), "Must be free");

#if CROSS_GENERATOR
  bool try_skip_lookup = true;
  // IMPL_NOTE: this needs to be fixed so that the AOT-compiled code can 
  // directly invoke the method without a look up
  if (method->is_native() && 
      method->get_native_code() == (address)Java_unimplemented) {
    // Used by AOT only: we are calling a MIDP native method, which
    // is not resolved during romization
    try_skip_lookup = false;
  } else if (method->is_quick_native() && 
             method->get_quick_native_code() == (address)Java_unimplemented) {
    try_skip_lookup = false;
  }
#else
  const bool try_skip_lookup = true;
#endif

  if (try_skip_lookup && method->is_impossible_to_compile()) {
    // We don't need to hunt for the entry.  Its current entry will not change
    const bool can_optimize_quick_natives = !ENABLE_WTK_PROFILER;

    if (can_optimize_quick_natives && method->is_quick_native()) {
      address native_code = method->get_quick_native_code();
      // We actually only need to flush the end of the stack containing the
      // arguments, but we don't really have any way of doing that..
      flush_frame();
      if (size_of_parameters > 0) {
        add_imm(jsp, jsp,
              size_of_parameters * -JavaStackDirection * BytesPerStackElement);
        int offset = method->is_static()
                     ? JavaFrame::arg_offset_from_sp(0)
                     : JavaFrame::arg_offset_from_sp(-1);
        if (offset == 0) {
          set_kni_parameter_base(jsp);
        } else {
          add_imm(tmp, jsp, offset);
          set_kni_parameter_base(tmp);
        }
      } else {
        mov(tmp, zero);
        set_kni_parameter_base(tmp);
      }
#ifdef AZZERT
      mov(tmp, one);
      set_jvm_in_quick_native_method(tmp);
#endif
      Value result(T_INT);      // this is a lie.  But vcall wants something
      vcall_simple_c_runtime(result, (address)native_code, NULL);
#ifdef AZZERT
      mov(tmp, zero);
      set_jvm_in_quick_native_method(tmp);
#endif
      is_native = true;
    } else {
      address target = method->execution_entry();
      mov_imm(tmp, target);
      flush_frame();
#if ENABLE_TRAMPOLINE && !CROSS_GENERATOR      
      call_from_compiled_code(method, tmp, 0, size_of_parameters JVM_CHECK);
#else
      call_from_compiled_code(tmp, 0, size_of_parameters JVM_CHECK);
#endif
    }
  } else {
    {
      GUARANTEE(!frame()->is_mapping_something(tmp), "Must be free");
      GUARANTEE(!frame()->is_mapping_something(callee), "Must be free");
      CodeInterleaver weaver(this);
        // WARNING: Each of these instructions in the first part must be a
        // simple instruction that doesn't use literals or labels or anything
        // like that.  Each  of these instructions get interleaved with the
        // flush() below.
        // The first instruction of the flush() comes before the first
        // ldr(tmp...) below
        if (ObjectHeap::contains_moveable(method->obj()) && !GenerateROMImage) {
          ldr_imm_index(tmp, callee, Method::heap_execution_entry_offset());
        } else {
          ldr_imm_index(tmp, callee, Method::variable_part_offset());
          ldr_imm_index(tmp, tmp);
        }
      weaver.start_alternate(JVM_SINGLE_ARG_CHECK);
        flush_frame();
      weaver.flush();
    }
    // invoke the method
#if ENABLE_TRAMPOLINE && !CROSS_GENERATOR          
    call_from_compiled_code(method, tmp, 0, size_of_parameters JVM_CHECK);
#else
    call_from_compiled_code(tmp, 0, size_of_parameters JVM_CHECK);
#endif
  }

  Signature::Raw signature = method->signature();
  adjust_for_invoke(method->size_of_parameters(),
                    signature().return_type(), is_native);

#if ENABLE_WTK_PROFILER
  flush_frame();
  call_vm((address)jprof_record_method_transition, T_VOID JVM_CHECK);
#endif
}
#if ENABLE_INLINE && ARM
void CodeGenerator::virtual_method_override_verify(
                            Method* method, ClassInfo* info  JVM_TRAPS) {
  int size_of_parameters = method->size_of_parameters();
  Assembler::Condition cond;

  Value hold_method(T_OBJECT);
  Value hold_tmp(T_INT);

  RegisterAllocator::reference(lr);  // tmp cannot be lr
  hold_tmp.assign_register();
  RegisterAllocator::dereference(lr);

  Register tmp = hold_tmp.lo_register();
  
  {
    Value receiver(T_OBJECT);
    frame()->receiver(receiver, size_of_parameters);
    if (receiver.must_be_null()) {
      // The receiver is known to be null at compile time!
      go_to_interpreter(JVM_SINGLE_ARG_CHECK);
      return;
    }
#if ENABLE_NPCE
    bool is_npce;
    cond = maybe_null_check_1_signal(receiver, is_npce);
#else
    cond = maybe_null_check_1(receiver);
#endif //ENABLE_NPCE
    ldr(tmp, imm_index(receiver.lo_register()), cond);

#if ENABLE_NPCE
    maybe_null_check_2_signal(cond, is_npce JVM_CHECK);
#else
    maybe_null_check_2(cond JVM_CHECK);
#endif //ENABLE_NPCE
  }
  ldr(tmp, imm_index(tmp, JavaNear::class_info_offset()));
  TempRegister tmp2;
  ldr_oop(tmp2.reg(), info);
  cmp(tmp, reg(tmp2.reg()));

}

#endif
void CodeGenerator::invoke_virtual(Method* method, int vtable_index,
                                   BasicType return_type JVM_TRAPS) {
  GUARANTEE(vtable_index >= 0, "Must be positive");

  int size_of_parameters = method->size_of_parameters();
  Assembler::Condition cond;

  Value hold_tmp(T_INT);

  RegisterAllocator::reference(lr);      // tmp cannot be lr
  RegisterAllocator::reference(callee);  // tmp cannot be Register::callee
  hold_tmp.assign_register();
  RegisterAllocator::dereference(lr);
  RegisterAllocator::dereference(callee);

  Register tmp = hold_tmp.lo_register();
  GUARANTEE(tmp != lr,     "Must not have lr as temporary register");
  GUARANTEE(tmp != callee, "Must not have callee as temporary register");

  int load_near_offset;
  {
    Value receiver(T_OBJECT);
    frame()->receiver(receiver, size_of_parameters);
    if (receiver.must_be_null()) {
      // We are guaranteed to be in the test suite.  The receiver is known to
      // be null at compile time!  This just isn't worth worrying about
      go_to_interpreter(JVM_SINGLE_ARG_CHECK);
      return;
    }

#if ENABLE_NPCE
    bool is_npe;
    cond = maybe_null_check_1_signal(receiver, is_npe);
#else
    cond = maybe_null_check_1(receiver);
#endif

    ldr(tmp, imm_index(receiver.lo_register()), cond);
    load_near_offset = code_size();

#if ENABLE_NPCE
    maybe_null_check_2_signal(cond,is_npe JVM_CHECK);
#else
    maybe_null_check_2(cond JVM_CHECK);
#endif
  }

  // This would flush callee to the stack if necessary.
  Value hold_method(T_OBJECT);
  hold_method.set_register(RegisterAllocator::allocate(callee));

  const bool preload_class_info = (code_size() > load_near_offset);

  if (preload_class_info) {
    // Poor-boy's code scheduler. If some code appears after tmp was loaded,
    // tmp should be ready now (at least on ARM7/StrongARM). By loading it here
    // instead of inside the CodeInterleaver block below, sometimes we can
    // avoid one stall.
    ldr_imm_index(tmp, tmp, JavaNear::class_info_offset());
  }

  {
    GUARANTEE(!frame()->is_mapping_something(tmp), "Must be free");
    GUARANTEE(!frame()->is_mapping_something(callee), "Must be free");

    CodeInterleaver weaver(this);
      // WARNING: Each of these instructions in the first part must be a
      // simple instruction that doesn't use literals or labels or anything
      // like that.  Each  of these instructions get interleaved with the
      // flush() below.
      // The first instruction of the flush() comes before the first
      // ldr(tmp...) below
      //
      // If the IMPL_NOTE below ever gets done, the constant must be generated
      // outside of the interleaver if it generates a literal.
      if (!preload_class_info) {
        ldr_imm_index(tmp, tmp, JavaNear::class_info_offset());
      }

      // tmp = ClassInfo
      // IMPL_NOTE: how large can the constant be?
      //        Do we need to compute it more slowly.  See comment above
      const int offset = vtable_index * 4 + ClassInfoDesc::header_size();
      if (!has_room_for_imm(abs(offset), 12)) {
        Compiler::abort_active_compilation(true JVM_THROW);
      }
      ldr_imm_index(callee, tmp, offset);

      if (ObjectHeap::contains_moveable(method->obj()) && !GenerateROMImage) {
        ldr_imm_index(tmp, callee, Method::heap_execution_entry_offset());
      } else {
        ldr_imm_index(tmp, callee, Method::variable_part_offset());
        ldr_imm_index(tmp, tmp);
      }
    weaver.start_alternate(JVM_SINGLE_ARG_CHECK);
      flush_frame();
    weaver.flush();
  }
  call_from_compiled_code(tmp, 0, size_of_parameters JVM_CHECK);

  adjust_for_invoke(size_of_parameters, return_type);

#if ENABLE_WTK_PROFILER 
  flush_frame();
  call_vm((address)jprof_record_method_transition, T_VOID JVM_CHECK);
#endif
}

void CodeGenerator::invoke_interface(JavaClass* klass, int itable_index,
                                     int parameters_size,
                                     BasicType return_type JVM_TRAPS) {

  UsingFastOops fast_oops;

  IncompatibleClassChangeStub::Fast icc_stub =
        IncompatibleClassChangeStub::allocate_or_share(JVM_SINGLE_ARG_CHECK);

  // Make sure that tmp0 isn't taken by receiver, below
  frame()->spill_register(tmp0);
  Value tmp(T_OBJECT);
  tmp.set_register(RegisterAllocator::allocate(tmp0));

  // Check for NULL
  Assembler::Condition cond;
  Value receiver(T_OBJECT);
  frame()->receiver(receiver, parameters_size);
  if (receiver.must_be_null()) {
    // We are guaranteed to be in the test suite.  The receiver is known to
    // be null at compile time!  This just isn't worth worrying about
    go_to_interpreter(JVM_SINGLE_ARG_CHECK);
    return;
  }
#if ENABLE_NPCE
  bool is_npe;
  cond = maybe_null_check_1_signal(receiver, is_npe);
#else
  cond = maybe_null_check_1(receiver);
#endif

  ldr(tmp0, imm_index(receiver.lo_register()), cond);
#if ENABLE_NPCE
  maybe_null_check_2_signal(cond, is_npe JVM_CHECK);
#else
  maybe_null_check_2(cond JVM_CHECK);
#endif
  ldr(tmp0, imm_index(tmp0), cond);

  // Flush the virtual stack frame and unmap everything.
  flush_frame();

  // tmp0: klass of receiver
  // tmp1:
  // tmp3:
  // tmp4:
  // tos_val: scratch

  ldr_imm_index(tmp0, tmp0, JavaClass::class_info_offset());
  // tmp0: ClassInfo of receiver

  // Get the itable from the ClassInfo of the receiver object.
  ldrh(tmp1, imm_index3(tmp0, ClassInfo::vtable_length_offset()));
  ldrh(tmp4, imm_index3(tmp0, ClassInfo::itable_length_offset()));

  add_imm(tmp2, tmp0, ClassInfoDesc::header_size() - 2 * BytesPerWord);
  add(tmp2, tmp2, imm_shift(tmp1, lsl, 2));
  // tmp2: itable entries

  mov_imm(tmp3, klass->class_id());
  // tmp3: klass_index of interface

  // Lookup interface method table by linear search
  Label lookup, error;
bind(lookup);
  sub(tmp4, tmp4, one, set_CC);
  ldr(tos_val, imm_index(tmp2, 2 *BytesPerWord, pre_indexed), ge);
  b(&icc_stub, lt);
  cmp(tos_val, reg(tmp3));
  b(lookup, ne);

  // Found the itable entry - now get the method table offset from there
  ldr_imm_index(tmp1, tmp2, BytesPerWord);

  // Now get the method
  add_imm(callee, tmp0, BytesPerWord * itable_index);
  ldr(callee, add_index(callee, tmp1));

  // Get the method entry from the method.
  // We require that Register::callee contain the method
  ldr_imm_index(tmp4, callee, Method::variable_part_offset());
  ldr_imm_index(tmp4, tmp4);
  call_from_compiled_code(tmp4, 0, parameters_size JVM_CHECK);

  adjust_for_invoke(parameters_size, return_type);

#if ENABLE_WTK_PROFILER 
  flush_frame();
  call_vm((address)jprof_record_method_transition, T_VOID JVM_CHECK);
#endif
}

void CodeGenerator::adjust_for_invoke(int parameters_size,
                                      BasicType return_type, bool native) {
  if (!native) {
    if (TaggedJavaStack) {
      int tag = ::basic_type2tag(stack_type_for(return_type));
      // Make sure the return value is tagged correctly
      // Only Java code sets the return type.  Native code doesn't.
      if (GenerateCompilerAssertions) {
        cmp(method_return_type, imm(MAKE_IMM(tag)));
        breakpoint(ne);
      }
      frame()->set_has_literal_value(method_return_type, tag);
    }
  }

  // Pretend that we returned a void.
  frame()->adjust_for_invoke(parameters_size, T_VOID);
  if (return_type != T_VOID) {
    // Like magic, the actual return value(s) are in registers.
    Value result(return_type);

    RegisterAllocator::reference(r0);
    if (result.is_two_word()) {
      RegisterAllocator::reference(r1);
      result.set_registers(r0, r1);
    } else {
      result.set_register(r0);
    }
    frame()->push(result);
  }
}

void CodeGenerator::invoke_native(BasicType return_kind, address entry JVM_TRAPS) {
  Label redo;
  bind(redo);
  GUARANTEE(method()->max_locals() == method()->size_of_parameters(),
            "invoke_native can happen only in native method");

  // Set _kni_parameter_base to point to first native param. Note that
  // this must be done in the redo loop: when the invokenative is being
  // redone, another native method may have executed and overwritten
  // _kni_parameter_base.
  if (method()->size_of_parameters() > 0) {
    LocationAddress base(0, T_OBJECT); // Type is immaterial
    int offset = base.get_fixed_offset(); // Offset from fp of first argument
    if (method()->is_static()) {
      // KNI-ism, fake parameter slot for static method
      offset += -JavaStackDirection * BytesPerStackElement;
      COMPILER_COMMENT(("Set _kni_parameter_base (static method)"));
    } else {
      COMPILER_COMMENT(("Set _kni_parameter_base (virtual method)"));
    }
    add_imm(tmp2, fp, offset);
  } else {
    GUARANTEE(method()->is_static(), "Of course");
    mov(tmp2, zero);
  }
  set_kni_parameter_base(tmp2);

#if ENABLE_PROFILER
  if (UseProfiler) {
    COMPILER_COMMENT(("Inform Profiler we're inside native method"));

    mov_imm(tmp1, 1);
    mov_imm(tmp2, (int)&_jvm_profiler_in_native_method);
    str(tmp1, imm_index(tmp2));
  }
#endif

  COMPILER_COMMENT(("invoke native method"));
  call_vm(entry, return_kind JVM_CHECK);

  COMPILER_COMMENT(("Check if native method needs redoing"));
  get_thread(tmp1);
  mov(tmp3, zero);
  ldr_imm_index(tmp2, tmp1, Thread::async_redo_offset());
  str(tmp3, imm_index(tmp1, Thread::async_redo_offset()));
  cmp(tmp2, zero);
  b(redo, ne);

  COMPILER_COMMENT(("Clear Thread::async_info"));
  str(tmp3, imm_index(tmp1, Thread::async_info_offset()));

#if ENABLE_PROFILER
  if (UseProfiler) {
    COMPILER_COMMENT(("Inform Profiler we're out of native method"));

    mov_imm(tmp1, 0);
    mov_imm(tmp2, (int)&_jvm_profiler_in_native_method);
    str(tmp1, imm_index(tmp2));
  }
#endif

  adjust_for_invoke(0, return_kind, true);
}
#if ENABLE_TRAMPOLINE && !CROSS_GENERATOR
void CodeGenerator::call_from_compiled_code(Method* callee, Register dst, int offset,
                                            int parameters_size,
                                            bool indirect, bool speed
                                            JVM_TRAPS) {
  GUARANTEE(dst != lr,   "Register lr will be destroyed");
  if (indirect && speed) {
    ldr_imm_index(r12, dst, offset);
    dst = r12; offset = 0;
    indirect = false;
  }
  int code_offset = code_size(); // current pc
  add(lr, pc, imm_rotate(0,0)); // immediate filled in at bottom
  // This is never used to call C code directly.
  // We don't need to worry about THUMB
      if (indirect) {
        ldr_imm_index(pc, dst, offset);
      } else if (offset == 0) {
        // This instruction is faster than the "add" on the StrongArm
        mov(pc, reg(dst));
      } else {
        add(pc, dst, imm(offset));
      }

  write_literals();
  write_call_info(parameters_size JVM_CHECK);

  if (callee && 
    CompiledMethodCache::has_index((CompiledMethodDesc*)callee->execution_entry())
    && ( callee->is_static() || callee->is_final() || callee->is_private()) )
  {
     CompiledMethod *cm = compiled_method();
      address target =  callee->execution_entry();

      BranchTable::append(cm->entry()+ code_offset + 4,(address) cm->obj(), 
                                    (address) (target - CompiledMethodDesc::entry_offset()), 
                                    *(int*) (cm->entry() + code_offset + 4));
    
  }
  // Patch the "add" instruction to make lr point at following instruction
  if (!has_overflown_compiled_method()) {
    *(int *)addr_at(code_offset) |= imm(code_size() - code_offset - 8);
  }
}

#endif
void CodeGenerator::call_from_compiled_code(Register dst, int offset,
                                            int parameters_size,
                                            bool indirect, bool speed
                                            JVM_TRAPS) {
  GUARANTEE(dst != lr,   "Register lr will be destroyed");
  if (indirect && speed) {
    ldr_imm_index(r12, dst, offset);
    dst = r12; offset = 0;
    indirect = false;
  }
  int code_offset = code_size(); // current pc
  add(lr, pc, imm_rotate(0,0)); // immediate filled in at bottom
  // This is never used to call C code directly.
  // We don't need to worry about THUMB
  if (indirect) {
    ldr_imm_index(pc, dst, offset);
  } else if (offset == 0) {
    // This instruction is faster than the "add" on the StrongArm
    mov(pc, reg(dst));
  } else {
    add(pc, dst, imm(offset));
  }
  write_literals();
  write_call_info(parameters_size JVM_CHECK);
  // Patch the "add" instruction to make lr point at following instruction
  if (!has_overflown_compiled_method()) {
    *(int *)addr_at(code_offset) |= imm(code_size() - code_offset - 8);
  }
}

void CodeGenerator::write_call_info(int parameters_size JVM_TRAPS) {
#if ENABLE_EMBEDDED_CALLINFO
  if (CallInfo::fits_compiled_compact_format(bci(),
                                          code_size(),
                                          frame()->virtual_stack_pointer() + 1)
        && frame()->fits_compiled_compact_format()) {
    CallInfo info = CallInfo::compiled_compact(bci(), code_size());
    frame()->fill_in_tags(info, parameters_size);
    emit_ci(info);
  } else {
    {
      TypeArray::Raw extended_tag_info =
        frame()->generate_callinfo_stackmap(JVM_SINGLE_ARG_CHECK);
      for (int i = extended_tag_info().length() - 1; i >= 0; i--) {
        emit_int(extended_tag_info().int_at(i));
      }
    }
    CallInfo info = CallInfo::compiled(bci(), code_size() JVM_CHECK);
    emit_ci(info);
  }
#endif // ENABLE_EMBEDDED_CALLINFO

#if ENABLE_APPENDED_CALLINFO
  append_callinfo_record(code_size() JVM_NO_CHECK_AT_BOTTOM);
#endif
  (void)parameters_size;
}

bool CodeGenerator::quick_catch_exception(const Value &exception_obj,
                                          JavaClass* catch_type, 
                                          int handler_bci JVM_TRAPS) {
  TempRegister tmp1;
  Label quick_case;

  int class_id = catch_type->class_id();
  ldr(tmp1, imm_index(exception_obj.lo_register()));          // near object
  ldr(tmp1, imm_index(tmp1, JavaNear::class_info_offset()));  // class_info
  ldrh(tmp1, imm_index3(tmp1, ClassInfo::class_id_offset())); // class_id
  CompilerLiteralAccessor cla;
  cmp_imm(tmp1, class_id, &cla);
  b(quick_case, eq);

  QuickCatchStub::Raw stub =
      QuickCatchStub::allocate(bci(), exception_obj, handler_bci,
                               quick_case JVM_CHECK_0);
  stub().insert();

  // If we go to the stub, we can't be guaranteed it has preserved literals
  frame()->clear_literals();

  return true; // successful!
}

void CodeGenerator::call_vm_extra_arg(const Register extra_arg) {
  mov_reg(r1, extra_arg);
}

void CodeGenerator::call_vm_extra_arg(const int extra_arg) {
  mov_imm(r1, extra_arg);
}

void CodeGenerator::call_vm(address entry, BasicType return_value_type 
                            JVM_TRAPS) {
  // all registers must be flushed (not necessarily unmapped) before calling
  // call_vm
  write_literals_if_desperate();
  if (entry == (address)timer_tick) {
    call_through_gp(gp_compiler_timer_tick_ptr, false JVM_NO_CHECK_AT_BOTTOM);
  } else {
    if (return_value_type != T_ILLEGAL) {
      mov_imm(r3, entry);
    }
    COMPILER_COMMENT(("call vm"));
    if (stack_type_for(return_value_type) == T_OBJECT) {
#if ENABLE_ISOLATES
      if (GenerateCompilerAssertions) {
        if (StopAtCIBHit && entry == (address)compiled_code_task_barrier) {
          breakpoint();
        }
      }
#endif
      call_through_gp(gp_shared_call_vm_oop_ptr, false JVM_NO_CHECK_AT_BOTTOM);
    } else if (return_value_type == T_ILLEGAL) {
      call_through_gp(gp_shared_call_vm_exception_ptr, false
                      JVM_NO_CHECK_AT_BOTTOM);
    } else {
      call_through_gp(gp_shared_call_vm_ptr, false JVM_NO_CHECK_AT_BOTTOM);
    }
  }
}

void
CodeGenerator::type_check(Value& array, Value& index, Value& object JVM_TRAPS)
{
  Label slow_case, done_checking;

  COMPILER_COMMENT(("Array store type check"));

  frame()->push(array);
  frame()->push(index);
  frame()->push(object);

  // Since register allocation might cause spilling we have to allocate *all*
  // registers before checking for for null object.

  TempRegister tmp1;
  TempRegister tmp2;
  TempRegister tmp3;

  ldr_imm_index(tmp2, array.lo_register());
  // Check for null object.
  cmp(object.lo_register(), zero);
  ldr(tmp1, imm_index(object.lo_register()), ne);
  b(done_checking, eq);

  // Get the class and the element class of the array
  ldr_imm_index(tmp2, tmp2);
  ldr_imm_index(tmp1, tmp1);
  ldr_imm_index(tmp2, tmp2, ObjArrayClass::element_class_offset());

  // Fast check against the subtype check caches.
  ldr_imm_index(tmp3, tmp1, JavaClass::subtype_cache_1_offset());
  ldr_imm_index(tmp1, tmp1, JavaClass::subtype_cache_2_offset());
  cmp(tmp3, reg(tmp2));
  cmp(tmp1, reg(tmp2), ne);
  if (need_to_force_literals()) {
    b(done_checking, eq);
    b(slow_case, ne);
    write_literals();
  } else {
    b(slow_case, ne);
  }

  // Cache hit.
  bind(done_checking);

  TypeCheckStub::Raw stub =
      TypeCheckStub::allocate(bci(), slow_case, done_checking 
                              JVM_NO_CHECK);
  if (stub.not_null()) {
    stub().insert();
    frame()->pop(object);
    frame()->pop(index);
    frame()->pop(array);

    // If we go to the stub, we can't be guaranteed it has preserved literals
    frame()->clear_literals();
  }
}

CodeGenerator::Condition
CodeGenerator::convert_condition(BytecodeClosure::cond_op cond)  {
  GUARANTEE(0 == (int) BytecodeClosure::null && 
            1 == (int) BytecodeClosure::nonnull && 
            2 == (int) BytecodeClosure::eq && 
            3 == (int) BytecodeClosure::ne && 
            4 == (int) BytecodeClosure::lt && 
            5 == (int) BytecodeClosure::ge && 
            6 == (int) BytecodeClosure::gt && 
            7 == (int) BytecodeClosure::le,    "sanity");

  static const jubyte table[] = {
    /* case BytecodeClosure::null   */ eq,
    /* case BytecodeClosure::nonnull*/ ne,
    /* case BytecodeClosure::eq     */ eq,
    /* case BytecodeClosure::ne     */ ne,
    /* case BytecodeClosure::lt     */ lt,
    /* case BytecodeClosure::ge     */ ge,
    /* case BytecodeClosure::gt     */ gt,
    /* case BytecodeClosure::le     */ le,
  };

  GUARANTEE(int(BytecodeClosure::null) <= int(cond) &&
            int(cond) <= int(BytecodeClosure::le),
            "sanity");
  return (CodeGenerator::Condition)(table[cond]);
}

#if ENABLE_LOOP_OPTIMIZATION && ARM
void CodeGenerator::conditional_jmp(Assembler::Condition cond,
                                     int destination JVM_TRAPS) {
  Label branch_taken;
  b(branch_taken, cond);
  COMPILER_COMMENT(("Creating continuation for target bci = %d",destination));
  CompilationContinuation::insert(destination,
                                  branch_taken JVM_NO_CHECK);

  write_literals_if_desperate();
}
#endif//#if ENABLE_LOOP_OPTIMIZATION && ARM

void CodeGenerator::conditional_jump(BytecodeClosure::cond_op condition,
                                     int destination,
                                     bool assume_backward_jumps_are_taken
                                     JVM_TRAPS)
{
  if (assume_backward_jumps_are_taken && destination < bci()) {
    Label fall_through;
    b(fall_through, not_cond(convert_condition(condition)));
    int next = bci() + Bytecodes::length_for(method(), bci());
    CompilationContinuation::insert(next, fall_through JVM_CHECK);
    COMPILER_COMMENT(("Creating continuation for fallthrough to bci = %d",
                      bci() + Bytecodes::length_for(method(), bci())));
    branch(destination JVM_NO_CHECK_AT_BOTTOM);
  } else {
    Label branch_taken;
    b(branch_taken, convert_condition(condition));
    COMPILER_COMMENT(("Creating continuation for target bci = %d", destination));
    CompilationContinuation::insert(destination,
                                    branch_taken JVM_NO_CHECK_AT_BOTTOM);
  }
  write_literals_if_desperate();
}

// This function is a little bit more complicated than it needs to be at
// present, but it may do more work in the future.
//
// It takes the argCount *Value's passed and moves them into the registers.
// It can handle both immediates and registers.

void CodeGenerator::setup_c_args(int ignore, ...) {
  // Unfortunately, we need an ignored first argument.
  // Ansi C doesn't allow setup_c_args(...)
  va_list ap;
  va_start(ap, ignore);
     vsetup_c_args(ap);
  va_end(ap);
}

void CodeGenerator::vsetup_c_args(va_list ap) {
  int i, targetRegister, regCount, immCount;
  Register srcReg[6], dstReg[6];
  Register dstImm[6];
  jint     srcImm[6];

  regCount = immCount = 0;
  targetRegister = 0;
  for(;;) {
    Value* value = va_arg(ap, Value*);
    if (value == NULL) break;
    if (value->type() == T_ILLEGAL) {
      // just a place holder.  We'll fill in the register late
    } else if (value->in_register()) {
      // Make a list of the values that need to be copied from
      // one register into another
      dstReg[regCount] = as_register(targetRegister);
      srcReg[regCount] = value->lo_register();
      regCount++;
      if (value->is_two_word()) {
        dstReg[regCount] = as_register(targetRegister + 1);
        srcReg[regCount] = value->hi_register();
        regCount++;
      }
    } else {
      dstImm[immCount] = as_register(targetRegister);
      srcImm[immCount] = value->lo_bits();
      immCount++;
      if (value->is_two_word()) {
        dstImm[immCount] = as_register(targetRegister + 1);
        srcImm[immCount] = value->hi_bits();
        immCount++;
      }
    }
    targetRegister += value->is_two_word() ? 2 : 1;
  }

  // Copy the info from srcReg to dstReg
  if (regCount > 0) {
    if (regCount == 1) {
      // This happens often enough to optimize and avoid all the work of
      // shuffling the registers
      mov_reg(dstReg[0], srcReg[0]);
    } else {
      shuffle_registers(dstReg, srcReg, regCount);
    }
  }

  // Write the immediate values.
  CompilerLiteralAccessor cla;
  for (i = 0; i < immCount; i++) {
    mov_imm(dstImm[i], srcImm[i], &cla);
  }
}

void CodeGenerator::shuffle_registers(Register* dstReg, Register* srcReg,
                                      int regCount) {
  int i, j;
#ifdef AZZERT
  bool using_scratch = false;
#endif

  // Allocate a scratch register that isn't one of our sources and isn't one
  // of our targets.  The sources are already in Values, so we don't need to
  // "reference" them.
  for (i = 0; i < regCount; i++) {
    RegisterAllocator::reference(dstReg[i]);
  }
  TempRegister scratch;
  for (i = 0; i < regCount; i++) {
    RegisterAllocator::dereference(dstReg[i]);
  }

  // We need to copy srcReg[0..regCount-1] to dstReg[0..regCount-1];
  //
  // There may be duplications about the srcReg's, but the dstReg's are
  // each unique.
  while (regCount > 0) {
    if (dstReg[0] == srcReg[0]) {
      regCount--; srcReg++; dstReg++;
      continue;
    }
    // Find a dstReg[i] which isn't also a srcReg.
    for (i = 0; i < regCount; i++) {
      for (j = 0;  j < regCount; j++) {
        if (dstReg[i] == srcReg[j]) {
          goto continue_outer_for_loop;
        }
      }
      break;
continue_outer_for_loop:
      ;
    }

    if (i < regCount) {
      // Nothing uses dstReg[i] as a source.  It is safe to write to it.
#ifdef AZZERT
      if (srcReg[i] == scratch) {
        GUARANTEE(using_scratch, "Where did it come from?");
        using_scratch = false;
      }
#endif
      mov(dstReg[i], reg(srcReg[i]));
      // This helps remove permutations.  Change anything that had used
      // srcReg[i] as a source to instead use dstReg[i].
      for (int j = 0; j < regCount; j++) {
        if (i != j && srcReg[j] == srcReg[i]) {
          srcReg[j] = dstReg[i];
        }
      }
      // And decrement. . . .
      regCount--;
      dstReg[i] = dstReg[0]; dstReg++;
      srcReg[i] = srcReg[0]; srcReg++;
    } else {
      // srcReg[] and dstReg[] are permutations of each other.  We need
      // to use the scratch register to break the permutation.
#ifdef AZZERT
      GUARANTEE(!using_scratch, "Can't have a permutation with scratch");
      using_scratch = true;
#endif
      mov(scratch, reg(srcReg[0]));
      srcReg[0] = scratch.reg(); // Workaround for: srcReg[0] = scratch;
    }
  }
}

void CodeGenerator::load_from_object(Value& result, Value& object, jint offset,
                                    bool null_check JVM_TRAPS) {
  Assembler::Condition cond = al;
#if ENABLE_NPCE
  bool is_npe;
#endif
  if (null_check) {
#if ENABLE_NPCE
    cond = maybe_null_check_1_signal(object, is_npe);  
#else
    cond = maybe_null_check_1(object);
#endif
  }
  Value object_copy;
  object.copy(object_copy);
  FieldAddress address(object_copy, offset, result.type());
  load_from_address(result, result.type(), address, cond);
  if (null_check) {
#if ENABLE_NPCE
    maybe_null_check_3_signal(cond, is_npe, result.type() 
                              JVM_NO_CHECK_AT_BOTTOM);
#else
    maybe_null_check_2(cond JVM_NO_CHECK_AT_BOTTOM);
#endif
  }
}

                                    
void CodeGenerator::init_static_array(Value& result JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  // Flush the virtual stack frame.
  flush_frame();  

  const Register src = tos_tag;
  const Register dst = tos_val;
  const Register size = tmp0;
  RegisterAllocator::reference(src);
  RegisterAllocator::reference(dst);
  RegisterAllocator::reference(size);

  Method::Raw cur_method = 
    Compiler::current()->current_compiled_method()->method();
  ldr_oop(src, &cur_method);

  mov_reg(dst, result.lo_register());
  if (GenerateCompilerAssertions) {
    cmp(dst, zero);
    breakpoint(eq);
  }

  add_imm(src, src, Method::base_offset() + bci() + 1);

  // Load type size shift.
  ldrb(tmp5, imm_index(src, 1, post_indexed)); 
  // Load elements count low byte.
  ldrb(size, imm_index(src, 1, post_indexed)); 
  // Load elements count hi byte.
  ldrb(tmp2, imm_index(src, 1, post_indexed)); 

  ldr(tmp3, imm_index(dst, Array::length_offset()));  //load array size
  orr(size, size, imm_shift(tmp2, lsl, BitsPerByte));
    
  mov(size, reg_shift(size, lsl, tmp5));

  add_imm(dst, dst, Array::base_offset());  

  Value src_val(T_INT);
  src_val.set_register(src);

  Value dst_val(T_INT);
  dst_val.set_register(dst);

  Value size_val(T_INT);
  size_val.set_register(size);
  
  call_simple_c_runtime(dst_val, (address)jvm_memcpy, 
    dst_val, src_val, size_val);
}

void CodeGenerator::assign_register(Value& result, Value& op) {
  GUARANTEE(!result.is_present(), "result must not be present");
  if (!op.in_register() ||
        frame()->is_mapping_something(op.lo_register()) ||
       (op.is_two_word() && frame()->is_mapping_something(op.hi_register()))) {
     result.assign_register();
  } else {
     op.copy(result);
  }
}

#ifdef AZZERT
void
CodeGenerator::verify_location_is_constant(jint index, const Value& constant) {
  if (GenerateCompilerAssertions) {
    CompilerLiteralAccessor cla;
    LocationAddress address(index, constant.type());
    TempRegister tmp;
    ldr(tmp, address.lo_address_2());
    cmp_imm(tmp, constant.lo_bits(), &cla);
    if (constant.is_two_word()) {
      ldr(tmp, address.hi_address_2(),      eq);
      cmp_imm(tmp, constant.hi_bits(), &cla, eq);
    }
    breakpoint(ne);
  }
}
#endif

#if CROSS_GENERATOR && !ENABLE_ISOLATES

void CodeGenerator::initialize_class(InstanceClass* klass JVM_TRAPS) {
  GUARANTEE(klass->not_null() && !klass->is_initialized(), 
            "Should only be called for non-initialized classes");
  // initialize_class(Thread&, raw_class);
  COMPILER_COMMENT(("Initialize class if needed"));
  COMPILER_COMMENT(("Flush frame"));
  flush_frame();

  Label class_initialized;

  COMPILER_COMMENT(("Load class"));
  ldr_oop(r1, klass);

  COMPILER_COMMENT(("Quick check if the class is initialized"));
  ldr(r0, imm_index(r1, JavaClass::java_mirror_offset()));
  ldr(r0, imm_index(r0, JavaClassObj::status_offset()));
  tst(r0, imm(JavaClassObj::INITIALIZED));
  b(class_initialized, ne);

  COMPILER_COMMENT(("Class is not initialized - initialize it"));
  call_vm((address) ::initialize_class, T_VOID JVM_NO_CHECK_AT_BOTTOM);

  bind(class_initialized);
}

#endif

#endif

#endif /*#if !ENABLE_THUMB_COMPILER*/
