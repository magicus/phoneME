/*
 * @(#)PPCMenuItemPeer.cpp	1.18 06/10/10
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
#include "sun_awt_pocketpc_PPCMenuItemPeer.h"
#include "sun_awt_pocketpc_PPCCheckboxMenuItemPeer.h"
#include "PPCMenuItemPeer.h"
#include "PPCMenuBarPeer.h"
#include "PPCToolkit.h"

#include <java_awt_event_InputEvent.h>

#define BM_SIZE 26  /* height and width of check.bmp */

HBITMAP AwtMenuItem::bmpCheck;
jobject AwtMenuItem::hSystemFont;

/**
 * JNI Functions
 */
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCMenuItemPeer_initIDs (JNIEnv *env, jclass cls)
{

  GET_FIELD_ID  (PPCMenuItemPeer_isCheckboxFID, "isCheckbox", "Z");

  GET_FIELD_ID  (PPCMenuItemPeer_shortcutLabelFID, 
	             "shortcutLabel", "Ljava/lang/String;");

  GET_METHOD_ID (PPCMenuItemPeer_getDefaultFontMID, 
	             "getDefaultFont", "()Ljava/awt/Font;");

  GET_METHOD_ID (PPCMenuItemPeer_handleActionMID,
	             "handleAction", "(I)V");

  // In the future, the following might be moved to a PPCFont*.cpp file
  cls = env->FindClass ("sun/awt/pocketpc/PPCFontMetrics");
  if (cls == NULL)
	  return;
  GET_METHOD_ID (PPCFontMetrics_getHeightMID,
	             "getHeight", "()I");
  GET_STATIC_METHOD_ID (PPCFontMetrics_getFontMetricsMID, 
	                    "getFontMetrics", "(Ljava/awt/Font;)Ljava/awt/FontMetrics;");


  cls = env->FindClass ("java/awt/CheckboxMenuItem");
  if (cls == NULL)
     return;
  GET_METHOD_ID (java_awt_CheckboxMenuItem_getStateMID,
                "getState", "()Z");

  cls = env->FindClass ("java/awt/MenuItem");
	
  if (cls == NULL)
    return;

  GET_METHOD_ID (java_awt_MenuItem_isEnabledMID, "isEnabled",
                 "()Z");
  GET_METHOD_ID (java_awt_MenuItem_getLabelMID,
                "getLabel", "()Ljava/lang/String;");


  cls = env->FindClass ("java/awt/Menu");
  if (cls == NULL)
    return;
  GET_METHOD_ID (java_awt_Menu_countItemsMID,
                  "countItems", "()I");
  GET_METHOD_ID (java_awt_Menu_getItemMID,
                  "getItem", "(I)Ljava/awt/MenuItem;");

  cls = env->FindClass ("java/awt/MenuComponent");
  if (cls == NULL)
     return;
  GET_METHOD_ID (java_awt_MenuComponent_getFontMID,
                 "getFont", "()Ljava/awt/Font;");

}


JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCCheckboxMenuItemPeer_initIDs (JNIEnv *env, jclass cls)
{
  GET_METHOD_ID (PPCCheckboxMenuItemPeer_handleActionMID,
	             "handleAction", "(ZI)V");
}


JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCMenuItemPeer_create (JNIEnv *env, jobject thisObj,
                                                jobject menu)
{

  CHECK_NULL(menu, "null Menu");
  AwtToolkit::CreateComponent(
    thisObj, menu, (AwtToolkit::ComponentFactory)AwtMenuItem::Create);

//  CHECK_PEER_CREATION(thisObj);
 
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCMenuItemPeer_disposeNative (JNIEnv *env, jobject thisObj)
{

  CHECK_PEER(thisObj);
  AwtObject::WinThreadExec(thisObj, AwtMenuItem::MENUITEM_DISPOSE);
  ASSERT(PDATA(AwtObject, thisObj) == NULL);
/*    
    if (self == NULL || unhand(self)->pData == 0) {
        return;
    }
    AwtMenuItem *p = PDATA(AwtMenuItem, self);

    if (p != NULL) {
	::SendMessage(AwtToolkit::GetInstance().GetHWnd(), WM_AWT_DISPOSE,
		      (WPARAM)self, 0);
    }
*/
   
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCMenuItemPeer__1setLabel (JNIEnv *env, 
                                                        jobject thisObj, 
                                                        jstring label)
{

  CHECK_PEER(thisObj);
  CHECK_NULL(label, "null label");
  JavaStringBuffer sb(env, label);
  AwtObject::WinThreadExec(thisObj, AwtMenuItem::MENUITEM_SETLABEL, (LPARAM)&sb);
/*    
    AwtMenuItem* menuItem = (AwtMenuItem*)unhand(self)->pData;
    AwtMenu* menu = menuItem->GetMenuContainer();
    JavaStringBuffer sb(label);
    ASSERT(menu != NULL && menuItem->GetID() >= 0);

    MENUITEMINFO mii;
    mii.cbSize = sizeof mii;
    mii.fMask = MIIM_TYPE;
    mii.fType = MFT_OWNERDRAW;
    mii.dwTypeData = sb;
    SetMenuItemInfo(menu->GetHMenu(), menuItem->GetID(), FALSE, &mii);

    // Redraw menu bar if it was affected.
    if (menu->GetMenuBar() == menu) {
        ::DrawMenuBar(menu->GetOwnerHWnd());
    }
*/
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCMenuItemPeer_enable (JNIEnv *env, 
                                                    jobject thisObj,
                                                    jboolean enabled)
{

  CHECK_PEER(thisObj);
  AwtObject::WinThreadExec(thisObj, AwtMenuItem::MENUITEM_ENABLE, (LPARAM)enabled );
/*    
    AwtMenuItem* menuItem = (AwtMenuItem*)unhand(self)->pData;
    AwtMenu* menu = menuItem->GetMenuContainer();
    ASSERT(menu != NULL && menuItem->GetID() >= 0);
    VERIFY(::EnableMenuItem(menu->GetHMenu(), menuItem->GetID(),
                            MF_BYCOMMAND | (e ? MF_ENABLED : MF_GRAYED))
           != 0xFFFFFFFF);

    // Redraw menu bar if it was affected.
    if (menu->GetMenuBar() == menu) {
        ::DrawMenuBar(menu->GetOwnerHWnd());
    }
*/    
}


JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCCheckboxMenuItemPeer_setState (JNIEnv *env, 
                                                       jobject thisObj, 
                                                       jboolean isChecked)
{
    CHECK_PEER(thisObj);
    AwtObject::WinThreadExec(thisObj, AwtMenuItem::MENUITEM_SETSTATE, isChecked);
/*    
    AwtMenuItem* menuItem = (AwtMenuItem*)unhand(self)->pData;
    AwtMenu* menu = menuItem->GetMenuContainer();
    ASSERT(menu != NULL && menuItem->GetID() >= 0);
    VERIFY(::CheckMenuItem(menu->GetHMenu(), menuItem->GetID(),
                           MF_BYCOMMAND | (check ? MF_CHECKED : MF_UNCHECKED))
           != 0xFFFFFFFF);

    // Redraw menu bar if it was affected.
    if (menu->GetMenuBar() == menu) {
        ::DrawMenuBar(menu->GetOwnerHWnd());
    }
*/

}


AwtMenuItem::AwtMenuItem() : m_lock("AwtMenuItem") {
    m_peerObject = NULL;
    m_menuContainer = NULL;
    m_Id = (UINT)-1;
}

AwtMenuItem::~AwtMenuItem() {
  CriticalSection::Lock l(GetLock());

  if (m_peerObject != 0) {
    JNIEnv *env;

    if (JVM->AttachCurrentThread((void **)&env, NULL) != 0)
      return;

    SET_PDATA(m_peerObject, NULL);

  }

}

const char* AwtMenuItem::GetClassName() {
  return "SunAwtMenuItem";
}

////////////////////////////////////////
// AwtMenuItem and Java Peer interaction

// Link the C++ and Java peers together.
//
void AwtMenuItem::LinkObjects(jobject hPeer)
{

  JNIEnv *env;

  if (JVM->AttachCurrentThread((void **)&env, NULL) != 0)
    return;

  m_peerObject = env->NewGlobalRef(hPeer);    // C++ -> JavaPeer

  SET_PDATA(hPeer, (jint)this);  // JavaPeer -> C++
}

AwtMenuItem* AwtMenuItem::Create(jobject self,
                                 jobject hMenuPeer)
{

  JNIEnv *env;

  if (JVM->AttachCurrentThread((void **)&env, NULL) != 0)
    return NULL;

  jobject target = env->GetObjectField(self, 
                   WCachedIDs.PPCObjectPeer_targetFID);

  AwtMenu* menu = PDATA(AwtMenu, hMenuPeer);

  AwtMenuItem* item = new AwtMenuItem();

  item->LinkObjects(self);

  item->SetMenuContainer(menu);
  item->SetID(AwtToolkit::GetInstance().CreateCmdID(item));
  menu->AddItem(item);

  return item;
}

MsgRouting AwtMenuItem::WmNotify(UINT notifyCode)
{
    return mrDoDefault;
}

jobject AwtMenuItem::GetFont()
{
  jobject self = (jobject)GetPeer();
  
  JNIEnv *env;

  if (JVM->AttachCurrentThread((void **)&env, NULL) != 0)
    return NULL;

  jobject target = env->GetObjectField(self, 
                   WCachedIDs.PPCObjectPeer_targetFID);
  
  jobject font = env->CallObjectMethod(target, 
	             WCachedIDs.java_awt_MenuComponent_getFontMID);

  if (font != NULL) {
    return font;
  }

  if (hSystemFont == NULL) {
	 hSystemFont = env->CallObjectMethod(self, WCachedIDs.PPCMenuItemPeer_getDefaultFontMID);
     ASSERT(hSystemFont);
  }

  return hSystemFont;
}

void AwtMenuItem::DrawSelf(DRAWITEMSTRUCT far& drawInfo)
{
  jobject self = GetPeer();

  JNIEnv *env;

  if (JVM->AttachCurrentThread((void **)&env, NULL) != 0)
    return;

  jobject target = env->GetObjectField(self, 
                   WCachedIDs.PPCObjectPeer_targetFID);
    
  HDC hDC = drawInfo.hDC;
  RECT rect = drawInfo.rcItem;
  RECT textRect = rect;
  SIZE size;
  jobject hJavaFont;
  jobject hJavaString;
  DWORD crBack,crText;

  hJavaFont = GetFont();
  hJavaString = GetJavaString();

  size = AwtFont::getMFStringSize(hDC,hJavaFont, (jstring)hJavaString);

  //Check whether the MenuItem is disabled.
  jboolean bEnabled = env->CallBooleanMethod(target,
                     WCachedIDs.java_awt_MenuItem_isEnabledMID);

  if ((drawInfo.itemState) & (ODS_SELECTED)) {
	// Set background and text colors for selected item
	crBack = ::GetSysColor (COLOR_HIGHLIGHT);
        // Disabled text must be drawn in gray.
	crText = ::GetSysColor (bEnabled ? COLOR_HIGHLIGHTTEXT : COLOR_GRAYTEXT);
  } else {
	// Set background and text colors for unselected item
	crBack = ::GetSysColor (COLOR_MENU);
        // Disabled text must be drawn in gray.
	crText = ::GetSysColor (bEnabled ? COLOR_MENUTEXT : COLOR_GRAYTEXT);
  }
    
  // Fill item rectangle with background color
  AwtBrush* brBack = AwtBrush::Get(crBack);
  ASSERT(brBack->GetHandle());
  VERIFY(::FillRect(hDC, &rect, (HBRUSH)brBack->GetHandle()));
  brBack->Release();
  
  // Set current background and text colors
  ::SetBkColor (hDC, crBack);
  ::SetTextColor (hDC, crText);

  int nOldBkMode = ::SetBkMode(hDC, OPAQUE);
  ASSERT(nOldBkMode != 0);

  //draw check mark

  int checkWidth = BM_SIZE;

  jboolean isCheckbox = env->GetBooleanField
	                    (self, WCachedIDs.PPCMenuItemPeer_isCheckboxFID);
  if (isCheckbox) {

    jobject target = env->GetObjectField(self, 
                   WCachedIDs.PPCObjectPeer_targetFID);;

    jboolean state = env->CallBooleanMethod(target, WCachedIDs.java_awt_CheckboxMenuItem_getStateMID);

    if (state) {
      ASSERT(drawInfo.itemState & ODS_CHECKED);
      RECT checkRect;
      ::CopyRect(&checkRect, &textRect);
      checkRect.right = checkRect.left + checkWidth;
      DrawCheck(hDC, checkRect);
    }
  }
    
  ::SetBkMode(hDC, TRANSPARENT);

    //draw string
  if (!IsTopMenu())
	textRect.left = textRect.left + checkWidth;
  else
	textRect.left = (textRect.left+textRect.right-size.cx)/2;

  int x = textRect.left;
  int y = (textRect.top+textRect.bottom-size.cy)/2;

  // Text must be drawn in emboss if the Menu is disabled and not selected.
  BOOL bEmboss = !bEnabled && !(drawInfo.itemState & ODS_SELECTED);
  if (bEmboss) {
    ::SetTextColor(hDC, GetSysColor(COLOR_BTNHIGHLIGHT));
    AwtFont::drawMFString(hDC,hJavaFont,(jstring)hJavaString, x+1, y+1);
    ::SetTextColor(hDC, GetSysColor(COLOR_BTNSHADOW));
  }

  AwtFont::drawMFString(hDC,hJavaFont,(jstring)hJavaString,x,y);

  jstring shortcutLabel = (jstring) env->GetObjectField(self, 
                          WCachedIDs.PPCMenuItemPeer_shortcutLabelFID);

  if (!IsTopMenu() && shortcutLabel != NULL) {

    AwtFont *awtFont = AwtFont::GetFont(env, hJavaFont);
    textRect.right -= checkWidth;
    ::SelectObject(hDC, awtFont->GetHFont(0));
    DrawText(hDC, TO_WSTRING(shortcutLabel), env->GetStringLength(shortcutLabel), &textRect,
	                     DT_VCENTER | DT_RIGHT);

  }
  
  VERIFY(::SetBkMode(hDC,nOldBkMode));
}

void AwtMenuItem::DrawItem(DRAWITEMSTRUCT far& drawInfo)
{
  ASSERT(drawInfo.CtlType == ODT_MENU);
    
  if (drawInfo.itemID != m_Id)
    return;
    
  DrawSelf(drawInfo);
}

void AwtMenuItem::MeasureSelf(HDC hDC, MEASUREITEMSTRUCT far& measureInfo)
{
  jobject self = GetPeer();
  jobject hJavaFont = GetFont();
  jobject fontMetrics = GetFontMetrics(hJavaFont);
  jobject hJavaString = GetJavaString();
 
  SIZE size = AwtFont::getMFStringSize(hDC,hJavaFont,(jstring)hJavaString);

  JNIEnv *env;
  if (JVM->AttachCurrentThread((void **)&env, NULL) != 0)
    return;

#ifdef POCKETPC_MENUS
  if (IsTopMenu()) {
    measureInfo.itemHeight = GetSystemMetrics(SM_CYMENU) + 2;
  } else {
    measureInfo.itemHeight = env->CallIntMethod(fontMetrics, WCachedIDs.PPCFontMetrics_getHeightMID);
    measureInfo.itemHeight += measureInfo.itemHeight/3; //3 is a heuristic number
  }
#else
  measureInfo.itemHeight = env->CallIntMethod(fontMetrics, WCachedIDs.PPCFontMetrics_getHeightMID);
  measureInfo.itemHeight += measureInfo.itemHeight/3 + 3; //3 is a heuristic number
#endif


  measureInfo.itemWidth = size.cx;
  if (!IsTopMenu()) {
    int checkWidth = BM_SIZE;
    // The 4 is padding in case it's a submenu that displays the arrow 
    measureInfo.itemWidth += checkWidth + 4;
    jstring shortcutLabel = (jstring) env->GetObjectField(self, 
                          WCachedIDs.PPCMenuItemPeer_shortcutLabelFID);
        // Add in shortcut width, if one exists.
    if (shortcutLabel != NULL) {
      size = AwtFont::getMFStringSize(hDC, hJavaFont,
                                      shortcutLabel);
      measureInfo.itemWidth += size.cx + checkWidth;
    }

  } else {
      /* for some reason, items are not wide enough */
    measureInfo.itemWidth += 18;
  }
}

void AwtMenuItem::MeasureItem(HDC hDC, MEASUREITEMSTRUCT far& measureInfo)
{
  ASSERT(measureInfo.CtlType == ODT_MENU);
    
  if (measureInfo.itemID != m_Id)
	return;
    
  MeasureSelf(hDC, measureInfo);
}

jobject AwtMenuItem::GetFontMetrics(jobject hJavaFont)
{


  JNIEnv *env;
  if (JVM->AttachCurrentThread((void **)&env, NULL) != 0)
    return NULL;

  jobject fontMetrics = env->CallStaticObjectMethod(WCachedIDs.PPCFontMetricsClass,
		                WCachedIDs.PPCFontMetrics_getFontMetricsMID, hJavaFont);
// ASSERT(!exceptionOccurred(EE()));

  return fontMetrics;
}

BOOL AwtMenuItem::IsTopMenu()
{
  return FALSE;
}

void AwtMenuItem::DrawCheck(HDC hDC, RECT rect)
{
  if (bmpCheck == NULL) {
#if 1
    /* Resources with Library */
    bmpCheck = ::LoadBitmap((HINSTANCE)AwtToolkit::GetInstance().GetModuleHandle(),
                                TEXT("CHECK_BITMAP"));
#else
	/* resources are with the executable, not the library */
	/* this should probably be changed */
	 bmpCheck = ::LoadBitmap((HINSTANCE)GetModuleHandle(NULL),
				 TEXT("CHECK_BITMAP"));
#endif

     ASSERT(bmpCheck != NULL);
  }

#ifdef POCKETPC_MENUS
  int width = rect.right - rect.left;
  int height = rect.bottom - rect.top;
  if (width > height) {
    rect.left += (width-height)/2;
    rect.right -= (width-height)/2;
  }  else if (height > width) {
    rect.top += (height-width)/2;
    rect.bottom -= (height-width)/2;
  }
#else
  // Square the rectangle, so the check is proportional.
  int width = rect.right - rect.left;
  int diff = max(rect.bottom - rect.top - width, 0) ;
  int bottom = diff / 2;
  rect.bottom -= bottom;
  rect.top += diff - bottom;
#endif

  HDC hdcBitmap = ::CreateCompatibleDC(hDC);
  ASSERT(hdcBitmap != NULL);
  HBITMAP hbmSave = (HBITMAP)::SelectObject(hdcBitmap, bmpCheck);
  VERIFY(::StretchBlt(hDC, rect.left, rect.top, 
                      rect.right - rect.left, rect.bottom - rect.top,
                      hdcBitmap, 0, 0, BM_SIZE, BM_SIZE, SRCCOPY));
  ::SelectObject(hdcBitmap, hbmSave);
  VERIFY(::DeleteDC(hdcBitmap));
}

void AwtMenuItem::DoCommand(void)
{
  jobject peer = GetPeer();

  // Determine which modifier keys have been pressed.
  long modifiers = 0;
  if (HIBYTE(::GetAsyncKeyState(VK_CONTROL))) {
      modifiers |= java_awt_event_InputEvent_CTRL_MASK;
  }		
  if (HIBYTE(::GetAsyncKeyState(VK_RBUTTON))) {
      modifiers |= java_awt_event_InputEvent_META_MASK;
  }
  if (HIBYTE(::GetAsyncKeyState(VK_SHIFT))) {
      modifiers |= java_awt_event_InputEvent_SHIFT_MASK;
  }
  if (HIBYTE(::GetAsyncKeyState(VK_MBUTTON))) {
      modifiers |= java_awt_event_InputEvent_ALT_MASK;
  }

  JNIEnv *env;
  if (JVM->AttachCurrentThread((void **)&env, NULL) != 0)
    return;

  jboolean isCheckbox = env->GetBooleanField
	                    (peer, WCachedIDs.PPCMenuItemPeer_isCheckboxFID);
  if (isCheckbox) {

	MENUITEMINFO miInfo;
	
	miInfo.fMask = MIIM_STATE;
	::GetMenuItemInfo(GetMenuContainer()->GetHMenu(), GetID(), FALSE,
			   &miInfo);

	env->CallVoidMethod(peer, WCachedIDs.PPCCheckboxMenuItemPeer_handleActionMID, ((miInfo.fState & MFS_CHECKED) == 0), modifiers);

  } else {
	env->CallVoidMethod(peer, WCachedIDs.PPCMenuItemPeer_handleActionMID, modifiers);
  }
}

void AwtMenuItem::SetLabel(JavaStringBuffer *sb)
{
  AwtMenu* menu = GetMenuContainer();
  ASSERT(menu != NULL && GetID() >= 0);

  MENUITEMINFO mii;
  mii.cbSize = sizeof mii;
  mii.fMask = MIIM_TYPE;
  mii.fType = MFT_OWNERDRAW;
  mii.dwTypeData = (TCHAR *)(*sb);
  SetMenuItemInfo(menu->GetHMenu(), GetID(), FALSE, &mii);
   // Redraw menu bar if it was affected.
  if (menu->GetMenuBar() == menu) {
    ::DrawMenuBar(menu->GetOwnerHWnd());
  }
}

void AwtMenuItem::Enable(BOOL isEnabled)
{
  AwtMenu* menu = GetMenuContainer();
  ASSERT(menu != NULL && GetID() >= 0);
  VERIFY(::EnableMenuItem(menu->GetHMenu(), GetID(),
                          MF_BYCOMMAND | (isEnabled ? MF_ENABLED : MF_GRAYED))
         != 0xFFFFFFFF);

  // Redraw menu bar if it was affected.
  if (menu->GetMenuBar() == menu) {
    ::DrawMenuBar(menu->GetOwnerHWnd());
  }
}

void AwtMenuItem::SetState(BOOL isChecked)
{
  AwtMenu* menu = GetMenuContainer();
  ASSERT(menu != NULL && GetID() >= 0);
  VERIFY(::CheckMenuItem(menu->GetHMenu(), GetID(),
                         MF_BYCOMMAND | (isChecked ? MF_CHECKED : MF_UNCHECKED))
         != 0xFFFFFFFF);

  // Redraw menu bar if it was affected.
  if (menu->GetMenuBar() == menu) {
    ::DrawMenuBar(menu->GetOwnerHWnd());
  }
}

void AwtMenuItem::Dispose()
{
  AWT_TRACE((TEXT("AwtMenuItem::Dispose %x\n"), this));
  if (strcmp(GetClassName(), "SunAwtMenuItem") == 0) {
	AwtToolkit::GetInstance().RemoveCmdID( GetID() );
  }
  delete this;
}

LONG AwtMenuItem::WinThreadExecProc(ExecuteArgs * args)
{
  switch( args->cmdId ) {
	case MENUITEM_SETLABEL:
	{
      JavaStringBuffer * sb = (JavaStringBuffer *)args->param1;
	  ASSERT(!IsBadReadPtr(sb, sizeof(JavaStringBuffer)));
	  this->SetLabel(sb);
	}
	break;

	case MENUITEM_ENABLE:
	{
	  BOOL	isEnabled = (BOOL)args->param1;
	  this->Enable(isEnabled);
	}
	break;

	case MENUITEM_SETSTATE:
	{
      BOOL isChecked = (BOOL)args->param1;
	  this->SetState(isChecked);
	}
	break;

	case MENUITEM_DISPOSE:
	{
	  this->Dispose();
	}
	break;

	default:
	  AwtObject::WinThreadExecProc(args);
	  break;
    }
    return 0L;
}

