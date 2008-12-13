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

# include "incls/_precompiled.incl"
# include "incls/_StringTable.cpp.incl"

inline void StringTable::insert(String* string) {
  const juint mask = juint(length()-1);
  juint index = string->hash() & mask;    

  AZZERT_ONLY(const unsigned start_index = index;)

  while (obj_at(index) != NULL) {
    index ++;
    index &= mask;
    // Do not rewrite as  index = (++index & mask);
    // ADS compiler generates incorrect code.
    GUARANTEE(index != start_index, "String table overflow");
  }

  obj_at_put(index, string);
}

void StringTable::expand(JVM_SINGLE_ARG_TRAPS) {
#ifdef AZZERT
  handle_uniqueness_verification();
#endif
  const int old_size = length();
  const int new_size = 2*old_size;
  if( Verbose ) {
    TTY_TRACE_CR(("Expanding string_table to %d entries", new_size));
  }

  // NOTE:
  // Don't do this: string_table() = Universe::new_obj_array(...);
  // If the allocation fails string_table will become NULL, and subsequent
  // access to string_table would cause error in VM!
  StringTable::Raw new_table = initialize( new_size JVM_CHECK );

  AllocationDisabler raw_pointers_used_in_this_block;
  for (int i = 0; i < old_size; i++) {
    String::Raw old_string(obj_at(i));
    if (old_string.not_null()) {
      new_table().insert(&old_string);
    }
  }
  set_obj(new_table);
  *current() = new_table;
#if ENABLE_ISOLATES
  Task::current()->set_string_table(new_table);
#endif
}

ReturnOop StringTable::interned_string_for(String *string JVM_TRAPS) {
  const juint hash = string->hash();

  // (1) Loop up in the ROM string table
  if (UseROM) {
    ReturnOop old_string = ROM::string_from_table(string, hash);
    if (old_string != NULL) {
      return old_string;
    }
  }

  // (2) Look up in the HEAP StringTable
  const juint mask = juint(length()-1);
  juint index = hash & mask;
  GUARANTEE(index == (hash % length()), "sanity");

  const unsigned start = index;
  do {
    String::Raw old_string = obj_at(index);
    if (old_string.not_null()) {
      if (old_string().matches(string)) {
        return old_string;
      }
    } else {
      obj_at_put(index, string);
      string->set_klass((Oop*)&_interned_string_near_addr);

      const unsigned max_count = (mask+1)*3/4;  // 75%
      if( Task::current()->incr_string_table_count() >= max_count ) {
        expand(JVM_SINGLE_ARG_CHECK_0);
      }
      return (ReturnOop)(*string);
    }
    index ++;
    index &= mask;
    // Do not rewrite as  while( (index = (++index & mask)) != start );
    // ADS compiler generates incorrect code.
  } while (index != start);

  Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW_0);
}
 
#ifndef PRODUCT
void StringTable::iterate(StringTableVisitor* visitor JVM_TRAPS) {
  UsingFastOops fast_oops;
  String::Fast string;
  for (int i = 0; i < length(); i++) {
    string = obj_at(i);
    if (string.not_null()) {
      visitor->do_string(&string JVM_CHECK);
    }
  }
}
#endif
