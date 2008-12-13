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
# include "incls/_CompiledMethodDesc_arm.cpp.incl"

#if ENABLE_COMPILER || USE_COMPILER_STRUCTURES
extern "C" {
  extern address gp_base_label;
}
#endif


#if ENABLE_COMPILER

void CompiledMethodDesc::update_relative_offsets(int delta) {
  if ((_flags_and_size & HAS_BRANCH_RELOCATION_MASK) == 0) {
    return;
  }

  CompiledMethod::Raw cm = this;
  if (cm().size() > 0) {
    // Compiled methods of size 0 are discarded compiled methods that we're
    // in the middle of discarding.  Attempting to use a RelocationReader
    // on them fails.

    for (RelocationReader stream(&cm); !stream.at_end(); stream.advance()) {
      if (stream.is_long_branch()) {
        Branch b(cm().entry() + stream.code_offset());
        b.relocate(delta);
      }
    }
  }
}

#endif

#if USE_COMPILER_STRUCTURES

int CompiledMethodDesc::get_cache_index ( void ) const {
  // The top 9 bits are the cache index.
  return (int)(_flags_and_size >> 
               CompiledMethodDesc::NUMBER_OF_NON_INDEX_BITS);
}

void CompiledMethodDesc::set_cache_index ( const int i ) {
  const size_t non_index_mask = ~CompiledMethodDesc::INDEX_MASK;
  const size_t bits_and_size = _flags_and_size & non_index_mask;
  _flags_and_size = bits_and_size | 
                    (size_t(i)<<CompiledMethodDesc::NUMBER_OF_NON_INDEX_BITS);

  // Patch the "strb gp, [gp, #xxxx]" instruction. It does not appear
  // at a fixed location because of code scheduling.
  juint* index_loc = (juint*) (this+1);

#ifdef AZZERT
  juint* index_end = index_loc + 10; // we might have debug code at the
                                     // beginning of the compiled method prolog
#else
  juint* index_end = index_loc + 3;
#endif

  const juint opcode = 
    (juint)Assembler::always << 28 | 1 << 26 | 1 << 22 | Assembler::gp << 12;
  const juint addressing = 3 << 23 | Assembler::gp << 16;
  const juint pattern = opcode | addressing;
  const juint mask = 0xfffff000;

  for (; index_loc < index_end; index_loc++) {
    juint instr = *index_loc & mask;
    if (instr == pattern) {
      *index_loc = instr |
         (i + (address(_method_execution_sensor) - address(&gp_base_label)));
      return;
    }
  }
  SHOULD_NOT_REACH_HERE();

  // set_cach_index is called before major GC
  // The worst that may happen in the rare case when icache is not updated
  // is just a minor deviation in statistics.
  // OsMisc_flush_icache( (address) index_loc, sizeof *index_loc );
}

#endif // USE_COMPILER_STRUCTURES
