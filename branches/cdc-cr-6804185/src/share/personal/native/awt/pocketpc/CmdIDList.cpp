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

#include "CmdIDList.h"

#define MIN_WIN_CMD_ID ((UINT)1)
#define MAX_WIN_CMD_ID ((UINT)65535)


AwtCmdIDList::AwtCmdIDList() : m_lock("AwtCmdIDList")
{
    m_size = 0;
    m_capacity = 10;
    m_array = new AwtObject*[m_capacity];
    ASSERT(m_array != NULL);
}

AwtCmdIDList::~AwtCmdIDList() 
{
    delete (m_array);
}

// Add an object to the end of the vector, increasing its capacity if necessary
UINT AwtCmdIDList::Add(AwtObject* obj)
{
    CriticalSection::Lock l(m_lock);
    if (m_size == m_capacity) {
        m_capacity += 10;
        m_array = (AwtObject**)realloc(m_array, m_capacity * sizeof(void*));
        ASSERT(m_array != NULL);
    }
    UINT id = MIN_WIN_CMD_ID + m_size++;
    if (id >= MAX_WIN_CMD_ID) {
    // part of fix 4133279
    // if we exceed Windows limit of 64K for an id (unlikely, but possible), recycle command id's
	m_size = 0;
	id = MIN_WIN_CMD_ID;
    }
    m_array[id - MIN_WIN_CMD_ID] = obj;
    return id;
}

// Return the associated object.
AwtObject* AwtCmdIDList::Lookup(UINT id)
{
    CriticalSection::Lock l(m_lock);
    ASSERT(id <= m_size);
    return m_array[id - MIN_WIN_CMD_ID];
}

// Erase the object association, but make no attempt to reclaim the space.
// (We'll never allocate that many ids that it will be an issue.)
void AwtCmdIDList::Remove(UINT id)
{
    CriticalSection::Lock l(m_lock);
    m_array[id] = NULL;
}
