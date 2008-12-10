/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

# include "incls/_precompiled.incl"
# include "incls/_CodeGenerator_thumb2.cpp.incl"

#if ENABLE_COMPILER

#include <math.h>

class TempRegister {
private:
  const Assembler::Register _reg;
public:
  TempRegister( void ):
    _reg( RegisterAllocator::allocate() ) {
  }
  TempRegister(const Assembler::Register reg): _reg(reg) {
    RegisterAllocator::allocate(reg);
  }
 ~TempRegister( void ) {
    RegisterAllocator::dereference(_reg);
  }

  operator Assembler::Register( void ) const { return _reg; }
  Assembler::Register reg     ( void ) const { return _reg; }
};

#if ENABLE_ARM_VFP
class TempVFPRegister {
private:
  const Assembler::Register _reg;
public:
  TempVFPRegister( void ):
    _reg( RegisterAllocator::allocate_float_register() ) {
  }
 ~TempVFPRegister( void ) {
    RegisterAllocator::dereference(_reg);
  }
  operator Assembler::Register( void ) const { return _reg; }
  Assembler::Register reg     ( void ) const { return _reg; }
};
#endif

class CompilerLiteralAccessor : public LiteralAccessor {
public:
  virtual bool has_literal(int imm32, Assembler::Register& result) const;

  virtual Assembler::Register get_literal(int imm32) const {
    return frame()->get_literal(imm32, *this);
  }
#if ENABLE_ARM_VFP
  Assembler::Register has_vfp_literal( const int imm32 ) const;
#endif

  CompilerLiteralAccessor( void ): _frame( CodeGenerator::current()->frame() ) {}
private:
  VirtualStackFrame* const _frame;
  VirtualStackFrame* frame( void ) const { return _frame; }
};

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
                                     bool needs_cib JVM_TRAPS){
  write_literals_if_desperate();

  {
    UsingFastOops fast_oops;
    JavaClass::Fast jc = klass;
    statics_holder.assign_register();
    get_mirror_list_base(statics_holder.lo_register());
    ldr(statics_holder.lo_register(),
        statics_holder.lo_register(),
        (int)jc().class_id() * sizeof(OopDesc *));
  }
  if (needs_cib){
    Label class_is_initialized, need_init;
    // Can we make the flush conditional for  get/put static ?
    //  see if register usage cross compiled bytecode.
    frame()->flush(JVM_SINGLE_ARG_CHECK);
    // check for no task mirror at all.
    tst(statics_holder.lo_register(), statics_holder.lo_register());
    b(need_init, eq);
    if (GenerateROMImage) {
      // The marker cannot be treated as a constant value, as it would break
      // cross-compilation. Thus we load it from GP table.
      const TempRegister tmp;
      get_task_class_init_marker(tmp);
      cmp(statics_holder.lo_register(), reg(tmp));
    } else {
      // The marker is at the start of the heap or in ROM text, so it can be
      // treated as a constant value for the cib test.
      cmp(statics_holder.lo_register(), (int)_task_class_init_marker);
    }
    b(class_is_initialized, ne);
bind(need_init);
    // Call the runtime system.
    // pass klass as extra args, move in correct register beforehand
    // if necessary
    // Passing the klass in parameter.
    {
      // KEEP these brackets: without them the klass_parameter's destructor would
      // not be called before call_vm and cause an error.
      Value klass_parameter(T_OBJECT);
      klass_parameter.set_obj(klass);
      if (klass_parameter.lo_register() != r1) {
        call_vm_extra_arg(klass_parameter.lo_register());
      }
    }
    call_vm((address) compiled_code_task_barrier, T_OBJECT JVM_CHECK);
    // Need to move mirror to expected register
    if (statics_holder.lo_register()!= r0){
      mov(statics_holder.lo_register(), r0);
    }
bind(class_is_initialized);
  }
}

void CodeGenerator::check_cib(Oop *klass JVM_TRAPS){
  Label class_is_initialized, need_init;
  // IMPL_NOTE: Cannot make the flush conditionally.
  //  see how this can be made conditional!
  frame()->flush(JVM_SINGLE_ARG_CHECK);

  // add to the klass oop to get the address of the appropriate
  // task mirror table entry
  const TempRegister task_mirror;
  Value klass_value(T_OBJECT);
  klass_value.set_obj(klass);
  {
    get_mirror_list_base(task_mirror);
    JavaClass::Raw jc = klass;
    ldr(task_mirror, task_mirror, (int)jc().class_id() * sizeof(OopDesc*));
  }

  {
    // check for no task mirror at all.
    tst(task_mirror, task_mirror);
    b(need_init, eq);
    if (GenerateROMImage) {
      // The marker cannot be treated as a constant value, as it would break
      // cross-compilation. Thus we load it from GP table.
      const TempRegister tmp;
      get_task_class_init_marker(tmp);
      cmp(task_mirror, reg(tmp));
    } else {
      // The marker is at the start of the heap or in ROM text, so it can be
      // treated as a constant value for the cib test.
      cmp(task_mirror, (int)_task_class_init_marker);
    }
    b(class_is_initialized, ne);
  }
bind(need_init);
  // Call the runtime system.
  // pass klass as extra args
  // frame()->flush();
  if (klass_value.lo_register() != r1){
    call_vm_extra_arg(klass_value.lo_register());
  }
  klass_value.destroy();
  call_vm((address) compiled_code_task_barrier, T_OBJECT JVM_CHECK);
  bind(class_is_initialized);
}

#endif

#if ENABLE_INLINED_ARRAYCOPY
bool CodeGenerator::arraycopy(JVM_SINGLE_ARG_TRAPS) {
  return false;
}

bool CodeGenerator::unchecked_arraycopy(BasicType type JVM_TRAPS) {
  return false;
}
#endif

void CodeGenerator::bytecode_prolog() {
}

void CodeGenerator::flush_epilogue(JVM_SINGLE_ARG_TRAPS) {
}

extern "C" {
  extern address gp_base_label;
  extern address gp_shared_call_vm_ptr;
  extern address gp_shared_call_vm_oop_ptr;
  extern address gp_shared_call_vm_exception_ptr;
  extern address gp_compiler_new_object_ptr;
  extern address gp_compiler_new_type_array_ptr;
  extern address gp_compiler_new_obj_array_ptr;
  extern address gp_shared_monitor_enter_ptr;
  extern address gp_shared_monitor_exit_ptr;
  extern address gp_compiler_idiv_irem_ptr;
  extern address gp_shared_lock_synchronized_method_ptr;
  extern address gp_shared_unlock_synchronized_method_ptr;
  extern address gp_compiler_throw_NullPointerException_ptr;
  extern address gp_compiler_throw_ArrayIndexOutOfBoundsException_ptr;
}


bool CompilerLiteralAccessor::has_literal(int imm32,
                                          Assembler::Register& result) const {
  for( LiteralElementStream les( frame() ); !les.eos(); les.next() ) {
    if( les.value() == imm32) {
      const Assembler::Register reg( les.reg() );
#if ENABLE_ARM_VFP
      if( reg > Assembler::r15 ) {
        continue;
      }
#endif
      result = reg;
      return true;
    }
  }
  return false;
}

#if ENABLE_ARM_VFP
Assembler::Register
CompilerLiteralAccessor::has_vfp_literal( const int imm32 ) const {
  for( LiteralElementStream les(frame()); !les.eos() ; les.next() ) {
    if( les.value() == imm32 ) {
      return les.reg();
    }
  }
  return Assembler::no_reg;
}
#endif

void CodeGenerator::load_from_address(Value& result, BasicType type,
                                      MemoryAddress& address, Condition cond) {

  GUARANTEE(cond == al, "load_from_address: not conditionally executable");

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
  if (lo >= s0) {
    const Assembler::Address5 field_addr = address.lo_address_5();
    switch( type ) {
      case T_FLOAT:
        flds(lo, field_addr);
        return;
      case T_DOUBLE:
        fldd(lo, field_addr);
        return;
      default:
        SHOULD_NOT_REACH_HERE();
    }
  }
#endif

  Assembler::Register address_reg;
  int address_offset = 0;
  address.get_indexed_address(true, address_reg, address_offset);

  switch(type) {
    case T_BOOLEAN : // fall through
    case T_BYTE    :
      ldrsb_imm12_w(lo, address_reg, address_offset);
      break;
    case T_CHAR    :
      ldrh(lo, address_reg, address_offset);
      break;
    case T_SHORT   :
      ldrsh_imm12_w(lo, address_reg, address_offset);
      break;
    case T_INT     : // fall through
#if !ENABLE_ARM_VFP
    case T_FLOAT   : // fall through
#endif
    case T_ARRAY   : // fall through
    case T_OBJECT  :
      // load data at lo address
      ldr(lo, address_reg, address_offset);
      break;
    case T_LONG    : // fall through
#if !ENABLE_ARM_VFP
    case T_DOUBLE  :
#endif
      // load data at lo address
      ldr(lo, address_reg, address_offset);
      // prepare and load data at hi address
      address.get_indexed_address(false, address_reg, address_offset);
      ldr(result.hi_register(), address_reg, address_offset);
      break;
    default        :
      SHOULD_NOT_REACH_HERE();
      break;
  }
}

void CodeGenerator::store_to_address(Value& value, BasicType type,
                                     MemoryAddress& address)
{
  // if the value to store isn't present there nothing left to do
  if (!value.is_present()) {
    return;
  }

  enum {max_codesize = 34};
  write_literals_if_desperate(max_codesize);

  GUARANTEE(stack_type_for(type) == value.stack_type(),
             "types must match (taking stack types into account)");
  Register reg;
  const CompilerLiteralAccessor cla;

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

  Assembler::Register address_reg;
  int address_offset = 0;

  switch(type) {
    case T_BOOLEAN : // fall through
    case T_BYTE    :
      // prepare and load data at lo address
      address.get_indexed_address(true, address_reg, address_offset);
      strb(reg, address_reg, address_offset);
      break;

    case T_CHAR    : // fall through
    case T_SHORT   :
      // prepare and load data at lo address
      address.get_indexed_address(true, address_reg, address_offset);
      strh(reg, address_reg, address_offset);
      break;

    case T_ARRAY   :
    case T_OBJECT  :
      GUARANTEE(value.in_register() || value.must_be_null(),
                "Only NULLs can be immediate");
      if (!value.not_on_heap()) {
        // Need to do pointer setting
        address.write_barrier_prolog();
        // prepare and load data at lo address
        address.get_preindexed_address(true, address_reg, address_offset);
        str(reg, address_reg, address_offset);
        if (value.is_immediate()) {
          RegisterAllocator::dereference(reg);
          reg = no_reg;
        }
        // Free up register that's not needed any more.  Pointer setting
        // uses up lots of registers, and we want to minimize pressure
        value.destroy();

        address.write_barrier_epilog();
        break;
      } // Fall through

    case T_FLOAT   :
#if ENABLE_ARM_VFP
      if(reg >= s0) {
        fsts(reg, address.lo_address_5());
        break;
      } // Fall through
#endif
    case T_INT     :
      // prepare and load data at lo address
      address.get_indexed_address(true, address_reg, address_offset);
      str(reg, address_reg, address_offset);
      break;

    case T_DOUBLE  :
#if ENABLE_ARM_VFP
      if (!value.is_immediate() && reg >= s0) {
        fstd(reg, address.lo_address_5());
        break;
      } // Fall through
#endif
    case T_LONG    :
#if ENABLE_ARM_VFP
      if(reg >= s0) {
        fsts(reg, address.lo_address_5());
      } else
#endif
      {
        // prepare and load data at lo address
        address.get_indexed_address(true, address_reg, address_offset);
        str(reg, address_reg, address_offset);
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
        // prepare and load data at *hi* address
        address.get_indexed_address(false, address_reg, address_offset);
        str(reg, address_reg, address_offset);
      }
      break;
    default        :
      SHOULD_NOT_REACH_HERE();
      break;
  }
  if (reg != no_reg && value.is_immediate()) {
    RegisterAllocator::dereference(reg);
  }
}

#if ENABLE_ARM_VFP
void CodeGenerator::move_vfp_immediate(const Register dst, const jint src) {
  const CompilerLiteralAccessor cla;
  {
    const Assembler::Register reg = cla.has_vfp_literal(src);
    if( reg != Assembler::no_reg ) {
      if (reg > Assembler::r15) {
        fcpys(dst, reg);
      } else {
        fmsr(dst, reg);
      }
      return;
    }
  }

  if( src >= 0 && has_room_for_imm( abs(src), 8) ) {
    const TempRegister tmp;
    if( tmp < r8 ) {
      mov_imm8(tmp, src);
    } else {
      mov_imm12_w(tmp, Imm12(src));
    }
    fmsr(dst, tmp);
    set_has_literal_value(tmp, src);
    return;
  }

  {
    const Imm12 modified_imm12 = try_modified_imm12(src);
    if( modified_imm12 != invalid_imm12 ) {
      const TempRegister tmp;
      mov_imm12_w(tmp, modified_imm12);
      fmsr(dst, tmp);
      set_has_literal_value(tmp, src);
      return;
    }
  }

  {
    const Imm12 modified_imm12 = try_modified_imm12(~src);
    if( modified_imm12 != invalid_imm12 ) {
      const TempRegister tmp;
      mvn_imm12_w(tmp, modified_imm12);
      fmsr(dst, tmp);
      set_has_literal_value(tmp, src);
      return;
    }
  }
  ldr_literal(dst, NULL, src);
}

inline void
CodeGenerator::move_float_immediate(const Register dst, const jint src) {
  if( src == 0 ) {
    Register reg = frame()->find_zero();
    if( reg == Assembler::no_reg ) {
      reg = frame()->find_non_NaN();
      if (is_vfp_register(reg)) {
        fsubs(dst, reg, reg);
      } else {
        if (reg == Assembler::no_reg) {
          const TempRegister tmp;
          mov(tmp, 0);
          fmsr(dst, tmp);
          set_has_literal_value(tmp, 0);
        } else {
          fmsr(dst, reg);
          fsubs(dst, dst, dst);
        }
      }
    } else if( is_vfp_register(reg) ) {
      fcpys(dst, reg);
    } else {
      fmsr(dst, reg);
    }
  } else {
    move_vfp_immediate(dst, src);
  }
}

inline void
CodeGenerator::move_double_immediate(const Register dst,
                                     const jint src_lo, const jint src_hi) {
  const Register dst_lo = dst;
  const Register dst_hi = Register(dst + 1);

  if (src_lo == 0 && src_hi == 0) {
    Register zero_reg = frame()->find_zero();
    if (zero_reg != Assembler::no_reg) {
      if (is_vfp_register(zero_reg)) {
        fcvtds(dst_lo, zero_reg);
      } else {
        fmdrr(dst_lo, zero_reg, zero_reg);
      }
      return;
    }
    Register double_reg = frame()->find_double_non_NaN();
    if (double_reg != Assembler::no_reg) {
      fsubd(dst_lo, double_reg, double_reg);
      return;
    }
    Register non_nan_reg = frame()->find_non_NaN();
    if (non_nan_reg != Assembler::no_reg) {
      if (is_vfp_register(non_nan_reg)) {
        fsubs(dst_lo, non_nan_reg, non_nan_reg);
        fcpys(dst_hi, dst_lo);
      } else {
        fmdrr(dst_lo, non_nan_reg, non_nan_reg);
        fsubd(dst_lo, dst_lo, dst_lo);
      }
      return;
    }
    const TempRegister tmp;
    mov(tmp, 0);
    fmdrr(dst_lo, tmp, tmp);
    set_has_literal_value(tmp, 0);
    return;
  }
  {
    const Register reg = frame()->find_double_vfp_literal(src_lo, src_hi);
    if (reg != Assembler::no_reg) {
      fcpyd(dst_lo, reg);
      return;
    }
  }
  {
    const CompilerLiteralAccessor cla;
    const Register reg_lo = cla.has_vfp_literal(src_lo);
    const Register reg_hi = cla.has_vfp_literal(src_hi);
    if (is_arm_register(reg_lo) && is_arm_register(reg_hi)) {
      fmdrr(dst_lo, reg_lo, reg_hi);
      return;
    }
    if (is_vfp_register(reg_hi)) {
      fcpys(dst_hi, reg_hi);
      if (src_lo == 0 && VirtualStackFrame::is_non_NaN(src_hi)) {
        fsubs(dst_lo, dst_hi, dst_hi);
        return;
      }
      if (is_arm_register(reg_lo)) {
        fmsr(dst_lo, reg_lo);
        return;
      }
      move_vfp_immediate(dst_lo, src_lo);
      return;
    }

    if (is_vfp_register(reg_lo)) {
      fcpys(dst_lo, reg_lo);
      if (src_hi == 0 && VirtualStackFrame::is_non_NaN(src_lo)) {
        fsubs(dst_hi, dst_lo, dst_lo);
        return;
      }
      if (is_arm_register(reg_hi)) {
        fmsr(dst_hi, reg_hi);
        return;
      }
      move_vfp_immediate(dst_hi, src_hi);
      return;
    }
    move_vfp_immediate(dst_lo, src_lo);
    if (src_lo == src_hi) {
      fcpys(dst_hi, dst_lo);
    } else {
      move_vfp_immediate(dst_hi, src_hi);
    }
  }
}
#endif // ENABLE_ARM_VFP

void
CodeGenerator::move(const Value& dst, const Value& src, const Condition cond) {
  // On thumb, condition is expected to be "always"
  GUARANTEE(cond == al, "move(Value, Value, cond) : cannot use condition");

  // if the source isn't present there's nothing left to do
  if (!src.is_present()) {
    return;
  }

  GUARANTEE(dst.type() == src.type(), "type check");
  GUARANTEE(dst.in_register(), "destination must be in register");

  const Register dst_lo_reg = dst.lo_register();
  if (src.is_immediate()) {
    const int src_lo_bits = src.lo_bits();
#if ENABLE_ARM_VFP
    const BasicType type = dst.type();
    if( type == T_FLOAT ) {
      move_float_immediate( dst_lo_reg, src_lo_bits );
    } else if( type == T_DOUBLE ) {
      move_double_immediate( dst_lo_reg, src_lo_bits, src.hi_bits() );
    } else
#endif
    {
      mov( dst_lo_reg, src_lo_bits );
      if (dst.is_two_word()) {
        mov( dst.hi_register(), src.hi_bits());
      }
    }
    set_has_literal_value( dst.lo_register(), src.lo_bits() );
    if(dst.is_two_word()) {
      set_has_literal_value( dst.hi_register(), src.hi_bits() );
    }
  } else {
    GUARANTEE(src.in_register(), "source must be in register");
    mov(dst_lo_reg, src.lo_register());
    if( dst.is_two_word() ) {
      mov( dst.hi_register(), src.hi_register() );
    }
  }
}

void CodeGenerator::move(Value& dst, Oop* obj, Condition cond) {
  GUARANTEE(dst.type() == T_OBJECT || dst.type() == T_ARRAY, "type check");
  ldr_oop(dst.lo_register(), obj, cond);
}

void CodeGenerator::move(Assembler::Register dst, Assembler::Register src,
                         Condition cond) {
  it(cond);
#if ENABLE_ARM_VFP
  // Does not support Doubles reg moves
  if (dst >= s0) {
    if (src >= s0) {
      fcpys(dst, src);
    } else {
      fmsr(dst, src);
    }
  } else if (src >= s0) {
    fmrs(dst, src);
  } else
#endif
  {
    mov(dst, src);
  }
}

void CodeGenerator::array_check(Value& array, Value& index JVM_TRAPS) {
  write_literals_if_desperate();

  const bool null_check = need_null_check(array);

  const bool use_null_pointer_handler =
    ENABLE_ARM_V7 && UseHandlers && null_check &&
    is_inline_exception_allowed(ThrowExceptionStub::rte_null_pointer JVM_CHECK);

  const bool use_array_index_out_of_bounds_handler =
    ENABLE_ARM_V7 && UseHandlers &&
    is_inline_exception_allowed(ThrowExceptionStub::rte_array_index_out_of_bounds JVM_CHECK);

  const TempRegister length;
  if (null_check) {
    if( !use_null_pointer_handler ) {
      cmp(array.lo_register(), zero);
      NullCheckStub* null_check_stub =
        NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(null_check_stub));
      b(null_check_stub, eq);
    }
    frame()->set_value_must_be_nonnull(array);
    ldr(length, array.lo_register(), Array::length_offset());
  } else {
    ldr(length, array.lo_register(), Array::length_offset());
  }

#if ENABLE_ARM_V7
  if (use_array_index_out_of_bounds_handler) {
    if (index.is_immediate()) {
      const TempRegister index_reg;
      mov(index_reg, index.as_int());
      chka(length, index_reg);
    } else {
      chka(length, reg(index.lo_register()));
    }
  } else
#endif
  {
    if (index.is_immediate()) {
      // IMPL_NOTE: need revisit
      // CompilerLiteralAccessor cla;
      cmp(length, index.as_int());
    } else {
      cmp(length, reg(index.lo_register()));
    }
    IndexCheckStub* index_check_stub =
      IndexCheckStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(index_check_stub));
    b(index_check_stub, ls);
  }
}

// Note: We cannot bailout in null_check, as null_check is used in
// combination with other instructions.
// This means that we do not have enough data to reconstruct the virtual
// stack frame for the  uncommon trap
void CodeGenerator::null_check(const Value& object JVM_TRAPS) {
  const bool could_use_xenon_features = ENABLE_ARM_V7 &&
    is_inline_exception_allowed(ThrowExceptionStub::rte_null_pointer JVM_CHECK);
  if (!could_use_xenon_features) {
    NullCheckStub* check_stub =
      NullCheckStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(check_stub));
    if (object.must_be_null()) {
      b(check_stub);
    } else {
      cmp(object.lo_register(), zero);
      b(check_stub, eq);
    }
  } else {
    if (object.must_be_null()) {
      mov(r0, imm12(method()->max_locals()));
      ldr_using_gp(pc, "compiler_throw_NullPointerException");
    } else {
      const TempRegister temp_reg;
      ldr(temp_reg, object.lo_register(), 0);
    }
  }
}

void CodeGenerator::overflow(const Assembler::Register& stack_pointer,
                             const Assembler::Register& method_pointer) {
  GUARANTEE(!ENABLE_ARM_V7 || !UseHandlers, "Should not reach here");
  GUARANTEE(method_pointer == Assembler::r0, "Method in wrong register");
  GUARANTEE(stack_pointer == Assembler::r1, "Stack in wrong register");
  ldr_address(r3, (address)interpreter_method_entry);
  bx(r3);
}

void CodeGenerator::method_entry(Method* method JVM_TRAPS) {
  if (method->max_execution_stack_count() > 60) {
    // IMPL_NOTE: this currently generates callinfo that are too large for
    // the trampoline jumping code
    Compiler::abort_active_compilation(true JVM_THROW);
  }

  // r0 keeps the method. Add a reference to prevent it from being used as a
  // scratch register.
  RegisterAllocator::reference(r0);
  // lr keeps return address.
  RegisterAllocator::reference(lr);

  bool need_stack_overflow_check = true;
  if (method->is_native() || !method->access_flags().has_invoke_bytecodes()) {
    int stack_needed =
      (method->max_execution_stack_count() * BytesPerStackElement) +
      JavaFrame::frame_desc_size();
    if (stack_needed < LeafMethodStackPadding) {
      // We're sure this method won't cause a stack overflow.
      need_stack_overflow_check = false;
    }
  }

  const int SignedBytesPerStackElement =
      JavaStackDirection * BytesPerStackElement;
  int caller_jsp_shift =
        method->size_of_parameters() * SignedBytesPerStackElement;
  int locals_and_frame =
      method->max_locals()*BytesPerStackElement + JavaFrame::frame_desc_size();
  int adjustment = JavaStackDirection*locals_and_frame - caller_jsp_shift;
#if ENABLE_ARM_V7
  // method_prolog_handler cannot be used with Method Traps because
  // it thinks that method's execution entry always points to HB instruction
  if (UseHandlers && !ENABLE_METHOD_TRAPS) {
    // IMPL_NOTE: we could afoid checking stack overflow sometimes
    if (ObjectHeap::contains_moveable(method->obj()) && !GenerateROMImage) {
      hb(method_prolog_handler_heap_ex_sens_update);
    } else {
      if (GenerateROMImage) {
        hb(method_prolog_handler_rom);
      } else {
        hb(method_prolog_handler_rom_ex_sens_update);
      }
    }
  } else
#endif
  {
    if (!GenerateROMImage) {
      NOT_PRODUCT(comment("update method_execution_sensor"));
      strb_imm12_w(gp, gp,
        address(_method_execution_sensor) - address(&gp_base_label));
    }

    Register return_reg = lr;
    if (need_stack_overflow_check) {
#if ENABLE_ARM_V7
      if (UseHandlers) {
        return_reg = r7; // any register not used by stack_overflow_handler
        mov_hi(return_reg, lr);
        const int stack_count = method->max_execution_stack_count();
        hbl_with_parameter(check_stack_overflow_handler, stack_count);
      } else
#endif
      {
        check_stack_overflow(method JVM_CHECK);
      }
    }

    // Save space for the locals and the first half of the frame
#if ENABLE_ARM_V7
    if (adjustment >= 0) {
      add(jsp, jsp, imm12(adjustment));
    } else {
      sub(jsp, jsp, imm12(-adjustment));
    }

    str_jsp(fp, JavaFrame::caller_fp_offset()
                                     - JavaFrame::empty_stack_offset());
    str_jsp(return_reg, JavaFrame::return_address_offset()
                                     - JavaFrame::empty_stack_offset());

    const int empty_stack_offset = JavaFrame::empty_stack_offset();
    if (empty_stack_offset >= 0) {
      sub(fp, jsp, imm12(empty_stack_offset));
    } else {
      add(fp, jsp, imm12(-empty_stack_offset));
    }
#else
    add(jsp, jsp, adjustment);

    // The new fp will be at jsp - JavaFrame::empty_stack_offset().  We need to
    // save the old value of fp before setting the new one
    str_jsp(fp, JavaFrame::caller_fp_offset() - JavaFrame::empty_stack_offset());
    str_jsp(return_reg, JavaFrame::return_address_offset() -
                        JavaFrame::empty_stack_offset());
    // Return address is stored in the frame.
    sub(fp, jsp, JavaFrame::empty_stack_offset());
#endif
  }

  RegisterAllocator::dereference(lr);

  if (UseProfiler) {
    // Instead of calling
    //    ldr_oop(r0, compiled_method());
    // we calculate compiled_method() as an offset from the current pc.
    sub(r0, pc, (code_end_offset() + 2 * BytesPerWord));
    str(r0, fp, JavaFrame::method_offset());
  }

  if (method->access_flags().is_synchronized()) {
    if (method->access_flags().is_static()) {
      UsingFastOops fast_oops;
      // Get the class mirror object.
#if ENABLE_ISOLATES
      const TempRegister task_mirror(tmp0);
      InstanceClass::Fast klass = method->holder();
      Value klass_value(T_OBJECT);
      klass_value.set_obj(&klass);

      if (StopAtRealMirrorAccess){
          breakpoint();
      }
      load_task_mirror(&klass, klass_value, true JVM_CHECK);
      // Now load the real mirror
      ldr(r0, klass_value.lo_register(),
                        TaskMirror::real_java_mirror_offset());
#else
      JavaClass::Fast klass = method->holder();
      Instance::Fast mirror = klass().java_mirror();
      NOT_PRODUCT(comment("Static method. Synchronize on the class "
                          "mirror object"));
      if (GenerateROMImage) {
        // ldr_oop handles classes correctly
        ldr_oop(r0, &klass);
        ldr(r0, r0, JavaClass::java_mirror_offset());
      } else {
        ldr_oop(r0, &mirror);
      }
#endif
    } else {
      NOT_PRODUCT(comment("Non-static method. Synchronize on the receiver"));
      LocationAddress address(0, T_OBJECT);
      Assembler::Register address_reg;
      int address_offset = 0;
      // prepare and load data at lo address
      address.get_preindexed_address(true, address_reg, address_offset);
      ldr(r0, address_reg, address_offset);
    }
    call_through_gp(gp_shared_lock_synchronized_method_ptr JVM_CHECK);
  } else { // not synchronized
    if (method->access_flags().has_monitor_bytecodes()) {
      // Method isn't synchronized, but it has monitor bytecodes.
      NOT_PRODUCT(comment("fill in the stack bottom pointer"));
      str(jsp, fp, JavaFrame::stack_bottom_pointer_offset());
    } else {
#ifdef AZZERT
#if !ENABLE_ARM_V7 // IMPL_NOTE: move to HB block
      if (!GenerateROMImage && GenerateCompilerAssertions) {
        NOT_PRODUCT(comment("insert bogus stack bottom pointer"));
        mov(r0, 0xA000000B); // pretty bogus immediate
        str(r0, fp, JavaFrame::stack_bottom_pointer_offset());
      }
#endif
#endif
    }
  }

#if ENABLE_PROFILER
  check_timer_tick(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
#endif // ENABLE_PROFILER
  RegisterAllocator::dereference(r0);
}

void CodeGenerator::clear_stack() {
  if (method()->access_flags().is_synchronized()
             || method()->access_flags().has_monitor_bytecodes()) {
    // if the method is synchronized or has monitor bytecodes the
    // stack bottom pointer in the frame descriptor is filled in
    if (ENABLE_ARM_V7) {
      ldr_imm12_w(jsp, fp, JavaFrame::stack_bottom_pointer_offset());
    } else {
      ldr(jsp, fp, JavaFrame::stack_bottom_pointer_offset());
    }
  } else {
    add(jsp, fp, JavaFrame::empty_stack_offset());
  }
}

void CodeGenerator::clear_object_location(jint index) {
  // The field is actual T_OBJECT, but T_INT is simpler to work with, and
  // the result is the same
  Value zero_val(T_INT);
  zero_val.set_int(0);
  LocationAddress address(index, T_INT);
  store_to_address(zero_val, T_INT, address);
}

void CodeGenerator::int_binary_do(Value& result, Value& op1, Value& op2,
                                  BytecodeClosure::binary_op op JVM_TRAPS) {
  write_literals_if_desperate();

  GUARANTEE(!result.is_present(), "result must not be present");
  GUARANTEE(op1.in_register(), "op1 must be in a register");
  GUARANTEE(op2.is_immediate() || op2.in_register(),
            "op2 must be in a register or an immediate");

  switch (op) {
    case BytecodeClosure::bin_sub  :
    {
      assign_register(result, op1);
      if (!op2.is_immediate()) {
        sub(result.lo_register(),
                 op1.lo_register(), op2.lo_register());
      } else {
        sub(result.lo_register(), op1.lo_register(),
            op2.as_int());
      }
      break;
    }
    case BytecodeClosure::bin_rsb  :
      op2.materialize();
      arithmetic (_sub, result, op2, op1);
      break;
    case BytecodeClosure::bin_add  :
      assign_register(result, op1);
      if (!op2.is_immediate()) {
        add(result.lo_register(),
                 op1.lo_register(), op2.lo_register());
      } else {
        add(result.lo_register(), op1.lo_register(),
            op2.as_int());
      }
      break;
    case BytecodeClosure::bin_and  :
      arithmetic (_and, result, op1, op2);
      break;
    case BytecodeClosure::bin_xor  :
      arithmetic (_eor, result, op1, op2);
      break;
    case BytecodeClosure::bin_or   :
      arithmetic (_orr, result, op1, op2);
      break;
    case BytecodeClosure::bin_shr  :
      shift (asr_shift, result, op1, op2);
      break;
    case BytecodeClosure::bin_shl  :
      shift (lsl_shift, result, op1, op2);
      break;
    case BytecodeClosure::bin_ushr :
      shift (lsr_shift, result, op1, op2);
      break;
    case BytecodeClosure::bin_mul  :
      imul (result, op1, op2 JVM_NO_CHECK_AT_BOTTOM);
      break;
    case BytecodeClosure::bin_min  :
    case BytecodeClosure::bin_max  :
    {
      assign_register(result, op1);
      op2.materialize();
      mov(result.lo_register(), op1.lo_register());
      cmp(op1.lo_register(), reg(op2.lo_register()));
      NearLabel skip_mov;
      b(skip_mov, not_cond((op == BytecodeClosure::bin_min) ? gt : lt));
      mov(result.lo_register(), reg(op2.lo_register()));
      bind(skip_mov);
      break;
    }
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
  write_literals_if_desperate();

  GUARANTEE(!result.is_present(), "result must not be present");
  GUARANTEE(op1.in_register(), "op1 must be in a register");

  assign_register(result, op1);
  if( op == BytecodeClosure::una_abs ) {
    add(result.lo_register(), op1.lo_register(), zero);
    it(lt);
  }
  rsb_imm12_w(result.lo_register(), op1.lo_register(), imm12(zero), set_CC);
}

void CodeGenerator::long_binary_do(Value& result, Value& op1, Value& op2,
                                   BytecodeClosure::binary_op op JVM_TRAPS) {
  write_literals_if_desperate(24);

  GUARANTEE(!result.is_present(), "result must not be present");
  GUARANTEE(op1.in_register(), "op1 must be in a register");
  GUARANTEE(op2.is_immediate() || op2.in_register(),
            "op2 must be in a register or an immediate");

  switch (op) {
    case BytecodeClosure::bin_sub:
      larithmetic(_sub, _sbc, result, op1, op2 JVM_NO_CHECK_AT_BOTTOM);
      break;
    case BytecodeClosure::bin_rsb:
      op2.materialize();
      larithmetic(_sub, _sbc, result, op2, op1 JVM_NO_CHECK_AT_BOTTOM);
      break;
    case BytecodeClosure::bin_add:
      larithmetic(_add, _adc, result, op1, op2 JVM_NO_CHECK_AT_BOTTOM);
      break;
    case BytecodeClosure::bin_and:
      larithmetic(_and, _and, result, op1, op2 JVM_NO_CHECK_AT_BOTTOM);
      break;
    case BytecodeClosure::bin_xor:
      larithmetic(_eor, _eor, result, op1, op2 JVM_NO_CHECK_AT_BOTTOM);
      break;
    case BytecodeClosure::bin_or :
      larithmetic(_orr, _orr, result, op1, op2 JVM_NO_CHECK_AT_BOTTOM);
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
      lshift(asr_shift, result, op1, op2 JVM_NO_CHECK_AT_BOTTOM);
      break;

    case BytecodeClosure::bin_shl :
      lshift(lsl_shift, result, op1, op2 JVM_NO_CHECK_AT_BOTTOM);
      break;

    case BytecodeClosure::bin_ushr:
      lshift(lsr_shift, result, op1, op2 JVM_NO_CHECK_AT_BOTTOM);
      break;

    case BytecodeClosure::bin_min:
      call_simple_c_runtime(result, (address)::jvm_lmin, op1, op2);
      break;

    case BytecodeClosure::bin_max:
      call_simple_c_runtime(result, (address)::jvm_lmax, op1, op2);
      break;

    default                    :
      SHOULD_NOT_REACH_HERE();        break;
  }
}

void CodeGenerator::long_unary_do(Value& result, Value& op1,
                                   BytecodeClosure::unary_op op JVM_TRAPS) {
  write_literals_if_desperate();

  GUARANTEE(!result.is_present(), "result must not be present");
  GUARANTEE(op1.in_register(), "op1 must be in a register");
  NearLabel done;

  assign_register(result, op1);

  Register A1 =  op1.lsw_register();
  Register A2 =  op1.msw_register();

  Register R1 =  result.lsw_register();
  Register R2 =  result.msw_register();

  switch (op) {
    case BytecodeClosure::una_neg: {
      const TempRegister zero_reg;
      ldr_address(zero_reg, 0);
      // IMPL_NOTE: should set_CC
      sub(R1, zero_reg, A1);  // rsb
      sbc(zero_reg, A2);           // rsc
      mov(R2, zero_reg);
      break;
    }
    case BytecodeClosure::una_abs: {
      mov(R1, A1);
      add(R2, A2, zero);
      b(done, ge);              // If hi register >= 0, positive
      const TempRegister zero_reg;
      ldr_address(zero_reg, 0);
      sub(R1, zero_reg, A1);  // rsb
      sbc(zero_reg, A2);           // rsc
      mov(R2, zero_reg);
      bind(done);
      break;
    }
    default:
      SHOULD_NOT_REACH_HERE();
      break;
  }
}

void CodeGenerator::arithmetic(Opcode opcode,
                               Value& result, Value& op1, Value& op2){
  assign_register(result, op1);
  if (result.lo_register() != op1.lo_register()) {
    mov(result.lo_register(), op1.lo_register());
  }

  if (op2.is_immediate()) {
    const CompilerLiteralAccessor cla;
    arith_imm(opcode, result.lo_register(), op2.as_int(), cla);
  } else {
    arith(opcode, result.lo_register(), op2.lo_register());
  }
}

void CodeGenerator::imul(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  // Check.  Can we reuse op1.lo_register() in result?
  result.assign_register();

  if (op2.is_immediate()) {
    const TempRegister tmp;
    mul_imm(result.lo_register(), op1.lo_register(), op2.as_int(), tmp);
  } else {
    mov(result.lo_register(), op2.lo_register());
    mul(result.lo_register(), op1.lo_register());
  }
}

void CodeGenerator::idiv_rem(Value& result, Value& op1, Value& op2,
                             bool isRemainder JVM_TRAPS) {
  int divisor = op2.in_register() ? 0 : op2.as_int();
  bool negate = false;
  if (divisor < 0) {
    // We only need to negate the result for division
    negate = !isRemainder;
  }

  if (!op2.in_register()) {
    if (divisor == 0) {
      ZeroDivisorCheckStub* zero_error =
        ZeroDivisorCheckStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(zero_error));
      b(zero_error);
    } else if (divisor == 1) {
      if (isRemainder) {
        result.set_int(0);
      } else if (negate) {
        int_unary_do(result, op1, BytecodeClosure::una_neg JVM_NO_CHECK_AT_BOTTOM);
      } else {
        op1.copy(result);
      }
    }
  }

#if ENABLE_ARM_V7 && 0 // sdiv is not supported in ARMv7l
  // Can use SDIV.W
  if (!op2.in_register()) {
    op2.assign_register();
    mov(op2.lo_register(), divisor);
  }

  if (isRemainder) {
    result.assign_register();
    sdiv_w(result.lo_register(), op1.lo_register(), op2.lo_register());
    // IMPL_NOTE: use instead of r12 any unreferenced register
    smull_w(result.lo_register(), r12, result.lo_register(), op2.lo_register());
    Assembler::sub(result.lo_register(), op1.lo_register(), result.lo_register());
  } else {
    assign_register(result, op1);
    sdiv_w(result.lo_register(), op1.lo_register(), op2.lo_register());
  }
#else
  frame()->flush(JVM_SINGLE_ARG_CHECK);
  if (!op2.in_register()) {
    op2.assign_register();
    mov(op2.lo_register(), divisor);
  }
  setup_c_args(2, &op1, &op2, NULL);
  call_through_gp(gp_compiler_idiv_irem_ptr JVM_CHECK);
  Register result_register = isRemainder ? r0 : r1;
  RegisterAllocator::reference(result_register);
  result.set_register(result_register);
#endif
}

void CodeGenerator::shift(Shift shifter, Value& result, Value& op1, Value& op2){
  assign_register(result, op1);

  if (op2.is_immediate()) {
    // We have to treat 0 as a special case since "asr 0" and "lsr 0"
    // don't actually mean "shift right by zero"
    int shift = (op2.as_int() & 0x1f);
    if (shift == 0) {
      mov(result.lo_register(), op1.lo_register());
    } else {
      switch(shifter) {
        case lsl_shift:
          lsl_imm5(result.lo_register(), op1.lo_register(), shift);
          break;

        case lsr_shift:
          lsr_imm5(result.lo_register(), op1.lo_register(), shift);
          break;

        case asr_shift:
          asr_imm5(result.lo_register(), op1.lo_register(), shift);
          break;

        case ror_shift: {
          mov(result.lo_register(), op1.lo_register());
          const TempRegister tmp;
          mov(tmp, shift);
          ror(result.lo_register(), tmp);
          break;
        }
        default:
          SHOULD_NOT_REACH_HERE();
          break;
      }
    }
  } else {
    if (result.lo_register() != op1.lo_register()) {
      mov(result.lo_register(), op1.lo_register());
    }
    const TempRegister shift_reg;
    mov(shift_reg, 0x1f);
    andr(shift_reg, op2.lo_register());

    switch(shifter) {
      case lsl_shift:
        lsl(result.lo_register(), shift_reg);
        break;

      case lsr_shift:
        lsr(result.lo_register(), shift_reg);
        break;

      case asr_shift:
        asr(result.lo_register(), shift_reg);
        break;

      case ror_shift:
        ror(result.lo_register(), shift_reg);
        break;

      default:
      {
        SHOULD_NOT_REACH_HERE();
        break;
      }
    }
  }
}

#if ENABLE_FLOAT
extern "C" {
  float jvm_fadd (float x, float y);
  float jvm_fsub (float x, float y);
  float jvm_fmul (float x, float y);
  float jvm_fdiv (float x, float y);
  float jvm_frem (float x, float y);
  int   jvm_fcmpl(float x, float y);
  int   jvm_fcmpg(float x, float y);
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

void CodeGenerator::ensure_not_in_float_register(Value& value) {
  if (value.type() == T_FLOAT && value.in_register()) {
    Register r = value.lo_register();
    if (r >= s0) {
      value.set_register(RegisterAllocator::allocate());
      fmrs(value.lo_register(), r);
    }
  } else if (value.type() == T_DOUBLE && value.in_register()) {
    Register l = value.lo_register();
    Register h = value.hi_register();
    if (l >= s0 && h >= s0) {
      Register lo = RegisterAllocator::allocate();
      Register hi = RegisterAllocator::allocate();
      value.set_registers(lo, hi);
      fmrrd(value.lo_register(), value.hi_register(), l);
    }
  }
}
#endif  // ENABLE_ARM_VFP

void CodeGenerator::float_binary_do(Value& result, Value& op1, Value& op2,
                                    BytecodeClosure::binary_op op JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  typedef float (*runtime_func_type)(float, float);
  static const runtime_func_type funcs [] = {
    jvm_fadd,   // bin_add = 0
    jvm_fsub,   // bin_sub = 1
    jvm_fmul,   // bin_mul = 2
    jvm_fdiv,   // bin_div = 3
    jvm_frem    // bin_rem = 4
  };

  GUARANTEE(int(op) >= int(BytecodeClosure::bin_add) &&
            int(op) <= int(BytecodeClosure::bin_rem), "sanity");
  runtime_func_type const runtime_func = funcs[op];

  if (op1.is_immediate() && op2.is_immediate()) {
    const float result_imm = runtime_func(op1.as_float(), op2.as_float());
    result.set_float(result_imm);
    return;
  }

#if ENABLE_ARM_VFP
  if (int(op) < int(BytecodeClosure::bin_rem)) {
    op1.materialize();
    op2.materialize();

    ensure_in_float_register(op1);
    ensure_in_float_register(op2);

    RegisterAllocator::reference(op1.lo_register());
    RegisterAllocator::reference(op2.lo_register());
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

  } else {      // bin_rem
    call_simple_c_runtime(result, (address)runtime_func, op1, op2);
  }
#else
  if ((op == BytecodeClosure::bin_add || op == BytecodeClosure::bin_mul)
      && (   (op1.in_register() && op1.lo_register() == r1)
          || (op2.in_register() && op2.lo_register() == r0))) {
    // Avoid register shuffling on the commutative operations.
    call_simple_c_runtime(result, (address)runtime_func, op2, op1);
  } else {
    call_simple_c_runtime(result, (address)runtime_func, op1, op2);
  }
#endif  // ENABLE_ARM_VFP
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
  if( op == BytecodeClosure::una_neg ) {
    fnegs(result.lo_register(), op1.lo_register());
  } else {
    fabss(result.lo_register(), op1.lo_register());
  }
#else
  const Opcode opcode = (op == BytecodeClosure::una_neg ? _eor : _bic);
  const TempRegister tmp;
  mov(tmp, 2);
  ror(tmp, tmp);
  if (result.lo_register() != op1.lo_register()) {
    mov(result.lo_register(), op1.lo_register());
  }

  arith(opcode, result.lo_register(), tmp);
#endif
}

void CodeGenerator::float_cmp (Value& result, BytecodeClosure::cond_op cond,
                               Value& op1, Value& op2 JVM_TRAPS) {

  int (*runtime_func)(float, float);
  switch (cond) {
    case BytecodeClosure::lt: runtime_func = jvm_fcmpl; break;
    case BytecodeClosure::gt: runtime_func = jvm_fcmpg; break;
    default                 : runtime_func = 0; SHOULD_NOT_REACH_HERE();
  }
  if (op1.is_immediate() && op2.is_immediate()) {
    result.set_int(runtime_func(op1.as_float(), op2.as_float()));
  } else {
#if ENABLE_ARM_VFP
    NearLabel done;
    op1.materialize();
    ensure_in_float_register(op1);

    op2.materialize();
    ensure_in_float_register(op2);

    result.assign_register();

    fcmpes(op1.lo_register(), op2.lo_register());
    fmstat();
    mov(result.lo_register(), 1);
    if (cond == BytecodeClosure::lt) {
      b(done, gt);
      fcmps(op1.lo_register(), op2.lo_register());
      fmstat();
      it(eq, ELSE_THEN);
      neg(result.lo_register(), result.lo_register());
      mov(result.lo_register(), 0);
    } else {
      it(mi);
      neg(result.lo_register(), result.lo_register());
      b(done, gt);
      fcmps(op1.lo_register(), op2.lo_register());
      fmstat();
      mov(result.lo_register(), 0, eq);
    }
    bind(done);
#else   // !ENABLE_ARM_VFP
    call_simple_c_runtime(result, (address)runtime_func, op1, op2);
#endif  // !ENABLE_ARM_VFP
  }
}

extern "C" {
  double jvm_dadd       (double x, double y);
  double jvm_dsub       (double x, double y);
  double jvm_dmul       (double x, double y);
  double jvm_ddiv       (double x, double y);
  double jvm_drem       (double x, double y);
  int    jvm_dcmpl      (double x, double y);
  int    jvm_dcmpg      (double x, double y);
}

void CodeGenerator::double_binary_do(Value& result, Value& op1, Value& op2,
                                     BytecodeClosure::binary_op op JVM_TRAPS) {
  double (*runtime_func)(double, double);
  switch (op) {
    case BytecodeClosure::bin_sub: runtime_func = jvm_dsub; break;
    case BytecodeClosure::bin_add: runtime_func = jvm_dadd; break;
    case BytecodeClosure::bin_mul: runtime_func = jvm_dmul; break;
    case BytecodeClosure::bin_div: runtime_func = jvm_ddiv; break;
    case BytecodeClosure::bin_rem: runtime_func = jvm_drem; break;
    default:                       runtime_func = 0; SHOULD_NOT_REACH_HERE();
  }
  if (op1.is_immediate() && op2.is_immediate()) {
    const jdouble result_imm = runtime_func(op1.as_double(), op2.as_double());
    result.set_double(result_imm);
    return;
  }
#if ENABLE_ARM_VFP
  if( int(op) < int(BytecodeClosure::bin_rem) ) {
    op1.materialize();
    op2.materialize();

    ensure_in_float_register(op1);
    ensure_in_float_register(op2);

    RegisterAllocator::reference(op1.lo_register());
    RegisterAllocator::reference(op1.hi_register());
    RegisterAllocator::reference(op2.lo_register());
    RegisterAllocator::reference(op2.hi_register());

    result.assign_register();

    switch (op) {
      case BytecodeClosure::bin_add:
        faddd(result.lo_register(), op1.lo_register(), op2.lo_register());
        break;
      case BytecodeClosure::bin_sub:
        fsubd(result.lo_register(), op1.lo_register(), op2.lo_register());
        break;
      case BytecodeClosure::bin_mul:
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
    return;
  }
#endif  // ENABLE_ARM_VFP
  call_simple_c_runtime(result, (address)runtime_func, op1, op2);
}

void CodeGenerator::double_unary_do(Value& result, Value& op1,
                                    BytecodeClosure::unary_op op JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();

  GUARANTEE(!result.is_present(), "result must not be present");
  GUARANTEE(op1.in_register(), "op1 must be in a register");

  GUARANTEE(op == BytecodeClosure::una_neg || op == BytecodeClosure::una_abs,
            "Sanity")

  ensure_in_float_register(op1);
  assign_register(result, op1);

#if ENABLE_ARM_VFP
  if (op == BytecodeClosure::una_neg) {
    fnegd(result.lo_register(), op1.lo_register());
  } else {
    fabsd(result.lo_register(), op1.lo_register());
  }
#else
  const Opcode opcode =  (op == BytecodeClosure::una_neg) ? _eor  : _bic;
  const TempRegister tmp;
  mov(tmp, 2);
  ror(tmp, tmp);

  if (MSW_FIRST_FOR_DOUBLE) {
    // The first word contains the sign bit
    if (result.lo_register() != op1.lo_register()) {
      mov(result.lo_register(), op1.lo_register());
    }
    arith(opcode, result.lo_register(), tmp);
    mov(result.hi_register(), op1.hi_register());
  } else {
    // The second word contains the sign bit
    if (result.hi_register() != op1.hi_register()) {
      mov(result.hi_register(), op1.hi_register());
    }
    arith(opcode, result.hi_register(), tmp);
    mov(result.lo_register(), op1.lo_register());
  }
#endif
}

void CodeGenerator::double_cmp(Value& result, BytecodeClosure::cond_op cond,
                               Value& op1, Value& op2 JVM_TRAPS) {
  int (*runtime_func)(double, double);
  switch (cond) {
    case BytecodeClosure::lt: runtime_func = jvm_dcmpl; break;
    case BytecodeClosure::gt: runtime_func = jvm_dcmpg; break;
    default                 : runtime_func = 0; SHOULD_NOT_REACH_HERE();
  }
  if (op1.is_immediate() && op2.is_immediate()) {
    result.set_int(runtime_func(op1.as_double(), op2.as_double()));
  } else {
#if ENABLE_ARM_VFP
    NearLabel done;
    op1.materialize();
    op2.materialize();

    ensure_in_float_register(op1);
    ensure_in_float_register(op2);

    result.assign_register();

    fcmped(op1.lo_register(), op2.lo_register());
    fmstat();
    mov(result.lo_register(), 1);

    if (cond == BytecodeClosure::lt) {
      b(done, gt);
      fcmpd(op1.lo_register(), op2.lo_register());
      fmstat();
      it(eq, ELSE_THEN);
      neg(result.lo_register(), result.lo_register());
      mov(result.lo_register(), 0);
    } else {
      it(mi);
      neg(result.lo_register(), result.lo_register());
      b(done, gt);
      fcmps(op1.lo_register(), op2.lo_register());
      fmstat();
      mov(result.lo_register(), 0, eq);
    }
    bind(done);
#else  // !ENABLE_ARM_VFP
    call_simple_c_runtime(result, (address)runtime_func, op1, op2);
#endif  // !ENABLE_ARM_VFP
  }
}
#endif

// Currently, this function is only used for floating point.
// It is actually rather generic and can be used for any C function
// that is guaranteed to never call into the VM.

void CodeGenerator::vcall_simple_c_runtime(Value& result,
                                          address runtime_func, ...) {
  GUARANTEE(runtime_func != 0, "sanity check");

#if ENABLE_ARM_VFP
  // Some C runtime routine such as d2i may touch the FP registers. Let's
  // make sure all FP registers are flushed to the stack.
  frame()->flush_fpu();
#endif

  int i;
  static const Register ctemps[] = { r0, r1, r2, r3, lr };

  for (i = 0; i < ARRAY_SIZE(ctemps); i++) {
    RegisterAllocator::reference(ctemps[i]);
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

  bool stole_r8 = false;
  if (RegisterAllocator::is_referenced(r7) ||
      RegisterAllocator::is_mapping_something(r7)){
    mov_hi(r8, r7);
    stole_r8 = true;
  }
  ldr_address(r7, runtime_func); // // IMPL_NOTE: move it directly to r12 using THUMB2
  mov(r12, reg(r7));
  if (stole_r8) {
    mov_hi(r7, r8);
  }

  if (JavaStackDirection > 0 && sp == jsp) {
    // IMPL_NOTE:  We don't have to move to the C stack for the functions written
    // in assembly language in Interpreter_arm.s.
    // fcmpl, fcmpg, dcmpl, dcmpg, jvm_i2f, jvm_i2d, jvm_f2i
    mov(lr, reg(jsp));
    ldr_using_gp(sp, (address)&_primordial_sp);
    sub(sp, sp, BytesPerWord);
    str(lr, sp);
  }

  // the constant in this instruction is patched below
  // to return after the written literals
  addw_imm12_w(lr, pc, 0);
  int code_offset = code_size(); // current pc

  // call the c function
  leavex();
#if ENABLE_ARM_V7
  mov(pc, r12);  // all code called by compiled code must be in Xenon mode
#else
  bx(r12);
#endif

  write_literals();

  if (!has_overflown_compiled_method()) {
    int new_imm = code_size() - (code_offset & 0xfffffffc) + 1;
    GUARANTEE(new_imm > 0 && new_imm <= 2047 && new_imm & 1,
              "Invalid lr patching");
    *(short *)(addr_at(code_offset) - 2) =
      (new_imm & 0xff) | ((new_imm >> 8) << 12) | (lr << 8);
  }

  enterx();

  if (JavaStackDirection > 0 && sp == jsp) {
    ldr(jsp, sp);
  }

#if ENABLE_ARM_VFP
  if (result.type() == T_FLOAT || result.type() == T_DOUBLE) {
    result.assign_register();
    const Register lo = result.lo_register();
    if (result.type() == T_FLOAT) {
      fmsr(lo, r0);
    } else {
      fmdrr(lo, r0, r1);
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
  GUARANTEE(value.in_register(), "Immediate case already handled");
  write_literals_if_desperate();

  result.assign_register();
  lsl_imm5(result.lo_register(), value.lo_register(),
      BitsPerWord - BitsPerByte);
  asr_imm5(result.lo_register(), result.lo_register(),
      BitsPerWord - BitsPerByte);
}

void CodeGenerator::i2c(Value& result, Value& value JVM_TRAPS) {
  GUARANTEE(value.in_register(), "Immediate case already handled");
  write_literals_if_desperate();

  result.assign_register();
  lsl_imm5(result.lo_register(), value.lo_register(),
      BitsPerWord - BitsPerShort);
  lsr_imm5(result.lo_register(), result.lo_register(),
      BitsPerWord - BitsPerShort);
}

void CodeGenerator::i2s(Value& result, Value& value JVM_TRAPS) {
  GUARANTEE(value.in_register(), "Immediate case already handled");
  write_literals_if_desperate();

  result.assign_register();
  lsl_imm5(result.lo_register(), value.lo_register(),
      BitsPerWord - BitsPerShort);
  asr_imm5(result.lo_register(), result.lo_register(),
      BitsPerWord - BitsPerShort);

}

void CodeGenerator::i2l(Value& result, Value& value JVM_TRAPS) {
  GUARANTEE(value.in_register(), "Immediate case already handled");
  write_literals_if_desperate();
  RegisterAllocator::reference(value.lo_register());
  if (MSW_FIRST_FOR_LONG) {
    result.set_registers(RegisterAllocator::allocate(), value.lo_register());
    asr_imm5(result.lo_register(), result.hi_register(), 31);
  } else {
    result.set_registers(value.lo_register(), RegisterAllocator::allocate());
    asr_imm5(result.hi_register(), result.lo_register(), 31);
  }
}

#if ENABLE_FLOAT
extern "C" {
  float   jvm_i2f(int);
  double  jvm_i2d(int);

  float   jvm_l2f(jlong);
  double  jvm_l2d(jlong);

  int     jvm_f2i(float);
  jlong   jvm_f2l(float);
  double  jvm_f2d(float);

  int     jvm_d2i(double);
  jlong   jvm_d2l(double);
  float   jvm_d2f(double);
};

void CodeGenerator::i2f(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_float(::jvm_i2f(value.as_int()));
  } else {
#if ENABLE_ARM_VFP
    ensure_not_in_float_register(value);
    result.assign_register();
    fmsr(result.lo_register(), value.lo_register());
    fsitos(result.lo_register(), result.lo_register());
#else  // !ENABLE_ARM_VFP
    call_simple_c_runtime(result, (address)::jvm_i2f, value);
#endif // !ENABLE_ARM_VFP
  }
}

void CodeGenerator::i2d(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_double(::jvm_i2d(value.as_int()));
  } else {
#if ENABLE_ARM_VFP
    ensure_not_in_float_register(value);
    result.assign_register();
    fmsr(result.lo_register(), value.lo_register());
    fsitod(result.lo_register(), result.lo_register());
#else  // !ENABLE_ARM_VFP
    call_simple_c_runtime(result, (address)::jvm_i2d, value);
#endif  // !ENABLE_ARM_VFP
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

    ftosizs(fresult, value.lo_register());

    RegisterAllocator::dereference(value.lo_register());

    result.assign_register();
    fmrs(result.lo_register(), fresult);
#else  // !ENABLE_ARM_VFP
    call_simple_c_runtime(result, (address)::jvm_f2i, value);
#endif // !ENABLE_ARM_VFP
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
#if ENABLE_ARM_VFP
    ensure_in_float_register(value);
    RegisterAllocator::reference(value.lo_register());

    result.assign_register();
    fcvtds(result.lo_register(), value.lo_register());

    RegisterAllocator::dereference(value.lo_register());
#else  // !ENABLE_ARM_VFP
    call_simple_c_runtime(result, (address)::jvm_f2d, value);
#endif  // !ENABLE_FLOAT
  }
}

void CodeGenerator::d2i(Value& result, Value& value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  write_literals_if_desperate();
  if (value.is_immediate()) {
    result.set_int(::jvm_d2i(value.as_double()));
  } else {
#if ENABLE_ARM_VFP
    ensure_in_float_register(value);

    RegisterAllocator::reference(value.lo_register());
    RegisterAllocator::reference(value.hi_register());

    TempVFPRegister freg;
    ftosizd(freg, value.lo_register());

    RegisterAllocator::dereference(value.lo_register());
    RegisterAllocator::dereference(value.hi_register());

    result.assign_register();
    fmrs(result.lo_register(), freg);
#else  // !ENABLE_ARM_VFP
    call_simple_c_runtime(result, (address)::jvm_d2i, value);
#endif // !ENABLE_ARM_VFP
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
#if ENABLE_ARM_VFP
    ensure_in_float_register(value);

    RegisterAllocator::reference(value.lo_register());
    RegisterAllocator::reference(value.hi_register());

    result.assign_register();
    fcvtsd(result.lo_register(), value.lo_register());

    RegisterAllocator::dereference(value.lo_register());
    RegisterAllocator::dereference(value.hi_register());
#else   // !ENABLE_ARM_VFP
    call_simple_c_runtime(result, (address)::jvm_d2f, value);
#endif  // ENABLE_ARM_VFP
  }
}
#endif  // ENABLE_FLOAT

void CodeGenerator::larithmetic(Opcode opcode1, Opcode opcode2,
                                Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  write_literals_if_desperate();
  jlong (*runtime_func)(jlong, jlong);

  // Check and see if assign_register was successful
  // if not we need to execute using a C method
  if (!RegisterAllocator::has_free(2, true)) {
    switch(opcode1) {
      case _add: runtime_func = ::jvm_ladd; break;
      case _sub: runtime_func = ::jvm_lsub; break;
      case _and: runtime_func = ::jvm_land; break;
      case _eor: runtime_func = ::jvm_lxor; break;
      case _orr: runtime_func = ::jvm_lor;  break;
      default  : runtime_func = NULL; SHOULD_NOT_REACH_HERE();
    }
    call_simple_c_runtime(result, (address)runtime_func, op1, op2);
    return;
  }

  assign_register(result, op1);
  GUARANTEE(result.in_register(), "Value should be in register now");

  // Remember that "lo" and "hi" refer to the address in memory.
  // For big endian machines, these are counter-intuitive
  Register A1 = op1.lsw_register();
  Register A2 = op1.msw_register();

  Register R1 = result.lsw_register();
  Register R2 = result.msw_register();

  if (A1 != R1) {
    mov(R1, A1);
  }

  if (A2 != R2) {
    mov(R2, A2);
  }

  if (op2.is_immediate()) {
    // // IMPL_NOTE: Optimize moves
    const TempRegister op2_imm;
    mov(op2_imm.reg(), op2.msw_bits());
    mov_hi(r8, op2_imm.reg());
    mov(op2_imm.reg(), op2.lsw_bits());
    arith(opcode1, R1, op2_imm.reg());
    mov_hi(op2_imm.reg(), r8);
    arith(opcode2, R2, op2_imm.reg());
  } else {
    arith(opcode1, R1, op2.lsw_register());
    arith(opcode2, R2, op2.msw_register());
  }
}

void CodeGenerator::lmul(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  write_literals_if_desperate();
  runtime_long_op(result, op1, op2, false, (address)jvm_lmul JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::runtime_long_op(Value& result, Value& op1, Value& op2,
                                    bool check_zero, address routine JVM_TRAPS) {
  write_literals_if_desperate();
  if (check_zero) {
    if (op2.in_register() || (op2.is_immediate() && op2.as_long() == 0)) {
      ZeroDivisorCheckStub* zero_error =
        ZeroDivisorCheckStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(zero_error));
      if (op2.is_immediate()) {
        jmp(zero_error);
      } else {
        const TempRegister tmp;
        mov(tmp, op2.lo_register());
        orr(tmp, reg(op2.hi_register()));
        b(zero_error, eq);
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
                           Value& op1, Value& op2 JVM_TRAPS) {
  write_literals_if_desperate();

  if (op2.is_immediate()) {
    op2.set_int(op2.as_int() & 0x3F);
  } else {
    const TempRegister tmp_reg;
    mov(tmp_reg, 0x3F);
    andr(op2.lo_register(), tmp_reg);
  }

  switch(type) {
    case lsl_shift :
      runtime_long_op(result, op1, op2, false, (address)jvm_lshl JVM_NO_CHECK_AT_BOTTOM);
      break;
    case lsr_shift :
      runtime_long_op(result, op1, op2, false, (address)jvm_lushr JVM_NO_CHECK_AT_BOTTOM);
      break;
    case asr_shift :
      runtime_long_op(result, op1, op2, false, (address)jvm_lshr JVM_NO_CHECK_AT_BOTTOM);
      break;
  }
  return;
}

void CodeGenerator::cmp_values(Value& op1, Value& op2) {
  GUARANTEE(op1.in_register(), "op1 must be in a register");
  GUARANTEE(op2.is_immediate() || op2.in_register(),
            "op2 must be in a register or an immediate");

  if (op2.is_immediate()) {
    const CompilerLiteralAccessor cla;
    cmp(op1.lo_register(), op2.as_int());
  } else {
    cmp(op1.lo_register(), reg(op2.lo_register()));
  }
}

void CodeGenerator::long_cmp(Value& result, Value& op1, Value& op2 JVM_TRAPS) {
  write_literals_if_desperate();
  GUARANTEE(!op1.is_immediate() || !op2.is_immediate(),
              "Immediate case handled by generic code");
  runtime_long_op(result, op1, op2, false,
                  (address)jvm_lcmp JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::check_bytecode_counter() {
  if (Deterministic) {
    Label det_done;
    Register reg = RegisterAllocator::allocate();

    get_bytecode_counter(reg);
    // IMPL_NOTE : removed set_CC ??
    sub(reg, reg, 1);
    b(det_done, ne);

    get_rt_timer_ticks(reg);
    add(reg, reg, 1);
    set_rt_timer_ticks(reg);

    mov(reg, RESCHEDULE_COUNT);
    bind(det_done);
    set_bytecode_counter(reg);
    RegisterAllocator::dereference(reg);
  }
}

void CodeGenerator::check_stack_overflow(Method *m JVM_TRAPS) {
  NOT_PRODUCT(comment("Check for stack overflow"));
  Label stack_overflow, done;

  // Method should be in r0 from invoke
  NOT_PRODUCT(comment("Check for stack overflow"));
  const TempRegister limit(r1);
  const TempRegister tmp(r2);
  get_current_stack_limit(tmp);
  add(r1, jsp, JavaStackDirection * (JavaFrame::frame_desc_size() +
               (m->max_execution_stack_count() *
                          BytesPerStackElement)));
  cmp(r1, reg(tmp));
  b(stack_overflow, JavaStackDirection < 0 ? ls : hs);
bind(done);

  StackOverflowStub* stub =
    StackOverflowStub::allocate(stack_overflow, done, r1, r0 JVM_ZCHECK(stub));
  stub->insert();

  // If we go to the stub, we can't be guaranteed it has preserved literals
  frame()->clear_literals();
}

void CodeGenerator::check_timer_tick(JVM_SINGLE_ARG_TRAPS) {
#if !ENABLE_ARM_V7 // IMPL_NOTE: V7 -> move operation into handler block
  Label timer_tick, done;

  NOT_PRODUCT(comment("check for timer tick"));
  const TempRegister tmp;
  get_rt_timer_ticks(tmp);
  cmp(tmp, 0x0);
  b(timer_tick, gt);
bind(done);

  TimerTickStub* stub =
    TimerTickStub::allocate(Compiler::current()->bci(),
                              timer_tick, done JVM_ZCHECK(stub));
  stub->insert();

  // If we go to the stub, we can't be guaranteed it has preserved literals
  frame()->clear_literals();
#endif
}

void CodeGenerator::check_cast(Value& object, Value& klass, int class_id
                               JVM_TRAPS) {
  write_literals_if_desperate();

  Label slow_case, done_checking;

  NOT_PRODUCT(comment("Typecast type check"));
  frame()->push(object);

  // Since register allocation might cause spilling we have to allocate *all*
  // registers before checking for null object.
  const TempRegister tmp1;
  const TempRegister tmp2;

  NOT_PRODUCT(comment("Check for NULL object, get its class if not null"));
  cmp(object.lo_register(), zero);
  b(done_checking, eq);

  ldr(tmp2, object.lo_register(), 0);
  ldr(tmp2, tmp2, 0);

  NOT_PRODUCT(comment("Check the subtype caches"));
  ldr(tmp1, tmp2, JavaClass::subtype_cache_1_offset());
  ldr(tmp2, tmp2, JavaClass::subtype_cache_2_offset());

  cmp(tmp1, reg(klass.lo_register()));
  NearLabel skip_cmp;
  b(skip_cmp, eq);
  cmp(tmp2, reg(klass.lo_register()));
  bind(skip_cmp);
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

void CodeGenerator::instance_of(Value& result, Value& object, Value& klass,
                                int class_id JVM_TRAPS) {
  (void)class_id;
  write_literals_if_desperate();

  Label slow_case, done_checking;
  result.assign_register();

  NOT_PRODUCT(comment("Instance-of type check"));
  frame()->push(object);

  // Since register allocation might cause spilling we have to allocate *all*
  // registers before checking for null object.
  const TempRegister tmp1;
  const TempRegister tmp2;

  NOT_PRODUCT(comment("Check for NULL object; Get the class for the object"));
  cmp(object.lo_register(), zero);
  NearLabel skip_mov1;
  b(skip_mov1, ne);
  // Object is NULL.. Skip loading class etc.
  ldr_address(result.lo_register(), 0);
  b(done_checking);
  bind(skip_mov1);
  // Object is non-null, load its class
  ldr_address(result.lo_register(), 0);
  ldr(tmp2, object.lo_register());
  ldr(tmp2, tmp2);

  NOT_PRODUCT(comment("Check the subtype caches"));
  ldr(tmp1, tmp2, JavaClass::subtype_cache_1_offset());
  ldr(tmp2, tmp2, JavaClass::subtype_cache_2_offset());

  NearLabel join, check_cache_2;

  // Check if cache_1 was a hit
  cmp(tmp1, reg(klass.lo_register()));
  NearLabel skip_cmp;
  b(check_cache_2, ne);
  mov(result.lo_register(), one);
  b(join);

  // Check if cache_2 was a hit
  bind(check_cache_2);
  cmp(tmp2, reg(klass.lo_register()));
  it(eq);
  mov(result.lo_register(), one);

  bind(join);

  // Check if we hit the caches. This seemingly
  // redundant step is required on thumb since
  // mov instructions intrinsically modifies flags.
  cmp(result.lo_register(), one);

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

void CodeGenerator::if_then_else(Value& result,
                                 BytecodeClosure::cond_op condition,
                                 Value& op1, Value& op2,
                                 ExtendedValue& result_true,
                                 ExtendedValue& result_false JVM_TRAPS) {

  GUARANTEE(false, "OptimizeForwardBranches : Disabled for thumb");

  if (result_true.is_value()) {
    assign_register(result, result_true.value());
  } else {
    result.assign_register();
  }

  cmp_values(op1, op2);
  // Destroy op1, op2 since quite often result_true is using same register
  op1.destroy(); op2.destroy();


  Condition cond = convert_condition(condition);
  NearLabel join_label, false_label;
  b(false_label, not_cond(cond));
  move(result, result_true);
  b(join_label);
  bind(false_label);
  move(result, result_false);
  bind(join_label);
}

void CodeGenerator::if_iinc(Value& result, BytecodeClosure::cond_op condition,
                            Value& op1, Value& op2,
                            Value& arg, int increment JVM_TRAPS) {

  GUARANTEE(false, "OptimizeForwardBranches : Disabled for thumb");

  Condition cond = convert_condition(condition);
  arg.materialize();
  assign_register(result, arg);
  // We hope that the following generates no code!
  move(result, arg);

  cmp_values(op1, op2);
  // Destroy op1, op2 since quite often value is using the same register
  op1.destroy(); op2.destroy();

  NearLabel skip_add;
  b(skip_add, not_cond(cond));
  add(result.lo_register(), result.lo_register(), increment);
  bind(skip_add);
}


void CodeGenerator::new_object(Value& result, JavaClass* klass JVM_TRAPS) {
  NOT_PRODUCT(comment("new_object"));
#ifdef AZZERT
  InstanceSize size = klass->instance_size();
  GUARANTEE(size.is_fixed(), "Size must be fixed in order to do allocation");
#endif
  // Do flushing, and remember to unmap.
  frame()->flush(JVM_SINGLE_ARG_CHECK);

  // Handle finalization by going slow-case for objects with finalizers.
  if (klass->has_finalizer()) {
    // _newobject(Thread&, raw_class);
    RegisterAllocator::reference(r1);
    ldr_oop(r1, klass);
    call_vm((address) _newobject, T_OBJECT JVM_CHECK);
    RegisterAllocator::dereference(r1);
  } else {
    RegisterAllocator::reference(r0);
    ldr_oop(r0, klass);
#if ENABLE_ARM_V7
    if (UseHandlers) {
      hbl(compiler_new_object_handler); //NOT_PRODUCT(eol_comment("compiler_new_object_handler"));
      write_call_info(0 JVM_CHECK);
    } else
#endif
    {
      call_through_gp(gp_compiler_new_object_ptr, false JVM_CHECK);
    }
    RegisterAllocator::dereference(r0);
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
  frame()->flush(JVM_SINGLE_ARG_CHECK);

  // Call the allocation routine.
  Value save_reg_for_oop(T_ILLEGAL);
  setup_c_args(2, &save_reg_for_oop, &length, NULL);

  ldr_oop(r0, &java_near);
#if ENABLE_ARM_V7
  if (UseHandlers) {
    hbl(comp_new_obj_array_handler);
    write_call_info(0 JVM_CHECK);
  } else
#endif
  {
    call_through_gp(gp_compiler_new_obj_array_ptr JVM_CHECK);
  }

  // The result is in r0
  RegisterAllocator::reference(r0);
  result.set_register(r0);
}

void CodeGenerator::new_basic_array(Value& result, BasicType type,
                                    Value& length JVM_TRAPS) {
  UsingFastOops fast_oops;
  // Do flushing, and remember to unmap.
  frame()->flush(JVM_SINGLE_ARG_CHECK);

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

    Register reg_actual_length = actual_length.lo_register();
    RegisterAllocator::reference(reg_actual_length);

    switch(array_class->scale()) {
      case 1: {
        const TempRegister tmp;
        GUARANTEE(tmp.reg() != reg_actual_length, "new_basic_array: Invalid register assignment");
        mov(tmp, Array::base_offset() + BytesPerWord - 1);
        add(reg_actual_length, length.lo_register(), tmp);
        mov(tmp, 3);
        bic(reg_actual_length, tmp);
        break;
      }
      case 2: {
        const TempRegister tmp;
        GUARANTEE(tmp.reg() != reg_actual_length, "new_basic_array: Invalid register assignment");
        lsl_imm5(reg_actual_length, length.lo_register(), 1);
        mov(tmp, Array::base_offset() + BytesPerWord - 1);
        add(reg_actual_length, reg_actual_length, tmp);
        mov(tmp, 3);
        bic(reg_actual_length, tmp);
        break;
      }
      default:
        lsl_imm5(reg_actual_length, length.lo_register(),
                jvm_log2(array_class->scale()));
        add(reg_actual_length, reg_actual_length,
            Array::base_offset());
        break;
    }
    RegisterAllocator::dereference(reg_actual_length);
  }
  setup_c_args(3, &save_reg_for_oop, &length, &actual_length, NULL);

  ldr_oop(r0, &java_near);
  RegisterAllocator::reference(r2);
#if ENABLE_ARM_V7
  if (UseHandlers) {
    hbl(comp_new_type_array_handler);
    write_call_info(0 JVM_CHECK);
  } else
#endif
  {
    call_through_gp(gp_compiler_new_type_array_ptr JVM_CHECK);
  }
  RegisterAllocator::dereference(r2);

  // The result is in r0
  RegisterAllocator::reference(r0);
  result.set_register(r0);
}

void CodeGenerator::new_multi_array(Value& result JVM_TRAPS) {
  frame()->flush(JVM_SINGLE_ARG_CHECK);

  // Call the runtime system.
  call_vm((address) multianewarray, T_ARRAY JVM_CHECK);

  // The result is in r0
  RegisterAllocator::reference(r0);
  result.set_register(r0);
}

void CodeGenerator::monitor_enter(Value& object JVM_TRAPS) {
  // For now we flush before calling the compiler monitor enter stub.
  frame()->flush(JVM_SINGLE_ARG_CHECK);
  mov(r0, object.lo_register());
  call_through_gp(gp_shared_monitor_enter_ptr JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::monitor_exit(Value& object JVM_TRAPS) {
  // For now we flush before calling the compiler monitor exit stub.
  frame()->flush(JVM_SINGLE_ARG_CHECK);
  // Make sure the object is in register r0 (tos_val).
  mov(r0, object.lo_register());
  call_through_gp(gp_shared_monitor_exit_ptr JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::return_result(Value& result JVM_TRAPS) {
  // unlocking of synchronized methods occurs with unlock_activation
  // setup result

  // Put the result into r0 or r0/r1
  setup_c_args(1, &result, NULL);
  RegisterAllocator::reference(r0);
  RegisterAllocator::reference(r1);

#if ENABLE_ARM_V7
  if (UseHandlers) {
    const int max_locals = method()->max_locals();
    // Use hbl even though we don't need a link,
    // since hb has only 3 bits for immediate.
    hbl_with_parameter(return_handler, max_locals);
  } else
#endif
  {
    RegisterAllocator::reference(r3);
    restore_last_frame(r3);
    mov(pc, r3); // We always return to Java code or an assembly language stub
    RegisterAllocator::dereference(r3);
  }

  RegisterAllocator::dereference(r0);
  RegisterAllocator::dereference(r1);
  write_literals();
}

void CodeGenerator::return_void(JVM_SINGLE_ARG_TRAPS) {
  // unlocking of synchronized methods occurs with unlock_activation
  NOT_PRODUCT(comment("return void"));
#if ENABLE_ARM_V7
  if (UseHandlers) {
    const int max_locals = method()->max_locals();
    // Use hbl even though we don't need a link,
    // since hb has only 3 bits for immediate.
    hbl_with_parameter(return_handler, max_locals);

  } else
#endif
  {
    RegisterAllocator::reference(r3);
    restore_last_frame(r3);

    mov(pc, r3);
    RegisterAllocator::dereference(r3);
  }

  // An excellent place to write literals
  write_literals();
}

void CodeGenerator::return_error(Value& value JVM_TRAPS) {
  // This looks almost like return_void, except that we save
  // the return address in lr, and put the error into r0
  NOT_PRODUCT(comment("return with error"));

  mov(r1, value.lo_register());
#if ENABLE_ARM_V7
  if (UseHandlers) {
    const int max_locals = method()->max_locals();
    // Use hbl even though we don't need a link,
    // since hb has only 3 bits for immediate.
    hbl_with_parameter(return_error_handler, max_locals);
  } else
#endif
  {
    RegisterAllocator::reference(r0);
    RegisterAllocator::reference(r1);

    ldr_address(r0, (address)shared_call_vm_exception);
    restore_last_frame(lr);
    bx(r0);

    RegisterAllocator::dereference(r0);
    RegisterAllocator::dereference(r1);
  }

  write_literals();
}

void CodeGenerator::restore_last_frame(Register return_address) {
  jint locals = method()->max_locals();
  jint offset = JavaFrame::end_of_locals_offset()
                    -  locals * JavaStackDirection * BytesPerStackElement;
  // We can avoid loading a memory location by dead reckoning the new value
  // of jsp off of fp, then by pulling it out of the frame.
  if (return_address == lr) {
    const TempRegister tmp_reg;
    ldr(tmp_reg, fp, JavaFrame::return_address_offset());
    mov(lr, tmp_reg);
  } else {
    GUARANTEE(return_address < r8, "sanity");
    ldr(return_address, fp, JavaFrame::return_address_offset());
  }
  mov(jsp, reg(fp));
  GUARANTEE(JavaFrame::caller_fp_offset() == 0, "Code assumption");
  ldr_jsp(fp, 0);
#if ENABLE_ARM_V7
  if (offset >= 0) {
    add(jsp, jsp, imm12(offset));
  } else {
    sub(jsp, jsp, imm12(-offset));
  }
#else
  add(jsp, jsp, offset);
#endif
}

// See ThrowExceptionStub::compile in CompilationQueue.cpp about 3 ways of
// throwing runtime exceptions.
void CodeGenerator::throw_simple_exception(int rte JVM_TRAPS) {
  if (rte == ThrowExceptionStub::rte_null_pointer) {
#if ENABLE_ARM_V7
    mov(r0, method()->max_locals());
    ldr_using_gp(pc, "compiler_throw_NullPointerException");
#else
    Register tmp_reg = r1;
    address &target = gp_compiler_throw_NullPointerException_ptr;
    mov(r0, method()->max_locals());
    long offset = (long)&target - (long)&gp_base_label;
    mov(tmp_reg, gp);
    add(tmp_reg, tmp_reg, offset);
    ldr(tmp_reg, tmp_reg, 0);
    bx(tmp_reg);
#endif
    write_literals();
  } else if (rte == ThrowExceptionStub::rte_array_index_out_of_bounds) {
#if ENABLE_ARM_V7
    mov(r0, method()->max_locals());
    ldr_using_gp(pc, "compiler_throw_ArrayIndexOutOfBoundsException");
#else
    Register tmp_reg = r1;
    address &target = gp_compiler_throw_ArrayIndexOutOfBoundsException_ptr;
    mov(r0, method()->max_locals());
    long offset = (long)&target - (long)&gp_base_label;
    mov(tmp_reg, gp);
    add(tmp_reg, tmp_reg, offset);
    ldr(tmp_reg, tmp_reg, 0);
    bx(tmp_reg);
#endif
    write_literals();
  } else {
    frame()->clear(); // Is this necessary?
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

  NOT_PRODUCT(comment("Unlock synchronized method"));
  frame()->flush(JVM_SINGLE_ARG_CHECK);
  call_through_gp(gp_shared_unlock_synchronized_method_ptr
                  JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::check_monitors(JVM_SINGLE_ARG_TRAPS) {
  // Make sure there are no locked monitors on the stack.
  // Add the stub for the unlock exception.
  Label unlocking_loop, unlocking_loop_entry;

  const TempRegister lock;
  const TempRegister object;
  const TempRegister end;

  write_literals_if_desperate();

  const CompilerLiteralAccessor cla;
  NOT_PRODUCT(comment("Point at the object of the topmost stack lock"));
  ldr(lock, fp, JavaFrame::stack_bottom_pointer_offset());
  add(end, fp,
          JavaFrame::pre_first_stack_lock_offset() + StackLock::size());
  if (JavaStackDirection < 0) {
    add(lock, lock, StackLock::size());
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
#ifndef PRODUCT
  breakpoint(JavaStackDirection < 0 ? hi : lo);
#endif

  NearLabel pre_load_obj;
  int lock_offset = -JavaStackDirection * (BytesPerWord + StackLock::size());

  b(pre_load_obj, not_cond(ne));
  ldr(object, lock, 0);
  add(lock, lock, lock_offset);

  bind(pre_load_obj);
  b(unlocking_loop, ne);
}

void CodeGenerator::table_switch(Value& index, jint table_index,
                                 jint default_dest, jint low, jint high
                                 JVM_TRAPS) {
  GUARANTEE(index.in_register(), "Immediates handled by caller");
  NOT_PRODUCT(comment("tableswitch"));

  if (default_dest <= bci()) {
    // Negative offset in a branch table is not a usual case
    Compiler::abort_active_compilation(true JVM_THROW);
  }

  Register entry = RegisterAllocator::allocate();
  sub(entry, index.lo_register(), low);

  // IMPL_NOTE:
  //CompilerLiteralAccessor cla;
  cmp(entry, high - low);
  {
    // We fall through to here if not in the range low <= index <= high
    Label label;
    b(label, hi);
    CompilationContinuation::insert(default_dest, label JVM_CHECK);
  }

  lsl_imm5(entry, entry, 2);
  add_hi(pc, entry);
  nop();

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

void CodeGenerator::lookup_switch(Value& index, jint table_index,
                                  jint default_dest, jint num_of_pairs JVM_TRAPS) {

  NearLabel skip_literals;
  b(skip_literals);
  write_literals(true);
  bind(skip_literals);

  const CompilerLiteralAccessor cla;
  for (int i = 0, offset = table_index + 8;
       i < num_of_pairs;
       i++, offset += 8) {
    int key = method()->get_java_switch_int(offset);
    int jump_offset = method()->get_java_switch_int(offset + 4);
    if (jump_offset <= 0) {
      // Negative offset in a branch table is not a usual case
      Compiler::abort_active_compilation(true JVM_THROW);
    }
    cmp(index.lo_register(), key);
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
  const CompilerLiteralAccessor cla;
#ifndef PRODUCT
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
      cmp_imm(index, key, cla);
      conditional_jump(BytecodeClosure::eq, jump_bci, false JVM_CHECK);
    }
    // Allowed to fall through on default
  } else {
    Label larger, smaller_default;
    int i = (start + end) >> 1;
    int key = method()->get_java_switch_int(8 * i + table_index + 8);
    int jump_bci =
             bci() + method()->get_java_switch_int(8 * i + table_index + 12);

    cmp_imm(index, key, cla);
    conditional_jump(BytecodeClosure::eq, jump_bci, false JVM_CHECK);
    b(larger, gt);

    {
      PreserveVirtualStackFrameState state(frame() JVM_CHECK);
      // Handle start .. i - 1
      lookup_switch(index, table_index, start, i-1, default_dest JVM_CHECK);
      b(smaller_default);
      CompilationContinuation::insert(default_dest, smaller_default JVM_CHECK);
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

void CodeGenerator::invoke(const Method* method, bool must_do_null_check JVM_TRAPS) {
  bool is_native = false;
  int size_of_parameters = method->size_of_parameters();

  if (must_do_null_check &&
      !(frame()->receiver_must_be_nonnull(size_of_parameters)))
  {
    Value receiver(T_OBJECT);
    frame()->receiver(receiver, size_of_parameters);
    maybe_null_check(receiver JVM_CHECK);
  }

  Value hold_method(T_OBJECT);
  Value hold_tmp(T_INT);

  if (ENABLE_ARM_V7 && UseHandlers) {
    frame()->flush(JVM_SINGLE_ARG_CHECK);
    hold_method.set_register(RegisterAllocator::allocate(r0));
    hold_tmp.set_register(RegisterAllocator::allocate(r7));
  } else {
    hold_method.set_register(RegisterAllocator::allocate(r0));
    RegisterAllocator::reference(lr); // tmp cannot be lr
    hold_tmp.assign_register();
    RegisterAllocator::dereference(lr);
  }
  Register tmp = hold_tmp.lo_register();
  GUARANTEE(tmp != lr, "Must not have lr as temporary register");

  // The method must be in r0 when we enter it
  ldr_oop(r0, (Method*)method);

#if CROSS_GENERATOR
  bool try_skip_lookup = true;
  // IMPL_NOTE: this needs to be fixed so that the AOT-compiled code can
  // directly invoke the method without a look up
  if (method->is_native() &&
      method->get_native_code() == (address)Java_void_unimplemented) {
    // Used by AOT only: we are calling a MIDP native method, which
    // is not resolved during romization
    try_skip_lookup = false;
  } else if (method->is_quick_native() &&
             method->get_quick_native_code() ==
             (address)Java_void_unimplemented) {
    try_skip_lookup = false;
  }
#else
  const bool try_skip_lookup = true;
#endif

  if (try_skip_lookup && method->is_impossible_to_compile()) {
    // We don't need to hunt for the entry.  Its current entry will not change

    const bool can_optimize_quick_natives = !ENABLE_WTK_PROFILER &&
        !ENABLE_ARM_V7;
    // IMPL_NOTE: ^^^^^^^^^ direct call to native code not supproed in T2EE mode

    if (can_optimize_quick_natives && method->is_quick_native()) {
      address native_code = method->get_quick_native_code();
      // We actually only need to flush the end of the stack containing the
      // arguments, but we don't really have any way of doing that..
      frame()->flush(JVM_SINGLE_ARG_CHECK);
      if (size_of_parameters > 0) {
        add(jsp, jsp,
              size_of_parameters * -JavaStackDirection * BytesPerStackElement);
        int offset = method->is_static()
                     ? JavaFrame::arg_offset_from_sp(0)
                     : JavaFrame::arg_offset_from_sp(-1);
        if (offset == 0) {
          set_kni_parameter_base(jsp);
        } else {
          add(tmp, jsp, offset);
          set_kni_parameter_base(tmp);
        }
      } else {
        ldr_address(tmp, 0);
        set_kni_parameter_base(tmp);
      }
      Value result(T_INT);      // this is a lie.  But vcall wants something
      vcall_simple_c_runtime(result, (address)native_code, NULL);
      is_native = true;
    } else {
      address target = method->execution_entry();
      ldr_address(tmp, target);
      frame()->flush(JVM_SINGLE_ARG_CHECK);
      call_from_compiled_code(tmp, 0, size_of_parameters JVM_CHECK);
    }
  } else {
    GUARANTEE(!frame()->is_mapping_something(tmp), "Must be free");
    GUARANTEE(!frame()->is_mapping_something(r0), "Must be free");

    const bool is_heap_method =
      ObjectHeap::contains_moveable(method->obj()) && !GenerateROMImage;
#if ENABLE_ARM_V7
    if (UseHandlers) {
      hbl(is_heap_method ? invoke_heap_handler : invoke_rom_handler);
      write_call_info(size_of_parameters JVM_CHECK);
    } else
#endif
    {
      // NOT_CURRENTLY_USED: CodeInterleaver weaver(this);
      // WARNING: Each of these instructions in the first part must be a
      // simple instruction that doesn't use literals or labels or anything
      // like that.  Each  of these instructions get interleaved with the
      // flush() below.
      // The first instruction of the flush() comes before the first
      // ldr(tmp...) below
      if (is_heap_method) {
        ldr(tmp, r0, Method::heap_execution_entry_offset());
      } else {
        ldr(tmp, r0, Method::variable_part_offset());
        ldr(tmp, tmp);
      }
      // NOT_CURRENTLY_USED: weaver.start_alternate(JVM_SINGLE_ARG_CHECK);
      frame()->flush(JVM_SINGLE_ARG_CHECK);
      // NOT_CURRENTLY_USED: weaver.flush();
      // invoke the method
      call_from_compiled_code(tmp, 0, size_of_parameters JVM_CHECK);
    }
  }

  Signature::Raw signature = method->signature();
  adjust_for_invoke(method->size_of_parameters(),
                    signature().return_type(), is_native);
}

void CodeGenerator::invoke_virtual(Method* method, int vtable_index,
                                   BasicType return_type JVM_TRAPS) {
  int size_of_parameters = method->size_of_parameters();

  {
    Value receiver(T_OBJECT);
    frame()->receiver(receiver, size_of_parameters);
    if (receiver.must_be_null()) {
      // We are guaranteed to be in the test suite.  The receiver is known to
      // be null at compile time!  This just isn't worth worrying about
      go_to_interpreter(JVM_SINGLE_ARG_CHECK);
      return;
    }
    const bool can_skip_null_check = ENABLE_ARM_V7 &&
      is_inline_exception_allowed(ThrowExceptionStub::rte_null_pointer JVM_CHECK);
    if (!can_skip_null_check) {
      maybe_null_check(receiver JVM_CHECK);
    }

    Register recv = receiver.lo_register();
    receiver.destroy();
    ldr(RegisterAllocator::allocate(r0), recv);
  }

  Value hold_tmp(T_INT);
  Value hold_method(T_OBJECT);
  hold_method.set_register(r0);

  const bool is_heap_method =
    ObjectHeap::contains_moveable(method->obj()) && !GenerateROMImage;
#if ENABLE_ARM_V7
  if (UseHandlers) {
    frame()->flush(JVM_SINGLE_ARG_CHECK);
    int invoke_virtual_handler = is_heap_method ?
      invoke_virtual_heap_handler : invoke_virtual_rom_handler;
    hbl_with_parameter(invoke_virtual_handler, vtable_index);
    write_call_info(size_of_parameters JVM_CHECK);
  } else
#endif
  {
    RegisterAllocator::reference(lr);  // tmp cannot be lr
    hold_tmp.assign_register();
    RegisterAllocator::dereference(lr);
    Register tmp = hold_tmp.lo_register();
    GUARANTEE(tmp != lr, "Must not have lr as temporary register");
    GUARANTEE(!frame()->is_mapping_something(tmp), "Must be free");

    ldr(r0, r0, JavaNear::class_info_offset());
    // tmp = ClassInfo
    // IMPL_NOTE: how large can the constant be?
    //        Do we need to compute it more slowly.  See comment above
    ldr(r0, r0, vtable_index * 4 + ClassInfoDesc::header_size());

    if (is_heap_method) {
      ldr(tmp, r0, Method::heap_execution_entry_offset());
    } else {
      ldr(tmp, r0, Method::variable_part_offset());
      ldr(tmp, tmp);
    }
    frame()->flush(JVM_SINGLE_ARG_CHECK);
    call_from_compiled_code(tmp, 0, size_of_parameters JVM_CHECK);
  }

  adjust_for_invoke(size_of_parameters, return_type);
}

void CodeGenerator::invoke_interface(JavaClass* klass, int itable_index,
                                     int parameters_size,
                                     BasicType return_type JVM_TRAPS) {
  Value receiver(T_OBJECT);
  frame()->receiver(receiver, parameters_size);
  if (receiver.must_be_null()) {
    // We are guaranteed to be in the test suite.  The receiver is known to
    // be null at compile time!  This just isn't worth worrying about
    go_to_interpreter(JVM_SINGLE_ARG_CHECK);
    return;
  }

  const bool can_skip_null_check = ENABLE_ARM_V7 &&
    is_inline_exception_allowed(ThrowExceptionStub::rte_null_pointer JVM_CHECK);
  if (!can_skip_null_check) {
    maybe_null_check(receiver JVM_CHECK);
  }

  Register recv = receiver.lo_register();
  receiver.destroy();
  ldr(RegisterAllocator::allocate(tmp0), recv);

  Value tmp(T_OBJECT);
  tmp.set_register(tmp0);

  // Flush the virtual stack frame and unmap everything.
  frame()->flush(JVM_SINGLE_ARG_CHECK);
#if ENABLE_ARM_V7
  if (UseHandlers) {
    mov(tos_tag, klass->class_id());
    hbl_with_parameter(invoke_interface_handler, itable_index);
    write_call_info(parameters_size JVM_CHECK);
  } else
#endif
  {
    ldr(tmp0, tmp0, 0);
    ldr(tmp0, tmp0, JavaClass::class_info_offset());
    ldrh(tmp1, tmp0, ClassInfo::vtable_length_offset());

    add(tmp2, tmp0, imm12(ClassInfoDesc::header_size() - 2 * BytesPerWord));
    add(tmp1, tmp2, tmp1, lsl_shift, 2);

    ldrh(tmp5, tmp0, ClassInfo::itable_length_offset());

    mov(tos_tag, klass->class_id());

    // Lookup interface method table by linear search
    Label lookup;
  bind(lookup);
    sub(tmp5, tmp5, one);

    IncompatibleClassChangeStub* icc_error =
      IncompatibleClassChangeStub::allocate_or_share(JVM_SINGLE_ARG_ZCHECK(icc_error));
    b(icc_error, lt);

    Macros::ldr(tos_val, tmp1, PreIndex(2 * BytesPerWord));

    cmp(tos_val, reg(tos_tag));
    b(lookup, ne);

    write_literals_if_desperate();

    // Found the itable entry - now get the method table offset from there
    ldr(tmp1, tmp1, BytesPerWord);

    // Now get the method
    addw_imm12_w(tos_val, tmp0, BytesPerWord * itable_index);

    // Cannot use ldr_regs because implicit shift is applied in Xenon mode
    ldr_w(tos_val, tos_val, tmp1, 0);

    // Get the method entry from the method.
    // We require that tos_val contain the method
    ldr(tmp5, tos_val, Method::variable_part_offset());
    ldr(tmp5, tmp5);
    call_from_compiled_code(tmp5, 0, parameters_size JVM_CHECK);
  }

  adjust_for_invoke(parameters_size, return_type);
}

void CodeGenerator::adjust_for_invoke(int parameters_size,
                                      BasicType return_type, bool native) {
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
  Register reg_kni_params = RegisterAllocator::allocate();
  if (method()->size_of_parameters() > 0) {
    LocationAddress base(0, T_OBJECT); // Type is immaterial
    int offset = base.get_fixed_offset(); // Offset from fp of first argument
    if (method()->is_static()) {
      // KNI-ism, fake parameter slot for static method
      offset += -JavaStackDirection * BytesPerStackElement;
      NOT_PRODUCT(comment("Set _kni_parameter_base (static method)"));
    } else {
      NOT_PRODUCT(comment("Set _kni_parameter_base (virtual method)"));
    }
    if (!ENABLE_ARM_V7) {
      add(reg_kni_params, fp, offset);
    } else if (base.fixed_register() == jsp) {
      add(reg_kni_params, jsp, offset);
    } else {
      add(reg_kni_params, fp, imm12(offset));
    }
  } else {
    GUARANTEE(method()->is_static(), "Of course");
    ldr_address(reg_kni_params, 0);
  }
  set_kni_parameter_base(reg_kni_params);
  RegisterAllocator::dereference(reg_kni_params);

  NOT_PRODUCT(comment("invoke native method"));
  call_vm(entry, return_kind JVM_CHECK);

#if ENABLE_ARM_V7
  RegisterAllocator::reference(r0);
  if (return_kind == T_LONG || return_kind == T_DOUBLE) {
    RegisterAllocator::reference(r1);
  }
#else
  mov_hi(r9, r0);
  if (return_kind == T_LONG || return_kind == T_DOUBLE) {
    mov_hi(r10, r1);
  }
#endif

  NOT_PRODUCT(comment("Check if native method needs redoing"));
  Register reg_zero = RegisterAllocator::allocate();
  Register reg_thread = RegisterAllocator::allocate();
  Register reg_async_redo = RegisterAllocator::allocate();

  get_thread(reg_thread);
  ldr_address(reg_zero, 0);
  ldr(reg_async_redo, reg_thread, Thread::async_redo_offset());
  str(reg_zero, reg_thread, Thread::async_redo_offset());
  cmp(reg_async_redo, 0);
  b(redo, ne);

  NOT_PRODUCT(comment("Clear Thread::async_info"));
  str(reg_zero, reg_thread, Thread::async_info_offset());

  RegisterAllocator::dereference(reg_zero);
  RegisterAllocator::dereference(reg_async_redo);
  RegisterAllocator::dereference(reg_thread);

#if ENABLE_ARM_V7
  RegisterAllocator::dereference(r0);
  if (return_kind == T_LONG || return_kind == T_DOUBLE) {
    RegisterAllocator::dereference(r1);
  }
#else
  mov_hi(r0, r9);
  if (return_kind == T_LONG || return_kind == T_DOUBLE) {
    mov_hi(r1, r10);
  }
#endif

  adjust_for_invoke(0, return_kind, true);
}

void CodeGenerator::call_from_compiled_code(Register dst, int offset,
                                            int parameters_size,
                                            bool indirect, bool add_lr
                                            JVM_TRAPS) {
  GUARANTEE(dst != lr, "Register lr will be destroyed");

  Register tmp_reg = r7;
  RegisterAllocator::reference(tmp_reg);

  if (indirect) {
    ldr(tmp_reg, dst, offset);
    dst = tmp_reg;
    offset = 0;
  }

  if (offset != 0) {
    GUARANTEE(dst != gp, "call_from_compiled_code: bashing gp");
    add(dst, dst, offset);
  }

  // We need tmp_reg below to calculate the
  // return address. If the dst is same
  // as tmp_reg then make use of r12
#if ENABLE_ARM_V7
  blx(dst);  // all code called by compiled code must be in Xenon mode
  RegisterAllocator::dereference(tmp_reg);
  write_call_info(parameters_size JVM_CHECK);
#else
  if (dst == tmp_reg) {
    mov_hi(r12, dst);
    dst = r12;
  }

  int code_offset = code_size(); // current pc
  add_pc_imm8x4(tmp_reg, 0); // immediate filled in at bottom
  if (add_lr) {
    add(tmp_reg, tmp_reg, 1);
  } else {
    // The callee knows it's being called from Thumb code, and will add 1 to
    // lr on our behalf. E.g., compiler_new_object
  }
  mov_hi(lr, tmp_reg);
  bx(dst);
  RegisterAllocator::dereference(tmp_reg);
  write_literals();
  if ((code_size() & 0x3) != 0) {
    nop();
  }

  write_call_info(parameters_size JVM_CHECK);
  // Patch the "add" instruction to make lr point at following instruction
  if (!has_overflown_compiled_method()) {
    // add_pc rounds down pc to word boundary
    int new_imm = code_size() - (code_offset & ~3) - 4;
    GUARANTEE((new_imm % 4) == 0 && new_imm > 0 &&
              has_room_for_imm(abs(new_imm/4), 8), "Invalid lr patching");
    *(short *)addr_at(code_offset) |= (new_imm/4);
  }
#endif
}

void CodeGenerator::call_through_gp(address& target, bool add_lr JVM_TRAPS) {
  Register dst_reg = r2;
  if (RegisterAllocator::is_referenced(r2) ||
      RegisterAllocator::is_mapping_something(r2)) {
    dst_reg = r7;
  }
  RegisterAllocator::reference(dst_reg);

  ldr_using_gp(dst_reg, (address)&target);
  call_from_compiled_code(dst_reg, 0, 0, false, add_lr
                          JVM_NO_CHECK_AT_BOTTOM);

  RegisterAllocator::dereference(dst_reg);
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
    TypeArray::Raw extended_tag_info =
      frame()->generate_callinfo_stackmap(JVM_SINGLE_ARG_CHECK);
    for (int i = extended_tag_info().length() - 1; i >= 0; i--) {
      emit_int(extended_tag_info().int_at(i));
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
  // Fast exception catching not implemented on thumb2.
  return false;
}

void CodeGenerator::call_vm_extra_arg(const int extra_arg) {
  mov(r1, extra_arg);
}

void CodeGenerator::call_vm_extra_arg(const Register extra_arg) {
  mov(r1, extra_arg);
}

void CodeGenerator::call_vm(address entry, BasicType return_value_type JVM_TRAPS) {
  // all registers must be flushed (not necessarily unmapped) before calling
  // call_vm
  write_literals_if_desperate();
  if (return_value_type != T_ILLEGAL) {
    RegisterAllocator::reference(r3);
    ldr_address(r3, entry);
  }
  NOT_PRODUCT(comment("call vm"));
  if (stack_type_for(return_value_type) == T_OBJECT) {
#if ENABLE_ISOLATES
#ifndef PRODUCT
    if (StopAtCIBHit && entry == (address)compiled_code_task_barrier) {
      breakpoint();
    }
#endif
#endif
    call_through_gp(gp_shared_call_vm_oop_ptr JVM_NO_CHECK_AT_BOTTOM);
  } else if (return_value_type == T_ILLEGAL) {
    call_through_gp(gp_shared_call_vm_exception_ptr JVM_NO_CHECK_AT_BOTTOM);
  } else {
    call_through_gp(gp_shared_call_vm_ptr JVM_NO_CHECK_AT_BOTTOM);
  }

  if (return_value_type != T_ILLEGAL) {
    RegisterAllocator::dereference(r3);
  }
}

void CodeGenerator::type_check(Value& array, Value& index, Value& object JVM_TRAPS)
{
  Label slow_case, done_checking;

  NOT_PRODUCT(comment("Array store type check"));

  frame()->push(array);
  frame()->push(index);
  frame()->push(object);

  // Since register allocation might cause spilling we have to allocate *all*
  // registers before checking for for null object.

  const TempRegister tmp1;
  const TempRegister tmp2;

  ldr(tmp2, array.lo_register());
  // Check for null object.
  cmp(object.lo_register(), zero);
  b(done_checking, eq);

  ldr(tmp1, object.lo_register(), 0);

  // Get the class and the element class of the array
  ldr(tmp2, tmp2);
  ldr(tmp1, tmp1);
  ldr(tmp2, tmp2, ObjArrayClass::element_class_offset());

  mov_hi(r12, tmp2);
  // Fast check against the subtype check caches.
  ldr(tmp2, tmp1, JavaClass::subtype_cache_1_offset());
  ldr(tmp1, tmp1, JavaClass::subtype_cache_2_offset());
  cmp_hi(tmp2, r12);
  NearLabel skip_cmp;
  b(skip_cmp, eq);
  cmp_hi(tmp1, r12);
  bind(skip_cmp);
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
CodeGenerator::convert_condition(BytecodeClosure::cond_op condition)  {
  switch (condition) {
    case BytecodeClosure::null   : // fall through
    case BytecodeClosure::eq     : return eq;
    case BytecodeClosure::nonnull: // fall through
    case BytecodeClosure::ne     : return ne;
    case BytecodeClosure::lt     : return lt;
    case BytecodeClosure::le     : return le;
    case BytecodeClosure::gt     : return gt;
    case BytecodeClosure::ge     : return ge;
  }
  SHOULD_NOT_REACH_HERE();
  return nv;
}

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
  Register srcReg[6];
  Register dstReg[6];
  int regCount = 0;

#if ENABLE_ARM_VFP
  jbyte floatMap[32];
  juint floatMask = 0;
#endif

  Register dstImm[6];
  jint     srcImm[6];
  int immCount = 0;

  for(int targetRegister = 0;;) {
    Value* value = va_arg(ap, Value*);
    if (value == NULL) break;
    if (value->type() == T_ILLEGAL) {
      // just a place holder.  We'll fill in the register late
    } else if (value->in_register()) {
#if ENABLE_ARM_VFP
      if( Assembler::is_vfp_register( value->lo_register() ) ) {
        int lo = value->lo_register() - s0;
        floatMap[lo] = jbyte(targetRegister);
        floatMask |= 1 << lo;
        if( value->is_two_word() ) {
          ++lo;
          floatMap[lo] = jbyte(targetRegister+1);
          floatMask |= 1 << lo;
        }
      } else
#endif
      {
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
      mov(dstReg[0], srcReg[0]);
    } else {
      shuffle_registers(dstReg, srcReg, regCount);
    }
  }

  // Write the immediate values.
  {
    for( int i = 0; i < immCount; i++ ) {
      mov(dstImm[i], srcImm[i]);
    }
  }

#if ENABLE_ARM_VFP
  {
    for( int i = 0; floatMask; i++, floatMask >>= 1 ) {
      if( floatMask & 1 ) {
        const Register dst = as_register( floatMap[i] );
        if( floatMask & 2 ) {
          fmrrd( dst, Register(floatMap[i+1]), Register(s0 + i) );
          floatMask >>= 1;
          i++;
        } else {
          fmrs( dst, Register(s0 + i) );
        }
      }
    }
  }
#endif
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
  Register scratch = r8;
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
      srcReg[0] = scratch; // Workaround for: srcReg[0] = scratch;
    }
  }
}


void CodeGenerator::load_from_object(Value& result, Value& object, jint offset,
                                    bool null_check JVM_TRAPS) {
  bool could_use_xenon_features = ENABLE_ARM_V7 &&
     is_inline_exception_allowed(ThrowExceptionStub::rte_null_pointer JVM_CHECK);
  //we don't need to check for null implicitly, cause load would cause
  //jump to npe handler in xenon
  if (null_check && !could_use_xenon_features) {
    maybe_null_check(object JVM_CHECK);
  }

  FieldAddress address(object, offset, result.type());
  load_from_address(result, result.type(), address);
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

#if ENABLE_ARM_V7
void CodeGenerator::load_from_array(Value& result, Value& array, Value& index) {
  const BasicType type = result.type();
  if (type == T_BOOLEAN || type == T_BYTE || type == T_LONG || type == T_DOUBLE) {
    IndexedAddress address(array, index, type);
    load_from_address(result, type, address);
    return;
  }

  write_literals_if_desperate();

  // illegal types do not require any loading
  if (type == T_ILLEGAL) {
    return;
  }

  GUARANTEE(stack_type_for(type) == result.stack_type(),
            "types must match (taking stack types into account)");
  result.assign_register();
  GUARANTEE(array.in_register(), "we assume it here");
  if (index.is_immediate())  {
    int array_index = index.as_int();
    int offset = array_index * byte_size_for(type) + Array::base_offset();

    if (has_room_for_imm(offset/2, 5) && type == T_CHAR) {
      ldrh(result.lo_register(), array.lo_register(), offset);
      index.destroy();
      array.destroy();
      return;
    } else if (has_room_for_imm(offset/4, 5) && type != T_SHORT) {
      ldr(result.lo_register(), array.lo_register(), offset);
      index.destroy();
      array.destroy();
      return;
    }
    index.assign_register();
    mov(index.lo_register(), array_index);
  }

  GUARANTEE(index.in_register(), "we assume it here");
  switch(type) {
  case T_CHAR    :
      add_imm3(result.lo_register(), index.lo_register(), Array::base_offset() / 2);
      Assembler::ldrh(result.lo_register(), array.lo_register(), result.lo_register());
      break;

  case T_SHORT   :
      add_imm3(result.lo_register(), index.lo_register(), Array::base_offset() / 2);
      Assembler::ldrsh(result.lo_register(), array.lo_register(), result.lo_register());
      break;

  case T_INT     : // fall through
  case T_FLOAT   : // fall through
  case T_ARRAY   : // fall through
  case T_OBJECT  :
      add_imm3(result.lo_register(), index.lo_register(), Array::base_offset() / 4);
      Assembler::ldr(result.lo_register(), array.lo_register(), result.lo_register());
      break;

  default        :
      SHOULD_NOT_REACH_HERE();
      break;
  }
  index.destroy();
  array.destroy();
}

void CodeGenerator::store_to_array(Value& value, Value& array, Value& index) {
  const BasicType type = value.type();
  IndexedAddress address(array, index, type);
  if (type == T_BOOLEAN || type == T_BYTE || type == T_LONG || type == T_DOUBLE) {
    store_to_address(value, type, address);
    return;
  }

  if (!RegisterAllocator::has_free((value.is_immediate() ? 2 : 1) + (index.is_immediate() ? 1 : 0))) {
    store_to_address(value, type, address);
    return;
  }

  write_literals_if_desperate();

  // if the value to store isn't present there nothing left to do
  if (!value.is_present()) {
    return;
  }

  GUARANTEE(stack_type_for(type) == value.stack_type(),
             "types must match (taking stack types into account)");
  Register value_reg;
  const CompilerLiteralAccessor cla;

  if (value.is_immediate()) {
    value_reg = cla.get_literal(value.lo_bits());
    // We must use ::reference, not ::allocate, since the latter flushes the
    // register from the frame!
    // We need to make sure that this register isn't flushed when doing the
    // address calculations.
    RegisterAllocator::reference(value_reg);
  } else {
    value_reg = value.lo_register();
  }

  if (index.is_immediate())  {
    int array_index = index.as_int();
    int offset = array_index * byte_size_for(type) + Array::base_offset();

    if (has_room_for_imm(offset/2, 5) && (type == T_CHAR || type == T_SHORT) ) {
      strh(value_reg, array.lo_register(), offset);
      if (value_reg != no_reg && value.is_immediate()) {
        RegisterAllocator::dereference(value_reg);
      }
      return;
    } else if (has_room_for_imm(offset/4, 5) && (type != T_SHORT && type != T_CHAR) ) {
      str(value_reg, array.lo_register(), offset);
      if (value_reg != no_reg && value.is_immediate()) {
        RegisterAllocator::dereference(value_reg);
      }
      if (type == T_OBJECT || type == T_ARRAY) {
        Register temp_reg = RegisterAllocator::allocate();
        add(temp_reg, array.lo_register(), offset);
        if (UseHandlers) {
          GUARANTEE(r0 <= temp_reg && temp_reg <= r7, "Must be low regster");
          hbl(CodeGenerator::write_barrier_handler_r0 + (int)temp_reg);
        } else {
          Assembler::Register tmp1 = RegisterAllocator::allocate();
          Assembler::Register tmp2 = RegisterAllocator::allocate();
          Assembler::Register tmp3 = Assembler::r12;
          oop_write_barrier(temp_reg, tmp1, tmp2, tmp3, false);
          RegisterAllocator::dereference(tmp1);
          RegisterAllocator::dereference(tmp2);
        }
        RegisterAllocator::dereference(temp_reg);
      }
      return;
    }

    index.assign_register();
    mov(index.lo_register(), array_index);
  }
  Register temp_reg = RegisterAllocator::allocate();
  GUARANTEE(temp_reg != no_reg, "sanity");
  switch(type) {
    case T_CHAR    : // fall through
    case T_SHORT   :
      if (temp_reg < r8) {
        add_imm3(temp_reg, index.lo_register(), Array::base_offset() / 2);
        Assembler::strh(value_reg, array.lo_register(), temp_reg);
      } else {
        add(temp_reg, index.lo_register(), Array::base_offset());
        strh_w(value_reg, array.lo_register(), temp_reg, 0);
      }
      break;

    case T_ARRAY   :
    case T_OBJECT  :
      GUARANTEE(value.in_register() || value.must_be_null(),
                "Only NULLs can be immediate");
      if (!value.not_on_heap()) {
        // Need to do pointer setting
        // prepare and load data at lo address
        if (temp_reg < r8) {
          add_imm3(temp_reg, index.lo_register(), Array::base_offset() / 4);
          Assembler::str(value_reg, array.lo_register(), temp_reg);
        } else {
          add(temp_reg, index.lo_register(), Array::base_offset());
          strh_w(value_reg, array.lo_register(), temp_reg, 0);
        }
        if (value.is_immediate()) {
          RegisterAllocator::dereference(value_reg);
          value_reg = no_reg;
        }
        // Free up register that's not needed any more.  Pointer setting
        // uses up lots of registers, and we want to minimize pressure
        value.destroy();

        add(temp_reg, array.lo_register(), temp_reg, lsl_shift, 2);
        if (UseHandlers) {
          GUARANTEE(r0 <= temp_reg && temp_reg <= r7, "Must be low regster");
          hbl(CodeGenerator::write_barrier_handler_r0 + (int)temp_reg);
        } else {
          Assembler::Register tmp1 = RegisterAllocator::allocate();
          Assembler::Register tmp2 = RegisterAllocator::allocate();
          Assembler::Register tmp3 = Assembler::r12;
          oop_write_barrier(temp_reg, tmp1, tmp2, tmp3, false);
          RegisterAllocator::dereference(tmp1);
          RegisterAllocator::dereference(tmp2);
        }
        break;
      }

    case T_FLOAT   :
    case T_INT     :
        if (temp_reg < r8) {
          add_imm3(temp_reg, index.lo_register(), Array::base_offset() / 4);
          Assembler::str(value_reg, array.lo_register(), temp_reg);
        } else {
          add(temp_reg, index.lo_register(), Array::base_offset());
          strh_w(value_reg, array.lo_register(), temp_reg, 0);
        }
      break;

    default        :
        SHOULD_NOT_REACH_HERE();
        break;
  }
  RegisterAllocator::dereference(temp_reg);
  if (value_reg != no_reg && value.is_immediate()) {
    RegisterAllocator::dereference(value_reg);
  }
}
#endif


#ifndef PRODUCT
void
CodeGenerator::verify_location_is_constant(jint index, const Value& constant) {
  // IMPL_NOTE: need revisit
  // CompilerLiteralAccessor cla;
  LocationAddress address(index, constant.type());
  const TempRegister tmp;
  Assembler::Register address_reg;
  int address_offset = 0;
  address.get_preindexed_address(true, address_reg, address_offset);
  ldr(tmp, address_reg, address_offset);
  cmp(tmp, constant.lo_bits());
  breakpoint(ne);
  if (constant.is_two_word()) {
    Assembler::Register address_reg;
    int address_offset;
    address.get_preindexed_address(false, address_reg, address_offset);
    ldr(tmp, address_reg, address_offset);
    cmp(tmp, constant.hi_bits());
  }
  breakpoint(ne);
}
#endif

#if CROSS_GENERATOR && !ENABLE_ISOLATES

void CodeGenerator::initialize_class(InstanceClass* klass JVM_TRAPS) {
  GUARANTEE(klass->not_null() && !klass->is_initialized(),
            "Should only be called for non-initialized classes");
  // initialize_class(Thread&, raw_class);
  NOT_PRODUCT(comment("Initialize class if needed"));
  NOT_PRODUCT(comment("Flush frame"));
  flush_frame(JVM_SINGLE_ARG_CHECK);

  Label class_initialized;

  NOT_PRODUCT(comment("Load class"));
  {
    const TempRegister klass_reg(r1);
    ldr_oop(klass_reg, klass);

    NOT_PRODUCT(comment("Quick check if the class is initialized"));
    {
      const TempRegister tmp; ldr(tmp, klass_reg, JavaClass::java_mirror_offset()); ldr(tmp, tmp,
      JavaClassObj::status_offset()); tst(tmp, JavaClassObj::INITIALIZED); b(class_initialized, ne);
    }
  }

  NOT_PRODUCT(comment("Class is not initialized - initialize it"));
#if ENABLE_ARM_V7
  if (UseHandlers) {
    write_literals_if_desperate();
    hbl(init_class_handler);
    write_call_info(0 JVM_CHECK);
  } else
#endif
  {
    call_vm((address) ::initialize_class, T_VOID JVM_NO_CHECK_AT_BOTTOM);
  }

  bind(class_initialized);
}
#endif

#if !ENABLE_CPU_VARIANT
void CodeGenerator::init_static_array(Value& result JVM_TRAPS) {
  // Flush the virtual stack frame.
  flush_frame(JVM_SINGLE_ARG_CHECK);

  const Register src = tos_tag;
  const Register dst = tos_val;
  const Register size = tmp0;

  RegisterAllocator::reference(src);
  RegisterAllocator::reference(dst);
  RegisterAllocator::reference(size);

  Method::Raw cur_method =
    Compiler::current()->current_compiled_method()->method();
  ldr_oop(src, &cur_method);

  add(dst, result.lo_register(), Array::base_offset());
  NOT_PRODUCT(cmp(result.lo_register(), zero);)
  NOT_PRODUCT(breakpoint(eq);)

  add(src, src, Method::base_offset() + bci());

  RegisterAllocator::reference(tmp1);
  // Loading elements count low byte.
  ldrb(tmp1, src, 2);
  // Loading elements count hi byte.
  ldrb(size, src, 3);

  RegisterAllocator::reference(tmp5);
  ldrb(tmp5, src, 1); //load log bytes for array type
  add(src, src, 4);

  lsl_imm5(size, size, BitsPerByte);
  orr(size, tmp1);
  RegisterAllocator::dereference(tmp1);

  lsl(size, tmp5);
  RegisterAllocator::dereference(tmp5);

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

void CodeGenerator::check_cast_stub(CompilationQueueElement* cqe JVM_TRAPS) {
  int class_id = CheckCastStub::cast(cqe)->class_id();
  RegisterAllocator::reference(tmp0);
  mov(tmp0, class_id);
  call_through_gp(gp_compiler_checkcast_ptr JVM_NO_CHECK_AT_BOTTOM);
  RegisterAllocator::dereference(tmp0);

}

void CodeGenerator::instance_of_stub(CompilationQueueElement* cqe JVM_TRAPS) {
  int class_id = InstanceOfStub::cast(cqe)->class_id();
  mov(r0, class_id);
  call_through_gp(gp_compiler_instanceof_ptr JVM_NO_CHECK_AT_BOTTOM);
}

#if ENABLE_INLINE_COMPILER_STUBS
void CodeGenerator::new_object_stub(CompilationQueueElement* cqe JVM_TRAPS) {
  Assembler::Register jnear = NewObjectStub::cast(cqe)->java_near();
  ldr(r1, jnear, JavaNear::klass_offset());
  call_through_gp(gp_compiler_new_object_ptr JVM_NO_CHECK_AT_BOTTOM);
}

void CodeGenerator::new_type_array_stub(CompilationQueueElement* cqe JVM_TRAPS) {
  Assembler::Register jnear = NewTypeArrayStub::cast(cqe)->java_near();
  Assembler::Register length = NewTypeArrayStub::cast(cqe)->length();
  if (length != r1) {
    ldr(r1, jnear, JavaNear::klass_offset());
    mov(r2, length);
  } else if (jnear != r2) {
    mov(r2, length);
    ldr(r1, jnear, JavaNear::klass_offset());
  } else {
    Assembler::Register tmp = NewTypeArrayStub::cast(cqe)->result_register();
    ldr(tmp, jnear, JavaNear::klass_offset());
    mov(r2, length);
    mov(r1, tmp);
  }
  call_through_gp(gp_compiler_new_type_array_ptr JVM_NO_CHECK_AT_BOTTOM);
}
#endif // ENABLE_INLINE_COMPILER_STUBS

#endif
