/*
 * @(#)invokeNative_sparc.S	1.25 06/10/10
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
 *
 */

#include "javavm/include/asmmacros_cpu.h"

	.text
	.align	4
!
! SUBROUTINE CVMjniInvokeNative
!
! This function translates the "Java" calling convention into the "C"
! calling convention used in native methods. Java VM passes all the 
! arguments in the Java stack, and expects the results to be placed there 
! as well. We therefore have to copy the arguments into the C stack (or 
! registers), and place the return values back into the Java stack.
!
! With a small sacrifise in efficiency, this approach avoids having to
! generate a stub function for every native methods.
!
! See "The SPARC Architecture Manual" for information on native 
! argument-passing conventions.
!
! Since the SPARC does not pass DOUBLEs in floating-point registers, and 
! does not have double-word alignment requirement for DOUBLEs and LONGs, 
! we do not have to parse the method descriptor to obtain the exact type 
! information for each argument. 
!
! The first argument to CVMjniInvokeNative is a pointer to the JNI
! environment, which should be passed unmodified as the first argument
! to the native method.
!
! The second argument is a pointer to the "real" native method function.
!
! The third argument (stk) is a pointer to the Java stack, where all
! the arguments are stored (as stk[0], stk[1], etc.).
!
! The fourth argument is the terse signature of the native method.
!
! The fifth argument is the total size (in 32-bit words) of the
! arguments on the Java stack. Note that the Java stack does not have
! any alignment requirement, and stores all arguments consecutively in
! words and double words. The argument size includes the "this" pointer
! for non-static methods.
!
! The sixth argument is 0 for non-static methods, or a jclass
! for static methods. Non-static native methods receive an object
! reference as the second argument (which is simply the address of
! stk[0]). The "real" method arguments to non-static methods begin at
! stk[1]. Static native methods receive a class reference as the second
! argument.
!
! The return value of the native method is placed at stk[0] for
! word-sized results, or at stk[0] and stk[1] for
! double-word-sized results. The return value of CVMjniInvokeNative is
! 0 if the native method returns void, 1 if the native
! method returns a word, 2 if the native method returns a
! double word, or -1 if the native method returns an object.

! Arguments:
!
! i0	   JNI environment
! i1	   native function
! i2	   Java stack top
! i3	   method descriptor
! i4	   number of argument words to be passed to native function
! i5	   class for static methods, or 0 for non-static methods
! [%fp+92] result pointer
!
! Local registers:
! l0
! l1	   return type syllable
! l2	   sig word buffer
!
! l4       dispatch base for arg/return type switch jump.
!
! Results:	
! o0	return value word count or -1 for object
!

#define SIGPTR %i3
#define SIGBYTE %g2
#define JSTKPTR %g3
#define CSTKPTR %g4	

#define RETTYPE %l1
#define SIGBUFF  %l2
#define SWITCHBASE %l4

#define TYPEMASK 0xf
#define TYPESHIFT 4
	       
ENTRY(CVMjniInvokeNative)

! Set up a big enough stack frame (and make sure it is 8-byte aligned):

	subcc	%o4,4,%g1	! We can freely use %g1.
	bpos	more_than_6_args
	nop
	mov	%g0,%g1

more_than_6_args:
	add	%g1,1,%g1
	sra	%g1,1,%g1
        sll	%g1,3,%g1
	sub	%g0,%g1,%g1	! negate %g1
        sub	%g1,96,%g1
	save	%sp,%g1,%sp

! transferring arguments
!
! first 6 in register

! The first argument is always the JNI env.

	mov	%i0,%o0

! The next one is %i2 or %i5 if it is non-zero (for static methods).

	mov	%i2,JSTKPTR	! keep a backup for %i2.

	ld	[SIGPTR], SIGBUFF	! preload signature
	add	%sp,76,CSTKPTR		! l0 points to 3rd argument ???
	inc	4,SIGPTR
	and	SIGBUFF,TYPEMASK,RETTYPE ! stash return type for later use.
	srl	SIGBUFF,TYPESHIFT,SIGBUFF

	set	arg_jumps,SWITCHBASE	! load the base of the jump table

        orcc	%i5,0,%i5
	be	non_static
        nop
	ba	args_loop
	mov	%i5,%o1

non_static:
	mov	JSTKPTR,%o1
        add     JSTKPTR,4,JSTKPTR

args_loop:
	and	SIGBUFF,TYPEMASK,SIGBYTE
	sll	SIGBYTE,2,SIGBYTE
	ld	[SWITCHBASE + SIGBYTE],SIGBYTE
	jmp     SIGBYTE
	srl	SIGBUFF,TYPESHIFT,SIGBUFF

arg_32:			! move a 32-bit value from [JSTKPTR] to [CSTKPTR].
	ld	[JSTKPTR],%o2
	and	SIGBUFF,TYPEMASK,SIGBYTE
	add	JSTKPTR,4,JSTKPTR
	st	%o2,[CSTKPTR]
	sll	SIGBYTE,2,SIGBYTE
	ld	[SWITCHBASE + SIGBYTE],SIGBYTE
	add	CSTKPTR,4,CSTKPTR	
	jmp     SIGBYTE
	srl	SIGBUFF,TYPESHIFT,SIGBUFF

arg_64:
	ld	[JSTKPTR],%o2
	and	SIGBUFF,TYPEMASK,SIGBYTE
	ld	[JSTKPTR+4],%o3
	add	JSTKPTR,8,JSTKPTR
	st	%o2,[CSTKPTR]
	sll	SIGBYTE,2,SIGBYTE
	st	%o3,[CSTKPTR+4]
	add	CSTKPTR,8,CSTKPTR
	ld	[SWITCHBASE + SIGBYTE],SIGBYTE
	jmp     SIGBYTE
	srl	SIGBUFF,TYPESHIFT,SIGBUFF
		
arg_object:
	ld	[JSTKPTR],%o2
	cmp	%o2,0
	be	object_checked
	and	SIGBUFF,TYPEMASK,SIGBYTE ! always executed
	mov	JSTKPTR,%o2
object_checked:

	add	JSTKPTR,4,JSTKPTR
	st	%o2,[CSTKPTR]
	add	CSTKPTR,4,CSTKPTR	
	sll	SIGBYTE,2,SIGBYTE
	ld	[SWITCHBASE + SIGBYTE],SIGBYTE
	jmp     SIGBYTE
	srl	SIGBUFF,TYPESHIFT,SIGBUFF

arg_reload:
	! get another word full of types
	! then re-dispatch
	! since most signatures are short, this does not happen
	! very often.
	ld	[SIGPTR], SIGBUFF	! preload signature
	inc	4,SIGPTR
	and	SIGBUFF,TYPEMASK,SIGBYTE
	sll	SIGBYTE,2,SIGBYTE
	ld	[SWITCHBASE + SIGBYTE],SIGBYTE
	jmp     SIGBYTE
	srl	SIGBUFF,TYPESHIFT,SIGBUFF

args_done:
	add	%sp,76,CSTKPTR
	ld	[CSTKPTR],%o2
	ld	[CSTKPTR+4],%o3
	ld	[CSTKPTR+8],%o4
	ld	[CSTKPTR+12],%o5	

	set	ret_jumps,SWITCHBASE
	
	jmpl	%i1,%o7
	sll	RETTYPE,2,RETTYPE
	
	ld	[%fp+92],%i2		! pointer to result buffer

	ld	[SWITCHBASE + RETTYPE],SIGBYTE	! thread the return address to the
	jmp	SIGBYTE			! proper code for our return type
	nop

ret_obj:
	st	%o0,[%i2]
	ret
	restore %g0,-1,%o0		! -1 indicates object return
	
ret_f64:	
#ifdef ALIGNED_DOUBLES
	std     %f0,[%i2]
#else
	st      %f0,[%i2]
        st      %f1,[%i2+4]
#endif
        ret
	restore %g0,2,%o0		! 2 indicates double-word return

ret_f32:
	st      %f0,[%i2]
	ret
	restore %g0,1,%o0		! 1 indicates single-word return

ret_s32:	
	st	%o0,[%i2]
	ret
	restore %g0,1,%o0		! 1 indicates single-word return

ret_s64:
#ifdef ALIGNED_LONG_LONGS
	std	%o0,[%i2]
#else
	st	%o0,[%i2]
	st	%o1,[%i2+4]
#endif
	ret
	restore %g0,2,%o0		! 2 indicates double-word return

ret_s8:
#ifdef JDK12
	sll	%o0,24,%o0
	sra	%o0,24,%o0
#endif
	st	%o0,[%i2]
	ret
	restore %g0,1,%o0		! 1 indicates single-word return

ret_u8:
#ifdef JDK12
	sll	%o0,24,%o0
	srl	%o0,24,%o0
#endif
	st	%o0,[%i2]
	ret
	restore %g0,1,%o0		! 1 indicates single-word return

ret_s16:
#ifdef JDK12
	sll	%o0,16,%o0
	sra	%o0,16,%o0
#endif
	st	%o0,[%i2]
	ret
	restore %g0,1,%o0		! 1 indicates single-word return
	
ret_u16:
#ifdef JDK12
	sll	%o0,16,%o0
	srl	%o0,16,%o0
#endif
	st	%o0,[%i2]
	ret
	restore %g0,1,%o0		! 1 indicates single-word return

ret_void:
        ret
	restore %g0,0,%o0		! 0 indicates void return

ret_jumps:		
	.word	ret_void	! error
	.word	ret_void	! ENDFUNC should not get called
	.word	ret_void! void
	.word	ret_s32	! int
	.word	ret_s16	! short
	.word	ret_u16	! char
	.word	ret_s64	! long
	.word	ret_s8	! byte
	.word	ret_f32	! float
	.word	ret_f64	! double
	.word	ret_u8	! bool
	.word	ret_obj
	.word	ret_void	! this is invalid and should not get called

arg_jumps:
	.word	arg_reload	! no more data this word: go get more
	.word	args_done 	! end-of-args
	.word	ret_void	! this is invalid and should not get called
	.word	arg_32	! int
	.word	arg_32	! short
	.word	arg_32	! char
	.word	arg_64	! long
	.word	arg_32	! byte
	.word	arg_32	! float
	.word	arg_64	! double
	.word	arg_32	! bool
	.word	arg_object
	.word	ret_void	! this is invalid and should not get called

SET_SIZE(CVMjniInvokeNative)
