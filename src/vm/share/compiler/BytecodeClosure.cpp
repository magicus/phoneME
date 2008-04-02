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

# include "incls/_precompiled.incl"
# include "incls/_BytecodeClosure.cpp.incl"

void BytecodeClosure::initialize(Method* method) {
  _method = method->obj();
  _cp     = method->constants();
}

void BytecodeClosure::illegal_code(JVM_SINGLE_ARG_TRAPS) { 
    JVM_IGNORE_TRAPS;
    SHOULD_NOT_REACH_HERE();
}

const BytecodeClosure::cond_op 
BytecodeClosure::reverse_condition_table[BytecodeClosure::number_of_cond_ops] =
{
  BytecodeClosure::null,    // null
  BytecodeClosure::nonnull, // nonnull, 
  BytecodeClosure::eq,      // eq,
  BytecodeClosure::ne,      // ne,
  BytecodeClosure::gt,      // lt,
  BytecodeClosure::le,      // ge
  BytecodeClosure::lt,      // gt, 
  BytecodeClosure::ge,      // le,
};

const BytecodeClosure::cond_op 
BytecodeClosure::negate_condition_table[BytecodeClosure::number_of_ext_cond_ops]=
{
  BytecodeClosure::nonnull, // null,
  BytecodeClosure::null,    // nonnull, 
  BytecodeClosure::ne,      // eq,
  BytecodeClosure::eq,      // ne,
  BytecodeClosure::ge,      // lt,
  BytecodeClosure::lt,      // ge
  BytecodeClosure::le,      // gt, 
  BytecodeClosure::gt,      // le,
#if ENABLE_CONDITIONAL_BRANCH_OPTIMIZATIONS
  BytecodeClosure::positive,// negative,
  BytecodeClosure::negative,// positive,
#endif
};

void BytecodeClosure::aload_0_fast_get_field_1(BasicType field_type JVM_TRAPS){
  load_local(T_OBJECT, 0 JVM_CHECK);
  int offset = method()->get_ubyte(_bci+1) * BytesPerWord;
  fast_get_field(field_type, offset JVM_CHECK);
}

#if !ENABLE_CPU_VARIANT
void BytecodeClosure::aload_0_fast_get_field_n(int bytecode JVM_TRAPS) {
  load_local(T_OBJECT, 0 JVM_CHECK);

  const static jbyte types[4] = {T_OBJECT, T_INT, T_OBJECT, T_INT};
  const static jbyte sizes[4] = {4, 4, 8, 8};
  const int index = bytecode - Bytecodes::_aload_0_fast_agetfield_4;

  BasicType field_type = (BasicType)types[index];
  int offset = (int)sizes[index];
  fast_get_field(field_type, offset JVM_CHECK);
}
#endif

void  BytecodeClosure::pop_and_npe_if_null(JVM_SINGLE_ARG_TRAPS) {
  pop(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}
