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

// Compiler context stores suspended compiler state

#if ENABLE_COMPILER
#define GENERIC_COMPILER_CONTEXT_FIELDS_DO(template)\
  template( CompilationQueueElement*, compilation_queue              )\
  template( CompilationQueueElement*, current_element                )\
  template( EntryTableType*,          entry_table                    )\
  template( CompilerByteArray*,       entry_counts_table             )\
  template( CompilerByteArray*,       bci_flags_table                )\
  template( int,                      bci                            )\
  template( int,                      saved_num_stack_lock_words     )\
  template( int,                      local_base                     )\
  template( bool,                     in_loop                        )\
  template( bool,                     has_loops                      )

#if ENABLE_INLINE
#define INLINER_COMPILER_CONTEXT_FIELDS_DO(template)  \
  template( int, inline_return_label_encoding )
#else
#define INLINER_COMPILER_CONTEXT_FIELDS_DO(template)
#endif

#if ENABLE_CODE_OPTIMIZER && ENABLE_NPCE
#define SCHEDULER_COMPILER_CONTEXT_FIELDS_DO(template) \
  template( CompilerIntArray*,  null_point_exception_ins_table       )\
  template( int,                codes_can_throw_null_point_exception )\
  template( int,                null_point_record_counter            )
#else
#define SCHEDULER_COMPILER_CONTEXT_FIELDS_DO(template) 
#endif

#define COMPILER_CONTEXT_FIELDS_DO(template) \
        GENERIC_COMPILER_CONTEXT_FIELDS_DO(template) \
        INLINER_COMPILER_CONTEXT_FIELDS_DO(template) \
        SCHEDULER_COMPILER_CONTEXT_FIELDS_DO(template) 

class CompilerContext {
public:
  typedef EntryArray EntryTableType;

  #define DEFINE_FIELD( type, name ) \
    type _##name;             \
    type name         ( void ) const { return _##name;  }\
    void set_##name   ( type val )   { _##name = val;   }\
    void clear_##name ( void )       { set_##name( 0 ); }
  COMPILER_CONTEXT_FIELDS_DO(DEFINE_FIELD)
  #undef DEFINE_FIELD

  void cleanup( void ) { jvm_memset( this, 0, sizeof *this ); }
};
#endif
