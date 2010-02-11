/*
 * @(#)debug.h	1.6 06/10/10
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

// @(#)debug.h       2.11     93/10/25
#ifndef DEBUGH
#define DEBUGH
extern int debugging;
extern int verbose;

#define D_DUMPTABLES	1
#define D_RULEWORDS	2
#define D_RULETREE	4
#define D_PARENTS	8
#define D_INTERSECT	16
#define D_TRIMMERS	32
#define D_COMBINE	64
#define D_NSTATES	128
#define D_MALLOC	256
#define D_SCAN		512
#define D_PATTERNS	1024
#define D_CLOSURES	2048
#define D_SETS		4096
#define D_COMPRESS	8192
#define D_ANY		(1<<31) /* set if any debugging at all */

#define DEBUG(x) (debugging & D_##x )

/* also declare the primary debugging print entry points */
extern void printrules(void);
extern void printsymbols(int);
extern void printstates(int);
extern void printitems(void);
extern void printclosures(void);
extern void printsetstats(void);

#endif
