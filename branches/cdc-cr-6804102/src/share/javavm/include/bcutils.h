/*
 * @(#)bcutils.h	1.10 06/10/10
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

#ifndef _INCLUDED_BCUTILS_H
#define _INCLUDED_BCUTILS_H

#ifdef CVM_DEBUG
/*
 * Disassemble Java method
 */
extern void
CVMbcList(CVMMethodBlock* mb);

/*
 * Disassemble Java method, with indentation level 'level'.
 * If 'doRecurse' is set, disassembly continues recursively as long as
 * 
 */
extern void
CVMbcListWithIndentation(CVMMethodBlock* mb, CVMUint32 level,
			 CVMBool doRecurse);

/*
 * Disassemble a Java method in pc range [codeStart, codeEnd)
 */
extern void
CVMbcDisassembleRange(CVMMethodBlock* mb,
		      CVMUint8* codeStart,
		      CVMUint8* codeEnd);
#endif

#endif /* _INCLUDED_BCUTILS_H */
