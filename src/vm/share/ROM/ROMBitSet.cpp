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

#include "incls/_precompiled.incl"
#include "incls/_ROMBitSet.cpp.incl"

#if ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR
HANDLE_CHECK(ROMBitSet, is_type_array())

int ROMBitSet::_size;
int ROMBitSet::_bit_size;
int ROMBitSet::_range_start;
int ROMBitSet::_range_length;

void ROMBitSet::print_class_names( Stream* out, const char prefix[] ) {
  for (SystemClassStream st; st.has_next(); ) {
    InstanceClass::Raw klass = st.next();
    GUARANTEE(klass.not_null(), "Sanity");

    if( get_bit( klass().class_id() ) ) {
      out->print_raw( prefix );
      klass().print_name_on( out );
      out->cr();
    }
  }
}

void ROMBitSet::print_bytes( Stream* out, const int from, int count ) {
  GUARANTEE(unsigned(from)       <  unsigned(length() * BytesPerInt), "Sanity");
  GUARANTEE(unsigned(from+count) <= unsigned(length() * BytesPerInt), "Sanity");
  GUARANTEE(count > 0, "Sanity");

  const jubyte* p = bytes() + from;
  do {
    out->print( "0x%x, ", *p++ );
  } while( --count );
}
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR

