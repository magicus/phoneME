/*
 * @(#)PPCMenuBarPeer.h	1.6 06/10/10
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
#ifndef _WINCEMENU_BAR_PEER_H_
#define _WINCEMENU_BAR_PEER_H_

#include "awt.h"
#include "PPCMenuPeer.h"
#include "PPCFramePeer.h"
#include "java_awt_MenuBar.h"
#include "sun_awt_pocketpc_PPCMenuBarPeer.h"
#include "sun_awt_pocketpc_PPCFramePeer.h"

class AwtFrame; 

//
// Menubar class for accessing items in a HWND's menubar
//
class AwtMenuBar : public AwtMenu {
public:
    // id's for methods executed on toolkit thread
    enum MenuExecIds {
	  MENUBAR_DELITEM = MENU_LAST+1
    };

    AwtMenuBar();
    virtual ~AwtMenuBar();
    virtual const char* GetClassName();

    // Create a new AwtMenuBar.  This must be run on the main thread.
    static AwtMenuBar* Create(jobject self,
                              jobject hFramePeer);

    // Return the associated AWT peer object.
    INLINE jobject GetPeer() { 
	  return m_peerObject; 
    }

    virtual AwtMenuBar* GetMenuBar() { 
	  return this;
	}

    INLINE AwtFrame* GetFrame() {
	  return m_frame; 
	}

    virtual HWND GetOwnerHWnd();

    AwtMenuItem* GetItem(jobject target, long index);

    int CountItem(jobject target);

    void SendDrawItem(AwtMenuItem* awtMenuItem,
		      DRAWITEMSTRUCT far& drawInfo);

    void SendMeasureItem(AwtMenuItem* awtMenuItem,
			 HDC hDC, MEASUREITEMSTRUCT far& measureInfo);

    void DrawItem(DRAWITEMSTRUCT far& drawInfo);

    void MeasureItem(HDC hDC, MEASUREITEMSTRUCT far& measureInfo);

    void AddItem(AwtMenuItem* item);

    void DeleteItem(long index);

    virtual LONG WinThreadExecProc(ExecuteArgs * args);

protected:

    AwtFrame *m_frame;

};

#endif
