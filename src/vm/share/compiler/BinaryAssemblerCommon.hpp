/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#if ENABLE_COMPILER

class BinaryAssemblerCommon: public Macros {
 private:
  RelocationStream _relocation;
 protected:
  jint             _code_offset;
#if USE_LITERAL_POOL
  LiteralPoolElement* _first_literal;
  LiteralPoolElement* _first_unbound_literal;
  LiteralPoolElement* _last_literal;
#if ENABLE_THUMB_COMPILER
  LiteralPoolElement* _first_unbound_branch_literal;
  LiteralPoolElement* _last_unbound_branch_literal;
#endif

  int                 _unbound_literal_count;
  int                 _code_offset_to_force_literals;
                      // and to boldly split infinitives
  int                 _code_offset_to_desperately_force_literals;  

#if ENABLE_THUMB_COMPILER
  int                 _unbound_branch_literal_count;
#endif
  friend class Compiler; // for oops_do only
#endif

  BinaryAssemblerCommon(CompiledMethod* compiled_method)
    : _relocation( compiled_method ) {
    _code_offset     = 0;
#if USE_LITERAL_POOL
    _first_literal         = NULL;
    _first_unbound_literal = NULL;
    _last_literal          = NULL;
#if ENABLE_THUMB_COMPILER
    _first_unbound_branch_literal = NULL;
    _last_unbound_branch_literal  = NULL;
    _unbound_branch_literal_count = 0;
#endif

    zero_literal_count();
#endif
  }
  BinaryAssemblerCommon(const CompilerState* compiler_state,
                        CompiledMethod* compiled_method)
    : _relocation( compiler_state, compiled_method ) {
    _code_offset     = compiler_state->code_size();
#if USE_LITERAL_POOL
    _first_literal         = compiler_state->first_literal();
    _first_unbound_literal = compiler_state->first_unbound_literal();
    _last_literal          = compiler_state->last_literal();
#if ENABLE_THUMB_COMPILER
    _first_unbound_branch_literal = compiler_state->first_unbound_branch_literal();
    _last_unbound_branch_literal  = compiler_state->last_unbound_branch_literal();
    _unbound_branch_literal_count = compiler_state->unbound_branch_literal_count();
#endif

    _unbound_literal_count                     = compiler_state->unbound_literal_count();
    _code_offset_to_force_literals             = compiler_state->code_offset_to_force_literals();
    _code_offset_to_desperately_force_literals = compiler_state->code_offset_to_desperately_force_literals();
#endif
  }

  void emit_relocation(const Relocation::Kind kind) {
    emit_relocation(kind, _code_offset);
  }        
  void emit_relocation(const Relocation::Kind kind, const jint code_offset);
  void emit_relocation(const Relocation::Kind kind, const jint code_offset,
                                                    const jint param) {
    GUARANTEE(Relocation::has_param(kind), "Sanity");
    emit_relocation(kind, code_offset);
    emit_ushort((jushort) param);
  }
  void emit_oop( const OopDesc* obj ) {
    if( ObjectHeap::contains_moveable( obj ) ) {
      // GC needs to know about these
      emit_relocation_oop();
    } else { 
#ifndef PRODUCT
      // Let the disassembler know that this is an oop
      emit_relocation(Relocation::rom_oop_type, _code_offset);
#endif
    }
  }
  void emit_sentinel( void ) {
     emit_ushort(0);
  }
  void emit_vsf(const VirtualStackFrame* frame);

#if ENABLE_CODE_PATCHING
  void emit_checkpoint_info_record(const int code_offset, 
                                   const unsigned int original_instruction,
                                   const int stub_position);
#endif

 public:
  void emit_osr_entry(const jint bci) { 
    emit_relocation(Relocation::osr_stub_type, _code_offset, bci); 
  }

  void save_state( CompilerState* compiler_state ) const {
    compiler_state->set_code_size(_code_offset);
    _relocation.save_state(compiler_state);
#if USE_LITERAL_POOL
    compiler_state->set_first_literal        ( _first_literal );
    compiler_state->set_first_unbound_literal( _first_unbound_literal );
    compiler_state->set_last_literal         ( _last_literal );

#if ENABLE_THUMB_COMPILER
    compiler_state->set_first_unbound_branch_literal(_first_unbound_branch_literal);
    compiler_state->set_last_unbound_branch_literal(_last_unbound_branch_literal);
    compiler_state->set_unbound_branch_literal_count(_unbound_branch_literal_count);
#endif

    compiler_state->set_unbound_literal_count( _unbound_literal_count );
    compiler_state->set_code_offset_to_force_literals( _code_offset_to_force_literals );
    compiler_state->set_code_offset_to_desperately_force_literals( _code_offset_to_desperately_force_literals );
#endif
  }

  BinaryAssembler* assembler( void ) { return (BinaryAssembler*)this; }
        
  CompiledMethod* compiled_method( void ) const {
    return _relocation.compiled_method();
  }

  jint code_size      ( void ) const { return _code_offset; }
  jint code_end_offset( void ) const { return offset_at(code_size()); }

  // Returns the size of the relocation data in bytes.
  jint relocation_size( void ) const {
    return compiled_method()->end_offset()
           - current_relocation_offset()
           - sizeof(jushort);
  }

#if USE_LITERAL_POOL
  jint unbound_literal_count( void ) const { return _unbound_literal_count; }
#endif

  int   offset_at(const int position) const  { 
    return position + CompiledMethod::base_offset();
  }
  address addr_at(const jint pos) const {
    return (address)(compiled_method()->field_base(offset_at(pos)));
  }

  jint  byte_at(const int position) const  {
    return compiled_method()->byte_field(offset_at(position));
  }
  jint  short_at(const int position) const  {
    return compiled_method()->short_field(offset_at(position));
  }
  jint  int_at(const int position) const  {
    return compiled_method()->int_field(offset_at(position));
  }

  void  byte_at_put(const int position, const jbyte value)  {
    compiled_method()->byte_field_put(offset_at(position), value);
  }
  void  short_at_put(const int position, const jshort value) const {
    compiled_method()->short_field_put(offset_at(position), value);
  }
  void  int_at_put(const int position, const jint value) const {
    compiled_method()->int_field_put(offset_at(position), value);
  }

#if USE_LITERAL_POOL
  void zero_literal_count( void ) { 
    _unbound_literal_count                     = 0;
    _code_offset_to_force_literals             = 0x7FFFFFFF; // never need to force
    _code_offset_to_desperately_force_literals = 0x7FFFFFFF;
  }

  // We should write out the literal pool at our first convenience
  bool need_to_force_literals( void ) const {
    return _code_offset >= _code_offset_to_force_literals;
  }

  // We should write out the literal pool very very soon, even if it
  // generates bad code
  bool desperately_need_to_force_literals(const int extra_bytes = 0) const {
    return _code_offset + extra_bytes >= _code_offset_to_desperately_force_literals;
  }
#endif

  // Returns the remaining free space in the compiled method.
  jint free_space( void ) const {
    return (_relocation.current_relocation_offset() + sizeof(jushort)) - 
            offset_at(code_size());
  }

  // check if there's room for a few extra bytes in the compiled method
  bool has_room_for(const int bytes) const { 
    // Using 8 instead of 0 as defensive programming
    // The shrink operation at the end of compilation will regain the extra
    // space 
    // The extra space ensures that there is always a sentinel at the end of
    // the relocation data and that there is always a null oop that the last
    // relocation entry can address. 
    return free_space() >= bytes + /* slop */8;
  }

  bool has_overflown_compiled_method( void ) const { 
    return !has_room_for(0); 
  }

  // If compiler_area is enabled, move the relocation data to higher
  // address to make room for more compiled code.
  void ensure_compiled_method_space( int delta = 0 );

#if !defined(PRODUCT) || USE_COMPILER_COMMENTS
  void comment(const char* fmt, ...);
#else
  void comment(const char* fmt, ...) { (void) fmt; }
#endif // !defined(PRODUCT) || USE_COMPILER_COMMENTS

 private:
  enum {
    type_width   = Relocation::type_width,
    offset_width = Relocation::offset_width
  };

  jint current_code_offset( void ) const {
    return _relocation._current_code_offset;
  }
  void set_current_code_offset( const jint code_offset ) {
    _relocation._current_code_offset = code_offset;
  }

  jint current_relocation_offset( void ) const {
    return _relocation._current_relocation_offset;
  }
  void set_current_relocation_offset( const jint relocation_offset ) {
    _relocation._current_relocation_offset = relocation_offset;
  }

  jint current_oop_relocation_offset( void ) const {
    return _relocation._current_oop_relocation_offset;
  }
  void set_current_oop_relocation_offset( const jint oop_relocation_offset ) {
    _relocation._current_oop_relocation_offset = oop_relocation_offset;
  }

  jint current_oop_code_offset( void ) const {
    return _relocation._current_oop_code_offset;
  }
  void set_current_oop_code_offset( const jint oop_code_offset ) {
    _relocation._current_oop_code_offset = oop_code_offset;
  }

  jint compute_embedded_offset(const jint code_offset);
  void emit_ushort(const jushort value );
  void emit_dummy (const jint code_offset);
  void emit_relocation_oop( void );
};

#endif
