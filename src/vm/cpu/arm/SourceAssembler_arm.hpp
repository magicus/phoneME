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

#if !ENABLE_THUMB_COMPILER

class Segment;
class SourceAssembler: public Macros {
#ifndef PRODUCT
 protected:
  Stream*                       _stream;
  Disassembler _disasm;

  // disassembler
  virtual void emit(int instr);
  virtual void ldr_big_integer(Register rd, int imm32, Condition cond = al);
  char _eol_comment[1024];

  // helper routines for symbolic output
  static const char* cond_name(Condition cond) {
    return Disassembler::cond_name(cond);
  }

  static const char* reg_name(Register reg) {
    return Disassembler::reg_name(reg);
  }

  // If we're generating glue code, we cannot use b or bl instructions
  static void set_in_glue_code(bool state) {
    _in_glue_code = state;
  }

  static bool in_glue_code() {
    return _in_glue_code;
  }
 public:
  class Label {
   private:
    static int _next_id;

    enum { anonymous_unused, anonymous_used, anonymous_bound,
           undefined, imported, exported, internal } _state;

    bool is_anonymous() const { return _state <= anonymous_bound; }

    union {
      int         _id;
      const char* _symbol;
    };

    friend class SourceAssembler;
    friend class Literal;

    void import  (Stream* s);
    void make_local(Stream* s);   // should be the default!
    void global  (Stream* s);
    void bind    (Stream* s, bool is_global = false);
    void print_on(Stream* s) const;

   public:
    Label(const char* symbol = NULL);
    ~Label();

   
    int id() const                               { return _id; }
    const char* symbol() const                   { return _symbol; }
  };

  class Literal {
   public:
    enum Kind { undefined, integer, string, label };

   private:
    static int _next_id;

    int  _id;
    Kind _kind;
    union {
      int         _ivalue;
      const char* _svalue;
    };

   public:
    // creation
    Literal()              :                  _kind(undefined)                                       {}
    Literal(int x)         : _id(_next_id++), _kind(integer  ), _ivalue(x     )                      {}
    Literal(const char* s) : _id(_next_id++), _kind(string   ),                  _svalue(s         ) {}
    Literal(Label& L)      : _id(_next_id++), _kind(label    ) {
      _ivalue = L.id();
      _svalue = L.symbol();
    }

    // accessors
    int id() const                     { return _id; }
    Kind kind() const                  { return _kind; }
    int ivalue() const                 { GUARANTEE(kind() == integer, "illegal access"); return _ivalue; }
    const char* svalue() const         { GUARANTEE(kind() == string , "illegal access"); return _svalue; }

    // printing
    void print_label_on(SourceAssembler* sasm);
    void print_value_on(SourceAssembler* sasm);
  };

  class LiteralBuffer {
   private:
    enum { max_length = 32 };
    Literal _data[max_length];
    int _length;

   public:
    // creation/destruction
    LiteralBuffer() : _length(0)       {}
    ~LiteralBuffer()                   { 
        GUARANTEE(_length == 0, "buffer must be empty"); 
    }

    // manipulation
    void add(Literal& lit);
    void empty(SourceAssembler* sasm);
  };

  // creation & start/stop
  SourceAssembler(Stream* stream) :
    _stream(stream), _disasm(stream), _segment(NULL), _segment_type(no_segment)
  {
    _eol_comment[0] = '\0';
    _use_offset_comments = false;
  }
  void start();
  void stop();

  // segment control
  enum SegmentType {
    no_segment,
    code_segment,
    data_segment,
    gp_segment,     /* For global pointer info */
    bss_segment     /* Can only contains zeros */
  };

  void beg_segment(Segment *segment, SegmentType segment_type);
  void end_segment();

 private:
  // Note: The InterpreterGenerator needs to be a friend only because
  //       of a misdesign that requires the InterpreterGenerator to
  //       mess with the SourceAssembler's state directly. This needs
  //       to be fixed at some point.
  friend class InterpreterGenerator;
  LiteralBuffer _literals;
  Segment*      _segment;
  SegmentType   _segment_type;
  bool          _use_offset_comments;
  int           _current_commented_offset;

  friend class Segment;

  void emit_comment_and_cr();

 public:
  // accessors
  Stream* stream() const {
    return _stream;
  } 
  Disassembler& disassembler() {
    return _disasm;
  }
  // comments
  virtual void comment        (const char* fmt, ...);
  void eol_comment    (const char* fmt, ...);
  void comment_section(const char* fmt, ...);
  void set_use_offset_comments(bool value) {
    _use_offset_comments = value;
  }
  void set_current_commented_offset(int value) {
    _current_commented_offset = value;
  }

 public:

  // directives
  void align(int offset);

  // support for 32bit immediates
  void ldr_string(Register r, const char* string, Condition cond = al);
  void ldr_label (Register r, Label& L          , Condition cond = al);
  void ldr_label (Register r, const char*name, 
                  bool import_label = true, Condition cond = al) {
      Label L(name); 
      if (import_label) import(L); 
      ldr_label(r, L, cond);
  }
  void ldr_label_offset (Register r, const char*name, int offset,
                  bool import_label = true, Condition cond = al) {
      Label L(name); 
      char buffer[100];
      jvm_sprintf(buffer, "%s + %d", name, offset);
      Label M(buffer);
      if (import_label) import(L); 
      ldr_label(r, M, cond);
  }

  void ldr_nearby_label(Register r, Label& L, Condition cond = al);
  void ldr_from   (Register r, Label& L, int offset = 0, Condition cond = al);
  void ldr_gp_base(Register r, Condition cond = al) { 
    ldr_label(r, "gp_base_label", false, cond);  
  }

  // support for labels
  void import(Label& L)                { L.import(stream()); }
    void import(const char* name)      { Label L(name); import(L); }
  void global(Label& L)                { L.global(stream()); }
  void make_local(Label& L)            { L.make_local(stream()); }
  void bind  (Label& L)                { L.bind(stream()); }
  void bind_global(Label& L);
  void bind  (const char *name)        { Label L(name); bind(L); }
  void bind_global(const char *name)   { Label L(name); bind_global(L); }
                                       
  void bind_local(Label& L)            { make_local(L); bind(L); }
  void bind_local(const char *name)    { Label L(name); bind_local(L);}
  void bind_rom_linkable(const char *name, bool generate_fixed=false);

  void b (const Label& L, Condition cond = al);
  void bl(const Label& L, Condition cond = al);

  void b (const char *name, Condition cond = al) { Label L(name); b(L, cond); }
  void bl(const char *name, Condition cond = al) { Label L(name); bl(L, cond); }

  void jmpx(Register r, Condition cond = al) { 
    if (GenerateThumbInterwork) {
      bx(r, cond);
    } else { 
      mov(pc, reg(r), cond);
    }
  }

  // data definition
  void define_byte(int    x);
  void define_word(int    x);
  void define_long(int    x);
  void define_long(const Label& L);

  void define_bytes(const char* s, bool word_align = true);
  void define_zeros(int size); // zeroed area of (size) bytes

  void define_call_info();


  Register low_from_pair(Register a, Register b) { 
    return JavaStackDirection < 0 ? a : b;
  }
  Register high_from_pair(Register a, Register b) { 
    return JavaStackDirection < 0 ? b : a;
  }
  // This is the inverse of the above
  Register pair1_from_low_high(Register a, Register b) {
    return JavaStackDirection < 0 ? a : b;
  }
  Register pair2_from_low_high(Register a, Register b) {
    return JavaStackDirection < 0 ? b : a;
 }


  Register msw_from_long_pair(Register a, Register b) {
    return MSW_FIRST_FOR_LONG ? low_from_pair(a,b) : high_from_pair(a,b);
  }
  Register lsw_from_long_pair(Register a, Register b) {
    return MSW_FIRST_FOR_LONG ? high_from_pair(a,b) : low_from_pair(a,b);
  }
  Register msw_from_double_pair(Register a, Register b) {
    return MSW_FIRST_FOR_DOUBLE ? low_from_pair(a,b) : high_from_pair(a,b);
  }
  Register lsw_from_double_pair(Register a, Register b) {
    return MSW_FIRST_FOR_DOUBLE ? high_from_pair(a,b) : low_from_pair(a,b);
  }

  int  find_gp_offset(const char *name);
  void ldr_using_gp(Register reg, const char *name, Condition cond = al);
  void str_using_gp(Register reg, const char *name, Condition cond = al);
  void add_using_gp(Register reg, const char *name, Condition cond = al);

#define DEFINE_GP_FOR_SOURCE(name, size) \
  virtual void get_ ## name(Register reg, Condition cond = al) {   \
     ldr_using_gp(reg, #name, cond);                               \
  }                                                                \
  virtual void set_ ## name(Register reg, Condition cond = al) {   \
     str_using_gp(reg, #name, cond);                               \
  }                                                                \
  virtual void get_ ## name ## _addr(Register reg, Condition cond = al) {  \
     add_using_gp(reg, #name, cond);                               \
  }                                                                \

  GP_GLOBAL_SYMBOLS_DO(_xx_pointers, DEFINE_GP_FOR_SOURCE)

private:
  static bool _in_glue_code;
#endif // PRODUCT
};

// Segment control

#ifndef PRODUCT

class Segment: public StackObj {
 private:
  SourceAssembler* _sasm;
  const char *_title;
  bool _in_global;
 public:
  Segment(SourceAssembler* sasm, SourceAssembler::SegmentType segment, 
          const char* title = NULL);
  ~Segment();
  void set_in_global() {
    _in_global = true;
  }
  void flush_global();
};

class FunctionDefinition: public StackObj { 
  private:
    SourceAssembler* _sasm;    
    const char*      _name; 
    
    static int       _count;
  public:
    enum { 
        Global, Local, ROM
    };

    FunctionDefinition(SourceAssembler* sasm, const char *name, 
                       int type = Global);
    ~FunctionDefinition();

};

#endif // PRODUCT

#endif /*#if ENABLE_THUMB_COMPILER*/
