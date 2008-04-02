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

#ifndef PRODUCT
#include "incls/_SourceAssembler_i386.cpp.incl"

int SourceAssembler::Label::_next_id = 0;

SourceAssembler::SourceAssembler(Stream* output) {
  // Setup the output stream.
  _output = output;
  _inside_entry = false;
  _current_segment = CODE_SEGMENT;
}

SourceAssembler::~SourceAssembler() {
  // Do nothing.
}

void SourceAssembler::start() {
  _current_segment = NO_SEGMENT;

  // Emit the header.
  emit_comment("Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.");
  emit_comment("DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER");
  emit_comment("");
  emit_comment("This program is free software; you can redistribute it and/or");
  emit_comment("modify it under the terms of the GNU General Public License version");
  emit_comment("2 only, as published by the Free Software Foundation.");
  emit_comment("");
  emit_comment("This program is distributed in the hope that it will be useful, but");
  emit_comment("WITHOUT ANY WARRANTY; without even the implied warranty of");
  emit_comment("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU");
  emit_comment("General Public License version 2 for more details (a copy is");
  emit_comment("included at /legal/license.txt).");
  emit_comment("");
  emit_comment("You should have received a copy of the GNU General Public License");
  emit_comment("version 2 along with this work; if not, write to the Free Software");
  emit_comment("Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA");
  emit_comment("02110-1301 USA");
  emit_comment("");
  emit_comment("Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa");
  emit_comment("Clara, CA 95054 or visit www.sun.com if you need additional");
  emit_comment("information or have any questions.");
  emit("\n");

  if (GenerateInlineAsm)
      if (1)                    // visual c++
          emit("#define __DECLSPEC_ALIGN(x)\n");
      else
          emit("#define __DECLSPEC_ALIGN(x) __declspec(align(x))\n");

  // Make sure people know that this file shouldn't be edited.
  comment_section("Generated assembly file -- do *not* edit");
  emit("\n");
 
  // Emit the platform and model specifiers.
  if (!GenerateGNUCode) {
    if (!GenerateInlineAsm) {
      emit("\t.486P\n");
      emit("\t.MODEL flat, C\n\n");
    }
  } else
    emit("\t.arch i486\n");
  start_code_segment();
}

void SourceAssembler::stop() {
  stop_code_segment();
  if (!GenerateGNUCode)
    if (!GenerateInlineAsm)
      emit("\n\tEND\n");
    else
      emit("\n// END\n");
}

void SourceAssembler::start_code_segment() { 
  if (_current_segment != NO_SEGMENT) {
    // complain?
  }
  _current_segment = CODE_SEGMENT;

  // Emit start of code segment header.
  if (!GenerateGNUCode) {
    if (GenerateInlineAsm)
      emit("// { "); // hide next line in the comment
    emit("CodeSegment segment  para  public   'CODE'\n");
  } else
    emit(".text\n");
  
}

void SourceAssembler::start_data_segment() { 
  if (_current_segment != NO_SEGMENT) {
    // complain?
  }
  _current_segment = DATA_SEGMENT;

  // Emit start of data segment header.
  if (!GenerateGNUCode) {
    if (GenerateInlineAsm)
      emit("// { "); // hide next line in the comment
    emit("DataSegment segment  para  public   'DATA'\n");
  } else
    emit(".data\n");
}

void SourceAssembler::stop_code_segment() {
  if (_current_segment != CODE_SEGMENT) {
    // complain?
  }
  _current_segment = NO_SEGMENT;

  // Emit the end of code segment footer.
  if (!GenerateGNUCode)
    if (!GenerateInlineAsm)
      emit("\nCodeSegment ends\n");
    else
      emit("\n// } CodeSegment ends\n");
}

void SourceAssembler::stop_data_segment() {
  if (_current_segment != DATA_SEGMENT) {
    // complain?
  }
  _current_segment = NO_SEGMENT;

  // Emit the end of data segment footer.
  if (!GenerateGNUCode)
    if (!GenerateInlineAsm)
      emit("\nDataSegment ends\n");
    else
      emit("\n// } DataSegment ends\n");
}

void SourceAssembler::jcc(Condition condition, const Constant& cst) {
  if (cst.is_reference()) {
    if (!GenerateGNUCode)
      if (!GenerateInlineAsm)
        emit("\tEXTERNDEF %s%s:PROC\n", extern_c_prefix(), cst.reference());
      else
        emit("\t{\n\textern void %s();\n", cst.reference());
    else
      emit("\t.extern %s%s\n", extern_c_prefix(), cst.reference());
  }
  if (!GenerateInlineAsm)
    emit("\tj");
  else
    emit("\t__asm j");
  emit_cc(condition);
  emit(" ");
  emit_constant(cst);
  emit("\n");
  if (GenerateInlineAsm && cst.is_reference())
    emit("\t}\n");
}

void SourceAssembler::define_call_info() {
#if ENABLE_EMBEDDED_CALLINFO
  // Only the interpreter defined call info in the source assembler
  CallInfo ci = CallInfo::interpreter();
  emit_comment("call info");
  if (!GenerateGNUCode)
    if (!GenerateInlineAsm)
      emit("\ttest eax, %d\n", ci.raw());
    else
      emit("\t__asm test eax, %d\n", ci.raw());
  else
    emit("\ttest $%d, %%eax\n", ci.raw());
#endif // ENABLE_EMBEDDED_CALLINFO
}

void SourceAssembler::align(int alignment) {
  if (alignment > 0) {
    if (GenerateGNUCode) {
      emit("\t.align %d\n", alignment);
    } else if (!GenerateInlineAsm) {
      emit("\tALIGN %d\n", alignment);
    } else {
      if (_inside_entry) {
        emit("\t__asm ALIGN %d\n", alignment);
      } else {
        emit("__DECLSPEC_ALIGN(%d) ", alignment);
      }
    }
  }
}

void SourceAssembler::bind(const Label& label, int alignment) {
  align(alignment);
  if (GenerateGNUCode)
    emit(".");
  emit("L%d:\n", label.id());
}

void SourceAssembler::entry(const char* name, int alignment) {
  _inside_entry = true;
  emit("\n");
  if (!GenerateInlineAsm) {
    align(alignment);
    if (!GenerateGNUCode)
      emit("\tPUBLIC %s%s\n", extern_c_prefix(), name);
    else
      emit("\t.global %s%s\n", extern_c_prefix(), name);
    emit("%s%s:\n", extern_c_prefix(), name);
  } else {
    emit("extern __declspec(naked)");
    if (alignment)
      emit(" __DECLSPEC_ALIGN(%d)", alignment);
    emit(" void\n%s()\n{\n", name);
  }
}

void SourceAssembler::alt_entry(const char* name, int alignment) {
  if (!GenerateInlineAsm) {
    entry(name, alignment);
  }
  else {
    // IMPL_NOTE: enable later
    // SHOULD_NOT_REACH_HERE();
  }
}

void SourceAssembler::entry_end() {
  _inside_entry = false;
  if (GenerateInlineAsm)
    emit("\n}\n");
}

void SourceAssembler::rom_linkable_entry(const char* name, int alignment) {
  if (!GenerateGNUCode) {
    emit("\n");
    if (!GenerateInlineAsm) {
      align(alignment);
      emit("\tPUBLIC %s%s\n", extern_c_prefix(), name);
      emit("%s%s:\n", extern_c_prefix(), name);
    }
    else {
      entry(name, alignment /*???*/);
    }
  } else {
    entry(name);
  }
}

void SourceAssembler::rom_linkable_entry_end() {
  entry_end();
}

// Returns from a static buffer
const char * SourceAssembler::extern_c_prefix() {
  if (AddExternCUnderscore) {
    return "_";
  } else {
    return "";
  }
}

void SourceAssembler::comment(const char* format, ...) {
  emit("\n");
  va_list arguments;
  va_start(arguments, format);
  emit_comment(format, arguments);
  va_end(arguments);
}

void SourceAssembler::comment_section(const char* format, ...) {
  emit("\n");
  emit_comment("----------------------------------------------------------");
  va_list arguments;
  va_start(arguments, format);
  emit_comment(format, arguments);
  va_end(arguments);
  emit_comment("----------------------------------------------------------");
}

SourceAssembler::Condition SourceAssembler::negate_cc(Condition condition) {
  switch (condition) {
  case equal         : return not_equal;
  case not_equal     : return equal;
  case less          : return greater_equal;
  case less_equal    : return greater;
  case greater       : return less_equal;
  case greater_equal : return less;
  case below         : return above_equal;
  case below_equal   : return above;
  case above         : return below_equal;
  case above_equal   : return below;
  case overflow      : return no_overflow;
  case no_overflow   : return overflow;
  case negative      : return positive;
  case positive      : return negative;
  case parity        : return no_parity;
  case no_parity     : return parity;
  }

  SHOULD_NOT_REACH_HERE();
  return zero;
}

// VC++ inline asm doesn't support D[BWDQ] directives after the __asm
// keyword.  But it can emit constant bytes using "_emit" keyword.
void SourceAssembler::emit_byte(const Constant& cst) {
  int imm = cst.immediate();
  if (GenerateInlineAsm)
    emit("\t__asm _emit 0x%02x\n", imm);
  else if (GenerateGNUCode)
    emit("\t.byte \t0x%02x\n", imm);
  else
    emit("\tdb %d\t\n", imm);
}

void SourceAssembler::emit_data_name(const char* c_type, const char* name) {
  GUARANTEE(name != NULL, "Must supply name");

  if (GenerateInlineAsm) {
    emit("\n%s %s", c_type, name);
  } else if (GenerateGNUCode) {
    emit("\n\t.global %s%s\n", extern_c_prefix(), name);
    emit("%s%s:\n", extern_c_prefix(), name);
  } else {
    emit("\n\tPUBLIC %s%s\n", extern_c_prefix(), name);
    // no newline so that Dx is on the same line
    emit("%s%s ", extern_c_prefix(), name);
  }
}

void SourceAssembler::emit_data_value(OperandSize size, const Constant& cst) {
  if (GenerateInlineAsm) {
    emit_constant_displacement(cst);
  } else if (GenerateGNUCode) {
    switch (size) {
      case byte_operand      : emit("\t.byte \t");   break;
      case word_operand      : emit("\t.word \t");   break;
      case long_operand      : emit("\t.long \t");   break;
      case very_long_operand : emit("\tdq \t");      break;
      case very_very_long_operand : emit("\tdt \t"); break;
      default                : SHOULD_NOT_REACH_HERE();
    }
    emit_constant_displacement(cst);
    emit("\n");
  } else {
    switch (size) {
      case byte_operand      : emit("\tdb \t");      break;
      case word_operand      : emit("\tdw \t");      break;
      case long_operand      : emit("\tdd \t");      break;
      case very_long_operand : emit("\tdq \t");      break;
      case very_very_long_operand : emit("\tdt \t"); break;
      default                : SHOULD_NOT_REACH_HERE();
    }
    emit_constant_displacement(cst);
    emit("\n");
  }
}

void SourceAssembler::emit_variable(const char* c_type, OperandSize size,
                                    const char* name, const Constant& cst) {
  emit_data_name(c_type, name);
  if (GenerateInlineAsm)
    emit(" = ");
  emit_data_value(size, cst);
  if (GenerateInlineAsm)
    emit(";\n");
}

void SourceAssembler::define_array_begin(const char* c_type, const char* name,
                                         int size) {
  emit_data_name(c_type, name);
  if (GenerateInlineAsm) {
    emit("[");
    if (size > 0)
      emit("%d", size);
    emit("] = {\n");
  }
}

void SourceAssembler::emit_array_element(OperandSize size,
                                         const Constant& cst) {
  emit_data_value(size, cst);
  if (GenerateInlineAsm)
    emit(",\n");
}

void SourceAssembler::define_array_end() {
  if (GenerateInlineAsm)
    emit("};\n");
}

void SourceAssembler::define_struct_begin(const char* struct_type_name, 
                                          const char* struct_name) {
  emit_comment("Definition of %s", struct_name);
  if (GenerateInlineAsm) {
    emit("struct %s {\n", struct_type_name);
  } else {
    entry(struct_name);
  }
}

void SourceAssembler::define_struct_field(const char* struct_name,
                                          const char* c_type, 
                                          const char* name, 
                                          OperandSize size) {
  emit_data_name(c_type, name);
  if (GenerateInlineAsm) {
    emit(";\n");
    emit("#define %s %s.%s\n", name, struct_name, name);
  } else {
    emit_data_value(size, Constant(0));
  }
}

void SourceAssembler::define_struct_end(const char* struct_name) {
  if (GenerateInlineAsm) {
    emit("} %s;\n", struct_name);
  }
  emit_comment("End definition of struct %s", struct_name);
}

void SourceAssembler::emit_data(OperandSize size, const Constant& cst,
                                int dups, const char* name) {
  GUARANTEE(!GenerateInlineAsm, "No inline data in inline asm");

if (!GenerateGNUCode) { 
  if (name != 0) {
    emit("\n\tPUBLIC %s%s\n", extern_c_prefix(), name);
  }
  if (cst.is_reference()) {
    emit("\tEXTERNDEF %s%s:PROC\n", extern_c_prefix(), cst.reference());
  }
  if (name != 0) {
    emit("%s%s ", extern_c_prefix(), name);
  }
  switch (size) {
  case byte_operand      : emit("\tdb \t"); break;
  case word_operand      : emit("\tdw \t"); break;
  case long_operand      : emit("\tdd \t"); break;
  case very_long_operand : emit("\tdq \t"); break;
  case very_very_long_operand : emit("\tdt \t"); break;
  default                : SHOULD_NOT_REACH_HERE();
  }
  if (dups > 1) {
    emit("%d\tdup\t(", dups);
    emit_constant(cst);
    emit(")");
  } else {
   emit_constant(cst);
  }
  emit("\n");
} else {
  if (name != 0) {
    emit("\n\t.global %s%s\n", extern_c_prefix(), name);
  }
  if (cst.is_reference()) {
    emit("\t.extern %s%s\n", extern_c_prefix(), cst.reference());
  }
  if (name != 0) {
    emit("%s%s: ", extern_c_prefix(), name);
  }
  if (dups > 1) {
    emit(".rept %d\n\t", dups);

  }
  switch (size) {
  case byte_operand      : emit("\t.byte \t");   break;
  case word_operand      : emit("\t.word \t");   break;
  case long_operand      : emit("\t.long \t");   break;
  case very_long_operand : emit("\tdq \t");      break;
  case very_very_long_operand : emit("\tdt \t"); break;
  default                : SHOULD_NOT_REACH_HERE();
  }
  emit_constant_displacement(cst);

  if (dups > 1)
    emit("\n\t.endr");

  emit("\n");
}
}

void SourceAssembler::emit(const char* format, ...) {
  va_list arguments;
  va_start(arguments, format);
  _output->vprint(format, arguments);
  va_end(arguments);
}

void SourceAssembler::emit_instruction(const char* format, ...) {
  va_list arguments;
  va_start(arguments, format);
  emit_instruction(format, arguments);
  va_end(arguments);
}

void SourceAssembler::emit_instruction(const char* format, va_list arguments) {
  // Due to a peculiarity in the Microsoft SourceAssembler we have to
  // make a special case out of jumps and calls. Try figuring out if
  // it's either of them.
  const bool jump_or_call = (strncmp(format, "jmp", 3) == 0) || (strncmp(format, "call", 4) == 0);
  const bool loop = (strncmp(format, "loop", 4) == 0);
  const bool is_int3 = (strncmp(format, "int 3", 5) == 0);
  const bool has_cl = (strlen(format) > 10 && strncmp(format+10, "cl", 2) == 0);
if (!GenerateGNUCode) {
  bool has_externals = emit_externals(format, arguments);
  emit("\t");
  if (GenerateInlineAsm)
    emit("__asm ");
  while (*format) {
    if (*format == '%') {
      format++;
      
      // Parse the size specifier.
      OperandSize size = long_operand;
      switch (*format) {
      case 't' : size = very_very_long_operand; format++; break;
      case 'v' : size = very_long_operand; format++;      break;
      case 'l' : size = long_operand; format++;           break;
      case 'w' : size = word_operand; format++;           break;
      case 'b' : size = byte_operand; format++;           break;
      }

      switch (*format) {
      case 'c' : { // Constant
        const Constant* cst = va_arg(arguments, const Constant *);
        if (!cst->is_immediate() && !jump_or_call) {
          emit("offset ");
        }
        emit_constant(*cst);
        break;
      }
      case 'a' : { // Address. 
        const Address* adr = va_arg(arguments, const Address *);
        emit_address(*adr, size);
        break; 
      }
      case 'r' : { // Register.
        const Register* reg = va_arg(arguments, const Register *);
        emit_register(*reg, size);
        break;
      }
      }  
    } else {
      emit("%c", *format);
    }
    format++;
  }
  emit("\n");
  if (has_externals && GenerateInlineAsm)
    emit("\t}\n");
} else {
  // I think this code need to be clean up somewhat
  char* rev_format;
  if (*format && !((strncmp(format, "ffree", 5) == 0) || 
                   (strncmp(format, "fld", 3) == 0)   ||                   
                   (strncmp(format, "fstp", 4) == 0)  ||
                   (strstr(format, "st(") != NULL)))  {
    rev_format = reverse_format(format); 
    format = rev_format;
  } else {
    if (strstr(format, "st(")) {
      emit("\t%s\n", format);
      return;
    }
  }
  emit_externals(format, arguments);
    emit("\t");
  if (is_int3) {
    emit("int $3\n");
    return;
  }
  while (*format) {
    if (*format == '%') {
      format++;
      
      // Parse the size specifier.
      OperandSize size = long_operand;
      switch (*format) {
      case 't' : size = very_very_long_operand; format++; break;
      case 'v' : size = very_long_operand; format++;      break;
      case 'l' : size = long_operand; format++;           break;
      case 'w' : size = word_operand; format++;           break;
      case 'b' : size = byte_operand; format++;           break;
      }

      switch (*format) {
      case 'c' : { // Constant
        const Constant* cst = va_arg(arguments, const Constant*);
        if (jump_or_call || loop) {
          emit_constant(*cst);
        } else {
          emit_constant2(*cst);
        }
        break;
      }
      case 'a' : { // Address. 
        const Address* adr = va_arg(arguments, const Address*);
        if (jump_or_call) {
          emit("*");
        }
        emit_address(*adr, size);
        break; 
      }
      case 'r' : { // Register.
        const Register* reg = va_arg(arguments, const Register*);
        if (jump_or_call) {
          emit("*");
        }
        emit("%%");     
        emit_register(*reg, size);
        break;
      }
      }  
    } else {
      if (has_cl && *format == 'c')
        emit("%%");
      emit("%c", *format);
    }
    format++;
  }
  emit("\n");
}
}

bool SourceAssembler::emit_externals(const char* format, va_list arguments)
{
  bool emited = false;

  // Due to a peculiarity in the Microsoft SourceAssembler we have to
  // make a special case out of jumps and calls. Try figuring out if
  // it's either of them.
  const bool jump_or_call = (strncmp(format, "jmp", 3) == 0) || (strncmp(format, "call", 4) == 0);
if (!GenerateGNUCode) {
  while (*format) {
    if (*format == '%') {
      format++;

      if (*format == 'v' || *format == 'l' || *format == 'w' || *format == 'b') {
        // Ignore size specifier.
        format++;
      }
          
      switch (*format) {
      case 'c' : { // Constant.
        const Constant* cst = va_arg(arguments, const Constant*);
        if (cst->is_reference()) {
          const char * const reference = cst->reference();

          if (!emited && GenerateInlineAsm)
            emit("\t{\n");
          emited = true;
          if (!GenerateInlineAsm)
            emit("\tEXTERNDEF %s%s:%s\n", extern_c_prefix(), reference,
                 (jump_or_call || cst->is_proc()) ? "PROC" : "DWORD");
          else if (jump_or_call || cst->is_proc())
            emit("\textern void %s();\n", reference);
          else if (!is_jvm_fast_global(reference)) {
            // jvm_fast_globals are already defined as structure fields,
            // do not to declare them here.
            emit("\textern int %s;\n", reference);
          }
        }
        break;
      }
      case 'a' : { // Address.
        const Address* adr = va_arg(arguments, const Address*);
        if (adr->displacement().is_reference()) {
          const char * const reference = adr->displacement().reference();

          if (!emited && GenerateInlineAsm)
            emit("\t{\n");
          emited = true;
          if (!GenerateInlineAsm)
            emit("\tEXTERNDEF %s%s:%s\n", extern_c_prefix(), reference,
                 (adr->displacement().is_proc()) ? "PROC" : "DWORD");
          else {
            // IMPL_NOTE: we need to special case this, b/c compiler will be
            // unhappy after it has seen the definition of this array.
            if (strcmp(reference, "interpreter_dispatch_table") == 0)
              emit("\textern void * const interpreter_dispatch_table[256];\n");
            else if (!is_jvm_fast_global(reference)) {
              // jvm_fast_globals are already defined as structure fields,
              // do not declare them here.
              emit("\textern int* %s;\n", reference);
            }
          }
        }
        break;
      }
      case 'r' : { // Register.
        const Register* reg = va_arg(arguments, const Register*);
        break;
      }
      }
    }
    format++;
  }
} else {
  while (*format) {
    if (*format == '%') {
      format++;

      if (*format == 'v' || *format == 'l' || *format == 'w' || *format == 'b') {
        // Ignore size specifier.
        format++;
      }
          
      switch (*format) {
      case 'c' : { // Constant.
        const Constant* cst = va_arg(arguments, const Constant*);
        if (cst->is_reference()) {
          emit("\t.extern %s%s\n", extern_c_prefix(), cst->reference());
        }
        break;
      }
      case 'a' : { // Address.
        const Address* adr = va_arg(arguments, const Address*);
        if (adr->displacement().is_reference()) {
          emit("\t.extern %s%s\n", extern_c_prefix(), 
               adr->displacement().reference());
        }
        break;
      }
      case 'r' : { // Register.
        const Register* reg = va_arg(arguments, const Register*);
        break;
      }
      }
    }
    format++;
  }
}
  return emited;
}

void SourceAssembler::emit_register(const Register reg, OperandSize size) {
  if (!GenerateGNUCode)
    switch (size) {
        case byte_operand : emit("%s", name_for_byte_register(reg)); break;
        case word_operand : emit("%s", name_for_work_register(reg)); break;
        case long_operand : emit("%s", name_for_long_register(reg)); break;
         default           : SHOULD_NOT_REACH_HERE();
    }
  else
    switch (size) {
        case byte_operand : emit("%s", name_for_byte_register(reg)); break;
        case word_operand : emit("%s", name_for_work_register(reg)); break;
        case long_operand : emit("%s", name_for_long_register(reg)); break;
         default           : SHOULD_NOT_REACH_HERE();
    }
}

void SourceAssembler::emit_address(const Address& adr, OperandSize size) {
   // The Microsoft SourceAssembler does not like prefixing addresses with '+'. Make sure
   // we do not emit any such code.
   bool first_part = true;

  if (!GenerateGNUCode) { 

   switch (size) {
   case byte_operand           : emit("byte ptr [");  break;
   case word_operand           : emit("word ptr [");  break;
   case long_operand           : emit("dword ptr ["); break;
   case very_long_operand      : emit("qword ptr ["); break;
   case very_very_long_operand : emit("tbyte ptr ["); break;
   }

   // Emit the base if any.
   if (adr.base() != no_reg) {
     emit_register(adr.base(), long_operand);
     first_part = false;
   }
   
   // Emit the index if any.
   if (adr.index() != no_reg){
     if (!first_part) emit(" + ");
     emit_register(adr.index(), long_operand);
     emit(" * %d", (1 << adr.scale()));
     first_part = false;
   }   

   // Emit the displacement.
   if (!first_part) emit(" + ");
   emit_constant(adr.displacement());
   emit("]");  

  } else {

   // Emit the displacement.
   emit_constant_displacement(adr.displacement());
   first_part = false;
   
   if (adr.base() != no_reg) {
     emit("(%%");
     emit_register(adr.base(), long_operand);
     first_part = true;
   }
   
   // Emit the index if any.
   if (adr.index() != no_reg){
     if (!first_part) {
        emit("(");
    first_part = true;
     }
     emit(",%%");
     emit_register(adr.index(), long_operand);
     emit(",%d", (1 << adr.scale()));
   }   
   if (first_part)
     emit(")");  
   
  }
}

void SourceAssembler::emit_constant(const Constant& cst) {
  if (cst.is_immediate()) {
    emit((GenerateGNUCode ? "$%d" : "%d"), cst.immediate());
  } else { 
    if (cst.is_reference()) { 
      emit("%s%s", extern_c_prefix(), cst.reference());
    } else { 
      emit((GenerateGNUCode ? ".L%d" : "L%d"), cst.label().id());
    } 
    if (cst.offset() != 0) { 
      emit(" + %d", cst.offset());
    }
  }
}

void SourceAssembler::emit_constant2(const Constant& cst) {
  if (cst.is_immediate()) {
    emit((GenerateGNUCode ? "$%d" : "%d"), cst.immediate());
  } else { 
    if (cst.is_reference()) { 
      emit((GenerateGNUCode ? "$(%s%s" : "(%s%s"), extern_c_prefix(),
           cst.reference());
    } else { 
      emit((GenerateGNUCode ? "$(.L%d" : "(L%d"), cst.label().id());
    } 
    if (cst.offset() != 0) { 
      emit(" + %d", cst.offset());
    }
    emit(")");
  }
}

void SourceAssembler::emit_constant_displacement(const Constant& cst) {
  if (cst.is_immediate()) {
    emit("%d", cst.immediate());
  } else { 
    if (cst.is_reference()) {
      emit("%s%s", extern_c_prefix(), cst.reference());
    } else {
      emit((GenerateGNUCode ? ".L%d" : "L%d"), cst.label().id());
    }
    if (cst.offset() != 0) { 
      emit(" + %d", cst.offset());
    }
  }
}

void SourceAssembler::emit_cc(Condition condition) {
  switch (condition) {
  case equal         : emit("e");  break;
  case not_equal     : emit("ne"); break;
  case less          : emit("l");  break;
  case less_equal    : emit("le"); break;
  case greater       : emit("g");  break;
  case greater_equal : emit("ge"); break;
  case below         : emit("b");  break;
  case below_equal   : emit("be"); break;
  case above         : emit("a");  break;
  case above_equal   : emit("ae"); break;
  case overflow      : emit("o");  break;
  case no_overflow   : emit("no"); break;
  case negative      : emit("s");  break;
  case positive      : emit("ns"); break;
  case parity        : emit("p");  break;
  case no_parity     : emit("np"); break;
  default            : SHOULD_NOT_REACH_HERE(); break;
  }
}

void SourceAssembler::emit_comment(const char* format, ...) {
  va_list arguments;
  va_start(arguments, format);
  emit_comment(format, arguments);
  va_end(arguments);
}

char* SourceAssembler::reverse_format(const char* format) {

  static char buf[32];
  char savepart[8] = {0};
  char* saveptr;
  int i = 0;
#if NOT_CURRENTLY_USED
  int j = 0;
#endif
  bool gotsaved = false;

 saveptr = (char*) format;
 while (*format) {
    if (*format == '%' && i < 8   && (strstr(format, "st(") == NULL)) {
      savepart[0] = *format++;
      if (*format == 'c') {
        savepart[1] = *format++;
        savepart[2] = '\0';
      } else {
        savepart[1] = *format++;
        savepart[2] = *format++;
        savepart[3] = '\0';
      }
      gotsaved = true;
    } else if (*format == ',') {
      format++;    // Comma
      format++;    // Space
      buf[i++] = *format++; // Percent
      if (*format == 'c' || buf[i-1] == 'c') {
        buf[i++] = *format++;
      } else {
        buf[i++] = *format++;
        buf[i++] = *format++;
      }
      buf[i++] = ',';
      buf[i++] = ' ';
      buf[i++] = savepart[0];
      buf[i++] = savepart[1];
      buf[i++] = savepart[2];
      gotsaved = false;
    } else {
      buf[i++] = *format++;
    }
  }
  if (gotsaved) {
      buf[i++] = savepart[0];
      buf[i++] = savepart[1];
      buf[i++] = savepart[2];
  }
  buf[i] = '\0';

  return(&buf[0]);

#if NOT_CURRENTLY_USED
  format = saveptr;
  for (j = 0; j < i; j++)
    *saveptr++ = buf[j];

  char buf[32];
  char savepart[8];
  int i;
  bool gotsaved = false;
  i = 0;
  
  while (*format) {
    if (*format == '%' && i < 8 ) {
      savepart[0] = *format++;
      if (*format == 'c') {
        savepart[1] = *format++;
        savepart[2] = '\0';
      } else {
        savepart[1] = *format++;
        savepart[2] = *format++;
        savepart[3] = '\0';
      }
      gotsaved = true;
    } else if (*format == ',') {
      format++;    // Comma
      format++;    // Space
      buf[i++] = *format++; // Percent
      if (*format == 'c' || buf[i-1] == 'c') {
        buf[i++] = *format++;
      } else {
        buf[i++] = *format++;
        buf[i++] = *format++;
      }
      buf[i++] = ',';
      buf[i++] = ' ';
      buf[i++] = savepart[0];
      buf[i++] = savepart[1];
      buf[i++] = savepart[2];
      gotsaved = false;
    } else {
      buf[i++] = *format++;
    }
  }
  if (gotsaved) {
      buf[i++] = savepart[0];
      buf[i++] = savepart[1];
      buf[i++] = savepart[2];
  }
  buf[i] = '\0';
  return(&buf[0]);
#endif
}

void SourceAssembler::emit_comment(const char* format, va_list arguments) {
  // Compute the comment message.
  char message[2048];

  vsnprintf(message, 2048, format, arguments);

  const char *comment_start;
  if (GenerateInlineAsm)
    comment_start = "//";
  else if (GenerateGNUCode)
    comment_start = "#";
  else
    comment_start = ";";

  // Print the error message line by line.
  char* begin = message;
  while (true) {
    // Get the next line.
    char* end = strchr(begin, '\n');
    if (!end) break;

    // Print the line.
    *end = '\0';
    emit("\t%s %s\n", comment_start, begin);
    begin = end + 1;
  }

  // Print the last line.
  if (*begin)
    emit("\t%s %s\n", comment_start, begin);
}

bool SourceAssembler::is_jvm_fast_global(const char * const reference) {
#define DECLARE_NAME(DUMMY, type, name) "_" XSTR(name),
  static const char * const jvm_fast_global_names[] = {
    FORALL_JVM_FAST_GLOBALS(DECLARE_NAME, DUMMY)
  };
#undef DECLARE_NAME

  for (int i = 0; i < ARRAY_SIZE(jvm_fast_global_names); i++) {
    if (jvm_strcmp(reference, jvm_fast_global_names[i]) == 0) {
      return true;
    }
  }

  return false;
}

#endif
