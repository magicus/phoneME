/*
 * @(#)jitstackmap.h	1.11 06/10/10
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

#ifndef _INCLUDED_JITSTACKMAP_H
#define _INCLUDED_JITSTACKMAP_H

struct CVMJITStackmapItem {
    CVMUint32		pcOffset;
    CVMJITSet		localMap;
    int			nTemps;
    CVMJITSet		tempMap;
    int			stackDepth;
    int			nParams; /* words of outgoing parameter */
    CVMJITSet		stackMap;
    CVMJITStackmapItem*	next;
};

/*
 * Capture a snapshot of the stackmap at the current pc.
 * pwords is the number of words of parameters, applicable at the
 * point of a method invocation. Should be 0 otherwise.
 */
extern void
CVMJITcaptureStackmap(CVMJITCompilationContext*, int pwords);

/*
 * Construct the stackmap data structures for this method.
 * Returns the address in long-lived memory of the result.
 */
extern void
CVMJITwriteStackmaps(CVMJITCompilationContext*);

/*
 * Returns the logicalPC of the previous stackmap emitted.
 */
extern CVMInt32
CVMJITgetPreviousStackmapLogicalPC(CVMJITCompilationContext*);

#endif /* _INCLUDED_JITSTACKMAP_H */
