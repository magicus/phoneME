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

class InterpreterGenerator: public SourceMacros {
 public:
  // Construct a new interpreter generator.
  InterpreterGenerator(Stream* output) : SourceMacros(output) { }

  // Generate the interpreter.
  void generate();

  // print actual registers used for Java FP, SP, etc
  static void print_register_definitions();

 private:
  void generate_interpreter_signature();
  void generate_interpreter_method_entry();
  void generate_interpreter_grow_stack();
  void generate_interpreter_fast_method_entry(int extra_locals);
  void generate_interpreter_bytecode_templates();
  void generate_interpreter_bytecode_templates_for(Bytecodes::Code bc);
  void generate_interpreter_bytecode_template(Template* t, bool is_wide);
  void generate_interpreter_dispatch_table();
  void generate_interpreter_bytecode_counters_table();
  void generate_interpreter_pair_counters_table();
  void generate_quick_native_method_entry(BasicType return_type);
  void generate_test_code();
  void generate_fast_memroutines();
  void generate_fast_globals();
#if GENERATE_LIBC_GLUE
  void generate_libc_glue();
  void generate_interwork_stub(const char* name, bool from_thumb = true);
  void generate_fast_routines();
#endif
};

#endif // ENABLE_INTERPRETER_GENERATOR
