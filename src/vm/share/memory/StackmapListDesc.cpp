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
# include "incls/_StackmapListDesc.cpp.incl"

const jushort StackmapListDesc::SHORT_ENTRY_FLAG              = 0x1;
const jushort StackmapListDesc::SHORT_ENTRY_STACK_HEIGHT_MASK = 0x3;
const jushort StackmapListDesc::SHORT_ENTRY_STACK_HEIGHT_MAX  = 0x3; 
const jushort StackmapListDesc::SHORT_ENTRY_LOCALS_MAX        = 0xF; 
const jushort StackmapListDesc::SHORT_STACKMAP_BITS_MAX       = 0x10;  //16
const jushort StackmapListDesc::SHORT_ENTRY_BCI_MAX           = 0x1FF; //511

// Initializes the object after allocation

void StackmapListDesc::initialize(OopDesc* klass, int size) {
  _entry_count = size;
  OopDesc::initialize(klass);
}

void StackmapListDesc::variable_oops_do(void do_oop(OopDesc**)) {
  unsigned int* entries = stackmap_entries_base();
  unsigned int* end = entries + _entry_count;

  while (entries < end) {
    unsigned int _entry_stat = *entries;
    if (!(_entry_stat & SHORT_ENTRY_FLAG)) {
      do_oop((OopDesc**)entries);
    }
    ++ entries;
  }
}

bool StackmapListDesc::is_shortmap_type(juint locals, juint stack_height, 
                                        juint bci) {
  if ((locals > SHORT_ENTRY_LOCALS_MAX) ||
      (bci > SHORT_ENTRY_BCI_MAX) ||
      (stack_height > SHORT_ENTRY_STACK_HEIGHT_MAX)) {
    return false;
  }

  if (stack_height + locals > SHORT_STACKMAP_BITS_MAX) {
    return false;
  }
  
  return true;
}
