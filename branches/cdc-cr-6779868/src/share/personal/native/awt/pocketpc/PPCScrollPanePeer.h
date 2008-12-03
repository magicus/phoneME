/*
 * @(#)PPCScrollPanePeer.h	1.6 06/10/10
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
#ifndef _WINCESCROLL_PANE_PEER_H_
#define _WINCESCROLL_PANE_PEER_H_

#include "PPCComponentPeer.h"

class AwtScrollPane : public AwtComponent {
public:
    AwtScrollPane();

    virtual const TCHAR* GetClassName();
    
    static AwtScrollPane* Create(jobject self, jobject hParent);

    // Return the associated AWT peer object.
    INLINE jobject GetPeer() { 
	return m_peerObject; 
    }

    // Return the associated AWT target object.
    INLINE jobject GetTarget(JNIEnv* env) { 
        return env->GetObjectField(m_peerObject, WCachedIDs.PPCObjectPeer_targetFID);
    }

    void SetPageIncrement(int value, int orient);
    void SetInsets();
    void ScrollTo(int x, int y);
    int NewScrollPosition(UINT scrollCode, UINT orient, UINT pos,
                          UINT lineIncrement, UINT pageIncrement);
    void RecalcSizes(int parentWidth, int parentHeight, 
		     int childWidth, int childHeight);
    virtual void Show();
    virtual void Reshape(int x, int y, int w, int h);
    virtual void BeginValidate() {}
    virtual void EndValidate() {}

    //////// Windows message handler functions

    virtual MsgRouting WmHScroll(UINT scrollCode, UINT pos, HWND hScrollBar);
    virtual MsgRouting WmVScroll(UINT scrollCode, UINT pos, HWND hScrollBar);

#ifdef DEBUG
    virtual void VerifyState(); // verify target and peer are in sync.
#endif

private:
    void SetScrollInfo(int orient, int max, int page, BOOL disableNoScroll);
};

#endif // AWT_SCROLLPANE_H


