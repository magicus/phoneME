/*
 * @(#)jitirrange.c	1.6 06/10/10
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
#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/stacks.h"
#include "javavm/include/bcattr.h"
#include "javavm/include/utils.h"
#include "javavm/include/typeid.h"
#include "javavm/include/opcodes.h"
#include "javavm/include/globals.h"
#include "javavm/include/clib.h"
#include "javavm/include/porting/int.h"
#include "javavm/include/porting/system.h"

#include "javavm/include/bcutils.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitir.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitutils.h"
#include "javavm/include/jit/jitirnode.h"
#include "javavm/include/jit/jitirrange.h"
#include "javavm/include/jit/jitmemory.h"
#include "javavm/include/clib.h"


static void
CVMJITirrangeInit(CVMJITCompilationContext* con, CVMJITIRRange* r, 
    CVMUint16 pc)
{
    CVMassert(pc <= CVMjmdCodeLength(con->mc->jmd));
    r->startPC = pc;
    r->endPC = pc;
    r->mc = con->mc;
    r->firstRoot = NULL;
}

/*
 * Create a new range and initialize fields.
 */

CVMJITIRRange*
CVMJITirrangeCreate(CVMJITCompilationContext* con, CVMUint16 pc)
{
    CVMJITIRRange* r = 
	(CVMJITIRRange*)CVMJITmemNew(con, JIT_ALLOC_IRGEN_OTHER,
				     sizeof(CVMJITIRRange)); 

#if 0
    /* Initialize root list */
    CVMJITirblockInitRootList(con, r);
#endif

    /* Initialize fields in block */
    CVMJITirrangeInit(con, r, pc);

    return r;
}
