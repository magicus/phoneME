/*
 * @(#)invokeNative_sarm.S	1.6 06/10/10
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


;	.align	4
;
; SUBROUTINE CVMjniInvokeNative
;
; This function translates the "Java" calling convention into the "C"
; calling convention used in native methods. Java VM passes all the 
; arguments in the Java stack, and expects the results to be placed there 
; as well. We therefore have to copy the arguments into the C stack (or 
; registers), and place the return values back into the Java stack.
;
; With a small sacrifise in efficiency, this approach avoids having to
; generate a stub function for every native methods.
;
; The most widely accepted StrongARM SA-100 programming model has
; arguments passed to an invoked subroutine in registers a1-a4, with
; any further arguments passed on the stack.
;
; The first argument to CVMjniInvokeNative is a pointer to the JNI
; environment, which should be passed unmodified as the first argument
; to the native method.
;
; The second argument is a pointer to the "real" native method function.
;
; The third argument (stk) is a pointer to the Java stack, where all
; the arguments are stored (as stk[0], stk[1], etc.).
;
; The fourth argument is the terse signature of the native method.
;
; The fifth argument is the total size (in 32-bit words) of the
; arguments on the Java stack. Note that the Java stack does not have
; any alignment requirement, and stores all arguments consecutively in
; words and double words. The argument size includes the "this" pointer
; for non-static methods.
;
; The sixth argument is 0 for non-static methods, or a jclass
; for static methods. Non-static native methods receive an object
; reference as the second argument (which is simply the address of
; stk[0]). The "real" method arguments to non-static methods begin at
; stk[1]. Static native methods receive a class reference as the second
; argument.
;
; The return value of the native method is placed at stk[0] for
; word-sized results, or at stk[0] and stk[1] for
; double-word-sized results. The return value of CVMjniInvokeNative is
; 0 if the native method returns void, 1 if the native
; method returns a word, 2 if the native method returns a
; double word, or -1 if the native method returns an object.

;	.global CVMjniInvokeNative
; Arguments:
;
; a1	   JNI environment
; a2	   native function
; a3	   Java stack top
; a4	   method descriptor
; stk1	   number of argument words to be passed to native function
; stk2	   class for static methods, or 0 for non-static methods
; stk3     result pointer
;
; Local registers:
; l0
; l1	   return type syllable
; l2	   sig word buffer
;
; l4       dispatch base for arg/return type switch jump.
;
; Results:	
; a1	return value word count or -1 for object
;

;#define SIGPTR	a4
;#define SIGBYTE v3
;#define JSTKPTR v1
;#define CSTKPTR v7

;#define RETTYPE v6
;#define SIGBUFF  v5
;#define SWITCHBASE v2

;#define FuncPtr	ip

;#define TYPEMASK #0xf
;#define TYPESHIFT #4

SAVESET RLIST {v1-v7,fp,ip,lr}
RSTRSET	RLIST {v1-v7,fp,sp,pc}

	MACRO
	RETURN
	LDMDB	fp, RSTRSET
	MEND

	INCLUDE kxarm.h

	TTL	invokeNative_sarm.S

	NESTED_ENTRY CVMjniInvokeNative

    ; Set up registers for processing the signature, etc.

	mov	ip, sp
	STMFD   sp!, SAVESET
	mov	fp, ip

	^ 0,fp
stk1	# 4
stk2	# 4
stk3	# 4

	PROLOG_END

; transferring arguments
;
; first 4 in register

; The first argument (a1) is already the JNI env.

; The next one is a3 or stk2 if it's non-zero (for static methods).

	mov	ip,a2
	mov	v1,a3	; keep a backup for a3

	ldr	v5, [a4], #4	; preload signature
	and	v6,v5,#0xf ; stash return type for later use.
	mov	v5,v5, lsr #4

	adr	v2,arg_jumps	; load the base of the jump table

        ldr	a2, stk1
	mov	a2, a2, asl #2
	sub	sp, sp, a2
	mov	v7,sp

        ldr	a2, stk2
	cmp	a2, #0
	moveq	a2, v1
	addeq	v1, v1, #4

args_loop
	and	v3,v5,#0xf
	mov	v5,v5, lsr #4
	ldr	pc, [v2, v3, lsl #2]

arg_32			; move a 32-bit value from [v1] to [v7].
	ldr	a3,[v1], #4
	and	v3,v5,#0xf
	str	a3,[v7], #4

	mov	v5,v5, lsr #4
	ldr	pc, [v2, v3, lsl #2]

arg_64
	LDMIA	v1!, {a3,v3}
	stmia	v7!, {a3,v3}
	and	v3,v5,#0xf
	mov	v5, v5, lsr #4
	ldr	pc, [v2, v3, lsl #2]
		
arg_object
	ldr	a3,[v1],#4
	cmp	a3,#0
	and	v3,v5,#0xf
	beq	object_checked
	sub	a3,v1,#4

object_checked

	str	a3,[v7], #4
	mov	v5,v5, lsr #4
	ldr	pc, [v2, v3, lsl #2]

arg_reload
	; get another word full of types
	; then re-dispatch
	; since most signatures are short, this doesn't happen
	; very often.
	ldr	v5, [a4], #4	; preload signature
	and	v3,v5,#0xf

	mov	v5,v5, lsr #4
	ldr	pc, [v2, v3, lsl #2]

args_done
    ; The ARM procedure call standard we are using specifies that the first
    ; four arguments are passed in registers a1-a4.  a1 and a2 are both set.
    ; Now we just load up a3 and a4 with the first 2 arg method arguments.
    ; If there are no such args, this doesn't hurt anything.

	ldr	a3,[sp, #0]
	ldr	a4,[sp, #4]

;#define TMP v7

	ldr	v7, stk1
	cmp	v7,#2
	addge	sp, sp, #8

	adr	v2, ret_jumps
	
	MOV	lr, pc
	MOV	pc, ip			; call JNI function
	
	ldr	a4, stk3		; pointer to result buffer

	; thread the return address to the
	; proper code for our return type
	ldr	pc,[v2, v6, lsl #2]

ret_obj
	str	a1,[a4]
	mov	a1,#-1	; -1 indicates object return
	RETURN
	
ret_s32
	str	a1,[a4]
	mov	a1,#1	; 1 indicates single-word return
	RETURN

ret_s64
	stmia	a4, {a1,a2}
	mov	a1,#2	; 2 indicates double-word return
	RETURN

ret_f32	
	str	a1,[a4] ;fsts	f0,[a4]
	mov	a1,#1	; 1 indicates single-word return
	RETURN


ret_f64	
	stmia	a4, {a1,a2}   ;fstd	f0,[a4]
	mov	a1,#2	; 2 indicates double-word return
	RETURN

ret_void
	mov	a1,#0	; 0 indicates void return
	RETURN

	ENTRY_END

ret_jumps		
	DCD	ret_void	; error
	DCD	ret_void	; ENDFUNC shouldn't get called
	DCD	ret_void	; void
	DCD	ret_s32		; int
	DCD	ret_s32		; short
	DCD	ret_s32		; char
	DCD	ret_s64		; long
	DCD	ret_s32		; byte
	DCD	ret_s32		; float
	DCD	ret_s64		; double
	DCD	ret_s32		; bool
	DCD	ret_obj
	DCD	ret_void	; this is invalid and shouldn't get called

arg_jumps
	DCD	arg_reload	; no more data this word: go get more
	DCD	args_done 	; end-of-args
	DCD	ret_void	; this is invalid and shouldn't get called
	DCD	arg_32		; int
	DCD	arg_32		; short
	DCD	arg_32		; char
	DCD	arg_64		; long
	DCD	arg_32		; byte
	DCD	arg_32		; float
	DCD	arg_64		; double
	DCD	arg_32		; bool
	DCD	arg_object
	DCD	ret_void	; this is invalid and shouldn't get called

	END
