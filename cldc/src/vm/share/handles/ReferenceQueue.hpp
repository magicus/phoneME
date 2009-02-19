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

/** \file ReferenceQueue.hpp
 *
 * Interface to java.lang.ref.ReferenceQueue objects from C code.
 */

#if ENABLE_CLDC_11

class ReferenceQueue : public Instance {
 public:
  HANDLE_DEFINITION_CHECK(ReferenceQueue, Instance);

  static int head_offset() {
    return header_size() + 0 * sizeof(jobject);
  }

  ReturnOop head() const {
    return obj_field(head_offset());
  }

  void set_head(Oop* value) {
    obj_field_put(head_offset(), value);
  }

  static void verify_fields() PRODUCT_RETURN;
};

#endif // ENABLE_CLDC_11
