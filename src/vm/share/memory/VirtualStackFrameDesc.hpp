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
  int            _literals_mask;
  int            _flush_count;
#if ENABLE_REMEMBER_ARRAY_LENGTH
  int            _bound_mask;
#endif

  // Followed by ((_location_map_size+_literals_map_size) * sizeof(int)) bytes

  friend class VirtualStackFrame;
  friend class Universe;
  friend class OopDesc;
};
