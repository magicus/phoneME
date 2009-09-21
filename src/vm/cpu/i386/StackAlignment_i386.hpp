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

#if ENABLE_STACK_ALIGNMENT

#if (STACK_ALIGNMENT_VALUE != 16)
#error "Only 16 byte stack alignment supported currently"
#endif

/**
 * Calculates, how many bytes to subtract from the stack pointer to make it 
 * properly aligned at the next call.
 *
 * @param OFFSET the number of bytes pushed on the stack from the last known
 *      aligned SP value
 * @param RESERVE the number of bytes which will be pushed on the stack before 
 *      the call (parameters)
 */
#define STACK_ALIGNMENT_FIX(OFFSET, RESERVE) \
    ((((OFFSET) + (RESERVE) + 15) & ~15) - (OFFSET) - (RESERVE))

/**
 * Subtracts from the stack pointer the minimal number of bytes to make it 
 * aligned at the next call.
 *
 * @param OFFSET the number of bytes pushed on the stack from the last known
 *      aligned SP value
 * @param RESERVE the number of bytes which will be pushed on the stack before 
 *      the call (parameters)
 */
#define APPLY_STACK_ALIGNMENT_FIX(OFFSET, RESERVE)                      \
  do {                                                                  \
    if (STACK_ALIGNMENT_FIX((OFFSET), (RESERVE)) > 0) {                 \
      subl(esp, Constant(STACK_ALIGNMENT_FIX((OFFSET), (RESERVE))));    \
    }                                                                   \
  } while (0);

/**
 * Reverts the stack alignment fix applied to SP. When called after the call
 * for which <code>APPLY_STACK_ALIGNMENT_FIX</code> was used, it will put the
 * stack to the state in which it was before applying the fix (removes the 
 * fix and the <code>RESERVE</code> bytes).
 *
 * @param OFFSET the number of bytes pushed on the stack from the last known
 *      aligned SP value
 * @param RESERVE the number of bytes which will be pushed on the stack before 
 *      the call (parameters)
 */
#define REVERT_STACK_ALIGNMENT_FIX(OFFSET, RESERVE)                     \
  do {                                                                  \
    if ((STACK_ALIGNMENT_FIX((OFFSET), (RESERVE)) + (RESERVE)) > 0) {   \
      addl(esp, Constant(STACK_ALIGNMENT_FIX((OFFSET), (RESERVE))       \
                             + (RESERVE)));                             \
    }                                                                   \
  } while (0);

/**
 * Generates native 2-parameter call with properly aligned stack. Used
 * when the last known aligned stack point is unknown and so the
 * <code>APPLY_STACK_ALIGNMENT_FIX</code> macro can't be used.
 *
 * @param FN_NAME the name of the function to call
 * @param PAR1 the first parameter of the call
 * @param PAR2 the second parameter of the call
 */ 
#define ALIGNED_CALL_2(FN_NAME, PAR1, PAR2)                 \
  do {                                                      \
    pushl(ebp);                                             \
    movl(ebp, esp);                                         \
    subl(esp, Constant(2 * BytesPerWord));                  \
    andl(esp, Constant(-16));                               \
                                                            \
    movl(Address(esp), PAR1);                               \
    movl(Address(esp, Constant(BytesPerWord)), PAR2);       \
    call(Constant(FN_NAME));                                \
                                                            \
    movl(esp, ebp);                                         \
    popl(ebp);                                              \
  } while (0)

/**
 * Generates native 4-parameter call with properly aligned stack. Used
 * when the last known aligned stack point is unknown and so the
 * <code>APPLY_STACK_ALIGNMENT_FIX</code> macro can't be used.
 *
 * @param FN_NAME the name of the function to call
 * @param PAR1 the first parameter of the call
 * @param PAR2 the second parameter of the call
 * @param PAR3 the third parameter of the call
 * @param PAR4 the fourth parameter of the call
 */ 
#define ALIGNED_CALL_4(FN_NAME, PAR1, PAR2, PAR3, PAR4)     \
  do {                                                      \
    pushl(ebp);                                             \
    movl(ebp, esp);                                         \
    subl(esp, Constant(4 * BytesPerWord));                  \
    andl(esp, Constant(-16));                               \
                                                            \
    movl(Address(esp), PAR1);                               \
    movl(Address(esp, Constant(BytesPerWord)), PAR2);       \
    movl(Address(esp, Constant(2 * BytesPerWord)), PAR3);   \
    movl(Address(esp, Constant(3 * BytesPerWord)), PAR4);   \
    call(Constant(FN_NAME));                                \
                                                            \
    movl(esp, ebp);                                         \
    popl(ebp);                                              \
  } while (0)

#else // ENABLE_STACK_ALIGNMENT

#define APPLY_STACK_ALIGNMENT_FIX(OFFSET, RESERVE)

#define REVERT_STACK_ALIGNMENT_FIX(OFFSET, RESERVE)                     \
  do {                                                                  \
    if ((RESERVE) > 0) {                                                \
      addl(esp, Constant((RESERVE)));                                   \
    }                                                                   \
  } while (0);

#define ALIGNED_CALL_2(FN_NAME, PAR1, PAR2)                 \
  do {                                                      \
    pushl(PAR2);                                            \
    pushl(PAR1);                                            \
    call(Constant(FN_NAME));                                \
    addl(esp, Constant(2 * BytesPerWord));                  \
  } while (0)

#define ALIGNED_CALL_4(FN_NAME, PAR1, PAR2, PAR3, PAR4)     \
  do {                                                      \
    pushl(PAR4);                                            \
    pushl(PAR3);                                            \
    pushl(PAR2);                                            \
    pushl(PAR1);                                            \
    call(Constant(FN_NAME));                                \
    addl(esp, Constant(4 * BytesPerWord));                  \
  } while (0)

#endif // ENABLE_STACK_ALIGNMENT

