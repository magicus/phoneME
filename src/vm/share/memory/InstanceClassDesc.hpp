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

class InstanceClassDesc: public JavaClassDesc { 
 public:
  // GC support.
  void variable_oops_do(void do_oop(OopDesc**));

 private:
  // Compute allocation size
  static size_t allocation_size(size_t static_field_size,
                                size_t oop_map_size,
                                size_t vtable_length) {
    size_t size = sizeof(InstanceClassDesc) + oop_map_size;

#if !ENABLE_ISOLATES 
    size += static_field_size;
#else
    (void)static_field_size;
#endif

#if !ENABLE_ISOLATES && USE_EMBEDDED_VTABLE_BITMAP
    size += bitmap_size(vtable_length);
#else
    (void)vtable_length;
#endif

    return align_allocation_size(size);
  }

  friend class Universe;
  friend class ROMOptimizer;
};
