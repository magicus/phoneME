/*
 * @(#)call_via.S	1.3	06/10/10
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

#include "javavm/include/asmmacros_cpu.h"

        .file           "call_via.S"

/* Trampoline code for calling from THUMB code via a register.
   Some THUMBs don't have blx (branch,link,exchange) so the compiler
   generates bl to one of these instructions, which do the bx
*/
ENTRY ( _call_via_r0 )
        @ r0 = addr of callee
        bx	r0
	b	1f	/*  try to detect return/fall thru */

ENTRY ( _call_via_r1 )
        @ r1 = addr of callee
        bx	r1
	b	1f	/*  try to detect return/fall thru */

ENTRY ( _call_via_r2 )
        @ r2 = addr of callee
        bx	r2
1:	b	1b	/*  try to detect return/fall thru */
ENTRY ( _call_via_r3 )
        @ r3 = addr of callee
        bx	r3
2:	b	2b	/*  try to detect return/fall thru */
ENTRY ( _call_via_r4 )
        @ r4 = addr of callee
        bx	r4
3:	b	3b	/*  try to detect return/fall thru */
ENTRY ( _call_via_r5 )
        @ r5 = addr of callee
        bx	r5
4:	b	4b	/*  try to detect return/fall thru */
ENTRY ( _call_via_r6 )
        @ r6 = addr of callee
        bx	r6
5:	b	5b	/*  try to detect return/fall thru */

ENTRY ( _call_via_r7 )
        @ r7 = addr of callee
        bx	r7
	b	5b	/*  try to detect return/fall thru */

ENTRY ( _call_via_r8 )
        @ r8 = addr of callee
        bx	r8
	b	5b	/*  try to detect return/fall thru */

ENTRY ( _call_via_r9 )
        @ r9 = addr of callee
        bx	r9
	b	5b	/*  try to detect return/fall thru */

ENTRY ( _call_via_fp )
        @ fp = addr of callee
        bx	fp
	b	5b	/*  try to detect return/fall thru */

ENTRY ( _call_via_ip )
        @ ip = addr of callee
        bx	ip
	b	5b	/*  try to detect return/fall thru */

ENTRY ( _call_via_sl )
        @ sl = addr of callee
        bx	sl
	b	5b	/*  try to detect return/fall thru */

