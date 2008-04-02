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

/** \class CallInfo
    Describes the word associated with each call in
    both the interpreter and the compiler generated code.

    A CallInfo contains information about where the call is
    located:
    in_compiled_code
    in_interpreter
    If the call is in compiled code we also store bci and 
    offset to the start of the compiled method.
\verbatim
    On x86:
      call foo
      test eax, {DWORD}

    On ARM:
      call foo
      {DWORD}      "return +4 is used to return past CallInfo

    The CallInfo as two formats:

     format1:
       0-12,  tag information for 13 locations
       13-20,  byte code index for call
       21-30,  offset to start of compiled method
       31,  = 1 (location must be in compiled code)

     format2:
       0-15,  bci for call
       16-30,  offset to start of compiled method
       31,  = 0 

The value 0 is reserved for interpreted code. 
\endverbatim
*/

#if ENABLE_EMBEDDED_CALLINFO

class CallInfo {
 public:
  enum {
    compact_start              = 31,

    format1_tag_width          = 13,
    format1_bci_width          =  8,
    format1_start_offset_width = 10,

    format1_tag_start          =  0,
    format1_bci_start          =  format1_tag_start + format1_tag_width,
    format1_start_offset_start =  format1_bci_start + format1_bci_width,
    
    format2_bci_width          = 16,
    format2_start_offset_width = 15,
    
    format2_bci_start          =  0,
    format2_start_offset_start =  format2_bci_start + format2_bci_width
  };

 private:
  intptr_t _value;
  CallInfo(int value) { _value = value; }

  static int stored_start_offset(int start_offset);
  int restored_start_offset(int stored_start_offset);

 public:
  // Construction of CallInfo
 
  // Tells whether the information qualifies for the compact format
  // Remember an additional requirement is that
  // the tags must represent either oop or int like
  static bool fits_compiled_compact_format(int bci, int offset_to_start, int number_of_tags);

  // Create format1 for compiled code
  static CallInfo compiled(int bci, int offset_to_start JVM_TRAPS);

  // Create format2 for compiled code
  static CallInfo compiled_compact(int bci, int offset_to_start);

  // Create format2 for interpreter
  static CallInfo interpreter() { return CallInfo(0); }

  // Access to CallInfo

  // Set the index to have an oop, only works with compact information
  void set_oop_tag_at(int index);

  // Tells where the CallInfo is located
  bool in_interpreter()   { return _value == 0; }
  bool in_compiled_code() { return _value != 0; }

  // Tells whether the information is stored in format1
  bool is_compact_format() { return is_set_nth_bit(_value, compact_start); }

  // Accessors for CallInfo in compiled code
  int bci();
  int start_offset();

  Tag compact_tag_at(int index);
  Tag extended_tag_at(int index);

  // Return the raw data
  int raw() { return _value; }

#if USE_COMPILER_STRUCTURES
  CompiledMethodDesc* compiled_method();
#endif

#if ENABLE_COMPILER
  static ReturnOop new_tag_array(int size JVM_TRAPS);
#endif
  static void set_tag(TypeArray*, int index, BasicType type);

private:    
  enum { tag_array_overhead = 4 };

  static Tag nybble_to_stack_tag(int nybble) {
    if (nybble == 0) { 
        return uninitialized_tag; 
    } else { 
        return (Tag)(1 << (nybble - 1));
    }
  }

  static int stack_tag_to_nybble(Tag stack_tag) {
    if (stack_tag == uninitialized_tag) { 
      return 0;
    } else {
       return exact_log2(stack_tag) + 1;
    }
  }

  static void index_to_offsets(int index, 
                               int& word_offset, int& bit_offset) {
    // 8 nybbles per word, each nybble is 4 bits.
    word_offset = ((unsigned) (index + tag_array_overhead)) >> 3;
    bit_offset  = ((index + tag_array_overhead) & 7) << 2;
  }

public:

#ifndef PRODUCT 
  void print(Stream* st = tty);
#else
  void print(Stream*) {}
#endif
};

#endif // ENABLE_EMBEDDED_CALLINFO
