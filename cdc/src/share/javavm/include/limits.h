/*
 * @(#)limits.h	1.7 06/10/10
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
 * This file states values for our implementation limits.
 */

#ifndef _INCLUDED_LIMITS_H
#define _INCLUDED_LIMITS_H

#include "javavm/include/defs.h"
#include "javavm/include/typeid.h"
#include "javavm/include/constantpool.h"

/*
 * An object can't be larger than 16K-1 words long, including the header.
 * Bigger objects cannot encode their instance sizes in the 16-bit
 * class instance size field.
 */
#define CVM_LIMIT_OBJECT_NUMFIELDWORDS \
    ((16 * 1024) - (sizeof(CVMObjectHeader) / sizeof(CVMObject*)) - 1)

/*
 * A method table cannot include more than 65535 entries
 */
#define CVM_LIMIT_NUM_METHODTABLE_ENTRIES 65535

#endif /* _INCLUDED_LIMITS_H */
