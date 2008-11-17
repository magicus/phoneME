/*
 * @(#)jitopt.h	1.6 06/10/10
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

#ifndef _INCLUDED_JITOPT_H
#define _INCLUDED_JITOPT_H

#include "javavm/include/typeid.h"
#include "javavm/include/objects.h"
#include "javavm/include/jit/jit_defs.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitirnode.h"

#define CVMJITOPT_SIZE_OF_NOT_SEQUENCE     8

/*======================================================================
// Code Sequence list management utilities: 
*/

/* Purpose: Add the specified PC to the list of code sequences. */
extern void
CVMJIToptAddCodeSequence(CVMJITCompilationContext *con, CVMUint16 pcIndex,
                         CVMUint32 sequenceType);

/*======================================================================
// Code Sequence recognition utilities: 
*/

/* Purpose: Checks if the bytecode steam at the specified location is a NOT
   expression. */
extern CVMBool
CVMJIToptPatternIsNotSequence(CVMJITCompilationContext* con, CVMUint8 *absPc);

/*======================================================================
// Strength reduction and constant folding optimizers: 
*/

/* Purpose: Optimizes the specified binary int expression. */
extern CVMJITIRNode *
CVMJIToptimizeBinaryIntExpression(CVMJITCompilationContext* con,
                                  CVMJITIRNode* node);

/* Purpose: Optimizes the specified binary int expression. */
extern CVMJITIRNode *
CVMJIToptimizeUnaryIntExpression(CVMJITCompilationContext* con,
                                 CVMJITIRNode* node);

#endif /* _INCLUDED_JITOPT_H */
