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

/** \class SharedStubs
    Used to generate some routines in the interpreter loop.
*/

#if ENABLE_INTERPRETER_GENERATOR

class SharedStubs: public SourceMacros {
 public:
  SharedStubs(Stream* output) : SourceMacros(output) { }

  void generate();

 public:
  // These two are used for generating ARM compiler glue code.
  void generate_shared_monitor_enter();
  void generate_shared_monitor_exit();

 private:
  void generate_shared_invoke_method();
  void generate_shared_invoke_compiler();
  void generate_shared_fast_accessors();

  void generate_call_on_primordial_stack();

  void generate_shared_call_vm(Label& shared_entry_return_point, 
                                    BasicType return_value_type);
  void generate_shared_entry(Label& shared_entry_return_point);

  void generate_shared_forward_exception(Label& shared_entry_return_point);
  void generate_shared_catch_exception();

  void generate_invoke_pending_entries();

  void generate_fast_memclear();
  void generate_brute_force_icache_flush();
  void generate_shared_invoke_debug();

#if ENABLE_METHOD_TRAPS
  void generate_cautious_invoke();
#endif
};

#endif /*#if ENABLE_INTERPRETER_GENERATOR*/
