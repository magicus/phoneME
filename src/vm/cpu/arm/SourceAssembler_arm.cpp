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
#include "incls/_SourceAssembler_arm.cpp.incl"

// Implementation of SourceAssembler::Label

int SourceAssembler::Label::_next_id = 0;
bool SourceAssembler::_in_glue_code = false;
static int GenerateSDTCode = 0;

void SourceAssembler::Label::import(Stream* s) {
  char *cmd;
  if (GenerateGNUCode) {
    cmd = ".extern";
  } else {
    cmd = "IMPORT";
  }

  GUARANTEE(_state == undefined || _state == imported, "bad label to import");
  if (_state == undefined) {
    s->print("\t%s\t", cmd);
    print_on(s);
    s->cr();
    _state = imported;
  }
}

void SourceAssembler::Label::global(Stream* s) {
  char *cmd;
  if (GenerateGNUCode) {
    cmd = ".global";
  } else {
    cmd = "EXPORT";
  }
  GUARANTEE(_state == undefined, "bad label to make global");
  s->print("\n\t%s\t", cmd);
  print_on(s);
  s->cr();
  _state = exported;
}

void SourceAssembler::Label::make_local(Stream* s) {
  GUARANTEE(_state == undefined, "bad label to make local");
  if (GenerateGNUCode) { 
    _state = internal;
  } else {
    global(s);
  }
}

void SourceAssembler::Label::bind(Stream* s, bool is_global) {
  switch(_state) {
    case undefined:           global(s);                break;
    case anonymous_unused:   /* Fall through */
    case anonymous_used:       _state = anonymous_bound; break;
    case anonymous_bound:     SHOULD_NOT_REACH_HERE();  break;
    default:                                            break; 
  }
  // start a new line if we are not at the beginning
  if (s->position() > 0) {
    s->cr();
  }

  GUARANTEE(s->position() == 0, "wrong label position");
  print_on(s);
  if (GenerateGNUCode) {
    s->print(":");
  } else {
    if (is_global && !GenerateSDTCode) {
      s->print(" PROC");
    }
  }
  s->cr();
}

void SourceAssembler::Label::print_on(Stream* s) const {
  if (is_anonymous()) {
    s->print("L%d", _id);
    if (_state < anonymous_used) {
     ((SourceAssembler::Label*)this)-> _state = anonymous_used;
    }
  } else {
    s->print("%s", _symbol);
  } 
}

SourceAssembler::Label::Label(const char* symbol) {
  if (symbol == NULL) {
    _state  = anonymous_unused;
    _id     = _next_id++;
  } else {
    _state  = undefined;
    _symbol = symbol;
  }
}

SourceAssembler::Label::~Label() {
  // Ideally, all symbolic labels should be defined centrally only once.
  // Then we could also check that a symbolic label has been imported or
  // exported. For now, don't do the check.
  //
  // (The problem right now is that interpreter_call_vm(_redo) really is a
  // 'local' label (not imported or exported) and thus we really should either
  // use an anonymous label or have a 3rd kind of symbolic labels that are
  // local; and than we need to differentiate between export and bind as well).

  GUARANTEE(_state != anonymous_used, "undefined anonymous label");
  if (_state == anonymous_unused) { 
    tty->print("Unused label: ");
    print_on(tty);
    tty->cr();
  }
}

// Implementation of SourceAssembler::Literal

int SourceAssembler::Literal::_next_id = 0;

void SourceAssembler::Literal::print_label_on(SourceAssembler* sasm) {
  sasm->stream()->print("_D%d", _id);
}

void SourceAssembler::Literal::print_value_on(SourceAssembler* sasm) {
  switch (_kind) {
    case integer:
      sasm->define_long(_ivalue);
      break;
    case string :
      sasm->define_bytes(_svalue);
      break;
    case label  : {
      char *tag;
      if (GenerateGNUCode) {
        tag = ".word";
      } else {
        tag = "DCD";
      }
      if (_svalue == NULL) {
        // anonymous (numbered) label
        sasm->stream()->print_cr("\t%s\tL%d", tag, _ivalue);
      } else {
        // symbolic (named) label
        sasm->stream()->print_cr("\t%s\t%s", tag, _svalue);
      }
      break;
    }
    default     : SHOULD_NOT_REACH_HERE();
  }
}

// Implementation of SourceAssembler::LiteralBuffer

void SourceAssembler::LiteralBuffer::add(Literal& lit) {
  GUARANTEE(0 <= _length && _length < max_length,
            "buffer full - increase max_length");
  _data[_length++] = lit;
}

void SourceAssembler::LiteralBuffer::empty(SourceAssembler* sasm) {
  sasm->stream()->print_cr("\t%s", GenerateGNUCode ? ".pool" : "LTORG");
  if (_length > 0) {
    sasm->comment("literals");
    for (int i = 0; i < _length; i++) {
      Literal* lit = &_data[i];
      lit->print_label_on(sasm);
      if (GenerateGNUCode) {
        sasm->stream()->print(":");
      }
      lit->print_value_on(sasm);
    }
    _length = 0;
  }
}

// Implementation of SourceAssembler

void SourceAssembler::emit(int instr) {
  stream()->print("\t");
  disassembler().disasm(NULL, instr);
  emit_comment_and_cr();
}

void SourceAssembler::emit_comment_and_cr() {
  if (!GenerateGPTableOnly) {
    if (_eol_comment[0] != 0 || _use_offset_comments) {
      stream()->print(" ");
      while (stream()->position() <= 40) {
        stream()->print(" ");
      }
      stream()->print("%c", (GenerateGNUCode ? '@' : ';'));
    }
    if (_eol_comment[0] != 0) {
      stream()->print(" %s", _eol_comment);
    }
    if (_use_offset_comments) {
      stream()->print(" offset=%d (0x%x)",
                      _current_commented_offset, _current_commented_offset);
    }
    _eol_comment[0] = 0;
  }
  stream()->cr();
}


void SourceAssembler::ldr_big_integer(Register r, int x, Condition cond) {

  stream()->print("\tldr%s\t%s, =0x%x", cond_name(cond), reg_name(r), x);
  emit_comment_and_cr();
}

void SourceAssembler::start() {
  // header
  comment("Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.");
  comment("DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER");
  comment("");
  comment("This program is free software; you can redistribute it and/or");
  comment("modify it under the terms of the GNU General Public License version");
  comment("2 only, as published by the Free Software Foundation.");
  comment("");
  comment("This program is distributed in the hope that it will be useful, but");
  comment("WITHOUT ANY WARRANTY; without even the implied warranty of");
  comment("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU");
  comment("General Public License version 2 for more details (a copy is");
  comment("included at /legal/license.txt).");
  comment("");
  comment("You should have received a copy of the GNU General Public License");
  comment("version 2 along with this work; if not, write to the Free Software");
  comment("Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA");
  comment("02110-1301 USA");
  comment("");
  comment("Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa");
  comment("Clara, CA 95054 or visit www.sun.com if you need additional");
  comment("information or have any questions.");

  // make sure people know that this file shouldn't be edited
  comment_section("Generated assembly file -- do *not* edit");
 
  if (!GenerateGNUCode) {
    stream()->print_cr("tos_val RN %d", tos_val);
    stream()->print_cr("tos_tag RN %d", tos_tag);
    stream()->print_cr("tmp0    RN %d", tmp0);
    stream()->print_cr("tmp1    RN %d", tmp1);
    stream()->print_cr("bcode   RN %d", bcode);
    stream()->print_cr("bcp     RN %d", bcp);
    stream()->print_cr("gp      RN %d", gp);
    stream()->print_cr("locals  RN %d", locals);
    // sl = r10
    // fp = r11
    stream()->print_cr("tmp2    RN %d", tmp2);
    // sp = r13
    stream()->print_cr("tmp3    RN %d", tmp3);
    // lr is also r14
    // pc == r15
  }
}

void SourceAssembler::stop() {
  if (GenerateGNUCode) {
    stream()->print_cr("\t.end");
  } else { 
    stream()->print_cr("\tEND");
  }
}

void SourceAssembler::beg_segment(Segment *segment, SegmentType segment_type) {
  GUARANTEE(_segment == NULL, "no nested segments");
  _segment = segment;
  int segment_number = 1;

  GUARANTEE(segment_type != no_segment, "must specify segment");
  if (_segment_type != segment_type) {
    if (!GenerateSDTCode && !GenerateGNUCode && !GenerateMicrosoftCode) {
      // Generating code for ADS or RVCT. Need to add PRESERVE8
      // if we're building for RVCT 2.0 or later
      stream()->print_cr("\tIF {ARMASM_VERSION} >= 200000");
      stream()->print_cr("\tPRESERVE8");
      stream()->print_cr("\tENDIF");
    }
    _segment_type = segment_type;   // Since this controls the stream() below
    switch (segment_type) {
      case code_segment:
        if (GenerateSDTCode) {
          stream()->print_cr("\tAREA |.text%d|, CODE", segment_number++);
        } else {
          stream()->print_cr(GenerateGNUCode? ".text":"\tAREA |.text|, CODE");
        }
        break;

      case data_segment:
        if (GenerateSDTCode) {
          stream()->print_cr("\tAREA |.data%d|, DATA", segment_number++);
        } else if (GenerateGNUCode) {
          stream()->print_cr(".data");
        } else {
          stream()->print_cr("\tAREA |.data|, DATA");
        }
        break;

      case bss_segment:
        if (GenerateSDTCode) {
          stream()->print_cr("\tAREA |.data%d|, DATA", segment_number++);
        } else {
          stream()->print_cr(GenerateGNUCode ? ".bss":"\tAREA |.data|, DATA");
        }
        break;

      case gp_segment:
        break;

      default:
        SHOULD_NOT_REACH_HERE();
    }
  }
}

void SourceAssembler::end_segment() {
  if (_segment_type == code_segment) { 
    _literals.empty(this);
  } else {
    // We should guarantee that no literals were created. . .
    if (_segment_type == data_segment) {
    }
  }
  _segment = NULL;
}

void SourceAssembler::comment(const char* fmt, ...) {
  if (!GenerateGPTableOnly) {
    if (jvm_strlen(fmt) == 0) { 
      stream()->cr();
    } else { 
      va_list ap;
      va_start(ap, fmt);
        stream()->print("\t%s ", GenerateGNUCode ? "@ " : ";");
        stream()->vprint(fmt, ap);
        stream()->cr();
      va_end(ap);
    }
  }
}

void SourceAssembler::eol_comment(const char* fmt, ...) {
  if (_eol_comment[0] == 0) {
    va_list ap;
    va_start(ap, fmt);
      jvm_vsprintf(_eol_comment, fmt, ap);
    va_end(ap);
  } else { 
    // comments from inner macros shouldn't override already created ones.
  }
}


void SourceAssembler::comment_section(const char* fmt, ...) {
  stream()->cr();
  stream()->cr();
  comment("------------------------------------------------------");
  va_list ap;
  va_start(ap, fmt);
    stream()->print("\t%s ", GenerateGNUCode ? "@ " : ";");
    stream()->vprint(fmt, ap);
    stream()->cr();
  va_end(ap);
}

void SourceAssembler::align(int alignment) {
  if (alignment > 0) { 
    if (GenerateGNUCode) { 
        stream()->print_cr("\t.align\t%d", jvm_log2(alignment));
    } else { 
        stream()->print_cr("\tALIGN\t%d", alignment);
    }
  }
}

void SourceAssembler::ldr_string(Register r, const char* string, Condition cond) {
  GUARANTEE(r != pc, "probably incorrect code");
  GUARANTEE(string != NULL, "Sanity check");
  Literal lit(string);
  // EVC ASM doesn't understand adrls
#if EVC_ASM_QUIRK
  //  stream()->print("\tldr%s\t%s, _D%d",
   stream()->print("\tadrl%s\t%s, _D%d",
                     cond_name(cond), reg_name(r), lit.id());
#else
   stream()->print("\tadr%s\t%s, _D%d",
                     cond_name(cond), reg_name(r), lit.id());
#endif
  emit_comment_and_cr();
  _literals.add(lit);
}

void SourceAssembler::ldr_label(Register r, Label& L, Condition cond) {
  // Loads the address of the label.
  GUARANTEE(r != pc, "probably incorrect code");

  stream()->print("\tldr%s\t%s, =", cond_name(cond), reg_name(r));
  L.print_on(stream()); 
  emit_comment_and_cr();
}

void SourceAssembler::ldr_nearby_label(Register r, Label& L, Condition cond) {
  // Loads the address of the label.  
  // Label must be within  approximately 256 instructions
  GUARANTEE(r != pc, "probably incorrect code");
  // EVC ASM doesn't understand adrls
#if EVC_ASM_QUIRK
  stream()->print("\tadrl%s\t%s, ", cond_name(cond), reg_name(r));

  //  stream()->print("\tldr%s\t%s, ", cond_name(cond), reg_name(r));
#else
  stream()->print("\tadr%s\t%s, ", cond_name(cond), reg_name(r));
#endif
  L.print_on(stream()); 
  emit_comment_and_cr();
}

void SourceAssembler::ldr_from(Register r, Label& L, int offset, Condition cond) {
  // Loads the contents of the label.  Must be within ~1024 instructions
  stream()->print("\tldr%s\t%s, ", cond_name(cond), reg_name(r));
  L.print_on(stream()); 
  if (offset != 0) {
    stream()->print(" + %d", offset);
  }
  emit_comment_and_cr();
}

void SourceAssembler::b(const Label& L, Condition cond) {
  if (_in_glue_code && !L.is_anonymous()) {
    stream()->print("\tldr%s\tpc, =", cond_name(cond));
  } else {
    stream()->print("\tb%s\t", cond_name(cond));
  }
  L.print_on(stream());
  emit_comment_and_cr();
}

void SourceAssembler::bl(const Label& L, Condition cond) {
  stream()->print("\tbl%s\t", cond_name(cond)); 
  L.print_on(stream());
  emit_comment_and_cr();
}

void SourceAssembler::define_byte(int x) {
  stream()->print("\t%s\t0x%x", (GenerateGNUCode ? ".byte" : "DCB"), x);
  emit_comment_and_cr();
  _current_commented_offset += x;
}

void SourceAssembler::define_long(int x) {
  char *tag, *spec = "0x%x";
  if (GenerateGNUCode) {
    tag = ".long";
  } else {
    tag = "DCD";
  }
  stream()->print("\t%s\t", tag);
  stream()->print(spec, x);
  emit_comment_and_cr();
  _current_commented_offset += sizeof(jint);
}

void SourceAssembler::define_long(const Label& L) {
  char *tag;
  if (GenerateGNUCode) {
    tag = ".long";
  } else {
    tag = "DCD";
  }
  stream()->print("\t%s\t", tag);
  L.print_on(stream()); 
  emit_comment_and_cr();
  _current_commented_offset += sizeof(jint);
}

void SourceAssembler::define_bytes(const char* s, bool word_align) {
  GUARANTEE(s != NULL, "string must exist");
  int d = word_align ? (-(int)(jvm_strlen(s) + 1) & 3) : 0;
  int numbytes = (int)jvm_strlen(s) + d;
  if (GenerateGNUCode) { 
    stream()->print("\t.ascii\t\"%s\\0", s);
    while (d-- > 0) stream()->print("\\0");
    stream()->print("\"");
  } else {
    stream()->print("\tDCB\t\"%s\", 0", s);
    while (d-- > 0) stream()->print(", 0");
  }
  emit_comment_and_cr();
  _current_commented_offset += numbytes;
}

void SourceAssembler::define_zeros(int size) {
  stream()->print("\t%s\t%d", (GenerateGNUCode ? ".space" : "%"), size);
  emit_comment_and_cr();
  _current_commented_offset += size;
}

void SourceAssembler::define_call_info() {
#if ENABLE_EMBEDDED_CALLINFO
  CallInfo ci = CallInfo::interpreter();
  define_long(ci.raw());
#endif
}

int SourceAssembler::find_gp_offset(const char *name) {
#if !ENABLE_THUMB_GP_TABLE
  int offset = 256 * sizeof(OopDesc*); // skip the bytecode table
  if (ENABLE_DISPATCH_TABLE_PADDING) {
    offset += 8 * sizeof(OopDesc*);    // 8 extra bytecodes.
  }
#else
  int offset = 1 * sizeof(OopDesc*); // skip the nop bytecode
#endif

  static const GPTemplate gp_templates[] = {
    GP_SYMBOLS_DO(DEFINE_GP_POINTER, DEFINE_GP_VALUE)
    {NULL, 0, 0, 0}
  };

  for (const GPTemplate* tmpl = gp_templates; tmpl->name; tmpl++) {
    if (jvm_strcmp(name, tmpl->name) == 0) {
      return offset;
    }
    offset += tmpl->size;
    GUARANTEE((offset % 4) == 0, "must be word aligned");
  }

  SHOULD_NOT_REACH_HERE();
  return 0;
}

void SourceAssembler::ldr_using_gp(Register reg, const char *name, 
                                   Condition cond) {
  int offset = find_gp_offset(name);
  ldr(reg, imm_index(gp, offset), cond);
}                  

void SourceAssembler::str_using_gp(Register reg, const char *name, 
                                   Condition cond) {
  int offset = find_gp_offset(name);
  str(reg, imm_index(gp, offset), cond);
}                  

void SourceAssembler::add_using_gp(Register reg, const char *name, 
                                   Condition cond) {
  int offset = find_gp_offset(name);
  if (is_rotated_imm(offset)) {
    eol_comment(name);
    add(reg, gp, imm(offset), cond);
  } else {
    char buff[128];
    jvm_sprintf(buff, "slow add_gp_imm %s %d", name, offset);
    eol_comment(buff);
    offset -= 1024;
    GUARANTEE(is_rotated_imm(1024), "sanity");
    GUARANTEE(is_rotated_imm(offset), "sanity");
    add(reg, gp, imm(1024), cond);
    add(reg, reg, imm(offset), cond);
  }
}                  

void SourceAssembler::bind_global(Label& L) {
  _segment->flush_global();

  L.bind(stream(), true);
  _segment->set_in_global();
}

void SourceAssembler::bind_rom_linkable(const char *name, bool generate_fixed) {
  char fixed_name[256];
  jvm_sprintf(fixed_name, "fixed_%s", name);
  Label L(name);
  Label L_fixed(fixed_name);

  if (generate_fixed) { 
    bind_global(L_fixed); 
    nop(); 
  } 
  bind_global(L);
}

// Implementation of Segment

Segment::Segment(SourceAssembler* sasm,
                 SourceAssembler::SegmentType segment_type,
                 const char* title)
  : _sasm(sasm), _title(title), _in_global(false)
{
  if (SourceAssembler::in_glue_code()) {
    // If glue code contains a data segment, it makes it hard to relocate 
    // the code segment to the Java heap. However, it's OK for the glue code
    // to use "ldr reg, =xxx" to refer to a variable in the data segment.
    GUARANTEE(segment_type == SourceAssembler::code_segment,
              "glue code cannot contain data");
  }

  if (GenerateGPTableOnly) {
    return;
  }
  sasm->beg_segment(this, segment_type);
  sasm->comment("------------------------------------------------------");
  if (title) {
    sasm->comment("Start segment: %s", title);
  } else {
    sasm->comment("Start segment:  unnamed");
  }
}

void Segment::flush_global() {
  if (GenerateGPTableOnly) {
    return;
  }
  if (_in_global) {
    _in_global = false;
    if (!GenerateGNUCode && !GenerateSDTCode) {
      _sasm->stream()->print_cr("\n\tENDP");
    }
  }
}

Segment::~Segment() {
  if (GenerateGPTableOnly) {
    return;
  }
  _sasm->end_segment();
  flush_global();
  if (_title) {
    _sasm->comment("End segment: %s", _title);
  } else {
    _sasm->comment("End segment: unnamed");
  }
  _sasm->stream()->cr();
  _sasm->stream()->cr();
}

// We don't currently use this, but we probably should in the future.
// This makes it easier for debuggers to recognize the function that they are
// in by using the appropriate wrappers.

int FunctionDefinition::_count = 0;

FunctionDefinition::FunctionDefinition(SourceAssembler* sasm, 
                                       const char *name, int type)
    : _sasm(sasm), _name(name)
{
  switch(type) {
    case Global:
      _sasm->bind_global(name);
      break;
    case Local:
      _sasm->bind_local(name);
      break;
    case ROM:
      _sasm->bind_rom_linkable(name);
      break;
  }
}

FunctionDefinition::~FunctionDefinition() {
  if (GenerateGNUCode) { 
    _sasm->stream()->print_cr(".LLfe%d:", _count);
    _count++;
  }
}

#endif // PRODUCT
