/*
 * @(#)PPCTextFieldPeer.cpp	1.6 06/10/10
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
#include "sun_awt_pocketpc_PPCTextFieldPeer.h"
#include "PPCTextFieldPeer.h"
#include "PPCCanvasPeer.h"

/** 
 * Constructor
 */ 
AwtTextField :: AwtTextField() //      : AwtTextComponent()
{

}

// Create a new AwtTextField object and window.  
AwtTextField* AwtTextField::Create(jobject peer, jobject hParent)
{
   JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return NULL;
    }
    CHECK_NULL_RETURN(hParent, "null hParent");
    AwtCanvas* parent = (AwtCanvas*) env->GetIntField(hParent, 
      	                              WCachedIDs.PPCObjectPeer_pDataFID); 
    CHECK_NULL_RETURN(parent, "null parent");
    jobject target = env->GetObjectField(peer, 
                                         WCachedIDs.PPCObjectPeer_targetFID);
    CHECK_NULL_RETURN(target, "null target");

    AwtTextField* c = new AwtTextField();
    CHECK_NULL_RETURN(c, "AwtTextField() failed");

    DWORD style = WS_CHILD | WS_CLIPSIBLINGS |
                  ES_LEFT | ES_AUTOHSCROLL | ES_NOHIDESEL | 
                  (IS_WIN4X ? 0 : WS_BORDER);
    DWORD exStyle = IS_WIN4X ? WS_EX_CLIENTEDGE : 0;
    c->CreateHWnd(TEXT(""), style, exStyle,
         (int)env->CallIntMethod(target, WCachedIDs.java_awt_Component_getXMID),
         (int)env->CallIntMethod(target, WCachedIDs.java_awt_Component_getYMID),
         (int)env->CallIntMethod(target,WCachedIDs.java_awt_Component_getWidthMID),
         (int)env->CallIntMethod(target,WCachedIDs.java_awt_Component_getHeightMID),
                  parent->GetHWnd(),
                  (HMENU)parent->CreateControlID(),
                  ::GetSysColor(COLOR_WINDOWTEXT),
                  ::GetSysColor(COLOR_WINDOW),
                  peer
                 );

    c->m_backgroundColorSet = TRUE;  // suppress inheriting parent's color.
    return c;
}

MsgRouting AwtTextField::WmChar(UINT character, UINT repCnt, UINT flags, BOOL system)
{
    if (!system) {
        if (character == VK_RETURN) {
            JNIEnv *env;
            if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
                return mrPassAlong;
            }
            env->CallVoidMethod(GetPeer(), 
                                WCachedIDs.PPCTextFieldPeer_handleActionMID);
            return mrConsume;
        }
        if (character == VK_TAB) {
            // illegal character -- handled in Java code.
            return mrConsume;
        }
    }
    return AwtComponent::WmChar(character, repCnt, flags, system);
}


/**
 * JNI Functions
 */ 
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCTextFieldPeer_initIDs (JNIEnv *env, jclass cls)
{
    GET_METHOD_ID (PPCTextFieldPeer_handleActionMID, "handleAction", "()V");
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCTextFieldPeer_create (JNIEnv *env, jobject thisObj, 
                                               jobject hParent )
{
    CHECK_PEER(hParent);
    AwtToolkit::CreateComponent(thisObj, hParent, (AwtToolkit::ComponentFactory)
                                AwtTextField::Create);
    
    CHECK_PEER_CREATION(thisObj);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCTextFieldPeer_setEchoCharacter (JNIEnv *env, 
                                                         jobject self, jchar ch)
{
    CHECK_PEER(self);
    AwtComponent* c = PDATA(AwtComponent, self);
    c->SendMessage(EM_SETPASSWORDCHAR, ch);
}
