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

#if ENABLE_INTERPRETER_GENERATOR

#ifndef PRODUCT

class FloatSupport: public SourceMacros {
 public:
  FloatSupport(Stream* output): SourceMacros(output) {}

  void generate();

 private:
#if ARM
  void generate_jvm_fadd();
  void generate_jvm_fsub();
  void generate_jvm_fmul();
  void generate_jvm_fdiv();
  void generate_jvm_frem();  
  void generate_jvm_dcmp();
  void generate_jvm_fcmp();
  void generate_jvm_dadd();
  void generate_jvm_dsub();
  void generate_jvm_dmul();
  void generate_jvm_ddiv();
  void generate_jvm_drem();  
  
  void generate_jvm_i2d();
  void generate_jvm_i2f();
  void generate_jvm_l2f();
  void generate_jvm_l2d();
  void generate_jvm_f2i();
  void generate_jvm_f2l();
  void generate_jvm_f2d();
  void generate_jvm_d2i();
  void generate_jvm_d2l();
  void generate_jvm_d2f();  
  
#if ENABLE_ARM_VFP
  void generate_set_vfp_fast_mode();  
#endif  // ENABLE_ARM_VFP
#endif  // ARM
};

#endif

#endif /*#if ENABLE_INTERPRETER_GENERATOR*/
