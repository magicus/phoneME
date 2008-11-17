/*
 * @(#)jitirlist.c	1.7 06/10/10
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

#include "javavm/include/assert.h"
#include "javavm/include/jit/jitirlist.h"

/*
 * CVMJITIRList implementation
 */

void
CVMJITirlistPrepend(CVMJITCompilationContext* con, CVMJITIRList* lst,
		    void* item)
{
    if (CVMJITirlistIsEmpty(lst)) {
        lst->head = lst->tail = item;
        lst->cnt = 1;
    } else {
        CVMJITIRListItem* cur = lst->head;
        cur->prev = item;
        ((CVMJITIRListItem*)item)->next = cur;
        lst->cnt++;
        lst->head = item;
    }
}

void
CVMJITirlistAppend(CVMJITCompilationContext* con, CVMJITIRList* lst,
                   void* item)
{
    if (CVMJITirlistIsEmpty(lst)) {
        lst->head = lst->tail = item;

        lst->cnt = 1;
    } else {
        CVMJITIRListItem* cur = lst->tail;
        cur->next = item;
        ((CVMJITIRListItem*)item)->prev = cur;
        lst->cnt++;
        lst->tail = item;
    }
}

void
CVMJITirlistRemove(CVMJITCompilationContext* con, CVMJITIRList* lst,
                   void* item0)
{
    CVMJITIRListItem* item = item0;
    CVMassert(!CVMJITirlistIsEmpty(lst));
    if (lst->head == item0) {
	lst->head = item->next;
    } else {
	((CVMJITIRListItem *)item->prev)->next = item->next;
    }
    if (lst->tail == item0) {
	lst->tail = item->prev;
    } else {
	((CVMJITIRListItem *)item->next)->prev = item->prev;
    }
    --lst->cnt;
}

void
CVMJITirlistAppendListAfter(CVMJITCompilationContext* con, CVMJITIRList* to,
			    void *item0, CVMJITIRList* from)
{
    if (!CVMJITirlistIsEmpty(from)) {
	CVMJITIRListItem *item = (CVMJITIRListItem *)item0;
	CVMJITIRListItem *next = item->next;
	((CVMJITIRListItem *)from->head)->prev = item;
	item->next = from->head;
	if (next != NULL) {
	    ((CVMJITIRListItem *)from->tail)->next = next;
	    next->prev = from->tail;
	} else {
	    CVMassert(item == to->tail);
	    to->tail = from->tail;
	}
	to->cnt += from->cnt;
	from->cnt = 0;
	from->head = NULL;
	from->tail = NULL;
    }
}

void
CVMJITirlistAppendList(CVMJITCompilationContext* con, CVMJITIRList* to,
		       CVMJITIRList* from)
{
    if (CVMJITirlistIsEmpty(to)) {
	*to = *from;
	from->cnt = 0;
	from->head = NULL;
	from->tail = NULL;
    } else {
	CVMJITirlistAppendListAfter(con, to, to->tail, from);
    }
}

CVMBool
CVMJITirlistInsert(CVMJITCompilationContext* con, CVMJITIRList* list,
                   void* curItem, void* newItem)
{
    CVMBool istail = CVM_FALSE;
    CVMJITIRListItem* curListItem = (CVMJITIRListItem*)curItem;
    CVMJITIRListItem* newListItem = (CVMJITIRListItem*)newItem;

    CVMassert((list != NULL) && (curItem != NULL) && (newItem != NULL));
    
    if (curListItem->next == NULL) {
        curListItem->next = newListItem;
        newListItem->prev = curListItem;
        newListItem->next = NULL;
        list->tail = newListItem;
        istail = CVM_TRUE;
    } else {
        newListItem->next = curListItem->next;
        newListItem->prev = curListItem;
        ((CVMJITIRListItem*)curListItem->next)->prev = newListItem;
        curListItem->next = newListItem;
    }
    list->cnt++;
    return istail;
}

void
CVMJITirlistInit(CVMJITCompilationContext* con, CVMJITIRList* irList)
{
    irList->head = irList->tail = NULL;
    irList->cnt = 0;
}
