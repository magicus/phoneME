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

// The methods have been optimized for performance; machine
// independent C++ code is in comments

#ifndef BYTES_I386_HPP
#define BYTES_I386_HPP
class Bytes: AllStatic {
 public:
  // Returns true if the byte ordering used by Java is different from
  // the native byte ordering of the underlying machine. For example,
  // this is true for Intel x86, but false for Solaris on Sparc.
  static inline bool is_Java_byte_ordering_different() { return true; }

  // Efficient reading and writing of unaligned unsigned data in
  // platform-specific byte ordering (no special code is needed since
  // x86 CPUs can access unaligned data)
  static inline jushort get_native_u2(const_address p) { return *(jushort*)p; }
  static inline juint   get_native_u4(const_address p) { return *(juint*)p; }

  static inline void put_native_u2(address p, jushort x) { *(jushort*)p = x; }
  static inline void put_native_u4(address p, juint x)   { *(juint*)p = x; }

  // Efficient reading and writing of unaligned unsigned data in Java
  // byte ordering (i.e. big-endian ordering). Byte-order reversal is
  // needed since x86 CPUs use little-endian format.
  static inline jushort get_Java_u2(const_address const p) {
    return swap_u2(get_native_u2(p));
  }
  static inline juint get_Java_u4(const_address const p) {
    return swap_u4(get_native_u4(p));
  }

  static inline julong get_Java_u8(const_address p) {
      return ((julong)get_Java_u4(p) << 32 | (julong)get_Java_u4(p + 4));
  }

  static inline void put_Java_u2(address p, jushort x) {
    put_native_u2(p, swap_u2(x));
  }
  static inline void put_Java_u4(address p, juint x) {
    put_native_u4(p, swap_u4(x));
  }

  static inline void put_Java_u8(address p, julong x) {
    put_Java_u4(p, (juint)(x >> 32));
    put_Java_u4(p + 4, (juint)(x & 0xFFFFFFFFL));
  }

  // Efficient swapping of byte ordering
  static inline jushort swap_u2(jushort x);   // C++ compiler-dependent
  static inline juint   swap_u4(juint x);     // C++ compiler-dependent
};

#pragma warning(disable: 4035) // Disable warning 4035: no return value

// Efficient swapping of data bytes from Java byte
// ordering to native byte ordering and vice versa.
inline jushort Bytes::swap_u2(jushort x) {
#ifdef NO_INLINE_ASSEMBLER
   return (x & 0x00ff) << 8 
        | (x & 0xff00) >> 8;
#else

#ifdef __GNUC__
   jushort res;
   asm volatile ("movw    %1, %%ax;"
       "xchgb   %%ah, %%al;"
       "movw    %%ax, %0;"
        : "=r" (res)
        : "r" (x)
        : "eax"
        );
   return (res);
#else
  __asm {
    mov ax, x
    xchg al, ah
  }
#endif
  // no return statement needed, result is already in ax
  // compiler warning C4035 disabled via warning pragma
#endif
}

inline juint Bytes::swap_u4(juint x) {
#ifdef NO_INLINE_ASSEMBLER
   juint l = swap_u2(x         & 0x0000ffff);
   juint h = swap_u2((x >> 16) & 0x0000ffff );
   juint res = (l << 16) | h;
   return res;
#else

#ifdef __GNUC__
  juint res;
  asm volatile ("movl %1, %0;\n"
       "bswapl %0"
      : "=r" (res)
      : "r" (x)
      );
  return (res);
#else
  __asm {
    mov eax, x
    bswap eax
  }
#endif
  // no return statement needed, result is already in eax
  // compiler warning C4035 disabled via warning pragma
#endif
}

#pragma warning(default: 4035) // Enable warning 4035: no return value

#endif
