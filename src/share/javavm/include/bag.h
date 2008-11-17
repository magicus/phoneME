/*
 * @(#)bag.h	1.10 06/10/10
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

#ifndef _INCLUDED_BAG_H
#define _INCLUDED_BAG_H

#include "javavm/include/defs.h"

/* Declare general routines for manipulating a bag data structure.
 * Synchronized use is the responsibility of caller.
 */

struct CVMBag;

/* Must be used to create a bag.  itemSize is the size
 * of the items stored in the bag. initialAllocation is a hint
 * for the initial number of items to allocate. Returns the
 * allocated bag, returns NULL if out of memory.
 */
struct CVMBag *CVMbagCreateBag(int itemSize, int initialAllocation);

/* Destroy the bag and reclaim the space it uses.
 */
void CVMbagDestroyBag(struct CVMBag *theBag);

/* Find 'key' in bag.  Assumes first entry in item is a pointer.
 * Return found item pointer, NULL if not found. 
 */
void *CVMbagFind(struct CVMBag *theBag, void *key);

/* Add space for an item in the bag.
 * Return allocated item pointer, NULL if no memory. 
 */
void *CVMbagAdd(struct CVMBag *theBag);

/* Delete specified item from bag. 
 * Does no checks.
 */
void CVMbagDelete(struct CVMBag *theBag, void *condemned);

/* Delete all items from the bag.
 */
void CVMbagDeleteAll(struct CVMBag *theBag);

/* Return the count of items stored in the bag.
 */
int CVMbagSize(struct CVMBag *theBag);

/* Enumerate over the items in the bag, calling 'func' for 
 * each item.  The function is passed the item and the user 
 * supplied 'arg'.  Abort the enumeration if the function
 * returns FALSE.  Return TRUE if the enumeration completed
 * successfully and FALSE if it was aborted.
 * Addition and deletion during enumeration is not supported.
 */
typedef CVMBool (*CVMBagEnumerateFunction)(void *item, void *arg);

CVMBool CVMbagEnumerateOver(struct CVMBag *theBag, 
			    CVMBagEnumerateFunction func, void *arg);

#endif /* !_INCLUDED_BAG_H */
