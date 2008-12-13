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

/*
 * WARNING -- the #if ENABLE_COMPILER stuff is needed to make AOT work
 * for the ARM platform. More cleanup is necessary
 */

extern bool __is_arm_compiler_active();

inline int JavaFrame__arg_offset_from_sp(int index) {
  // The stack pointer points at the beginning of the last argument
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_JavaFrame__arg_offset_from_sp(index);
  }
#endif
  return BytesPerStackElement * index;
}

inline int JavaFrame__stack_bottom_pointer_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_JavaFrame__stack_bottom_pointer_offset();
  }
#endif
  return  -28;
}
inline int JavaFrame__stored_int_value1_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_JavaFrame__stored_int_value1_offset();
  }
#endif
  return  -24;
}
inline int JavaFrame__stored_int_value2_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_JavaFrame__stored_int_value2_offset();
  }
#endif
  return  -20;
}
inline int JavaFrame__bcp_store_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_JavaFrame__bcp_store_offset();
  }
#endif
  return  -16;
}
inline int JavaFrame__locals_pointer_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_JavaFrame__locals_pointer_offset();
  }
#endif
  return  -12;
}
inline int JavaFrame__cpool_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_JavaFrame__cpool_offset();
  }
#endif
  return  -8;
}
inline int JavaFrame__method_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_JavaFrame__method_offset();
  }
#endif
  return  -4;
}
inline int JavaFrame__caller_fp_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_JavaFrame__caller_fp_offset();
  }
#endif
  return   0;
}
inline int JavaFrame__return_address_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_JavaFrame__return_address_offset();
  }
#endif
  return   4;
}

inline int JavaFrame__frame_desc_size() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_JavaFrame__frame_desc_size();
  }
#endif
  return 9*BytesPerWord;
}  

inline int JavaFrame__end_of_locals_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_JavaFrame__end_of_locals_offset();
  }
#endif
  return   8;
}
inline int JavaFrame__empty_stack_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_JavaFrame__empty_stack_offset();
  }
#endif
  return -28;
}


inline int EntryFrame__pending_activation_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_EntryFrame__pending_activation_offset();
  }
#endif
  return  -28;
}
inline int EntryFrame__pending_exception_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_EntryFrame__pending_exception_offset();
  }
#endif
  return  -24;
}
inline int EntryFrame__stored_obj_value_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_EntryFrame__stored_obj_value_offset();
  }
#endif
  return  -20;
}
inline int EntryFrame__stored_int_value2_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_EntryFrame__stored_int_value2_offset();
  }
#endif
  return  -16;
}
inline int EntryFrame__stored_int_value1_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_EntryFrame__stored_int_value1_offset();
  }
#endif
  return  -12;
}
inline int EntryFrame__stored_last_sp_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_EntryFrame__stored_last_sp_offset();
  }
#endif
  return -8;
}
inline int EntryFrame__stored_last_fp_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_EntryFrame__stored_last_fp_offset();
  }
#endif
  return -4;
}
inline int EntryFrame__real_return_address_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_EntryFrame__real_return_address_offset();
  }
#endif
  return  0;
}
inline int EntryFrame__fake_return_address_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_EntryFrame__fake_return_address_offset();
  }
#endif
  return +4;
}

inline int EntryFrame__frame_desc_size() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_EntryFrame__frame_desc_size();
  }
#endif
  return 9 * BytesPerWord;
}

// When an EntryFrame is empty, the sp points at the word above
inline int EntryFrame__empty_stack_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_EntryFrame__empty_stack_offset();
  }
#endif
  return  -28;
}

#if ENABLE_EMBEDDED_CALLINFO
// number of bytes between the return address and the start of the callinfo
inline int JavaFrame__callinfo_offset_from_return_address() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_JavaFrame__callinfo_offset_from_return_address();
  }
#endif
  return -4;
}
#endif // ENABLE_EMBEDDED_CALLINFO

// number of bytes between a stack tag value and its tag
inline int StackValue__stack_tag_offset() {
#if ENABLE_COMPILER
  if (__is_arm_compiler_active()) {
    return arm_StackValue__stack_tag_offset();
  }
#endif
  return 4;
}
