/*
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
 */

#include "PPCObjectPeer.h"
#include "ObjectList.h"
#ifdef DEBUG
#include "PPCUnicode.h"
#endif

#ifdef DEBUG
static BOOL reportEvents = FALSE;
#endif

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCObjectPeer_initIDs (JNIEnv *env, jclass cls)
{
    WCachedIDs.PPCObjectPeerClass = (jclass) env->NewGlobalRef(cls);
    
    GET_FIELD_ID (PPCObjectPeer_pDataFID, "pData", "I");
    GET_FIELD_ID (PPCObjectPeer_targetFID, "target",
		  "Ljava/lang/Object;");
    
    GET_STATIC_METHOD_ID (PPCObjectPeer_getPeerForTargetMID,
			  "getPeerForTarget",
			  "(Ljava/lang/Object;)Lsun/awt/pocketpc/PPCObjectPeer;");
}

AwtObject *
AwtObject::GetPeerFromJNI(JNIEnv *env, jobject peer) 
{
    AwtObject *peerObj = (AwtObject *)
	env->GetIntField(peer, WCachedIDs.PPCObjectPeer_pDataFID);
    
    if (peerObj == 0) {
	env->ThrowNew (WCachedIDs.NullPointerExceptionClass,
		       "peer object is null");
    }

    return peerObj;
}

AwtObject::AwtObject()
{
    theAwtObjectList.Add(this);
    m_peerObject = NULL;
    m_callbacksEnabled = TRUE;
}

AwtObject::~AwtObject()
{
    theAwtObjectList.Remove(this);

    if (m_peerObject != 0) {
	JNIEnv *env;
	
	if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	    return;
	}

	env->DeleteGlobalRef(m_peerObject);
    }
}

// Return the peer associated with some target.  This information is
// maintained in a hashtable at the java level.  
jobject AwtObject::GetPeerForTarget(jobject target)
{
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return NULL;
    }

    jobject peer = env->CallStaticObjectMethod(WCachedIDs.PPCObjectPeerClass,
					       WCachedIDs.PPCObjectPeer_getPeerForTargetMID, target);

    ASSERT(!env->ExceptionCheck());
    return peer;
}

//might need to be moved to PPCComponentPeer
void AwtObject::SendEvent(jobject hEvent)
{
    JNIEnv *env;

    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return;
    }

#ifdef DEBUG
    /* FIX or REMOVE
    if (reportEvents) {
        jstring eventStr = (jstring) env->CallObjectMethod(hEvent,
							   WCachedIDs.java_lang_Object_toStringMID);
        ASSERT(!env->ExceptionCheck());
        jstring targetStr =  (jstring)
	    env->CallObjectMethod(GetTarget(),
				  WCachedIDs.java_awt_Component_getNameMID);
        ASSERT(!env->ExceptionCheck());
        printf("Posting %S to %S\n", GET_WSTRING(eventStr),
               GET_WSTRING(targetStr));
    }
    */
#endif

    // Post event to the system EventQueue.
    env->CallVoidMethod(GetPeer(),
			WCachedIDs.PPCComponentPeer_postEventMID,
			hEvent);
    ASSERT(!env->ExceptionCheck());
}

//
// (static)
// Switches to Windows thread via SendMessage and synchronously
// calls AwtObject::WinThreadExecProc with the given command id
// and parameters.
//
// Useful for writing code that needs to be synchronized with
// what's happening on the Windows thread.
//
LONG AwtObject::WinThreadExec(jobject peerObject,
			      UINT cmdId,
			      LPARAM param1,
			      LPARAM param2,
			      LPARAM param3,
			      LPARAM param4 )
{
    ASSERT( IsWindow( AwtToolkit::GetInstance().GetHWnd() ) );

    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, NULL) != 0) {
	return 0L;
    }

    AwtObject* object = PDATA(AwtObject, peerObject);
    ASSERT( !IsBadReadPtr(object, sizeof(AwtObject)) );

    ExecuteArgs		args;
    LONG		retVal;

    // setup arguments
    args.cmdId = cmdId;
    args.param1 = param1;
    args.param2 = param2;
    args.param3 = param3;
    args.param4 = param4;

    // call WinThreadExecProc on the toolkit thread
    retVal = ::SendMessage(AwtToolkit::GetInstance().GetHWnd(), WM_AWT_EXECUTE_SYNC, (WPARAM)object, (LPARAM)&args);

    return retVal;
}

LONG AwtObject::WinThreadExecProc(ExecuteArgs * args)
{
    ASSERT(FALSE); // no default handler
    return 0L;
}
