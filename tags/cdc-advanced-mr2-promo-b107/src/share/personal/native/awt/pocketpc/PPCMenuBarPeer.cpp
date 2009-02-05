/*
 * @(#)PPCMenuBarPeer.cpp	1.10 06/10/10
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
#include "sun_awt_pocketpc_PPCMenuBarPeer.h"
#include "PPCMenuBarPeer.h"
#include "PPCMenuItemPeer.h"
#include <wceCompat.h>

/**
 * JNI Functions
 */ 
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCMenuBarPeer_initIDs (JNIEnv *env, jclass cls)
{
  cls = env->FindClass ("java/awt/MenuBar");
  if (cls == NULL)
    return;
  GET_METHOD_ID (java_awt_MenuBar_countMenusMID, "countMenus", "()I");
  GET_METHOD_ID (java_awt_MenuBar_getMenuMID, "getMenu", "(I)Ljava/awt/Menu;");
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCMenuBarPeer_create (JNIEnv *env, jobject thisObj, jobject hFrame)
{

  AwtToolkit::CreateComponent(thisObj, hFrame, (AwtToolkit::ComponentFactory)
                              AwtMenuBar::Create);

 // CHECK_PEER_CREATION(self);

}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCMenuBarPeer_addMenu (JNIEnv *env, jobject thisObj, jobject menu)
{ 
 
  CHECK_PEER(thisObj);

  AwtMenuBar *menubar = PDATA(AwtMenuBar, thisObj);

  // The menu was already created and added during peer creation -- redraw.
  ::DrawMenuBar(menubar->GetOwnerHWnd());
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCMenuBarPeer_delMenu (JNIEnv *env, jobject thisObj,
                                            jint index)
{
    CHECK_PEER(thisObj);
    AwtObject::WinThreadExec(thisObj, AwtMenuBar::MENUBAR_DELITEM, index);
/*    
    AwtMenuBar* menubar = (AwtMenuBar*)unhand(self)->pData;
    menubar->DeleteItem(index);
    ::DrawMenuBar(menubar->GetOwnerHWnd());
*/
}

AwtMenuBar::AwtMenuBar() {
  m_frame = NULL;
}

AwtMenuBar::~AwtMenuBar() {
  m_frame = NULL;
}

const char* AwtMenuBar::GetClassName() {
  return "SunAwtMenuBar";
}

// Create a new AwtMenuBar object and menu.  
AwtMenuBar* AwtMenuBar::Create(jobject self,
                               jobject hFramePeer)
{

  JNIEnv *env;
  if (JVM->AttachCurrentThread((void **) &env, NULL) != 0) {
	return NULL;
  }

  jobject target = env->GetObjectField(self, 
                   WCachedIDs.PPCObjectPeer_targetFID);
  CHECK_NULL_RETURN(target, "null target");

  AwtMenuBar* menuBar = new AwtMenuBar();
  CHECK_NULL_RETURN(menuBar, "AwtMenuBar() failed");

  menuBar->LinkObjects(self);

  HMENU hMenu = wceCreateMenuBarMenu(); 

  ASSERT(hMenu);
  menuBar->SetHMenu(hMenu);

  if (hFramePeer != NULL) {
    CHECK_OBJ(hFramePeer);
	menuBar->m_frame = PDATA(AwtFrame, hFramePeer);
  }
  
  return menuBar;
}

HWND AwtMenuBar::GetOwnerHWnd()
{
  return m_frame->GetHWnd();
}

void AwtMenuBar::SendDrawItem(AwtMenuItem* awtMenuItem, DRAWITEMSTRUCT far& drawInfo)
{
  awtMenuItem->DrawItem(drawInfo);
}

void AwtMenuBar::SendMeasureItem(AwtMenuItem* awtMenuItem,
		HDC hDC, MEASUREITEMSTRUCT far& measureInfo)
{
  awtMenuItem->MeasureItem(hDC, measureInfo);
}

int AwtMenuBar::CountItem(jobject target)
{

  JNIEnv *env;
// FIXME: the below call should not return 0 in the error case
// Probably want to call this in the method that calls CountItem
  if (JVM->AttachCurrentThread((void **)&env, NULL) != 0)
    return 0;

  int nCount = env->CallIntMethod(target, 
               WCachedIDs.java_awt_MenuBar_countMenusMID);

  return nCount;
}

AwtMenuItem* AwtMenuBar::GetItem(jobject target, long index)
{

  JNIEnv *env;
// FIXME: the below call should not return NULL in the error case
// Probably want to call this in the method that calls CountItem
  if (JVM->AttachCurrentThread((void **)&env, NULL) != 0)
    return NULL;

  jobject menu = env->CallObjectMethod(target, 
		         WCachedIDs.java_awt_MenuBar_getMenuMID, index);

  jobject wMenuItemPeer = env->CallStaticObjectMethod(WCachedIDs.PPCObjectPeerClass,
                          WCachedIDs.PPCObjectPeer_getPeerForTargetMID,
						  menu);
  CHECK_PEER_RETURN(wMenuItemPeer);

  AwtMenuItem *awtMenuItem = PDATA(AwtMenuItem, wMenuItemPeer);
  return awtMenuItem;

}

void AwtMenuBar::DrawItem(DRAWITEMSTRUCT far& drawInfo)
{
  ASSERT(drawInfo.CtlType == ODT_MENU);

  AwtMenu::DrawItems(drawInfo);
}

void AwtMenuBar::MeasureItem(HDC hDC, MEASUREITEMSTRUCT far& measureInfo)
{
  ASSERT(measureInfo.CtlType == ODT_MENU);

  AwtMenu::MeasureItem(hDC, measureInfo);
}
        
void AwtMenuBar::AddItem(AwtMenuItem* item)
{
  AwtMenu::AddItem(item);

  VERIFY(::InvalidateRect(GetOwnerHWnd(),0,TRUE));
}

void AwtMenuBar::DeleteItem(long index)
{
  AwtMenu::DeleteItem(index);
 
  VERIFY(::InvalidateRect(GetOwnerHWnd(),0,TRUE));
  ::DrawMenuBar(GetOwnerHWnd());
}

LONG AwtMenuBar::WinThreadExecProc(ExecuteArgs * args)
{
  switch( args->cmdId ) {
	case MENUBAR_DELITEM:
	    this->DeleteItem(args->param1);
	    break;

	default:
	    AwtMenu::WinThreadExecProc(args);
	    break;
  }
  return 0L;
}

