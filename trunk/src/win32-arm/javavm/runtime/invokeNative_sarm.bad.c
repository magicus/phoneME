/*
 * @(#)invokeNative_sarm.bad.c	1.5 06/10/10
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
#include "javavm/include/defs.h"
#include "jni.h"

CVMInt32
CVMjniInvokeNative(void * env, void * f, CVMUint32 * stk,
		   CVMUint32 * sig, CVMInt32 sz,
		   void * cls,
		   CVMJNIReturnValue * res) 
{
_asm {
	mov	esi, stk
	mov	edx, sz
	mov	ecx, cls

	cmp	ecx, 0
	jne	static_call
	mov	ecx, esi
	add 	esi, 4
	jmp	SHORT static_done

static_call:
	add	edx, 1

static_done:
	add	edx, 1
        shl     edx, 2 		; word address -> byte address

	mov	sz, edx

	sub	esp, edx
	mov	edi, esp

	mov	eax, env
	mov	DWORD PTR [edi], eax
	add	edi, 4
	mov	DWORD PTR [edi], ecx
	add	edi, 4

	mov	ebx, sig
	xor	ecx, ecx	; zero it

	;; ebx is current index into sig
	;; cl (ecx) is current char in sig
	;; esi is index into java stack
	;; edx is (was) the # args, though we don't
	;; need it anymore.
args_loop:
;	args_again
	mov	edx, DWORD PTR[ebx]
	add	ebx, 4
	shr	edx, 4
;	;; args_again 
	mov	cl, dl
	and	ecx, 15
	shr	edx, 4
	jmp    arg_jumps


arg_reload: ;fetch more signature
	mov	edx, DWORD PTR [ebx]
	add	ebx, 4
	;; args_again
	mov	cl, dl
	and	ecx, 15
	shr	edx, 4
	jmp     arg_jumps


arg_32:
	mov	eax, DWORD PTR [esi]
	add	esi, 4
	mov	DWORD PTR [edi], eax
	add	edi, 4
	;;args_again
	mov	cl, dl
	and	ecx, 15
	shr	edx, 4
	jmp     arg_jumps
	

arg_64:
	mov	eax, DWORD PTR [esi]
	add	esi, 4
	mov	DWORD PTR [edi], eax
	add	edi, 4
	mov	eax, DWORD PTR [esi]
	add	esi, 4
	mov	DWORD PTR [edi], eax
	add	edi, 4
	;;args_again
	mov	cl, dl
	and	ecx, 15
	shr	edx, 4
	jmp     arg_jumps

arg_object:
	mov	eax, DWORD PTR [esi]
	cmp	eax, 0
	je	object_checked
	mov	eax, esi
object_checked:
	mov	DWORD PTR [edi], eax

	add	esi, 4
	add	edi, 4	
	;;args_again
	mov	cl, dl
	and	ecx, 15
	shr	edx, 4
	jmp     arg_jumps

args_done:

	call	f
	mov	ebx, sz
	add	esp, ebx

	mov	esi, res
	xor	ecx, ecx	; c register volatile cross calls - zero it
	mov	ebx, sig
	mov	cl, BYTE PTR [ebx]
	and	ecx, 15
;	jmp	DWORD PTR ret_jumps[ecx*4]
;ret_jumps:
;	DD	OFFSET FLAT:ret_void	; this is bogus and shouldn't get called
	cmp	ecx, 0
	je 	ret_void

;	DD	OFFSET FLAT:ret_void
	cmp	ecx, 1
	je 	ret_void

;	DD	OFFSET FLAT:ret_void
	cmp	ecx, 2
	je 	ret_void

;	DD	OFFSET FLAT:ret_s32
	cmp	ecx, 3
	je 	ret_s32

;	DD	OFFSET FLAT:ret_s16
	cmp	ecx, 4
	je 	ret_s16

;	DD	OFFSET FLAT:ret_u16
	cmp	ecx, 5
	je 	ret_u16

;	DD	OFFSET FLAT:ret_s64
	cmp	ecx, 6
	je 	ret_s64

;	DD	OFFSET FLAT:ret_s8
	cmp	ecx, 7
	je 	ret_s8

;	DD	OFFSET FLAT:ret_f32
	cmp	ecx, 8
	je 	ret_f32

;	DD	OFFSET FLAT:ret_f64
	cmp	ecx, 9
	je 	ret_f64

;	DD	OFFSET FLAT:ret_u8
	cmp	ecx, 10
	je 	ret_u8

;	DD	OFFSET FLAT:ret_obj
	cmp	ecx, 11
	je 	ret_obj

;	DD	OFFSET FLAT:ret_void
	jmp	ret_void
;;
;;
ret_obj:
	mov	DWORD PTR[esi], eax
	mov	eax, -1
	jmp	SHORT done

ret_f64:
	fstp	QWORD PTR [esi]
	mov	eax, 2
	jmp	SHORT done

ret_f32:
	fstp	DWORD PTR [esi]
	mov	eax, 1
	jmp	SHORT done

ret_s32:
	mov	DWORD PTR [esi], eax
	mov	eax, 1
	jmp	SHORT done

ret_s64:
	mov	DWORD PTR [esi], eax
	mov	eax, 2
	mov	DWORD PTR [esi+4], edx
	jmp	SHORT done

ret_s8:
	shl	eax, 24
	sar	eax, 24
	mov	DWORD PTR [esi], eax
	mov	eax, 1
	jmp	SHORT done

ret_u8:
	shl	eax, 24
	shr	eax, 24
	mov	DWORD PTR [esi], eax
	mov	eax, 1
	jmp	SHORT done

ret_s16:
	shl	eax, 16
	sar	eax, 16
	mov	DWORD PTR [esi], eax
	mov	eax, 1
	jmp	SHORT done

ret_u16:
	shl	eax, 16
	shr	eax, 16
	mov	DWORD PTR [esi], eax
	mov	eax, 1
	jmp	SHORT done

ret_void:
	mov	eax, 0

done:
	ret


; keep these offsets in sync with the enum in signature.h
arg_jumps:
;	DD	OFFSET FLAT:arg_reload	
	cmp	ecx, 0
	je 	arg_reload

;	DD	OFFSET FLAT:args_done
	cmp	ecx, 1
	je 	args_done

;	DD	OFFSET FLAT:ret_void
	cmp	ecx, 2
	je 	ret_void

;	DD	OFFSET FLAT:arg_32	;int
	cmp	ecx, 3
	je 	arg_32

;	DD	OFFSET FLAT:arg_32	; short
	cmp	ecx, 4
	je 	arg_32

;	DD	OFFSET FLAT:arg_32	; char
	cmp	ecx, 5
	je 	arg_32

;	DD	OFFSET FLAT:arg_64	; long
	cmp	ecx, 6
	je 	arg_64

;	DD	OFFSET FLAT:arg_32	; byte
	cmp	ecx, 7
	je 	arg_32

;	DD	OFFSET FLAT:arg_32	; float
	cmp	ecx, 8
	je 	arg_32

;	DD	OFFSET FLAT:arg_64	; double
	cmp	ecx, 9
	je 	arg_64

;	DD	OFFSET FLAT:arg_32	; bool
	cmp	ecx, 10
	je 	arg_32

;	DD	OFFSET FLAT:arg_object 
	cmp	ecx, 11
	je 	arg_object

;	DD	OFFSET FLAT:ret_void   
	jmp 	ret_void
}
}



