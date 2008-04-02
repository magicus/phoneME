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
#include "incls/_Location.cpp.incl"

#if ENABLE_COMPILER

inline CodeGenerator* RawLocation::code_generator( void ) {
  return _compiler_code_generator;
}

inline RawLocation::Actions 
RawLocation::merge_actions( const RawLocation* other ) const {
  #define ACTIONS_POS( src, dst )       ( ((src)*3 + (dst)) * 3 )
  GUARANTEE( ACTIONS_POS( changed + 1, 0 ) <= 32, "sanity" );

  #define ACTIONS( src, dst, actions )  ((actions) << ACTIONS_POS(src, dst)) |
  enum { actions =
    ACTIONS( flushed, flushed,  0                   )
    ACTIONS( flushed, cached,   LOC_LOAD            )
    ACTIONS( flushed, changed,  LOC_LOAD            )
    ACTIONS( cached,  flushed,  0                   )
    ACTIONS( cached,  cached,   REG_STORE           )
    ACTIONS( cached,  changed,  REG_STORE           )
    ACTIONS( changed, flushed,  LOC_STORE           )
    ACTIONS( changed, cached,   REG_STORE|LOC_STORE )
    ACTIONS( changed, changed,  REG_STORE           )
  0 };
  #undef ACTIONS

  // compute the merge action
  return ( actions >> ACTIONS_POS(status(), other->status()) ) & ALL_ACTIONS;
  #undef ACTIONS_POS
}

inline bool
RawLocation::is_used_in(const VirtualStackFrame* frame, const Value& value) {
  GUARANTEE(value.in_register(), "value must be in register");
  return frame->is_mapping_something(value.lo_register()) ||
    ( value.use_two_registers() &&
         frame->is_mapping_something(value.hi_register()) );
}

bool RawLocation::is_register_identical_to(int my_index, RawLocation* other,
                                           int other_index) {
  GUARANTEE(in_register(), "must be in register");

  // if the the other location isn't in a register we're not identical
  if (!other->in_register()) {
    return false;
  }

  // make sure the types match
  GUARANTEE(stack_type() == other->stack_type(), "types must match");

  const Value this_value (this,  my_index);
  const Value other_value(other, other_index);

  return this_value.lo_register() == other_value.lo_register() &&
    (!this_value.use_two_registers() ||
         this_value.hi_register() == other_value.hi_register() );
}

RawLocation::Actions 
RawLocation::do_conform_to(int my_index, RawLocation* other, int other_index, 
                           RawLocation::Actions allowed_actions) {
  // make sure object registers and locations have object values 
  if (other->stack_type() == T_OBJECT && stack_type() != T_OBJECT) {
    // Conformance code makes it so that this is no longer necessary.
    SHOULD_NOT_REACH_HERE();

#if NOT_CURRENTLY_USED
    // if we clear an cached object location there's no need to clear
    // any registers, since the register cache (due to type conflicts) is 
    // guaranteed never to be used again
    if (other.is_flushed() || other.is_cached()) {
      code_generator()->clear_object_location(index());
    } else {
      GUARANTEE(other.is_changed(), "only case left");
      Value other_value(other.type());
      other.read_value(other_value);
      Oop::Raw null_obj;      
      code_generator()->move(other_value, &null_obj);
    }
    return;
#endif
  }
  // compute the merge action
  const Actions required_actions = merge_actions(other);

  const Actions actions = required_actions & allowed_actions;

  // handle loads/stores from/to locations
  if (actions & LOC_LOAD)  {
    other->update_cache(other_index);
  }
  if (actions & LOC_STORE) {
    write_changes(my_index);
  }
  
  // handle register stores
  if (actions & REG_STORE && other->in_register()) {
    // declare & read values for both source and destination
    const Value this_value (this,  my_index   );
    const Value other_value(other, other_index);

    // do the register store 
    if (!other->is_register_identical_to(my_index, this, other_index)) {
      code_generator()->move(other_value, this_value);
    }
  }

  return required_actions & ~allowed_actions;
}

bool RawLocation::has_register_conflict_with(int my_index,
                                             VirtualStackFrame *other_frame,
                                             RawLocation* other_loc,
                                             int other_index) {
  // locations not in registers cannot cause register conflicts
  if (!in_register()) {
    return false;
  }

  // compute the merge action
  const int actions = merge_actions(other_loc);

  if (actions & (REG_STORE | LOC_STORE)) {
    const Value this_value(this, my_index);
    if( is_used_in(other_frame, this_value) &&
        ( (actions & LOC_STORE) ||
          !is_register_identical_to(my_index, other_loc, other_index) ) ) {
      return true;
    }
  }  

  // no conflict
  return false;
}

void RawLocation::flush(int index) {
  // write any changes to memory
  write_changes(index);
  // mark the location as being flushed
  mark_as_flushed();
}

void RawLocation::write_changes(int index) {
  // write any changes to memory
  if (is_changed()) {
    // get the value for this location
    Value value(this, index);
    // mark the location as being cached (do this before store_local to avoid
    // recursion when handling x86 floats). 
    mark_as_cached();
    // write the value into memory
    code_generator()->store_to_location(value, index);
  }
}

void RawLocation::update_cache(int index) {
  // this forces an update of the cache from memory
  // even if the location has been changed
  if (!is_flushed()) {
    // get the value for this location
    Value value(this, index);
    // read the value from memory
    if (value.in_register()) { 
      code_generator()->load_from_location(value, index);
    }
  }
}

void RawLocation::change_register(Assembler::Register dst,
                                  Assembler::Register src) {
  GUARANTEE(!is_flushed() && in_register(), "Sanity");
  GUARANTEE(this->value() == src || 
            (is_two_word() && next_location()->value() == src), 
            "Only called if necessary");

  if (this->value() == src) { 
    this->set_value(dst);
  }
  if (is_two_word()) { 
    RawLocation *next = next_location();
    if (next->value() == src) { 
      next->set_value(dst);
    }
  }
}

void Value::initialize( RawLocation* loc, const int index ) {
  initialize( loc->type() );
  loc->read_value( *this, index );
}

void Value::initialize( Location* loc ) {
  initialize( loc->raw_location(), loc->index() );
}

#endif
