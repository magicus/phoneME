/*
 * @(#)PPCLabelPeer.h	1.6 06/10/10
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
 * @(#)PPCLabelPeer.h	1.6 06/10/10
 */
#ifndef _WINCELABEL_PEER_H_
#define _WINCELABEL_PEER_H_

#include "PPCComponentPeer.h"

#include "java_awt_Label.h"
#include "sun_awt_pocketpc_PPCLabelPeer.h"

//
// Component class for system-provided labels
//
class AwtLabel : public AwtComponent {
public:
    AwtLabel();
    ~AwtLabel();

    virtual const TCHAR* GetClassName();
    
    static AwtLabel* Create( jobject self,
                             jobject hParent );

    // Return the associated AWT peer object.
    INLINE jobject GetPeer() { 
        return m_peerObject; 
    }

    //////// Windows message handler functions
    virtual MsgRouting WmPaint( HDC hDC );
    virtual MsgRouting WmPrintClient( HDC hDC, UINT flags );

private:
    BOOL DoPaint( HDC hDC, RECT& r );
};

#endif
