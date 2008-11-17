/*
 * @(#)PPCFramePeer.h	1.6 06/10/10
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
 * @(#)PPCFramePeer.h	1.6 06/10/10
 */

#ifndef _WINCEFRAME_PEER_H_
#define _WINCEFRAME_PEER_H_

#include "PPCWindowPeer.h"
#include "PPCMenuBarPeer.h" //add for multifont

#include "java_awt_Frame.h"
#include "sun_awt_pocketpc_PPCFramePeer.h"
#include "PPCImage.h"

#ifdef WINCE
#include <commctrl.h>
#endif /* WINCE */


class AwtFrame : public AwtWindow {
public:
    AwtFrame();
    virtual ~AwtFrame();

    virtual const TCHAR *GetClassName();
    
    // Create a new AwtFrame.  This must be run on the main thread.
    static AwtFrame* Create( jobject self,
                             jobject hParent );

    // Return the associated AWT peer object.
    INLINE jobject GetPeer() {
        return m_peerObject;
    }

    // Returns whether this frame is embedded in an external native frame.
    INLINE BOOL IsEmbedded() { return m_isEmbedded; }

    void Reshape( int x, int y, int width, int height );
    void Show();

    INLINE void DrawMenuBar() { VERIFY(::DrawMenuBar( GetHWnd() )); }

#ifdef WINCE
    virtual void AwtFrame::TranslateToClient( int &x, int &y );
    virtual void AwtFrame::TranslateToClient( long &x, long &y );
#endif /* WINCE */
    void SetIcon( AwtImage *pImage );

    //for WmDrawItem and WmMeasureItem method
    AwtMenuBar* GetMenuBar();
    void SetMenuBar( AwtMenuBar* );

    MsgRouting WmSize( int type, int w, int h );
    MsgRouting WmSetCursor( HWND hWnd, UINT hitTest, UINT message, 
                            BOOL& retVal );
    MsgRouting WmActivate( UINT nState, BOOL fMinimized );
    MsgRouting WmDrawItem( UINT ctrlId, DRAWITEMSTRUCT far& drawInfo );
    MsgRouting WmMeasureItem( UINT ctrlId,
                              MEASUREITEMSTRUCT far& measureInfo );
    MsgRouting WmEnterMenuLoop( BOOL isTrackPopupMenu );
    MsgRouting WmExitMenuLoop( BOOL isTrackPopupMenu );

    LONG WinThreadExecProc( ExecuteArgs * args );

private:
    static BOOL CALLBACK OwnedSetIcon( HWND hWnd, LPARAM lParam );

    // The frame's icon.  If NULL, just paint the HotJava logo.
    HICON m_hIcon;

    // The frame's embedding parent (if any)
    HWND m_parentWnd;

    // The frame's menubar.
    AwtMenuBar* menuBar;

    // The frame is an EmbeddedFrame.
    BOOL m_isEmbedded;

    // height added by SetMenuBar
    int m_extraHeight;

    // used so that calls to ::MoveWindow in SetMenuBar don't propogate
    // because they are immediately followed by calls to Component.resize
    BOOL m_ignoreWmSize;

    // tracks whether or not menu on this frame is dropped down
    BOOL m_isMenuDropped;
};

#endif
