/*
 * @(#)PPCListPeer.h	1.6 06/10/10
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
#ifndef _WINCELIST_PEER_H_
#define _WINCELIST_PEER_H_

#include "PPCComponentPeer.h"

#include "java_awt_List.h"
#include "sun_awt_pocketpc_PPCListPeer.h"
#include "PPCGraphics.h"


//
// Component class for system provided buttons
//
class AwtList : public AwtComponent {
public:
    AwtList();

    virtual const TCHAR* GetClassName();
    
    static AwtList* Create(jobject self,
			   jobject hParent);

    // Return the associated AWT peer object.
    INLINE jobject GetPeer() { 
	return m_peerObject; 
    }

    virtual BOOL NeedDblClick() { return TRUE; }

    INLINE void Select(int pos) {
        if (isMultiSelect) {
            SendMessage(LB_SETSEL, TRUE, pos);
        }
        else {
            SendMessage(LB_SETCURSEL, pos);
        }
    }
    INLINE void Deselect(int pos) {
        if (isMultiSelect) {
            SendMessage(LB_SETSEL, FALSE, pos);
        }
        else {
            SendMessage(LB_SETCURSEL, (WPARAM)-1);
        }
    }
    INLINE int GetCount() {
        return SendMessage(LB_GETCOUNT);
    }
    INLINE int GetCurrentSelection() {
        return SendMessage(LB_GETCURSEL);
    }
    INLINE void InsertString(int index, TCHAR* str) {
        VERIFY(SendMessage(LB_INSERTSTRING, index, (LPARAM)str) != LB_ERR);
    }
    INLINE BOOL IsItemSelected(int index) {
        int ret = SendMessage(LB_GETSEL, index);
        ASSERT(ret != LB_ERR);
        return (ret > 0);
    }

    // Adjust the horizontal scrollbar as necessary
    void AdjustHorizontalScrollbar();
    void UpdateMaxItemWidth();

    INLINE long GetMaxWidth() {
        return m_nMaxWidth;
    }

    INLINE void CheckMaxWidth(long nWidth) {
        if (nWidth > m_nMaxWidth) {
            m_nMaxWidth = nWidth;
            AdjustHorizontalScrollbar();
        }
    }
	
	//Netscape : Change the font on the list and redraw the 
	//items nicely.
	virtual void SetFont(AwtFont *pFont);

    // Set whether a list accepts single or multiple selections.
    void SetMultiSelect(BOOL ms);

    //for multifont list
    jobject PreferredItemSize(JNIEnv* env);

    //////// Windows message handler functions

    MsgRouting WmNotify(UINT notifyCode);

    //for multifont list
    MsgRouting WmDrawItem(UINT ctrlId, DRAWITEMSTRUCT far& drawInfo);
    MsgRouting WmMeasureItem(UINT ctrlId, MEASUREITEMSTRUCT far& measureInfo);

    //for horizontal scrollbar
    MsgRouting WmSize(int type, int w, int h);

private:
    BOOL isMultiSelect;
    
    // The width of the longest item in the listbox
    long m_nMaxWidth;
};

#endif
