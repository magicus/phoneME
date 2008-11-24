/*
 * @(#)basictypes.c	1.19 06/10/10
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

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/typeid.h"
#include "javavm/include/basictypes.h"
#include "javavm/include/signature.h"
#include "javavm/include/preloader.h"

/*
 * A mapping from basic type to size in bytes
 */
const CVMUint32 CVMbasicTypeSizes[] = {
    0, /* unused */
    0, /* unused */
    sizeof(CVMObjectICell),  /* CVM_T_CLASS */
    0, /* unused */
    sizeof(CVMJavaBoolean),  /* CVM_T_BOOLEAN */
    sizeof(CVMJavaChar),     /* CVM_T_CHAR */
    sizeof(CVMJavaFloat),    /* CVM_T_FLOAT */
    sizeof(CVMTwoJavaWords), /* CVM_T_DOUBLE */
    sizeof(CVMJavaByte),     /* CVM_T_BYTE */
    sizeof(CVMJavaShort),    /* CVM_T_SHORT */
    sizeof(CVMJavaInt),	     /* CVM_T_INT */
    sizeof(CVMTwoJavaWords), /* CVM_T_LONG */
    0, /* unused */
    0, /* unused */
    0, /* unused */
    0, /* unused */
    0, /* unused */
    0  /* CVM_T_VOID */
};

/*
 * A mapping from basic type to signatures in bytes
 */
const char CVMbasicTypeSignatures[] = {
    0, /* unused */
    0, /* unused */
    CVM_SIGNATURE_CLASS,   /* CVM_T_CLASS */
    0, /* unused */
    CVM_SIGNATURE_BOOLEAN, /* CVM_T_BOOLEAN */
    CVM_SIGNATURE_CHAR,    /* CVM_T_CHAR */
    CVM_SIGNATURE_FLOAT,   /* CVM_T_FLOAT */
    CVM_SIGNATURE_DOUBLE,  /* CVM_T_DOUBLE */
    CVM_SIGNATURE_BYTE,    /* CVM_T_BYTE */
    CVM_SIGNATURE_SHORT,   /* CVM_T_SHORT */
    CVM_SIGNATURE_INT,     /* CVM_T_INT */
    CVM_SIGNATURE_LONG,    /* CVM_T_LONG */
    0, /* unused */
    0, /* unused */
    0, /* unused */
    0, /* unused */
    0, /* unused */
    CVM_SIGNATURE_VOID 	   /* CVM_T_VOID */
};

/*
 * A mapping from basic type to class type ID
 */
const CVMClassTypeID CVMbasicTypeID[] = {
    0, /* unused */
    0, /* unused */
    CVM_TYPEID_OBJ,   /* CVM_T_CLASS */
    0, /* unused */
    CVM_TYPEID_BOOLEAN, /* CVM_T_BOOLEAN */
    CVM_TYPEID_CHAR,    /* CVM_T_CHAR */
    CVM_TYPEID_FLOAT,   /* CVM_T_FLOAT */
    CVM_TYPEID_DOUBLE,  /* CVM_T_DOUBLE */
    CVM_TYPEID_BYTE,    /* CVM_T_BYTE */
    CVM_TYPEID_SHORT,   /* CVM_T_SHORT */
    CVM_TYPEID_INT,     /* CVM_T_INT */
    CVM_TYPEID_LONG,    /* CVM_T_LONG */
    0, /* unused */
    0, /* unused */
    0, /* unused */
    0, /* unused */
    0, /* unused */
    CVM_TYPEID_VOID 	   /* CVM_T_VOID */
};

/*
 * A mapping from basic type to array class of the basic type.
 */
const CVMClassBlock* const CVMbasicTypeArrayClassblocks[] = {
    0, /* unused */
    0, /* unused */
    CVMsystemArrayClassOf(Object),  /* A generic objects array */
    0, /* unused */
    CVMsystemArrayClassOf(Boolean), /* CVM_T_BOOLEAN */
    CVMsystemArrayClassOf(Char),    /* CVM_T_CHAR */
    CVMsystemArrayClassOf(Float),   /* CVM_T_FLOAT */
    CVMsystemArrayClassOf(Double),  /* CVM_T_DOUBLE */
    CVMsystemArrayClassOf(Byte),    /* CVM_T_BYTE */
    CVMsystemArrayClassOf(Short),   /* CVM_T_SHORT */
    CVMsystemArrayClassOf(Int),     /* CVM_T_INT */
    CVMsystemArrayClassOf(Long),    /* CVM_T_LONG */
    0, /* unused */
    0, /* unused */
    0, /* unused */
    0, /* unused */
    0, /* unused */
    0  /* CVM_T_VOID no such thing as array of void! */
};

/*
 * A mapping from basic type to primitive class for basic type
 */
const CVMClassBlock* const CVMbasicTypeClassblocks[] = {
    0, /* unused */
    0, /* unused */
    0, /* unused */
    0, /* unused */
    CVMprimitiveClass(boolean), /* CVM_T_BOOLEAN */
    CVMprimitiveClass(char),    /* CVM_T_CHAR */
    CVMprimitiveClass(float),   /* CVM_T_FLOAT */
    CVMprimitiveClass(double),  /* CVM_T_DOUBLE */
    CVMprimitiveClass(byte),    /* CVM_T_BYTE */
    CVMprimitiveClass(short),   /* CVM_T_SHORT */
    CVMprimitiveClass(int),     /* CVM_T_INT */
    CVMprimitiveClass(long),    /* CVM_T_LONG */
    0, /* unused */
    0, /* unused */
    0, /* unused */
    0, /* unused */
    0, /* unused */
    CVMprimitiveClass(void),    /* CVM_T_VOID */
};

/*
 * A mapping from terse type to primitive class for basic type
 */
const CVMClassBlock* const CVMterseTypeClassblocks[] = {
    0, /* ERROR */
    0, /* ERROR */
    CVMprimitiveClass(void),    /* CVM_TYPEID_VOID */
    CVMprimitiveClass(int),     /* CVM_TYPEID_INT */
    CVMprimitiveClass(short),   /* CVM_TYPEID_SHORT */
    CVMprimitiveClass(char),    /* CVM_TYPEID_CHAR */
    CVMprimitiveClass(long),    /* CVM_TYPEID_LONG */
    CVMprimitiveClass(byte),    /* CVM_TYPEID_BYTE */
    CVMprimitiveClass(float),   /* CVM_TYPEID_FLOAT */
    CVMprimitiveClass(double),  /* CVM_TYPEID_DOUBLE */
    CVMprimitiveClass(boolean)  /* CVM_TYPEID_BOOLEAN */
};

/*
 * A mapping from terse type to primitive class for basic type
 */
const CVMBasicType CVMterseTypeBasicTypes[] = {
    CVM_T_ERR,	   /* ERROR */
    CVM_T_ERR,	   /* ERROR */
    CVM_T_VOID,    /* CVM_TYPEID_VOID */
    CVM_T_INT,     /* CVM_TYPEID_INT */
    CVM_T_SHORT,   /* CVM_TYPEID_SHORT */
    CVM_T_CHAR,    /* CVM_TYPEID_CHAR */
    CVM_T_LONG,    /* CVM_TYPEID_LONG */
    CVM_T_BYTE,    /* CVM_TYPEID_BYTE */
    CVM_T_FLOAT,   /* CVM_TYPEID_FLOAT */
    CVM_T_DOUBLE,  /* CVM_TYPEID_DOUBLE */
    CVM_T_BOOLEAN, /* CVM_TYPEID_BOOLEAN */
    CVM_T_CLASS    /* CVM_TYPEID_OBJ */
};

const char CVMterseTypePrimitiveSignatures[] = {
    0, /* ERROR */
    0, /* ERROR */
    CVM_SIGNATURE_VOID,    /* CVM_TYPEID_VOID */
    CVM_SIGNATURE_INT,     /* CVM_TYPEID_INT */
    CVM_SIGNATURE_SHORT,   /* CVM_TYPEID_SHORT */
    CVM_SIGNATURE_CHAR,    /* CVM_TYPEID_CHAR */
    CVM_SIGNATURE_LONG,    /* CVM_TYPEID_LONG */
    CVM_SIGNATURE_BYTE,    /* CVM_TYPEID_BYTE */
    CVM_SIGNATURE_FLOAT,   /* CVM_TYPEID_FLOAT */
    CVM_SIGNATURE_DOUBLE,  /* CVM_TYPEID_DOUBLE */
    CVM_SIGNATURE_BOOLEAN, /* CVM_TYPEID_BOOLEAN */
    CVM_SIGNATURE_CLASS    /* CVM_TYPEID_OBJ */
};
