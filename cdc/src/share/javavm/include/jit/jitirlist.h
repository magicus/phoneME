/*
 * @(#)jitirlist.h	1.7 06/10/10
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

#ifndef _INCLUDED_JITIRLIST_H
#define _INCLUDED_JITIRLIST_H

#include "javavm/include/defs.h"
#include "javavm/include/jit/jit_defs.h"

/*
 * CVMJITIRList macros and APIs.
 *
 * A generic list index used to keep the state of blocks in a method, or
 * keep the state of root nodes in a block.
 */

#define CVMJITirlistIsEmpty(lst) \
    (CVMassert(lst != NULL), (lst)->cnt == 0) 

#define CVMJITirlistSetEmpty(lst) {	\
    CVMassert((lst) != NULL); 		\
    (lst)->cnt = 0;			\
    (lst)->head = (lst)->tail = NULL; 	\
} 

#define CVMJITirlistGetHead(lst)           ((lst)->head)
#define CVMJITirlistGetTail(lst)           ((lst)->tail)
#define CVMJITirlistGetCnt(lst)            ((lst)->cnt)
#define CVMJITirlistSetTail(lst, item)     ((lst)->tail = item)
#define CVMJITirlistIncCnt(lst)            ((lst)->cnt++)
#define CVMJITirlistDecCnt(lst)            ((lst)->cnt--)

extern void 
CVMJITirlistInit(CVMJITCompilationContext* con, CVMJITIRList* lst);

extern void
CVMJITirlistPrepend(CVMJITCompilationContext* con, CVMJITIRList* lst,
    void* item);

extern void
CVMJITirlistAppend(CVMJITCompilationContext* con, CVMJITIRList* lst,
    void* item);

extern CVMBool
CVMJITirlistInsert(CVMJITCompilationContext* con, CVMJITIRList* list,
    void* curItem, void* newItem);

void
CVMJITirlistRemove(CVMJITCompilationContext* con, CVMJITIRList* lst,
    void* item);

void
CVMJITirlistAppendList(CVMJITCompilationContext* con, CVMJITIRList* to,
		    	CVMJITIRList* from);

void
CVMJITirlistAppendListAfter(CVMJITCompilationContext* con, CVMJITIRList* to,
                            void *item, CVMJITIRList* from);
 
struct CVMJITIRList {
    void*     head;
    void*     tail;
    CVMUint16 cnt;
};

struct CVMJITIRListItem {
    void* next;
    void* prev;
};

#endif /* _INCLUDED_JITIRLIST_H */
