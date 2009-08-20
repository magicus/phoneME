/*
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

#include "ObjectList.h"

///////////////////////////////////////////////////////////////////////////
// AwtObject list -- track all created widgets for cleanup and debugging

AwtObjectList theAwtObjectList;

AwtObjectList::AwtObjectList() : m_lock("AwtObjectList")
{
    m_head = NULL;
}

void AwtObjectList::Add(AwtObject* obj)
{
    CriticalSection::Lock l(m_lock);

    AwtObjectListItem* item = new AwtObjectListItem(obj);
    item->next = theAwtObjectList.m_head;
    theAwtObjectList.m_head = item;
}

void AwtObjectList::Remove(AwtObject* obj)
{
    CriticalSection::Lock l(m_lock);

    AwtObjectListItem* item = theAwtObjectList.m_head;
    AwtObjectListItem* lastItem = NULL;

    while (item != NULL) {
	if (item->obj == obj) {
	    if (lastItem == NULL) {
		theAwtObjectList.m_head = item->next;
	    } else {
		lastItem->next = item->next;
	    }
            ASSERT(item != NULL);
	    delete item;
	    return;
	}
	lastItem = item;
	item = item->next;
    }
// removed assert. if item previously removed from list it 
// shouldn't be a fatal error. Even in debug mode.
//    ASSERT(FALSE);  // should never get here...
}

void AwtObjectList::Cleanup()
{
    ASSERT(GetCurrentThreadId() == AwtToolkit::MainThread());

    CriticalSection::Lock l(theAwtObjectList.m_lock);

    AwtObjectListItem* item = theAwtObjectList.m_head;
    if (item != NULL) {
	while (item != NULL) {
            // The AwtObject's destructor will call AwtObjectList::Remove(),
            // which will delete the item structure.
	    AwtObjectListItem* next = item->next;
            delete item->obj;
	    item = next;
	}
    }
    theAwtObjectList.m_head = NULL;
}
