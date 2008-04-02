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

#if !defined(PRODUCT) || USE_PRODUCT_BINARY_IMAGE_GENERATOR || ENABLE_TTY_TRACE

class VisitorField : public StackObj {
 public:
  VisitorField(bool in_vm, const char* comment) {
    _in_vm      = in_vm;
    _comment    = comment;
    _hex_output = false;
    _is_pointer = false;
  }
  virtual void print_on(Stream* st) JVM_PURE_VIRTUAL_1_PARAM(st);
  const char* comment() { return _comment; }
  virtual bool is_named_field() const { return false; }
  void set_hex_output(bool value) {
    _hex_output = value;
  }
  bool is_hex_output() {
    return _hex_output;
  }
  void set_is_pointer(bool value) {
    _is_pointer = value;
  }
  bool is_pointer() {
    return _is_pointer;
  }
 private:
  bool  _in_vm;
  bool  _hex_output;
  bool  _is_pointer;
  const char* _comment;
};

class OopROMVisitor : public StackObj {

public:
  virtual void do_oop(VisitorField* field, int offset, bool visit_value)    
    JVM_PURE_VIRTUAL_3_PARAM(field, offset, visit_value);
  virtual void do_comment(const char* comment) 
    JVM_PURE_VIRTUAL_1_PARAM(comment);
  virtual void do_int(VisitorField* field, int offset, bool visit_value)
    JVM_PURE_VIRTUAL_3_PARAM(field, offset, visit_value);

  void set_obj(Oop* obj) {
    _obj = obj;
  }

  // Returns the object being visited
  ReturnOop obj() {
    return _obj->obj();
  }


  Oop* _obj;
};

class IndexableField : public VisitorField {
 public:
  IndexableField(int index, bool in_vm, const char* comment = "")
      : VisitorField(in_vm, comment)
  {
    _index = index;
  }
  virtual void print_on(Stream* st);
 private:
  int _index;
};
class NamedField : public VisitorField {
 public:
  NamedField(const char* name, bool in_vm, const char* comment = "")
      : VisitorField(in_vm, comment)
  {
    _name = name;
  }
  virtual bool is_named_field() const { return true; }
  virtual void print_on(Stream* st);
 private:
  const char* _name;
};

#endif

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
class OopVisitor : public OopROMVisitor {
 public:

  // Called before visiting an object
  virtual void prologue() JVM_PURE_VIRTUAL;

  // Callback methods for each field type in an object
  virtual void do_byte(VisitorField* field, int offset, bool visit_value) {
    // pure-virtual
    //SHOULD_NOT_REACH_HERE(); (void)field; (void)offset; (void)visit_value;
    JVM_PURE_VIRTUAL_3_PARAM(field, offset, visit_value);
  }
  virtual void do_char(VisitorField* field, int offset, bool visit_value)
    JVM_PURE_VIRTUAL_3_PARAM(field, offset, visit_value);
  virtual void do_bool(VisitorField* field, int offset, bool visit_value) {
    // pure-virtual
    //SHOULD_NOT_REACH_HERE(); (void)field; (void)offset; (void)visit_value;
    JVM_PURE_VIRTUAL_3_PARAM(field, offset, visit_value);
  }
  virtual void do_short(VisitorField* field, int offset, bool visit_value) {
    // pure-virtual
    //SHOULD_NOT_REACH_HERE(); (void)field; (void)offset; (void)visit_value;
    JVM_PURE_VIRTUAL_3_PARAM(field, offset, visit_value);
  }
  virtual void do_long(VisitorField* field, int offset, bool visit_value) {
    // pure-virtual
    //SHOULD_NOT_REACH_HERE(); (void)field; (void)offset; (void)visit_value;
    JVM_PURE_VIRTUAL_3_PARAM(field, offset, visit_value);
  }
  virtual void do_float(VisitorField* field, int offset, bool visit_value) {
    // pure-virtual
    //SHOULD_NOT_REACH_HERE(); (void)field; (void)offset; (void)visit_value;
    JVM_PURE_VIRTUAL_3_PARAM(field, offset, visit_value);
  }
  virtual void do_double(VisitorField* field, int offset, bool visit_value) {
    // pure-virtual
    //SHOULD_NOT_REACH_HERE(); (void)field; (void)offset; (void)visit_value;
    JVM_PURE_VIRTUAL_3_PARAM(field, offset, visit_value);
  }
  // Unsigned versions
  virtual void do_ubyte(VisitorField* field, int offset, bool visit_value) {
    // pure-virtual
    //SHOULD_NOT_REACH_HERE(); (void)field; (void)offset; (void)visit_value;
    JVM_PURE_VIRTUAL_3_PARAM(field, offset, visit_value);
  }
  virtual void do_ushort(VisitorField* field, int offset, bool visit_value) {
    // pure-virtual
    //SHOULD_NOT_REACH_HERE(); (void)field; (void)offset; (void)visit_value;
    JVM_PURE_VIRTUAL_3_PARAM(field, offset, visit_value);
  }
  virtual void do_uint(VisitorField* field, int offset, bool visit_value) {
    // pure-virtual
    //SHOULD_NOT_REACH_HERE(); (void)field; (void)offset; (void)visit_value;
    JVM_PURE_VIRTUAL_3_PARAM(field, offset, visit_value);
  }
  virtual void do_ulong(VisitorField* field, int offset, bool visit_value) {
    // pure-virtual
    //SHOULD_NOT_REACH_HERE(); (void)field; (void)offset; (void)visit_value;
    JVM_PURE_VIRTUAL_3_PARAM(field, offset, visit_value);
  }

  // Called after visiting an object
  virtual void epilogue() 
    JVM_PURE_VIRTUAL;

};


class SymbolField : public VisitorField {
 public:
  SymbolField(Symbol* name, bool in_vm, const char* comment = "")
      : VisitorField(in_vm, comment)
  {
    _name = name;
    _renamed = false;
  }
  virtual bool is_named_field() const { return true; }
  virtual void print_on(Stream* st);
  void set_renamed(bool value) {
    _renamed = value;
  }
 private:
  Symbol* _name;
  bool _renamed;
};
#endif

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

class OopPrinter : public OopVisitor {
 public:
  OopPrinter(Stream* st) {
    _st = st;
    _last_offset = -1;
  }
  virtual void prologue();
  virtual void do_oop(VisitorField* field, int offset, bool visit_value);
  virtual void do_byte(VisitorField* field, int offset, bool visit_value);
  virtual void do_char(VisitorField* field, int offset, bool visit_value);
  virtual void do_bool(VisitorField* field, int offset, bool visit_value);
  virtual void do_short(VisitorField* field, int offset, bool visit_value);
  virtual void do_int(VisitorField* field, int offset, bool visit_value);
  virtual void do_long(VisitorField* field, int offset, bool visit_value);
  virtual void do_float(VisitorField* field, int offset, bool visit_value);
  virtual void do_double(VisitorField* field, int offset, bool visit_value);
  virtual void do_ubyte(VisitorField* field, int offset, bool visit_value);
  virtual void do_ushort(VisitorField* field, int offset, bool visit_value);
  virtual void do_uint(VisitorField* field, int offset, bool visit_value);
  virtual void do_ulong(VisitorField* field, int offset, bool visit_value);

  virtual void do_comment(const char* comment);

  virtual void epilogue();
 private:
  Stream* _st;
  int     _last_offset;
  void    save_offset(int offset);
  Stream* st() const { return _st; }
};

#endif
