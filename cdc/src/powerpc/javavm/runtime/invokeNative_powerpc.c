/*
 * @(#)invokeNative_powerpc.c	1.12 06/10/10
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

#include <sys/types.h>
#include "javavm/include/clib.h"
#include "javavm/include/typeid.h"
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/jni.h"
#include "javavm/include/interpreter.h"

/* TODO: This should really be written in assembler for better performance
 * and safer stack usage.
 */

/*
 * Invoke a native method using the PowerPC ABI calling conventions.
 * We store as many arguments as we can in registers, then we allocate
 * stack space for the remaining arguments if needed.
 */

CVMInt32
CVMjniInvokeNative(void * env, void * nativeCode, CVMUint32 * args,
		   CVMUint32 * terseSig, CVMInt32 argsSize,
		   void * classObject,
		   CVMJNIReturnValue * returnValue) {

	CVMUint32	sig;
	CVMUint32	retType;
	register CVMUint32	*curp;
	register CVMUint32	saveFp;
	CVMUint32	gpreg;
	CVMUint32	*gpregs;
#ifndef _SOFT_FLOAT
	CVMUint32	fpreg;
	double		*fpregs;
#endif /* ! _SOFT_FLOAT */

/* "Local" macro definitions */

/*
 * Allocate parameter list area; the amount of space needed is estimated
 * by considering the worst-case situation with respect to the alignment
 * of the remaining args.
 * We can't use gcc's alloca() because it's broken (wrong resulting stack
 * pointer alignment).
 *
 * We need room for argSize words of parameters, plus 2 extra words for the
 * saved SP and RetPC. We also need to keep the stack 16-byte aligned, thus
 * we round the number of words up to a 4 word boundary.
 *
 * Note we need to be somewhat pessimistic with argsSize since it does not
 * take into account the need to align 64-bit arguments. At worse this
 * alignment will require an extra argSize/3 words.
 */
#define init_parameter_list_area(curp)					\
	if (curp == NULL) {						\
	    CVMInt32 maxArgWords = argsSize + 2 + argsSize/3;		\
	    register CVMInt32 size =					\
		-(((maxArgWords + 3) & ~3) << 2);			\
	    asm volatile("lwz %0,0(%%r1)" : "=r" (curp));		\
	    asm volatile("stwux %0,%%r1,%1" : : "r" (curp), "r" (size)); \
	    asm volatile("addi %0,%%r1,8" : "=r" (curp));		\
	}

#ifdef _SOFT_FLOAT

#define invokeAndCast(ret_type)						\
	(((ret_type (*)(						\
		CVMUint32, CVMUint32, CVMUint32, CVMUint32,		\
		CVMUint32, CVMUint32, CVMUint32, CVMUint32))nativeCode)(\
		gpregs[0], gpregs[1], gpregs[2], gpregs[3],		\
		gpregs[4], gpregs[5], gpregs[6], gpregs[7]))

#else /* ! _SOFT_FLOAT */

#define invokeAndCast(ret_type)						\
	(((ret_type (*)(						\
		CVMUint32, CVMUint32, CVMUint32, CVMUint32,		\
		CVMUint32, CVMUint32, CVMUint32, CVMUint32,		\
		double, double, double, double,				\
		double, double, double, double))nativeCode)(		\
		gpregs[0], gpregs[1], gpregs[2], gpregs[3],		\
		gpregs[4], gpregs[5], gpregs[6], gpregs[7],		\
		fpregs[0], fpregs[1], fpregs[2], fpregs[3],		\
		fpregs[4], fpregs[5], fpregs[6], fpregs[7]))

#endif /* _SOFT_FLOAT */

	gpreg = (CVMUint32)env;
	env = (void *)CVMjniEnv2ExecEnv((JNIEnv *)env);
	gpregs = CVMexecEnv2threadID((CVMExecEnv *)env)->archData.gpregs;
	gpregs[0] = gpreg;
	if (classObject == NULL) {
		/* non-static method: pass 'this' indirect pointer */
		gpregs[1] = (CVMUint32) args++;
		argsSize--;
	} else {
		/* static method: pass class pointer */
		gpregs[1] = (CVMUint32)classObject;
	}

	gpreg = 2;
	curp = NULL;
	asm volatile("mr %0,%%r1" : "=r" (saveFp));
#ifndef _SOFT_FLOAT
	fpreg = 0;
	fpregs = CVMexecEnv2threadID((CVMExecEnv *)env)->archData.fpregs;
#endif /* ! _SOFT_FLOAT */
	/* init sig and save return type in retType */
	sig = *terseSig++;
	retType = sig & 0xf;
	sig >>= 4;
	while ((sig & 0xf) != CVM_TYPEID_ENDFUNC) {
		switch (sig & 0xf) {
		case 0:
			/* reload */
			sig = *terseSig++;
			continue;
		case CVM_TYPEID_VOID:
			/* should not occur */
			CVMassert(0);
			break;
		case CVM_TYPEID_INT:
		case CVM_TYPEID_SHORT:
		case CVM_TYPEID_BYTE:
		case CVM_TYPEID_CHAR:
		case CVM_TYPEID_BOOLEAN:
#ifdef _SOFT_FLOAT
		case CVM_TYPEID_FLOAT:
#endif /* _SOFT_FLOAT */
			/* 4-byte by value */
			if (gpreg > 7) {
				init_parameter_list_area(curp);
				*curp++ = *args++;
			} else {
				gpregs[gpreg++] = *args++;
			}
			argsSize--;
			break;
		case CVM_TYPEID_LONG:
#ifdef _SOFT_FLOAT
		case CVM_TYPEID_DOUBLE:
#endif /* _SOFT_FLOAT */
			/* 8-byte by value */
			if (gpreg > 6) {
				init_parameter_list_area(curp);
			        /* round up to multiple of 8 */
				curp = (CVMUint32*)(((int)curp + 7) & ~7);
				*curp++ = *args++;
				*curp++ = *args++;
			} else {
			        /* round up to multiple of 2 */
				gpreg = (gpreg + 1) & ~1;
				gpregs[gpreg++] = *args++;
				gpregs[gpreg++] = *args++;
			}
			argsSize -= 2;
			break;
#ifndef _SOFT_FLOAT
		case CVM_TYPEID_FLOAT:
			/* 4-byte by value */
			if (fpreg > 7) {
				init_parameter_list_area(curp);
				*curp++ = *args++;
			} else {
				fpregs[fpreg++] = *(float *)args++;
			}
			argsSize--;
			break;
		case CVM_TYPEID_DOUBLE:
			/* 8-byte by value */
			if (fpreg > 7) {
				init_parameter_list_area(curp);
				*curp++ = *args++;
				*curp++ = *args++;
			} else {
				fpregs[fpreg++] = *(double *)args;
				args += 2;
			}
			argsSize -= 2;
			break;
#endif /* ! _SOFT_FLOAT */
		case CVM_TYPEID_OBJ:
			/* 4-byte by reference */
			if (gpreg > 7) {
				init_parameter_list_area(curp);
				if (*args != 0) {
					*curp++ = (CVMUint32)args++;
				} else {
					*curp++ = 0;
					args++;
				}
			} else {
				if (*args != 0) {
					gpregs[gpreg++] = (CVMUint32)args++;
				} else {
					gpregs[gpreg++] = 0;
					args++;
				}
			}
			argsSize--;
			break;
		}
		sig >>= 4;
	}

#ifdef CVM_DEBUG_ASSERTS
	/* Make sure curp did not overrun the stack */
	if (curp != NULL) {
	    int stack_space_left = saveFp - (CVMUint32)curp;
	    CVMassert(stack_space_left >= 0);
	}
#endif

	CVMassert(argsSize == 0);

	switch (retType) {
	case CVM_TYPEID_INT:
	case CVM_TYPEID_SHORT:
	case CVM_TYPEID_BYTE:
	case CVM_TYPEID_CHAR:
	case CVM_TYPEID_BOOLEAN:
#ifdef _SOFT_FLOAT
	case CVM_TYPEID_FLOAT:
#endif /* _SOFT_FLOAT */
		returnValue->i = invokeAndCast(CVMUint32);
		asm volatile("mr %%r1,%0" : : "r" (saveFp));
		return (1);
#ifdef _SOFT_FLOAT
	case CVM_TYPEID_DOUBLE:
#endif /* _SOFT_FLOAT */
	case CVM_TYPEID_LONG:
		*(CVMUint64 *)returnValue = invokeAndCast(CVMUint64);
		asm volatile("mr %%r1,%0" : : "r" (saveFp));
		return (2);
#ifndef _SOFT_FLOAT
	case CVM_TYPEID_FLOAT:
		returnValue->f = invokeAndCast(float);
		asm volatile("mr %%r1,%0" : : "r" (saveFp));
		return (1);
	case CVM_TYPEID_DOUBLE:
		*(double *)returnValue = invokeAndCast(double);
		asm volatile("mr %%r1,%0" : : "r" (saveFp));
		return (2);
#endif /* ! _SOFT_FLOAT */
	case CVM_TYPEID_OBJ:
		returnValue->o = invokeAndCast(void *);
		asm volatile("mr %%r1,%0" : : "r" (saveFp));
		return (-1);
	case CVM_TYPEID_VOID:
	default:
		invokeAndCast(void);
		asm volatile("mr %%r1,%0" : : "r" (saveFp));
		return (0);
	}

}
