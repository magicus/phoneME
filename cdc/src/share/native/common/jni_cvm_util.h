/*
 * @(#)jni_cvm_util.h	1.11 06/10/10
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

#include "javavm/include/jdk2cvm.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/interpreter.h"

#define JNUThrowFunc(exception, env, msg) \
    CVMthrow##exception(CVMjniEnv2ExecEnv(env), "%s", msg);

/* Throw common exceptions */
#define JNU_ThrowNullPointerException(env, msg) \
    JNUThrowFunc(NullPointerException, env, msg)

#define JNU_ThrowArrayIndexOutOfBoundsException(env, msg) \
    JNUThrowFunc(ArrayIndexOutOfBoundsException, env, msg)

#define JNU_ThrowOutOfMemoryError(env, msg) \
    JNUThrowFunc(OutOfMemoryError, env, msg)

#define JNU_ThrowIllegalArgumentException(env, msg) \
    JNUThrowFunc(IllegalArgumentException, env, msg)

#define JNU_ThrowIllegalAccessError(env, msg) \
    JNUThrowFunc(IllegalAccessError, env, msg)

#define JNU_ThrowIllegalAccessException(env, msg) \
    JNUThrowFunc(IllegalAccessException, env, msg)

#define JNU_ThrowInternalError(env, msg) \
    JNUThrowFunc(InternalError, env, msg)

#define JNU_ThrowNoSuchFieldException(env, msg) \
    JNUThrowFunc(NoSuchFieldException, env, msg)

#define JNU_ThrowNoSuchMethodException(env, msg) \
    JNUThrowFunc(NoSuchMethodException, env, msg)

#define JNU_ThrowClassNotFoundException(env, msg) \
    JNUThrowFunc(ClassNotFoundException, env, msg)

#define JNU_ThrowNumberFormatException(env, msg) \
    JNUThrowFunc(NumberFormatException, env, msg)

#define JNU_ThrowIOException(env, msg) \
    JNUThrowFunc(IOException, env, msg)

#define JNU_ThrowNoSuchFieldError(env, msg) \
    JNUThrowFunc(NoSuchFieldError, env, msg)

#define JNU_ThrowNoSuchMethodError(env, msg) \
    JNUThrowFunc(NoSuchMethodError, env, msg)

#define JNU_ThrowStringIndexOutOfBoundsException(env, msg) \
    JNUThrowFunc(StringIndexOutOfBoundsException, env, msg)

#define JNU_ThrowInstantiationException(env, msg) \
    JNUThrowFunc(InstantiationException, env, msg)

/* Class constants */ 
#define JNU_ClassString(env) \
    (CVMcbJavaInstance(CVMsystemClass(java_lang_String)))

#define JNU_ClassClass(env) \
    (CVMcbJavaInstance(CVMsystemClass(java_lang_Class)))

#define JNU_ClassObject(env) \
    (CVMcbJavaInstance(CVMsystemClass(java_lang_Object)))

#define JNU_ClassThrowable(env) \
    (CVMcbJavaInstance(CVMsystemClass(java_lang_Throwable)))

/* These functions are in verifyformat.c, which used to be 
 * called check_format.c.
 */
extern jboolean VerifyClassname(char *name, jboolean allowArrayClass);
extern jboolean VerifyFixClassname(char *utf_name);
extern jboolean TranslateFromVMClassName(char *utf_name);
