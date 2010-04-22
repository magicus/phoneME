/*
 * @(#)PPCCanvasPeer.cpp	1.7 06/10/10
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
#include "PPCCanvasPeer.h"
#include "PPCGraphics.h"
#include "sun_awt_pocketpc_PPCCanvasPeer.h"

/**
 * Constructor
 */
AwtCanvas :: AwtCanvas() {}
AwtCanvas :: ~AwtCanvas() {}

const TCHAR *
AwtCanvas::GetClassName() {
   return TEXT("SunAwtCanvas");
}

AwtCanvas* 
AwtCanvas::Create(jobject self, jobject hParent) {

    JNIEnv *env; 

    /* associate JNIEnv with the current thread */
    if (JVM -> AttachCurrentThread((void**) &env, 0) != 0) {
       return NULL;
    }

    CHECK_NULL_RETURN(hParent, "null hParent");

    AwtComponent* parent = (AwtComponent*) env->GetIntField(hParent, 
                  WCachedIDs.PPCObjectPeer_pDataFID); 

    CHECK_NULL_RETURN(parent, "null parent");

    jobject target = 
       env -> GetObjectField(self, WCachedIDs.PPCObjectPeer_targetFID);

    CHECK_NULL_RETURN(target, "null target");

    AwtCanvas* canvas = new AwtCanvas();
    CHECK_NULL_RETURN(canvas, "AwtCanvas() failed");

    canvas->CreateHWnd(TEXT(""),
                       WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0,
                       (int)env->CallIntMethod(target, WCachedIDs.java_awt_Component_getXMID),
                       (int)env->CallIntMethod(target, WCachedIDs.java_awt_Component_getYMID),
                       (int)env->CallIntMethod(target, WCachedIDs.java_awt_Component_getWidthMID),
                       (int)env->CallIntMethod(target, WCachedIDs.java_awt_Component_getHeightMID),
                       parent->GetHWnd(),
                       NULL,
                       ::GetSysColor(COLOR_WINDOWTEXT),
                       ::GetSysColor(COLOR_WINDOW),
                       self
                      );

    return canvas;
}

MsgRouting 
AwtCanvas::WmEraseBkgnd(HDC hDC, BOOL& didErase) {
    m_eraseBackground = TRUE;
    didErase = TRUE;
    return mrConsume;
}

MsgRouting 
AwtCanvas::WmPaint(HDC hDC) {

    // get the shared DC and lock it
    AwtSharedDC *pDC = GetSharedDC();
    if (pDC == NULL) {
       return mrPassAlong;  // CHECK: is this the right value to return with?
    }
    pDC->Lock();
    pDC->GrabDC(NULL);
    PAINTSTRUCT ps;
    HDC hDC2 = ::BeginPaint(GetHWnd(), &ps);// the passed-in HDC is ignored.
    ASSERT(hDC2);
    RECT& r = ps.rcPaint;
    pDC->Unlock();

    if ((r.right-r.left) > 0 && (r.bottom-r.top) > 0 &&
        m_peerObject != NULL && m_callbacksEnabled) { 
  
        JNIEnv *env;

        /* associate JNIEnv with the current thread */
        if (JVM -> AttachCurrentThread((void**) &env, 0) != 0) {
           return mrPassAlong;  // IS THIS the right return value???
        }

        env -> CallVoidMethod(m_peerObject,
                              WCachedIDs.PPCComponentPeer_handleExposeMID,
                              r.left, r.top, r.right-r.left, r.bottom-r.top);
    }
    m_eraseBackground = FALSE;
    VERIFY(::EndPaint(GetHWnd(), &ps));

    return mrConsume;
}

MsgRouting 
AwtCanvas::WmMouseDown(UINT flags, int x, int y, int button) {
    ASSERT(::GetCapture() == NULL || ::GetCapture() == GetHWnd()); // don't want to interfere with other controls
    if (::GetCapture() != GetHWnd()) {
        ::SetCapture(GetHWnd());
    }
    return AwtComponent::WmMouseDown(flags, x, y, button);
}

MsgRouting 
AwtCanvas::WmMouseUp(UINT flags, int x, int y, int button){
    BOOL        isMouseCaptured = (::GetCapture() == GetHWnd());
    BOOL        areButtonsReleased = (flags & ALL_MK_BUTTONS) == 0;

    if ( isMouseCaptured && areButtonsReleased ) {
    // user has released all buttons, so release the capture
        ::ReleaseCapture();
    }
    return AwtComponent::WmMouseUp(flags, x, y, button);
}

/**
 * JNI Functions
 */ 
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCCanvasPeer_create (JNIEnv *env, jobject thisObj, jobject parent)
{
    CHECK_PEER(parent);
    AwtToolkit::CreateComponent(thisObj, parent, 
                (AwtToolkit::ComponentFactory)AwtCanvas::Create);

    CHECK_PEER_CREATION(thisObj);
}

