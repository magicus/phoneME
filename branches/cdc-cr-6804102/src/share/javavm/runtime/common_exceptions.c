/*
 * @(#)common_exceptions.c	1.43 06/10/10
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

#include "javavm/include/interpreter.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/directmem.h"

#undef THROW_FUNC
#define THROW_FUNC(PkgName, ExcName)					\
void CVMthrow##ExcName(CVMExecEnv *ee, const char *format, ...)		\
{									\
    va_list argList;							\
    va_start(argList, format);						\
    CVMsignalErrorVaList(ee, CVMsystemClass(PkgName##_##ExcName), 	\
			 format, argList);				\
    va_end(argList);							\
}

#ifdef CVM_CLASSLOADING
THROW_FUNC(java_lang, ClassCircularityError)
THROW_FUNC(java_lang, ClassFormatError)
THROW_FUNC(java_lang, IllegalAccessError)
THROW_FUNC(java_lang, InstantiationError)
THROW_FUNC(java_lang, LinkageError)
THROW_FUNC(java_lang, UnsupportedClassVersionError)
THROW_FUNC(java_lang, VerifyError)
#endif

#ifdef CVM_DYNAMIC_LINKING
THROW_FUNC(java_lang, UnsatisfiedLinkError)
#endif

#ifdef CVM_REFLECT
THROW_FUNC(java_lang, NegativeArraySizeException)
THROW_FUNC(java_lang, NoSuchFieldException)
THROW_FUNC(java_lang, NoSuchMethodException)
#endif

#if defined(CVM_CLASSLOADING) || defined(CVM_REFLECT)
THROW_FUNC(java_lang, IncompatibleClassChangeError)
#endif

THROW_FUNC(java_lang, AbstractMethodError)
THROW_FUNC(java_lang, ArithmeticException)
THROW_FUNC(java_lang, ArrayIndexOutOfBoundsException)
THROW_FUNC(java_lang, ArrayStoreException)
THROW_FUNC(java_lang, ClassCastException)
THROW_FUNC(java_lang, ClassNotFoundException)
THROW_FUNC(java_lang, CloneNotSupportedException)
THROW_FUNC(java_lang, IllegalAccessException)
THROW_FUNC(java_lang, IllegalArgumentException)
THROW_FUNC(java_lang, IllegalMonitorStateException)
#ifdef CVM_INSPECTOR
THROW_FUNC(java_lang, IllegalStateException)
#endif
THROW_FUNC(java_lang, InstantiationException)
THROW_FUNC(java_lang, InternalError)
THROW_FUNC(java_lang, InterruptedException)
THROW_FUNC(java_lang, NoClassDefFoundError)
THROW_FUNC(java_lang, NoSuchFieldError)
THROW_FUNC(java_lang, NoSuchMethodError)
THROW_FUNC(java_lang, NullPointerException)
THROW_FUNC(java_lang, OutOfMemoryError)
THROW_FUNC(java_lang, StackOverflowError)
THROW_FUNC(java_lang, StringIndexOutOfBoundsException)
THROW_FUNC(java_lang, UnsupportedOperationException)

THROW_FUNC(java_io, InvalidClassException)
THROW_FUNC(java_io, IOException)

THROW_FUNC(sun_io, ConversionBufferFullException)
THROW_FUNC(sun_io, UnknownCharacterException)
THROW_FUNC(sun_io, MalformedInputException)
