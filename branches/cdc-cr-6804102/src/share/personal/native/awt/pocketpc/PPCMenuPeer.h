/*
 * @(#)PPCMenuPeer.h	1.6 06/10/10
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
#ifndef _WINCEMENU_PEER_H_
#define _WINCEMENU_PEER_H_

#include "PPCMenuItemPeer.h"

#include "java_awt_MenuItem.h"
#include "sun_awt_pocketpc_PPCMenuItemPeer.h"
#include "java_awt_Menu.h"
#include "sun_awt_pocketpc_PPCMenuPeer.h"

class AwtMenuBar;

//
// Menu class for accessing an HMENU
//
class AwtMenu : public AwtMenuItem {
public:
    // id's for methods executed on toolkit thread
    enum {
	  MENU_ADDSEPARATOR = MENUITEM_LAST+1,
	  MENU_DELITEM,
	  MENU_LAST
    };

    AwtMenu();
    virtual ~AwtMenu();
    virtual const char* GetClassName();

    // Create a new AwtMenu.  This must be run on the main thread.
    static AwtMenu* Create(jobject self,
                           AwtMenu* parentMenu);

    INLINE HMENU GetHMenu() { 
		return m_hMenu; 
	}
    
	INLINE void SetHMenu(HMENU hMenu) { 
	  m_hMenu = hMenu;
	}

    virtual AwtMenuBar* GetMenuBar();

    INLINE AwtMenu* GetParent() { 
	  return m_parentMenu; 
	}

    INLINE void SetParentMenu(AwtMenu* parent) {
	  m_parentMenu = parent;
	}

    void AddSeparator();
    virtual void AddItem(AwtMenuItem* item);
    virtual void DeleteItem(long index);
    virtual HWND GetOwnerHWnd();

    //for multifont menu
    BOOL IsTopMenu();
    virtual AwtMenuItem* GetItem(jobject target, long index);
    virtual int CountItem(jobject target);
    virtual void SendDrawItem(AwtMenuItem* awtMenuItem,
			      DRAWITEMSTRUCT far& drawInfo);
    virtual void SendMeasureItem(AwtMenuItem* awtMenuItem,
				 HDC hDC, MEASUREITEMSTRUCT far& measureInfo);
    void DrawItem(DRAWITEMSTRUCT far& drawInfo);
    void DrawItems(DRAWITEMSTRUCT far& drawInfo);
    void MeasureItem(HDC hDC, MEASUREITEMSTRUCT far& measureInfo);
    void MeasureItems(HDC hDC, MEASUREITEMSTRUCT far& measureInfo);

    virtual LONG WinThreadExecProc(ExecuteArgs * args);

protected:
    HMENU    m_hMenu;
    AwtMenu* m_parentMenu;	// can also be an AwtMenuBar
};

#endif
