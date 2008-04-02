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

#if !ENABLE_THUMB_COMPILER
#if ENABLE_COMPILER
#include "incls/_CodeGenerator_arm.cpp.incl"

bool CodeGenerator::_interleave_frame_linking;

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

#if ENABLE_ARM_VFP
class TempVFPRegister {
private:
    Assembler::Register _reg;
public:
    TempVFPRegister() {
      _reg = RegisterAllocator::allocate_float_register();
    }
    ~TempVFPRegister() { RegisterAllocator::dereference(_reg); }

    operator Assembler::Register() { return _reg; }

    // Simple accessor as a workaround for above UDC
    Assembler::Register reg() { return _reg; }
};
#endif

class CompilerLiteralAccessor : public LiteralAccessor {
public:
  virtual bool has_literal(int imm32, Assembler::Address1& result);

  virtual Assembler::Register get_literal(int imm32) {
    return frame()->get_literal(imm32, *this);
  }

#if ENABLE_ARM_VFP
  Assembler::Register has_vfp_literal(const int imm32);
#endif

private:
  VirtualStackFrame* frame() {
    return Compiler::frame();
  }
};

// target_ptr must be one of gp_xxx_ptr. They are declared in
// Assembler_armthumb.hpp and defined in Interpreter_arm.s
void CodeGenerator::call_through_gp(address* target_ptr, bool speed JVM_TRAPS)
{
  const int parameters_size = 0;
  const bool indirect = true;
  address target = *target_ptr;

#if USE_COMPILER_GLUE_CODE
  if (!GenerateROMImage && !need_to_force_literals() &&
      address(target) >= address(&compiler_glue_code_start) &&
      address(target) <  address(&compiler_glue_code_end)) {
    bl(target);
    write_call_info(parameters_size JVM_NO_CHECK_AT_BOTTOM);
    return;
  }
#endif

  int offset = int(target_ptr) - int(&gp_base_label);
  call_from_compiled_code(gp, offset, parameters_size, indirect, speed
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
    flush_frame(JVM_SINGLE_ARG_CHECK);
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
  // IMPL_NOTE Cannot make the flush conditionally.
  //  see how this can be made conditional!
  flush_frame(JVM_SINGLE_ARG_CHECK);

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
      if (ENABLE_ARM_VFP && les.reg() > Assembler::r15) {
        continue;
      }
      result = Assembler::reg(les.reg());
      return true;
    }
  }

  // IMPL_NOTE: need revisit?
  // This for loop should probably be deleted.  It's just not worth it.
  for(les.reset(); !les.eos(); les.next()) {
    int value = les.value();
    if (value == 0) {
      continue;
    }
    Assembler::Register rd = les.reg();
    if (ENABLE_ARM_VFP && rd > Assembler::r15) {
      continue;
    }
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

#if ENABLE_ARM_VFP
Assembler::Register CompilerLiteralAccessor::has_vfp_literal(int imm32) {
  LiteralElementStream les(frame());
  for ( ; !les.eos() ; les.next()) {
    if (les.value() == imm32) {
      return les.reg();
    }
  }
  return Assembler::no_reg;
}
#endif

void CodeGenerator::load_from_address(Value& result, BasicType type,
                                      MemoryAddress& address,
                                      Assembler::Condition cond) {
  write_literals_if_desperate();

   // illegal types do not require any loading
  if (type == T_ILLEGAL) {
    return;
  }

  GUARANTEE(stack_type_for(type) == result.stack_type(),
            "types must match (taking stack types into account)");
  result.try_to_assign_register();
  const Register lo = result.lo_register();

#if ENABLE_ARM_VFP
  if (type == T_FLOAT && lo >= s0) {
    flds(lo, address.lo_address_5(), cond);
    return;
  } else if (type == T_DOUBLE && lo >= s0) {
    fldd(lo, address.lo_address_5(), cond);
    return;
  }
#endif

  switch (type) {
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
      // T_BYTE doesn't get compiled very often, so let's put it
      // here. Gcc generates better code to make it faster to compile
      // the common case (loading 4-bytes).
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
          // when the iand or i2s bytecode sits at a branch target, so we check
          // it with the Compiler::entry_count_for() lines below.
          if (Compiler::current()->entry_count_for(nextbci) == 1 &&
              Compiler::current()->entry_count_for(nextbci+1) == 1) {
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
                   Compiler::current()->entry_count_for(nextbci) == 1 &&
                   nextbci + 3 < len &&
                   method()->bytecode_at(nextbci+3) == Bytecodes::_iand &&
                   Compiler::current()->entry_count_for(nextbci+3) == 1) {
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
#if ENABLE_ISOLATES                                       
  enum {max_codesize = 48};
#else  
  enum {max_codesize = 32};
#endif
  write_literals_if_desperate(max_codesize);

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
#if ENABLE_ARM_VFP
        if(reg >= s0) {
          fsts(reg, address.lo_address_5());
          break;
        } // Intentionally no break here
#endif
    case T_INT     :
        str(reg, address.lo_address_2());
        break;

    case T_DOUBLE  :
#if ENABLE_ARM_VFP
        if (!value.is_immediate() && reg >= s0) {
          fstd(reg, address.lo_address_5());
          break;
        } // Intentionally no break here
#endif
    case T_LONG    :
#if ENABLE_ARM_VFP
        if(reg >= s0) {
          fsts(reg, address.lo_address_5());
        } else
#endif
        {
          str(reg, address.lo_address_2());
        }

        if (value.is_immediate()) {
          // Unreference the old literal.  Get the new literal and reference it
          RegisterAllocator::dereference(reg);
          reg = cla.get_literal(value.hi_bits());
          RegisterAllocator::reference(reg);
        } else {
          reg = value.hi_register();
        }

#if ENABLE_ARM_VFP
        if(reg >= s0) {
          fsts(reg, address.hi_address_5());
        } else
#endif
        {
          str(reg, address.hi_address_2());
        }
        break;

    default        :
        SHOULD_NOT_REACH_HERE();
        break;
  }
  if (value.is_immediate()) {
    RegisterAllocator::dereference(reg);
  }
}

#if ENABLE_NPCE
void CodeGenerator::store_to_address_and_record_offset_of_exception_instr(Value& value, BasicType type,
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
  NullCheckStub* unlinked_npe_stub =
    Compiler::current()->get_unlinked_exception_stub(bci());

  if (unlinked_npe_stub) {
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
            record_npe_point(unlinked_npe_stub, -1) ;
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
#if ENABLE_ARM_VFP
  if(reg >= s0) {
    fsts(reg, address.lo_address_5());
    break;
  } // Intentionally no break here
#endif
    case T_INT     :
        str(reg, address.lo_address_2());
        break;

    case T_DOUBLE  : // fall through
    case T_LONG    :
        str(reg, address.lo_address_2());
        if (need_npe_check) {
          record_npe_point(unlinked_npe_stub, -1);
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
          unlinked_npe_stub->set_is_two_instr((code_size()-old_code_size)>>2);
          //record the LDR instr into the table used for extend basic block scheduling
          record_npe_point(NULL, -1) ;
        }
        break;

    default        :
        SHOULD_NOT_REACH_HERE();
        break;
  }
  if (need_npe_check && (type !=T_DOUBLE && type != T_LONG )) {
          record_npe_point(unlinked_npe_stub,-1) ;
  }
  if (value.is_immediate()) {
    RegisterAllocator::dereference(reg);
  }
}
#endif //NPCE

#if ENABLE_ARM_VFP
void CodeGenerator::move_vfp_immediate(const Register dst,
                                       const jint src, const Condition cond) {
  CompilerLiteralAccessor cla;
  const Assembler::Register reg = cla.has_vfp_literal(src);

  if (reg != Assembler::no_reg) {
    if (reg > Assembler::r15) {
      fcpys(dst, reg, cond);
    } else {
      fmsr(dst, reg, cond);
    }
  } else {
    if (is_rotated_imm(src)) {
      TempRegister tmp;
      mov_imm(tmp, src, &cla, cond);
      fmsr(dst, tmp, cond);
    } else {
      fld_literal(dst, src, cond);
    }
  }
}

inline void
CodeGenerator::move_float_immediate(const Register dst,
                                    const jint src, const Condition cond) {
  if (src == 0) {
    Register reg = frame()->find_zero();
    if (reg == Assembler::no_reg) {
      reg = frame()->find_non_NaN();
      if (is_vfp_register(reg)) {
        fsubs(dst, reg, reg, cond);
      } else {
        if (reg == Assembler::no_reg) {
          TempRegister tmp;
          mov_imm(tmp, 0, cond);
          fmsr(dst, tmp, cond);
          frame()->set_has_literal_value(tmp, 0);
        } else {
          fmsr(dst, reg, cond);
          fsubs(dst, dst, dst, cond);
        }
      }
    } else if (is_vfp_register(reg)) {
      fcpys(dst, reg, cond);
    } else {
      fmsr(dst, reg, cond);
    }
  } else {
    move_vfp_immediate(dst, src, cond);
  }
}

inline void
CodeGenerator::move_double_immediate(const Register dst,
                                     const jint src_lo, const jint src_hi,
                                     const Condition cond) {
  const Register dst_lo = dst;
  const Register dst_hi = Register(dst + 1);

  if (src_lo == 0 && src_hi == 0) {
    Register zero_reg = frame()->find_zero();
    if (zero_reg != Assembler::no_reg) {
      if (is_vfp_register(zero_reg)) {
        fcvtds(dst_lo, zero_reg, cond);
      } else {
        fmdrr(dst_lo, zero_reg, zero_reg, cond);
      }
      return;
    }
    Register double_reg = frame()->find_double_non_NaN();
    if (double_reg != Assembler::no_reg) {
      fsubd(dst_lo, double_reg, double_reg, cond);
      return;
    }
    Register non_nan_reg = frame()->find_non_NaN();
    if (non_nan_reg != Assembler::no_reg) {
      if (is_vfp_register(non_nan_reg)) {
        fsubs(dst_lo, non_nan_reg, non_nan_reg, cond);
        fcpys(dst_hi, dst_lo);
      } else {
        fmdrr(dst_lo, non_nan_reg, non_nan_reg, cond);
        fsubd(dst_lo, dst_lo, dst_lo, cond);
      }
      return;
    }
    TempRegister tmp;
    mov_imm(tmp, 0, cond);
    fmdrr(dst_lo, tmp, tmp, cond);
    frame()->set_has_literal_value(tmp, 0);
    return;
  }
  {
    const Register reg = frame()->find_double_vfp_literal(src_lo, src_hi);
    if (reg != Assembler::no_reg) {
      fcpyd(dst_lo, reg, cond);
      return;
    }
  }
  {
    CompilerLiteralAccessor cla;
    const Register reg_lo = cla.has_vfp_literal(src_lo);
    const Register reg_hi = cla.has_vfp_literal(src_hi);
    if (is_arm_register(reg_lo) && is_arm_register(reg_hi)) {
      fmdrr(dst_lo, reg_lo, reg_hi, cond);
      return;
    }
    if (is_vfp_register(reg_hi)) {
      fcpys(dst_hi, reg_hi, cond);
      if (src_lo == 0 && VirtualStackFrame::is_non_NaN(src_hi)) {
        fsubs(dst_lo, dst_hi, dst_hi, cond);
        return;
      }
      if (is_arm_register(reg_lo)) {
        fmsr(dst_lo, reg_lo, cond);
        return;
      }
      move_vfp_immediate(dst_lo, src_lo, cond);
      return;
    }

    if (is_vfp_register(reg_lo)) {
      fcpys(dst_lo, reg_lo, cond);
      if (src_hi == 0 && VirtualStackFrame::is_non_NaN(src_lo)) {
        fsubs(dst_hi, dst_lo, dst_lo, cond);
        return;
      }
      if (is_arm_register(reg_hi)) {
        fmsr(dst_hi, reg_hi, cond);
        return;
      }
      move_vfp_immediate(dst_hi, src_hi, cond);
      return;
    }    
    move_vfp_immediate(dst_lo, src_lo, cond);
    if (src_lo == src_hi) {
      fcpys(dst_hi, dst_lo, cond);
    } else {
      move_vfp_immediate(dst_hi, src_hi, cond);
    }
  }
}
#endif // ENABLE_ARM_VFP

void
CodeGenerator::move(const Value& dst, const Value& src, const Condition cond) {
  // if the source isn't present there's nothing left to do
  if (!src.is_present()) {
    return;
  }

  GUARANTEE(dst.type() == src.type(), "type check");
  GUARANTEE(dst.in_register(), "destination must be in register");
  if (src.is_immediate()) {
    /* move always 1 word */
    Register lo = dst.lo_register();
#if ENABLE_ARM_VFP
    if (dst.type() == T_FLOAT) {
      move_float_immediate(lo, src.lo_bits(), cond);
    } else if (dst.type() == T_DOUBLE) {
      move_double_immediate(lo, src.lo_bits(), src.hi_bits(), cond);
    }
    else
#endif  // ENABLE_ARM_VFP
    {
      CompilerLiteralAccessor cla;
#if ENABLE_ARM_VFP
      const Assembler::Register reg = cla.has_vfp_literal(src.lo_bits());
      if( reg != Assembler::no_reg && reg > Assembler::r15 ) {
        fmrs(lo, reg, cond);
      } else
#endif
      {
        mov_imm(lo, src.lo_bits(), &cla, cond);
      }
      if (dst.is_two_word()) {
#if ENABLE_ARM_VFP
        const Assembler::Register reg = cla.has_vfp_literal(src.hi_bits());
        if( reg != Assembler::no_reg && reg > Assembler::r15 ) {
          fmrs(dst.hi_register(), reg, cond);
        } else
#endif
        {
          mov_imm(dst.hi_register(), src.hi_bits(), &cla, cond);
        }
      }
    }
    if (cond == al) {
      frame()->set_has_literal_value(dst.lo_register(), src.lo_bits());
    }
    if (dst.is_two_word()) {
      if (cond == al) {
        frame()->set_has_literal_value(dst.hi_register(), src.hi_bits());
      }
    }
  } else {
    GUARANTEE(src.in_register(), "source must be in register");
    /* move always 1 word */
    move(dst.lo_register(), src.lo_register(), cond);
    if (dst.is_two_word()) {
      move(dst.hi_register(), src.hi_register(), cond);
    }
  }
}

void CodeGenerator::move(Value& dst, Oop* obj, Condition cond) {
  GUARANTEE(dst.type() == T_OBJECT || dst.type() == T_ARRAY, "type check");
  ldr_oop(dst.lo_register(), obj, cond);
}

void CodeGenerator::move(Assembler::Register dst, Assembler::Register src,
                         Condition cond) {
#if ENABLE_ARM_VFP
  // Does not support Doubles reg moves
  if (dst >= s0) {
    if (src >= s0) {
      // FP <- FP
      fcpys(dst, src, cond);
    } else {
      // FP <- INT
      fmsr(dst, src, cond);
    }
  } else if (src >= s0) {
    // INT <- SRC
    fmrs(dst, src, cond);
  }
  else
#endif
  {
    mov_reg(dst, src, no_CC, cond);
  }
}

#if ENABLE_REMEMBER_ARRAY_LENGTH
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

  const bool null_check = need_null_check(array);

#if ENABLE_REMEMBER_ARRAY_LENGTH
  //skip the array length load operation if the length is cached
  //in register (we called it bound mask registers)
  Register length;
  bool first_time = !(array.is_not_first_time_access());
  if (null_check) {
    NullCheckStub* null_check_stub =
      NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(null_check_stub));
#if ENABLE_NPCE
    //this will be invoked for each byte code which
    //may throw null point exception
    record_npe_point(null_check_stub);
    frame()->set_value_must_be_nonnull(array);
#else
    cmp(array.lo_register(), zero);
    frame()->set_value_must_be_nonnull(array);
    b(null_check_stub, eq);
#endif
  }

  length = frame()->cached_array_length(array.lo_register(),
                                    first_time, Assembler::al);

#else // !ENABLE_REMEMBER_ARRAY_LENGTH
  TempRegister length; //temp register used here
  if (null_check) {
#if ENABLE_NPCE
    NullCheckStub* null_check_stub =
      NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(null_check_stub));
    record_npe_point(null_check_stub);
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
      NullCheckStub* null_check_stub =
        NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(null_check_stub));
      b(null_check_stub, eq);
    }
#endif
  } else {
    ldr_imm_index(length, array.lo_register(), Array::length_offset());
  }
#endif

  //remember array length checking
  if( frame()->try_to_set_must_be_index_checked(length , index) ) {
    return;
  }

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
    IndexCheckStub* index_check_stub =
      IndexCheckStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(index_check_stub));
    b(index_check_stub, ls);
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

  const int locals = Compiler::root()->method()->max_locals();

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
    NullCheckStub* check_stub =
      NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(check_stub));
    b(check_stub, eq);
  }
}


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
      NullCheckStub* error =
        NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(error));
      b(error, eq);
    }
  }
}

#if ENABLE_NPCE
void CodeGenerator::null_check_by_npce(Value& object, bool need_tigger_instr,
                      bool is_quick_return, BasicType type_of_data JVM_TRAPS)
{
  NullCheckStub* check_stub =
    NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(check_stub));
  if(!check_stub->is_persistent() &&
        (type_of_data == T_LONG || type_of_data == T_DOUBLE)) {
    //the offset of second LDR will be updated in
    //store_to_address()
    check_stub->set_is_two_instr();
  }

  if (is_quick_return) {
    return;
  }
      
  record_npe_point(check_stub);
      
  if (need_tigger_instr) {
    TempRegister dummy;
    COMPILER_COMMENT((" generate a faked ldr instruction =>\n"));
    ldr_imm_index(dummy, object.lo_register(), 0);
  }
}

void CodeGenerator::maybe_null_check_2_by_npce(Value& object,
                                             BasicType type
                                              JVM_TRAPS) {
  bool need_npe = false;
  if (object.must_be_null()) {
    need_npe = true;
  } else if (need_null_check(object)) {
    need_npe = true;
    frame()->set_value_must_be_nonnull(object);
  }
  
  if ( need_npe ) {
    NullCheckStub* error =
        NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(error));
    if (type == T_LONG || type == T_DOUBLE) {
       error->set_is_two_instr(1);
       
      record_npe_point(error, -2);
      record_npe_point(NULL,  -1);
    } else {
      record_npe_point(error, -1);
    } 
  }
}
#endif //ENABLE_NPCE

void CodeGenerator::overflow(const Assembler::Register& stack_pointer,
                             const Assembler::Register& method_pointer) {
  if (USE_OVERFLOW_STUB) {
    if (_interleave_frame_linking) {
      COMPILER_COMMENT(("revert jsp change during method entry"));
      int jsp_shift = get_jsp_shift(method());
      add_imm(jsp, jsp, -JavaStackDirection * jsp_shift);
    }

    if (method_pointer != Assembler::r0) {
      mov(r0, reg(method_pointer));
    }
    if (stack_pointer != Assembler::r1) {
      mov(r1, reg(stack_pointer));
    }
    int offset = (int)&gp_interpreter_method_entry_ptr -
                 (int)&gp_base_label;
    ldr(pc, imm_index(gp, offset));
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

  // On xscale and ARM11, a conditional branch is faster than a conditional
  // ldr pc
  _interleave_frame_linking = false;

  if (omit_stack_frame()) {
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

    get_compiler_stack_limit(stack_limit);

    if (!GenerateROMImage) {
      strb(gp, imm_index(gp,
        address(_method_execution_sensor) - address(&gp_base_label)));
    }

    if (stack_bytes_needed < LeafMethodStackPadding &&
        method->max_execution_stack_count() < 20) {
      // Don't need to do an exact check -- if we overwrite slightly over
      // current_stack_limit, we will write into the StackPadding area, and
      // thus will not write outside of the legal stack area.
      if (USE_OVERFLOW_STUB) {
        link_frame_step1();
        _interleave_frame_linking = true;
      }
      if (JavaStackDirection < 0) {
        cmp(stack_limit, reg(jsp));
      } else {
        cmp(jsp, reg(stack_limit));
      }
    } else {
      TempRegister tmp(r3);
      add_imm(tmp, jsp, JavaStackDirection * stack_bytes_needed);
      if (JavaStackDirection < 0) {
        cmp(stack_limit, reg(tmp));
      } else {
        cmp(tmp, reg(stack_limit));
      }
    }

    // We trap back to interpreter if
    // (JavaStackDirection < 0) ->
    //      (stack_limit > jsp)
    //
    // (JavaStackDirection > 0) ->
    //      (jsp > stack_limit)
    if (USE_OVERFLOW_STUB) {
      Label stack_overflow, done;
      b(stack_overflow, hi);
      bind(done); // Not actually used on ARM port

      StackOverflowStub* stub =
        StackOverflowStub::allocate(stack_overflow, done, r1, r0 JVM_ZCHECK(stub));
      stub->insert();
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

  if (omit_stack_frame()) {
    // The rest of method_entry deal with pushing the call frame, so
    // we can safely return here.
    GUARANTEE(!ENABLE_WTK_PROFILER, "Profiler always need call frame");
    return;
  }

  if (!_interleave_frame_linking) {
    link_frame_step1();
  }

  COMPILER_COMMENT(("frame linking step2"));
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
    call_through_gp(&gp_shared_lock_synchronized_method_ptr JVM_CHECK);
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

// Perform the first step of frame linking. This code may be scheduled
// with stack overflow check to avoid pipeline stalls
void CodeGenerator::link_frame_step1() {
  COMPILER_COMMENT(("frame linking step1"));
  int jsp_shift = get_jsp_shift(method());

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

void CodeGenerator::imla(Value& result, 
                         Value& op1, Value& op2, Value& op3 JVM_TRAPS) {
  // NOTE: for now we handle only a simple case where all the arguments 
  // are in registers.
  if (op1.in_register() && op2.in_register() && op3.in_register()) {
    GUARANTEE(!op1.use_two_registers() && !op2.use_two_registers() && 
              !op3.use_two_registers(), "Integer operands expected");

    assign_register(result, op1);

    const Register res_reg = result.lo_register();
    const Register op1_reg = op1.lo_register();
    const Register op2_reg = op2.lo_register();
    const Register op3_reg = op3.lo_register();
    mla(res_reg, op1_reg, op2_reg, op3_reg);
  } else {
    Value temp(T_INT);
    imul(temp, op1, op2 JVM_CHECK);
    arithmetic(_add, result, temp, op3);    
  }
}

bool CodeGenerator::fold_arithmetic(Value& result, 
                                    Value& op1, Value& op2 JVM_TRAPS) {
  const int start_bci = Compiler::bci();
  Method * const mth = method();
  const Bytecodes::Code code = mth->bytecode_at(start_bci);
  int next_bci = start_bci + Bytecodes::length_for(code);

  if (Compiler::current()->entry_count_for(next_bci) > 1) {
    return false;
  }

  if (code == Bytecodes::_imul) {
    const Bytecodes::Code next_code = mth->bytecode_at(next_bci);

    if (next_code == Bytecodes::_iadd) {
#if !defined(PRODUCT) || USE_COMPILER_COMMENTS
      if (GenerateCompilerComments) {
        FixedArrayOutputStream output;
        Verbose++; // force callee names to be printed out, etc.
        mth->print_bytecodes(&output, next_bci);
        Verbose--;
        comment(output.array());
      }
#endif

      next_bci = next_bci + Bytecodes::length_for(mth, next_bci);

      Value summand(T_INT);
      Compiler::frame()->pop(summand);
      imla(result, op1, op2, summand JVM_CHECK_0);
      Compiler::closure()->set_next_bytecode_index(next_bci);
      return true;
    }
  }

  return false;
}

void CodeGenerator::int_binary_do(Value& result, Value& op1, Value& op2,
                                  BytecodeClosure::binary_op op JVM_TRAPS) {
  write_literals_if_desperate();

  GUARANTEE(!result.is_present(), "result must not be present");
  GUARANTEE(op1.in_register(), "op1 must be in a register");
  GUARANTEE(op2.is_immediate() || op2.in_register(),
            "op2 must be in a register or an immediate");


  const bool folded = fold_arithmetic(result, op1, op2 JVM_CHECK);
  if (folded) {
    return;
  }

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

  const CCMode cc = CCMode(ENABLE_CONDITIONAL_BRANCH_OPTIMIZATIONS);

  switch (op) {
    case BytecodeClosure::bin_sub  :
    case BytecodeClosure::bin_rsb  :
    case BytecodeClosure::bin_add  :
    case BytecodeClosure::bin_and  :
    case BytecodeClosure::bin_xor  :
    case BytecodeClosure::bin_or   :
      GUARANTEE(int(op) >= 0 && op < sizeof(table), "sanity");
      GUARANTEE(table[op] != 0xff, "sanity");
      arithmetic((Opcode)(table[op]), result, op1, op2, cc);
      break;
    case BytecodeClosure::bin_shr  :
      shift (asr, result, op1, op2, cc);
      break;
    case BytecodeClosure::bin_shl  :
      shift (lsl, result, op1, op2, cc);
      break;
    case BytecodeClosure::bin_ushr :
      shift (lsr, result, op1, op2, cc);
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
      rsb(resReg, opReg, zero, CCMode(ENABLE_CONDITIONAL_BRANCH_OPTIMIZATIONS));
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
                      Value& result, Value& op1, Value& op2, const CCMode cc)
{
  assign_register(result, op1);

  const Register resReg = result.lo_register();
  const Register op1Reg  = op1.lo_register();
  if (op2.is_immediate()) {
    CompilerLiteralAccessor cla;
    arith_imm(opcode,
              resReg, op1Reg, op2.as_int(), &cla, cc);
  } else {
    arith(opcode,
          resReg, op1Reg, reg(op2.lo_register()), cc);
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
    flush_frame(JVM_SINGLE_ARG_CHECK);
    setup_c_args(2, &op1, &op2, NULL);
    // Call the compiler stub.
    call_through_gp(&gp_compiler_idiv_irem_ptr JVM_CHECK);
    Register result_register = isRemainder ? r0 : r1;
    RegisterAllocator::reference(result_register);
    result.set_register(result_register);
  } else if (divisor == 0) {
    ZeroDivisorCheckStub* zero =
      ZeroDivisorCheckStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(zero));
    b(zero);
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

void CodeGenerator::shift(Shift shifter, Value& result, Value& op1, Value& op2,
                  const CCMode cc)
{
  if (op2.is_immediate()) {
    assign_register(result, op1);
    const Register resReg = result.lo_register();

    // We have to treat 0 as a special case since "asr 0" and "lsr 0"
    // don't actually mean "shift right by zero"
    int shift = (op2.as_int() & 0x1f);
    if (shift == 0) {
      mov_reg(resReg, op1.lo_register(), cc);
    } else {
      mov(resReg, imm_shift(op1.lo_register(), shifter, shift), cc);
    }
  } else {
    assign_register(result, op2); // result & op1 can't be same
    const Register resReg = result.lo_register();
    andr(resReg, op2.lo_register(), imm(0x1f));
    mov(resReg, reg_shift(op1.lo_register(), shifter, resReg), cc);
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
#if ENABLE_ARM_VFP
    if (int(op) < int(BytecodeClosure::bin_rem)) {
      if (op1.is_immediate()) {
        op1.materialize();
      }
      if (op2.is_immediate()) {
        op2.materialize();
      }
      ensure_in_float_register(op1);
      ensure_in_float_register(op2);

      RegisterAllocator::reference(op1.lo_register());
      RegisterAllocator::reference(op2.lo_register());

#if ENABLE_SOFT_FLOAT
      if (RunFastMode) {
        RegisterAllocator::spill(lr);
        GUARANTEE(!RegisterAllocator::is_referenced(lr), "lr must be free");
        RegisterAllocator::reference(lr);
      }
#endif

      result.assign_register();

      switch (op) {
      case BytecodeClosure::bin_add:
        fadds(result.lo_register(), op1.lo_register(), op2.lo_register());
        break;
      case BytecodeClosure::bin_sub:
        fsubs(result.lo_register(), op1.lo_register(), op2.lo_register());
        break;
      case BytecodeClosure::bin_mul:
        fmuls(result.lo_register(), op1.lo_register(), op2.lo_register());
        break;
      case BytecodeClosure::bin_div:
        fdivs(result.lo_register(), op1.lo_register(), op2.lo_register());
        break;
      }

      RegisterAllocator::dereference(op1.lo_register());
      RegisterAllocator::dereference(op2.lo_register());

#if ENABLE_SOFT_FLOAT
      if (RunFastMode) {
        GUARANTEE(result.in_register(), "result must be in a register");
        GUARANTEE(is_vfp_register(result.lo_register()), "result must be in a vfp register");
        RegisterAllocator::reference(result.lo_register());

        COMPILER_COMMENT(("Check for VFP exceptions"));
        fmrx(lr, fpscr);
        tst(lr, imm(0x8f));

        bl((address)vfp_redo, ne);
        RegisterAllocator::dereference(result.lo_register());
        RegisterAllocator::dereference(lr);
      }
#endif // ENABLE_SOFT_FLOAT
    } else
#endif  // ENABLE_ARM_VFP
    {
      ensure_not_in_float_register(op1);
      ensure_not_in_float_register(op2);

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
}

void CodeGenerator::float_unary_do(Value& result, Value& op1,
                                   BytecodeClosure::unary_op op JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();

  GUARANTEE(op == BytecodeClosure::una_neg || op == BytecodeClosure::una_abs,
            "Sanity")

  GUARANTEE(!result.is_present(), "result must not be present");
  GUARANTEE(op1.in_register(), "op1 must be in a register");

#if ENABLE_ARM_VFP
  ensure_in_float_register(op1);
#else
  ensure_not_in_float_register(op1);
#endif

  assign_register(result, op1);
#if ENABLE_ARM_VFP
  if (op == BytecodeClosure::una_neg) {
    fnegs(result.lo_register(), op1.lo_register());
  } else {
    fabss(result.lo_register(), op1.lo_register());
  }
#else
  Opcode opcode = (op == BytecodeClosure::una_neg ? _eor : _bic);
  arith(opcode, result.lo_register(), op1.lo_register(), imm_rotate(2,2));
#endif
}

void CodeGenerator::float_cmp(Value& result, BytecodeClosure::cond_op cond,
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
#if ENABLE_ARM_VFP
    Label done;
    if (op1.is_immediate()) {
      op1.materialize();
    }
    if (op2.is_immediate()) {
      op2.materialize();
    }
    ensure_in_float_register(op1);
    ensure_in_float_register(op2);

#if ENABLE_SOFT_FLOAT
    if (RunFastMode) {
      RegisterAllocator::spill(lr);
      GUARANTEE(!RegisterAllocator::is_referenced(lr), "lr must be free");
      RegisterAllocator::reference(lr);
    }
#endif

    result.assign_register();

    fcmpes(op1.lo_register(), op2.lo_register());
    fmstat();
    mov_imm(result.lo_register(), 1);

    if (cond == BytecodeClosure::lt) {
      b(done, gt);
      fcmps(op1.lo_register(), op2.lo_register());
      fmstat();
      mvn(result.lo_register(), imm(0), ne);
    } else if (cond == BytecodeClosure::gt) {
      mvn(result.lo_register(), imm(0), mi);
      b(done, gt);
      fcmps(op1.lo_register(), op2.lo_register());
      fmstat();
    }
    mov_imm(result.lo_register(), 0, eq);
    bind(done);

#if ENABLE_SOFT_FLOAT
    if (RunFastMode) {
      COMPILER_COMMENT(("Check for VFP exceptions"));
      fmrx(lr, fpscr);
      tst(lr, imm(0x8f));
      bl((address)vfp_fcmp_redo, ne);
      RegisterAllocator::dereference(lr);
    }
#endif

#else  // !ENABLE_ARM_VFP
    ensure_not_in_float_register(op1);
    ensure_not_in_float_register(op2);
    call_simple_c_runtime(result, (address)runtime_func, op1, op2);
#endif  // ENABLE_ARM_VFP
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
#if ENABLE_ARM_VFP
    if (int(op) < int(BytecodeClosure::bin_rem)) {
      if (op1.is_immediate()) {
        op1.materialize();
      }
      if (op2.is_immediate()) {
        op2.materialize();
      }
      ensure_in_float_register(op1);
      ensure_in_float_register(op2);

      RegisterAllocator::reference(op1.lo_register());
      RegisterAllocator::reference(op1.hi_register());
      RegisterAllocator::reference(op2.lo_register());
      RegisterAllocator::reference(op2.hi_register());

#if ENABLE_SOFT_FLOAT
      if (RunFastMode) {
        RegisterAllocator::spill(lr);
        GUARANTEE(!RegisterAllocator::is_referenced(lr), "lr must be free");
        RegisterAllocator::reference(lr);
      }
#endif

      result.assign_register();

      switch (op) {
      case BytecodeClosure::bin_add:
        faddd(result.lo_register(), op1.lo_register(), op2.lo_register());
        break;
      case BytecodeClosure::bin_sub:
        fsubd(result.lo_register(), op1.lo_register(), op2.lo_register());
        break;
      case BytecodeClosure::bin_mul:
#if ENABLE_SOFT_FLOAT && ENABLE_ARM9_VFP_BUG_WORKAROUND      
        if (RunFastMode) {                
          COMPILER_COMMENT(("fmuld to fmrx hazard workaround"));
          fmuld(result.lo_register(), op1.lo_register(), op2.lo_register());     
        }
#endif        
        fmuld(result.lo_register(), op1.lo_register(), op2.lo_register());
        break;        
      case BytecodeClosure::bin_div:
        fdivd(result.lo_register(), op1.lo_register(), op2.lo_register());
        break;
      }

      RegisterAllocator::dereference(op1.lo_register());
      RegisterAllocator::dereference(op1.hi_register());
      RegisterAllocator::dereference(op2.lo_register());
      RegisterAllocator::dereference(op2.hi_register());
      
#if ENABLE_SOFT_FLOAT
      if (RunFastMode) {
        GUARANTEE(result.in_register(), "result must be in a register");
        GUARANTEE(is_vfp_register(result.lo_register()), "result must be in a vfp register");
        RegisterAllocator::reference(result.lo_register());
        RegisterAllocator::reference(result.hi_register());

        COMPILER_COMMENT(("Check for VFP exceptions"));
        fmrx(lr, fpscr);            
        tst(lr, imm(0x9F));
        bl((address)vfp_double_redo, ne);
        
        RegisterAllocator::dereference(result.lo_register());
        RegisterAllocator::dereference(result.hi_register());
        RegisterAllocator::dereference(lr);
      }
#endif // ENABLE_SOFT_FLOAT
    } else
#endif  // ENABLE_ARM_VFP
    {
      ensure_not_in_float_register(op1);
      ensure_not_in_float_register(op2);
      call_simple_c_runtime(result, (address)runtime_func, op1, op2);
    }
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

#if ENABLE_ARM_VFP
  ensure_in_float_register(op1);
#endif

  assign_register(result, op1);

#if ENABLE_ARM_VFP
  if (op == BytecodeClosure::una_neg) {
    fnegd(result.lo_register(), op1.lo_register());
  } else {
    fabsd(result.lo_register(), op1.lo_register());
  }
#else
  const Opcode opcode =  (op == BytecodeClosure::una_neg) ? _eor  : _bic;

  if (TARGET_MSW_FIRST_FOR_DOUBLE) {
    // The first word contains the sign bit
    arith(opcode, result.lo_register(),  op1.lo_register(), imm_rotate(2,2));
    mov_reg(      result.hi_register(),  op1.hi_register());
  } else {
    // The second word contains the sign bit
    arith(opcode, result.hi_register(),  op1.hi_register(), imm_rotate(2,2));
    mov_reg(      result.lo_register(),  op1.lo_register());
  }
#endif
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
#if ENABLE_ARM_VFP
    Label done;
    if (op1.is_immediate()) {
      op1.materialize();
    }
    if (op2.is_immediate()) {
      op2.materialize();
    }
    ensure_in_float_register(op1);
    ensure_in_float_register(op2);

#if ENABLE_SOFT_FLOAT
    if (RunFastMode) {
      RegisterAllocator::spill(lr);
      GUARANTEE(!RegisterAllocator::is_referenced(lr), "lr must be free");
      RegisterAllocator::reference(lr);
    }
#endif

    result.assign_register();

    fcmped(op1.lo_register(), op2.lo_register());
    fmstat();
    mov_imm(result.lo_register(), 1);

    if (cond == BytecodeClosure::lt) {
      b(done, gt);
      fcmpd(op1.lo_register(), op2.lo_register());
      fmstat();
      mvn(result.lo_register(), imm(0), ne);
    } else if (cond == BytecodeClosure::gt) {
      mvn(result.lo_register(), imm(0), mi);
      b(done, gt);
      fcmpd(op1.lo_register(), op2.lo_register());
      fmstat();
    }
    mov_imm(result.lo_register(), 0, eq);
    bind(done);

#if ENABLE_SOFT_FLOAT
    if (RunFastMode) {
      COMPILER_COMMENT(("Check for VFP exceptions"));
      fmrx(lr, fpscr);
      tst(lr, imm(0x8f));
      bl((address)vfp_dcmp_redo, ne);
      RegisterAllocator::dereference(lr);
    }
#endif

#else  // !ENABLE_ARM_VFP
    ensure_not_in_float_register(op1);
    ensure_not_in_float_register(op2);
    call_simple_c_runtime(result, (address)runtime_func, op1, op2);
#endif  // ENABLE_ARM_VFP
  }
}
#endif

// Currently, this function is only used for floating point.
// It is actually rather generic and can be used for any C function
// that is guaranteed to never call into the VM.

void CodeGenerator::vcall_simple_c_runtime(Value& result,
                                           address runtime_func, ...) {
#if ENABLE_ARM_VFP
  // Some C runtime routine such as d2i may touch the FP registers. Let's
  // make sure all FP registers are flushed to the stack.
  frame()->flush_fpu();
#endif
  GUARANTEE(runtime_func != 0, "sanity check");
  GUARANTEE(!omit_stack_frame(),
            "cannot call runtime functions with omitted compiled frame");
  int i;
  static const Register ctemps[] = { r0, r1, r2, r3, r12, lr };

  for (i = 0; i < ARRAY_SIZE(ctemps); i++) {
    RegisterAllocator::reference(ctemps[i]);

    //cse
    RegisterAllocator::wipe_notation_of(ctemps[i]);
    VERBOSE_CSE(("clear reg %s", Disassembler::reg_name(ctemps[i])));

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

#if ENABLE_ARM_V5TE && !ENABLE_EMBEDDED_CALLINFO
  // When possible, use 
  //   blx   dst
  // instead of
  //   mov   lr, pc
  //   mov   pc, dst
  if (unbound_literal_count() == 0 || !need_to_force_literals()) {
    blx(r12);
  } else
#endif
  {
    int offset = code_size();  // offset of the next instruction
    add(lr, pc, imm_rotate(0,0));
    // IMPL_NOTE: macro needed to avoid #if
#if ENABLE_THUMB_VM
    bx(r12);
#else
    mov(pc, reg(r12));
#endif
    write_literals();
    if (!has_overflown_compiled_method()) {
      *(int *)addr_at(offset) |= imm(code_size() - offset - 8);
    }
  }

  if (JavaStackDirection > 0 && sp == jsp) {
    ldr_imm_index(jsp, sp);
  }

#if ENABLE_ARM_VFP
  if (result.type() == T_FLOAT || result.type() == T_DOUBLE) {
    result.assign_register();
    if (result.type() == T_FLOAT) {
      fmsr(result.lo_register(), r0);
    } else {
      fmdrr(result.lo_register(), r0, r1);
    }
    return;
  }
#endif  // ENABLE_ARM_VFP

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
#if ENABLE_ARM_VFP
    TempVFPRegister fvalue;

    ensure_not_in_float_register(value);
    fmsr(fvalue, value.lo_register());

#if ENABLE_SOFT_FLOAT
      if (RunFastMode) {
        RegisterAllocator::spill(lr);
        GUARANTEE(!RegisterAllocator::is_referenced(lr), "lr must be free");
        RegisterAllocator::reference(lr);
      }
#endif // ENABLE_SOFT_FLOAT

    result.assign_register();
    fsitos(result.lo_register(), fvalue);

#if ENABLE_SOFT_FLOAT
      if (RunFastMode) {
        GUARANTEE(result.in_register(), "result must be in a register");
        GUARANTEE(is_vfp_register(result.lo_register()), "result must be in a vfp register");
        RegisterAllocator::reference(result.lo_register());

        COMPILER_COMMENT(("Check for VFP exceptions"));
        fmrx(lr, fpscr);
        tst(lr, imm(0x8e));

        bl((address)vfp_redo, ne);
        RegisterAllocator::dereference(result.lo_register());
        RegisterAllocator::dereference(lr);
      }
#endif // ENABLE_SOFT_FLOAT

#else  // !ENABLE_ARM_VFP
    call_simple_c_runtime(result, (address)::jvm_i2f, value);
#endif // ENABLE_ARM_VFP
  }
}

void CodeGenerator::i2d(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_double(::jvm_i2d(value.as_int()));
  } else {
#if ENABLE_ARM_VFP
    Assembler::Register dreg = RegisterAllocator::allocate_double_register();

    ensure_not_in_float_register(value);
    fmsr(dreg, value.lo_register());

#if ENABLE_SOFT_FLOAT
     if (RunFastMode) {
        RegisterAllocator::spill(lr);
        GUARANTEE(!RegisterAllocator::is_referenced(lr), "lr must be free");
        RegisterAllocator::reference(lr);
     }
#endif // ENABLE_SOFT_FLOAT

    result.assign_register();
    fsitod(result.lo_register(), dreg);

#if ENABLE_SOFT_FLOAT
      if (RunFastMode) {
        GUARANTEE(result.in_register(), "result must be in a register");
        GUARANTEE(is_vfp_register(result.lo_register()), "result must be in a vfp register");
        RegisterAllocator::reference(result.lo_register());
        RegisterAllocator::reference(result.hi_register());

        COMPILER_COMMENT(("Check for VFP exceptions"));
        fmrx(lr, fpscr);
        tst(lr, imm(0x8e));

        bl((address)vfp_double_redo, ne);
        RegisterAllocator::dereference(result.lo_register());
        RegisterAllocator::dereference(result.hi_register());
        RegisterAllocator::dereference(lr);
      }
#endif // ENABLE_SOFT_FLOAT

    RegisterAllocator::dereference(dreg);
    RegisterAllocator::dereference((Assembler::Register)((int)dreg + 1));
#else  // !ENABLE_ARM_VFP
    call_simple_c_runtime(result, (address)::jvm_i2d, value);
#endif  // ENABLE_ARM_VFP
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
#if ENABLE_ARM_VFP
    TempVFPRegister fresult;

    ensure_in_float_register(value);
    RegisterAllocator::reference(value.lo_register());

#if ENABLE_SOFT_FLOAT
      if (RunFastMode) {
        RegisterAllocator::spill(lr);
        GUARANTEE(!RegisterAllocator::is_referenced(lr), "lr must be free");
        RegisterAllocator::reference(lr);
      }
#endif // ENABLE_SOFT_FLOAT

    result.assign_register();
    ftosizs(fresult, value.lo_register());

    RegisterAllocator::dereference(value.lo_register());

#if ENABLE_SOFT_FLOAT
      if (RunFastMode) {
        RegisterAllocator::reference(result.lo_register());

        COMPILER_COMMENT(("Check for VFP exceptions"));
        fmrx(lr, fpscr);
        tst(lr, imm(0x8e));

        bl((address)vfp_redo, ne);
        RegisterAllocator::dereference(result.lo_register());
        RegisterAllocator::dereference(lr);
      }
#endif // ENABLE_SOFT_FLOAT

      fmrs(result.lo_register(), fresult);

#else  // !ENABLE_ARM_VFP
    ensure_not_in_float_register(value);
    call_simple_c_runtime(result, (address)::jvm_f2i, value);
#endif // ENABLE_ARM_VFP
  }
}

void CodeGenerator::f2l(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_long(::jvm_f2l(value.as_float()));
  } else {
    ensure_not_in_float_register(value);
    call_simple_c_runtime(result, (address)::jvm_f2l, value);
  }
}

void CodeGenerator::f2d(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_double(::jvm_f2d(value.as_float()));
  } else {
#if ENABLE_ARM_VFP
    ensure_in_float_register(value);

    RegisterAllocator::reference(value.lo_register());

#if ENABLE_SOFT_FLOAT
    if (RunFastMode) {
      RegisterAllocator::spill(lr);
      GUARANTEE(!RegisterAllocator::is_referenced(lr), "lr must be free");
      RegisterAllocator::reference(lr);
    }
#endif // ENABLE_SOFT_FLOAT

    result.assign_register();
    fcvtds(result.lo_register(), value.lo_register());

    RegisterAllocator::dereference(value.lo_register());

#if ENABLE_SOFT_FLOAT
    if (RunFastMode) {
      RegisterAllocator::reference(result.lo_register());
      RegisterAllocator::reference(result.hi_register());

      COMPILER_COMMENT(("Check for VFP exceptions"));
      fmrx(lr, fpscr);
      tst(lr, imm(0x8e));

      bl((address)vfp_redo, ne);
      RegisterAllocator::dereference(result.lo_register());
      RegisterAllocator::dereference(result.hi_register());
      RegisterAllocator::dereference(lr);
    }
#endif // ENABLE_SOFT_FLOAT

#else  // !ENABLE_ARM_VFP
    call_simple_c_runtime(result, (address)::jvm_f2d, value);
#endif  // ENABLE_FLOAT
  }
}

void CodeGenerator::d2i(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_int(::jvm_d2i(value.as_double()));
  } else {
#if ENABLE_ARM_VFP
    TempVFPRegister freg;

    ensure_in_float_register(value);
    RegisterAllocator::reference(value.lo_register());
    RegisterAllocator::reference(value.hi_register());

#if ENABLE_SOFT_FLOAT
    if (RunFastMode) {
      RegisterAllocator::spill(lr);
      GUARANTEE(!RegisterAllocator::is_referenced(lr), "lr must be free");
      RegisterAllocator::reference(lr);
    }
#endif // ENABLE_SOFT_FLOAT

    result.assign_register();
    ftosizd(freg, value.lo_register());

    RegisterAllocator::dereference(value.lo_register());
    RegisterAllocator::dereference(value.hi_register());

#if ENABLE_SOFT_FLOAT
    if (RunFastMode) {
      RegisterAllocator::reference(result.lo_register());

      COMPILER_COMMENT(("Check for VFP exceptions"));
      fmrx(lr, fpscr);
      tst(lr, imm(0x8e));

      bl((address)vfp_double_redo, ne);
      RegisterAllocator::dereference(result.lo_register());
      RegisterAllocator::dereference(lr);
    }
#endif // ENABLE_SOFT_FLOAT

    fmrs(result.lo_register(), freg);
#else  // !ENABLE_ARM_VFP
    call_simple_c_runtime(result, (address)::jvm_d2i, value);
#endif // ENABLE_ARM_VFP
  }
}

void CodeGenerator::d2l(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_long(::jvm_d2l(value.as_double()));
  } else {
    ensure_not_in_float_register(value);
    call_simple_c_runtime(result, (address)::jvm_d2l, value);
  }
}

void CodeGenerator::d2f(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_float(::jvm_d2f(value.as_double()));
  } else {
#if ENABLE_ARM_VFP
    ensure_in_float_register(value);
    RegisterAllocator::reference(value.lo_register());
    RegisterAllocator::reference(value.hi_register());

#if ENABLE_SOFT_FLOAT
    if (RunFastMode) {
      RegisterAllocator::spill(lr);
      GUARANTEE(!RegisterAllocator::is_referenced(lr), "lr must be free");
      RegisterAllocator::reference(lr);
    }
#endif // ENABLE_SOFT_FLOAT

    result.assign_register();
    fcvtsd(result.lo_register(), value.lo_register());

    RegisterAllocator::dereference(value.lo_register());
    RegisterAllocator::dereference(value.hi_register());

#if ENABLE_SOFT_FLOAT
    if (RunFastMode) {
      RegisterAllocator::reference(result.lo_register());

      COMPILER_COMMENT(("Check for VFP exceptions"));
      fmrx(lr, fpscr);
      tst(lr, imm(0x8e));

      bl((address)vfp_double_redo, ne);
      RegisterAllocator::dereference(result.lo_register());
      RegisterAllocator::dereference(lr);
    }
#endif // ENABLE_SOFT_FLOAT

#else  // !ENABLE_ARM_VFP
    call_simple_c_runtime(result, (address)::jvm_d2f, value);
#endif // ENABLE_ARM_VFP
  }
}
#endif  // ENABLE_FLOAT

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
      ZeroDivisorCheckStub* zero =
        ZeroDivisorCheckStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(zero));
      if (op2.is_immediate()) {
        jmp(zero);
      } else {
        TempRegister tmp;
        orr(tmp, op2.lo_register(), reg(op2.hi_register()), set_CC);
        b(zero, eq);
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

void CodeGenerator::cmp_values(Value& op1, Value& op2, 
                               Assembler::Condition cond) {
  GUARANTEE(op1.in_register(), "op1 must be in a register");
  GUARANTEE(op2.is_immediate() || op2.in_register(),
            "op2 must be in a register or an immediate");

  const Register op1_lo_reg = op1.lo_register();
  if (op2.is_immediate()) {
    CompilerLiteralAccessor cla;
    cmp_imm(op1_lo_reg, op2.as_int(), &cla, cond);
  } else {
    cmp(op1_lo_reg, reg(op2.lo_register()), cond);
  }
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
#elif !ENABLE_PAGE_PROTECTION
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
  COMPILER_COMMENT(("timer tick checkpoint"));

#if ENABLE_CODE_PATCHING
  if (!GenerateROMImage && Compiler::is_in_loop() &&
      BytecodeCompileClosure::jump_from_bci() != 0) {
    // can skip timer tick check
    TimerTickStub::Raw stub = TimerTickStub::allocate(Compiler::bci(),
                              code_size(), timer_tick, done JVM_NO_CHECK);
    if (stub.not_null()) {
      stub().insert();
      frame()->clear_literals();
    }
    return;
  }
#endif

#if ENABLE_PAGE_PROTECTION
  strb(gp, imm_index(gp, -8));
  emit_compressed_vsf(frame());
  write_call_info(0 JVM_NO_CHECK_AT_BOTTOM);
  return;
#elif ENABLE_XSCALE_WMMX_TIMER_TICK && !ENABLE_TIMER_THREAD
  textrcb(0);
  b(timer_tick, ne);
#else
  TempRegister tmp;
  get_rt_timer_ticks(tmp);
  cmp(tmp, imm(0));
  b(timer_tick, ne);
#endif

  write_literals_if_desperate();
bind(done);

  TimerTickStub* stub =
    TimerTickStub::allocate(Compiler::bci(), timer_tick, done JVM_ZCHECK(stub));
  stub->insert();
  frame()->clear_literals();
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
  InstanceOfStub* stub =
    InstanceOfStub::allocate(bci(), class_id, slow_case, done_checking,
                               result.lo_register() JVM_ZCHECK(stub));
  stub->insert();
  frame()->pop(object);

  // If we go to the stub, we can't be guaranteed it has preserved literals
  frame()->clear_literals();
}

void CodeGenerator::check_cast_stub(CompilationQueueElement* cqe JVM_TRAPS) {
  int class_id = CheckCastStub::cast(cqe)->class_id();
  mov_imm(tmp0, class_id);
  call_through_gp(&gp_compiler_checkcast_ptr JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::instance_of_stub(CompilationQueueElement* cqe JVM_TRAPS) {
  int class_id = InstanceOfStub::cast(cqe)->class_id();
  mov_imm(r0, class_id);
  call_through_gp(&gp_compiler_instanceof_ptr JVM_NO_CHECK_AT_BOTTOM);
}

#if ENABLE_INLINE_COMPILER_STUBS
void CodeGenerator::new_object_stub(CompilationQueueElement* cqe JVM_TRAPS) {
  Assembler::Register jnear = NewObjectStub::cast(cqe)->java_near();
  ldr(r1, imm_index(jnear, JavaNear::klass_offset()));
  call_through_gp(&gp_compiler_new_object_ptr JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::new_type_array_stub(CompilationQueueElement* cqe JVM_TRAPS) {
  Assembler::Register jnear = NewTypeArrayStub::cast(cqe)->java_near();
  Assembler::Register length = NewTypeArrayStub::cast(cqe)->length();
  if (length != r1) {
    ldr(r1, imm_index(jnear, JavaNear::klass_offset()));
    mov_reg(r2, length);
  } else if (jnear != r2) {
    mov_reg(r2, length);
    ldr(r1, imm_index(jnear, JavaNear::klass_offset()));
  } else {
    Assembler::Register tmp = NewTypeArrayStub::cast(cqe)->result_register();
    ldr(tmp, imm_index(jnear, JavaNear::klass_offset()));
    mov_reg(r2, length);
    mov_reg(r1, tmp);
  }
  call_through_gp(&gp_compiler_new_type_array_ptr JVM_NO_CHECK_AT_BOTTOM);
}
#endif // ENABLE_INLINE_COMPILER_STUBS

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

#if ENABLE_INLINE_COMPILER_STUBS
  
  InstanceSize size = klass->instance_size();
  GUARANTEE(size.is_fixed(), "Sanity");
  Label slow_case, done;

  // Handle finalization by going slow-case for objects with finalizers.
  if (klass->has_finalizer()) {
    flush_frame(JVM_SINGLE_ARG_CHECK);
    ldr_oop(r1, klass);
    call_vm((address) _newobject, T_OBJECT JVM_CHECK);
    RegisterAllocator::reference(r0);
    result.set_register(r0);
    return;
  }

  // Dump literals if max code size generated for the bytecode
  // makes the offset invalid.
  enum {max_codesize = 32};
  write_literals_if_desperate(max_codesize);

  const Register new_top = RegisterAllocator::allocate();
  const Register old_end = RegisterAllocator::allocate();
  result.assign_register();
  const Register obj = result.lo_register();
  const int bytes = size.value();

  get_inline_allocation_top(obj);
  get_inline_allocation_end(old_end);
  add_imm(new_top, obj, bytes);
  cmp(new_top, reg(old_end));

  const Register jnear = old_end;
  Oop::Raw java_near = klass->prototypical_near();
  ldr_oop(jnear, &java_near);

  b(slow_case, hi);
  set_inline_allocation_top(new_top);
  str(jnear, imm_index(obj, JavaOop::klass_offset()));
  bind(done);

  RegisterAllocator::dereference(new_top);
  RegisterAllocator::dereference(old_end);

  NewObjectStub* stub =
    NewObjectStub::allocate(Compiler::bci(), obj, jnear,
                              slow_case, done JVM_ZCHECK(stub));
  stub->insert();

  // If we go to the stub, we can't be guaranteed it has preserved literals
  frame()->clear_literals();

#else // !ENABLE_INLINE_COMPILER_STUBS

  GUARANTEE(klass->instance_size().is_fixed(), "Sanity");
  // Do flushing, and remember to unmap.
  flush_frame(JVM_SINGLE_ARG_CHECK);

  // Handle finalization by going slow-case for objects with finalizers.
  if (klass->has_finalizer()) {
    // _newobject(Thread&, raw_class);
    ldr_oop(r1, klass);
    call_vm((address) _newobject, T_OBJECT JVM_CHECK);
  } else {
    ldr_oop(r0, klass);
    call_through_gp(&gp_compiler_new_object_ptr JVM_CHECK);
  }
  // The result is in r0
  RegisterAllocator::reference(r0);
  result.set_register(r0);
#endif // ENABLE_INLINE_COMPILER_STUBS
}

void CodeGenerator::new_object_array(Value& result, JavaClass* element_class,
                                     Value& length JVM_TRAPS) {
  UsingFastOops fast_oops;
  JavaClass::Fast array_class = element_class->get_array_class(1 JVM_CHECK);
  JavaNear::Fast java_near = array_class().prototypical_near();

  // Do flushing, and remember to unmap.
  flush_frame(JVM_SINGLE_ARG_CHECK);

  // Call the allocation routine.
  Value save_reg_for_oop(T_ILLEGAL);
  setup_c_args(2, &save_reg_for_oop, &length, NULL);
  ldr_oop(r0, &java_near);
  call_through_gp(&gp_compiler_new_obj_array_ptr JVM_CHECK);

  // The result is in r0
  RegisterAllocator::reference(r0);
  result.set_register(r0);
}

void CodeGenerator::new_basic_array(Value& result, BasicType type,
                                    Value& length JVM_TRAPS) {
  COMPILER_COMMENT(("new_type_array"));

#if ENABLE_INLINE_COMPILER_STUBS

  // Dump literals if max code size generated for the bytecode
  // makes the offset invalid.
  enum {max_codesize = 48};
  write_literals_if_desperate(max_codesize);

  TypeArrayClass* array_class = Universe::as_TypeArrayClass(type);
  JavaNear::Raw java_near = array_class->prototypical_near();
  const int maximum_safe_array_length = 1 << 20;
  Label slow_case, done;

  const Register new_top = RegisterAllocator::allocate();
  const Register old_end = RegisterAllocator::allocate();
  const Register jnear   = old_end;
  result.assign_register();
  const Register obj = result.lo_register();
  Register len;

  get_inline_allocation_top(obj);
  get_inline_allocation_end(old_end);

  if (length.is_immediate()) {
    len = new_top;
    int value = length.as_int();
    if (value < 0 || value > maximum_safe_array_length) {
      // not a usual case
      ldr_oop(jnear, &java_near);
      mov_imm(len, value);
      b(slow_case);
    } else {
      int size = ArrayDesc::allocation_size(value, array_class->scale());
      add_imm(new_top, obj, size);
      cmp(new_top, reg(old_end));
      ldr_oop(jnear, &java_near);
      set_inline_allocation_top(new_top, ls);
      mov_imm(len, value);
      b(slow_case, hi);
      str(jnear, imm_index(obj, JavaOop::klass_offset()));
      str(len, imm_index(obj, Array::length_offset()));
    }
  } else {
    int shift = jvm_log2(array_class->scale());
    len = length.lo_register();
    cmp(len, imm(maximum_safe_array_length));
    add(new_top, obj, imm_shift(length.lo_register(), lsl, shift), ls);
    if (shift >= LogBytesPerWord) {
      add(new_top, new_top, imm(Array::base_offset()), ls);
    } else {
      add(new_top, new_top, imm(Array::base_offset() + BytesPerWord - 1), ls);
      bic(new_top, new_top, imm(3), ls);
    }
    cmp(new_top, reg(old_end), ls);
    ldr_oop(jnear, &java_near);
    b(slow_case, hi);
    set_inline_allocation_top(new_top);
    str(jnear, imm_index(obj, JavaOop::klass_offset()));
    str(len, imm_index(obj, Array::length_offset()));
  }
  bind(done);

  RegisterAllocator::dereference(new_top);
  RegisterAllocator::dereference(old_end);

  NewTypeArrayStub* stub =
    NewTypeArrayStub::allocate(Compiler::bci(), obj, jnear, len,
                                 slow_case, done JVM_ZCHECK(stub));
  stub->insert();
  
  // If we go to the stub, we can't be guaranteed it has preserved literals
  frame()->clear_literals();

#else // !ENABLE_INLINE_COMPILER_STUBS

  // Do flushing, and remember to unmap.
  flush_frame(JVM_SINGLE_ARG_CHECK);

  TypeArrayClass* array_class = Universe::as_TypeArrayClass(type);

  UsingFastOops fast_oops;
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
  call_through_gp(&gp_compiler_new_type_array_ptr JVM_CHECK);

  // The result is in r0
  RegisterAllocator::reference(r0);
  result.set_register(r0);
#endif // ENABLE_INLINE_COMPILER_STUBS
}

void CodeGenerator::new_multi_array(Value& result JVM_TRAPS) {
  flush_frame(JVM_SINGLE_ARG_CHECK);

  // Call the runtime system.
  call_vm((address) multianewarray, T_ARRAY JVM_CHECK);

  // The result is in r0
  RegisterAllocator::reference(r0);
  result.set_register(r0);
}

void CodeGenerator::monitor_enter(Value& object JVM_TRAPS) {
  // For now we flush before calling the compiler monitor enter stub.
  flush_frame(JVM_SINGLE_ARG_CHECK);
  mov_reg(r0, object.lo_register());
  call_through_gp(&gp_shared_monitor_enter_ptr JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::monitor_exit(Value& object JVM_TRAPS) {
  // For now we flush before calling the compiler monitor exit stub.
  flush_frame(JVM_SINGLE_ARG_CHECK);
  // Make sure the object is in register r0 (tos_val).
  mov_reg(r0, object.lo_register());
  call_through_gp(&gp_shared_monitor_exit_ptr JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::set_result_register(Value& result) {  
#if USE_FP_RESULT_IN_VFP_REGISTER
  GUARANTEE(result.is_present(), "Sanity");
  switch (result.type()) {
    case T_FLOAT:      
      if (result.is_immediate()) {
        if (!frame()->result_register_contains(result.lo_bits())) {
          move_float_immediate(Assembler::s0, result.lo_bits());
        }
        return;
      }
      if (result.in_register()) {
        const Register reg = result.lo_register();
        if (reg != Assembler::s0) {
          fcpys(Assembler::s0, reg);
        }
        return;
      } 
      SHOULD_NOT_REACH_HERE();
      break;
    case T_DOUBLE:
      if (result.is_immediate()) {
        if (!frame()->result_register_contains(result.lo_bits(), result.hi_bits())) {
          move_double_immediate(Assembler::s0, result.lo_bits(), result.hi_bits());
        }
        return;
      }
      if (result.in_register()) {
        const Register reg = result.lo_register();
        if (reg != Assembler::s0) {
          fcpyd(Assembler::s0, reg);
        }
        return;
      } 
      SHOULD_NOT_REACH_HERE();
      break;
  }
#endif // USE_FP_RESULT_IN_VFP_REGISTER

  // Put the result into r0 or r0/r1
  setup_c_args(1, &result, NULL);
}

#if ENABLE_ARM_VFP
void CodeGenerator::ensure_in_float_register(Value& value) {
  if (value.type() == T_FLOAT && value.in_register()) {
    Register r = value.lo_register();
    if (is_arm_register(r)) {
      value.set_register(RegisterAllocator::allocate_float_register());
      fmsr(value.lo_register(), r);
    }
  } else if (value.type() == T_DOUBLE && value.in_register()) {
    Register lo = value.lo_register();
    Register hi = value.hi_register();
    if (is_arm_register(lo) && is_arm_register(hi)) {
      value.set_vfp_double_register(RegisterAllocator::allocate_double_register());
      fmdrr(value.lo_register(), lo, hi);
    }
  }
}

void CodeGenerator::ensure_not_in_float_register(Value& value,
                                                        bool need_to_copy) {
  if (value.type() == T_FLOAT && value.in_register()) {
    Register r = value.lo_register();
    if (r >= s0) {
      value.set_register(RegisterAllocator::allocate());
      if (need_to_copy) {
        fmrs(value.lo_register(), r);
      }
    }
  } else if (value.type() == T_DOUBLE && value.in_register()) {
    Register l = value.lo_register();
    Register h = value.hi_register();
    if (l >= s0 && h >= s0) {
      Register lo = RegisterAllocator::allocate();
      Register hi = RegisterAllocator::allocate();
      value.set_registers(lo, hi);
      if (need_to_copy) {
        fmrrd(value.lo_register(), value.hi_register(), l);
      }
    }
  }
}
#endif  // ENABLE_ARM_VFP

void CodeGenerator::return_result(Value& result JVM_TRAPS) {
  COMPILER_COMMENT(("return result"));
  set_result_register(result);
  restore_last_frame(JVM_SINGLE_ARG_CHECK);
  mov(pc, reg(lr));
  write_literals();
}

void CodeGenerator::return_void(JVM_SINGLE_ARG_TRAPS) {
  COMPILER_COMMENT(("return void"));
  restore_last_frame(JVM_SINGLE_ARG_CHECK);
  mov(pc, reg(lr));
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
  if (omit_stack_frame()) {
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
    if(omit_stack_frame()) {
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

    if(omit_stack_frame()) {
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

  flush_frame(JVM_SINGLE_ARG_CHECK);
  call_through_gp(&gp_shared_unlock_synchronized_method_ptr
                  JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::check_monitors(JVM_SINGLE_ARG_TRAPS) {
  // Make sure there are no locked monitors on the stack.
  Label unlocking_loop, unlocking_loop_entry;

  TempRegister lock;
  TempRegister object;
  TempRegister end;

  write_literals_if_desperate();

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
  UnlockExceptionStub* unlock_exception =
    UnlockExceptionStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(unlock_exception));
  b(unlock_exception, ne);

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

void CodeGenerator::invoke(const Method* method, 
                           bool must_do_null_check JVM_TRAPS) {
  bool is_native = method->is_native();
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

#if USE_AOT_COMPILATION
  bool skip_lookup = method->is_impossible_to_compile();
  if (GenerateROMImage) {
    // IMPL_NOTE: this needs to be fixed so that the AOT-compiled code can
    // directly invoke the method without a look up
    if (method->is_native() &&
        method->get_native_code() == (address)Java_void_unimplemented) {
      // Used by AOT only: we are calling a MIDP native method, which
      // is not resolved during romization
      skip_lookup = false;
    } else if (method->is_quick_native() &&
               method->get_quick_native_code() == 
               (address)Java_void_unimplemented) {
      skip_lookup = false;
    }
  }
#else
  const bool skip_lookup = method->is_impossible_to_compile();
#endif

  // We know that the method_execution_entry is stored inside the method.
  const bool entry_inside_method =
      (ObjectHeap::contains_moveable(method->obj()) && !GenerateROMImage);

  // We know that the method_execution_entry is stored at a fixed address
  const bool entry_in_fixed_address =
      ROM::system_text_contains(method->obj()) ||
      ((USE_IMAGE_MAPPING || USE_IMAGE_PRELOADING) &&
       ROM::in_any_loaded_bundle(method->obj()));
  const bool prefetch_entry_address = (!skip_lookup && entry_in_fixed_address);
  address entry_address = (address)method->variable_part();

  if (!must_do_null_check) {
    if (prefetch_entry_address) {
      mov_imm(tmp, entry_address);
    }
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
      COMPILER_PRINT_AS_YOU_GO((" generate a faked ldr instruction invoke=>\n"));
      ldr(tmp, imm_index(receiver.lo_register()));
      maybe_null_check_2_by_npce(receiver, T_OBJECT JVM_CHECK);
      if (prefetch_entry_address) {
        mov_imm(tmp, entry_address);
      }
      ldr_oop(callee, method);
    }
#else
    if (prefetch_entry_address) {
      mov_imm(tmp, entry_address);
    }
    cond = maybe_null_check_1(receiver);
    ldr_oop(callee, method);
    maybe_null_check_2(cond JVM_CHECK);
#endif
  }

  // The method must be in callee when we enter it
  hold_method.set_register(RegisterAllocator::allocate(callee));
  GUARANTEE(!frame()->is_mapping_something(callee), "Must be free");

  if (skip_lookup) {
    // We don't need to hunt for the entry.  Its current entry will not change
    const bool can_optimize_quick_natives = !ENABLE_WTK_PROFILER;

    if (can_optimize_quick_natives && method->is_quick_native()) {
      address native_code = method->get_quick_native_code();
      // We actually only need to flush the end of the stack containing the
      // arguments, but we don't really have any way of doing that..
      flush_frame(JVM_SINGLE_ARG_CHECK);
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
      mov(tmp, one);
      set_jvm_in_quick_native_method(tmp);
      Value result(T_INT);      // this is a lie.  But vcall wants something
      vcall_simple_c_runtime(result, (address)native_code, NULL);
      mov(tmp, zero);
      set_jvm_in_quick_native_method(tmp);
      {
        Label ok;
        get_jvm_quick_native_exception(tmp);
        cmp(tmp, zero);
        b(ok, eq);

        get_persistent_handles_addr(r0);
        ldr(r0, imm_index(r0, Universe::quick_native_throw_method_index * 
                          BytesPerWord));
        int code_offset = code_size(); // current pc
        add(lr, pc, imm_rotate(0,0));
        ldr_using_gp(pc, (address)&gp_interpreter_method_entry_ptr);
        write_call_info(size_of_parameters JVM_CHECK);
        // Patch the "add" instruction to make lr point 
        // at following instruction
        if (!has_overflown_compiled_method()) {
          *(int *)addr_at(code_offset) |= imm(code_size() - code_offset - 8);
        }

        bind(ok);
      }
      is_native = true;
    } else {
      address target = method->execution_entry();
      mov_imm(tmp, target);
      flush_frame(JVM_SINGLE_ARG_CHECK);
#if ENABLE_TRAMPOLINE
      if (!USE_AOT_COMPILATION || !GenerateROMImage) {
        call_from_compiled_code(method, tmp, 0, size_of_parameters JVM_CHECK);
      } else 
#endif
      {
        call_from_compiled_code(tmp, 0, size_of_parameters JVM_CHECK);
      }
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
        if (entry_inside_method) {
          ldr_imm_index(tmp, callee, Method::heap_execution_entry_offset());
        } else if (entry_in_fixed_address) {
          ldr_imm_index(tmp, tmp);
        } else {
          ldr_imm_index(tmp, callee, Method::variable_part_offset());
          ldr_imm_index(tmp, tmp);
        }
      weaver.start_alternate(JVM_SINGLE_ARG_CHECK);
      flush_frame(JVM_SINGLE_ARG_CHECK);
      weaver.flush();
    }
    // invoke the method
#if ENABLE_TRAMPOLINE
      if (!USE_AOT_COMPILATION || !GenerateROMImage) {
        call_from_compiled_code(method, tmp, 0, size_of_parameters JVM_CHECK);
      } else 
#endif
      {
        call_from_compiled_code(tmp, 0, size_of_parameters JVM_CHECK);
      }
  }

  Signature::Raw signature = method->signature();
  adjust_for_invoke(method->size_of_parameters(),
                    signature().return_type(), is_native);

#if ENABLE_WTK_PROFILER
  flush_frame(JVM_SINGLE_ARG_CHECK);
  call_vm((address)jprof_record_method_transition, T_VOID JVM_CHECK);
#endif
}

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
    ldr(tmp, imm_index(receiver.lo_register()));
    load_near_offset = code_size();

    maybe_null_check_2_by_npce(receiver, T_OBJECT JVM_CHECK);
#else
    cond = maybe_null_check_1(receiver);

    ldr(tmp, imm_index(receiver.lo_register()), cond);
    load_near_offset = code_size();

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
      flush_frame(JVM_SINGLE_ARG_CHECK);
    weaver.flush();
  }
  call_from_compiled_code(tmp, 0, size_of_parameters JVM_CHECK);
  
  adjust_for_invoke(size_of_parameters, return_type);

#if ENABLE_WTK_PROFILER
  flush_frame(JVM_SINGLE_ARG_CHECK);
  call_vm((address)jprof_record_method_transition, T_VOID JVM_CHECK);
#endif
}

void CodeGenerator::invoke_interface(JavaClass* klass, int itable_index,
                                     int parameters_size,
                                     BasicType return_type JVM_TRAPS) {
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
  cond = Assembler::al;
  ldr(tmp0, imm_index(receiver.lo_register()));
  maybe_null_check_2_by_npce(receiver, T_OBJECT JVM_CHECK);
#else
  cond = maybe_null_check_1(receiver);
  ldr(tmp0, imm_index(receiver.lo_register()), cond);
  maybe_null_check_2(cond JVM_CHECK);
#endif


  ldr(tmp0, imm_index(tmp0), cond);

  // Flush the virtual stack frame and unmap everything.
  flush_frame(JVM_SINGLE_ARG_CHECK);

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

  IncompatibleClassChangeStub* icc_stub =
    IncompatibleClassChangeStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(icc_stub));
  b(icc_stub, lt);

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
  flush_frame(JVM_SINGLE_ARG_CHECK);
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

#if USE_FP_RESULT_IN_VFP_REGISTER
    Register reg = r0;
    if ( return_type == T_FLOAT || return_type == T_DOUBLE ) {
      if( native ) {
        result.assign_register();
        if (return_type == T_FLOAT) {
          fmsr(result.lo_register(), r0);
        } else {
          fmdrr(result.lo_register(), r0, r1);
        }
        frame()->push(result);
        return;
      }
      reg = s0;
    }
#else
    const Register reg = r0;
#endif // USE_FP_RESULT_IN_VFP_REGISTER

    RegisterAllocator::reference(reg);
    if (result.is_two_word()) {
      RegisterAllocator::reference(Register(reg+1));
      result.set_registers(reg, Register(reg+1));
    } else {
      result.set_register(reg);
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
#if ENABLE_TRAMPOLINE
void CodeGenerator::call_from_compiled_code(const Method* callee, Register dst,
                                            int offset,
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

#if ENABLE_ARM_V5TE && !ENABLE_EMBEDDED_CALLINFO
  // When possible, use 
  //   blx   dst
  // instead of
  //   mov   lr, pc
  //   mov   pc, dst
  if (offset == 0 && !indirect &&
      (unbound_literal_count() == 0 || !need_to_force_literals())) {
    blx(dst);
    write_call_info(parameters_size JVM_NO_CHECK_AT_BOTTOM);
    return;
  }
#endif
  
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
  GUARANTEE(!Compiler::is_inlining(), 
            "Not tested: need to write root bci in the callinfo");
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

  QuickCatchStub* stub =
    QuickCatchStub::allocate(bci(), exception_obj, handler_bci,
                               quick_case JVM_ZCHECK_0(stub));
  stub->insert();

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
    call_through_gp(&gp_compiler_timer_tick_ptr, false JVM_NO_CHECK_AT_BOTTOM);
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
      call_through_gp(&gp_shared_call_vm_oop_ptr, false JVM_NO_CHECK_AT_BOTTOM);
    } else if (return_value_type == T_ILLEGAL) {
      call_through_gp(&gp_shared_call_vm_exception_ptr, false
                      JVM_NO_CHECK_AT_BOTTOM);
    } else {
      call_through_gp(&gp_shared_call_vm_ptr, false JVM_NO_CHECK_AT_BOTTOM);
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

  TypeCheckStub* stub =
    TypeCheckStub::allocate(bci(), slow_case, done_checking JVM_ZCHECK(stub));
  stub->insert();
  frame()->pop(object);
  frame()->pop(index);
  frame()->pop(array);

  // If we go to the stub, we can't be guaranteed it has preserved literals
  frame()->clear_literals();
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
#if ENABLE_CONDITIONAL_BRANCH_OPTIMIZATIONS
    /* case BytecodeClosure::negative*/ mi,
    /* case BytecodeClosure::positive*/ pl,
#endif
  };

  GUARANTEE(unsigned(cond) < sizeof table, "sanity");
  return CodeGenerator::Condition(table[cond]);
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

void CodeGenerator::conditional_jump_do(BytecodeClosure::cond_op condition, 
                                        Label& destination) {
  b(destination, convert_condition(condition));
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
      GUARANTEE(value->is_immediate(), "Sanity");
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
  if (null_check) {
#if !ENABLE_NPCE
    cond = maybe_null_check_1(object);
#endif
  }
  Value object_copy;
  object.copy(object_copy);
  FieldAddress address(object_copy, offset, result.type());
  load_from_address(result, result.type(), address, cond);
  if (null_check) {
#if ENABLE_NPCE
    maybe_null_check_2_by_npce(object, result.type()
                              JVM_NO_CHECK_AT_BOTTOM);
#else
    maybe_null_check_2(cond JVM_NO_CHECK_AT_BOTTOM);
#endif
  }
}

#if !ENABLE_CPU_VARIANT
void CodeGenerator::init_static_array(Value& result JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  flush_frame(JVM_SINGLE_ARG_CHECK);

  const Register src = tos_tag;
  const Register dst = tos_val;
  const Register size = tmp0;
  RegisterAllocator::reference(src);
  RegisterAllocator::reference(dst);
  RegisterAllocator::reference(size);

  if (GenerateCompilerAssertions) {
    cmp(result.lo_register(), zero);
    breakpoint(eq);
  }

  Method::Raw cur_method =
    Compiler::current()->current_compiled_method()->method();
  if (src == result.lo_register()) {
    add(dst, result.lo_register(), imm(Array::base_offset()));
    ldr_oop(src, &cur_method);
  } else {
    ldr_oop(src, &cur_method);
    add(dst, result.lo_register(), imm(Array::base_offset()));
  }

  // Load type size shift
  int params_offset = Method::base_offset() + bci() + 1;
  if (params_offset < 4096) {
    ldrb(tmp5, imm_index(src, params_offset, pre_indexed));
    add(src, src, one);
  } else {
    add_imm(src, src, params_offset);
    ldrb(tmp5, imm_index(src, 1, post_indexed));
  }
  // Load elements count low byte
  ldrb(size, imm_index(src, 1, post_indexed));
  // Load elements count high byte
  ldrb(tmp2, imm_index(src, 1, post_indexed));
  // Load array length
  ldr(tmp3, imm_index(dst, Array::length_offset() - Array::base_offset()));
  // Calculate actual size in bytes
  orr(size, size, imm_shift(tmp2, lsl, BitsPerByte));
  mov(size, reg_shift(size, lsl, tmp5));

  Value src_val(T_INT);
  src_val.set_register(src);

  Value dst_val(T_INT);
  dst_val.set_register(dst);

  Value size_val(T_INT);
  size_val.set_register(size);

  call_simple_c_runtime(dst_val, (address)jvm_memcpy,
    dst_val, src_val, size_val);
}
#endif //!ENABLE_CPU_VARIANT

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

#if ENABLE_INLINED_ARRAYCOPY
bool CodeGenerator::arraycopy(JVM_SINGLE_ARG_TRAPS) {
  write_literals_if_desperate();

  Label bailout, done;
  VirtualStackFrame* bailout_frame;

  enum {
    SRC_TYPE_CHECK,
    DST_TYPE_CHECK,
    SRC_NULL_CHECK,
    DST_NULL_CHECK,
    SRC_POS_CHECK,
    DST_POS_CHECK,
    LENGTH_CHECK,
    SRC_BOUND_CHECK,
    DST_BOUND_CHECK,
    CHECK_COUNT                            
  };

  BasicType array_element_type = T_ILLEGAL;
  int checks = right_n_bits(CHECK_COUNT); // all bits set

  RegisterAllocator::guarantee_all_free();

  {
    UsingFastOops fast_oops_2;
    JavaClass::Fast src_class;
    JavaClass::Fast dst_class;

    Value src(T_OBJECT);
    Value src_pos(T_INT);
    Value dst(T_OBJECT);
    Value dst_pos(T_INT);
    Value length(T_INT);

    int location = frame()->virtual_stack_pointer();
  
    GUARANTEE(location >= 4, "5 values must be on stack");
    frame()->value_at(length,  location--);
    frame()->value_at(dst_pos, location--);
    frame()->value_at(dst,     location--);
    frame()->value_at(src_pos, location--);
    frame()->value_at(src,     location--);

    // Type checks
    {
      const jushort src_class_id = src.class_id();
      const jushort dst_class_id = dst.class_id();

      src_class = Universe::class_from_id(src_class_id);
      dst_class = Universe::class_from_id(dst_class_id);

      if (src_class_id != 0 && dst_class_id != 0) {
        if (dst_class().is_array_class() && 
            src_class().is_subtype_of(&dst_class) &&
            (dst.is_exact_type() || dst_class().is_final_type())) {          
          clear_nth_bit(checks, SRC_TYPE_CHECK);
          clear_nth_bit(checks, DST_TYPE_CHECK);

          // Determine array element type
          if (dst_class().is_type_array_class()) {
            GUARANTEE(src_class().equals(&dst_class), "Must be equal");
            TypeArrayClass::Raw type_array_class = dst_class.obj();
            array_element_type = (BasicType)type_array_class().type();
          } else {
            GUARANTEE(dst_class().is_obj_array_class(), 
                      "Must be an object array class");
            array_element_type = T_OBJECT;
          }
        } else {
          // May need per-element type check. Go to slow case
          return false;
        }
      } else if (src_class_id != 0) {
        if (src_class().is_array_class() &&
            (src.is_exact_type() || src_class().is_final_type())) {
          clear_nth_bit(checks, SRC_TYPE_CHECK);
          
          // Determine array element type
          if (src_class().is_type_array_class()) {
            TypeArrayClass::Raw type_array_class = src_class.obj();
            array_element_type = (BasicType)type_array_class().type();
          } else {
            GUARANTEE(src_class().is_obj_array_class(), 
                      "Must be an object array class");
            array_element_type = T_OBJECT;
          }
        } else {
          return false;
        }
      } else if (dst_class_id != 0) {
        if (dst_class().is_array_class() &&
            (dst.is_exact_type() || dst_class().is_final_type())) {
          clear_nth_bit(checks, DST_TYPE_CHECK);

          // Determine array element type
          if (dst_class().is_type_array_class()) {
            TypeArrayClass::Raw type_array_class = dst_class.obj();
            array_element_type = (BasicType)type_array_class().type();
          } else {
            GUARANTEE(dst_class().is_obj_array_class(), 
                      "Must be an object array class");
            array_element_type = T_OBJECT;
          }
        } else {
          return false;
        }
      } else {
        // No type info available. Go to slow case
        return false;
      }
    }
    
    GUARANTEE(src_class.not_null() && dst_class.not_null(), 
              "Source and destination types must be determined by this point");
    GUARANTEE(array_element_type != T_ILLEGAL, 
              "Array element type must be determined by this point");

    // oop_write_barrier stuff seems too heavyweight for inlining
    if (array_element_type == T_OBJECT || array_element_type == T_ARRAY) {
      return false;
    }

    // Null checks
    {
      if (src.must_be_null() || dst.must_be_null()) {
        // Go to slow case
        return false;
      }

      if (src.must_be_nonnull()) {
        clear_nth_bit(checks, SRC_NULL_CHECK);
      }

      if (dst.must_be_nonnull()) {
        clear_nth_bit(checks, DST_NULL_CHECK);
      }
    }

    // Offset checks
    {
      if (src_pos.is_immediate()) {
        const int src_pos_imm = src_pos.as_int();
        if (src_pos_imm < 0) {
          return false;
        }

        clear_nth_bit(checks, SRC_POS_CHECK);
      }

      if (dst_pos.is_immediate()) {
        const int dst_pos_imm = dst_pos.as_int();
        if (dst_pos_imm < 0) {
          return false;
        }

        clear_nth_bit(checks, DST_POS_CHECK);
      }
    }

    // Bound checks
    {
      if (length.is_immediate()) {
        int min_length = 0;
        const int length_imm = length.as_int();
        if (length_imm < 0) {
          return false;
        }

        clear_nth_bit(checks, LENGTH_CHECK);

        if (src_pos.is_immediate() && src.has_known_min_length(min_length) && 
            ((unsigned)src_pos.as_int() + (unsigned)length_imm <= 
             (unsigned)min_length)) {
          clear_nth_bit(checks, SRC_BOUND_CHECK);
        }

        if (dst_pos.is_immediate() && dst.has_known_min_length(min_length) && 
            ((unsigned)dst_pos.as_int() + (unsigned)length_imm <= 
             (unsigned)min_length)) {
          clear_nth_bit(checks, DST_BOUND_CHECK);
        }
      }
    }

    bailout_frame = frame()->clone(JVM_SINGLE_ARG_ZCHECK_0(bailout_frame) );

    // Do null checks
    {
      Value null_value(T_INT);
      null_value.set_int(0);

      if (is_set_nth_bit(checks, SRC_NULL_CHECK) || 
          is_set_nth_bit(checks, DST_NULL_CHECK)) {
        comment("if (src == NULL || dst == NULL) goto bailout;");

        if (is_set_nth_bit(checks, SRC_NULL_CHECK)) {
          cmp_values(src, null_value);
          if (is_set_nth_bit(checks, DST_NULL_CHECK)) {
            cmp_values(dst, null_value, ne);
          }
        } else if (is_set_nth_bit(checks, DST_NULL_CHECK)) {
          cmp_values(dst, null_value);
        }

        b(bailout, eq);
      }
    }

    GUARANTEE(src.in_register() && dst.in_register(),
              "Non-null, so must be in registers");
    Register src_reg = src.lo_register();
    Register dst_reg = dst.lo_register();

    Register src_type_reg = Assembler::no_reg;
    Register dst_type_reg = Assembler::no_reg;

    // Do type check
    if (is_set_nth_bit(checks, SRC_TYPE_CHECK) ||
        is_set_nth_bit(checks, DST_TYPE_CHECK)) {
      // We need 2 spare registers for type check.
      // They must be available, since all regs were free at the beginning.
      GUARANTEE(RegisterAllocator::has_free(2, true/*spill*/),
                "Must have 2 registers available");

      src_type_reg = RegisterAllocator::allocate();
      dst_type_reg = RegisterAllocator::allocate();

      /*
       * For AOT-compilation, we ldr_oop classes from Java heap indirectly 
       * by index in class list. This is more expensive than load the
       * class right from the object.
       */
      const bool src_class_indirect_load = USE_AOT_COMPILATION &&
        GenerateROMImage && !ROM::system_contains(src_class.obj());
      const bool dst_class_indirect_load = USE_AOT_COMPILATION &&
        GenerateROMImage && !ROM::system_contains(dst_class.obj());

      comment("Get src.klass() and dst.klass()");
      if ((is_set_nth_bit(checks, SRC_TYPE_CHECK) && 
           is_set_nth_bit(checks, DST_TYPE_CHECK)) ||
          (is_set_nth_bit(checks, SRC_TYPE_CHECK) && 
           dst_class_indirect_load) ||
          (is_set_nth_bit(checks, DST_TYPE_CHECK) && 
           src_class_indirect_load)) {
        ldr(src_type_reg, imm_index(src_reg, Oop::klass_offset()));
        ldr(dst_type_reg, imm_index(dst_reg, Oop::klass_offset()));
        ldr(src_type_reg, imm_index(src_type_reg, Oop::klass_offset()));
        ldr(dst_type_reg, imm_index(dst_type_reg, Oop::klass_offset()));
      } else if (is_set_nth_bit(checks, SRC_TYPE_CHECK)) {
        ldr(src_type_reg, imm_index(src_reg, Oop::klass_offset()));
        ldr_oop(dst_type_reg, &dst_class);
        ldr(src_type_reg, imm_index(src_type_reg, Oop::klass_offset()));
      } else if (is_set_nth_bit(checks, DST_TYPE_CHECK)) {
        ldr(dst_type_reg, imm_index(dst_reg, Oop::klass_offset()));
        ldr_oop(src_type_reg, &src_class);
        ldr(dst_type_reg, imm_index(dst_type_reg, Oop::klass_offset()));
      } else {
        SHOULD_NOT_REACH_HERE();
      }
    }

    // Do simple sign checks
    if (is_set_nth_bit(checks, SRC_POS_CHECK)) {
      comment("if (src_pos < 0) goto bailout;");

      GUARANTEE(src_pos.in_register(), 
                "Not an immediate, so must be in register");
      Register src_pos_reg = src_pos.lo_register();
      cmp(src_pos_reg, zero);
    }

    if (is_set_nth_bit(checks, DST_POS_CHECK)) {
      comment("if (dst_pos < 0) goto bailout;");

      GUARANTEE(dst_pos.in_register(), 
                "Not an immediate, so must be in register");
      Register dst_pos_reg = dst_pos.lo_register();
      Condition cond = is_set_nth_bit(checks, SRC_POS_CHECK) ? ge : al;
      cmp(dst_pos_reg, zero, cond);
    }

    if (is_set_nth_bit(checks, LENGTH_CHECK)) {
      comment("if (length < 0) goto bailout;");
      
      GUARANTEE(length.in_register(), "Not immediate, so must be in register");
      Register length_reg = length.lo_register();
      Condition cond = 
        (is_set_nth_bit(checks, SRC_POS_CHECK) ||
         is_set_nth_bit(checks, DST_POS_CHECK)) ? ge : al;
      cmp(length_reg, zero, cond);
    }

    if (is_set_nth_bit(checks, SRC_POS_CHECK) || 
        is_set_nth_bit(checks, DST_POS_CHECK) ||
        is_set_nth_bit(checks, LENGTH_CHECK)) {
      b(bailout, lt);
    }

    // Complete type check
    if (is_set_nth_bit(checks, SRC_TYPE_CHECK) || 
        is_set_nth_bit(checks, DST_TYPE_CHECK)) {
      GUARANTEE(src_type_reg != no_reg && dst_type_reg != no_reg, "Sanity");

      comment("if (src.klass() != dst.klass()) goto bailout;");
      cmp(src_type_reg, reg(dst_type_reg));
      b(bailout, ne);
      
      RegisterAllocator::dereference(src_type_reg);
      RegisterAllocator::dereference(dst_type_reg);
    }

    // Bound checks
    if (is_set_nth_bit(checks, SRC_BOUND_CHECK) || 
        is_set_nth_bit(checks, DST_BOUND_CHECK)) {
      GUARANTEE((!src_pos.in_register() ||
                 frame()->is_mapping_something(src_pos.lo_register())) &&
                (!dst_pos.in_register() ||
                 frame()->is_mapping_something(dst_pos.lo_register())) &&
                (!length.in_register() ||
                 frame()->is_mapping_something(length.lo_register())),
                "Unmapped values can be damaged");

      if (is_set_nth_bit(checks, SRC_BOUND_CHECK) &&
          is_set_nth_bit(checks, DST_BOUND_CHECK)) {
        comment("if (src.length < src_pos + length || ");
        comment("    dst.length < dst_pos + length) goto bailout;");

        Value src_array_length(T_INT);
        Value dst_array_length(T_INT);
        Value src_sum(T_INT);
        Value dst_sum(T_INT);

        load_from_object(src_array_length, src, Array::length_offset(), 
                         false/*no null check*/ JVM_CHECK_0);
        load_from_object(dst_array_length, dst, Array::length_offset(), 
                         false/*no null check*/ JVM_CHECK_0);
        int_binary(src_sum, src_pos, length, 
                   BytecodeClosure::bin_add JVM_CHECK_0);
        int_binary(dst_sum, dst_pos, length, 
                   BytecodeClosure::bin_add JVM_CHECK_0);
        cmp_values(src_array_length, src_sum);
        cmp_values(dst_array_length, dst_sum, hs);
        b(bailout, lo);
      } else if (is_set_nth_bit(checks, SRC_BOUND_CHECK)) {
        comment("if (src.length < src_pos + length) goto bailout;");

        Value array_length(T_INT);
        Value sum(T_INT);

        load_from_object(array_length, src, Array::length_offset(), 
                         false/*no null check*/ JVM_CHECK_0);
        int_binary(sum, src_pos, length, BytecodeClosure::bin_add JVM_CHECK_0);
        cmp_values(array_length, sum);
        b(bailout, lo);
      } else if (is_set_nth_bit(checks, DST_BOUND_CHECK)) {
        comment("if (dst.length < dst_pos + length) goto bailout;");
        
        Value array_length(T_INT);
        Value sum(T_INT);

        load_from_object(array_length, dst, Array::length_offset(), 
                         false/*no null check*/ JVM_CHECK_0);
        int_binary(sum, dst_pos, length, BytecodeClosure::bin_add JVM_CHECK_0);
        cmp_values(array_length, sum);
        b(bailout, lo);
      } else {
        SHOULD_NOT_REACH_HERE();
      }
    }
  }

  RegisterAllocator::guarantee_all_free();

  const bool succeed = unchecked_arraycopy(array_element_type JVM_CHECK_0);
  // If failed to inline unchecked arraycopy, generate a call to it
  if (!succeed) {
    Method::Raw method = unchecked_arraycopy_method(array_element_type);
    GUARANTEE(method.not_null(), "unchecked arraycopy() not found");
    comment("Invoke unchecked_XXX_arraycopy()");
    invoke(&method, false/*no null checks*/ JVM_CHECK_0);
  }

  if (checks != 0) {
    b(done);
    write_literals();
    bind(bailout);    
    Symbol::Raw null_signature;
    Method::Raw method = 
      Universe::system_class()->lookup_method(Symbols::arraycopy_name(), 
                                              &null_signature);
    GUARANTEE(method.not_null(), "System.arraycopy() not found");

    comment("Bailout: invoke System.arraycopy()");
    VirtualStackFrameContext compile_in_bailout_frame( bailout_frame );
    invoke(&method, false/*no null checks*/ JVM_CHECK_0);
  }

  bind(done);

  return true;
}

bool CodeGenerator::unchecked_arraycopy(BasicType array_element_type 
                                        JVM_TRAPS) {
  GUARANTEE(array_element_type != T_ILLEGAL, "Illegal type");

  // oop_write_barrier stuff seems too heavyweight for inlining
  if (array_element_type == T_OBJECT || array_element_type == T_ARRAY) {
    return false;
  }

  RegisterAllocator::guarantee_all_free();

  Label bailout, done;
  const int element_size = byte_size_for(array_element_type);
  const int log_element_size = exact_log2(element_size);

  bool need_src_ne_dst_check = true;  

  /*
   * We don't want to inline several copy loops for all possible src/dst
   * alignments, as it will lead to significant code bloat.
   * If we cannot derive enough compile-time information to generate 
   * a specialized copy loop, we generate a primitive 2-unrolled copy loop. 
   * It is still faster than memcopy() on small arrays as we save on method
   * invocation, but for longer arrays memcopy() can be faster, especially
   * on byte and char arrays.
   * The solution is to bailout to original arraycopy() method if length
   * is above some platform-dependent limit.
   * Two globals ArrayCopyByteInlineLimit and ArrayCopyShortInlineLimit
   * can be used to tune it for a platform.
   */
  const static int limits[] = 
    { ArrayCopyByteInlineLimit, ArrayCopyShortInlineLimit, -1, -1 };
  const int inline_length_limit = limits[log_element_size];
  bool is_above_length_limit = false;
  bool need_length_limit_check = false;
  VirtualStackFrame* bailout_frame;

  {
    Value src_data(T_OBJECT);
    Value dst_data(T_OBJECT);
    Value length(T_INT);

    bool src_word_aligned = false;
    bool dst_word_aligned = false;
    bool is_backward = false;

    {
      Value src(T_OBJECT);
      Value src_pos(T_INT);
      Value dst(T_OBJECT);
      Value dst_pos(T_INT);
  
      int location = frame()->virtual_stack_pointer();
  
      GUARANTEE(location >= 4, "5 values must be on stack");
      frame()->value_at(length,  location--);
      frame()->value_at(dst_pos, location--);
      frame()->value_at(dst,     location--);
      frame()->value_at(src_pos, location--);
      frame()->value_at(src,     location--);

#ifdef AZZERT
      {
        const jushort src_class_id = src.class_id();
        const jushort dst_class_id = dst.class_id();
        JavaClass::Raw src_class = Universe::class_from_id(src_class_id);
        JavaClass::Raw dst_class = Universe::class_from_id(dst_class_id);
        JavaClass::Raw array_class = array_element_type == T_OBJECT ? 
          Universe::object_array_class()->obj() :
          Universe::as_TypeArrayClass(array_element_type)->obj();
        
        GUARANTEE(src_class_id == 0 ||
                  src_class().is_subtype_of(&array_class),
                  "Source array type inconsistency");
        GUARANTEE(dst_class_id == 0 ||
                  dst_class().is_subtype_of(&array_class),
                  "Destination array type inconsistency");
      }
#endif
      bailout_frame = frame()->clone(JVM_SINGLE_ARG_ZCHECK_0(bailout_frame));

      GUARANTEE(src.in_register() && dst.in_register(),
                "Non-null object values must be in register");
      
      const Register src_reg = src.lo_register();
      const Register dst_reg = dst.lo_register();

      // For copying within the same array we should go either forward or 
      // backward depending on relative offset of src and dst. 
      // We don't generate code for both, instead try determine direction 
      // at compile-time.
      if (src_pos.is_immediate() && dst_pos.is_immediate()) {
        need_src_ne_dst_check = false;

        if (src_pos.as_int() < dst_pos.as_int()) {
          is_backward = true;
        }
      } else if (src_pos.is_immediate() && src_pos.as_int() == 0) {
        need_src_ne_dst_check = false;
        is_backward = true;
      } else if (dst_pos.is_immediate() && dst_pos.as_int() == 0) {
        need_src_ne_dst_check = false;
      }

      if (need_src_ne_dst_check) {
        Label src_ne_dst;
        comment("if (src == dst) && (src_pos < dst_pos) goto bailout;");
        if (src_reg != dst_reg) {
          cmp(src_reg, reg(dst_reg));
          b(src_ne_dst, ne);
        }
        if (src_pos.in_register()) {
          cmp_values(src_pos, dst_pos);
          b(bailout, lt);
        } else {
          GUARANTEE(dst_pos.in_register(), "Must be in register");
          cmp_values(dst_pos, src_pos);
          b(bailout, gt);
        }
        bind(src_ne_dst);
      }

      // Remove arguments from stack to reduce register pressure
      const int virtual_sp = frame()->virtual_stack_pointer();
      frame()->set_virtual_stack_pointer(virtual_sp - /*parameter_size*/5);

      comment("Point at actual data");
      compute_data_start(src_data, src, src_pos, log_element_size, 
                         src_word_aligned);

      compute_data_start(dst_data, dst, dst_pos, log_element_size,
                         dst_word_aligned);
    }

    GUARANTEE(src_data.in_register() && dst_data.in_register(), 
              "Sanity");

    const Register src_data_reg = src_data.lo_register();
    const Register dst_data_reg = dst_data.lo_register();

    RegisterSet reg_set;
    
    // Forcibly allocate 2 registers for copying
    reg_set.add(RegisterAllocator::allocate());
    reg_set.add(RegisterAllocator::allocate());

    {
      Register r = no_reg;
      // Allocate as much as possible without spilling
      while ((r = RegisterAllocator::allocate_or_fail()) != no_reg) {
        reg_set.add(r);
      }
    }

    if (length.is_immediate()) {
      // The code below should not generate anything for zero length copy.
      // It doesn't make sense to optimize for this case as it doesn't happen 
      // in real life applications. We just add a guarantee below.
      AZZERT_ONLY(const int code_offset = code_size();)
      const int length_imm = length.as_int();

      GUARANTEE(length_imm >= 0, "Negative length in unchecked copy");

      const int bytes_to_copy = length_imm << log_element_size;
      const int words_to_copy = bytes_to_copy >> LogBytesPerWord;

      // Cannot use ldm/stm for backward copy or non word-aligned src/dst.
      const bool use_ldm_stm = 
        src_word_aligned && dst_word_aligned && !is_backward;

      // Estimate number of load instructions.
      const int load_count = 
        use_ldm_stm ? (words_to_copy / reg_set.length()) : length_imm;

      if (load_count <= ArrayCopyLoopUnrollingLimit) { 
        if (use_ldm_stm) {
          int i;
          Address4 copy_sets[Assembler::number_of_gp_registers];
          int copy_set_count = 0;

          RegisterSetIterator iter(reg_set);

          while (iter.has_next()) {
            Register r = iter.next();

            Address4 set = Assembler::set(r);
            if (copy_set_count == 0) {
              copy_sets[0] = set;
            } else {
              copy_sets[copy_set_count] = 
                Assembler::join(copy_sets[copy_set_count-1], set);
            }
            copy_set_count++;
          }

          GUARANTEE(copy_set_count == reg_set.length(), "Sanity");
          
          comment("Start copying");          
          for (i = 0; i < words_to_copy / copy_set_count; i++) {
            ldmia(src_data_reg, copy_sets[copy_set_count-1], writeback);
            stmia(dst_data_reg, copy_sets[copy_set_count-1], writeback);
          }
            
          const int words_left = words_to_copy % copy_set_count;
            
          const Register reg = reg_set.get_register();

          if (words_left > 1) {
            ldmia(src_data_reg, copy_sets[words_left-1], writeback);
            stmia(dst_data_reg, copy_sets[words_left-1], writeback);
          } else if (words_left == 1) {
            ldr(reg, imm_index(src_data_reg, BytesPerWord, post_indexed));
            str(reg, imm_index(dst_data_reg, BytesPerWord, post_indexed));
          }
            
          const int bytes_left = 
            bytes_to_copy - (words_to_copy << LogBytesPerWord);
          
          for (i = 0; i < (bytes_left >> LogBytesPerShort); i++) {
            ldrh(reg, imm_index3(src_data_reg, BytesPerShort, post_indexed));
            strh(reg, imm_index3(dst_data_reg, BytesPerShort, post_indexed));
          }
            
          for (i = 0; i < (bytes_left & right_n_bits(LogBytesPerShort)); i++) {
            ldrb(reg, imm_index(src_data_reg, BytesPerByte, post_indexed));
            strb(reg, imm_index(dst_data_reg, BytesPerByte, post_indexed));
          }
        } else {
          // Copy by one element
          if (is_backward) {
            comment("Copy backward");
            if (length_imm > 0) {
              add_imm(src_data_reg, src_data_reg,
                      length_imm << log_element_size);
              add_imm(dst_data_reg, dst_data_reg,
                      length_imm << log_element_size);
            }
          } else {
            comment("Copy forward");
          }

          int el_size = 1 << log_element_size;
          int el_count = length_imm;
          int i;

          // Handle long arrays as int arrays of double length.
          // IMPL_NOTE: consider using LDRD.
          if (log_element_size == LogBytesPerLong) {
            el_size >>= 1;
            el_count <<= 1;
          }

          const Mode mode = is_backward ? pre_indexed : post_indexed;
          const int reg_set_length = reg_set.length();

          for (i = 0; i < el_count / reg_set_length; i++) {
            RegisterSetIterator ld_iter(reg_set);

            while (ld_iter.has_next()) {
              Register reg = ld_iter.next();
              ldrx(reg, src_data_reg, el_size, mode, is_backward);
            }

            RegisterSetIterator st_iter(reg_set);

            while (st_iter.has_next()) {
              Register reg = st_iter.next();
              strx(reg, dst_data_reg, el_size, mode, is_backward);
            }
          }

          {
            RegisterSetIterator ld_iter(reg_set);
            for (i = 0; i < el_count % reg_set_length; i++) {
              GUARANTEE(ld_iter.has_next(), "Must be there");
              Register reg = ld_iter.next();
              ldrx(reg, src_data_reg, el_size, mode, is_backward);
            }

            RegisterSetIterator st_iter(reg_set);
            for (i = 0; i < el_count % reg_set_length; i++) {
              GUARANTEE(st_iter.has_next(), "Must be there");
              Register reg = st_iter.next();
              strx(reg, dst_data_reg, el_size, mode, is_backward);
            }
          }
        }
      } else if (inline_length_limit >= 0 && 
                 length_imm >= inline_length_limit) {
        is_above_length_limit = true;        
      } else {
        // Free up one register to materialize length
        Register reg = reg_set.get_register();
        reg_set.remove(reg);

        mov_imm(reg, length_imm);
        length.set_register(RegisterAllocator::allocate(reg));

        GUARANTEE(length.in_register() && length.lo_register() == reg, 
                  "Sanity");

        GUARANTEE(!reg_set.is_empty(), "Must contain at least one reg");
      }

      GUARANTEE(length_imm > 0 || code_offset == code_size(), 
                "No code generated for zero length")
    }

    if (length.in_register()) {
      if (inline_length_limit >= 0) {
        need_length_limit_check = true;

        CompilerLiteralAccessor cla;
        cmp_imm(length.lo_register(), inline_length_limit, &cla);
        b(bailout, ge);
      }

      Label again;
      // Make a defensive copy of length value
      Value length_copy(T_INT);

      {
        if (!length.in_register() ||
            frame()->is_mapping_something(length.lo_register())) {
           // Free up one register as it can be needed for length_copy      
           Register reg = reg_set.get_register();
           reg_set.remove(reg);
           length_copy.assign_register();
           GUARANTEE(length_copy.lo_register() == reg, "Wrong register");
        } else {
           length.copy(length_copy);
        }
      }

      // make sure there are at least two registers in the set
      while (reg_set.length() < 2) {
        Register r = RegisterAllocator::allocate();
        reg_set.add(r);
      }

      GUARANTEE(reg_set.length() >= 2, 
                "Must have at least 2 registers for copying");

      const Register orig_length_reg = length.lo_register();
      const Register length_reg = length_copy.lo_register();

      sub(length_reg, orig_length_reg, one, set_CC);
      comment("if (length == 0) goto done;");
      b(done, lt);

      if (is_backward) {
        comment("Copy backward - start from the tail");
        add(src_data_reg, src_data_reg, 
            imm_shift(length_reg, lsl, log_element_size));
        add(dst_data_reg, dst_data_reg, 
            imm_shift(length_reg, lsl, log_element_size));
      } else {
        comment("Copy forward");
      }

      GUARANTEE(reg_set.length() >= 2, "Two registers for copying");

      // Take 2 registers from the set.
      RegisterSetIterator iter(reg_set);
      const Register reg1 = iter.next();
      const Register reg2 = iter.next();

      bind(again);
      // Unroll 2 times
      if (log_element_size <= LogBytesPerWord) { 
        const int el_size = 1 << log_element_size;
        ldrx(reg1, src_data_reg, el_size, post_indexed, is_backward, always);
        ldrx(reg2, src_data_reg, el_size, post_indexed, is_backward, gt);
        strx(reg1, dst_data_reg, el_size, post_indexed, is_backward, always);
        strx(reg2, dst_data_reg, el_size, post_indexed, is_backward, gt);
      } else {
        // Can use multiple load/store for double-word arrays
        GUARANTEE(log_element_size == LogBytesPerLong, "Sanity");

        Address4 set1 = join(set(reg1), set(reg2));
        ldmxa(src_data_reg, set1, writeback, is_backward, always);
        stmxa(dst_data_reg, set1, writeback, is_backward, always);
        ldmxa(src_data_reg, set1, writeback, is_backward, gt);
        stmxa(dst_data_reg, set1, writeback, is_backward, gt);
      }
      sub(length_reg, length_reg, imm(2), set_CC);
      b(again, ge);
    }
  }

  RegisterAllocator::guarantee_all_free();

  const bool need_bailout = 
    need_src_ne_dst_check || need_length_limit_check || is_above_length_limit;

  if (need_bailout) {
    /* If we know we are above the limit, we just fall through to invoke */
    if (!is_above_length_limit) {
      b(done);
      write_literals();
    }
    bind(bailout);

    comment("Bailout: invoke unchecked_XXX_arraycopy()");
    VirtualStackFrameContext compile_in_bailout_frame( bailout_frame );

    UsingFastOops fast_oops;
    Method::Fast method = unchecked_arraycopy_method(array_element_type);
    GUARANTEE(method.not_null(), "unchecked arraycopy() not found");
    invoke(&method, false/*no null checks*/ JVM_CHECK_0);
  }

  bind(done);

  return true;
}

ReturnOop CodeGenerator::unchecked_arraycopy_method(BasicType type) {
  Symbol::Raw method_name;

  switch (type) {
  case T_BOOLEAN:
  case T_BYTE:
    method_name = Symbols::unchecked_byte_arraycopy_name();
    break;
  case T_CHAR:
  case T_SHORT:
    method_name = Symbols::unchecked_char_arraycopy_name();
    break;
  case T_INT:
  case T_FLOAT:
    method_name = Symbols::unchecked_int_arraycopy_name();
    break;
  case T_LONG:
  case T_DOUBLE:
    method_name = Symbols::unchecked_long_arraycopy_name();
    break;
  case T_OBJECT:
  case T_ARRAY:
    method_name = Symbols::unchecked_obj_arraycopy_name();
    break;
  default:
    SHOULD_NOT_REACH_HERE();
    break;
  }

  GUARANTEE(method_name.not_null(), "No entry for a type");

  Symbol::Raw null_signature;
  Method::Raw method = Universe::jvm_class()->lookup_method(&method_name, 
                                                            &null_signature);

  GUARANTEE(method.not_null(), "Method not found");

  return method.obj();
}

/*
 * Helper function to compute data start from offset and element size.
 */
void CodeGenerator::compute_data_start(Value& data, 
                                       Value& array, Value& offset, 
                                       int log_element_size,
                                       bool& offset_word_aligned) {
  GUARANTEE(array.in_register(), "Non-null value must be in register");

  data.assign_register();

  Register array_reg = array.lo_register();
  Register data_reg = data.lo_register();

  if (offset.is_immediate()) {
    int offset_imm = offset.as_int();
    GUARANTEE(offset_imm >= 0, "Negative offset");

    add(data_reg, array_reg, imm(Array::base_offset() + 
                                 (offset_imm << log_element_size)));

    offset_word_aligned = 
      ((offset_imm << log_element_size) & right_n_bits(BytesPerWord)) == 0;
  } else {
    GUARANTEE(offset.in_register(), "Must be in register");

    Register offset_reg = offset.lo_register();

    add(data_reg, array_reg, imm(Array::base_offset()));
    add(data_reg, data_reg, 
        imm_shift(offset_reg, lsl, log_element_size));

    // Conservatively assume unaligned offset
    offset_word_aligned = (log_element_size >= LogBytesPerWord);
  }
}

void CodeGenerator::ldrx(Register rd, Register rn, int element_size, 
                         Mode mode, bool is_backward, Condition cond) {
  const int offset = is_backward ? -element_size : element_size;
  
  switch (element_size) {
    case BytesPerByte: 
      ldrb(rd, imm_index(rn, offset, mode), cond); 
      break;
    case BytesPerShort:
      ldrh(rd, imm_index3(rn, offset, mode), cond);
      break;
    case BytesPerInt:
      ldr(rd, imm_index(rn, offset, mode), cond);
      break;
    default:
      // For now we handle double-word arrays as word arrays of double size
      SHOULD_NOT_REACH_HERE(); 
      break;
  }
}

void CodeGenerator::strx(Register rd, Register rn, int element_size, 
                         Mode mode, bool is_backward, Condition cond) {
  const int offset = is_backward ? -element_size : element_size;
  
  switch (element_size) {
    case BytesPerByte: 
      strb(rd, imm_index(rn, offset, mode), cond); 
      break;
    case BytesPerShort:
      strh(rd, imm_index3(rn, offset, mode), cond);
      break;
    case BytesPerInt:
      str(rd, imm_index(rn, offset, mode), cond);
      break;
    default:
      // For now we handle double-word arrays as word arrays of double size
      SHOULD_NOT_REACH_HERE(); 
      break;
  }
}

void CodeGenerator::ldmxa(Register rn, Address4 reg_set, WritebackMode mode, 
                          bool is_backward, Condition cond) {
  if (is_backward) {
    ldmda(rn, reg_set, writeback, cond);
  } else {
    ldmia(rn, reg_set, writeback, cond);
  }
}
 
void CodeGenerator::stmxa(Register rn, Address4 reg_set, WritebackMode mode, 
                          bool is_backward, Condition cond) {
  if (is_backward) {
    stmda(rn, reg_set, writeback, cond);
  } else {
    stmia(rn, reg_set, writeback, cond);
  }
}

CodeGenerator::RegisterSet::~RegisterSet() {
  int r = 0;
  while (_set != 0) {
    if (_set & 1) {
      Register reg = (Register)r;
      GUARANTEE(RegisterAllocator::references(reg) == 1, 
                "Must be only one reference");
      RegisterAllocator::dereference(reg);
    }
    _set >>= 1;
    r++;
  }
}

void CodeGenerator::RegisterSet::add(Register reg) {
  GUARANTEE(_length + 1 < Assembler::number_of_gp_registers, 
            "Bounds check");
  GUARANTEE((_set & (1 << reg)) == 0, "Already in set");
  GUARANTEE(RegisterAllocator::references(reg) == 1, "Must be referenced");
  _set |= 1 << reg;
  _length++;
}

void CodeGenerator::RegisterSet::remove(Register reg) {
  GUARANTEE((_set & (1 << reg)) != 0, "Not in set");
  GUARANTEE(RegisterAllocator::references(reg) == 1, "Must be referenced");
  _set &= ~(1 << reg);
  _length--;
  RegisterAllocator::dereference(reg);
}

Assembler::Register CodeGenerator::RegisterSet::get_register() const {
  return RegisterSetIterator(*this).next();
}

Assembler::Register CodeGenerator::RegisterSetIterator::next() {
  GUARANTEE(has_next(), "No next");
  while ((_set & 1) == 0) {
    _set >>= 1;
    _index++;
    GUARANTEE(_index < Assembler::number_of_gp_registers, "Sanity");
  }
  Register reg = (Register)_index;
  _set >>= 1;
  _index++;
  GUARANTEE(RegisterAllocator::references(reg) == 1, 
            "Must be one reference");
  return reg;
}

#endif

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

#if USE_AOT_COMPILATION && !ENABLE_ISOLATES

void CodeGenerator::initialize_class(InstanceClass* klass JVM_TRAPS) {
  GUARANTEE(klass->not_null() && !klass->is_initialized(),
            "Should only be called for non-initialized classes");
#ifdef AZZERT
  InstanceClass holder = method()->holder();
  GUARANTEE(!holder.is_subclass_of(klass),
            "Superclass of the holder must be already initialized");
#endif
  // initialize_class(Thread&, raw_class);
  COMPILER_COMMENT(("Initialize class if needed"));
  COMPILER_COMMENT(("Flush frame"));
  flush_frame(JVM_SINGLE_ARG_CHECK);

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

void CodeGenerator::bytecode_prolog() {
  // IMPL_NOTE: delta is used to make sure that it's not a time 
  // to write literals. This is required to prevent sequence of 
  // simple bytecodes without dumping literal pools.
  enum {delta = 0x100};
  write_literals_if_desperate(delta);
}

void CodeGenerator::flush_epilogue(JVM_SINGLE_ARG_TRAPS) {
  if (desperately_need_to_force_literals()) {
    Compiler::abort_active_compilation(true JVM_THROW);
  }
}

#endif
#endif /*#if !ENABLE_THUMB_COMPILER*/
