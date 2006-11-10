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
#include "incls/_Bytecodes.cpp.incl"

// Format strings interpretation:
//
// b: bytecode
// c: signed constant, Java byte-ordering
// i: unsigned index , Java byte-ordering
// j: unsigned index , native byte-ordering
// o: branch offset  , Java byte-ordering
// _: unused/ignored
// w: wide bytecode
//
// Note: Right now the format strings are used for 2 purposes:
//       1. to specify the length of the bytecode 
//          (= number of characters in format string)
//       2. to specify the bytecode attributes
//
//       The bytecode attributes are currently used only for bytecode tracing
//       (see BytecodeTracer); thus if more specific format information is
//       used, one would also have to adjust the bytecode tracer.
//
// Note: For bytecodes with variable length, the format string is the
// empty string.

#if USE_DEBUG_PRINTING
#define def(name, length, format, wide_length, wide_format, flags) \
  { _ ## name, #name, (jbyte)length, format, (jbyte)(wide_length|flags), \
    wide_format }
#else
#define def(name, length, format, wide_length, wide_format, flags) \
  { (jbyte)length, (jbyte)(wide_length|flags) }
#endif

#if ENABLE_FLOAT
#define fdef(name, length, format, wide_length, wide_format, flags) \
         def(name, length, format, wide_length, wide_format, flags)
#else
#define fdef(name, length, format, wide_length, wide_format, flags) \
         def(name,     -1, format,          -1, wide_format, flags)
#endif

#define JVM_NO_FALL_THRU 0x80

PRODUCT_CONST Bytecodes::BytecodeData Bytecodes::data[] = {
  def(nop                       , 1, "b"    , 0, ""      , 0),
  def(aconst_null               , 1, "b"    , 0, ""      , 0),
  def(iconst_m1                 , 1, "b"    , 0, ""      , 0),
  def(iconst_0                  , 1, "b"    , 0, ""      , 0),
  def(iconst_1                  , 1, "b"    , 0, ""      , 0),
  def(iconst_2                  , 1, "b"    , 0, ""      , 0),
  def(iconst_3                  , 1, "b"    , 0, ""      , 0),
  def(iconst_4                  , 1, "b"    , 0, ""      , 0),
  def(iconst_5                  , 1, "b"    , 0, ""      , 0),
  def(lconst_0                  , 1, "b"    , 0, ""      , 0),
  def(lconst_1                  , 1, "b"    , 0, ""      , 0),
 fdef(fconst_0                  , 1, "b"    , 0, ""      , 0),
 fdef(fconst_1                  , 1, "b"    , 0, ""      , 0),
 fdef(fconst_2                  , 1, "b"    , 0, ""      , 0),
 fdef(dconst_0                  , 1, "b"    , 0, ""      , 0),
 fdef(dconst_1                  , 1, "b"    , 0, ""      , 0),
  def(bipush                    , 2, "bc"   , 0, ""      , 0),
  def(sipush                    , 3, "bcc"  , 0, ""      , 0),
  def(ldc                       , 2, "bi"   , 0, ""      , 0),
  def(ldc_w                     , 3, "bii"  , 0, ""      , 0),
  def(ldc2_w                    , 3, "bii"  , 0, ""      , 0),
  def(iload                     , 2, "bi"   , 4, "wbii"  , 0),
  def(lload                     , 2, "bi"   , 4, "wbii"  , 0),
 fdef(fload                     , 2, "bi"   , 4, "wbii"  , 0),
 fdef(dload                     , 2, "bi"   , 4, "wbii"  , 0),
  def(aload                     , 2, "bi"   , 4, "wbii"  , 0),
  def(iload_0                   , 1, "b"    , 0, ""      , 0),
  def(iload_1                   , 1, "b"    , 0, ""      , 0),
  def(iload_2                   , 1, "b"    , 0, ""      , 0),
  def(iload_3                   , 1, "b"    , 0, ""      , 0),
  def(lload_0                   , 1, "b"    , 0, ""      , 0),
  def(lload_1                   , 1, "b"    , 0, ""      , 0),
  def(lload_2                   , 1, "b"    , 0, ""      , 0),
  def(lload_3                   , 1, "b"    , 0, ""      , 0),
 fdef(fload_0                   , 1, "b"    , 0, ""      , 0),
 fdef(fload_1                   , 1, "b"    , 0, ""      , 0),
 fdef(fload_2                   , 1, "b"    , 0, ""      , 0),
 fdef(fload_3                   , 1, "b"    , 0, ""      , 0),
 fdef(dload_0                   , 1, "b"    , 0, ""      , 0),
 fdef(dload_1                   , 1, "b"    , 0, ""      , 0),
 fdef(dload_2                   , 1, "b"    , 0, ""      , 0),
 fdef(dload_3                   , 1, "b"    , 0, ""      , 0),
  def(aload_0                   , 1, "b"    , 0, ""      , 0),
  def(aload_1                   , 1, "b"    , 0, ""      , 0),
  def(aload_2                   , 1, "b"    , 0, ""      , 0),
  def(aload_3                   , 1, "b"    , 0, ""      , 0),
  def(iaload                    , 1, "b"    , 0, ""      , 0),
  def(laload                    , 1, "b"    , 0, ""      , 0),
 fdef(faload                    , 1, "b"    , 0, ""      , 0),
 fdef(daload                    , 1, "b"    , 0, ""      , 0),
  def(aaload                    , 1, "b"    , 0, ""      , 0),
  def(baload                    , 1, "b"    , 0, ""      , 0),
  def(caload                    , 1, "b"    , 0, ""      , 0),
  def(saload                    , 1, "b"    , 0, ""      , 0),
  def(istore                    , 2, "bi"   , 4, "wbii"  , 0),
  def(lstore                    , 2, "bi"   , 4, "wbii"  , 0),
 fdef(fstore                    , 2, "bi"   , 4, "wbii"  , 0),
 fdef(dstore                    , 2, "bi"   , 4, "wbii"  , 0),
  def(astore                    , 2, "bi"   , 4, "wbii"  , 0),
  def(istore_0                  , 1, "b"    , 0, ""      , 0),
  def(istore_1                  , 1, "b"    , 0, ""      , 0),
  def(istore_2                  , 1, "b"    , 0, ""      , 0),
  def(istore_3                  , 1, "b"    , 0, ""      , 0),
  def(lstore_0                  , 1, "b"    , 0, ""      , 0),
  def(lstore_1                  , 1, "b"    , 0, ""      , 0),
  def(lstore_2                  , 1, "b"    , 0, ""      , 0),
  def(lstore_3                  , 1, "b"    , 0, ""      , 0),
 fdef(fstore_0                  , 1, "b"    , 0, ""      , 0),
 fdef(fstore_1                  , 1, "b"    , 0, ""      , 0),
 fdef(fstore_2                  , 1, "b"    , 0, ""      , 0),
 fdef(fstore_3                  , 1, "b"    , 0, ""      , 0),
 fdef(dstore_0                  , 1, "b"    , 0, ""      , 0),
 fdef(dstore_1                  , 1, "b"    , 0, ""      , 0),
 fdef(dstore_2                  , 1, "b"    , 0, ""      , 0),
 fdef(dstore_3                  , 1, "b"    , 0, ""      , 0),
  def(astore_0                  , 1, "b"    , 0, ""      , 0),
  def(astore_1                  , 1, "b"    , 0, ""      , 0),
  def(astore_2                  , 1, "b"    , 0, ""      , 0),
  def(astore_3                  , 1, "b"    , 0, ""      , 0),
  def(iastore                   , 1, "b"    , 0, ""      , 0),
  def(lastore                   , 1, "b"    , 0, ""      , 0),
 fdef(fastore                   , 1, "b"    , 0, ""      , 0),
 fdef(dastore                   , 1, "b"    , 0, ""      , 0),
  def(aastore                   , 1, "b"    , 0, ""      , 0),
  def(bastore                   , 1, "b"    , 0, ""      , 0),
  def(castore                   , 1, "b"    , 0, ""      , 0),
  def(sastore                   , 1, "b"    , 0, ""      , 0),
  def(pop                       , 1, "b"    , 0, ""      , 0),
  def(pop2                      , 1, "b"    , 0, ""      , 0),
  def(dup                       , 1, "b"    , 0, ""      , 0),
  def(dup_x1                    , 1, "b"    , 0, ""      , 0),
  def(dup_x2                    , 1, "b"    , 0, ""      , 0),
  def(dup2                      , 1, "b"    , 0, ""      , 0),
  def(dup2_x1                   , 1, "b"    , 0, ""      , 0),
  def(dup2_x2                   , 1, "b"    , 0, ""      , 0),
  def(swap                      , 1, "b"    , 0, ""      , 0),
  def(iadd                      , 1, "b"    , 0, ""      , 0),
  def(ladd                      , 1, "b"    , 0, ""      , 0),
 fdef(fadd                      , 1, "b"    , 0, ""      , 0),
 fdef(dadd                      , 1, "b"    , 0, ""      , 0),
  def(isub                      , 1, "b"    , 0, ""      , 0),
  def(lsub                      , 1, "b"    , 0, ""      , 0),
 fdef(fsub                      , 1, "b"    , 0, ""      , 0),
 fdef(dsub                      , 1, "b"    , 0, ""      , 0),
  def(imul                      , 1, "b"    , 0, ""      , 0),
  def(lmul                      , 1, "b"    , 0, ""      , 0),
 fdef(fmul                      , 1, "b"    , 0, ""      , 0),
 fdef(dmul                      , 1, "b"    , 0, ""      , 0),
  def(idiv                      , 1, "b"    , 0, ""      , 0),
  def(ldiv                      , 1, "b"    , 0, ""      , 0),
 fdef(fdiv                      , 1, "b"    , 0, ""      , 0),
 fdef(ddiv                      , 1, "b"    , 0, ""      , 0),
  def(irem                      , 1, "b"    , 0, ""      , 0),
  def(lrem                      , 1, "b"    , 0, ""      , 0),
 fdef(frem                      , 1, "b"    , 0, ""      , 0),
 fdef(drem                      , 1, "b"    , 0, ""      , 0),
  def(ineg                      , 1, "b"    , 0, ""      , 0),
  def(lneg                      , 1, "b"    , 0, ""      , 0),
 fdef(fneg                      , 1, "b"    , 0, ""      , 0),
 fdef(dneg                      , 1, "b"    , 0, ""      , 0),
  def(ishl                      , 1, "b"    , 0, ""      , 0),
  def(lshl                      , 1, "b"    , 0, ""      , 0),
  def(ishr                      , 1, "b"    , 0, ""      , 0),
  def(lshr                      , 1, "b"    , 0, ""      , 0),
  def(iushr                     , 1, "b"    , 0, ""      , 0),
  def(lushr                     , 1, "b"    , 0, ""      , 0),
  def(iand                      , 1, "b"    , 0, ""      , 0),
  def(land                      , 1, "b"    , 0, ""      , 0),
  def(ior                       , 1, "b"    , 0, ""      , 0),
  def(lor                       , 1, "b"    , 0, ""      , 0),
  def(ixor                      , 1, "b"    , 0, ""      , 0),
  def(lxor                      , 1, "b"    , 0, ""      , 0),
  def(iinc                      , 3, "bic"  , 6, "wbiicc", 0),
  def(i2l                       , 1, "b"    , 0, ""      , 0),
 fdef(i2f                       , 1, "b"    , 0, ""      , 0),
 fdef(i2d                       , 1, "b"    , 0, ""      , 0),
  def(l2i                       , 1, "b"    , 0, ""      , 0),
 fdef(l2f                       , 1, "b"    , 0, ""      , 0),
 fdef(l2d                       , 1, "b"    , 0, ""      , 0),
 fdef(f2i                       , 1, "b"    , 0, ""      , 0),
 fdef(f2l                       , 1, "b"    , 0, ""      , 0),
 fdef(f2d                       , 1, "b"    , 0, ""      , 0),
 fdef(d2i                       , 1, "b"    , 0, ""      , 0),
 fdef(d2l                       , 1, "b"    , 0, ""      , 0),
 fdef(d2f                       , 1, "b"    , 0, ""      , 0),
  def(i2b                       , 1, "b"    , 0, ""      , 0),
  def(i2c                       , 1, "b"    , 0, ""      , 0),
  def(i2s                       , 1, "b"    , 0, ""      , 0),
  def(lcmp                      , 1, "b"    , 0, ""      , 0),
 fdef(fcmpl                     , 1, "b"    , 0, ""      , 0),
 fdef(fcmpg                     , 1, "b"    , 0, ""      , 0),
 fdef(dcmpl                     , 1, "b"    , 0, ""      , 0),
 fdef(dcmpg                     , 1, "b"    , 0, ""      , 0),
  def(ifeq                      , 3, "boo"  , 0, ""      , 0),
  def(ifne                      , 3, "boo"  , 0, ""      , 0),
  def(iflt                      , 3, "boo"  , 0, ""      , 0),
  def(ifge                      , 3, "boo"  , 0, ""      , 0),
  def(ifgt                      , 3, "boo"  , 0, ""      , 0),
  def(ifle                      , 3, "boo"  , 0, ""      , 0),
  def(if_icmpeq                 , 3, "boo"  , 0, ""      , 0),
  def(if_icmpne                 , 3, "boo"  , 0, ""      , 0),
  def(if_icmplt                 , 3, "boo"  , 0, ""      , 0),
  def(if_icmpge                 , 3, "boo"  , 0, ""      , 0),
  def(if_icmpgt                 , 3, "boo"  , 0, ""      , 0),
  def(if_icmple                 , 3, "boo"  , 0, ""      , 0),
  def(if_acmpeq                 , 3, "boo"  , 0, ""      , 0),
  def(if_acmpne                 , 3, "boo"  , 0, ""      , 0),
  def(goto                      , 3, "boo"  , 0, ""      , JVM_NO_FALL_THRU),
  def(jsr                       ,-1, "boo"  , 0, ""      , 0),
  def(ret                       ,-1, "bi"   , 4, "wbii"  , JVM_NO_FALL_THRU),
  def(tableswitch               , 0, ""     , 0, ""      , JVM_NO_FALL_THRU),
  def(lookupswitch              , 0, ""     , 0, ""      , JVM_NO_FALL_THRU),
  def(ireturn                   , 1, "b"    , 0, ""      , JVM_NO_FALL_THRU),
  def(lreturn                   , 1, "b"    , 0, ""      , JVM_NO_FALL_THRU),
 fdef(freturn                   , 1, "b"    , 0, ""      , JVM_NO_FALL_THRU),
 fdef(dreturn                   , 1, "b"    , 0, ""      , JVM_NO_FALL_THRU),
  def(areturn                   , 1, "b"    , 0, ""      , JVM_NO_FALL_THRU),
  def(return                    , 1, "b"    , 0, ""      , JVM_NO_FALL_THRU),
  def(getstatic                 , 3, "bjj"  , 0, ""      , 0),
  def(putstatic                 , 3, "bjj"  , 0, ""      , 0),
  def(getfield                  , 3, "bjj"  , 0, ""      , 0),
  def(putfield                  , 3, "bjj"  , 0, ""      , 0),
  def(invokevirtual             , 3, "bjj"  , 0, ""      , 0),
  def(invokespecial             , 3, "bjj"  , 0, ""      , 0),
  def(invokestatic              , 3, "bjj"  , 0, ""      , 0),
  def(invokeinterface           , 5, "bjj__", 0, ""      , 0),
  def(xxxunusedxxx              ,-1, ""     , 0, ""      , 0),
  def(new                       , 3, "bii"  , 0, ""      , 0),
  def(newarray                  , 2, "bc"   , 0, ""      , 0),
  def(anewarray                 , 3, "bii"  , 0, ""      , 0),
  def(arraylength               , 1, "b"    , 0, ""      , 0),
  def(athrow                    , 1, "b"    , 0, ""      , JVM_NO_FALL_THRU),
  def(checkcast                 , 3, "bii"  , 0, ""      , 0),
  def(instanceof                , 3, "bii"  , 0, ""      , 0),
  def(monitorenter              , 1, "b"    , 0, ""      , 0),
  def(monitorexit               , 1, "b"    , 0, ""      , 0),
  def(wide                      , 0, ""     , 0, ""      , 0),
  def(multianewarray            , 4, "biic" , 0, ""      , 0),
  def(ifnull                    , 3, "boo"  , 0, ""      , 0),
  def(ifnonnull                 , 3, "boo"  , 0, ""      , 0),
  def(goto_w                    , 5, "boooo", 0, ""      , JVM_NO_FALL_THRU),
  def(jsr_w                     ,-1, "boooo", 0, ""      , 0),
  def(breakpoint                , 1, "b"    , 0, ""      , 0),

  def(fast_1_ldc                , 2, "bi"   , 0, ""      , 0),
  def(fast_1_ldc_w              , 3, "bii"  , 0, ""      , 0),
  def(fast_2_ldc_w              , 3, "bii"  , 0, ""      , 0),
                            
  def(fast_1_putstatic          , 3, "bjj"  , 0, ""      , 0),
  def(fast_2_putstatic          , 3, "bjj"  , 0, ""      , 0),
  def(fast_a_putstatic          , 3, "bjj"  , 0, ""      , 0),
                            
  def(fast_1_getstatic          , 3, "bjj"  , 0, ""      , 0),
  def(fast_2_getstatic          , 3, "bjj"  , 0, ""      , 0),

  def(fast_bputfield            , 3, "bjj"  , 0, ""      , 0),
  def(fast_sputfield            , 3, "bjj"  , 0, ""      , 0),
  def(fast_iputfield            , 3, "bjj"  , 0, ""      , 0),
  def(fast_lputfield            , 3, "bjj"  , 0, ""      , 0),
 fdef(fast_fputfield            , 3, "bjj"  , 0, ""      , 0),
 fdef(fast_dputfield            , 3, "bjj"  , 0, ""      , 0),
  def(fast_aputfield            , 3, "bjj"  , 0, ""      , 0),
  def(fast_bgetfield            , 3, "bjj"  , 0, ""      , 0),
  def(fast_sgetfield            , 3, "bjj"  , 0, ""      , 0),
  def(fast_igetfield            , 3, "bjj"  , 0, ""      , 0),
  def(fast_lgetfield            , 3, "bjj"  , 0, ""      , 0),
 fdef(fast_fgetfield            , 3, "bjj"  , 0, ""      , 0),
 fdef(fast_dgetfield            , 3, "bjj"  , 0, ""      , 0),
  def(fast_agetfield            , 3, "bjj"  , 0, ""      , 0),
  def(fast_cgetfield            , 3, "bjj"  , 0, ""      , 0),
  def(fast_invokevirtual        , 3, "bjj"  , 0, ""      , 0),
  def(fast_invokestatic         , 3, "bjj"  , 0, ""      , 0),
  def(fast_invokeinterface      , 5, "bjj__", 0, ""      , 0),
  def(fast_invokenative         , 8, "bc__jjjj", 0, ""   , JVM_NO_FALL_THRU),
  def(fast_invokevirtual_final  , 3, "bjj"  , 0, ""      , 0),
  def(fast_invokespecial        , 3, "bjj"  , 0, ""      , 0),
  def(fast_new                  , 3, "bii"  , 0, ""      , 0),
  def(fast_anewarray            , 3, "bii"  , 0, ""      , 0),
  def(fast_checkcast            , 3, "bii"  , 0, ""      , 0),
  def(fast_instanceof           , 3, "bii"  , 0, ""      , 0),
  def(fast_igetfield_1          , 2, "bi"   , 0, ""      , 0),
  def(fast_agetfield_1          , 2, "bi"   , 0, ""      , 0),
  def(aload_0_fast_agetfield_1  , 2, "bi"   , 0, ""      , 0),
  def(aload_0_fast_igetfield_1  , 2, "bi"   , 0, ""      , 0),
  def(pop_and_npe_if_null       , 1, "b"    , 0, ""      , 0),
  def(init_static_array         , 0, ""     , 0, ""      , 0),
  def(aload_0_fast_agetfield_4  , 1, "b"    , 0, ""      , 0),
  def(aload_0_fast_igetfield_4  , 1, "b"    , 0, ""      , 0),
  def(aload_0_fast_agetfield_8  , 1, "b"    , 0, ""      , 0),
  def(aload_0_fast_igetfield_8  , 1, "b"    , 0, ""      , 0),

  // IMPL_NOTE: the following fast_init_* bytecodes are used only in SVM ROM image,
  // it'll be good to have appropriate GUARANTEEs or #if !ENABLE_ISOLATES
  def(fast_init_1_putstatic     , 3, "bjj"  , 0, ""      , 0),
  def(fast_init_2_putstatic     , 3, "bjj"  , 0, ""      , 0),
  def(fast_init_a_putstatic     , 3, "bjj"  , 0, ""      , 0),
  def(fast_init_1_getstatic     , 3, "bjj"  , 0, ""      , 0),
  def(fast_init_2_getstatic     , 3, "bjj"  , 0, ""      , 0),
  def(fast_init_invokestatic    , 3, "bjj"  , 0, ""      , 0),
  def(fast_init_new             , 3, "bii"  , 0, ""      , 0),

#if USE_DEBUG_PRINTING
  {0, 0, 0, 0, 0}
#else
  {0, 0}
#endif
};

#undef def

int Bytecodes::wide_length_for(const Method* method, int bci, Code code) {
  switch (code) {
  case _tableswitch:{
    int a_bci = align_size_up(bci + 1, wordSize);
    int lo = method->get_java_switch_int(a_bci + wordSize); 
    int hi = method->get_java_switch_int(a_bci + 2 * wordSize);
    return a_bci + (3 + hi - lo + 1)*wordSize - bci;
  }
  case _lookupswitch: {
    int a_bci = align_size_up(bci + 1, wordSize);
    int npairs = method->get_java_switch_int(a_bci + wordSize);
    return a_bci + (2 + 2*npairs)*wordSize - bci;
  }
  case _init_static_array: {
    int size_factor = 1 << method->get_byte(bci + 1);
    int count = method->get_native_ushort(bci + 2);
    return 4 + size_factor * count;
  }
  case _wide: {
    return Bytecodes::wide_length_for(method->bytecode_at(bci+1));
  }
  default:
    SHOULD_NOT_REACH_HERE();
    return 0;
  }
}

#ifndef PRODUCT
void Bytecodes::verify() {
#if USE_DEBUG_PRINTING
  GUARANTEE(number_of_java_codes -1 <= 0xff, "Too many bytecodes");
  int index = 0;
  for(BytecodeData* p = data; p->_name != NULL; p++) {
    GUARANTEE(p->_index == index, "index must match");
    Code code = (Code) index;
    if (is_defined(code)) {
      GUARANTEE((int) jvm_strlen(p->_format) == length_for(code), 
                "checking length");
    }
    if (wide_is_defined(code)) {
      GUARANTEE((int) jvm_strlen(p->_wide_format) == wide_length_for(code), 
                "checking length");
    }

    if (is_defined(code)) {
      if ((code == Bytecodes::_goto) ||
          (code == Bytecodes::_goto_w) ||
          (code >= Bytecodes::_ireturn && code <= Bytecodes::_return) ||
          (code == Bytecodes::_ret) ||
          (code == Bytecodes::_tableswitch) ||
          (code == Bytecodes::_lookupswitch) ||
          (code == Bytecodes::_athrow) ||
          (code == Bytecodes::_fast_invokenative)) {
        GUARANTEE(!can_fall_through(code), "cannot fall through");
      } else {
        GUARANTEE(can_fall_through(code), "can fall through");
      }
    }
    index++;
  }
#endif
}
#endif

#if USE_DEBUG_PRINTING
void Bytecodes::print_definitions() {
  int value = 0;
  for (BytecodeData* p = data; p->_name != NULL; p++) {
    if (value < 16) {
      tty->print("    %3d  0x0%x", value, value);
    } else {
      tty->print("    %3d  0x%x", value, value);
    }
    tty->print_cr("  %s", p->_name);
    value ++;
  }
}
#endif
#if ENABLE_CSE
bool Bytecodes::cse_enabled(Bytecodes::Code code) {
    if ((code >= Bytecodes::_iaload && code <= Bytecodes::_saload)) {
      return true;
    }
  switch (code) {
    case Bytecodes::_getfield:
    case Bytecodes::_getstatic:
    case Bytecodes::_iadd: 
    case Bytecodes::_isub:
//    case Bytecodes::_imul:
//    case Bytecodes::_irem:
//    case Bytecodes::_ishl:
//    case Bytecodes::_ishr:
//    case Bytecodes::_iushr:
//    case Bytecodes::_iand:
//    case Bytecodes::_land:
//    case Bytecodes::_ior:
#if ENABLE_JAVA_STACK_TAGS

   // Don't change ordering of put/get codes without changing 
   // InterpreterRuntime compute_rewrite_bytecode as well.
   case Bytecodes::_fast_igetstatic:
   case Bytecodes::_fast_lgetstatic:
   case Bytecodes::_fast_fgetstatic:
   case Bytecodes::_fast_dgetstatic:
   case Bytecodes::_fast_agetstatic:
#else
   case Bytecodes::_fast_1_getstatic:
   case Bytecodes::_fast_2_getstatic:
#endif

   case Bytecodes::_fast_bgetfield:
   case Bytecodes::_fast_sgetfield:
   case Bytecodes::_fast_igetfield:
   case Bytecodes::_fast_lgetfield:
   case Bytecodes::_fast_fgetfield:
   case Bytecodes::_fast_dgetfield:
   case Bytecodes::_fast_agetfield:
   case Bytecodes::_fast_cgetfield:     // Not same as fast_sgetfield because of sign

   case Bytecodes::_fast_igetfield_1:   // same as _fast_igetfield, but takes 1-byte operand
   case Bytecodes::_fast_agetfield_1:   // same as _fast_agetfield, but takes 1-byte operand
   case Bytecodes::_aload_0_fast_agetfield_1: // same as aload_0 followed by fast_agetfield_1
   case Bytecodes::_aload_0_fast_igetfield_1: // same as aload_0 followed by fast_agetfield_1
   case Bytecodes::_aload_0_fast_agetfield_4: // same as aload_0 followed by fast_agetfield #4
   case Bytecodes::_aload_0_fast_igetfield_4: // same as aload_0 followed by fast_agetfield #4
   case Bytecodes::_aload_0_fast_agetfield_8: // same as aload_0 followed by fast_agetfield #8
   case Bytecodes::_aload_0_fast_igetfield_8: // same as aload_0 followed by fast_agetfield #8
      return true;
    default:
      return false;
  }
}
#endif
