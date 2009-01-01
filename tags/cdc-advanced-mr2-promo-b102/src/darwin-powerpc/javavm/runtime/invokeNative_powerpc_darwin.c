/*
 * @(#)invokeNative_powerpc_darwin.c	1.11 06/10/10
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

/* NOTE: This should really be written in assembler for better performance
 * and safer stack usage. */

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

        CVMExecEnv*     ee = CVMjniEnv2ExecEnv((JNIEnv*)env);
	CVMUint32	sig;
	CVMUint32	retType;
	register CVMUint32	*curp;
	register CVMUint32	saveFp;
	CVMUint32	gpreg;
	CVMUint32	*gpregs;
#ifndef SOFT_FLOAT
	CVMUint32	fpreg;
	double		*fpregs;
#endif /* ! SOFT_FLOAT */

#ifdef SOFT_FLOAT

#define invokeAndCast(ret_type)						\
	(((ret_type (*)(						\
		CVMUint32, CVMUint32, CVMUint32, CVMUint32,		\
		CVMUint32, CVMUint32, CVMUint32, CVMUint32))nativeCode)(\
		gpregs[0], gpregs[1], gpregs[2], gpregs[3],		\
		gpregs[4], gpregs[5], gpregs[6], gpregs[7]))

#else /* ! SOFT_FLOAT */

#define invokeAndCast(ret_type)						\
	(((ret_type (*)(						\
		CVMUint32, CVMUint32, CVMUint32, CVMUint32,		\
		CVMUint32, CVMUint32, CVMUint32, CVMUint32,		\
		double, double, double, double,				\
		double, double, double, double,				\
		double, double, double, double, double))nativeCode)(	\
		gpregs[0], gpregs[1], gpregs[2], gpregs[3],		\
		gpregs[4], gpregs[5], gpregs[6], gpregs[7],		\
		fpregs[0], fpregs[1], fpregs[2], fpregs[3],		\
		fpregs[4], fpregs[5], fpregs[6], fpregs[7],		\
		fpregs[8], fpregs[9], fpregs[10], fpregs[11], fpregs[12]))

#endif /* SOFT_FLOAT */

	/*
	 * Make room for the parameters and a new linkage area.
	 *
	 * NOTE. We should only do this if the arguments are know to
	 * take up more than 8 + 13 = 21 words. Also, I'm conernced
	 * about where the locals for this function are being stored,
	 * especially in debug builds. They may end up trouncing the
	 * old linkage area. Extensive testing has revealed no problem.
	 * However, CVMjniInvokeNative() should really be in assembler.
       	 */
	{
	    register CVMInt32 size = -((((argsSize+6)&~3) << 2) + 32);
	    asm volatile("mr %0,r1" : "=r" (saveFp));
	    asm volatile("lwz %0,0(r1)" : "=r" (curp));
	    asm volatile("stwux %0,r1,%1" : : "r" (curp), "r" (size));
	    asm volatile("addi %0,r1,24" : "=r" (curp));
	}

	gpregs = CVMexecEnv2threadID(ee)->archData.gpregs;
	gpregs[0] = (CVMUint32)env;
	if (classObject == NULL) {
		/* non-static method: pass 'this' indirect pointer */
		gpregs[1] = (CVMUint32) args++;
		argsSize--;
	} else {
		/* static method: pass class pointer */
		gpregs[1] = (CVMUint32)classObject;
	}

        curp += 2;
	gpreg = 2;

#ifndef SOFT_FLOAT
	fpreg = 0;
	fpregs = CVMexecEnv2threadID(ee)->archData.fpregs;
#endif /* ! SOFT_FLOAT */
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
#ifdef SOFT_FLOAT
		case CVM_TYPEID_FLOAT:
#endif /* SOFT_FLOAT */
			/* 4-byte by value */
			if (gpreg > 7) {
				curp[0] = args[0];
			} else {
				gpregs[gpreg++] = args[0];
			}
			curp++;
			args++;
			argsSize--;
			break;
		case CVM_TYPEID_LONG:
#ifdef SOFT_FLOAT
		case CVM_TYPEID_DOUBLE:
#endif /* SOFT_FLOAT */
			/* 8-byte by value */
		        if (gpreg == 7) {
				curp[0] = args[0];
				gpregs[gpreg++] = args[1];
			} else if (gpreg > 7) {
				curp[0] = args[0];
				curp[1] = args[1];
			} else {
				gpregs[gpreg++] = args[0];
				gpregs[gpreg++] = args[1];
			}
			curp += 2;
			args += 2;
			argsSize -= 2;
			break;
#ifndef SOFT_FLOAT
		case CVM_TYPEID_FLOAT:
			/* 4-byte by value */
			if (fpreg > 12) {
				curp[0] = args[0];
			} else {
				fpregs[fpreg++] = ((float *)args)[0];
			}
			gpreg++;
			curp++;
			args++;
			argsSize--;
			break;
		case CVM_TYPEID_DOUBLE:
			/* 8-byte by value */
			if (fpreg > 12) {
				curp[0] = args[0];
				curp[1] = args[1];
			} else {
				fpregs[fpreg++] = ((double *)args)[0];
			}
			gpreg += 2;
			curp += 2;
			args += 2;
			argsSize -= 2;
			break;
#endif /* ! SOFT_FLOAT */
		case CVM_TYPEID_OBJ:
			/* 4-byte by reference */
			if (gpreg > 7) {
				if (*args != 0) {
					curp[0] = (CVMUint32)args;
				} else {
					curp[0] = 0;
				}
			} else {
				if (*args != 0) {
					gpregs[gpreg++] = (CVMUint32)args;
				} else {
					gpregs[gpreg++] = 0;
				}
			}
			curp++;
			args++;
			argsSize--;
			break;
		}
		sig >>= 4;
	}

#if 0
	/* TODO: this assert happens sometimes when doing optimized debug
	 * builds, but it does not appear to be fatal. CVMjniInvokeNative
	 * should really be written in assembler.
	 */
	if (curp != NULL) {
		int stack_space_left =
		    8 + (char *)__builtin_frame_address(0) - (char *)curp;
		CVMassert(stack_space_left >= 0);
	}
#endif /* CVM_DEBUG */

	CVMassert(argsSize == 0);

	switch (retType) {
	case CVM_TYPEID_INT:
	case CVM_TYPEID_SHORT:
	case CVM_TYPEID_BYTE:
	case CVM_TYPEID_CHAR:
	case CVM_TYPEID_BOOLEAN:
#ifdef SOFT_FLOAT
	case CVM_TYPEID_FLOAT:
#endif /* SOFT_FLOAT */
		returnValue->i = invokeAndCast(CVMUint32);
		asm volatile("mr r1,%0" : : "r" (saveFp));
		return (1);
#ifdef SOFT_FLOAT
	case CVM_TYPEID_DOUBLE:
#endif /* SOFT_FLOAT */
	case CVM_TYPEID_LONG:
		*(CVMUint64 *)returnValue = invokeAndCast(CVMUint64);
		asm volatile("mr r1,%0" : : "r" (saveFp));
		return (2);
#ifndef SOFT_FLOAT
	case CVM_TYPEID_FLOAT:
		returnValue->f = invokeAndCast(float);
		asm volatile("mr r1,%0" : : "r" (saveFp));
		return (1);
	case CVM_TYPEID_DOUBLE:
		*(double *)returnValue = invokeAndCast(double);
		asm volatile("mr r1,%0" : : "r" (saveFp));
		return (2);
#endif /* ! SOFT_FLOAT */
	case CVM_TYPEID_OBJ:
		returnValue->o = invokeAndCast(void *);
		asm volatile("mr r1,%0" : : "r" (saveFp));
		return (-1);
	case CVM_TYPEID_VOID:
	default:
		invokeAndCast(void);
		asm volatile("mr r1,%0" : : "r" (saveFp));
		return (0);
	}

}
