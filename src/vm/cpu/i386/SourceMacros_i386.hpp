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

#ifndef PRODUCT

class SourceMacros: public SourceAssembler {
 public:
  SourceMacros(Stream* output) : SourceAssembler(output) {}

  // Support for fast byte/word loading with zero extension.
  void load_unsigned_byte(const Register dst, const Register src);
  void load_unsigned_byte(const Register dst, const Address& src);
  void load_unsigned_word(const Register dst, const Register src);
  void load_unsigned_word(const Register dst, const Address& src);

  // Support for fast byte/word loading with sign extension.
  void load_signed_byte(const Register dst, const Register src);
  void load_signed_byte(const Register dst, const Address& src);
  void load_signed_word(const Register dst, const Register src);
  void load_signed_word(const Register dst, const Address& src);

  // Increment/decrement a register by a constant value.
  void increment(const Register reg, const int value);
  void decrement(const Register reg, const int value);

  // Divide register eax by reg with the given register.
  void corrected_idivl(const Register reg);

  // Extra floating point support routines.
  void fremp();
  void fcmp();
  void fcmp2int(const Register dst, bool unordered_is_less);

  // Verify that the source contains the specified tag.
  void verify_tag(const Register src, const int tag);
  void verify_tag(const Address& src, const int tag);

  // Support for pushing tagged value on the stack.
  void push_int   (const Register src);
  void push_int   (const Address& src);
  void push_int   (const int imm32);

  void push_long  (const Register low, const Register high);
  void push_long  (const Address& low, const Address& high);
  void push_long  (const jlong imm64);

  void push_float (const Register src);
  void push_float (const Address&  src);
  void push_float (const jfloat imm32);

  void push_double(const Register low, const Register high);
  void push_double(const Address& low, const Address& high);
  void push_double(const jdouble imm64);

  void push_obj   (const Register src);
  void push_obj   (const Address&  src);

  void push_ret   (const Register src);

  // Special support for pushing the null object.
  void push_null_obj();

  // Support for popping tagged values from the stack.
  void pop_int    (const Register dst, const Register tag);
  void pop_int    (const Address& dst, const Register tag);
 
  void pop_long   (const Register low, const Register high);
  void pop_long   (const Address& low, const Address& high, const Register tag);

  void pop_float  (const Register dst, const Register tag);
  void pop_float  (const Address& dst, const Register tag);

  void pop_double (const Register low, const Register high);
  void pop_double (const Address& low, const Address& high, const Register tag);

  void pop_obj    (const Register dst, const Register tag);
  void pop_obj    (const Address& dst, const Register tag);

  void pop_obj_ret(const Register dst, const Register tag);
  void pop_obj_ret(const Address& dst, const Register tag);

  // Support for pushing locals onto the stack.
  void push_local_int    (const int index);
  void push_local_int    (const Register index);

  void push_local_long   (const int index);
  void push_local_long   (const Register index);

  void push_local_float  (const int index);
  void push_local_float  (const Register index);

  void push_local_double (const int index);
  void push_local_double (const Register index);

  void push_local_obj    (const int index);
  void push_local_obj    (const Register index);

  // Support for popping tagged values from the stack into locals.
  void pop_local_int     (const int index, const Register tag);
  void pop_local_int     (const Register index, const Register tag);

  void pop_local_long    (const int index, const Register tag);
  void pop_local_long    (const Register index, const Register tag);

  void pop_local_float   (const int index, const Register tag);
  void pop_local_float   (const Register index, const Register tag);

  void pop_local_double  (const int index, const Register tag);
  void pop_local_double  (const Register index, const Register tag);

  void pop_local_obj     (const int index, const Register tag);
  void pop_local_obj     (const Register index, const Register tag);

  void pop_local_obj_ret (const int index, const Register tag);
  void pop_local_obj_ret (const Register index, const Register tag);

  Address local_address(int n, int offset = 0) {
    return Address(edi, Constant((-n * BytesPerStackElement) + offset));    
  } 

  Address local_address(const Register reg, int offset = 0) { 
    return Address(edi, reg, TaggedJavaStack ? times_8 : times_4,
                   Constant(offset));     
  }

  Address local_tag_address     (int n) { 
    GUARANTEE(TaggedJavaStack, "Should not call this");
    return local_address(n, -4); 
  }

  Address local_address_high    (int n) { 
    return local_address(n, -0); 
  }

  Address local_tag_address_high(int n) { 
    GUARANTEE(TaggedJavaStack, "Should not call this");
    return local_address(n, -4); 
  }

  Address local_address_low     (int n) { 
    return local_address(n, -BytesPerStackElement); 
  }

  Address local_tag_address_low (int n) { 
    GUARANTEE(TaggedJavaStack, "Should not call this");
    return local_address(n, -12); 
  }

  Address local_tag_address     (const Register reg) { 
    GUARANTEE(TaggedJavaStack, "Should not call this");
    return local_address(reg, -4); 
  }

  Address local_address_high    (const Register reg) {
    return local_address(reg); 
  }

  Address local_tag_address_high(const Register reg) { 
    GUARANTEE(TaggedJavaStack, "Should not call this");
    return local_address(reg, -4); 
  }

  Address local_address_low     (const Register reg) { 
    return local_address(reg, -BytesPerStackElement); 
  }

  Address local_tag_address_low (const Register reg) { 
    GUARANTEE(TaggedJavaStack, "Should not call this");
    return local_address(reg, -12); 
  }

  Address stackvar_address         (int offset = 0) { 
    return Address(esp, Constant(JavaFrame::arg_offset_from_sp(0) + offset)); 
  }

  Address stackvar_tag_address     (int offset = 0) {
    GUARANTEE(TaggedJavaStack, "Should not call this");
    return stackvar_address(offset - sizeof(jint));
  }

  Address stackvar_address_high    (int offset = 0) { 
    return Address(esp, Constant(JavaFrame::arg_offset_from_sp(1) + offset)); 
  }

  Address stackvar_tag_address_high(int offset = 0) { 
    GUARANTEE(TaggedJavaStack, "Should not call this");
    return stackvar_address_high(offset - sizeof(jint));
  }

  Address stackvar_address_low     (int offset = 0) { 
    return Address(esp, Constant(JavaFrame::arg_offset_from_sp(0) + offset)); 
  }

  Address stackvar_tag_address_low (int offset = 0) { 
    GUARANTEE(TaggedJavaStack, "Should not call this");
    return stackvar_address_low(offset - sizeof(jint));
  }

  // Support for FPU stack operations.

  void push_from_fpu_stack(Tag tag, int offset, bool set_tags = true);
  void pop_to_fpu_stack(Tag tag, int& offset);
  void clear_one_from_fpu_stack(Tag tag, int offset);

  // Support for incrementing local integers.
  void increment_local_int(const Register index, const Register src);

  // Support for reading local return addresses (used for ret).
  void get_local_ret(const Register index, const Register dst);

  // Support for getting the current thread.
  void get_thread(const Register thread);
  void get_thread_handle(const Register thread);

#if ENABLE_ISOLATES
  void get_current_task(const Register &m);
  void get_task_class_init_marker(const Register &m);
  void get_clinit_list(const Register clinit_list);
  void get_mirror_from_clinit_list(const Register &tm, const Register &klass);
#endif // ENABLE_ISOLATES

  // Call from interpreter
  void call_from_interpreter(const Register routine, int offset = 0);
  void call_from_interpreter(const Constant& routine);

  void save_interpreter_state();
  void restore_interpreter_state();

  // Support for incrementing bytecode/pair counters.
  void increment_bytecode_counter(Bytecodes::Code bc);
  void increment_pair_counter(Bytecodes::Code bc);

  // Support for calling native functions.
  void interpreter_call_native (const Register native,
                                BasicType return_value_type);
  void trace_native_call(const Register tmp);
  void wtk_profile_quick_call(int param_size = -1);

  // Support for calling the virtual machine runtime system. The
  // redo version restarts the current bytecode upon return.
  void interpreter_call_vm     (const Constant& routine,
                                BasicType return_value_type);
  void interpreter_call_vm_redo(const Constant& routine);

  // Check for timer tick
  void check_timer_tick();

  // Dispatch operations.
  void dispatch_prologue(int step);
  void dispatch_epilogue(int step);
  void dispatch_next(int step = 0);

  // Unlock/remove the current activation.
  void unlock_activation (bool is_unwinding);
  void remove_activation (const Register return_address);

  // Write barrier operation. The dst register is changed if the preserve_dst
  // is false, otherwise we preserve it.
  void oop_write_barrier(const Register dst, const Register tmp,
                         bool preserve_dst = false,
                         bool check_heap_top = false);
  void load_class_from_list(const Register dst, const Register index,
                            const Register tmp);

  // special case where dst == index
  void load_class_from_list(const Register dst, const Register tmp) {
    load_class_from_list(dst, dst, tmp);
  }

#if ENABLE_ISOLATES
  void get_task_mirror_list(const Register dst);
  void load_task_mirror_from_list(const Register dst, const Register index, 
                                  const Register tmp);
  void load_task_mirror_from_list(const Register dst, const Register tmp) {
          load_task_mirror_from_list(dst, dst, tmp);
  }
  void cib_with_marker(const Register task_mirror,
                       Label class_is_initialized);
#else
  void initialize_class_when_needed(const Register dst,
                                    const Register tmp1, const Register tmp2,
                                    Label restart);
#endif

#if ENABLE_INTERPRETATION_LOG
  void update_interpretation_log();
#else
  void update_interpretation_log() {}
#endif

 protected:
   // the maximum array length that is safe to allocate
   // without having to worry about wrap-arounds.
   enum {
     maximum_safe_array_length = 0x000fffff
   };

  void call_shared_call_vm(BasicType return_value_type);
  void goto_shared_call_vm(BasicType return_value_type);
};

#endif // PRODUCT
