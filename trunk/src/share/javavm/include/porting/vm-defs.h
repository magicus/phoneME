/*
 * @(#)vm-defs.h	1.15 06/10/10
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
 */

#ifndef _INCLUDED_PORTING_VM_DEFS_H
#define _INCLUDED_PORTING_VM_DEFS_H

/*
 * These types are exported to the porting layer.
 */

#include "javavm/include/porting/defs.h"

#ifndef _ASM

/*
 * Convenience macros
 */
#define CVM_STRUCT_TYPEDEF(structName) \
    typedef struct structName structName
#define CVM_UNION_TYPEDEF(structName) \
    typedef union structName structName
#define CVM_ENUM_TYPEDEF(structName) \
    typedef enum structName structName

/*
 * C types corresponding to Java types
 */
typedef CVMInt8         CVMJavaByte;
typedef CVMInt16        CVMJavaShort;
typedef CVMUint16       CVMJavaChar;
typedef CVMUint8        CVMJavaBoolean;
typedef CVMInt32        CVMJavaInt;
typedef CVMfloat32      CVMJavaFloat;
typedef CVMInt64        CVMJavaLong;
typedef CVMfloat64      CVMJavaDouble;

typedef CVMUint32	CVMBool;

#define CVM_TRUE	(1 == 1)
#define CVM_FALSE	(!CVM_TRUE)

/* forward declarations, defined by porting layer */

/* sync.h */
CVM_STRUCT_TYPEDEF(CVMMutex);
CVM_STRUCT_TYPEDEF(CVMCondVar);
CVM_STRUCT_TYPEDEF(CVMMicroLock);
#ifdef CVM_ADV_THREAD_BOOST
CVM_STRUCT_TYPEDEF(CVMThreadBoostRecord);
CVM_STRUCT_TYPEDEF(CVMThreadBoostQueue);
#endif

/* threads.h */
CVM_STRUCT_TYPEDEF(CVMThreadID);

/* globals.h */
CVM_STRUCT_TYPEDEF(CVMTargetGlobalState);

#endif /* !_ASM */
#endif /* _INCLUDED_PORTING_VM_DEFS_H */
