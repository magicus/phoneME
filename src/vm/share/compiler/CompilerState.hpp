/*
 *   
 *
 * Portions Copyright  2000-2007 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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

class CompilerState  {
public:
  #define FIELD( type, name ) \
    type _##name;             \
    inline type name ( void ) const { return _##name; } \
    inline void set_##name ( type val ) { _##name = val; }

#if USE_LITERAL_POOL
  FIELD( LiteralPoolElement*, first_literal             )
  FIELD( LiteralPoolElement*, first_unbound_literal     )
  FIELD( LiteralPoolElement*, last_literal              )
#endif

#if ENABLE_THUMB_COMPILER
  FIELD( LiteralPoolElement*, first_unbound_branch_literal      )
  FIELD( LiteralPoolElement*, last_unbound_branch_literal       )
#endif

  FIELD( int,  code_size                 )
  FIELD( int,  current_relocation_offset )
  FIELD( int,  current_code_offset       )
  FIELD( int,  current_oop_relocation_offset )
  FIELD( int,  current_oop_code_offset       )
#if ENABLE_ISOLATES
  // The ID of the task that started this compilation. Compilation uses
  // information that are specific to a task's context -- for example, 
  // class_ids. Hence, a compilation must be resumed under the correct 
  // task context.
  FIELD( int, task_id                   )
#endif

#if USE_LITERAL_POOL
  FIELD( int, unbound_literal_count                     )
  FIELD( int, code_offset_to_force_literals             )
  FIELD( int, code_offset_to_desperately_force_literals )
#endif

#if ENABLE_THUMB_COMPILER
  FIELD( int, unbound_branch_literal_count              )
#endif

#undef FIELD

  bool _valid;

  bool valid ( void ) const { return _valid; }
  void allocate ( void ) { _valid = true;   }
  void dispose  ( void ) { _valid = false;  }
};

#endif
