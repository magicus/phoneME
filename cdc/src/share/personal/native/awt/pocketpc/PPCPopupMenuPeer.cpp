/*
 * @(#)PPCPopupMenuPeer.cpp	1.10 06/10/10
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
#include "jni.h"
#include "sun_awt_pocketpc_PPCPopupMenuPeer.h"
#include "PPCPopupMenuPeer.h"

/**
 * JNI Functions
 */
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCPopupMenuPeer_createMenu (JNIEnv *env, jobject thisObj, jobject hComponent)
{

  CHECK_NULL(hComponent, "null parent");
  AwtComponent *parent = PDATA(AwtComponent, hComponent);
  AwtToolkit::CreateComponent(
        thisObj, parent, (AwtToolkit::ComponentFactory)AwtPopupMenu::Create);

  //  CHECK_PEER_CREATION(self);
  
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCPopupMenuPeer_showPopup (JNIEnv *env, jobject thisObj,
                                               jobject event)
{

  CHECK_PEER(thisObj);
  CHECK_NULL(event, "null Event");
  jobject target = env->GetObjectField(thisObj, 
                   WCachedIDs.PPCObjectPeer_targetFID);
  CHECK_OBJ(target);

    // Invoke popup on toolkit thread -- yet another Win32 restriction.
  AwtPopupMenu *popupMenu = PDATA(AwtPopupMenu, thisObj);
  ::SendMessage(AwtToolkit::GetInstance().GetHWnd(), 
                WM_AWT_POPUPMENU_SHOW, (WPARAM)popupMenu, (LPARAM)event);
  
}

AwtPopupMenu::AwtPopupMenu() {
    m_parent = NULL;
}

AwtPopupMenu::~AwtPopupMenu() {
    m_parent = NULL;
}

const char* AwtPopupMenu::GetClassName() {
  return "SunAwtPopupMenu";
}

// Create a new AwtPopupMenu object and menu.  
AwtPopupMenu* AwtPopupMenu::Create(jobject self,
                                   AwtComponent* parent)
{
  
  JNIEnv *env;

  if (JVM->AttachCurrentThread((void **)&env, NULL) != 0)
    return NULL;

  jobject target = env->GetObjectField(self, 
                   WCachedIDs.PPCObjectPeer_targetFID);

  CHECK_NULL_RETURN(target, "null target");

  AwtPopupMenu* popupMenu = new AwtPopupMenu();
  CHECK_NULL_RETURN(popupMenu, "AwtPopupMenu() failed");

  popupMenu->LinkObjects(self);

  HMENU hMenu = ::CreatePopupMenu();
  ASSERT(hMenu);
  popupMenu->SetHMenu(hMenu);
  popupMenu->SetParent(parent);
    
  return popupMenu;
}

void AwtPopupMenu::Show(jobject evt) 
{
  // Convert the event's XY to absolute coordinates.  The XY is 
  // relative to the origin component, which is passed by PopupMenu
  // as the event's target.

  JNIEnv *env;

  if (JVM->AttachCurrentThread((void **)&env, NULL) != 0)
    return;

  jobject target = env->GetObjectField(evt, 
                   WCachedIDs.java_awt_Event_targetFID);

  jobject pOrigin = env->CallStaticObjectMethod(WCachedIDs.PPCObjectPeerClass,
                          WCachedIDs.PPCObjectPeer_getPeerForTargetMID,
						  target);

  CHECK_PEER(pOrigin);
  AwtComponent *origin = PDATA(AwtComponent, pOrigin);
  POINT pt;

  pt.x = env->GetIntField(evt, WCachedIDs.java_awt_Event_xFID);
  pt.y = env->GetIntField(evt, WCachedIDs.java_awt_Event_yFID);

  ::MapWindowPoints(origin->GetHWnd(), 0, (LPPOINT)&pt, 1);

  // Adjust to account for the Inset values
  RECT rctInsets;
  origin->GetInsets(&rctInsets);
  pt.x -= rctInsets.left;
  pt.y -= rctInsets.top;

  // Invoke the popup. Don't use VERIFY since TrackPopupMenu might fail if
  // another menu (such as a MenuBar pulldown) is currently being displayed.

  ::TrackPopupMenuEx(m_hMenu, TPM_LEFTALIGN,
		       pt.x, pt.y, m_parent->GetHWnd(), NULL);

}

