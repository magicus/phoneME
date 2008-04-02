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

#if ENABLE_COMPILER

class BinaryAssemblerCommon: public Macros {
 private:
  RelocationStream    _relocation;
  OopDesc*            _compiled_method;
#if ENABLE_ISOLATES
  // The ID of the task that started this compilation. Compilation uses
  // information that are specific to a task's context -- for example, 
  // class_ids. Hence, a compilation must be resumed under the correct 
  // task context.
  int                 _task_id;
#endif
 protected:
  jint                _code_offset;
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
#endif

  void initialize( OopDesc* compiled_method ) {
    _compiled_method = compiled_method;
    _relocation.initialize( this->compiled_method() );
#if ENABLE_ISOLATES
    _task_id = Task::current_id();
#endif
    // In PRODUCT CompilerObject is filled with zeroes
    NOT_PRODUCT( _code_offset                   = 0;    )
#if USE_LITERAL_POOL
    NOT_PRODUCT( _first_literal                 = NULL; )
    NOT_PRODUCT( _first_unbound_literal         = NULL; )
    NOT_PRODUCT( _last_literal                  = NULL; )
#if ENABLE_THUMB_COMPILER
    NOT_PRODUCT( _first_unbound_branch_literal  = NULL; )
    NOT_PRODUCT( _last_unbound_branch_literal   = NULL; )
    NOT_PRODUCT( _unbound_branch_literal_count  = 0;    )
#endif
    zero_literal_count();
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
    emit_relocation_ushort( param );
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
     emit_relocation_ushort(0);
  }

#if ENABLE_PAGE_PROTECTION
  void emit_vsf(const VirtualStackFrame* frame);
#endif

#if ENABLE_CODE_PATCHING
  void emit_checkpoint_info_record(const int code_offset, 
                                   const unsigned int original_instruction,
                                   const int stub_position);
#endif

 public:
  void emit_osr_entry(const jint bci) { 
    emit_relocation(Relocation::osr_stub_type, _code_offset, bci); 
  }
  BinaryAssembler* assembler( void ) { return (BinaryAssembler*)this; }

  CompiledMethod* compiled_method( void ) const {
    return (CompiledMethod*) &_compiled_method;
  }

#if ENABLE_ISOLATES
  int task_id( void ) const { return _task_id; }
#endif

  void oops_do( void do_oop(OopDesc**) ) {
    if( this ) {
      do_oop( &_compiled_method );
#if USE_LITERAL_POOL
      for( LiteralPoolElement* p = _first_literal; p != NULL; p = p->next() ) {
        p->oops_do( do_oop );
      }
#endif
    }
  }
        
  jint code_size( void ) const { return _code_offset; }
  jint code_end_offset( void ) const { return offset_at( code_size() ); }

  // Returns the size of the relocation data in bytes.
  jint relocation_size( void ) const {
    return compiled_method()->end_offset()
           - current_relocation_offset()
           - sizeof(jushort);
  }

  address addr_at(const jint pos) const {
    return (address)(compiled_method()->field_base(offset_at(pos)));
  }

#define BINARY_CODE_ACCESSOR_DO(template)\
  template(byte )\
  template(short)\
  template(int  )

#define DEFINE_CODE_ACCESSOR(type)\
  jint  type##_at(const int position) const  {                      \
    return compiled_method()->type##_field(offset_at(position));    \
  }                                                                 \
  void  type##_at_put(const int position, const j##type value) const {\
    compiled_method()->type##_field_put(offset_at(position), value);\
  }                                                                 \
  void emit_code_##type (const unsigned value);
BINARY_CODE_ACCESSOR_DO(DEFINE_CODE_ACCESSOR)
#undef DEFINE_CODE_ACCESSOR

#if USE_LITERAL_POOL
  jint unbound_literal_count( void ) const { return _unbound_literal_count; }

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
  void emit_relocation_ushort(const unsigned value);
  void emit_dummy (const jint code_offset);
  void emit_relocation_oop( void );

  static int offset_at(const int position) { 
    return position + CompiledMethod::base_offset();
  }

  // check if there's room for a few extra bytes in the compiled method
  bool has_room_for(const int code_offset, const int bytes) const {
    // Using 8 instead of 0 as defensive programming
    // The shrink operation at the end of compilation will regain the extra
    // space 
    // The extra space ensures that there is always a sentinel at the end of
    // the relocation data and that there is always a null oop that the last
    // relocation entry can address. 
    return free_space(code_offset) >= bytes + /* slop */8;
  }
  bool has_room_for(const int bytes) const {
    return has_room_for(_code_offset, bytes);
  }

  // Returns the remaining free space in the compiled method.
  jint free_space( const int code_offset ) const {
    return (current_relocation_offset() + sizeof(jushort)) - 
            offset_at(code_offset);
  }
  jint free_space( void ) const { return free_space( _code_offset ); }
};

#endif
