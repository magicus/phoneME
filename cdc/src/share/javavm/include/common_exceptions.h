/*
 * @(#)common_exceptions.h	1.40 06/10/27
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

/*
 * Common exceptions thrown from within JVM.  Add more here as you
 * find uses of SignalError. (Help the next guy avoid typos.)
 *
 * NATIVE LIBRARIES MUST NOT USE THESE ROUTINES.
 */

#ifndef _JAVASOFT_COMMON_EXCEPTIONS_H_
#define _JAVASOFT_COMMON_EXCEPTIONS_H_

typedef void CVMThrowFunc(CVMExecEnv *, const char *format, ...);


#ifdef CVM_CLASSLOADING
extern CVMThrowFunc CVMthrowClassCircularityError;
extern CVMThrowFunc CVMthrowClassFormatError;
extern CVMThrowFunc CVMthrowIllegalAccessError;
extern CVMThrowFunc CVMthrowInstantiationError;
extern CVMThrowFunc CVMthrowLinkageError;
extern CVMThrowFunc CVMthrowUnsupportedClassVersionError;
extern CVMThrowFunc CVMthrowVerifyError;
#endif

/* NOTE: CVM_CLASSLOADING and CVM_JVMTI both imply
   CVM_DYNAMIC_LINKING is defined; see build/share/defs.mk. */
#ifdef CVM_DYNAMIC_LINKING
extern CVMThrowFunc CVMthrowUnsatisfiedLinkError;
#endif

#ifdef CVM_REFLECT
extern CVMThrowFunc CVMthrowNegativeArraySizeException;
extern CVMThrowFunc CVMthrowNoSuchFieldException;
extern CVMThrowFunc CVMthrowNoSuchMethodException;
#endif

#if defined(CVM_CLASSLOADING) || defined(CVM_REFLECT)
extern CVMThrowFunc CVMthrowIncompatibleClassChangeError;
#endif

extern CVMThrowFunc CVMthrowAbstractMethodError;
extern CVMThrowFunc CVMthrowArithmeticException;
extern CVMThrowFunc CVMthrowArrayIndexOutOfBoundsException;
extern CVMThrowFunc CVMthrowArrayStoreException;
extern CVMThrowFunc CVMthrowClassCastException;
extern CVMThrowFunc CVMthrowClassNotFoundException;
extern CVMThrowFunc CVMthrowCloneNotSupportedException;
extern CVMThrowFunc CVMthrowIllegalAccessException;
extern CVMThrowFunc CVMthrowIllegalArgumentException;
extern CVMThrowFunc CVMthrowIllegalMonitorStateException;
#ifdef CVM_INSPECTOR
extern CVMThrowFunc CVMthrowIllegalStateException;
#endif
extern CVMThrowFunc CVMthrowInstantiationException;
extern CVMThrowFunc CVMthrowInternalError;
extern CVMThrowFunc CVMthrowInterruptedException;
extern CVMThrowFunc CVMthrowNoClassDefFoundError;
extern CVMThrowFunc CVMthrowNoSuchFieldError;
extern CVMThrowFunc CVMthrowNoSuchMethodError;
extern CVMThrowFunc CVMthrowNullPointerException;
extern CVMThrowFunc CVMthrowOutOfMemoryError;
extern CVMThrowFunc CVMthrowStackOverflowError;
extern CVMThrowFunc CVMthrowStringIndexOutOfBoundsException;
extern CVMThrowFunc CVMthrowUnsupportedOperationException;

extern CVMThrowFunc CVMthrowInvalidClassException;
extern CVMThrowFunc CVMthrowIOException;

extern CVMThrowFunc CVMthrowConversionBufferFullException;
extern CVMThrowFunc CVMthrowUnknownCharacterException;
extern CVMThrowFunc CVMthrowMalformedInputException;

#endif /* !_JAVASOFT_COMMON_EXCEPTIONS_H_ */
