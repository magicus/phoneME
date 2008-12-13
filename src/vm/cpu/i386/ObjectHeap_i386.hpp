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

#if !USE_GENERIC_BIT_SETTING_FUNCS
// The methods have been optimized for performance; machine
// independent C++ code is in comments

// bit set helpers for machine independent C++ code

// This implementation takes advantage of the Intel bit test instructions.
// Don't use EBX, ESI or EDI.

inline unsigned int* word_for(int offset, unsigned int* map) { 
  return map + (offset/BitsPerWord); 
}

inline void bit_clear(int offset, unsigned int* map) {
  intptr_t* w = (intptr_t*)word_for(offset, map);
  int pos = offset%BitsPerWord;
  clear_nth_bit(*w, pos);
}

inline void bit_set(int offset, unsigned int* map) {
  intptr_t* w = (intptr_t*)word_for(offset, map);
  int pos = offset%BitsPerWord;
  set_nth_bit(*w, pos);
}

inline bool is_bit_set(int offset, unsigned int* map) {
  return is_set_nth_bit(*word_for(offset, map), offset%BitsPerWord); 
}

inline void ObjectHeap::set_bit_for(OopDesc** p, 
                                    address bitvector_base) {
  GUARANTEE(contains(p), "Should be in object heap");
#if defined(NO_INLINE_ASSEMBLER) || defined(__GNUC__)
   unsigned int t = (unsigned int) p >> LogBytesPerWord;
   bit_set(t, (unsigned int *)bitvector_base);
#else

#ifdef __GNUC__
  // IMPL_NOTE: get this GCC inline asm to work again!
  asm volatile (
       "shrl $2, %0;"
       "movl %1, %%ecx;"
       "btsl %0, (%%ecx);"
        :
        : "r" (p), "r" (bitvector_base)
        : "ecx"
        );
#else
  __asm {
    mov eax, p
    mov ecx, bitvector_base
    shr eax, 2 // LogBytesPerWord
    bts [ecx], eax
  }
#endif

#endif
}

// Consider using word stores here, depends on the average length
inline void ObjectHeap::set_bit_range(OopDesc** start, int len) {
  GUARANTEE(contains(start), "Should be in object heap");

#if defined(NO_INLINE_ASSEMBLER) || defined(__GNUC__)
  unsigned int t = (unsigned int)start >> LogBytesPerWord;
  for (int i = 0; i < len; i++, t++) {
    bit_set(t, (unsigned int*)_bitvector_base);
  }

  // This piece of GCC inline-asm doesn't work if set_bit_range() is called multiple
  // times inside the same function.
  //
  //asm volatile ("movl %1, %%ecx;"
  //     "shrl $2, %0;"
  //     "movl _bitvector_base, %%edx;"
  //     "btsl %0, (%%edx);"
  //     "incl %0;"
  //     "decl %%ecx;"
  //     "jecxz .-5;"
  //      :
  //    : "r" (start), "r" (len)
  //    : "ecx", "edx"
  //    );

#else
  __asm {
    mov eax, start
    mov ecx, len
    mov edx, [_bitvector_base]
    shr eax, 2 // LogBytesPerWord
  set_next: 
    bts [edx], eax
    inc eax
    loop set_next  // decrements ecx
  }
#endif
}

inline void ObjectHeap::clear_bit_for(OopDesc** p, address bitvector_base) {
  GUARANTEE(contains(p), "Should be in object heap");
#if defined(NO_INLINE_ASSEMBLER) || defined(__GNUC__)
  unsigned int t = (unsigned int)p >> LogBytesPerWord;
  bit_clear(t, (unsigned int*)bitvector_base);
#else
#ifdef __GNUC__
  // IMPL_NOTE: get this GCC inline asm to work again!
  int dummy;
  asm volatile ("shrl $2, %0;"
       "movl bitvector_base, %%ecx;"
       "btrl %0, (%%ecx);"
        : "=r" (dummy)
        : "0" (p)
        : "ecx"
        );
#else
  __asm {
    mov eax, p
    mov ecx, bitvector_base
    shr eax, 2 // LogBytesPerWord
    btr [ecx], eax
  }
#endif

#endif
}

#pragma warning(disable: 4035) // Disable warning 4035: no return value

bool ObjectHeap::test_bit_for(OopDesc** p, address bitvector_base) {
  GUARANTEE(contains(p), "Should be in object heap");
#if defined(NO_INLINE_ASSEMBLER) || defined(__GNUC__)
   unsigned int t = (unsigned int)p >> LogBytesPerWord;
   return is_bit_set(t, (unsigned int*)bitvector_base);
#else

#ifdef __GNUC__
  // IMPL_NOTE: get this GCC inline asm to work again!
  bool res;
  asm volatile (
       "movl %1, %0;"
       "shrl $2, %0;"
       "movl %2, %%ecx;"
       "xorl %%eax, %%eax;"
       "btl %0, (%%ecx);"
       "adcl $0, %%eax;"
       "movl %%eax, %0;"
        : "=r" (res)
        : "r" (p), "r" (bitvector_base)
        : "eax", "ecx"
        );
  return (res);
#else
  __asm {
    mov edx, p
    mov ecx, bitvector_base
    shr edx, 2 // LogBytesPerWord
    xor eax, eax
    bt [ecx], edx
    adc eax, 0
  }
#endif
  // no return statement needed, result is already in eax
  // compiler warning C4035 disabled via warning pragma
#endif
}

inline bool ObjectHeap::test_and_set_bit_for(OopDesc** p, 
                                             address bitvector_base) {
  GUARANTEE(contains(p), "Should be in object heap");
#if defined(NO_INLINE_ASSEMBLER) || defined(__GNUC__)
  unsigned int t = (unsigned int)p >> LogBytesPerWord;
  bool res = is_bit_set(t, (unsigned int*)bitvector_base);
  bit_set(t, (unsigned int*)bitvector_base);
  return res;
#else

#ifdef __GNUC__
  // IMPL_NOTE: get this GCC inline asm to work again!
  bool res;
  asm volatile (
       "movl %1, %0;"
       "shrl $2, %0;"
       "movl bitvector_base, %%ecx;"
       "xorl %%eax, %%eax;"
       "btsl %0, (%%ecx);"
       "adcl $0, %%eax;"
       "movl %%eax, %0;"
        : "=r" (res)
        : "r" (p)
        : "ecx", "eax"
        );
  return (res);
#else
  __asm {
    mov edx, p
    mov ecx, bitvector_base
    shr edx, 2 // LogBytesPerWord
    xor eax, eax
    bts [ecx], edx
    adc eax, 0
  }
#endif
  // no return statement needed, result is already in eax
  // compiler warning C4035 disabled via warning pragma
#endif
}

#pragma warning(default: 4035)
#endif /* !USE_GENERIC_BIT_SETTING_FUNCS */
