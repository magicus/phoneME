/*
 * @(#)PPCMenuPeer.cpp	1.8 06/10/10
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
#include "sun_awt_pocketpc_PPCMenuPeer.h"
#include "PPCMenuPeer.h"
#include "PPCToolkit.h"

 
/**
 * JNI Functions
 */
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCMenuPeer_createMenu (JNIEnv *env, jobject thisObj,
											  jobject pMenuBar)
{

  CHECK_NULL(pMenuBar, "null MenuBar");
  AwtMenuBar *menuBar = PDATA(AwtMenuBar, pMenuBar);
  AwtToolkit::CreateComponent(thisObj, menuBar, (AwtToolkit::ComponentFactory)
                              AwtMenu::Create);

//  CHECK_PEER_CREATION(self);    
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCMenuPeer_createSubMenu (JNIEnv *env, jobject thisObj, 
                                                 jobject pMenu)
{

  CHECK_NULL(pMenu, "null Menu");
  AwtMenu *menu = PDATA(AwtMenu, pMenu);
  AwtToolkit::CreateComponent(thisObj, menu, (AwtToolkit::ComponentFactory)
                              AwtMenu::Create);

// CHECK_PEER_CREATION(self);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCMenuPeer_addSeparator (JNIEnv *env, jobject thisObj)
{

  CHECK_PEER(thisObj);
  AwtObject::WinThreadExec(thisObj, AwtMenu::MENU_ADDSEPARATOR);
/*    
    AwtMenu* menu = (AwtMenu*)unhand(self)->pData;
    menu->AddSeparator();
*/
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCMenuPeer_delItem (JNIEnv *env, jobject thisObj, 
                                                 jint index)
{

  CHECK_PEER(thisObj);
  AwtObject::WinThreadExec(thisObj, AwtMenu::MENU_DELITEM, index);
/*    
    AwtMenu* menu = (AwtMenu*)unhand(self)->pData;
    menu->DeleteItem(index);
*/
}

AwtMenu::AwtMenu() {
  m_hMenu = NULL;
  m_parentMenu = NULL;
}


AwtMenu::~AwtMenu() {
  if (m_hMenu != NULL) {
    // Don't verify -- may not be a valid anymore if its window was
    // disposed of first.
    ::DestroyMenu(m_hMenu);
    m_hMenu = NULL;
  }
}

const char* AwtMenu::GetClassName() {
  return "SunAwtMenu";
}

// Create a new AwtMenu object and menu.  
AwtMenu* AwtMenu::Create(jobject self, AwtMenu* parentMenu)
{

  JNIEnv *env;
  if (JVM->AttachCurrentThread((void **) &env, NULL) != 0) {
	return NULL;
  }

  jobject target = env->GetObjectField(self, 
                   WCachedIDs.PPCObjectPeer_targetFID);
  CHECK_NULL_RETURN(target, "null target");

  AwtMenu* menu = new AwtMenu();
  CHECK_NULL_RETURN(menu, "AwtMenu() failed");

  menu->LinkObjects(self);

  menu->SetMenuContainer(parentMenu);

  HMENU hMenu = ::CreatePopupMenu();

  ASSERT(hMenu);
  menu->SetHMenu(hMenu);

  if (parentMenu != NULL) {
    parentMenu->AddItem(menu);
  }

  return menu;
}

AwtMenuBar* AwtMenu::GetMenuBar() {
  return (m_parentMenu == NULL) ? NULL : m_parentMenu->GetMenuBar();
}

HWND AwtMenu::GetOwnerHWnd() {
  return (m_parentMenu == NULL) ? NULL : m_parentMenu->GetOwnerHWnd();
}

void AwtMenu::AddSeparator() {
  VERIFY(::AppendMenu(GetHMenu(), MF_SEPARATOR, 0, 0));
}

void AwtMenu::AddItem(AwtMenuItem* item)
{
  jobject jitem = (jobject)item->GetTarget();

  JNIEnv *env;
  if (JVM->AttachCurrentThread((void **) &env, NULL) != 0) {
	return;
  }

  jboolean enabled = env->CallBooleanMethod(jitem,
                     WCachedIDs.java_awt_MenuItem_isEnabledMID);
  UINT flags =  MF_OWNERDRAW | ((enabled) ? MF_ENABLED : MF_GRAYED);
  
  jstring hLabel = (jstring) env->CallObjectMethod(jitem,
                             WCachedIDs.java_awt_MenuItem_getLabelMID);

  if (strcmp(item->GetClassName(), "SunAwtMenu") == 0) {
    AwtMenu* subMenu = (AwtMenu*)item;

    UINT menuID = (UINT)subMenu->GetHMenu();
    VERIFY(::AppendMenu(GetHMenu(), flags | MF_POPUP, menuID, 
                            (LPCTSTR)subMenu));
    item->SetID(menuID);
    subMenu->SetParentMenu(this);
  } 
  else {
    if (hLabel && (wcscmp(TO_WSTRING(hLabel), L"-") == 0)) {
        AddSeparator();
    } else {
      VERIFY(::AppendMenu(GetHMenu(), flags, item->GetID(), 
                          (LPCTSTR)this));
    }
  }
}

void AwtMenu::DeleteItem(long index)
{
  VERIFY(::RemoveMenu(GetHMenu(), index, MF_BYPOSITION));
}

void AwtMenu::SendDrawItem(AwtMenuItem* awtMenuItem, DRAWITEMSTRUCT far& drawInfo)
{
  if (strcmp(awtMenuItem->GetClassName(), "SunAwtMenu") == 0) {
	AwtMenu* awtSubMenu = (AwtMenu*)awtMenuItem;
	awtSubMenu->DrawItem(drawInfo);
  }
  else {
	awtMenuItem->DrawItem(drawInfo);
  }
}

void AwtMenu::SendMeasureItem(AwtMenuItem* awtMenuItem,
		HDC hDC, MEASUREITEMSTRUCT far& measureInfo)
{
  if (strcmp(awtMenuItem->GetClassName(), "SunAwtMenu") == 0) {
	AwtMenu* awtSubMenu = (AwtMenu*)awtMenuItem;
	awtSubMenu->MeasureItem(hDC, measureInfo);
  }
  else {
	awtMenuItem->MeasureItem(hDC, measureInfo);
  }
}

int AwtMenu::CountItem(jobject target)
{
   JNIEnv *env;
// FIXME: the below call should not return 0 in the error case
// Probably want to call this in the method that calls CountItem
   if (JVM->AttachCurrentThread((void **)&env, NULL) != 0)
     return 0;

   int nCount = env->CallIntMethod(target, 
		              WCachedIDs.java_awt_Menu_countItemsMID);
   ASSERT(!env->ExceptionCheck());
   return nCount;
}

AwtMenuItem* AwtMenu::GetItem(jobject target, long index)
{

  JNIEnv *env;

  if (JVM->AttachCurrentThread((void **)&env, NULL) != 0)
    return NULL;

  jobject menuItem = env->CallObjectMethod(target, 
		             WCachedIDs.java_awt_Menu_getItemMID, index);
  ASSERT(!env->ExceptionCheck());
    
  jobject wMenuItemPeer = env->CallStaticObjectMethod(WCachedIDs.PPCObjectPeerClass,
                          WCachedIDs.PPCObjectPeer_getPeerForTargetMID,
						  menuItem);
  CHECK_PEER_RETURN(wMenuItemPeer);

  AwtMenuItem *awtMenuItem = PDATA(AwtMenuItem, wMenuItemPeer);
  return awtMenuItem;
}

void AwtMenu::DrawItems(DRAWITEMSTRUCT far& drawInfo)
{
  jobject target = GetTarget();

  int nCount = CountItem(target);
  for (int i = 0; i < nCount; i++) {
	AwtMenuItem* awtMenuItem = GetItem(target, i);
    if (awtMenuItem != NULL) {
      SendDrawItem(awtMenuItem,drawInfo);
    }
  }
}

void AwtMenu::DrawItem(DRAWITEMSTRUCT far& drawInfo)
{
  ASSERT(drawInfo.CtlType == ODT_MENU);

  if (drawInfo.itemID == (UINT)m_hMenu) {
	DrawSelf(drawInfo);
        return;
  }

  DrawItems(drawInfo);
}

void AwtMenu::MeasureItems(HDC hDC, MEASUREITEMSTRUCT far& measureInfo)
{
  jobject target = GetTarget();

  int nCount = CountItem(target);    
  for (int i = 0; i < nCount; i++) {
	AwtMenuItem* awtMenuItem = GetItem(target, i);
    if (awtMenuItem != NULL) {
      SendMeasureItem(awtMenuItem, hDC, measureInfo);
    }
  }
}

void AwtMenu::MeasureItem(HDC hDC, MEASUREITEMSTRUCT far& measureInfo)
{
  ASSERT(measureInfo.CtlType == ODT_MENU);

  if (measureInfo.itemID == (UINT)m_hMenu) {
	MeasureSelf(hDC, measureInfo);
        return;
  }

  MeasureItems(hDC, measureInfo);
}

BOOL AwtMenu::IsTopMenu()
{

  return ((AwtMenuItem *)GetMenuBar() == (AwtMenuItem *)GetParent());

}

LONG AwtMenu::WinThreadExecProc(ExecuteArgs * args)
{
  switch( args->cmdId ) {
	case MENU_ADDSEPARATOR:
	    this->AddSeparator();
	    break;

	case MENU_DELITEM:
	    this->DeleteItem(args->param1);
	    break;

	default:
	    AwtMenuItem::WinThreadExecProc(args);
	    break;
  }
  return 0L;
}
