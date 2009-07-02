/*
 * @(#)jni.h	1.16 06/10/10
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

#ifndef _INCLUDED_PORTING_JNI_H
#define _INCLUDED_PORTING_JNI_H

#include "javavm/include/porting/vm-defs.h"

/*
 * JDK1.2-compatible JNI definitions.
 *
 * JNICALL	// keywords for calling convention
 * JNIEXPORT	// keywords to export DLL function
 *     
 * For example, exported functions are defined like this:
 *
 * JNIEXPORT jint JNICALL
 * JNI_CreateJavaVM(JavaVM **pvm, void **penv, void *args);
 *
 * and win32 defines the macros like this:
 *
 * #define JNICALL __stdcall
 * #define JNIEXPORT __declspec(dllexport)
 *
 * JNI_LIB_PREFIX	// library prefix
 * JNI_LIB_SUFFIX	// library suffix
 *
 * Example for lib%s.so library pattern:
 *   #define JNI_LIB_PREFIX "lib"
 *   #define JNI_LIB_SUFFIX ".so"
 */

/*
 * CVMJNIReturnValue - Used to return the jni method result.
 * It is passed to CVMjniInvokeNative, which stores the result
 * in it after the java method returns to CVMjniInvokeNative.
 */
/* 
 * CVMmemCopy64Stub is used for copying the 'raw'
 * field of struct JavaVal32 into v64.
 * Because the raw field has been changed into type CVMAddr 
 * for the 64 bit port, the type has to be changed
 * accordingly.
 */
typedef union {
    CVMInt32	 i;
    CVMJavaFloat f;
    CVMAddr	 v64[2];
    void *	 o;
} CVMJNIReturnValue;

/*
 * CVMjniInvokeNative
 *
 * This function translates the "Java" calling convention into the "C"
 * calling convention used in native methods. Java VM passes all the
 * arguments in the Java stack, and places the results there
 * as well. We therefore have to copy the arguments into the C stack (or
 * registers), and place the return values into the CVMJNIReturnValue
 * structure pointed to by "returnValue".  Upon return the Java VM will
 * copy the result from there to the Java stack.
 *
 * env
 *
 * A pointer to the JNI environment, which should be passed unmodified
 * as the first argument to the native method.
 *
 * nativeCode
 *
 * A pointer to the "real" native method function.
 *
 * args
 *
 * A pointer to the Java stack, where all the arguments are stored
 * (as args[0], args[1], etc.).  The Java stack slot for a single-word
 * integer and float arguments can be read as a jint and jfloat,
 * respectively, and passed to the native method unchanged.  For
 * double-word arguments, a jlong or jdouble value should be read
 * from the current two stack slots using the same method as
 * CVMjvm2Long and CVMjvm2Double, respectively.  For an object argument,
 * a NULL (0) value should be passed if the stack slot contains 0,
 * otherwise the address of the stack slot should be passed as a
 * jobject to the native method.
 *
 * terseSig
 *
 * The encoded "terse" signature of the native method.  It contains
 * 32-bit words containing 8 4-bit nibbles each.  Each nibble contains
 * a signature parsing code or the type code of an argument in the
 * native method signature:
 *
 * 0x00	RELOAD
 * 0x01 END
 * 0x02 void ("V")	(never seen in a arguments, only in return value)
 * 0x03 int ("I")
 * 0x04 short ("S")
 * 0x05 char ("C")
 * 0x06 long ("J")
 * 0x07 byte ("B")
 * 0x08 float ("F")
 * 0x09 double ("D")
 * 0x0a boolean ("Z")
 * 0x0b object ("Z")
 *
 * The return type comes first, then the arguments, followed by an
 * END code.  Nibbles are read from the least-significant
 * bits of the word first.  Most signatures fit in a single word.
 * For longer signatures, multile words are used.  A RELOAD nibble
 * is used as an escape code to signal that the next word is a
 * pointer to a new terse signature word, which allows chaining of
 * words for longer signatures.
 *
 * argsSize
 *
 * The total size (in 32-bit words) of the arguments on the Java stack.
 * Note that the Java stack does not have any alignment requirement, and
 * stores all arguments consecutively in words and double words. The
 * argument size includes the "this" pointer for non-static methods.
 *
 * classObject
 *
 * 0 (NULL) for non-static methods, or a jclass for static methods.
 * Non-static native methods receive an object reference as the second
 * argument (which is simply the address of args[0]).  The "real" method
 * arguments to non-static methods begin at args[1].  Static native methods
 * receive a class reference ("classObject") as the second argument.
 *
 * returnValue
 *
 * The return value of the native method is placed in returnValue->i
 * for word-sized results, or returnValue->v64 for double-word-sized
 * results, according to the following table:
 * 
 * jdouble	CVMdouble2Jvm to returnValue->v64
 * jlong	CVMlong2Jvm to returnValue->v64
 * jint		copy to returnValue->i
 * jbyte	CVMbyte2Int to returnValue->i
 * jchar	CVMchar2Int to returnValue->i
 * jshort	CVMshort2Int to returnValue->i
 * jfloat	copy to returnValue->f
 * jobject	copy to returnValue->o
 *
 * The return value of CVMjniInvokeNative is 0 if the native method
 * returns void, 1 if the native method returns a word, 2 if the native
 * method returns a double word, or -1 if the native method returns an
 * object.
 *
 */

/* 
 * The function parameter args is used in conjunction with the 'raw'
 * field of struct JavaVal32.
 * Because the raw field has been changed into type CVMAddr,
 * the type has to be changed accordingly.
 */
extern CVMInt32
CVMjniInvokeNative(void * env, void * nativeCode, CVMAddr * args,
		   CVMUint32 * terseSig, CVMInt32 argsSize,
		   void * classObject,
		   CVMJNIReturnValue * returnValue);

#include CVM_HDR_JNI_H

#endif /* _INCLUDED_PORTING_JNI_H */
