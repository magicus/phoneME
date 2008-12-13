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

extern "C" {
  Bytecodes::Code _quicken(Method &method, jint bci JVM_TRAPS);

  Bytecodes::Code _rewrite_field_bytecode(Method &method,
                                          jint bci, 
                                          Bytecodes::Code base,
                                          bool is_get JVM_TRAPS);
  Bytecodes::Code _rewrite_static_field_bytecode(Method &method, jint bci,
                                                 bool is_get,
                                                 bool do_init JVM_TRAPS);
  Bytecodes::Code _quicken_invokestatic(Method &method, jint bci,
                                        bool do_init JVM_TRAPS);
#if ENABLE_INTERPRETER_GENERATOR || ENABLE_ROM_GENERATOR
  // These functions are used only in non-product mode by the Romizer
  // to pre-quicken bytecodes in system classes

  void _quicken_new(Method& method, jint bci JVM_TRAPS);
  void _quicken_anewarray(Method& method, jint bci JVM_TRAPS);
  void _quicken_instanceof(Method& method, jint bci JVM_TRAPS);
  void _quicken_checkcast(Method& method, jint bci JVM_TRAPS);
#endif
}
