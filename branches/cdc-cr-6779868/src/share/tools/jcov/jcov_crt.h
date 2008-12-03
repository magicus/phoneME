/*
 * @(#)jcov_crt.h	1.7 06/10/10
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

#ifndef _JCOV_CRT_H
#define _JCOV_CRT_H

#include "jcov.h"
#include "jcov_util.h"
#include "jcov_types.h"

#define CRT_ENTRY_PACK_BITS 10;
#define CRT_ENTRY_POS_MASK  0x3FF;

#define JDK11_CT_ENTRY_PACK_BITS 18
#define JDK11_CT_ENTRY_POS_MASK  0x3FFFF

void read_crt_table(int attr_len, jcov_method_t *meth, bin_class_context_t *c);
void read_cov_table(int attr_len, jcov_method_t *meth, bin_class_context_t *c);
cov_item_t *cov_item_new(UINT16 pc, UINT8 type, UINT8 instr_type, INT32 line, INT32 pos);

#endif /* _JCOV_CRT_H */
