/*
 * @(#)jitirdump.h	1.15 06/10/10
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

#ifndef _INCLUDED_JITIRDUMP_H
#define _INCLUDED_JITIRDUMP_H

#include "javavm/include/jit/jit_defs.h"

#ifdef CVM_TRACE_JIT

/* Purpose: Dumps the specified expression tree.  If a sub-expression tree has
            already been dumped previously, then only the node at the root of
            the sub-expression will be dumped. */
extern void 
CVMJITirdumpIRNode(CVMJITCompilationContext* con, CVMJITIRNode* node,
		   int indentCount, const char *prefix);

/* Purpose: Dumps the specified expression tree, and always fully dump every
            sub-expressions.  This dumper is usually used by error reporting
            code that wants to show the entire expression tree regardless of
            whether parts of it has been dumped before or not. */
extern void 
CVMJITirdumpIRNodeAlways(CVMJITCompilationContext* con, CVMJITIRNode* node,
			 int indentCount, const char *prefix);

extern void
CVMJITirdumpIRRootList(CVMJITCompilationContext* con, 
		       CVMJITIRBlock* bk);

extern void
CVMJITirdumpIRBlockList(CVMJITCompilationContext* con, 
			CVMJITIRList* bkList);

extern void
CVMJITirdumpIRListPrint(CVMJITCompilationContext* con, 
			CVMJITIRList* rlst);

#endif /* CVM_TRACE_JIT */

#endif /* _INCLUDED_JITIRDUMP_H */
