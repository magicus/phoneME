/*
 *
 * Portions Copyright  2003-2006 Sun Microsystems, Inc. All Rights Reserved.
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

class BasicBlock: public AllStatic {
private:
  static void add_entry(TypeArray *entry_counts, int bci) {
    jubyte entries = entry_counts->ubyte_at(bci);
    if (entries < 10) {
      entry_counts->byte_at_put(bci, (jbyte)(entries + 1));
    }
  }
  static bool compute_branch_entries(const Method* method, TypeArray *entry_counts);
#if ENABLE_NPCE && ENABLE_INTERNAL_CODE_OPTIMIZER
  //get the number of null point exception throwable byte code 
  static bool is_null_point_exception_throwable(Bytecodes::Code code);
#endif 
public:
  // Calculate the basic blocks for a given method. Returns an integer array
  // that represent the number of incoming nodes for each bytecode.
  static ReturnOop compute_entry_counts(Method* method, bool &has_loops
                                        JVM_TRAPS);
};
