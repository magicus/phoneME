/*
 * @(#)PPCChoicePeer.h	1.7 06/10/10
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
/*
 * @(#)PPCChoicePeer.h	1.7 06/10/10
 */
#ifndef _WINCECHOICE_PEER_H_
#define _WINCECHOICE_PEER_H_

#include "PPCComponentPeer.h"

#include "java_awt_Choice.h"
#include "sun_awt_pocketpc_PPCChoicePeer.h"

//
// Component class for system provided buttons
//
class AwtChoice : public AwtComponent {
public:
    AwtChoice();
    ~AwtChoice();

    virtual const TCHAR *GetClassName();
    
    static AwtChoice* Create( jobject self,
                              jobject hParent );

    // Return the associated AWT peer object.
    INLINE jobject GetPeer() { return m_peerObject; }
	
    //Netscape : Override to handle changes in font properly after choice 
    //creation.
    virtual void SetFont( AwtFont *pFont );

    virtual void Reshape( int x, int y, int w, int h );
    void ResetDropDownHeight();
    int GetDropDownHeight();

#ifdef DEBUG
    void VerifyState(); // verify component and peer are in sync.
#endif

    //for multifont list
    jobject PreferredItemSize(JNIEnv* env);

    //////// Windows message handler functions

    MsgRouting WmNotify( UINT notifyCode );

    //for multifont choice
    MsgRouting WmDrawItem( UINT ctrlId, DRAWITEMSTRUCT far& drawInfo );
    MsgRouting WmMeasureItem( UINT ctrlId, MEASUREITEMSTRUCT far& measureInfo );
private:
    int GetFieldHeight();
    int GetTotalHeight();
};

#endif
