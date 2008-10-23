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

#define COMPILER_STATIC_FIELDS_DO( def )            \
  def( VirtualStackFrame*, frame )                  \
  def( VirtualStackFrame*, conforming_frame )       \
  def( VirtualStackFrame*, cached_preserved_frame ) \
  def( CompilationQueueElement*, compilation_queue_pool )

class CompilerState: public CodeGenerator {
 public:
  typedef ThrowExceptionStub::RuntimeException RuntimeException;
  enum {
    number_of_runtime_exceptions =
      ThrowExceptionStub::number_of_runtime_exceptions
  };
 private:
  MethodDesc* _root_method;

  #define DECLARE_FIELD(type, name) type _##name;
  COMPILER_STATIC_FIELDS_DO(DECLARE_FIELD)
  #undef DECLARE_FIELD

  ThrowExceptionStub* _rte_handlers[number_of_runtime_exceptions];
  CompilerContext     _suspended_compiler_context;

  void initialize( OopDesc* compiled_method ) {
    CodeGenerator::initialize( compiled_method );

    #define INIT_FIELD(type, name) NOT_PRODUCT( _##name = NULL; )
    COMPILER_STATIC_FIELDS_DO(INIT_FIELD)
    #undef INIT_FIELD

    NOT_PRODUCT( clear_root_method(); )
    NOT_PRODUCT( jvm_memset( &_rte_handlers, 0, sizeof _rte_handlers ); )
    NOT_PRODUCT( _suspended_compiler_context.cleanup(); )
  }

 public:
  static CompilerState* allocate( const int size JVM_TRAPS ) {
    OopDesc* compiled_method = Universe::new_compiled_method(size JVM_NO_CHECK);
    if( compiled_method ) {
#ifdef PRODUCT
      CompilerState* state = COMPILER_OBJECT_ALLOCATE( CompilerState );
      if( state )
#else
      static CompilerState _state;
      CompilerState* state = &_state;
#endif
      {
        _compiler_state = state;
        state->initialize( compiled_method );
        return state;
      }
    }
    return NULL;
  }
  static void terminate( void ) {
    _compiler_state = NULL;
  }
  CodeGenerator* code_generator( void ) { return this; }

  Method* root_method    ( void ) const       { return (Method*) &_root_method; }
  void clear_root_method ( void )             { _root_method = NULL;            }
  void set_root_method   ( OopDesc* m )       { _root_method = (MethodDesc*) m; }
  void set_root_method   ( const Method* m )  { set_root_method( m->obj() );    }

  CompilerContext* suspended_compiler_context( void ) {
    return &_suspended_compiler_context;
  }
  const CompilerContext* suspended_compiler_context( void ) const {
    return &_suspended_compiler_context;
  }

  #define DEFINE_ACCESSOR(type, name) \
    type name       ( void ) const    { return _##name;    } \
    void set_##name ( type name )     { _##name = name;    }
  COMPILER_STATIC_FIELDS_DO(DEFINE_ACCESSOR)
  #undef DEFINE_ACCESSOR

  ThrowExceptionStub* rte_handler(const RuntimeException rte) const {
    return _rte_handlers[rte];
  }
  void set_rte_handler(const RuntimeException rte, ThrowExceptionStub* value) {
    _rte_handlers[rte] = value;
  }

  VirtualStackFrame* get_cached_preserved_frame( void ) {
    VirtualStackFrame* p = cached_preserved_frame();
    set_cached_preserved_frame( NULL );
    return p;
  }

  void remove( CompilationQueueElement* element ) {
    set_compilation_queue_pool( element->next() );
  }
  void append( CompilationQueueElement* element ) {
    element->set_next( compilation_queue_pool() );
    set_compilation_queue_pool( element );
  }

#if USE_DEBUG_PRINTING
  void print_on(Stream* st) const {
    #define PRINT_FIELD( type, name ) \
      st->print_cr("%-30s = 0x%08x", STR(name), _##name);
    COMPILER_STATIC_FIELDS_DO(PRINT_FIELD)
    #undef PRINT_FIELD
  }
#endif

  void oops_do( void do_oop(OopDesc**) ) {
    if( this ) {
      CodeGenerator::oops_do( do_oop );
      do_oop( (OopDesc**) &_root_method );
    }
  }
};

inline VirtualStackFrame* CodeGenerator::frame ( void ) {
  return _compiler_state->frame();
}

inline VirtualStackFrame* GenericAddress::frame ( void ) {
  return _compiler_state->frame();
}

inline void CompilationQueueElement::free( void ) {
  if( !( type() == CompilationQueueElement::throw_exception_stub &&
         ThrowExceptionStub::cast(this)->is_persistent() ) ) {
    _compiler_state->append( this );
  }
}
#endif
