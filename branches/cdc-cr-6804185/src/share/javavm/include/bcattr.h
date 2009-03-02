/*
 * @(#)bcattr.h	1.16 06/10/10
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

/*
 * The byte-code attributes (bcAttr) interface. Used for inquiring about
 * various opcode properties.
 */

#ifndef _INCLUDED_BCATTR_H
#define _INCLUDED_BCATTR_H

extern const CVMUint16 CVMbcAttributes[256];

#define CVM_BC_ATT_BIT(x)             (1 << (x))

#define CVM_BC_ATT_GCPOINT            CVM_BC_ATT_BIT(0)
#define CVM_BC_ATT_BRANCH             CVM_BC_ATT_BIT(1)
#define CVM_BC_ATT_THROWSEXCEPTION    CVM_BC_ATT_BIT(2)
#define CVM_BC_ATT_NOCONTROLFLOW      CVM_BC_ATT_BIT(3)
#define CVM_BC_ATT_INVOCATION         CVM_BC_ATT_BIT(4)
#define CVM_BC_ATT_COND_GCPOINT       CVM_BC_ATT_BIT(5)
#define CVM_BC_ATT_QUICK              CVM_BC_ATT_BIT(6)
#define CVM_BC_ATT_RETURN             CVM_BC_ATT_BIT(7)
#define CVM_BC_ATT_FP                 CVM_BC_ATT_BIT(8)

#define CVMbcAttr(opcode, attr) \
    ((CVMbcAttributes[(opcode)] & CVM_BC_ATT_##attr) != 0)

#endif /* _INCLUDED_BCATTR_H */
