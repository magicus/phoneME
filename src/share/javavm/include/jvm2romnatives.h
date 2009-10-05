/*
 * @(#)jvm2romnatives.h	1.11 06/10/10
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

#ifndef _INCLUDED_JVM2ROMNATIVES_H
#define _INCLUDED_JVM2ROMNATIVES_H

/*
 * We can't use RegisterNatives with JavaCodeCompact, so fake it here.
 */

#define JVM_IHashCode		 Java_java_lang_Object_hashCode
#define JVM_Clone		 Java_java_lang_Object_clone
#define JVM_MonitorNotify	 Java_java_lang_Object_notify
#define JVM_MonitorNotifyAll	 Java_java_lang_Object_notifyAll
#define JVM_MonitorWait		 Java_java_lang_Object_wait

#if JAVASE > 15
#define JVM_GetClassName	 Java_java_lang_Class_getName0
#else
#define JVM_GetClassName	 Java_java_lang_Class_getName
#endif
#define JVM_GetClassInterfaces	 Java_java_lang_Class_getInterfaces
#define JVM_GetClassLoader	 Java_java_lang_Class_getClassLoader0
#define JVM_GetClassSigners	 Java_java_lang_Class_getSigners
#define JVM_SetClassSigners	 Java_java_lang_Class_setSigners
#define JVM_IsArrayClass	 Java_java_lang_Class_isArray
#define JVM_IsPrimitiveClass	 Java_java_lang_Class_isPrimitive
#define JVM_GetComponentType	 Java_java_lang_Class_getComponentType
#define JVM_GetClassModifiers	 Java_java_lang_Class_getModifiers
#define JVM_GetClassFields	 Java_java_lang_Class_getFields0
#define JVM_GetClassMethods	 Java_java_lang_Class_getMethods0
#define JVM_GetClassConstructors Java_java_lang_Class_getConstructors0
#define JVM_GetClassField	 Java_java_lang_Class_getField0
#define JVM_GetClassMethod	 Java_java_lang_Class_getMethod0
#define JVM_GetClassConstructor	 Java_java_lang_Class_getConstructor0
#define JVM_GetProtectionDomain	 Java_java_lang_Class_getProtectionDomain0
#define JVM_SetProtectionDomain	 Java_java_lang_Class_setProtectionDomain0
#define JVM_GetDeclaredClasses	 Java_java_lang_Class_getDeclaredClasses0
#define JVM_GetDeclaringClass	 Java_java_lang_Class_getDeclaringClass
#define JVM_DesiredAssertionStatus Java_java_lang_Class_desiredAssertionStatus0
#define JVM_AssertionStatusDirectives Java_java_lang_ClassLoader_retrieveDirectives

#define JVM_StartThread		 Java_java_lang_Thread_start0
#define JVM_RunNativeThread	 Java_java_lang_Thread_runNative
#define JVM_StopThread		 Java_java_lang_Thread_stop0
#define JVM_IsThreadAlive	 Java_java_lang_Thread_isAlive
#define JVM_SuspendThread	 Java_java_lang_Thread_suspend0
#define JVM_ResumeThread	 Java_java_lang_Thread_resume0
#define JVM_SetThreadPriority	 Java_java_lang_Thread_setPriority0
#define JVM_Yield		 Java_java_lang_Thread_yield
#define JVM_Sleep		 Java_java_lang_Thread_sleep0
#define JVM_CurrentThread	 Java_java_lang_Thread_currentThread
#define JVM_CountStackFrames	 Java_java_lang_Thread_countStackFrames
#define JVM_IsInterrupted	 Java_java_lang_Thread_isInterrupted
#ifndef CDC_10
#define JVM_HoldsLock            Java_java_lang_Thread_holdsLock
#endif

#define JVM_CurrentTimeMillis	 Java_java_lang_System_currentTimeMillis
#define JVM_ArrayCopy		 Java_java_lang_System_arraycopy
#define JVM_InitializeCompiler	 Java_java_lang_Compiler_initialize
#define JVM_CompileClass	 Java_java_lang_Compiler_compileClass
#define JVM_CompileClasses	 Java_java_lang_Compiler_compileClasses
#define JVM_CompilerCommand	 Java_java_lang_Compiler_command
#define JVM_EnableCompiler	 Java_java_lang_Compiler_enable
#define JVM_DisableCompiler	 Java_java_lang_Compiler_disable

#endif /* _INCLUDED_JVM2ROMNATIVES_H */
