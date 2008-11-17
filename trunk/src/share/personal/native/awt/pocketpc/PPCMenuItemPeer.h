/*
 * @(#)PPCMenuItemPeer.h	1.6 06/10/10
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
#ifndef _WINCEMENU_ITEM_PEER_H_
#define _WINCEMENU_ITEM_PEER_H_

#include "PPCObjectPeer.h"
#include "PPCComponentPeer.h"

#include "java_awt_MenuItem.h"
#include "sun_awt_pocketpc_PPCMenuItemPeer.h"
#include "java_awt_Menu.h"
#include "sun_awt_pocketpc_PPCMenuPeer.h"
//#include "java_awt_MenuComponent.h"
//#include "java_awt_peer_MenuComponentPeer.h"
#include "java_awt_FontMetrics.h"

class AwtMenu;

//
// Menuitem class for accessing items in an HMENU
//
class AwtMenuItem : public AwtObject {
public:
    // id's for methods executed on toolkit thread
  enum {
    MENUITEM_SETLABEL,
	MENUITEM_ENABLE,
	MENUITEM_SETSTATE,
	MENUITEM_DISPOSE,
	MENUITEM_LAST
  };

  AwtMenuItem();
  virtual ~AwtMenuItem();

  virtual const char* GetClassName();
    
  static AwtMenuItem* Create(jobject self, jobject hMenu);

  INLINE AwtMenu* GetMenuContainer() {
	return m_menuContainer; 
  }
  
  INLINE void SetMenuContainer(AwtMenu* menu) {
	m_menuContainer = menu; 
  }
  
  INLINE UINT GetID() { 
    return m_Id;
  }
  
  INLINE void SetID(UINT id) { 
	m_Id = id;
  }

  // Execute the command associated with this item.
  virtual void DoCommand(void);

  void LinkObjects(jobject hPeer);

  INLINE CriticalSection& GetLock() {
    return m_lock; 
  }

  //for multifont menuitem
  INLINE jstring GetJavaString() {
    jobject target = GetTarget();
	JNIEnv *env;
	if (JVM->AttachCurrentThread((void **)&env, NULL) != 0) {
	  return NULL;
	}
	jstring label = (jstring)env->CallObjectMethod(
		             target, WCachedIDs.java_awt_MenuItem_getLabelMID);
	return label;
  }

  virtual void DrawItem(DRAWITEMSTRUCT far& drawInfo);
  void DrawSelf(DRAWITEMSTRUCT far& drawInfo);
  virtual void MeasureItem(HDC hDC, MEASUREITEMSTRUCT far& measureInfo);
  void MeasureSelf(HDC hDC, MEASUREITEMSTRUCT far& measureInfo);
  jobject GetFont();
  jobject GetFontMetrics(jobject hJavaFont);
  virtual BOOL IsTopMenu();
  void DrawCheck(HDC hDC, RECT rect);
  void SetLabel(JavaStringBuffer *sb);
  void Enable(BOOL isEnabled);
  void SetState(BOOL isChecked);
  void Dispose();

  //////// Windows message handler functions

  MsgRouting WmNotify(UINT notifyCode);

  virtual LONG WinThreadExecProc(ExecuteArgs * args);

protected:
  AwtMenu* m_menuContainer;  // The menu object containing this item
  UINT m_Id;                 // The id of this item

private:
  CriticalSection m_lock;

public:
  static HBITMAP bmpCheck;
  static jobject hSystemFont;
};

#endif
