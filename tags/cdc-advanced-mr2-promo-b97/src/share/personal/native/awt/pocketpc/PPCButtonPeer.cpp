/*
 * @(#)PPCButtonPeer.cpp	1.10 06/10/10
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
#include "sun_awt_pocketpc_PPCButtonPeer.h"
#include "PPCButtonPeer.h"

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCButtonPeer_initIDs (JNIEnv *env, jclass cls)
{
    GET_METHOD_ID(PPCButtonPeer_handleActionMID, 
                  "handleAction", "()V");

    cls = env->FindClass ("java/awt/Button");
    if (cls == NULL)
	return;

    GET_METHOD_ID (java_awt_Button_getLabelMID, "getLabel", 
                   "()Ljava/lang/String;");
}

AwtButton::AwtButton(){}

const TCHAR* 
AwtButton::GetClassName() {
    return TEXT("BUTTON");  /* System provided button class */
}

AwtButton* 
AwtButton::Create(jobject self, jobject hParent) {
    JNIEnv *env; 

    /* associate JNIEnv with the current thread */
    if (JVM -> AttachCurrentThread((void**) &env, 0) != 0) {
       return NULL;
    }

    AwtCanvas* parent = (AwtCanvas*) env->GetIntField(hParent, 
      	          WCachedIDs.PPCObjectPeer_pDataFID); 

    CHECK_NULL_RETURN(parent, "null parent");

    jobject target = 
       env -> GetObjectField(self, WCachedIDs.PPCObjectPeer_targetFID);

    CHECK_NULL_RETURN(target, "null target");

    AwtButton* c = new AwtButton();
    CHECK_NULL_RETURN(c, "AwtButton() failed");

    jstring label = (jstring)env->CallObjectMethod(target,
						   WCachedIDs.java_awt_Button_getLabelMID); 
    JavaStringBuffer sb(env, label);

    DWORD style = WS_CHILD | WS_CLIPSIBLINGS | BS_PUSHBUTTON | BS_OWNERDRAW;

    c->CreateHWnd(sb, style, 0,
                  (int)env->CallIntMethod(target, WCachedIDs.java_awt_Component_getXMID),
                  (int)env->CallIntMethod(target, WCachedIDs.java_awt_Component_getYMID),
                  (int)env->CallIntMethod(target, WCachedIDs.java_awt_Component_getWidthMID),
                  (int)env->CallIntMethod(target, WCachedIDs.java_awt_Component_getHeightMID),
                  parent->GetHWnd(),
                  (HMENU)parent->CreateControlID(),
                  ::GetSysColor(COLOR_BTNTEXT),
                  ::GetSysColor(COLOR_BTNFACE),
                  self
                 );

    c->m_backgroundColorSet = TRUE;  // suppress inheriting parent's color.
    return c;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCButtonPeer_create (JNIEnv *env, jobject thisobj, jobject parent)
{
    CHECK_PEER(parent);
    AwtToolkit::CreateComponent(
       thisobj, parent, (AwtToolkit::ComponentFactory)AwtButton::Create);
    CHECK_PEER_CREATION(thisobj);
}


MsgRouting 
AwtButton::WmNotify(UINT notifyCode) {

    
    if (notifyCode == BN_CLICKED) {
       JNIEnv *env;
       if (JVM -> AttachCurrentThread((void**) &env, 0) != 0) {
          return mrDoDefault;  // IS THIS a right thing to do??
       }

       env->CallVoidMethod(m_peerObject,
                           WCachedIDs.PPCButtonPeer_handleActionMID);
    }
    return mrDoDefault;
}

MsgRouting 
AwtButton::WmDrawItem(UINT ctrlId, DRAWITEMSTRUCT far& drawInfo) {

    JNIEnv *env;
    if (JVM -> AttachCurrentThread((void**) &env, 0) != 0) {
       return mrDoDefault;
    }

    CriticalSection::Lock l(GetLock());
    jobject self = GetPeer();
    jobject target = env-> GetObjectField(
                           self, WCachedIDs.PPCObjectPeer_targetFID); 
    HDC hDC = drawInfo.hDC;
    RECT rect = drawInfo.rcItem;
    UINT nState;
    SIZE size;
    jobject hJavaFont;
    jstring hJavaString;

    //Draw Button
    nState = DFCS_BUTTONPUSH;
    if (drawInfo.itemState & ODS_SELECTED)
        nState |= DFCS_PUSHED;

    ::FillRect(hDC, &rect, GetBackgroundBrush());
    UINT edgeType = (nState & DFCS_PUSHED) ? EDGE_SUNKEN : EDGE_RAISED;
    ::DrawEdge(hDC, &rect, edgeType, BF_RECT | BF_SOFT);

    //Draw WindowText

    hJavaFont = GET_FONT(target);
    hJavaString = (jstring)env->CallObjectMethod(target, 
                                WCachedIDs.java_awt_Button_getLabelMID);
    size = AwtFont::getMFStringSize(hDC,hJavaFont,hJavaString);

    //Check whether the button is disabled.
    BOOL bEnabled = isEnabled();

    int adjust = (nState & DFCS_PUSHED) ? 1 : 0;
    int x = (rect.left+rect.right-size.cx)/2 + adjust;
    int y = (rect.top+rect.bottom-size.cy)/2 + adjust;

    if (bEnabled) {
        AwtComponent::DrawWindowText(hDC, hJavaFont, hJavaString, x, y);
    } else {
        AwtComponent::DrawGrayText(hDC, hJavaFont, hJavaString, x, y);
    }

    //Draw focus rect
    if (drawInfo.itemState & ODS_FOCUS){
        const int inf = 3; // heuristic decision
        RECT focusRect;
        VERIFY(::CopyRect(&focusRect, &rect));
        VERIFY(::InflateRect(&focusRect,-inf,-inf));
        VERIFY(::DrawFocusRect(hDC, &focusRect));
    }

    // Notify any subclasses
    env -> CallVoidMethod(self,
                          WCachedIDs.PPCComponentPeer_handlePaintMID,
                          rect.left, rect.top,
                          rect.right-rect.left, rect.bottom-rect.top);
   
    return mrConsume;
}

MsgRouting 
AwtButton::WmPaint(HDC hDC) {
    /* Suppress peer notification, because it's handled in WmDrawItem.*/
    return mrDoDefault;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCButtonPeer_setLabelNative (JNIEnv *env, 
                                                      jobject thisobj,
                                                      jstring label)
                                                      //jbyteArray label) - JavaStringBuffer should take care of adding the null at the end of the string
{
    CHECK_PEER(thisobj);

    AwtComponent* c = (AwtComponent*) env->GetIntField(thisobj, 
      	          WCachedIDs.PPCObjectPeer_pDataFID); 
    c->SetText(JavaStringBuffer(env, label));
#ifdef XXX
    #define MAX_LABEL 256
    TCHAR uBuf[MAX_LABEL+1];
    int length = javaStringLength(label);
    if (length > MAX_LABEL) {
      length = MAX_LABEL;
    }
    javaString2unicode(label, uBuf, length);
    uBuf[length] = 0;
    c->SetText(uBuf);
#endif /* WINCE */
}

