/*
 * @(#)PPCTextComponentPeer.h	1.7 06/10/10
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
#ifndef _WINCETEXT_COMPONENT_PEER_H_
#define _WINCETEXT_COMPONENT_PEER_H_

#include "PPCComponentPeer.h"
#include "sun_awt_pocketpc_PPCTextComponentPeer.h"

//
// Component base class for system provided text edit controls
//
class AwtTextComponent : public AwtComponent {
public:
    AwtTextComponent();

    virtual const TCHAR* GetClassName();

    static WCHAR *AddCR(WCHAR *pStr, int nStrLen);
    static int RemoveCR(WCHAR *pStr);

    static char *AddCR(char *pStr, int nStrLen);
    static int RemoveCR(char *pStr);
#ifndef WINCE
    INLINE void SetText(const TCHAR *text) {
        ::SetWindowTextA(GetHWnd(), text); 
    }
#endif

    INLINE void SetText(const WCHAR* text) {
        ::SetWindowTextW(GetHWnd(), text); 
    }
#ifndef WINCE
    INLINE int GetText(char* buffer, int size) { 
        return ::GetWindowTextA(GetHWnd(), buffer, size); 
    }
#endif
    INLINE int GetText(WCHAR* buffer, int size) {
        return ::GetWindowTextW(GetHWnd(), buffer, size); 
    }

    // For TextComponents that contains WCHAR strings 
    // or messages with WCHAR parameters.
    INLINE LRESULT SendMessageW(UINT msg, WPARAM wParam = 0, LPARAM lParam = 0) {
        ASSERT(GetHWnd());
        return ::SendMessageW(GetHWnd(), msg, wParam, lParam);
    }

    void SetFont(AwtFont* font);

    //////// Windows message handler functions
    MsgRouting WmNotify(UINT notifyCode);
};

static long getJavaSelPos(AwtTextComponent *c, long orgPos);
static long getWin32SelPos(AwtTextComponent *c, long orgPos);

#endif
