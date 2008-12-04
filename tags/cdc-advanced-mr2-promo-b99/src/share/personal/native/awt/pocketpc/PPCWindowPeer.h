/*
 * @(#)PPCWindowPeer.h	1.6 06/10/10
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
#ifndef _WINCEWINDOW_PEER_H_
#define _WINCEWINDOW_PEER_H_

#include "PPCCanvasPeer.h"

#include "java_awt_Window.h"
#include "sun_awt_pocketpc_PPCWindowPeer.h"

class AwtWindow : public AwtCanvas {
public:
    AwtWindow();
    virtual ~AwtWindow();

    virtual const TCHAR *GetClassName();
    
    static AwtWindow* Create(jobject self,
                             jobject hParent);

    // Update the insets for this Window (container), its peer & optional other
    BOOL UpdateInsets(jobject insets);

    // Subtract inset values from a window origin. (2 versions)
    INLINE void SubtractInsetPoint(long& x, long& y) { 
	x -= m_insets.left;
	y -= m_insets.top;
    }
    INLINE void SubtractInsetPoint(int& x, int& y) { 
	x -= m_insets.left;
	y -= m_insets.top;
    }

    virtual void GetInsets(RECT* rect) { 
        VERIFY(::CopyRect(rect, &m_insets));
    }

#ifdef WINCE
    /* Similiar to Subtract Inset Points, but differs for frames w/ menubar */
    virtual void TranslateToClient(int &x, int &y);
    virtual void TranslateToClient(long &x, long &y);
#endif /* WINCE */

    virtual BOOL IsEmbedded() { return FALSE;} // to make embedded frames easier

    virtual BOOL IsContainer() { return TRUE;} // We can hold children

    // Return the associated AWT peer object.
    INLINE jobject GetPeer() { 
	return m_peerObject; 
    }

    virtual void Invalidate(RECT* r);
    virtual void Reshape(int x, int y, int width, int height);
    virtual void SetResizable(BOOL isResizable);
    virtual void RecalcNonClient();
    virtual void RedrawNonClient();

    // Post events to the EventQueue
    void SendComponentEvent(jint eventId);
    void SendWindowEvent(jint eventId);

    //////// Windows message handler functions

    virtual MsgRouting WmCreate();
    virtual MsgRouting WmClose();
    virtual MsgRouting WmDestroy();
    virtual MsgRouting WmMove(int x, int y);
    virtual MsgRouting WmSize(int type, int w, int h);
    virtual MsgRouting WmPaint(HDC hDC);
    virtual MsgRouting WmSysCommand(UINT uCmdType, UINT xPos, UINT yPos);
    virtual MsgRouting WmExitSizeMove();
    virtual MsgRouting WmSettingChange(WORD wFlag, LPCTSTR pszSection);
#ifndef WINCE
    virtual MsgRouting WmNcCalcSize(BOOL fCalcValidRects, 
				    LPNCCALCSIZE_PARAMS lpncsp, UINT& retVal);
    virtual MsgRouting WmNcPaint(HRGN hrgn);
    virtual MsgRouting WmNcHitTest(UINT x, UINT y, UINT& retVal);
#endif

private:
    BOOL m_iconic;          // are we in an iconic state (for resize tracking)
#ifndef WINCE
    RECT m_insets;          // a cache of the insets being used
#endif
    RECT m_old_insets;      // help determine if insets change
    BOOL m_resizing;        // in the middle of a resizing operation
    POINT m_sizePt;         // the last value of WM_SIZE
    RECT m_warningRect;     // The window's warning banner area, if any.
#ifdef WINCE
protected:
    RECT m_insets;
    HWND m_cmdBar;
#endif
};

#endif
