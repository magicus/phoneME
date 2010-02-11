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

#ifndef _WINCE_PEN_H
#define _WINCE_PEN_H

#include "PPCGDIObject.h"
#include "Hashtable.h"

/*
 * An AwtPen is a cached Windows pen.
 */
class AwtPen : public AwtGDIObject {
public:
    /*
     * Get a GDI object from its respective cache.  If it doesn't exist
     * it gets created, otherwise its reference count gets bumped.
     * Release() must be called when the pen is no longer needed
     */
    static AwtPen* Get(COLORREF color);

private:
    AwtPen(COLORREF color);

    static Hashtable cache;

    HPEN SafeCreatePen(int style, int width, COLORREF cr);
};

#endif // _WINCE_PEN_H
