/*
 * @(#)invokeNative_i386.S	1.12 06/10/10
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
	ENTRY(CVMjniInvokeNative)

 # This function translates the "Java" calling convention into the
 # C convention used in native methods. Java VM passes all the
 # arguments in the Java stack, and expects the results to be placed there
 # as well. We therefore have to copy the arguments into the C stack (or
 # registers), and place the return values back into the Java stack.
 #
 # With a small sacrifice in efficiency, this approach avoids having to
 # generate a stub function for every native method.
 #
 # The x86 passes all arguments on stack, and returns floating pointer
 # results in ST0, 32-bit results in eax, and 64-bit results in eax and
 # edx.
 #
 # The first argument to CVMjniInvokeNative is a pointer to the JNI
 # environment, which should be passed unmodified as the first argument
 # to the native method.
 #
 # The second argument is a pointer to the "real" native method function.
 #
 # The third argument (stk) is a pointer to the Java stack, where all
 # the arguments are stored (as stk[0], stk[1], etc.).
 #
 # The fourth argument is the "terse" signature of the native method,
 # which basically collapses all objects in the long signature into
 # one byte, since they're all treated the same.  This makes the parsing
 # in this routine simpler and faster.  See classload.c and classruntime.c
 # for details.
 #
 # The fifth argument is the total size (in 32-bit words) of the
 # arguments on the Java stack. Note that the Java stack does not have
 # any alignment requirement, and stores all arguments consecutively in
 # words and double words. The argument size includes the "this" pointer
 # for non-static methods.
 #
 # The sixth argument is 0 for non-static methods, or a jclass
 # for static methods. Non-static native methods receive an object
 # reference as the second argument (passed in the Java stack as
 # stk[0]). The "real" method arguments to non-static methods begin at
 # stk[1]. Static native methods receive a class reference as the second
 # argument.
 #
 # The return value of the native method is placed at stk[0] for
 # word-sized results, or at stk[0] and stk[1] for
 # double-word-sized results. The return value of CVMjniInvokeNative is
 # 0 if the native method returns void, 1 if the native
 # method returns a word, 2 if the native method returns a
 # double word, or -1 if the native method returns an object.

# #define args_again \
	# movb	%dl, %cl; \
	# andl	$0xf, %ecx; \
	# shrl	$4,%edx; \
	# jmp	*arg_jumps(,%ecx,4)

	

 # _env$ = 8 ; JNI environment
 # _f$ = 12 ; native method function
 # _stk$ = 16 ; Java stack
 # _sig$ = 20 ; method signature
 # _sz$ = 24 ; total argument size
 # _cls$ = 28 ; class (if static)
 # _res$ = 32 ; result pointer

	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%edi
	pushl	%ebx

	movl	16(%ebp), %esi	# stk
	movl	24(%ebp), %edx	# sz
	movl	28(%ebp), %ecx	# cls

	cmpl	$0, %ecx
	jne	static_call
	movl	%esi, %ecx 
	addl	$4, %esi 
	jmp	static_done

static_call: 
	addl	$1, %edx 

static_done:
	addl	$1, %edx
	shll	$2, %edx	# word address -> byte address

	# NOTE: At this point, %esp is %ebp + 3 pushl i.e. %ebp - 12.  We will
	#       need to restore #esp to this value later after the call.
	subl	%edx, %esp	# reserve stack space for JNI method args.
        andl    $0xfffffff0, %esp # keep stack 16 byte aligned
	movl	%esp, %edi

	movl	8(%ebp), %eax	# copy env to outgoing args[0]
	movl	%eax, (%edi)
	addl	$4, %edi
	movl	%ecx, (%edi)    # copy jclass or jobject to args[1]
	addl	$4, %edi

	movl	20(%ebp), %ebx
	xorl	%ecx, %ecx	# zero it

 # ebx is current index into sig
 # cl (ecx) is current char in sig
 # esi is index into java stack
 # edx is (was) the # args, though we do not
 # need it anymore.
args_loop:
	movl	(%ebx),%edx
	addl	$4,%ebx
	shrl	$4,%edx  # shift over return syllable
	# args_again 
	movb	%dl, %cl;
	andl	$0xf, %ecx;
	shrl	$4,%edx;
	jmp	*arg_jumps(,%ecx,4)

arg_reload: # fetch more signature
	movl	(%ebx),%edx
	addl	$4,%ebx
	# args_again
	movb	%dl, %cl;
	andl	$0xf, %ecx;
	shrl	$4,%edx;
	jmp	*arg_jumps(,%ecx,4)

arg_32:
	movl	(%esi), %eax
	addl	$4, %esi
	movl	%eax, (%edi)
	addl	$4, %edi
	# args_again 
	movb	%dl, %cl;
	andl	$0xf, %ecx;
	shrl	$4,%edx;
	jmp	*arg_jumps(,%ecx,4)

arg_64:
	movl	(%esi), %eax
	addl	$4, %esi
	movl	%eax, (%edi)
	addl	$4, %edi
	movl	(%esi), %eax
	addl	$4, %esi
	movl	%eax, (%edi)
	addl	$4, %edi
	# args_again 
	movb	%dl, %cl;
	andl	$0xf, %ecx;
	shrl	$4,%edx;
	jmp	*arg_jumps(,%ecx,4)

arg_object:
	movl	(%esi), %eax
	cmpl	$0, %eax
	je	object_checked
	movl	%esi, %eax
object_checked:
	movl	%eax, (%edi)
	addl	$4, %esi
	addl	$4, %edi
	# args_again 
	movb	%dl, %cl;
	andl	$0xf, %ecx;
	shrl	$4,%edx;
	jmp	*arg_jumps(,%ecx,4)

args_done:
	call	*12(%ebp)	# f
	leal	-12(%ebp), %esp	# restore old stack pointer / pop arguments.
				# works for both __stdcall and cdecl.
	
	movl	32(%ebp), %esi	# res
	xorl	%ecx, %ecx	# c register volatile cross calls - zero it
	movl	20(%ebp), %ebx	# method signature
	movb	(%ebx), %cl
	andl	$0xf, %ecx
	jmp	*ret_jumps(,%ecx,4)

ret_obj:
	movl	%eax, (%esi)
	movl	$-1, %eax
	jmp	done

ret_f64:
	fstpl	(%esi)
	movl	$2, %eax
	jmp	done

ret_f32:
	fstps	(%esi)
	movl	$1, %eax
	jmp	done

ret_s32:
	movl	%eax, (%esi)
	movl	$1, %eax
	jmp	done

ret_s64:
	movl	%eax, (%esi)
	movl	$2, %eax
	movl	%edx, 4(%esi)
	jmp	done

ret_s8:
	shll	$24, %eax
	sarl	$24, %eax
	movl	%eax, (%esi)
	movl	$1, %eax
	jmp	done

ret_u8:
	shll	$24, %eax
	shrl	$24, %eax
	movl	%eax, (%esi)
	movl	$1, %eax
	jmp	done

ret_s16:
	shll	$16, %eax
	sarl	$16, %eax
	movl	%eax, (%esi)
	movl	$1, %eax
	jmp	done

ret_u16:
	shll	$16, %eax
	shrl	$16, %eax
	movl	%eax, (%esi)
	movl	$1, %eax
	jmp	done

ret_void:
	movl	$0, %eax

done:
	popl	%ebx
	popl	%edi
	popl	%esi
	movl	%ebp, %esp
	popl	%ebp
	ret

ret_jumps:		
	.long	ret_void	# this is invalid and should not get called
	.long	ret_void	# ENDFUNC should not get called
	.long	ret_void	# no void arguments
	.long	ret_s32	# int
	.long	ret_s16	# short
	.long	ret_u16	# char
	.long	ret_s64	# long
	.long	ret_s8	# byte
	.long	ret_f32	# float
	.long	ret_f64	# double
	.long	ret_u8	# bool
	.long	ret_obj
	.long	ret_void	# this is invalid and should not get called

arg_jumps:
	.long	arg_reload
	.long	args_done # end-of-args
	.long	ret_void	# this is invalid and should not get called
	.long	arg_32	# int
	.long	arg_32	# short
	.long	arg_32	# char
	.long	arg_64	# long
	.long	arg_32	# byte
	.long	arg_32	# float
	.long	arg_64	# double
	.long	arg_32	# bool
	.long	arg_object
	.long	ret_void	# this is invalid and should not get called

	.align	4
	SET_SIZE(CVMjniInvokeNative)
