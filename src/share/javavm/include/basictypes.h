/*
 * @(#)basictypes.h	1.18 06/10/10
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
 * This file gives some basic types, and some info associated with
 * basic types as defined by the VM spec 
 */
#ifndef _INCLUDED_BASICTYPES_H
#define _INCLUDED_BASICTYPES_H

#include "javavm/include/defs.h"
#include "javavm/include/typeid.h"

enum CVMBasicType {
    CVM_T_ERR = 0,
    /*
     * These basic types are straight from the VM spec
     */
    CVM_T_CLASS = 2,
    CVM_T_BOOLEAN = 4,
    CVM_T_CHAR,
    CVM_T_FLOAT,
    CVM_T_DOUBLE,
    CVM_T_BYTE,
    CVM_T_SHORT,
    CVM_T_INT,
    CVM_T_LONG,
    CVM_T_VOID = 17,
    /* The following is needed to make sure CVMBasicType is 32-bits on all
     * platforms. Otherwise on some platforms OFFSET_CVMArrayInfo_elementCb
     * is not correct.
     */
    CVM_T_FORCE_32_BIT = 0x80000000
};

typedef enum CVMBasicType CVMBasicType;

/*
 * Now some data declarations
 */

/*
 * A mapping from basic type to size in bytes
 */
extern const CVMUint32 CVMbasicTypeSizes[];

/*
 * A mapping from basic type to one char signature
 */
extern const char CVMbasicTypeSignatures[];

/*
 * A mapping from basic type to FieldTypeID
 */
extern const CVMClassTypeID CVMbasicTypeID[];

/*
 * A mapping from basic type to primitive class for basic type
 */
extern const CVMClassBlock* const CVMbasicTypeClassblocks[];

/*
 * A mapping from basic type to array class of basic type 
 */
extern const CVMClassBlock* const CVMbasicTypeArrayClassblocks[];

/*
 * A mapping from terse type to primitive class for basic type
 */
extern const CVMClassBlock* const CVMterseTypeClassblocks[];

/*
 * A mapping from terse type to basic type
 */
extern const CVMBasicType CVMterseTypeBasicTypes[];

/*
 * A mapping from terse type to signature character. Only works for
 * typeids representing fields and only for primitive types.
 */
extern const char CVMterseTypePrimitiveSignatures[];

#endif /* _INCLUDED_BASICTYPES_H */

