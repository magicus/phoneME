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
#include "incls/_TemplateTable.cpp.incl"

#if ENABLE_INTERPRETER_GENERATOR

Template* TemplateTable::_template_table     [Bytecodes::number_of_java_codes];
Template* TemplateTable::_template_table_wide[Bytecodes::number_of_java_codes];
Bytecodes::Code TemplateTable::_duplicates   [Bytecodes::number_of_java_codes];

void TemplateTable::initialize(SourceMacros* assembler) {
  // Define helper macros.
  #define def_0(bc, options, tmplte)           \
      def      (bc, new tmplte (assembler, bc, options))
  #define def_1(bc, options, tmplte, arg)      \
      def      (bc, new tmplte (assembler, bc, options, arg))
  #define def_2(bc, options, tmplte, arg1, arg2) \
      def      (bc, new tmplte (assembler, bc, options, arg1, arg2))


  #define def_wide_0(bc, options, tmplte)      \
      def_wide (bc, new tmplte (assembler, bc, options))
  #define def_wide_1(bc, options, tmplte, arg) \
      def_wide (bc, new tmplte (assembler, bc, options, arg))
  #define def_wide_2(bc, options, tmplte, arg1, arg2) \
      def_wide (bc, new tmplte (assembler, bc, options, arg1, arg2))


  // This function is split into smaller pieces, since it is too big for some
  // compilers. 
  initialize_00(assembler);
  initialize_20(assembler);
  initialize_40(assembler);
  initialize_60(assembler);
  initialize_80(assembler);
  initialize_A0(assembler);
  initialize_C0(assembler);
  initialize_fast(assembler);
  initialize_duplicates();
}

    
void TemplateTable::initialize_00(SourceMacros* assembler) {
  def_0(Bytecodes::_nop,
        align_code_base,
        bc_nop);
  def_0(Bytecodes::_aconst_null,
        align_code_base,
        bc_aconst_null);
  def_1(Bytecodes::_iconst_m1,
        align_code_base,
        bc_iconst, -1);
  def_1(Bytecodes::_iconst_0,
        align_code_base,
        bc_iconst,  0);
  def_1(Bytecodes::_iconst_1,
        align_code_base,
        bc_iconst,  1);
  def_1(Bytecodes::_iconst_2,
        align_code_base,
        bc_iconst,  2);
  def_1(Bytecodes::_iconst_3,
        align_code_base,
        bc_iconst,  3);
  def_1(Bytecodes::_iconst_4,
        align_code_base,
        bc_iconst,  4);
  def_1(Bytecodes::_iconst_5,
        align_code_base,
        bc_iconst,  5);
  def_1(Bytecodes::_lconst_0,
        align_code_base,
        bc_lconst,  0);
  def_1(Bytecodes::_lconst_1,
        align_code_base,
        bc_lconst,  1);
#if ENABLE_FLOAT
  def_1(Bytecodes::_fconst_0,
        align_code_base | does_fp,
        bc_fconst,  0);
  def_1(Bytecodes::_fconst_1,
        align_code_base | does_fp,
        bc_fconst,  1);
  def_1(Bytecodes::_fconst_2,
        align_code_base | does_fp,
        bc_fconst,  2);
  def_1(Bytecodes::_dconst_0,
        align_code_base | does_fp,
        bc_dconst,  0);
  def_1(Bytecodes::_dconst_1,
        align_code_base | does_fp,
        bc_dconst,  1);
#endif // ENABLE_FLOAT
  def_0(Bytecodes::_bipush,
        align_code_base,
        bc_bipush);
  def_0(Bytecodes::_sipush,
        align_code_base,
        bc_sipush);
  def_1(Bytecodes::_ldc,
        does_dispatch,
        bc_call_vm_redo, "quicken");
  def_1(Bytecodes::_ldc_w,
        does_dispatch,
        bc_call_vm_redo, "quicken");
  def_1(Bytecodes::_ldc2_w,
        does_dispatch,
        bc_call_vm_redo, "quicken");
  def_2(Bytecodes::_iload,
        align_code_base,
        bc_load, T_INT, false);
  def_2(Bytecodes::_lload,
        align_code_base,
        bc_load, T_LONG, false);
#if ENABLE_FLOAT
  def_2(Bytecodes::_fload,
        align_code_base | does_fp,
        bc_load, T_FLOAT, false);
  def_2(Bytecodes::_dload,
        align_code_base | does_fp,
        bc_load, T_DOUBLE, false);
#endif
  def_2(Bytecodes::_aload,
        align_code_base,
        bc_load, T_OBJECT, false);
  def_2(Bytecodes::_iload_0,
        align_code_base,
        bc_load_n, T_INT,  0);
  def_2(Bytecodes::_iload_1,
        align_code_base,
        bc_load_n, T_INT,  1);
  def_2(Bytecodes::_iload_2,
        align_code_base,
        bc_load_n, T_INT,  2);
  def_2(Bytecodes::_iload_3,
        align_code_base,
        bc_load_n, T_INT,  3);
  def_2(Bytecodes::_lload_0,
        align_code_base,
        bc_load_n, T_LONG,  0);
  def_2(Bytecodes::_lload_1,
        align_code_base,
        bc_load_n, T_LONG,  1);
}

void TemplateTable::initialize_20(SourceMacros* assembler) {
  def_2(Bytecodes::_lload_2,
        align_code_base,
        bc_load_n, T_LONG,  2);
  def_2(Bytecodes::_lload_3,
        align_code_base,
        bc_load_n, T_LONG,  3);
#if ENABLE_FLOAT
  def_2(Bytecodes::_fload_0,
        align_code_base | does_fp,
        bc_load_n, T_FLOAT,  0);
  def_2(Bytecodes::_fload_1,
        align_code_base | does_fp,
        bc_load_n, T_FLOAT,  1);
  def_2(Bytecodes::_fload_2,
        align_code_base | does_fp,
        bc_load_n, T_FLOAT,  2);
  def_2(Bytecodes::_fload_3,
        align_code_base | does_fp,
        bc_load_n, T_FLOAT,  3);
  def_2(Bytecodes::_dload_0,
        align_code_base | does_fp,
        bc_load_n, T_DOUBLE,  0);
  def_2(Bytecodes::_dload_1,
        align_code_base | does_fp,
        bc_load_n, T_DOUBLE,  1);
  def_2(Bytecodes::_dload_2,
        align_code_base | does_fp,
        bc_load_n, T_DOUBLE,  2);
  def_2(Bytecodes::_dload_3,
        align_code_base | does_fp,
        bc_load_n, T_DOUBLE,  3);
#endif
  def_2(Bytecodes::_aload_0,
        align_code_base,
        bc_load_n, T_OBJECT,  0);
  def_2(Bytecodes::_aload_1,
        align_code_base,
        bc_load_n, T_OBJECT,  1);
  def_2(Bytecodes::_aload_2,
        align_code_base,
        bc_load_n, T_OBJECT,  2);
  def_2(Bytecodes::_aload_3,
        align_code_base,
        bc_load_n, T_OBJECT,  3);
  def_1(Bytecodes::_iaload,
        does_dispatch | align_code_base,
        bc_array_load, T_INT);
  def_1(Bytecodes::_laload,
        does_dispatch | align_code_base,
        bc_array_load, T_LONG);
#if ENABLE_FLOAT
  def_1(Bytecodes::_faload,
        does_dispatch | align_code_base | does_fp,
        bc_array_load, T_FLOAT);
  def_1(Bytecodes::_daload,
        does_dispatch | align_code_base | does_fp,
        bc_array_load, T_DOUBLE);
#endif
  def_1(Bytecodes::_aaload,
        does_dispatch | align_code_base,
        bc_array_load, T_OBJECT);
  def_1(Bytecodes::_baload,
        does_dispatch | align_code_base,
        bc_array_load, T_BYTE);
  def_1(Bytecodes::_caload,
        does_dispatch | align_code_base,
        bc_array_load, T_CHAR);
  def_1(Bytecodes::_saload,
        does_dispatch | align_code_base,
        bc_array_load, T_SHORT);
  def_2(Bytecodes::_istore,
        align_code_base,
        bc_store, T_INT, false);
  def_2(Bytecodes::_lstore,
        align_code_base,
        bc_store, T_LONG, false);
#if ENABLE_FLOAT
  def_2(Bytecodes::_fstore,
        align_code_base | does_fp,
        bc_store, T_FLOAT, false);
  def_2(Bytecodes::_dstore,
        align_code_base | does_fp,
        bc_store, T_DOUBLE, false);
#endif
  def_2(Bytecodes::_astore,
        align_code_base,
        bc_store, T_OBJECT, false);
  def_2(Bytecodes::_istore_0,
        align_code_base,
        bc_store_n, T_INT,  0);
} 

void TemplateTable::initialize_40(SourceMacros* assembler) {
  def_2(Bytecodes::_istore_1,
        align_code_base,
        bc_store_n, T_INT,  1);
  def_2(Bytecodes::_istore_2,
        align_code_base,
        bc_store_n, T_INT,  2);
  def_2(Bytecodes::_istore_3,
        align_code_base,
        bc_store_n, T_INT,  3);
  def_2(Bytecodes::_lstore_0,
        align_code_base,
        bc_store_n, T_LONG,  0);
  def_2(Bytecodes::_lstore_1,
        align_code_base,
        bc_store_n, T_LONG,  1);
  def_2(Bytecodes::_lstore_2,
        align_code_base,
        bc_store_n, T_LONG,  2);
  def_2(Bytecodes::_lstore_3,
        align_code_base,
        bc_store_n, T_LONG,  3);
#if ENABLE_FLOAT
  def_2(Bytecodes::_fstore_0,
        align_code_base | does_fp,
        bc_store_n, T_FLOAT,  0);
  def_2(Bytecodes::_fstore_1,
        align_code_base | does_fp,
        bc_store_n, T_FLOAT,  1);
  def_2(Bytecodes::_fstore_2,
        align_code_base | does_fp,
        bc_store_n, T_FLOAT,  2);
  def_2(Bytecodes::_fstore_3,
        align_code_base | does_fp,
        bc_store_n, T_FLOAT,  3);
  def_2(Bytecodes::_dstore_0,
        align_code_base | does_fp,
        bc_store_n, T_DOUBLE,  0);
  def_2(Bytecodes::_dstore_1,
        align_code_base | does_fp,
        bc_store_n, T_DOUBLE,  1);
  def_2(Bytecodes::_dstore_2,
        align_code_base | does_fp,
        bc_store_n, T_DOUBLE,  2);
  def_2(Bytecodes::_dstore_3,
        align_code_base | does_fp,
        bc_store_n, T_DOUBLE,  3);
#endif
  def_2(Bytecodes::_astore_0,
        align_code_base,
        bc_store_n, T_OBJECT,  0);
  def_2(Bytecodes::_astore_1,
        align_code_base,
        bc_store_n, T_OBJECT,  1);
  def_2(Bytecodes::_astore_2,
        align_code_base,
        bc_store_n, T_OBJECT,  2);
  def_2(Bytecodes::_astore_3,
        align_code_base,
        bc_store_n, T_OBJECT,  3);
  def_1(Bytecodes::_iastore,
        align_code_base,
        bc_array_store, T_INT);
  def_1(Bytecodes::_lastore,
        align_code_base,
        bc_array_store, T_LONG);
#if ENABLE_FLOAT
  def_1(Bytecodes::_fastore,
        align_code_base | does_fp,
        bc_array_store, T_FLOAT);
  def_1(Bytecodes::_dastore,
        align_code_base | does_fp,
        bc_array_store, T_DOUBLE);
#endif
  def_1(Bytecodes::_aastore,
        align_code_base,
        bc_array_store, T_OBJECT);
  def_1(Bytecodes::_bastore,
        align_code_base,
        bc_array_store, T_BYTE);
  def_1(Bytecodes::_castore,
        align_code_base,
        bc_array_store, T_CHAR);
  def_1(Bytecodes::_sastore,
        align_code_base,
        bc_array_store, T_SHORT);
  def_0(Bytecodes::_pop,
        align_code_base,
        bc_pop);
  def_0(Bytecodes::_pop2,
        align_code_base,
        bc_pop2);
  def_0(Bytecodes::_dup,
        align_code_base,
        bc_dup);
  def_0(Bytecodes::_dup_x1,
        align_code_base,
        bc_dup_x1);
  def_0(Bytecodes::_dup_x2,
        align_code_base,
        bc_dup_x2);
  def_0(Bytecodes::_dup2,
        align_code_base,
        bc_dup2);
  def_0(Bytecodes::_dup2_x1,
        align_code_base,
        bc_dup2_x1);
  def_0(Bytecodes::_dup2_x2,
        align_code_base,
        bc_dup2_x2);
  def_0(Bytecodes::_swap,
        align_code_base,
        bc_swap);
} 

void TemplateTable::initialize_60(SourceMacros* assembler) {
  def_1(Bytecodes::_iadd,
        align_code_base,
        bc_iop2, j_add);
  def_1(Bytecodes::_ladd,
        align_code_base,
        bc_lop2, j_add);
#if ENABLE_FLOAT
  def_0(Bytecodes::_fadd,
        align_code_base | does_fp,
        bc_fadd);
  def_0(Bytecodes::_dadd,
        align_code_base | does_fp,
        bc_dadd);
#endif
  def_1(Bytecodes::_isub,
        align_code_base,
        bc_iop2, j_sub);
  def_1(Bytecodes::_lsub,
        align_code_base,
        bc_lop2, j_sub);
#if ENABLE_FLOAT
  def_0(Bytecodes::_fsub,
        align_code_base | does_fp,
        bc_fsub);
  def_0(Bytecodes::_dsub,
        align_code_base | does_fp,
        bc_dsub);
#endif
  def_1(Bytecodes::_imul,
        align_code_base,
        bc_iop2, j_mul);
  def_0(Bytecodes::_lmul,
        align_code_base,
        bc_lmul);
#if ENABLE_FLOAT
  def_0(Bytecodes::_fmul,
        align_code_base | does_fp,
        bc_fmul);
  def_0(Bytecodes::_dmul,
        align_code_base | does_fp,
        bc_dmul);
#endif
  def_0(Bytecodes::_idiv,
        align_code_base,
        bc_idiv);
  def_0(Bytecodes::_ldiv,
        align_code_base,
        bc_ldiv);
#if ENABLE_FLOAT
  def_0(Bytecodes::_fdiv,
	align_code_base | does_fp,
	bc_fdiv);
  def_0(Bytecodes::_ddiv,
        align_code_base | does_fp,
        bc_ddiv);
#endif
  def_0(Bytecodes::_irem,
        align_code_base,
        bc_irem);
  def_0(Bytecodes::_lrem,
        align_code_base,
        bc_lrem);
#if ENABLE_FLOAT
  def_0(Bytecodes::_frem,
        align_code_base | does_fp,
        bc_frem);
  def_0(Bytecodes::_drem,
        align_code_base | does_fp,
        bc_drem);
#endif
  def_1(Bytecodes::_ineg,
        align_code_base,
        bc_neg, T_INT);
  def_1(Bytecodes::_lneg,
        align_code_base,
        bc_neg, T_LONG);
#if ENABLE_FLOAT
  def_1(Bytecodes::_fneg,
        align_code_base | does_fp,
        bc_neg, T_FLOAT);
  def_1(Bytecodes::_dneg,
        align_code_base | does_fp,
        bc_neg, T_DOUBLE);
#endif
  def_1(Bytecodes::_ishl,
        align_code_base,
        bc_iop2, j_shl);
  def_0(Bytecodes::_lshl,
        align_code_base,
        bc_lshl);
  def_1(Bytecodes::_ishr,
        align_code_base,
        bc_iop2, j_shr);
  def_1(Bytecodes::_lshr,
        align_code_base,
        bc_lshr, true);
  def_1(Bytecodes::_iushr,
        align_code_base,
        bc_iop2, j_ushr);
  def_1(Bytecodes::_lushr,
        align_code_base,
        bc_lshr, false);
  def_1(Bytecodes::_iand,
        align_code_base,
        bc_iop2, j_and);
  def_1(Bytecodes::_land,
        align_code_base,
        bc_lop2, j_and);
}

void TemplateTable::initialize_80(SourceMacros* assembler) {
  def_1(Bytecodes::_ior,
        align_code_base,
        bc_iop2, j_or);
  def_1(Bytecodes::_lor,
        align_code_base,
        bc_lop2, j_or);
  def_1(Bytecodes::_ixor,
        align_code_base,
        bc_iop2, j_xor);
  def_1(Bytecodes::_lxor,
        align_code_base,
        bc_lop2, j_xor);
  def_1(Bytecodes::_iinc,
        align_code_base,
        bc_iinc, false);
  def_0(Bytecodes::_i2l,
        align_code_base,
        bc_i2l);
#if ENABLE_FLOAT
  def_0(Bytecodes::_i2f,
        align_code_base | does_fp,
        bc_i2f);
  def_0(Bytecodes::_i2d,
        align_code_base | does_fp,
        bc_i2d);
#endif
  def_0(Bytecodes::_l2i,
        align_code_base,
        bc_l2i);
#if ENABLE_FLOAT
  def_0(Bytecodes::_l2f,
        align_code_base | does_fp,
        bc_l2f);
  def_0(Bytecodes::_l2d,
        align_code_base | does_fp,
        bc_l2d);
  def_0(Bytecodes::_f2i,
        align_code_base | does_fp,
        bc_f2i);
  def_0(Bytecodes::_f2l,
        align_code_base | does_fp,
        bc_f2l);
  def_0(Bytecodes::_f2d,
        align_code_base | does_fp,
        bc_f2d);
  def_0(Bytecodes::_d2i,
        align_code_base | does_fp,
        bc_d2i);
  def_0(Bytecodes::_d2l,
        align_code_base | does_fp,
        bc_d2l);
  def_0(Bytecodes::_d2f,
        align_code_base | does_fp,
        bc_d2f);
#endif
  def_0(Bytecodes::_i2b,
        align_code_base,
        bc_i2b);
  def_0(Bytecodes::_i2c,
        align_code_base,
        bc_i2c);
  def_0(Bytecodes::_i2s,
        align_code_base,
        bc_i2s);
  def_0(Bytecodes::_lcmp,
        align_code_base,
        bc_lcmp);
#if ENABLE_FLOAT
  def_1(Bytecodes::_fcmpl,
        align_code_base | does_fp,
        bc_fcmp, -1);
  def_1(Bytecodes::_fcmpg,
        align_code_base | does_fp,
        bc_fcmp,  1);
  def_1(Bytecodes::_dcmpl,
        align_code_base | does_fp,
        bc_dcmp, -1);
  def_1(Bytecodes::_dcmpg,
        align_code_base | does_fp,
        bc_dcmp,  1);
#endif
  def_1(Bytecodes::_ifeq,
        align_code_base,
        bc_if_0cmp, j_equal);
  def_1(Bytecodes::_ifne,
        align_code_base,
        bc_if_0cmp, j_not_equal);
  def_1(Bytecodes::_iflt,
        align_code_base,
        bc_if_0cmp, j_less);
  def_1(Bytecodes::_ifge,
        align_code_base,
        bc_if_0cmp, j_greater_equal);
  def_1(Bytecodes::_ifgt,
        align_code_base,
        bc_if_0cmp, j_greater);
  def_1(Bytecodes::_ifle,
        align_code_base,
        bc_if_0cmp, j_less_equal);
  def_1(Bytecodes::_if_icmpeq,
        align_code_base,
        bc_if_icmp, j_equal);
} 

void TemplateTable::initialize_A0(SourceMacros* assembler) {
  def_1(Bytecodes::_if_icmpne,
        align_code_base,
        bc_if_icmp, j_not_equal);
  def_1(Bytecodes::_if_icmplt,
        align_code_base,
        bc_if_icmp, j_less);
  def_1(Bytecodes::_if_icmpge,
        align_code_base,
        bc_if_icmp, j_greater_equal);
  def_1(Bytecodes::_if_icmpgt,
        align_code_base,
        bc_if_icmp, j_greater);
  def_1(Bytecodes::_if_icmple,
        align_code_base,
        bc_if_icmp, j_less_equal);
  def_1(Bytecodes::_if_acmpeq,
        align_code_base,
        bc_if_acmp, j_equal);
  def_1(Bytecodes::_if_acmpne,
        align_code_base,
        bc_if_acmp, j_not_equal);
  def_1(Bytecodes::_goto,
        does_dispatch | align_code_base,
        bc_goto, false);
  def_0(Bytecodes::_tableswitch,
        does_dispatch,
        bc_tableswitch);
  def_0(Bytecodes::_lookupswitch,
        does_dispatch,
        bc_lookupswitch);
  def_1(Bytecodes::_ireturn,
        does_dispatch | align_code_base,
        bc_return, T_INT);
  def_1(Bytecodes::_lreturn,
        does_dispatch | align_code_base,
        bc_return, T_LONG);
#if ENABLE_FLOAT
  def_1(Bytecodes::_freturn,
        does_dispatch | align_code_base | does_fp,
        bc_return, T_FLOAT);
  def_1(Bytecodes::_dreturn,
        does_dispatch | align_code_base | does_fp,
        bc_return, T_DOUBLE);
#endif
  def_1(Bytecodes::_areturn,
        does_dispatch | align_code_base,
        bc_return, T_OBJECT);
  def_1(Bytecodes::_return,
        does_dispatch | align_code_base,
        bc_return, T_VOID);
  
  def_1(Bytecodes::_getstatic,
        does_dispatch,
        bc_call_vm_dispatch, "getstatic");
  def_1(Bytecodes::_putstatic,
        does_dispatch,
        bc_call_vm_dispatch, "putstatic");
  def_1(Bytecodes::_getfield,
        does_dispatch,
        bc_call_vm_redo, "getfield");
  def_1(Bytecodes::_putfield,
        does_dispatch,
        bc_call_vm_redo, "putfield");
  def_1(Bytecodes::_invokevirtual,
        does_dispatch,
        bc_call_vm_redo, "quicken");
  def_1(Bytecodes::_invokespecial,
        does_dispatch,
        bc_call_vm_redo, "quicken");
  def_1(Bytecodes::_invokestatic,
        does_dispatch,
        bc_call_vm_dispatch, "quicken_invokestatic");
  def_1(Bytecodes::_invokeinterface,
        no_options,
        bc_call_vm_redo, "quicken");
  
  def_0(Bytecodes::_new,
        no_options,
        bc_new);
  def_0(Bytecodes::_newarray,
        align_code_base,
        bc_newarray);
  def_0(Bytecodes::_anewarray,
        no_options,
        bc_anewarray);
  def_0(Bytecodes::_arraylength,
        align_code_base,
        bc_arraylength);
  def_0(Bytecodes::_athrow,
        no_options,
        bc_athrow);
} 

void TemplateTable::initialize_C0(SourceMacros* assembler) {
  def_0(Bytecodes::_checkcast,
        no_options,
        bc_checkcast);
  def_0(Bytecodes::_instanceof,
        no_options,
        bc_instanceof);
  def_0(Bytecodes::_monitorenter,
        does_dispatch | align_code_base,
        bc_monitorenter);
  def_0(Bytecodes::_monitorexit,
        align_code_base,
        bc_monitorexit);
  def_0(Bytecodes::_wide,
        does_dispatch | align_code_base,
        bc_wide);
  def_0(Bytecodes::_multianewarray,
        no_options,
        bc_multianewarray);
  def_1(Bytecodes::_ifnull,
        align_code_base,
        bc_if_nullcmp, j_equal);
  def_1(Bytecodes::_ifnonnull,
        align_code_base,
        bc_if_nullcmp, j_not_equal);
  def_1(Bytecodes::_goto_w,
        align_code_base,
        bc_goto, true);
#if ENABLE_JAVA_DEBUGGER
  def_0(Bytecodes::_breakpoint,
        does_dispatch,
        bc_breakpoint);
#else
  def_0(Bytecodes::_breakpoint,
        does_dispatch,
        bc_unimplemented);
#endif
} 

void TemplateTable::initialize_fast(SourceMacros* assembler) {
  // Fast rewritten bytecodes.
#if ENABLE_JAVA_STACK_TAGS
  def_1(Bytecodes::_fast_ildc,
        align_code_base,
        bc_fast_ldc, T_INT, false);
#if ENABLE_FLOAT
  def_1(Bytecodes::_fast_fldc,
        align_code_base | does_fp,
        bc_fast_ldc, T_FLOAT, false);
#endif
  def_1(Bytecodes::_fast_aldc,
        align_code_base,
        bc_fast_ldc, T_OBJECT, false);
  def_1(Bytecodes::_fast_ildc_w,
        align_code_base,
        bc_fast_ldc, T_INT, true);
#if ENABLE_FLOAT
  def_1(Bytecodes::_fast_fldc_w,
        align_code_base | does_fp,
        bc_fast_ldc, T_FLOAT, true);
#endif
  def_1(Bytecodes::_fast_aldc_w,
        align_code_base,
        bc_fast_ldc, T_OBJECT, true);
  def_1(Bytecodes::_fast_lldc_w,
        align_code_base,
        bc_fast_ldc, T_LONG, true);
  def_1(Bytecodes::_fast_dldc_w,
        align_code_base | does_fp,
        bc_fast_ldc, T_DOUBLE, true);

  def_2(Bytecodes::_fast_iputstatic,
        align_code_base,
        bc_fast_putstatic, T_INT, false);
  def_2(Bytecodes::_fast_lputstatic,
        align_code_base,
        bc_fast_putstatic, T_LONG, false);
#if ENABLE_FLOAT
  def_2(Bytecodes::_fast_fputstatic,
        align_code_base | does_fp,
        bc_fast_putstatic, T_FLOAT, false);
#endif
  def_2(Bytecodes::_fast_dputstatic,
        align_code_base | does_fp,
        bc_fast_putstatic, T_DOUBLE, false);
  def_2(Bytecodes::_fast_aputstatic,
        align_code_base,
        bc_fast_putstatic, T_OBJECT, false);
  def_2(Bytecodes::_fast_igetstatic,
        align_code_base,
        bc_fast_getstatic, T_INT, false);
  def_2(Bytecodes::_fast_lgetstatic,
        align_code_base,
        bc_fast_getstatic, T_LONG, false);
#if ENABLE_FLOAT
  def_2(Bytecodes::_fast_fgetstatic,
        align_code_base | does_fp,
        bc_fast_getstatic, T_FLOAT, false);
  def_2(Bytecodes::_fast_dgetstatic,
        align_code_base | does_fp,
        bc_fast_getstatic, T_DOUBLE, false);
#endif
  def_2(Bytecodes::_fast_agetstatic,
        align_code_base,
        bc_fast_getstatic, T_OBJECT, false);

#else
  def_2(Bytecodes::_fast_1_ldc,
        align_code_base,
        bc_fast_ldc, T_INT, false);
  def_2(Bytecodes::_fast_1_ldc_w,
        align_code_base,
        bc_fast_ldc, T_INT, true);
  def_2(Bytecodes::_fast_2_ldc_w,
        align_code_base,
        bc_fast_ldc, T_LONG, true);

  def_2(Bytecodes::_fast_1_putstatic,
        align_code_base,
        bc_fast_putstatic, T_INT, false);
  def_2(Bytecodes::_fast_2_putstatic,
        align_code_base,
        bc_fast_putstatic, T_LONG, false);
  def_2(Bytecodes::_fast_a_putstatic,
        align_code_base,
        bc_fast_putstatic, T_OBJECT, false);

  def_2(Bytecodes::_fast_1_getstatic,
        align_code_base,
        bc_fast_getstatic, T_INT, false);
  def_2(Bytecodes::_fast_2_getstatic,
        align_code_base,
        bc_fast_getstatic, T_LONG, false);

  def_2(Bytecodes::_fast_init_1_putstatic,
        align_code_base,
        bc_fast_putstatic, T_INT, true);
  def_2(Bytecodes::_fast_init_2_putstatic,
        align_code_base,
        bc_fast_putstatic, T_LONG, true);
  def_2(Bytecodes::_fast_init_a_putstatic,
        align_code_base,
        bc_fast_putstatic, T_OBJECT, true);

  def_2(Bytecodes::_fast_init_1_getstatic,
        align_code_base,
        bc_fast_getstatic, T_INT, true);
  def_2(Bytecodes::_fast_init_2_getstatic,
        align_code_base,
        bc_fast_getstatic, T_LONG, true);
#endif

  def_1(Bytecodes::_fast_bputfield,
        align_code_base,
        bc_fast_putfield, T_BYTE);
  def_1(Bytecodes::_fast_sputfield,
        align_code_base,
        bc_fast_putfield, T_SHORT);
  def_1(Bytecodes::_fast_iputfield,
        align_code_base,
        bc_fast_putfield, T_INT);
  def_1(Bytecodes::_fast_lputfield,
        align_code_base,
        bc_fast_putfield, T_LONG);
#if ENABLE_FLOAT
  def_1(Bytecodes::_fast_fputfield,
        align_code_base | does_fp,
        bc_fast_putfield, T_FLOAT);
  def_1(Bytecodes::_fast_dputfield,
        align_code_base | does_fp,
        bc_fast_putfield, T_DOUBLE);
#endif
  def_1(Bytecodes::_fast_aputfield,
        align_code_base,
        bc_fast_putfield, T_OBJECT);
  def_2(Bytecodes::_fast_bgetfield,
        align_code_base,
        bc_fast_getfield, T_BYTE, 2);
  def_2(Bytecodes::_fast_sgetfield,
        align_code_base,
        bc_fast_getfield, T_SHORT, 2);
  def_2(Bytecodes::_fast_igetfield,
        align_code_base,
        bc_fast_getfield, T_INT, 2);
  def_2(Bytecodes::_fast_lgetfield,
        align_code_base,
        bc_fast_getfield, T_LONG, 2);
#if ENABLE_FLOAT
  def_2(Bytecodes::_fast_fgetfield,
        align_code_base | does_fp,
        bc_fast_getfield, T_FLOAT, 2);
  def_2(Bytecodes::_fast_dgetfield,
        align_code_base | does_fp,
        bc_fast_getfield, T_DOUBLE, 2);
#endif
  def_2(Bytecodes::_fast_agetfield,
        align_code_base,
        bc_fast_getfield, T_OBJECT, 2);
  def_2(Bytecodes::_fast_cgetfield,
        align_code_base,
        bc_fast_getfield, T_CHAR, 2);
  def_2(Bytecodes::_fast_igetfield_1,
        align_code_base,
        bc_fast_getfield, T_INT, 1);
  def_2(Bytecodes::_fast_agetfield_1,
        align_code_base,
        bc_fast_getfield, T_OBJECT, 1);
  def_1(Bytecodes::_aload_0_fast_igetfield_1,
        align_code_base,
        bc_aload_0_fast_getfield_1, T_INT);
  def_1(Bytecodes::_aload_0_fast_agetfield_1,
        align_code_base,
        bc_aload_0_fast_getfield_1, T_OBJECT);
  def_0(Bytecodes::_pop_and_npe_if_null,
        align_code_base,
        bc_pop_and_npe_if_null);
#if !ENABLE_CPU_VARIANT  
  def_2(Bytecodes::_aload_0_fast_igetfield_4,
        align_code_base,
        bc_aload_0_fast_getfield_n, T_INT, 4);
  def_2(Bytecodes::_aload_0_fast_agetfield_4,
        align_code_base,
        bc_aload_0_fast_getfield_n, T_OBJECT, 4);
  def_2(Bytecodes::_aload_0_fast_igetfield_8,
        align_code_base,
        bc_aload_0_fast_getfield_n, T_INT, 8);
  def_2(Bytecodes::_aload_0_fast_agetfield_8,
        align_code_base,
        bc_aload_0_fast_getfield_n, T_OBJECT, 8);
  def_0(Bytecodes::_init_static_array,
        align_code_base,
        bc_init_static_array);
#elif ENABLE_ARM11_JAZELLE_DLOAD_BUG_WORKAROUND 
//#if ENABLE_FLOAT
  def_wide_2(Bytecodes::_dload_safe,
             align_code_base/* | does_fp*/,
             bc_load, T_LONG, true);
  def_wide_2(Bytecodes::_dstore_safe,
             align_code_base/* | does_fp*/,
             bc_store, T_LONG, true);
//#endif //ENABLE_FLOAT
  def_wide_2(Bytecodes::_lload_safe,
             align_code_base,
             bc_load, T_LONG, true);
  def_wide_2(Bytecodes::_lstore_safe,
             align_code_base,
             bc_store, T_LONG, true);
#endif //!ENABLE_CPU_VARIANT
  def_1(Bytecodes::_fast_invokevirtual,
        align_code_base,
        bc_fast_invoke, false);
  def_1(Bytecodes::_fast_invokestatic,
        align_code_base,
        bc_fast_invoke, true);
  def_1(Bytecodes::_fast_init_invokestatic,
        align_code_base,
        bc_fast_invoke, true);
  def_0(Bytecodes::_fast_invokeinterface,
        align_code_base,
        bc_fast_invokeinterface);
  def_0(Bytecodes::_fast_invokenative,
        does_dispatch | align_code_base,
        bc_fast_invokenative);
  def_0(Bytecodes::_fast_new,
        align_code_base,
        bc_fast_new);
  def_0(Bytecodes::_fast_init_new,
        align_code_base,
        bc_fast_new);
  def_0(Bytecodes::_fast_anewarray,
        align_code_base,
        bc_fast_anewarray);
  def_0(Bytecodes::_fast_checkcast,
        align_code_base,
        bc_fast_checkcast);
  def_0(Bytecodes::_fast_instanceof,
        align_code_base,
        bc_fast_instanceof);
  def_1(Bytecodes::_fast_invokevirtual_final,
        align_code_base,
        bc_fast_invoke, true);
  def_0(Bytecodes::_fast_invokespecial,
        align_code_base,
        bc_fast_invokespecial);


  // Wide Java spec bytecodes             Dispatches    Must align    Generator          Argument
  def_wide_2(Bytecodes::_iload,
             align_code_base,
             bc_load, T_INT, true);
  def_wide_2(Bytecodes::_lload,
             align_code_base,
             bc_load, T_LONG, true);
#if ENABLE_FLOAT
  def_wide_2(Bytecodes::_fload,
             align_code_base | does_fp,
             bc_load, T_FLOAT, true);
  def_wide_2(Bytecodes::_dload,
             align_code_base | does_fp,
             bc_load, T_DOUBLE, true);
#endif
  def_wide_2(Bytecodes::_aload,
             align_code_base,
             bc_load, T_OBJECT, true);
  def_wide_2(Bytecodes::_istore,
             align_code_base,
             bc_store, T_INT, true);
  def_wide_2(Bytecodes::_lstore,
             align_code_base,
             bc_store, T_LONG, true);
#if ENABLE_FLOAT
  def_wide_2(Bytecodes::_fstore,
             align_code_base | does_fp,
             bc_store, T_FLOAT, true);
  def_wide_2(Bytecodes::_dstore,
             align_code_base | does_fp,
             bc_store, T_DOUBLE, true);
#endif
  def_wide_2(Bytecodes::_astore,
             align_code_base,
             bc_store, T_OBJECT, true);
  def_wide_1(Bytecodes::_iinc,
             align_code_base,
             bc_iinc, true);
} 
  // Undefine helper macros.
  #undef def_0
  #undef def_1

  #undef def_wide_0
  #undef def_wide_1

void TemplateTable::initialize_duplicates() {
  for (int i = 0; i < Bytecodes::number_of_java_codes; i++) {
    _duplicates[i] = (Bytecodes::Code)i;
  }
  _duplicates[Bytecodes::_aload] = Bytecodes::_iload;
  _duplicates[Bytecodes::_fload] = Bytecodes::_iload;
  _duplicates[Bytecodes::_dload] = Bytecodes::_lload;

  _duplicates[Bytecodes::_astore] = Bytecodes::_istore;
  _duplicates[Bytecodes::_fstore] = Bytecodes::_istore;
  _duplicates[Bytecodes::_dstore] = Bytecodes::_lstore;

  for (int j = 0; j <= 3; j++) { 
    _duplicates[Bytecodes::_aload_0 + j] =
        (Bytecodes::Code)(Bytecodes::_iload_0 + j);
    _duplicates[Bytecodes::_fload_0 + j] =
        (Bytecodes::Code)(Bytecodes::_iload_0 + j);
    _duplicates[Bytecodes::_dload_0 + j] =
        (Bytecodes::Code)(Bytecodes::_lload_0 + j);
    _duplicates[Bytecodes::_astore_0 + j] =
        (Bytecodes::Code)(Bytecodes::_istore_0 + j);
    _duplicates[Bytecodes::_fstore_0 + j] =
        (Bytecodes::Code)(Bytecodes::_istore_0 + j);
    _duplicates[Bytecodes::_dstore_0 + j] =
        (Bytecodes::Code)(Bytecodes::_lstore_0 + j);
  }
  
  _duplicates[Bytecodes::_aconst_null] = Bytecodes::_iconst_0;
  _duplicates[Bytecodes::_fconst_0]    = Bytecodes::_iconst_0;

  _duplicates[Bytecodes::_faload]      = Bytecodes::_iaload;
  _duplicates[Bytecodes::_aaload]      = Bytecodes::_iaload;

  _duplicates[Bytecodes::_daload]      = Bytecodes::_laload;

  _duplicates[Bytecodes::_fastore]     = Bytecodes::_iastore;
  _duplicates[Bytecodes::_dastore]     = Bytecodes::_lastore;
  _duplicates[Bytecodes::_castore]     = Bytecodes::_sastore;

  _duplicates[Bytecodes::_if_acmpeq]   = Bytecodes::_if_icmpeq;
  _duplicates[Bytecodes::_if_acmpne]   = Bytecodes::_if_icmpne;
  _duplicates[Bytecodes::_ifnull]      = Bytecodes::_ifeq;
  _duplicates[Bytecodes::_ifnonnull]   = Bytecodes::_ifne;

#if !USE_FP_RESULT_IN_VFP_REGISTER
  _duplicates[Bytecodes::_freturn]     = Bytecodes::_ireturn; 
  _duplicates[Bytecodes::_dreturn]     = Bytecodes::_lreturn;
#endif
     
  _duplicates[Bytecodes::_areturn]     = Bytecodes::_ireturn;

  _duplicates[Bytecodes::_fast_fputfield] = Bytecodes::_fast_iputfield;
  _duplicates[Bytecodes::_fast_dputfield] = Bytecodes::_fast_lputfield;
  // aputfield is >>not<< a duplicate of iputfield.  Write barriers

  _duplicates[Bytecodes::_fast_fgetfield] = Bytecodes::_fast_igetfield;
  _duplicates[Bytecodes::_fast_agetfield] = Bytecodes::_fast_igetfield;
  _duplicates[Bytecodes::_fast_dgetfield] = Bytecodes::_fast_lgetfield;

  _duplicates[Bytecodes::_fast_agetfield_1] = Bytecodes::_fast_igetfield_1;
  _duplicates[Bytecodes::_aload_0_fast_agetfield_1] =
                                          Bytecodes::_aload_0_fast_igetfield_1;
#if !ENABLE_CPU_VARIANT  
  _duplicates[Bytecodes::_aload_0_fast_agetfield_4] =
                                          Bytecodes::_aload_0_fast_igetfield_4;
  _duplicates[Bytecodes::_aload_0_fast_agetfield_8] =
                                          Bytecodes::_aload_0_fast_igetfield_8;
#elif ENABLE_ARM11_JAZELLE_DLOAD_BUG_WORKAROUND
  _duplicates[Bytecodes::_lload_safe]   = Bytecodes::_lload;
  _duplicates[Bytecodes::_lstore_safe]  = Bytecodes::_lstore;
  //we use l* bytecodes here cause they are duplicates 
  // for ordinary d* bytecodes!
  _duplicates[Bytecodes::_dload_safe]   = Bytecodes::_lload;
  _duplicates[Bytecodes::_dstore_safe]  = Bytecodes::_lstore;  
#endif //#if !ENABLE_CPU_VARIANT  
  // The following are all just "quicken"
  _duplicates[Bytecodes::_ldc_w]          = Bytecodes::_ldc;
  _duplicates[Bytecodes::_ldc2_w]         = Bytecodes::_ldc;
  _duplicates[Bytecodes::_invokevirtual]  = Bytecodes::_ldc;
  _duplicates[Bytecodes::_invokespecial]  = Bytecodes::_ldc;
} 

void TemplateTable::def(Bytecodes::Code code, Template* t) {
  _template_table[code] = t;
}

void TemplateTable::def_wide(Bytecodes::Code code, Template* t) {
  _template_table_wide[code] = t;
}

#endif // ENABLE_INTERPRETER_GENERATOR
