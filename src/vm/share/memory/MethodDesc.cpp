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
# include "incls/_MethodDesc.cpp.incl"

void MethodDesc::relocate_variable_part(int delta) {
  GUARANTEE(ObjectHeap::contains((OopDesc*)this), "sanity");
  _variable_part = (MethodVariablePart*)(int(_variable_part) + delta);
}

#if ENABLE_METHOD_TRAPS
/*
 * Tests if the method is trapped or not and returns pointer
 * to the corresponding trap structure if it is
 */
MethodTrapDesc* MethodVariablePart::get_trap() const {
  int index= (_execution_entry - (address)cautious_invoke) / trap_entry_offset;
  if (index >= 0 && index < max_traps && !_method_trap[index].is_free()) {
    return &_method_trap[index];
  } else {
    return NULL;
  }
}
#endif

address MethodVariablePart::execution_entry(void) const {
#if ENABLE_METHOD_TRAPS
  MethodTrapDesc* trap = get_trap();
  if (trap != NULL) {
    return trap->old_entry();
  }
#endif
  return _execution_entry;
}

void MethodDesc::set_execution_entry(address entry) {
  set_execution_entry_inline(entry);
}
