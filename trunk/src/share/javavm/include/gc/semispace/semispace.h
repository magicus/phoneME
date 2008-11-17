/*
 * @(#)semispace.h	1.9 06/10/10
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
 * This file is specific and only visible to semi-space copying GC
 * implementation.  
 */

#ifndef _INCLUDED_SEMISPACE_H
#define _INCLUDED_SEMISPACE_H

typedef struct CVMSsHeap CVMSsHeap;
typedef struct CVMSsSemispace CVMSsSemispace;

/*
 * The structure of the heap in a semi-space copying collector
 */
struct CVMSsHeap {
    CVMUint32       size;     /* Size of heap in no. of bytes */
    CVMSsSemispace* from;     /* The from-space */
    CVMSsSemispace* to;       /* The to-space */
    CVMUint32       sizeOfBitmaps; /* The size in bytes of the following map */
    CVMUint32*      forwarded;/* A bitmap indicating forwarded objects */
};

/*
 * The structure of a semi-space.
 */
struct CVMSsSemispace {
    CVMUint32* allocPtr;  /* The current allocation pointer */
    CVMUint32* topData;   /* The top of the heap */
    CVMUint32* threshold; /* The allocation threshold for GC */
    CVMUint32  data[1];   /* The data for this semi-space */
};

#endif /* _INCLUDED_SEMISPACE_H */

