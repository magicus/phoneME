/*
 * @(#)jitirrange.h	1.6 06/10/10
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

#ifndef _INCLUDED_JITIRRANGE_H
#define _INCLUDED_JITIRRANGE_H

#include "javavm/include/jit/jit_defs.h"

/*
 * A range is like a block, but it cannot be branched to directly.
 * Only the containing block can be the target of a branch, in which
 * case the branch target is logically the first range in the
 * block.  A block with inlined methods may contain multiple
 * ranges with different compilation depths.  The depth changes
 * by +-1 for each subsequent range, depending on if an inlined
 * method context has been pushed or popped.
 */
struct CVMJITIRRange {
    CVMJITIRRange  *prev;
    CVMJITIRRange  *next;
    CVMJITMethodContext *mc;
    CVMUint16       startPC;	/* range original Java bytecode PC */ 
    CVMUint16       endPC;	/* range original Java bytecode PC */ 
    CVMJITIRRoot   *firstRoot;
};

/*
 * CVMJITIRRange macro defines and interface APIs
 */

extern CVMJITIRRange*
CVMJITirrangeCreate(CVMJITCompilationContext* con, CVMUint16 pc);

#endif /* _INCLUDED_JITIRRANGE_H */
