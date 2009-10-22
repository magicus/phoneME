/*
 * @(#)hook.h	1.4 06/10/10
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

#ifndef _INCLUDED_HOOK_H
#define _INCLUDED_HOOK_H

#include "javavm/include/defs.h"

#ifdef CVM_EMBEDDED_HOOK

/* NOTE: The implementation of the following functions is to be provided
   by the user of the hook functionality if CVM_EMBEDDED_HOOK is defined.
   The shared code will not provide any implementations.
*/

/* Purpose: Called right after VM initialization is done. */
void CVMhookVMStart(CVMExecEnv *ee);

/* Purpose: Called right before VM shutdown is executed. */
void CVMhookVMShutdown(CVMExecEnv *ee);

#endif /* CVM_EMBEDDED_HOOK */

#endif /* _INCLUDED_HOOK_H */
