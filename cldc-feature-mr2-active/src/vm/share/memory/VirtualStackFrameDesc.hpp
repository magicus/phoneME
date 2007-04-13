/*
 *   
 *
 * Portions Copyright  2000-2006 Sun Microsystems, Inc. All Rights Reserved.
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

class VirtualStackFrameDesc: public MixedOopDesc { 
 private:
  static size_t header_size() {
    GUARANTEE(sizeof(VirtualStackFrameDesc) == 
              align_allocation_size(sizeof(VirtualStackFrameDesc)), "sanity");
    return align_allocation_size(sizeof(VirtualStackFrameDesc));
  }
  static int pointer_count() {
#if USE_COMPILER_FPU_MAP
    return 1;
#else
    return 0;
#endif
  }

 private:
#if USE_COMPILER_FPU_MAP
  TypeArrayDesc * _fpu_register_map;
#endif

  /* All oops must go before here.  If you change the number of oops, be
   * sure to change pointer_count()
   */

  int            _real_stack_pointer;
  int            _virtual_stack_pointer;
  int            _saved_stack_pointer;
  int            _flush_count;
#if ENABLE_REMEMBER_ARRAY_LENGTH
  //bitmap of _bound_mask
  //31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0  
  //|flag       |                                       |base      |boundary  |local index                    |
  //flag:  whether is valid
  //base:  store the index of array base register
  //boundary:  store the the index of array length register
  //local index: store the location index(in the Virstual Stack Frame) of a array boundary checked local variable
  //We use 4bits to represent the index of ARM register(16 registers). So we use other approach to represent the case 
  //there's no register assigned for base or boundary. 
  //we don't adjust register reference when one is cached.
  int            _bound_mask;
#endif

#if ENABLE_ARM_VFP  
  // Literals mask is necessary to distingish zero literals from non-literals
  // in literals map.  If literal map[reg] == 0 && literals_mask & 1 << reg
  // then reg contains zero literal otherwise reg does not contain a literal.
  int            _literals_mask[2];    
#endif  // ENABLE_ARM_VFP

  // Followed by ((_location_map_size+_literals_map_size) * sizeof(int)) bytes

  friend class VirtualStackFrame;
  friend class Universe;
  friend class OopDesc;
};
