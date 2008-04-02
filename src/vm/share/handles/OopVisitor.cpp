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
#include "incls/_OopVisitor.cpp.incl"

#if !defined(PRODUCT) || USE_PRODUCT_BINARY_IMAGE_GENERATOR || ENABLE_TTY_TRACE
#include <ctype.h>

void NamedField::print_on(Stream* st) {
  st->print("  %s", _name);
}


void IndexableField::print_on(Stream* st) {
  st->print("  [%d]", _index);
}

#endif

#ifndef PRODUCT
void SymbolField::print_on(Stream* st) {
#if USE_DEBUG_PRINTING
  st->print("  ");
  _name->print_symbol_on(st);
  if (_renamed) {
    st->print(" ++");
  }
#endif
}
#endif

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

void OopPrinter::prologue() {
  _obj->print_value_on(st());
  st()->print_cr(" = (");
}

class PrinterWrapper : public StackObj {
 public:
  PrinterWrapper(Stream* st, VisitorField* field, int offset, 
                 const char* type_name) {
    field->print_on(st);
    st->print(" ");
    st->fill_to(field->is_named_field() ? 20 : 7);
    this->st        = st;
    this->offset    = offset;
    this->type_name = type_name;
    this->field     = field;
  }
  ~PrinterWrapper() {
    if ((jvm_strlen(field->comment()) > 0) || Verbose) {
      if (st->position() > 50) {
        st->cr();
      }
      st->fill_to(50);
      st->print(" // %s", field->comment());

      if(Verbose) {
        st->print(" (%s@%d)", type_name, offset, field->comment());
      }
    }
    st->cr();
  }
 private:
  Stream*       st;
  int           offset;
  const char*   type_name;
  VisitorField* field;
};

void OopPrinter::save_offset(int offset) {
  //GUARANTEE(offset > _last_offset, "offste must be advancing");
  _last_offset = offset;
}

void OopPrinter::do_oop(VisitorField* field, int offset, bool visit_value) {
  PrinterWrapper pw(st(), field, offset, "oop");
  if (visit_value) {
    Oop::Raw value = _obj->obj_field(offset);
    value().print_value_on(st());
  }
  save_offset(offset);
}

void OopPrinter::do_byte(VisitorField* field, int offset, bool visit_value) {
  PrinterWrapper pw(st(), field, offset, "byte");
  if (visit_value) {
    st()->print("%d", _obj->byte_field(offset));
  }
  save_offset(offset);
}

void OopPrinter::do_char(VisitorField* field, int offset, bool visit_value) {
  PrinterWrapper pw(st(), field, offset, "char");
  jchar c = _obj->char_field(offset);
  if (visit_value) {
    st()->print(c < 256 && isprint(c) ? "'%c'" : "0x%x", c);
  }
  save_offset(offset);
}

void OopPrinter::do_bool(VisitorField* field, int offset, bool visit_value) {
  PrinterWrapper pw(st(), field, offset, "bool");
  if (visit_value) {
    st()->print("%d", _obj->bool_field(offset));
  }
  save_offset(offset);
}

void OopPrinter::do_short(VisitorField* field, int offset, bool visit_value) {
  PrinterWrapper pw(st(), field, offset, "short");
  if (visit_value) {
    st()->print("%d", _obj->short_field(offset));
  }
  save_offset(offset);
}

void OopPrinter::do_int(VisitorField* field, int offset, bool visit_value) {
  PrinterWrapper pw(st(), field, offset, "int");
  if (visit_value) {
    st()->print("%d", _obj->int_field(offset));
  }
  save_offset(offset);
}

void OopPrinter::do_long(VisitorField* field, int offset, bool visit_value) {
  PrinterWrapper pw(st(), field, offset, "long");
  if (visit_value) {
    st()->print(OsMisc_jlong_format_specifier(), _obj->long_field(offset));
  }
  save_offset(offset);
}

void OopPrinter::do_float(VisitorField* field, int offset, bool visit_value) {
  PrinterWrapper pw(st(), field, offset, "float");
  if (visit_value) {
    st()->print_double(jvm_f2d(_obj->float_field(offset)));
  }
  save_offset(offset);
}

void OopPrinter::do_double(VisitorField* field, int offset, bool visit_value) {
  PrinterWrapper pw(st(), field, offset, "double");
  if (visit_value) {
    st()->print_double(_obj->double_field(offset));
  }
  save_offset(offset);
}

void OopPrinter::do_ubyte(VisitorField* field, int offset, bool visit_value) {
  PrinterWrapper pw(st(), field, offset, "ubyte");
  if (visit_value) {
    st()->print("%u", _obj->ubyte_field(offset));
  }
  save_offset(offset);
}

void OopPrinter::do_ushort(VisitorField* field, int offset, bool visit_value) {
  PrinterWrapper pw(st(), field, offset, "ushort");
  if (visit_value) {
    st()->print("%u", _obj->ushort_field(offset));
  }
  save_offset(offset);
}

void OopPrinter::do_uint(VisitorField* field, int offset, bool visit_value) {
  PrinterWrapper pw(st(), field, offset, "uint");
  if (visit_value) {
    if (field->is_pointer() && !VerbosePointers) {
      st()->print("pointer");
    } else if (field->is_hex_output()) {
      st()->print("0x%x", _obj->uint_field(offset));
    } else {
      st()->print("%u", _obj->uint_field(offset));
    }
  }
  save_offset(offset);
}

void OopPrinter::do_ulong(VisitorField* field, int offset, bool visit_value) {
  PrinterWrapper pw(st(), field, offset, "ulong");
  if (visit_value) {
    st()->print(OsMisc_julong_format_specifier(), _obj->ulong_field(offset));
  }
  save_offset(offset);
}

void OopPrinter::do_comment(const char* comment) {
  st()->print_cr("  // %s", comment);
}

void OopPrinter::epilogue() {
  st()->print_cr(")");
}

#endif
