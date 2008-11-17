/*
 * @(#)PPCCheckboxPeer.cpp	1.13 06/10/10
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
#include "sun_awt_pocketpc_PPCButtonPeer.h"
#include "PPCCheckboxPeer.h"
#include "PPCCanvasPeer.h"

AwtCheckbox::AwtCheckbox() {
    m_fLButtonDowned = FALSE;
}

const TCHAR* 
AwtCheckbox::GetClassName() {
    // System provided checkbox class (a type of button) 
    return TEXT("BUTTON");  
}

// Create a new AwtCheckbox object and window.  
AwtCheckbox* AwtCheckbox::Create(jobject self,
                                 jobject hParent)
{
    JNIEnv* env; 
    /* associate JNIEnv with the current thread */
    if (JVM -> AttachCurrentThread((void**) &env, 0) != 0) {
       return NULL;
    }

    CHECK_NULL_RETURN(hParent, "null hParent");
    AwtCanvas* parent = (AwtCanvas*) env->GetIntField(hParent, 
                  WCachedIDs.PPCObjectPeer_pDataFID); 
    CHECK_NULL_RETURN(parent, "null parent");

    jobject target = 
       env -> GetObjectField(self, WCachedIDs.PPCObjectPeer_targetFID);
    CHECK_NULL_RETURN(target, "null target");

    AwtCheckbox* checkbox = new AwtCheckbox();
    CHECK_NULL_RETURN(checkbox, "AwtCheckbox() failed");

    DWORD style = WS_CHILD | WS_CLIPSIBLINGS | BS_OWNERDRAW;

    jstring label = (jstring)env->CallObjectMethod(target,
                                  WCachedIDs.java_awt_Checkbox_getLabelMID); 

    TCHAR *sb;
    if (label == NULL) {
       sb = TEXT("");
    } else {
       JavaStringBuffer sb(env, label);
    }

    checkbox->CreateHWnd(sb, style, 0,
                  (int)env->CallIntMethod(target, 
                            WCachedIDs.java_awt_Component_getXMID),
                  (int)env->CallIntMethod(target,
                            WCachedIDs.java_awt_Component_getYMID),
                  (int)env->CallIntMethod(target,
                            WCachedIDs.java_awt_Component_getWidthMID),
                  (int)env->CallIntMethod(target, 
                            WCachedIDs.java_awt_Component_getHeightMID),
                  parent->GetHWnd(),
                  (HMENU)parent->CreateControlID(),
                  ::GetSysColor(COLOR_WINDOWTEXT),
                  ::GetSysColor(COLOR_BTNFACE),
                  self
                  );

    return checkbox;
}

MsgRouting
AwtCheckbox::WmNotify(UINT notifyCode)
{
    if (notifyCode == BN_CLICKED) {
        BOOL fChecked = !GetState();

        JNIEnv* env; 
        /* associate JNIEnv with the current thread */
        if (JVM -> AttachCurrentThread((void**) &env, 0) != 0) {
           return mrDoDefault;  // CHECK: is this a right return type??
        }
 
        env->CallVoidMethod(m_peerObject,
                            WCachedIDs.PPCCheckboxPeer_handleActionMID, fChecked);
        
    }
    return mrDoDefault;
}

BOOL AwtCheckbox::GetState()
{
    jobject target = (jobject)GetTarget();

    JNIEnv* env; 
    /* associate JNIEnv with the current thread */
    if (JVM -> AttachCurrentThread((void**) &env, 0) != 0) {
       return FALSE;  // CHECK: is this a right return type??
    }
 
    return env-> CallBooleanMethod(target,
                           WCachedIDs.java_awt_Checkbox_getStateMID); 
 
    //return unhand(target)->state;
}

int AwtCheckbox::GetCheckSize()
{
    //using height of small icon for check mark size
    return ::GetSystemMetrics(SM_CYSMICON);
}

MsgRouting
AwtCheckbox::WmDrawItem(UINT /*ctrlId*/, DRAWITEMSTRUCT far& drawInfo)
{
    JNIEnv *env; 

    /* associate JNIEnv with the current thread */
    if (JVM -> AttachCurrentThread((void**) &env, 0) != 0) {
       return mrDoDefault; // CHECK: is this the right return type??
    }

    CriticalSection::Lock l(GetLock());

    jobject self = GetPeer();
    jobject target = 
       env -> GetObjectField(self, WCachedIDs.PPCObjectPeer_targetFID);

    HDC hDC = drawInfo.hDC;
    RECT rect = drawInfo.rcItem;
    int checkSize;
    UINT nState;
    SIZE size;
    jobject hJavaFont;
    jstring hJavaString;

    hJavaFont = GET_FONT(target);
    hJavaString = (jstring)env->CallObjectMethod(target,
                                  WCachedIDs.java_awt_Checkbox_getLabelMID); 
    size = AwtFont::getMFStringSize(hDC,hJavaFont,hJavaString);
 
    if (env->CallObjectMethod(target, WCachedIDs.java_awt_Checkbox_getCheckboxGroupMID) != NULL)
        nState = DFCS_BUTTONRADIO;
    else
        nState = DFCS_BUTTONCHECK;

    if (GetState())
        nState |= DFCS_CHECKED;
    else
        nState &= ~DFCS_CHECKED;

    if (drawInfo.itemState & ODS_SELECTED)
        nState |= DFCS_PUSHED;

    if (drawInfo.itemAction & ODA_DRAWENTIRE) {
        VERIFY(::FillRect (hDC, &rect, GetBackgroundBrush()));
    }

    //draw check mark
    checkSize = GetCheckSize();
    RECT boxRect;
    boxRect.left = rect.left;
    boxRect.top = (rect.bottom - rect.top - checkSize)/2;
    boxRect.right = boxRect.left + checkSize;
    boxRect.bottom = boxRect.top + checkSize;
    ::DrawFrameControl(hDC, &boxRect, DFC_BUTTON, nState);

    //draw string
    rect.left = rect.left + checkSize + checkSize/4; //4 is a heuristic number
    if (drawInfo.itemAction & ODA_DRAWENTIRE) {
        BOOL bEnabled = isEnabled();

        int x = rect.left;
        int y = (rect.top+rect.bottom-size.cy)/2;
        if (bEnabled) {
            AwtComponent::DrawWindowText(hDC, hJavaFont, hJavaString, x, y);
        } else {
            AwtComponent::DrawGrayText(hDC, hJavaFont, hJavaString, x, y);
        }
    }

    //Draw focus rect
    RECT focusRect;
    const int margin = 2; // 2 is a heuristic number
    focusRect.left = rect.left - margin;
    focusRect.top = (rect.top+rect.bottom-size.cy)/2;
    focusRect.right = focusRect.left + size.cx + 2*margin;
    focusRect.bottom = focusRect.top + size.cy;
    
    // draw focus rect
    if ((drawInfo.itemState & ODS_FOCUS) &&
        ((drawInfo.itemAction & ODA_FOCUS)||
         (drawInfo.itemAction &ODA_DRAWENTIRE))) {        
              VERIFY(::DrawFocusRect(hDC, &focusRect));
    }
    // erase focus rect
    else if (!(drawInfo.itemState & ODS_FOCUS) &&
             (drawInfo.itemAction & ODA_FOCUS)) {
        VERIFY(::DrawFocusRect(hDC, &focusRect));
    }

    // Notify any subclasses
    rect = drawInfo.rcItem;

    env -> CallVoidMethod(GetPeer(),
                          WCachedIDs.PPCComponentPeer_handlePaintMID,
                          rect.left, rect.top, 
                          rect.right-rect.left, rect.bottom-rect.top);

    return mrConsume;
}

MsgRouting AwtCheckbox::WmPaint(HDC)
{
    // Suppress peer notification, because it's handled in WmDrawItem.
    return mrDoDefault;
}

#ifdef DEBUG
void AwtCheckbox::VerifyState()
{
    if (AwtToolkit::GetInstance().VerifyComponents() == FALSE) {
        return;
    }

    if (m_callbacksEnabled == FALSE) {
        // Component is not fully setup yet.
        return;
    }

    AwtComponent::VerifyState();

    // prehaps we don't need this?
    //jobject hTarget = GetTarget();
    //jobject target = unhand(hTarget);
    jobject target = GetTarget();

    // Check button style
    DWORD style = ::GetWindowLong(GetHWnd(), GWL_STYLE);
    ASSERT(style & BS_OWNERDRAW);

    // Check label
    int len = ::GetWindowTextLength(GetHWnd());
    TCHAR* peerStr = new TCHAR[len+1];
    GetText(peerStr, len+1);
/* FIXME */
#ifndef UNICODE
    //ASSERT(strcmp(peerStr, JavaStringBuffer(target->label)) == 0);
#else
    //ASSERT(wcscmp(peerStr, JavaStringBuffer(target->label)) == 0);
#endif /* UNICODE */
}
#endif /* DEBUG */


/**
 * JNI Functions
 */

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCCheckboxPeer_initIDs (JNIEnv *env, jclass cls) {
    GET_METHOD_ID (PPCCheckboxPeer_handleActionMID, 
                   "handleAction", "(Z)V");

    cls = env->FindClass ("java/awt/Checkbox");
    if (cls == NULL)
	  return;
    GET_METHOD_ID (java_awt_Checkbox_getLabelMID, 
                   "getLabel", "()Ljava/lang/String;");
    GET_METHOD_ID (java_awt_Checkbox_getStateMID, 
                   "getState", "()Z");
    GET_METHOD_ID (java_awt_Checkbox_getCheckboxGroupMID, 
                   "getCheckboxGroup", "()Ljava/awt/CheckboxGroup;");
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCCheckboxPeer_create (JNIEnv *env, jobject thisObj,
                                                jobject hParent)
{
    CHECK_PEER(hParent);
    AwtToolkit::CreateComponent(
       thisObj, hParent, (AwtToolkit::ComponentFactory)AwtCheckbox::Create);

    CHECK_PEER_CREATION(thisObj);

#ifdef DEBUG
    AwtComponent* parent = (AwtComponent*) env->GetIntField(thisObj,
                  WCachedIDs.PPCObjectPeer_pDataFID);
    parent->VerifyState();
#endif 
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCCheckboxPeer_setState (JNIEnv *env, jobject thisObj,
                                                  jboolean state)
{
    CHECK_PEER(thisObj);
    AwtComponent* c = (AwtComponent*) env->GetIntField(thisObj,
                  WCachedIDs.PPCObjectPeer_pDataFID);

    //when multifont and group checkbox receive setState native method,
    //it must be redraw to display correct check mark
    jobject target =
       env -> GetObjectField(thisObj, WCachedIDs.PPCObjectPeer_targetFID);

    if (env->CallObjectMethod(target, WCachedIDs.java_awt_Checkbox_getCheckboxGroupMID) != NULL) {
        HWND hWnd = c->GetHWnd();
        RECT rect;
        VERIFY(::GetWindowRect(hWnd,&rect));
        VERIFY(::ScreenToClient(hWnd, (LPPOINT)&rect));
        VERIFY(::ScreenToClient(hWnd, ((LPPOINT)&rect)+1));
        VERIFY(::InvalidateRect(hWnd,&rect,TRUE));
        VERIFY(::UpdateWindow(hWnd));
    } else {
        c->SendMessage(BM_SETCHECK, (WPARAM)(state ? BST_CHECKED : BST_UNCHECKED));
        VERIFY(::InvalidateRect(c->GetHWnd(), NULL, FALSE));
    }

    c->VerifyState();
}

long
Java_sun_awt_pocketpc_PPCCheckboxPeer_getState(JNIEnv * env, jobject self)
{
    CHECK_PEER_RETURN(self);
    AwtComponent* c = (AwtComponent*) env->GetIntField(self,
                  WCachedIDs.PPCObjectPeer_pDataFID);
    c->VerifyState();
    return c->SendMessage(BM_GETCHECK);
}

void
Java_sun_awt_pocketpc_PPCCheckboxPeer_setLabelNative(JNIEnv *env, jobject self,
                                       jstring label)
{
    CHECK_PEER(self);
    AwtComponent* c = (AwtComponent*) env->GetIntField(self,
                  WCachedIDs.PPCObjectPeer_pDataFID);
    c->SetText(JavaStringBuffer(env, label));
}

void
Java_sun_awt_pocketpc_PPCCheckboxPeer_setCheckboxGroup(JNIEnv *env,
						       jobject self,
						       jobject group)
{
    CHECK_PEER(self);
#ifdef DEBUG
    if (group != NULL) {
        CHECK_OBJ(group);
	jclass cls = env->FindClass("java/awt/CheckboxGroup");
        ASSERT(env->IsInstanceOf(group, cls));
    }
#endif
    AwtComponent* c = (AwtComponent*) env->GetIntField(self,
                  WCachedIDs.PPCObjectPeer_pDataFID);
    c->SendMessage(BM_SETSTYLE, (WPARAM)BS_OWNERDRAW, (LPARAM)TRUE);
    c->VerifyState();
}
