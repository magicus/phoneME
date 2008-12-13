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
# include "incls/_CompiledMethod_i386.cpp.incl"

#if ENABLE_COMPILER

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

int DisassemblerEnv::code_offset(address target) { 
  int result = target - _code->entry();
  if (result < 0 || result > _code->size()) {
    return -1;
  }
  return result;
}

void DisassemblerEnv::comment_on_immediate(unsigned char* pc, int value) {
  static char temp[30];
  int code_offset = pc - _code->entry();

  RelocationReader stream(_code);

  switch (stream.kind_at(code_offset)) {
    case Relocation::oop_type:
    case Relocation::rom_oop_type:
      _oop = (OopDesc*)value;
      break;
    case Relocation::compiler_stub_type:
      _comment = " {Compiler stub}";
      break;
  }
}

void DisassemblerEnv::print_comment(Stream* st) { 
  if (_comment != NULL) { 
    st->print(_comment);
  } else { 
    _oop.print_value_on(st);
  }
}



void CompiledMethod::print_code_on(Stream* st, jint start, jint end) {
  // Warning this is not safe for garbage collection
  address pc = entry() + start;
  while (*pc != 0x00 && pc < entry() + end) {
    DisassemblerEnv env(this);
    address instruction_start = pc;
    st->print(" %4d: ", instruction_start - entry());
    pc = disasm(instruction_start, &env);
    st->print("%s", env.buffer());
    if (env.has_comment()) { 
      st->print("  // ");
      env.print_comment(st);
    }
    st->cr();
  }
}

void CompiledMethod::print_code_on(Stream* st) {
  // Warning this is not safe for garbage collection
  address pc = entry();
  while (*pc != 0x00) {
    DisassemblerEnv env(this);
    address instruction_start = pc;
    print_comment_for(instruction_start - entry(), st);
    st->print(" %4d: ", instruction_start - entry());
    pc = disasm(instruction_start, &env);
    st->print("%s", env.buffer());
    if (env.has_comment()) { 
      st->print("  // ");
      env.print_comment(st);
    }
    st->cr();
  }
}

#endif // !PRODUCT

#endif // COMPILER


#if ENABLE_ROM_GENERATOR

// generate a map of all the field types in this object
int CompiledMethod::generate_fieldmap(TypeArray* field_map) {
  SHOULD_NOT_REACH_HERE();
  return 0;
}

#endif /* #if ENABLE_ROM_GENERATOR*/
