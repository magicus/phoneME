/*
 *   
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
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
 *
 *!c<
 * Copyright 2006 Intel Corporation. All rights reserved.
 *!c>
 */

# include "incls/_precompiled.incl"
# include "incls/_CompiledMethod.cpp.incl"

#if USE_COMPILER_STRUCTURES
HANDLE_CHECK(CompiledMethod, is_compiled_method())
#endif

#if ENABLE_COMPILER

void CompiledMethod::shrink(jint code_size, jint relocation_size) {
  // The current implementation copies the relocation information down
  // and "shrinks" the compiled method object in place, allocating a
  // dummy filler object in the now unused end part.
  //
  // The compiled method object will generally not be the last object in
  // the heap, since the compiler allocates other objects and GC might
  // have occurred. However, if the GC always does sliding compaction
  // and the compiler *guarantees* not to hold on to any allocated
  // object other than the compiled method, we could simply move the
  // top of the object heap down!

  // Copy the relocation segment down
  void* src = field_base(end_offset() - relocation_size);
  void* dst = field_base(base_offset() + code_size);
  GUARANTEE(src >= dst, "should be copying down");
  jvm_memmove(dst, src, relocation_size); // possibly overlapping regions

  // Shrink compiled method object
  size_t new_size = CompiledMethodDesc::allocation_size(code_size +
                                                        relocation_size);
  Universe::shrink_object(this, new_size);
  ((CompiledMethodDesc*) obj())->set_size(code_size + relocation_size);
  GUARANTEE(object_size() == new_size, "invalid shrunk size");
}

bool CompiledMethod::expand_compiled_code_space(int delta, int relocation_size) {
  if( !ObjectHeap::expand_current_compiled_method(delta)) {
    return false;
  }

  if (Verbose) {
    TTY_TRACE_CR(("Expanding compiled method from %d to %d bytes", 
                  size(), size() + delta));
  }
  void* src = field_base(end_offset() - relocation_size);
  void* dst = DERIVED(void*, src, delta);
  GUARANTEE(src < dst, "should be copying up");
  jvm_memmove(dst, src, relocation_size); // possibly overlapping regions
  // It's probably OK only to clear dst[-1], but let's just make sure.
  jvm_memset(src, 0, delta);
  ((CompiledMethodDesc*) obj())->set_size(size() + delta);  

  if (VerifyGC > 2) {
    ObjectHeap::verify();
  }
  return true;
}

void CompiledMethod::flush_icache() {
  // Eventually, we can get rid of the relocation
  OsMisc_flush_icache(entry(), size());
}

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
void CompiledMethod::print_comment_for(int code_offset, Stream* st) {
#if USE_DEBUG_PRINTING
  bool emit_blank_line = true;
  for (RelocationReader stream(this); !stream.at_end(); stream.advance()) {
    if (stream.code_offset() == code_offset) {
      if (stream.is_comment()) {
        if (emit_blank_line) {
          st->cr();
          emit_blank_line = false;
        }
        st->print("       // ");
        stream.print_comment_on(st);
        st->cr();
      } else if (stream.kind() == Relocation::osr_stub_type) {
        st->print_cr("       @ bci = %d", stream.current(1));
      }
    }
  }
#endif
}

void CompiledMethod::print_name_on(Stream* st) {
#if USE_DEBUG_PRINTING
  print_value_on(st);
  st->cr();
#endif
}

void CompiledMethod::print_relocation_on(Stream* st) {
#if USE_DEBUG_PRINTING
  st->print_cr("Relocation information");
  RelocationReader reloc(this);
  reloc.print(st, true);
#endif
}

#endif // !PRODUCT

#endif // ENABLE_COMPILER

#if !defined(PRODUCT) || USE_DEBUG_PRINTING

void CompiledMethod::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  Oop::iterate(visitor);

  { // owning method
    NamedField id("method", true);
    visitor->do_oop(&id, method_offset(), true);
  }

  { // flags and size
    NamedField id("flags_and_size", true);
    visitor->do_uint(&id, flags_and_size_offset(), true);
  }
  // Do we want to disassemble this in verbose mode?
#endif
}

void CompiledMethod::print_value_on(Stream* st) {
#if USE_DEBUG_PRINTING
  UsingFastOops fast_oops;
  st->print("Compiled");
  Method::Fast m = method();

  bool saved = Verbose;
  Verbose = false;
  m().print_value_on(st);
  Verbose = saved;
#endif
}

void CompiledMethod::iterate_oopmaps(oopmaps_doer do_map, void* param) {
#if USE_OOP_VISITOR
 OOPMAP_ENTRY_4(do_map, param, T_INT,    flags_and_size);
 OOPMAP_ENTRY_4(do_map, param, T_OBJECT, method);
#if ENABLE_JVMPI_PROFILE
 OOPMAP_ENTRY_4(do_map, param, T_INT, jvmpi_code_size);
#endif
#endif
}

#endif
