/*
 * @(#)PPCDialogPeer.h	1.6 06/10/10
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
 * @(#)PPCDialogPeer.h	1.6 06/10/10
 */
#ifndef _WINCEDIALOG_PEER_H_
#define _WINCEDIALOG_PEER_H_

#include "PPCFramePeer.h"

#include "java_awt_Dialog.h"
#include "sun_awt_pocketpc_PPCDialogPeer.h"
//#include <awt_image.h>

class AwtDialog : public AwtFrame {
public:
    AwtDialog();
    virtual ~AwtDialog();

    virtual const TCHAR* GetClassName();
    virtual void RegisterClass();
    virtual void SetResizable( BOOL isResizable );

    // Create a new AwtDialog.  This must be run on the main thread.
    static AwtDialog* Create( jobject self,
                              jobject hParent );

    // Return the associated AWT peer object.
    INLINE jobject GetPeer()
    { 
	return m_peerObject; 
    }

    virtual MsgRouting WmShowModal();
    virtual MsgRouting WmEndModal();
    virtual MsgRouting WmStyleChanged( WORD wStyleType, LPSTYLESTRUCT lpss );
    virtual MsgRouting WmSize( int type, int w, int h );
    virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam );

    // utility functions for application-modal dialogs
    static void ModalDisable( HWND hwndDlg );
    static void ModalEnable( HWND hwndDlg );
    static void ModalNextWindowToFront( HWND hwndDlg );

private:
    static int GetDisabledLevel( HWND hwnd );
    static void IncrementDisabledLevel( HWND hwnd, int increment );
    static void ResetDisabledLevel( HWND hwnd );
    static BOOL CALLBACK DisableTopLevelsCallback( HWND hWnd, LPARAM lParam );
    static BOOL CALLBACK EnableTopLevelsCallback( HWND hWnd, LPARAM lParam );
    static BOOL CALLBACK NextWindowToFrontCallback( HWND hWnd, LPARAM lParam );

    void UpdateSystemMenu();
    void UpdateDialogIcon();

    HWND m_modalWnd;
#ifdef WINCE
    int disabledLevel;
#endif
};

#endif
