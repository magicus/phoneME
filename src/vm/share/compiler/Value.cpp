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
#include "incls/_Value.cpp.incl"

#if ENABLE_COMPILER
void Value::set_int(jint value) {
  GUARANTEE(type() == T_BOOLEAN ||
            type() == T_INT ||
            type() == T_BYTE ||
            type() == T_CHAR ||
            type() == T_SHORT, "check type");
  destroy();
  set_where(T_IMMEDIATE);
  _low = value;
}

#if ENABLE_FLOAT
void Value::set_float(jfloat value) {
  GUARANTEE(type() == T_FLOAT, "check type");
  destroy();
  set_where(T_IMMEDIATE);
  *(float *)(&_low) = value;
}

void Value::set_double(jdouble value) {
  GUARANTEE(type() == T_DOUBLE, "check type");
  destroy();
  set_where(T_IMMEDIATE);
  *(double *)(&_low) = value;

}
#endif

void Value::set_long(jlong value) {
  GUARANTEE(type() == T_LONG, "check type");
  destroy();
  set_where(T_IMMEDIATE);
  *(jlong *)(&_low) = value;
}

#if ENABLE_FLOAT
void Value::set_raw_int(jint value) {
  GUARANTEE(is_one_word(), "check type");
  destroy();
  set_where(T_IMMEDIATE);
  _low = value;
}

void Value::set_raw_long(jlong value) {
  GUARANTEE(is_two_word(), "check type");
  destroy();
  set_where(T_IMMEDIATE);
  *(jlong *)(&_low) = value;
}
#endif

void Value::set_obj(Oop* value) {
  // due to garbage collection considerations object immediates
  // are always put in a register
  GUARANTEE(type() == T_OBJECT || type() == T_ARRAY, "check type");
  // assign a register to this value
  if (value->is_null()) {
    set_where(T_IMMEDIATE);
    set_must_be_null();
    set_not_on_heap();
    _low = 0;
  } else {
    assign_register();
    // move the immediate object into the register
    Compiler::code_generator()->move(*this, value);
    if (!ObjectHeap::contains_moveable(value->obj())) {
      set_not_on_heap();
    }
#if ENABLE_COMPILER_TYPE_INFO
    FarClass::Raw value_class = value->blueprint();
    GUARANTEE(value_class.not_null(), "Sanity");
    if (value_class.is_java_class()) {
      JavaClass::Raw java_class = value_class.obj();
      set_class_id(java_class().class_id());
      set_is_exact_type();
    }
#else
    if (value->is_string()) {
      set_is_string();
    }
#endif
    set_must_be_nonnull();
  }
}

void ExtendedValue::set_obj(Oop* value) {
  if (value->is_null()) {
    is_value(T_OBJECT);
    this->value().set_obj(value);
  } else {
    // IMPL_NOTE:  Can we merge the code below with Value::set_value() ??
    set_oop(value);
    if (!ObjectHeap::contains_moveable(value->obj())) {
      _value.set_not_on_heap();
    }
#if ENABLE_COMPILER_TYPE_INFO
    FarClass::Raw value_class = value->blueprint();
    GUARANTEE(value_class.not_null(), "Sanity");
    if (value_class.is_java_class()) {
      JavaClass::Raw java_class = value_class.obj();
      _value.set_class_id(java_class().class_id());
      _value.set_is_exact_type();
    }
#else
    if (value->is_string()) {
      _value.set_is_string();
    }
#endif
    _value.set_must_be_nonnull();
  }
}


void Value::set_immediate_from_static_field(InstanceClass* ic, int offset) {
  TypeArray::Raw f = ic->fields();
  Oop::Raw object;

  for (int index = 0; index < f().length(); index += 5) {
    Field static_field(ic, index);

    if (static_field.offset() == offset) {
      // Note: there may be a non-static field with the same 'offset'
      // as the static field we're trying to find.
      if (static_field.is_final() && static_field.is_static()) {
        switch (static_field.type()) {
          case T_BOOLEAN : // fall-through - stored as 32-bit
          case T_CHAR    : // fall-through - stored as 32-bit
          case T_BYTE    : // fall-through - stored as 32-bit
          case T_SHORT   : // fall-through - stored as 32-bit
          case T_INT     : set_int(ic->int_field(offset));       break;

          case T_ARRAY   : // fall-through
          case T_OBJECT  :
            // Note: if setting immediate for object fields is enabled for cross
            // generator, this reference will be written to the TEXT block as a
            // part of relocation information for the compiled method. The
            // problem is that the referenced object may be put to the HEAP
            // block and it would be impossible to write its reference
            // in the TEXT block. At this point we don't know the block type of
            // the referenced object, so for now we disable setting immediate
            // for cross generator.
            if (!USE_AOT_COMPILATION || !GenerateROMImage) {
              object = ic->obj_field(offset); 
              set_obj(&object);
            }
            break;
#if ENABLE_FLOAT
          case T_FLOAT   : set_float(ic->float_field(offset));   break;
          case T_DOUBLE  : set_double(ic->double_field(offset)); break;
#endif
          case T_LONG    : set_long(ic->long_field(offset));     break;
          default        : SHOULD_NOT_REACH_HERE();              break;
        }
      }
      return;
    }
  }

  // This field is not final and was removed from the fields array
  // during romization
}

void Value::set_register(Assembler::Register reg) {
  destroy();
  GUARANTEE(type() != T_LONG, "tag check");
  set_where(T_REGISTER);
  _low = (int) reg;
}

void Value::set_registers(Assembler::Register low, Assembler::Register high) {
  destroy();
  GUARANTEE(use_two_registers(), "tag_check");
  set_where(T_REGISTER);
  _low  = (int) low;
  _high = (int) high;
}

#if ENABLE_ARM_VFP
void Value::set_vfp_double_register(Assembler::Register reg) {
  GUARANTEE(reg >= Assembler::d0, "Not a valid vfp double register");

  // Get the real registers reg uses
  Assembler::Register low  = reg;
  Assembler::Register high = (Assembler::Register)((int)low + 1);
  set_registers(low, high);
}
#endif

#if  ARM || HITACHI_SH
void Value::assign_register() {
  if (in_register()) {
    return;
  }

#if ENABLE_ARM_VFP
  if (stack_type() == T_FLOAT) {
    set_register(RegisterAllocator::allocate_float_register());
  } else if (stack_type() == T_DOUBLE) {
    set_vfp_double_register(RegisterAllocator::allocate_double_register());
  } else
#endif
  {
    if (!is_two_word()) {
      set_register(RegisterAllocator::allocate());
    } else {
      // NOTE: avoid doing this: set_registers(allocate(), allocate());
      // The order of parameter list evaluation is undefined in C, and
      // on linux/i386 and solaris/sparc the orders are opposite. The following
      // code forces the same order, so AOT generator will generate exact
      // same code on both Linux and Solaris hosts.
      Assembler::Register hi  = RegisterAllocator::allocate();
      Assembler::Register low = RegisterAllocator::allocate();
      set_registers(low, hi);
    }
  }
}

#else

void Value::assign_register() {
  if (in_register()) return;

  switch(stack_type()) {
    case T_OBJECT : // fall through
    case T_ARRAY  : // fall through
    case T_INT    : set_register(RegisterAllocator::allocate());
                    break;
    case T_LONG   : set_registers(RegisterAllocator::allocate(), RegisterAllocator::allocate());
                    break;
#if ENABLE_ARM_VFP
    case T_FLOAT  : set_register(RegisterAllocator::allocate_float_register());
                    break;
    case T_DOUBLE : set_vfp_double_register(RegisterAllocator::allocate_double_register());
                    break;
#else  // !ENABLE_ARM_VFP
    case T_FLOAT  : // fall through
    case T_DOUBLE : set_register(RegisterAllocator::allocate_float_register());
                    break;
#endif  // ENABLE_ARM_VFP
    default       : SHOULD_NOT_REACH_HERE();
                    break;
  }
}

void Value::force_to_byte_register() {
  GUARANTEE(in_register(), "must be in register");
  if (!Assembler::is_valid_byte_register(lo_register())) {
    Assembler::Register byte_register = RegisterAllocator::allocate_byte_register();
    Compiler::code_generator()->movl(byte_register, lo_register());
    set_register(byte_register);
  }
}

#endif

void Value::materialize() {
  GUARANTEE(is_immediate(), "value must be immediate");
  GUARANTEE(type() != T_OBJECT || must_be_null(), "object immediate is already materialized");

  Value result(type());
  result.assign_register();
  Compiler::code_generator()->move(result, *this);
  result.copy(*this);
}

void Value::destroy() {
  if (in_register()) {
    RegisterAllocator::dereference(lo_register());
    if (use_two_registers()) {
      RegisterAllocator::dereference(hi_register());
    }
  }
  set_where(T_NOWHERE);
}

void Value::copy(Value& result) {
  GUARANTEE(in_register(), "value must be in register");
  if (use_two_registers()) {
    result.set_registers(lo_register(), hi_register());
    RegisterAllocator::reference(lo_register());
    RegisterAllocator::reference(hi_register());
    //cse
    RegisterAllocator::wipe_notation_of(lo_register());
    RegisterAllocator::wipe_notation_of(hi_register());
  } else {
    result.set_register(lo_register());
    RegisterAllocator::reference(lo_register());
    //cse
    RegisterAllocator::wipe_notation_of(lo_register());
  }
}

#if !ARM

void Value::writable_copy(Value& result) {
  GUARANTEE(in_register(), "value must be in register");
  VirtualStackFrame* frame = Compiler::current()->frame();

  if (use_two_registers()) {
    if ((RegisterAllocator::references(lo_register()) == 1) &&
        (RegisterAllocator::references(hi_register()) == 1)) {
      // use this value as the writable copy.
      RegisterAllocator::spill(hi_register());
      RegisterAllocator::spill(lo_register());
      frame->dirtify(hi_register());
      frame->dirtify(lo_register());
      result.set_registers(lo_register(), hi_register());
      // make sure we increment the reference counters
      RegisterAllocator::reference(lo_register());
      RegisterAllocator::reference(hi_register());
    } else {
      // allocate two new registers and copy the value of these registers into it.
      result.set_registers(RegisterAllocator::allocate(), RegisterAllocator::allocate());
      Compiler::code_generator()->move(result, *this);
    }
  } else {
   if (RegisterAllocator::references(lo_register()) == 1) {
      // use this value as the writable copy.
      RegisterAllocator::spill(lo_register());
      frame->dirtify(lo_register());
      result.set_register(lo_register());
      // make sure we increment the reference counter
      RegisterAllocator::reference(lo_register());
    } else {
      // allocate a new register and copy the value of this register into it.
      result.assign_register();
      Compiler::code_generator()->move(result, *this);
    }
  }
}
#endif // !defined(ARM)

#endif
