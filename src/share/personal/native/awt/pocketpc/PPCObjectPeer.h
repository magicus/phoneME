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

#ifndef _WINCE_OBJECT_H
#define _WINCE_OBJECT_H

#include "awt.h"
#include "PPCToolkit.h"
#include "sun_awt_pocketpc_PPCObjectPeer.h"
#include "java_awt_Event.h"
#include "java_awt_AWTEvent.h"

class AwtObject {
public:
    class ExecuteArgs {
	public:
	    UINT	cmdId;
	    LPARAM	param1;
	    LPARAM	param2;
	    LPARAM	param3;
	    LPARAM	param4;
    };

    AwtObject();
    virtual ~AwtObject();

    // Return the associated AWT peer or target object.
    INLINE jobject GetPeer() { 
	return m_peerObject; 
    }
    INLINE jobject GetTarget() { 
	JNIEnv *env;
	
	if (JVM->AttachCurrentThread((void **) &env, NULL) != 0) {
	    return NULL;
	}
	
    	return env->GetObjectField(m_peerObject,
				   WCachedIDs.PPCObjectPeer_targetFID);
    }

    static AwtObject *GetPeerFromJNI(JNIEnv *env, jobject peer);

    // Return the peer associated with some target
    jobject GetPeerForTarget(jobject target);

    // Allocate and initialize a new event, and post it to the peer's
    // target object.  No response is expected from the target.
    void SendEvent(jobject hEvent);

    INLINE void EnableCallbacks(BOOL e) { m_callbacksEnabled = e; }

    // Execute any code associated with a command ID -- only classes with
    // DoCommand() defined should associate their instances with cmdIDs.
    virtual void DoCommand(void) {
        ASSERT(FALSE);
    }

    // execute given code on Windows message-pump thread
    static LONG WinThreadExec(jobject peerObject, UINT cmdId, LPARAM param1 = 0L, LPARAM param2 = 0L, LPARAM param3 = 0L, LPARAM param4 = 0L);
    // callback function to execute code on Windows message-pump thread
    virtual LONG WinThreadExecProc(AwtObject::ExecuteArgs * args);

protected:

    jobject m_peerObject;
    BOOL                          m_callbacksEnabled;
};

#endif // _WINCE_OBJECT_H
