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

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
#define def(name, length, format, wide_length, wide_format, flags) \
  { _ ## name, #name, (jbyte)length, format, (jushort)(wide_length|flags), \
    wide_format }
#else
#define def(name, length, format, wide_length, wide_format, flags) \
  { (jushort)(wide_length|flags), (jbyte)length, 0 }
#endif

#if ENABLE_FLOAT
#define fdef(name, length, format, wide_length, wide_format, flags) \
         def(name, length, format, wide_length, wide_format, flags)
#else
#define fdef(name, length, format, wide_length, wide_format, flags) \
         def(name,     -1, format,          -1, wide_format, flags)
#endif

PRODUCT_CONST Bytecodes::BytecodeData Bytecodes::data[] = {
  def(nop                       , 1, "b"    , 0, ""      , None),
  def(aconst_null               , 1, "b"    , 0, ""      , None),
  def(iconst_m1                 , 1, "b"    , 0, ""      , None),
  def(iconst_0                  , 1, "b"    , 0, ""      , None),
  def(iconst_1                  , 1, "b"    , 0, ""      , None),
  def(iconst_2                  , 1, "b"    , 0, ""      , None),
  def(iconst_3                  , 1, "b"    , 0, ""      , None),
  def(iconst_4                  , 1, "b"    , 0, ""      , None),
  def(iconst_5                  , 1, "b"    , 0, ""      , None),
  def(lconst_0                  , 1, "b"    , 0, ""      , None),
  def(lconst_1                  , 1, "b"    , 0, ""      , None),
 fdef(fconst_0                  , 1, "b"    , 0, ""      , None),
 fdef(fconst_1                  , 1, "b"    , 0, ""      , None),
 fdef(fconst_2                  , 1, "b"    , 0, ""      , None),
 fdef(dconst_0                  , 1, "b"    , 0, ""      , None),
 fdef(dconst_1                  , 1, "b"    , 0, ""      , None),
  def(bipush                    , 2, "bc"   , 0, ""      , None),
  def(sipush                    , 3, "bcc"  , 0, ""      , None),
  def(ldc                       , 2, "bi"   , 0, ""      , None),
  def(ldc_w                     , 3, "bii"  , 0, ""      , None),
  def(ldc2_w                    , 3, "bii"  , 0, ""      , None),
  def(iload                     , 2, "bi"   , 4, "wbii"  , None),
  def(lload                     , 2, "bi"   , 4, "wbii"  , None),
 fdef(fload                     , 2, "bi"   , 4, "wbii"  , None),
 fdef(dload                     , 2, "bi"   , 4, "wbii"  , None),
  def(aload                     , 2, "bi"   , 4, "wbii"  , None),
  def(iload_0                   , 1, "b"    , 0, ""      , None),
  def(iload_1                   , 1, "b"    , 0, ""      , None),
  def(iload_2                   , 1, "b"    , 0, ""      , None),
  def(iload_3                   , 1, "b"    , 0, ""      , None),
  def(lload_0                   , 1, "b"    , 0, ""      , None),
  def(lload_1                   , 1, "b"    , 0, ""      , None),
  def(lload_2                   , 1, "b"    , 0, ""      , None),
  def(lload_3                   , 1, "b"    , 0, ""      , None),
 fdef(fload_0                   , 1, "b"    , 0, ""      , None),
 fdef(fload_1                   , 1, "b"    , 0, ""      , None),
 fdef(fload_2                   , 1, "b"    , 0, ""      , None),
 fdef(fload_3                   , 1, "b"    , 0, ""      , None),
 fdef(dload_0                   , 1, "b"    , 0, ""      , None),
 fdef(dload_1                   , 1, "b"    , 0, ""      , None),
 fdef(dload_2                   , 1, "b"    , 0, ""      , None),
 fdef(dload_3                   , 1, "b"    , 0, ""      , None),
  def(aload_0                   , 1, "b"    , 0, ""      , None),
  def(aload_1                   , 1, "b"    , 0, ""      , None),
  def(aload_2                   , 1, "b"    , 0, ""      , None),
  def(aload_3                   , 1, "b"    , 0, ""      , None),
  def(iaload                    , 1, "b"    , 0, ""      , NullCheck | CSE),
  def(laload                    , 1, "b"    , 0, ""      , NullCheck | CSE),
 fdef(faload                    , 1, "b"    , 0, ""      , NullCheck | CSE),
 fdef(daload                    , 1, "b"    , 0, ""      , NullCheck | CSE),
  def(aaload                    , 1, "b"    , 0, ""      , NullCheck | CSE),
  def(baload                    , 1, "b"    , 0, ""      , NullCheck | CSE),
  def(caload                    , 1, "b"    , 0, ""      , NullCheck | CSE),
  def(saload                    , 1, "b"    , 0, ""      , NullCheck | CSE),
  def(istore                    , 2, "bi"   , 4, "wbii"  , None),
  def(lstore                    , 2, "bi"   , 4, "wbii"  , None),
 fdef(fstore                    , 2, "bi"   , 4, "wbii"  , None),
 fdef(dstore                    , 2, "bi"   , 4, "wbii"  , None),
  def(astore                    , 2, "bi"   , 4, "wbii"  , None),
  def(istore_0                  , 1, "b"    , 0, ""      , None),
  def(istore_1                  , 1, "b"    , 0, ""      , None),
  def(istore_2                  , 1, "b"    , 0, ""      , None),
  def(istore_3                  , 1, "b"    , 0, ""      , None),
  def(lstore_0                  , 1, "b"    , 0, ""      , None),
  def(lstore_1                  , 1, "b"    , 0, ""      , None),
  def(lstore_2                  , 1, "b"    , 0, ""      , None),
  def(lstore_3                  , 1, "b"    , 0, ""      , None),
 fdef(fstore_0                  , 1, "b"    , 0, ""      , None),
 fdef(fstore_1                  , 1, "b"    , 0, ""      , None),
 fdef(fstore_2                  , 1, "b"    , 0, ""      , None),
 fdef(fstore_3                  , 1, "b"    , 0, ""      , None),
 fdef(dstore_0                  , 1, "b"    , 0, ""      , None),
 fdef(dstore_1                  , 1, "b"    , 0, ""      , None),
 fdef(dstore_2                  , 1, "b"    , 0, ""      , None),
 fdef(dstore_3                  , 1, "b"    , 0, ""      , None),
  def(astore_0                  , 1, "b"    , 0, ""      , None),
  def(astore_1                  , 1, "b"    , 0, ""      , None),
  def(astore_2                  , 1, "b"    , 0, ""      , None),
  def(astore_3                  , 1, "b"    , 0, ""      , None),
  def(iastore                   , 1, "b"    , 0, ""      , NullCheck),
  def(lastore                   , 1, "b"    , 0, ""      , NullCheck),
 fdef(fastore                   , 1, "b"    , 0, ""      , NullCheck),
 fdef(dastore                   , 1, "b"    , 0, ""      , NullCheck),
  def(aastore                   , 1, "b"    , 0, ""      , NullCheck),
  def(bastore                   , 1, "b"    , 0, ""      , NullCheck),
  def(castore                   , 1, "b"    , 0, ""      , NullCheck),
  def(sastore                   , 1, "b"    , 0, ""      , NullCheck),
  def(pop                       , 1, "b"    , 0, ""      , None),
  def(pop2                      , 1, "b"    , 0, ""      , None),
  def(dup                       , 1, "b"    , 0, ""      , None),
  def(dup_x1                    , 1, "b"    , 0, ""      , None),
  def(dup_x2                    , 1, "b"    , 0, ""      , None),
  def(dup2                      , 1, "b"    , 0, ""      , None),
  def(dup2_x1                   , 1, "b"    , 0, ""      , None),
  def(dup2_x2                   , 1, "b"    , 0, ""      , None),
  def(swap                      , 1, "b"    , 0, ""      , None),
  def(iadd                      , 1, "b"    , 0, ""      , CSE),
  def(ladd                      , 1, "b"    , 0, ""      , SoftLong),
 fdef(fadd                      , 1, "b"    , 0, ""      , SoftFloat),
 fdef(dadd                      , 1, "b"    , 0, ""      , SoftFloat),
  def(isub                      , 1, "b"    , 0, ""      , CSE),
  def(lsub                      , 1, "b"    , 0, ""      , SoftLong),
 fdef(fsub                      , 1, "b"    , 0, ""      , SoftFloat),
 fdef(dsub                      , 1, "b"    , 0, ""      , SoftFloat),
  def(imul                      , 1, "b"    , 0, ""      , SoftFloat),
  def(lmul                      , 1, "b"    , 0, ""      , SoftLong),
 fdef(fmul                      , 1, "b"    , 0, ""      , SoftFloat),
 fdef(dmul                      , 1, "b"    , 0, ""      , SoftFloat),
  def(idiv                      , 1, "b"    , 0, ""      , SoftFloat | Exceptions),
  def(ldiv                      , 1, "b"    , 0, ""      , SoftLong | Exceptions),
 fdef(fdiv                      , 1, "b"    , 0, ""      , SoftFloat),
 fdef(ddiv                      , 1, "b"    , 0, ""      , SoftFloat),
  def(irem                      , 1, "b"    , 0, ""      , SoftFloat | Exceptions),
  def(lrem                      , 1, "b"    , 0, ""      , SoftLong | Exceptions),
 fdef(frem                      , 1, "b"    , 0, ""      , SoftFloat),
 fdef(drem                      , 1, "b"    , 0, ""      , SoftFloat),
  def(ineg                      , 1, "b"    , 0, ""      , None),
  def(lneg                      , 1, "b"    , 0, ""      , SoftLong),
 fdef(fneg                      , 1, "b"    , 0, ""      , SoftFloat),
 fdef(dneg                      , 1, "b"    , 0, ""      , SoftFloat),
  def(ishl                      , 1, "b"    , 0, ""      , None),
  def(lshl                      , 1, "b"    , 0, ""      , SoftLong),
  def(ishr                      , 1, "b"    , 0, ""      , None),
  def(lshr                      , 1, "b"    , 0, ""      , SoftLong),
  def(iushr                     , 1, "b"    , 0, ""      , None),
  def(lushr                     , 1, "b"    , 0, ""      , SoftLong),
  def(iand                      , 1, "b"    , 0, ""      , None),
  def(land                      , 1, "b"    , 0, ""      , SoftLong),
  def(ior                       , 1, "b"    , 0, ""      , None),
  def(lor                       , 1, "b"    , 0, ""      , SoftLong),
  def(ixor                      , 1, "b"    , 0, ""      , None),
  def(lxor                      , 1, "b"    , 0, ""      , SoftLong),
  def(iinc                      , 3, "bic"  , 6, "wbiicc", None),
  def(i2l                       , 1, "b"    , 0, ""      , None),
 fdef(i2f                       , 1, "b"    , 0, ""      , SoftFloat),
 fdef(i2d                       , 1, "b"    , 0, ""      , SoftFloat),
  def(l2i                       , 1, "b"    , 0, ""      , None),
 fdef(l2f                       , 1, "b"    , 0, ""      , SoftFloat),
 fdef(l2d                       , 1, "b"    , 0, ""      , SoftFloat),
 fdef(f2i                       , 1, "b"    , 0, ""      , SoftFloat),
 fdef(f2l                       , 1, "b"    , 0, ""      , SoftFloat),
 fdef(f2d                       , 1, "b"    , 0, ""      , SoftFloat),
 fdef(d2i                       , 1, "b"    , 0, ""      , SoftFloat),
 fdef(d2l                       , 1, "b"    , 0, ""      , SoftFloat),
 fdef(d2f                       , 1, "b"    , 0, ""      , SoftFloat),
  def(i2b                       , 1, "b"    , 0, ""      , None),
  def(i2c                       , 1, "b"    , 0, ""      , None),
  def(i2s                       , 1, "b"    , 0, ""      , None),
  def(lcmp                      , 1, "b"    , 0, ""      , SoftLong),
 fdef(fcmpl                     , 1, "b"    , 0, ""      , SoftFloat),
 fdef(fcmpg                     , 1, "b"    , 0, ""      , SoftFloat),
 fdef(dcmpl                     , 1, "b"    , 0, ""      , SoftFloat),
 fdef(dcmpg                     , 1, "b"    , 0, ""      , SoftFloat),
  def(ifeq                      , 3, "boo"  , 0, ""      , None),
  def(ifne                      , 3, "boo"  , 0, ""      , None),
  def(iflt                      , 3, "boo"  , 0, ""      , None),
  def(ifge                      , 3, "boo"  , 0, ""      , None),
  def(ifgt                      , 3, "boo"  , 0, ""      , None),
  def(ifle                      , 3, "boo"  , 0, ""      , None),
  def(if_icmpeq                 , 3, "boo"  , 0, ""      , None),
  def(if_icmpne                 , 3, "boo"  , 0, ""      , None),
  def(if_icmplt                 , 3, "boo"  , 0, ""      , None),
  def(if_icmpge                 , 3, "boo"  , 0, ""      , None),
  def(if_icmpgt                 , 3, "boo"  , 0, ""      , None),
  def(if_icmple                 , 3, "boo"  , 0, ""      , None),
  def(if_acmpeq                 , 3, "boo"  , 0, ""      , None),
  def(if_acmpne                 , 3, "boo"  , 0, ""      , None),
  def(goto                      , 3, "boo"  , 0, ""      , NoFallThru),
  def(jsr                       ,-1, "boo"  , 0, ""      , None),
  def(ret                       ,-1, "bi"   , 4, "wbii"  , NoFallThru),
  def(tableswitch               , 0, ""     , 0, ""      , NoFallThru),
  def(lookupswitch              , 0, ""     , 0, ""      , NoFallThru),
  def(ireturn                   , 1, "b"    , 0, ""      , NoFallThru),
  def(lreturn                   , 1, "b"    , 0, ""      , NoFallThru),
 fdef(freturn                   , 1, "b"    , 0, ""      , NoFallThru),
 fdef(dreturn                   , 1, "b"    , 0, ""      , NoFallThru),
  def(areturn                   , 1, "b"    , 0, ""      , NoFallThru),
  def(return                    , 1, "b"    , 0, ""      , NoFallThru),
  def(getstatic                 , 3, "bjj"  , 0, ""      , Exceptions | CSE),
  def(putstatic                 , 3, "bjj"  , 0, ""      , Exceptions),
  def(getfield                  , 3, "bjj"  , 0, ""      , NullCheck  | CSE),
  def(putfield                  , 3, "bjj"  , 0, ""      , NullCheck),
  def(invokevirtual             , 3, "bjj"  , 0, ""      , NullCheck  | NoPatching),
  def(invokespecial             , 3, "bjj"  , 0, ""      , NullCheck  | NoPatching),
  def(invokestatic              , 3, "bjj"  , 0, ""      , Exceptions | NoPatching),
  def(invokeinterface           , 5, "bjj__", 0, ""      , NullCheck  | NoPatching),
  def(xxxunusedxxx              ,-1, ""     , 0, ""      , Exceptions),
  def(new                       , 3, "bii"  , 0, ""      , Exceptions | NoPatching),
  def(newarray                  , 2, "bc"   , 0, ""      , Exceptions | NoPatching),
  def(anewarray                 , 3, "bii"  , 0, ""      , Exceptions | NoPatching),
  def(arraylength               , 1, "b"    , 0, ""      , NullCheck),
  def(athrow                    , 1, "b"    , 0, ""      , NoFallThru | NullCheck | NoInlining),
  def(checkcast                 , 3, "bii"  , 0, ""      , Exceptions | NoPatching),
  def(instanceof                , 3, "bii"  , 0, ""      , Exceptions | NoPatching),
  def(monitorenter              , 1, "b"    , 0, ""      , NoPatching),
  def(monitorexit               , 1, "b"    , 0, ""      , NoPatching),
  def(wide                      , 0, ""     , 0, ""      , None),
  def(multianewarray            , 4, "biic" , 0, ""      , Exceptions | NoPatching),
  def(ifnull                    , 3, "boo"  , 0, ""      , None),
  def(ifnonnull                 , 3, "boo"  , 0, ""      , None),
  def(goto_w                    , 5, "boooo", 0, ""      , NoFallThru),
  def(jsr_w                     ,-1, "boooo", 0, ""      , None),
  def(breakpoint                , 1, "b"    , 0, ""      , None),

  def(fast_1_ldc                , 2, "bi"   , 0, ""      , None),
  def(fast_1_ldc_w              , 3, "bii"  , 0, ""      , None),
  def(fast_2_ldc_w              , 3, "bii"  , 0, ""      , None),
                            
  def(fast_1_putstatic          , 3, "bjj"  , 0, ""      , Exceptions),
  def(fast_2_putstatic          , 3, "bjj"  , 0, ""      , Exceptions),
  def(fast_a_putstatic          , 3, "bjj"  , 0, ""      , Exceptions),
                            
  def(fast_1_getstatic          , 3, "bjj"  , 0, ""      , Exceptions | CSE),
  def(fast_2_getstatic          , 3, "bjj"  , 0, ""      , Exceptions | CSE),

  def(fast_bputfield            , 3, "bjj"  , 0, ""      , NullCheck),
  def(fast_sputfield            , 3, "bjj"  , 0, ""      , NullCheck),
  def(fast_iputfield            , 3, "bjj"  , 0, ""      , NullCheck),
  def(fast_lputfield            , 3, "bjj"  , 0, ""      , NullCheck),
 fdef(fast_fputfield            , 3, "bjj"  , 0, ""      , NullCheck),
 fdef(fast_dputfield            , 3, "bjj"  , 0, ""      , NullCheck),
  def(fast_aputfield            , 3, "bjj"  , 0, ""      , NullCheck),
  def(fast_bgetfield            , 3, "bjj"  , 0, ""      , NullCheck  | CSE),
  def(fast_sgetfield            , 3, "bjj"  , 0, ""      , NullCheck  | CSE),
  def(fast_igetfield            , 3, "bjj"  , 0, ""      , NullCheck  | CSE),
  def(fast_lgetfield            , 3, "bjj"  , 0, ""      , NullCheck  | CSE),
 fdef(fast_fgetfield            , 3, "bjj"  , 0, ""      , NullCheck  | CSE),
 fdef(fast_dgetfield            , 3, "bjj"  , 0, ""      , NullCheck  | CSE),
  def(fast_agetfield            , 3, "bjj"  , 0, ""      , NullCheck  | CSE),
  def(fast_cgetfield            , 3, "bjj"  , 0, ""      , NullCheck  | CSE),
  def(fast_invokevirtual        , 3, "bjj"  , 0, ""      , NullCheck  | NoPatching),
  def(fast_invokestatic         , 3, "bjj"  , 0, ""      , Exceptions | NoPatching),
  def(fast_invokeinterface      , 5, "bjj__", 0, ""      , NullCheck  | NoPatching),
  def(fast_invokenative         , 8, "bc__jjjj", 0, ""   , Exceptions | NoFallThru | NoPatching),
  def(fast_invokevirtual_final  , 3, "bjj"  , 0, ""      , NullCheck  | NoPatching),
  def(fast_invokespecial        , 3, "bjj"  , 0, ""      , NullCheck  | NoPatching),
  def(fast_new                  , 3, "bii"  , 0, ""      , Exceptions | NoPatching),
  def(fast_anewarray            , 3, "bii"  , 0, ""      , Exceptions | NoPatching),
  def(fast_checkcast            , 3, "bii"  , 0, ""      , Exceptions | NoPatching),
  def(fast_instanceof           , 3, "bii"  , 0, ""      , Exceptions | NoPatching),
  def(fast_igetfield_1          , 2, "bi"   , 0, ""      , NullCheck  | CSE),
  def(fast_agetfield_1          , 2, "bi"   , 0, ""      , NullCheck  | CSE),
  def(aload_0_fast_agetfield_1  , 2, "bi"   , 0, ""      , NullCheck  | CSE),
  def(aload_0_fast_igetfield_1  , 2, "bi"   , 0, ""      , NullCheck  | CSE),
  def(pop_and_npe_if_null       , 1, "b"    , 0, ""      , NullCheck),
#if !ENABLE_CPU_VARIANT
  def(init_static_array         , 0, ""     , 0, ""      , None),
  def(aload_0_fast_agetfield_4  , 1, "b"    , 0, ""      , Exceptions | CSE),
  def(aload_0_fast_igetfield_4  , 1, "b"    , 0, ""      , Exceptions | CSE),
  def(aload_0_fast_agetfield_8  , 1, "b"    , 0, ""      , Exceptions | CSE),
  def(aload_0_fast_igetfield_8  , 1, "b"    , 0, ""      , Exceptions | CSE),
#elif ENABLE_ARM11_JAZELLE_DLOAD_BUG_WORKAROUND
  def(lload_safe                , 2, "bi"   , 4, "wbii"  , None),
  def(lstore_safe               , 2, "bi"   , 4, "wbii"  , None),  
 fdef(dload_safe                , 2, "bi"   , 4, "wbii"  , None),
 fdef(dstore_safe               , 2, "bi"   , 4, "wbii"  , None),
#endif
  // IMPL_NOTE: the following fast_init_* bytecodes are used only in SVM ROM image,
  // it'll be good to have appropriate GUARANTEEs or #if !ENABLE_ISOLATES
  def(fast_init_1_putstatic     , 3, "bjj"  , 0, ""      , Exceptions),
  def(fast_init_2_putstatic     , 3, "bjj"  , 0, ""      , Exceptions),
  def(fast_init_a_putstatic     , 3, "bjj"  , 0, ""      , Exceptions),
  def(fast_init_1_getstatic     , 3, "bjj"  , 0, ""      , Exceptions),
  def(fast_init_2_getstatic     , 3, "bjj"  , 0, ""      , Exceptions),
  def(fast_init_invokestatic    , 3, "bjj"  , 0, ""      , Exceptions | NoPatching),
  def(fast_init_new             , 3, "bii"  , 0, ""      , Exceptions | NoPatching),

#if USE_DEBUG_PRINTING
  {0, 0, 0, 0, 0}
#else
  {0, 0}
#endif
};

#undef def
#undef fdef

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
#if !ENABLE_CPU_VARIANT
  case _init_static_array: {
    int size_factor = 1 << method->get_byte(bci + 1);
    int count = method->get_native_ushort(bci + 2);
    return 4 + size_factor * count;
  }
#endif //!ENABLE_CPU_VARIANT
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
  for (const BytecodeData* p = data; p->_name != NULL; p++) {
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

