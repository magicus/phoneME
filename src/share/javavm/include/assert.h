/*
 * @(#)assert.h	1.14 06/10/10
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

#ifndef _INCLUDED_ASSERT_H
#define _INCLUDED_ASSERT_H

#include "javavm/include/porting/ansi/assert.h"
#include "javavm/include/porting/system.h"

/*
 * Assert macro
 *
 * CVMassert(expression)   // enabled by CVM_DEBUG_ASSERTS
 *
 * Macro to define statements that are to be expanded only if asserts
 * are enabled:
 *
 * CVMwithAssertsOnly(stmt)
 */

#ifdef CVM_DEBUG_ASSERTS
/* Calling CVMassertHook is to allow us to set a breakpoint
 * when an assertion fails. Otherwise it doesn't seem to be possible
 * to do this in GDB, and once the assertion is thrown, often GDB
 * gets confused about its state.
 */
int CVMassertHook(const char *filename, int lineno, const char *expr);
#define CVMassert(expression) \
    ((void)((expression) || CVMassertHook(__FILE__, __LINE__, #expression) || \
           (CVMsystemPanic("CVMassertHook returned"), 0)))
#define CVMwithAssertsOnly(x) x
#else
#define CVMassert(expression) ((void)0)
#define CVMwithAssertsOnly(x)
#endif /* CVM_DEBUG */

#ifdef CVM_DEBUG_ASSERTS
#define CVMpanic(msg) \
    (CVMassertHook(__FILE__, __LINE__, msg), CVMsystemPanic(msg))
#else
#define CVMpanic(msg)  ( CVMsystemPanic(msg) )
#endif


#ifdef CVM_DEBUG_ASSERTS

/* Purpose: Used to conditionally added the specified arguments only if
            CVM_DEBUG_ASSERTS is defined. */
#define DECLARE_CALLER_INFO(callerFilename, callerLineNumber) \
    , const char *callerFilename, CVMUint32 callerLineNumber

/* Purpose: Used to conditionally pass the specified arguments only if
            CVM_DEBUG_ASSERTS is defined. */
#define PASS_CALLER_INFO() \
    , __FILE__, __LINE__

#else

#define DECLARE_CALLER_INFO(callerFilename, callerLineNumber)
#define PASS_CALLER_INFO()

#endif /* CVM_DEBUG_ASSERTS */

#endif /* _INCLUDED_ASSERT_H */
