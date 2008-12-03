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

#include "PPCPen.h"

Hashtable AwtPen::cache("Pen cache", ReleaseObject);

AwtPen::AwtPen(COLORREF color) : AwtGDIObject(color, SafeCreatePen(PS_SOLID, 1, color)) {
}

AwtPen* AwtPen::Get(COLORREF color) {
    AwtPen* obj = (AwtPen*)cache.get((void*)color);
    if (obj == NULL) {
        obj = new AwtPen(color);
        VERIFY(cache.put((void*)color, obj) == NULL);
    }
    obj->IncrRefCount();
    return obj;
}

HPEN AwtPen::SafeCreatePen(int style, int width, COLORREF cr) {
    //!CQ could do partial flushes more often, based on LRU or refcount == 1, etc
    HPEN hp = ::CreatePen(PS_SOLID, 1, cr);
    if (!hp) {
        // Flush cache (it calls back to Release each pen in it). Any current references to those
        // pens will remain valid until the last Release is called
        cache.clear();
        hp = ::CreatePen(PS_SOLID, 1, cr);
        // if that didn't work, then try a gc to cleanup potential java objects 
        if (!hp) {
            ForceGarbageCollection();
            hp = ::CreatePen(PS_SOLID, 1, cr);
            ASSERT(hp != NULL);
        }
    }
    return hp;
}
