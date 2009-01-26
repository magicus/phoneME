/*
 *   
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

#if ENABLE_JNI

class JniFrame : public MixedOop {
public:
  HANDLE_DEFINITION(JniFrame, MixedOop);

protected:
  static int prev_frame_offset() {
    return FIELD_OFFSET(JniFrameDesc, _prev_frame);
  }
  static int local_ref_index_offset() {
    return FIELD_OFFSET(JniFrameDesc, _local_ref_index);
  }

public:
  static ReturnOop allocate_jni_frame(JVM_SINGLE_ARG_TRAPS) {
    return Universe::new_mixed_oop(MixedOopDesc::Type_JniFrame,
                                   JniFrameDesc::allocation_size(),
                                   JniFrameDesc::pointer_count()
                                   JVM_NO_CHECK_AT_BOTTOM);
  }

  ReturnOop prev_frame() const {
    return obj_field(prev_frame_offset());
  }
  void set_prev_frame(JniFrame* frame) {
    set_prev_frame(frame->obj());
  }
  void set_prev_frame(OopDesc* frame) {
    obj_field_put(prev_frame_offset(), frame);
  }

  int local_ref_index() const {
    return int_field(local_ref_index_offset());
  }
  void set_local_ref_index(int index) {
    int_field_put(local_ref_index_offset(), index);
  }
};

#endif
