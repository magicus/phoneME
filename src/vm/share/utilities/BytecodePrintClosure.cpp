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
# include "incls/_BytecodePrintClosure.cpp.incl"

#if USE_DEBUG_PRINTING

static const char* type_name(BasicType t) {
  switch(t) {
    case T_BOOLEAN:   return "bool";
    case T_CHAR:      return "char";
    case T_FLOAT:     return "float";
    case T_DOUBLE:    return "double";
    case T_BYTE:      return "byte";
    case T_SHORT:     return "short";
    case T_INT:       return "int";
    case T_LONG:      return "long";
    case T_OBJECT:    return "obj";
    case T_ARRAY:     return "arr";
    case T_VOID:      return "void";
    case T_ILLEGAL:   return "##ILLEGAL##";
  }
  return "<unknown type>";
}

void BytecodePrintClosure::bytecode_prolog(JVM_SINGLE_ARG_TRAPS) {
  if (_verbose) {
    StackmapList stack_maps = method()->stackmaps();
    if (stack_maps.not_null()) {
      int count = stack_maps.entry_count();
      for (int i=0; i<count; i++) {
        if (stack_maps.get_bci(i) == bci()) {

          bool redundant =
              StackmapChecker::is_redundant(method(), i JVM_CHECK);
	  _st->print_cr(redundant ? "**REDUNDANT**" : "**NECESSARY**");

          _st->print("     ");
          stack_maps.print_entry_on(_st, i, false);
          _st->cr();
          break;
        }
      }
    }
  }
  _st->print("%3d: ", bci());
  const Bytecodes::Code code (current_bytecode());
  _st->print(_verbose ? "[ %-25s ] " : "%s ", Bytecodes::name(code));
}

void BytecodePrintClosure::bytecode_epilog(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  if (_include_nl) {
    _st->cr();
  }
}

void BytecodePrintClosure::push_int(jint value JVM_TRAPS) { 
  JVM_IGNORE_TRAPS;
  _st->print("%d (0x%x)", value, value);
}

void BytecodePrintClosure::push_long(jlong value JVM_TRAPS) { 
  JVM_IGNORE_TRAPS;
  _st->print(OsMisc_jlong_format_specifier(), value);
}

void BytecodePrintClosure::push_float(jfloat value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  _st->print_double(jvm_f2d(value));
}

void BytecodePrintClosure::push_double(jdouble value JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  _st->print_double(value); 
}

void BytecodePrintClosure::push_obj(Oop* value JVM_TRAPS) { 
  JVM_IGNORE_TRAPS;
  value->print_value_on(_st);
}

void BytecodePrintClosure::load_local(BasicType kind, int index JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  (void)kind;
  const Bytecodes::Code code( current_bytecode() );
  switch (code) {
    case Bytecodes::_aload_0_fast_igetfield_1:
    case Bytecodes::_aload_0_fast_agetfield_1:
      // Don't bother printing out the #0
      return;
  }

  if (current_bytecode_length() > 1) { 
    // Don't need to print out index for bytecodes of length 1
    _st->print("#%d", index); 
  }
}

void BytecodePrintClosure::store_local(BasicType kind, int index JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  (void)kind;
  if( current_bytecode_length() > 1 ) { 
    // Don't need to print out index for bytecodes of length 1
    _st->print("#%d", index); 
  }
}

void BytecodePrintClosure::increment_local_int(int index, jint offset JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  _st->print("L%d += %d", index, offset);
}


void BytecodePrintClosure::load_array(BasicType kind JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  if (_verbose) {
    _st->print("%s", type_name(kind)); 
  }
}

void BytecodePrintClosure::store_array(BasicType kind JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  if (_verbose) {
    _st->print("%s", type_name(kind)); 
  }
}

void BytecodePrintClosure::neg(BasicType kind JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  if (_verbose) {
    _st->print("%s", type_name(kind));
  }
}

void BytecodePrintClosure::binary(BasicType kind, binary_op op JVM_TRAPS) { 
  JVM_IGNORE_TRAPS;
  (void)op;
  if (_verbose) {
    _st->print("%s", type_name(kind));
  }
}

void BytecodePrintClosure::convert(BasicType from, BasicType to JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  if (_verbose) {
    _st->print("%s -> %s", type_name(from), type_name(to));
  }
}

void BytecodePrintClosure::branch_if(cond_op cond, int dest JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  (void)cond;
  _st->print("%d", dest);
}

void BytecodePrintClosure::branch_if_icmp(cond_op cond, int dest JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  (void)cond;
  _st->print("%d", dest);
}

void BytecodePrintClosure::branch_if_acmp(cond_op cond, int dest JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  (void)cond;
  _st->print("%d", dest);
}

void BytecodePrintClosure::compare(BasicType kind, cond_op cond JVM_TRAPS) { 
  JVM_IGNORE_TRAPS;
  (void)cond;
  if (_verbose) {
    _st->print("%s", type_name(kind));
  }
}

void BytecodePrintClosure::branch(int dest JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  _st->print("%d", dest);
}

void BytecodePrintClosure::return_op(BasicType kind JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  if (_verbose) {
    _st->print("%s", type_name(kind));
  }
}

void BytecodePrintClosure::table_switch(jint table_index, jint default_dest,
                                        jint low, jint high JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  _st->print("%d %d %d:", default_dest, low, high);
  for (int i = 0; i < (high - low + 1); i++) {
    int jump_bci =
        bci() + method()->get_java_switch_int(4 * i + table_index + 12);
    _st->print("%d=>%d, ", i + low, jump_bci);
  }
}

void BytecodePrintClosure::lookup_switch(jint table_index, jint default_dest,
                                         jint num_of_pairs JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  _st->print("%d %d: ", default_dest, num_of_pairs);
  for (int i = 0; i < num_of_pairs; i++) {
    int key = method()->get_java_switch_int(8 * i + table_index + 8);
    int jump_bci =
          bci() + method()->get_java_switch_int(8 * i + table_index + 12);
    _st->print("%d=>%d, ", key, jump_bci);
  }
}

void BytecodePrintClosure::get_field(int index JVM_TRAPS) {
  _st->print("%d", index);
  if (Verbose) {
    _st->print(", ");
    ConstantPool cp = method()->constants();
    cp.print_entry_on(_st, index JVM_CHECK);
  }
}

void BytecodePrintClosure::put_field(int index JVM_TRAPS) {
  _st->print("%d", index);
  if (Verbose) {
    _st->print(", ");
    ConstantPool cp = method()->constants();
    cp.print_entry_on(_st, index JVM_CHECK);
  }
}


void BytecodePrintClosure::get_static(int index JVM_TRAPS) {
  _st->print("%d", index);
  if (Verbose) {
    print_verbose_static_field(index JVM_CHECK);
  }
}

void BytecodePrintClosure::put_static(int index JVM_TRAPS) {
  _st->print("%d", index);
  if (Verbose) {
    print_verbose_static_field(index JVM_CHECK);
  }
}

void BytecodePrintClosure::invoke_interface(int index, int nofArgs JVM_TRAPS) {
  _st->print("%d %d", index, nofArgs);
  if (Verbose) {
    print_verbose_interface_method(index JVM_CHECK);
  }
}

void BytecodePrintClosure::invoke_special(int index JVM_TRAPS) {
  _st->print("%d", index);
  if (Verbose) {
    print_verbose_virtual_method(index JVM_CHECK);
  }
}

void BytecodePrintClosure::fast_invoke_special(int index JVM_TRAPS) {
  _st->print("%d", index);
  if (Verbose) {
    print_verbose_virtual_method(index JVM_CHECK);
  }
}

void BytecodePrintClosure::invoke_static(int index JVM_TRAPS) {
  _st->print("%d", index);
  if (Verbose) {
    print_verbose_static_method(index JVM_CHECK);
  }
}

void BytecodePrintClosure::invoke_virtual(int index JVM_TRAPS) {
  _st->print("%d", index);
  if (Verbose) {
    print_verbose_virtual_method(index JVM_CHECK);
  }
}

void BytecodePrintClosure::fast_invoke_virtual(int index JVM_TRAPS) {
  _st->print("%d", index);
  if (Verbose) {
    print_verbose_virtual_method(index JVM_CHECK);
  }
}

void BytecodePrintClosure::fast_invoke_virtual_final(int index JVM_TRAPS) {
  _st->print("%d", index);
  if (Verbose) {
    print_verbose_static_method(index JVM_CHECK);
  }
}

void BytecodePrintClosure::invoke_native(BasicType return_kind, address entry
                                         JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  (void)return_kind;
  if (Verbose && VerbosePointers) {
    _st->print("entry = 0x%lx, ");
  }
  _st->print("followed by implicit return_", entry);
}

void BytecodePrintClosure::new_object(int index JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  _st->print("%d", index);
  if (Verbose) {
    print_verbose_class(index JVM_CHECK);
  }
}

void BytecodePrintClosure::new_basic_array(int type JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  _st->print("%s", type_name((BasicType) type));
}

void BytecodePrintClosure::new_object_array(int index JVM_TRAPS) {
  _st->print("%d", index);
  if (Verbose) {
    print_verbose_class(index JVM_CHECK);
  }
}

void BytecodePrintClosure::new_multi_array(int index, int num_of_dims JVM_TRAPS) {
  _st->print("%d %d", index, num_of_dims);
  if (Verbose) {
    print_verbose_class(index JVM_CHECK);
  }
}

void BytecodePrintClosure::check_cast(int index JVM_TRAPS) {
  _st->print("%d", index);
  if (Verbose) {
    print_verbose_class(index JVM_CHECK);
  }
}

void BytecodePrintClosure::instance_of(int index JVM_TRAPS) {
  _st->print("%d", index);
  if (Verbose) {
    print_verbose_class(index JVM_CHECK);
  }
}

void BytecodePrintClosure::fast_get_field(BasicType field_type, 
                                          int field_offset JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  _st->print("%s offset=%d", type_name(field_type),  field_offset);
}

void BytecodePrintClosure::fast_put_field(BasicType field_type,
                                          int field_offset JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  _st->print("%s offset=%d", type_name(field_type),  field_offset);
}

void BytecodePrintClosure::print_verbose_virtual_method(int index JVM_TRAPS) {
  _st->print(", ");
  ConstantPool cp = method()->constants();
  cp.print_entry_on(_st, index JVM_CHECK);
}

void BytecodePrintClosure::print_verbose_interface_method(int index JVM_TRAPS) {
  _st->print(", ");
  ConstantPool cp = method()->constants();
  cp.print_entry_on(_st, index JVM_CHECK);
}

void BytecodePrintClosure::print_verbose_static_method(int index JVM_TRAPS) {
  ConstantPool::Raw cp = method()->constants();

  _st->print(", ");
  if (cp().tag_at(index).is_resolved_static_method()) {
    Method::Raw m = cp().resolved_static_method_at(index);
    m().print_value_on(_st);
  } else {
    ConstantPool cp = method()->constants();
    cp.print_entry_on(_st, index JVM_CHECK);
  }
}

void BytecodePrintClosure::print_verbose_class(int index JVM_TRAPS) {
  _st->print(", ");
  ConstantPool cp = method()->constants();
  cp.print_entry_on(_st, index JVM_CHECK);
}

void BytecodePrintClosure::print_verbose_static_field(int index JVM_TRAPS) {
  _st->print(", ");
  ConstantPool cp = method()->constants();
  cp.print_entry_on(_st, index JVM_CHECK);
}

#if !ENABLE_CPU_VARIANT
void BytecodePrintClosure::init_static_array(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  _st->print("init_static_array ");
  _st->print("type_size:%d array_size:%d ", method()->get_byte(bci() + 1),
             method()->get_native_ushort(bci() + 2));
  int count = method()->get_native_ushort(bci() + 2);
  int size_factor = 1 << method()->get_byte(bci() + 1);
  for (int i = 0; i < count*size_factor; i++) {
    _st->print("%d ", method()->get_byte(bci() + 4 + i));
  }
}
#endif //!ENABLE_CPU_VARIANT
#endif
