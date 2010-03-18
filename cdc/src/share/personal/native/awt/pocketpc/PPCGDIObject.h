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

#ifndef _WINCE_GDIOBJECT_H
#define _WINCE_GDIOBJECT_H

#include "awt.h"
#include "Hashtable.h"

/*
 * An AwtGDIObject is a cached, color-based GDI object, such as a pen or
 * brush.
 */
class AwtGDIObject {
public:
    INLINE COLORREF GetColor() { return m_color; }
    INLINE HGDIOBJ GetHandle() { return m_handle; }

    /*
     * Decrement the reference count of a GDI object.  When it hits 
     * zero, delete both the GDI object and this wrapper.
     */
    INLINE void Release() {
        if (m_refCount > 0 && ::InterlockedDecrement(&m_refCount) == 0)
            delete this;
    }

protected:
    INLINE BOOL IncrRefCount() {
        // If the object is already invalid (countReferences is zero)
        // we can't use the object
        if (!m_refCount)
            return FALSE;
        // Increase the number of references
        ::InterlockedIncrement(&m_refCount);
        return TRUE;
    }

    INLINE AwtGDIObject(COLORREF color, HGDIOBJ  handle) {
        m_color = color;
        m_handle = handle;
        m_refCount = 1;   // refcount starts at one since creator will own us initially
    }

    virtual ~AwtGDIObject() {
        if (m_handle != NULL) {
            ::DeleteObject(m_handle);
        }
    }

    /*
     * Callback used by Hashtable when it wants to let go of an object
     */
    static void ReleaseObject(void* obj);

    /*
     * Call back up to java to gc and finalize in order to indirectly free up GDI handles
     */
    static void ForceGarbageCollection();
    
private:
    COLORREF m_color;
    HGDIOBJ  m_handle;
    long     m_refCount;
};

#endif // _WINCE_GDIOBJECT_H
