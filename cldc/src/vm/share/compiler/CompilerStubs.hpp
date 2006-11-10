/*
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

#if ENABLE_INTERPRETER_GENERATOR

class CompilerStubs: public SourceMacros {
 public:
  CompilerStubs(Stream* output) : SourceMacros(output) { }

  void generate();

 private:
  void generate_compiler_idiv_irem();
  void generate_compiler_rethrow_exception();
  void generate_compiler_new_object();
  void generate_compiler_new_obj_array();
  void generate_compiler_new_type_array();
  void generate_compiler_throw_exceptions();
  void generate_compiler_timer_tick();
  void generate_compiler_checkcast();
  void generate_compiler_instanceof();
  void generate_indirect_execution_sensor_update();
  void generate_handlers();
#if ENABLE_ARM_V7
  void generate_common_method_prolog();
#endif

};

#endif /*#if ENABLE_INTERPRETER_GENERATOR*/
