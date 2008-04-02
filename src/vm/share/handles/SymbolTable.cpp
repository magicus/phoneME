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
# include "incls/_SymbolTable.cpp.incl"

juint SymbolTable::hash(utf8 s, int length) {
  juint value = 0;
  utf8 end = s + length;
  while (s < end) {
    // Be careful with the casting of s (which is a signed char).
    // Compilers may handle the sign extensions differently!
    //
    // This must yield the same result on all platforms, because
    // the Romizer may not run on the target platform.
    juint chr = *((unsigned char*)s);
    value = 31 * value + chr;
    s++;
  }
  return value;  
}
juint SymbolTable::hash(Symbol *symbol) {
  return hash(symbol->utf8_data(), symbol->length());
}

ReturnOop
SymbolTable::symbol_for(TypeArray *byte_array, utf8 s, int len,
                        bool check_only JVM_TRAPS) {
  if (byte_array == NULL) {
    // If the source of the new symbol lives in the heap, you must
    // GC-protect it by passing the source in a byte_array
    GUARANTEE(!ObjectHeap::contains((OopDesc*)s), "must not be in heap");
  }

  if (unsigned(len) > 0xfff0) {
    // Symbol's string length is a unsigned 16-bit number
    Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW_0);
  }

  const juint hash_value = hash(s, len);

  if (UseROM) {
    // The ROM symbol table's layout is different than that of the
    // symbol table in heap. The main reason is to avoids holes in 
    // the table.
    //

    // **************************************************************
    // WARNING: the value of ROM::symbol_table_num_buckets() is 1 off
    // what you would think. Read the following carefully.
    // **************************************************************
    //
    // Layout of the ROM symbol table with <n> buckets:
    //
    // (a) The first <n> elements are pointers to the start of the
    //     <n> buckets. Note that element <n> also marks the exclusive 
    //     end of the <n-1> bucket.
    // (b) One additional element is written at to mark the exclusive
    //     end of the very last bucket.
    // (c) If RewriteROMConstantPool is disabled, the contents of
    //     bucket #0 starts here, followed by the contents of bucket #1,
    //     and so forth.
    //
    // For example, say we have two buckets; bucket #0 contains "foo";
    // bucket #1 contains "bar" and "blah":
    //
    // symbol_table_size = 2;
    // symbol_table[] = {
    //     &symbol_table[3];          // (a) start of bucket#0
    //     &symbol_table[4];          // (a) end of bucket#0/start of bucket#1
    //     &symbol_table[6];          // (b) end of bucket#1
    //     pointer to Symbol "foo";   // (c) item in bucket#0
    //     pointer to Symbol "bar";   // (c) item in bucket#1
    //     pointer to Symbol "blah";  // (c) item in bucket#1
    // };
    //
    // Note that if RewriteROMConstantPool is enabled (the default),
    // part (c) is actually stored inside the merged constant pool;
    // parts (a) and (b) are written with special macros to point to
    // the merged constant pool instead.

    ReturnOop old_sym = ROM::symbol_for(s, hash_value, len);
    if( old_sym ) {
      return old_sym;
    }
  }

  const juint mask = juint(length() - 1);
  juint index = hash_value & mask;

  const juint start = index;

  SymbolDesc**base = (SymbolDesc**)base_address();
  SymbolDesc* old;

  if (0 < len && len <= 6) {
    // Quicker search. This happens very frequently if a MIDlet is
    // obfuscated -- many variables will have name length <= 6
    //
    // Note: this requires that the unused space in the SymbolDesc be filled
    // with zeros.
    union {
      jushort shorts[4];
      juint   ints[2];
    } blob;
    blob.ints[0] = 0;
    blob.ints[1] = 0;
    blob.shorts[0] = (jushort)len;
    jvm_memcpy(&blob.shorts[1], s, len);

    const juint blob0 = blob.ints[0];
    if( len <= 2 ) {
      do {
        old = base[index];
        if( old == NULL ) break;
        const juint *body = ((const juint*)old) + sizeof(OopDesc)/BytesPerWord;
        if (blob0 == body[0]) {
          return (ReturnOop)old;
        }
        index ++;
        index &= mask;
        // Do not rewrite as  while( (index = (++index & mask)) != start );
        // ADS compiler generates incorrect code.
      } while( index != start );
    } else {
      const juint blob1 = blob.ints[1];
      do {
        old = base[index];
        if( old == NULL ) break;
        const juint *body = ((const juint*)old) + sizeof(OopDesc)/BytesPerWord;
        if( blob0 == body[0] && blob1 == body[1] ) {
          return (ReturnOop)old;
        }
        index ++;
        index &= mask;
        // Do not rewrite as  while( (index = (++index & mask)) != start );
        // ADS compiler generates incorrect code.
      } while( index != start );
    }
  } else {
    do {
      old = base[index];
      if( old == NULL ) break;
      if( old->matches(s, len)) {
        return (ReturnOop)old;
      }
      index ++;
      index &= mask;
      // Do not rewrite as  while( (index = (++index & mask)) != start );
      // ADS compiler generates incorrect code.
    } while( index != start );
  }

  if (check_only) {
    // The specified symbol is not found
    return NULL;
  } else {
    if( old ) {
      // We'd come to here if we're really out of memory
      Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW_0);
    }

    // Create a new Symbol of the specified value
    UsingFastOops fast_oops;
    Symbol::Fast new_symbol = Universe::new_symbol(byte_array, s, len 
                                                   JVM_CHECK_0);
    obj_at_put(index, &new_symbol);

    int new_count = Task::current()->incr_symbol_table_count();
    if (new_count > desired_max_symbol_count()) {
      grow_and_replace_symbol_table();
    }
    return new_symbol.obj();
  }
}

ReturnOop SymbolTable::slashified_symbol_for(utf8 s, int len JVM_TRAPS) {
  UsingFastOops fast_oops;

  TypeArray::Fast byte_array = Universe::new_byte_array(len JVM_CHECK_0);

  utf8 p = (utf8)byte_array().base_address();
  jvm_memcpy(p, s, len);

  for (int i = 0; i < len; i++) {
    if (p[i] == '/') {
      // This makes external class names which already contain '/' instead
      // of '.' fail resolution
      p[i] = '.';
    } else if (p[i] == '.') {
      p[i] = '/';
    }
  }

  return symbol_for(&byte_array JVM_NO_CHECK_AT_BOTTOM_0);
}

// This function is not frequently used. No need to optimize.
ReturnOop SymbolTable::slashified_symbol_for(const char* s JVM_TRAPS) {
  return slashified_symbol_for((utf8)s, jvm_strlen(s) JVM_NO_CHECK_AT_BOTTOM);
}

// This function is not frequently used (only by Class.forName()).
// No need to optimize.
ReturnOop SymbolTable::symbol_for(String* string, bool slashify JVM_TRAPS) {
  // Speculatively allocate 4 bytes per jchar, in any case more than
  // the maximum possible space needed for UTF-8 conversion.
  UsingFastOops fast_oops;

  TypeArray::Fast byte_array = Symbol::copy_string_to_byte_array(string->obj(), slashify JVM_CHECK_0);
  
  return symbol_for(&byte_array, (utf8)byte_array().base_address(), byte_array().length() JVM_NO_CHECK_AT_BOTTOM_0);
}

inline void SymbolTable::insert(Symbol* symbol) {
  const juint mask = juint(length() - 1);
  juint index = hash(symbol) & mask;
  
  AZZERT_ONLY(const juint start = index;)
  while (obj_at(index) != NULL) {
    index ++;
    index &= mask;
    // Do not rewrite as  index = (++index & mask);
    // ADS compiler generates incorrect code.
    GUARANTEE(index != start, "New symbol hash table overflows");
  }

  obj_at_put(index, symbol);
}

// Rehash all symbols into a new bigger table
void SymbolTable::grow_and_replace_symbol_table( void ) {
#ifdef AZZERT
  handle_uniqueness_verification();
#endif

  const int old_length = length();
  const int new_length = old_length * 2;

  if (Verbose) {
    TTY_TRACE_CR(("Expanding symbol_table to %d entries", new_length));
  }

  // NOTE:
  // Don't do this: *symbol_table() = Universe::new_obj_array(...);
  // If the allocation fails symbol_table will become NULL, and subsequent
  // access to symbol_table would cause error in VM!
  SETUP_ERROR_CHECKER_ARG;
  SymbolTable::Raw new_table = initialize(new_length JVM_NO_CHECK);
  if( new_table.is_null() ) {
    Thread::clear_current_pending_exception();
    return;
  }
    
  for (int i = 0; i < old_length; i++) {
    Symbol::Raw symbol = obj_at(i);
    if (symbol.not_null()) {
      new_table().insert(&symbol);
    }
  }

  set_obj(new_table);
  *current() = new_table;
#if ENABLE_ISOLATES
  Task::current()->set_symbol_table(new_table);
#endif
}
