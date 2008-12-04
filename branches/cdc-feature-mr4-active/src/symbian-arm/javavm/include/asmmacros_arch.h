/*
 * @(#)asmmacros_arch.h	1.6 05/01/13
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

#ifndef _INCLUDED_ASMMACROS_ARCH_H
#define _INCLUDED_ASMMACROS_ARCH_H

#ifndef __RVCT__
#undef  POUND
#define POUND #

#define ARM /* empty */

#define PRESERVE8 /* empty */

#define SET_SECTION_EXEC(x)

#define SET_SECTION_EXEC_WRITE
	
#define ENTRY(x)		\
	.align 2;		\
	.global x;		\
	x:

#define ENTRY1(x)

#define SET_SIZE(x)

#define ALIGN(n)		\
	.align n

#define LABEL(x)		\
	x:

#define WORD(x)			\
	.word x

#define BYTE(x)			\
	.byte x

#define STRING(s)		\
	.string s

#define EQUIV(this, that)	\
	.equiv this, that

#define POOL			\
	.pool

#define END /* empty */

#define IMPORT(x)

#else /* __RVCT__ */

#define ARM /* ARM */

#define PRESERVE8 PRESERVE8

#define SET_SECTION_EXEC(x)	  \
	AREA    x, CODE, READONLY

#define LABEL(x)		\
	x

#define ENTRY0(x)		\
	EXPORT x

#define ENTRY(x)		\
	EXPORT x

#define ENTRY1(x)	LABEL(x)

#define SET_SIZE(x)

#define ALIGN(n)		\
	ALIGN 1 :SHL: (n)

#define WORD(x)			\
	DCD x

#define BYTE(x)			\
	DCB x

#define STRING(s)		\
	DCB s, 0

#define EQUIV(this, that)	\
	this EQU that

#define POOL

#define IMPORT(x)       IMPORT x

#define END END

#endif /* __RVCT__ */

#endif /* _INCLUDED_ASMMACROS_ARCH_H */
