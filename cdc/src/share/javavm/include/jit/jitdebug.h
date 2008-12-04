/*
 * @(#)jitdebug.h	1.9 06/10/10
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

#ifndef _INCLUDED_JITDEBUG_H
#define _INCLUDED_JITDEBUG_H

#ifdef CVM_JIT_DEBUG

#include "javavm/include/defs.h"
#include "javavm/include/porting/jit/jit.h"

/* ========================================================================*/

/* Purpose: Initialized the debugging state in the compilation context. */
extern void
CVMJITdebugInitCompilationContext(CVMJITCompilationContext *con,
                                  CVMExecEnv *ee);

/* ========================================================================*/

/* Purpose: Setup debug options before compilation begins. */
extern void
CVMJITdebugInitCompilation(CVMExecEnv *ee, CVMMethodBlock *mb);

/* Purpose: Clean up debug options after compilation ends. */
extern void
CVMJITdebugEndCompilation(CVMExecEnv *ee, CVMMethodBlock *mb);

/* Purpose: Enables tyracing of a method that is currently being compiled. 
   NOTE: This is only effective if USE_TRACING_LIST_FILTER is defined
         in jitdebug.c. */
extern void
CVMJITdebugStartTrace(void);

/* Purpose: Checks if the specified method is to be compiled or not. */
extern CVMBool
CVMJITdebugMethodIsToBeCompiled(CVMExecEnv *ee, CVMMethodBlock *mb);

#endif /* CVM_JIT_DEBUG */

#endif /* _INCLUDED_JITDEBUG_H */
