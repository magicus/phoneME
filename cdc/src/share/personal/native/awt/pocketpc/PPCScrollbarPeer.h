/*
 * @(#)PPCScrollbarPeer.h	1.6 06/10/10
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
#ifndef _WINCESCROLLBAR_PEER_H_
#define _WINCESCROLLBAR_PEER_H_

#include "PPCComponentPeer.h"

#include "java_awt_Scrollbar.h"
#include "sun_awt_pocketpc_PPCScrollbarPeer.h"

//
// Component class for system provided scrollbar controls
//
class AwtScrollbar : public AwtComponent {
public:
    AwtScrollbar();

    virtual const TCHAR* GetClassName();
    
    static AwtScrollbar* Create(jobject self, jobject hParent);

    // Return the associated AWT peer object.
    INLINE jobject GetPeer() { 
	return m_peerObject; 
    }

    void SetValue(int value);
    void SetValues(int value, int visible, int minimum, int maximum);
    void SetLineIncrement(int value) { m_lineIncr = value; }
    void SetPageIncrement(int value) { m_pageIncr = value; }

    //////// Windows message handler functions

    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

    virtual MsgRouting WmMouseDown(UINT flags, int x, int y, int button);
    virtual MsgRouting HandleEvent(MSG* msg);

    virtual MsgRouting WmSetFocus(HWND hWndLost);
    virtual MsgRouting WmKillFocus(HWND hWndGot);

    virtual MsgRouting WmHScroll(UINT scrollCode, UINT pos, HWND hScrollBar);
    virtual MsgRouting WmVScroll(UINT scrollCode, UINT pos, HWND hScrollBar);

private:
    UINT          m_orientation; // SB_HORZ or SB_VERT
    AwtComponent* m_hostWindow;  // if set, contains real hwnd we are riding on
    int           m_lineIncr;
    int           m_pageIncr;

    // 4063897: Workaround windows focus indicator bug
    BOOL m_ignoreFocusEvents;
    void UpdateFocusIndicator();

    // Don't do redundant callbacks
    const char *m_prevCallback;
    int m_prevCallbackPos;

    void DoScrollCallbackCoalesce(const char* methodName, int newPos);

    static const char * const SbNdragAbsolute;
    static const char * const SbNdragBegin;
    static const char * const SbNdragEnd;
    static const char * const SbNlineDown;
    static const char * const SbNlineUp;
    static const char * const SbNpageDown;
    static const char * const SbNpageUp;
};

#endif
