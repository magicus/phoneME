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
# include "incls/_CompiledMethodDesc_thumb.cpp.incl"

#if ENABLE_COMPILER || USE_COMPILER_STRUCTURES
extern "C" {
  extern address gp_base_label;
}
#endif


#if ENABLE_COMPILER


void CompiledMethodDesc::update_relative_offsets(int /*delta*/) {
  // Nothing to do on THUMB.
}

#endif

#if USE_COMPILER_STRUCTURES

int CompiledMethodDesc::get_cache_index ( void ) const {
#if !ENABLE_ARM_V6T2
  // The top 9 bits are the cache index.
  return (int)(_flags_and_size >> NUMBER_OF_SIZE_BITS);
#else
  const int* index_loc = (int*) (this+1) + 1;
  const int i = (*index_loc & 0x0FFF) +
    ((address)&gp_base_label) - ((address)_method_execution_sensor);
  GUARANTEE( 0 <= i && i < method_execution_sensor_size,
    "Wrong code in the beginning of compiled method" );
  return i;
#endif
}

void CompiledMethodDesc::set_cache_index ( const int i ) {
#if !ENABLE_ARM_V6T2
  // We don't need to patch any code. Sensor update is done by
  // moving Method::execution_entry()
  _flags_and_size = (size_t(i) << NUMBER_OF_SIZE_BITS) | code_size();
#else
  jushort* index_loc = (jushort*) (this+1) + 1;
  *index_loc = (*index_loc & 0xF000) |
    (i + (((address)_method_execution_sensor) - ((address)&gp_base_label)));
  // set_cach_index is called before major GC
  // The worst that may happen in the rare case when icache is not updated
  // is just a minor deviation in statistics.
  // OsMisc_flush_icache( (address) index_loc, sizeof *index_loc );
#endif
}

#endif // USE_COMPILER_STRUCTURES
