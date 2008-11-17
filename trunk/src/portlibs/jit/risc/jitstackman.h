/*
 * @(#)jitstackman.h	1.15 06/10/10
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

#ifndef _INCLUDED_JITSTACKMAN_H
#define _INCLUDED_JITSTACKMAN_H
/*
 * Compile-time management of the run-time Java expression and
 * parameter stack.
 * There will be target-specific things mixed in here, and they will be
 * obvious and easy to separate out.
 *
 */

extern void
CVMSMinit(CVMJITCompilationContext*);


extern void
CVMSMstartBlock(CVMJITCompilationContext*);

/* Purpose: Gets the current stack depth. */
#define CVMSMgetStackDepth(con) ((con)->SMdepth)

/*
 * Make sure that the named expression (which must be on the stack)
 * is the current top-of-stack element.
 */
#ifdef CVM_DEBUG
extern void
CVMSMassertStackTop(CVMJITCompilationContext*, CVMJITIRNode* expr);

#else
#define CVMSMassertStackTop( a, b ) (void)0
#endif

/*
 * push "size" words to the TOS from the given src register(s).
 * Adjusts TOS
 * CVMSMpushSingle for all single-word data types, including ref's
 * CVMSMpushDouble for both double-word types.
 */
extern void
CVMSMpushSingle(CVMJITCompilationContext*, CVMJITRMContext*, CVMRMResource* r);

extern void
CVMSMpushDouble(CVMJITCompilationContext*, CVMJITRMContext*, CVMRMResource* r);

/*
 * Record the effect on the stack of the method call.
 * The first trims the parameters off the stack. It should be
 * called before writing the stack map.
 * The second records pushing on of the return value. It returns a
 * CVMRMResource which is stack bound, or NULL for void methods.
 */
extern void
CVMSMpopParameters(CVMJITCompilationContext*, CVMUint16 numberOfArgs);

/*
 * Make JSP point just past the last argument. This is only needed when
 * postincrement stores are not supported. In this case we defer making
 * any stack adjustments until we are ready to make the method call.
 */
#ifndef CVMCPU_HAS_POSTINCREMENT_STORE
extern void
CVMSMadjustJSP(CVMJITCompilationContext*);
#endif

extern CVMRMResource*
CVMSMinvocation(CVMJITCompilationContext* con, CVMJITRMContext* rc,
		CVMJITIRNode* invokeNode);

/*
 * pop "size" words from the TOS into the given dest register(s).
 * Adjusts TOS.
 * If r == NULL, then JSP is adjusted without any read from memory.
 * CVMSMpopSingle for all single-word data types, including ref's
 * CVMSMpopDouble for both double-word types.
 * NOTE: The resource r must be of type JavaStackTopValue.
 */
extern void
CVMSMpopSingle(CVMJITCompilationContext*, CVMJITRMContext*, CVMRMResource* r);

extern void
CVMSMpopDouble(CVMJITCompilationContext*, CVMJITRMContext*, CVMRMResource* r);

#endif /* _INCLUDED_JITSTACKMAN_H */
