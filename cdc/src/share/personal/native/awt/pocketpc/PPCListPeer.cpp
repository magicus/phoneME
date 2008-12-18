/*
 * @(#)PPCListPeer.cpp	1.14 06/10/10
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
#include "PPCListPeer.h"
#include "PPCCanvasPeer.h"
#include "sun_awt_pocketpc_PPCListPeer.h"


//Netscape : Bug # 82754
#define LISTBOX_EX_STYLE	IS_WIN4X ? WS_EX_CLIENTEDGE : 0

/**
 * Contructor for PPCListPeer
 */
AwtList :: AwtList() 
{
    isMultiSelect = FALSE;
    m_nMaxWidth = 0;
}

const TCHAR* AwtList::GetClassName() {
    return TEXT("LISTBOX");  // System provided listbox class
}


// Create a new AwtList object and window.  
AwtList* AwtList::Create(jobject peer, jobject hParent)
{
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return NULL;
    }
    CHECK_NULL_RETURN(hParent, "null hParent");
    AwtCanvas *parent = PDATA( AwtCanvas, hParent );
    
    CHECK_NULL_RETURN(parent, "null parent");
    jobject target = env->GetObjectField(peer, 
                                         WCachedIDs.PPCObjectPeer_targetFID);

    AwtList* c = new AwtList(); 
    CHECK_NULL_RETURN(c, "AwtList() failed");

#ifndef WINCE
    long style = WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL | WS_HSCROLL |
      LBS_NOINTEGRALHEIGHT | LBS_NOTIFY | WS_BORDER;
#else // WINCE 
    long style = WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL | WS_HSCROLL |
                 LBS_NOINTEGRALHEIGHT | LBS_NOTIFY
                  | (IS_WIN4X ? 0 : WS_BORDER);

#endif // WINCE 
    //Netscape : Bug # 82754
    //long exStyle = IS_WIN4X ? WS_EX_CLIENTEDGE : 0;
    long exStyle = LISTBOX_EX_STYLE;

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

//Netscape : Override the AwtComponent method so we can set the item height
//for each item in the list.

void AwtList::SetFont(AwtFont* font)
{
    ASSERT(font != NULL);
    if (font->GetAscent() < 0)
    {
    	AwtFont::SetupAscent(font);
    }
    HANDLE hFont = font->GetHFont();
    SendMessage(WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));

    // get the shared DC and lock it
    AwtSharedDC *pDC = GetSharedDC();
    if (pDC == NULL) 
        return;
    pDC->Lock();
    pDC->GrabDC(NULL);
    HDC hDC = pDC->GetDC();

    TEXTMETRIC tm;
    VERIFY(::SelectObject(hDC, hFont) != NULL);
    VERIFY(::GetTextMetrics(hDC, &tm));

    // Free up the shared DC
    pDC->Unlock();
    long h = tm.tmHeight + tm.tmExternalLeading;

    int nCount = (int)::SendMessage(GetHWnd(), LB_GETCOUNT, 0, 0);
    for(int i = 0; i < nCount; ++i)
    {
    	VERIFY(::SendMessage(GetHWnd(), LB_SETITEMHEIGHT, i, MAKELPARAM(h, 0)) != LB_ERR);
    }
#ifndef WINCE
    VERIFY(::RedrawWindow(GetHWnd(), NULL, NULL, RDW_INVALIDATE |RDW_FRAME |RDW_ERASE));
#else // WINCE 
    // Wince dows not have RedrawWindow 
    ::UpdateWindow(GetHWnd());
#endif // WINCE 
    
}

void AwtList::SetMultiSelect(BOOL ms) {
    if (ms == isMultiSelect) {
        return;
    }
    isMultiSelect = ms;
    
    // Copy current box's contents to string array
    int nCount = GetCount();
    TCHAR** strings = new TCHAR*[nCount];
    for(int i = 0; i < nCount; i++) {
        int len = SendMessage(LB_GETTEXTLEN, i);
        TCHAR* text = new TCHAR[len + 1];
        VERIFY(SendMessage(LB_GETTEXT, i, (LPARAM)text) != LB_ERR);
        strings[i] = text;
    }
    int nCurSel = GetCurrentSelection();

    // Save old list box's attributes
    RECT rect;
    GetWindowRect(GetHWnd(), &rect);
    AwtComponent* pParent = GetParent();
    MapWindowPoints(0, pParent->GetHWnd(), (LPPOINT)&rect, 2);

    HANDLE hFont = (HANDLE)SendMessage(WM_GETFONT);
    DWORD style = GetStyle() | WS_VSCROLL;
    if (isMultiSelect) {
        style |= LBS_MULTIPLESEL;
    } else {    
        style &= ~LBS_MULTIPLESEL;
    } 
	
	//Netscape : Bug # 82754. Windows adds an undocumented bit in the
	//extended style when queried from the listbox.  Passing the 
	//undocumented bit to CreateWindowEx works OK in NT4.0 but not in NT 3.51
	//Since noone can change the extended style after the listbox is 
	//created, we can be assured that the style we want is the same as the
	//original so lets use that instead of querying the listbox's style.
    //DWORD exStyle = ::GetWindowLong(GetHWnd(), GWL_EXSTYLE);
    DWORD exStyle= LISTBOX_EX_STYLE;

    // Recreate list box
    jobject peer = GetPeer();
    DestroyHWnd();
    CreateHWnd(TEXT(""), style, exStyle, 
               rect.left, rect.top, 
               rect.right-rect.left, rect.bottom-rect.top, 
               pParent->GetHWnd(),
               (HMENU)m_myControlID,
               ::GetSysColor(COLOR_WINDOWTEXT),
               ::GetSysColor(COLOR_WINDOW),
               peer
        );

    SendMessage(WM_SETFONT, (WPARAM)hFont, (LPARAM)FALSE);
    SendMessage(LB_RESETCONTENT);  
    for (int j = 0; j < nCount; j++) {
        InsertString(j, strings[j]);
        delete strings[j];
    }
    AdjustHorizontalScrollbar();
    SendMessage(LB_SETCURSEL, nCurSel);
    delete[] strings; 
}

jobject AwtList::PreferredItemSize(JNIEnv* env)
{
    jobject dimension; 
    jobject peer = GetPeer();
    dimension = env->CallObjectMethod(peer, WCachedIDs.PPCListPeer_preferredSizeMID, 1);
    ASSERT(!env->ExceptionCheck());
    if (dimension == NULL)
        return NULL;
    //This size is too big for each item height.
    env->SetIntField(dimension, WCachedIDs.java_awt_Dimension_heightFID,
                     GetFontHeight());
    return dimension;
}


// Every time something gets added to the list, we increase the max width 
// of items that have ever been added.  If it surpasses the width of the 
// listbox, we show the scrollbar.  When things get deleted, we shrink 
// the scroll region back down and hide the scrollbar, if needed.
void AwtList::AdjustHorizontalScrollbar()
{
    // The border width is added to the horizontal extent to ensure that we
    // can view all of the text when we move the horz. scrollbar to the end.
    int  cxBorders = GetSystemMetrics( SM_CXBORDER ) * 2;
    RECT rect;
    VERIFY(::GetClientRect(GetHWnd(), &rect));
    int iHorzExt = SendMessage(LB_GETHORIZONTALEXTENT, 0, 0L ) - cxBorders;
    if ( (m_nMaxWidth > rect.left)  // if strings wider than listbox
      || (iHorzExt != m_nMaxWidth) ) //   or scrollbar not needed anymore.
    {
        SendMessage(LB_SETHORIZONTALEXTENT, m_nMaxWidth + cxBorders, 0L);
    }
}


// This function goes through all strings in the list to find the width, 
// in pixels, of the longest string in the list.
void AwtList::UpdateMaxItemWidth()
{
    m_nMaxWidth = 0;
    // get the shared DC and lock it
    AwtSharedDC * pDC = GetSharedDC();
    if ( pDC == NULL )
        return;
    pDC->Lock();
    pDC->GrabDC(NULL);
    HDC hDC = pDC->GetDC();

    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
        return;
    }
    jobject pPeer = GetPeer();
    jobject pTarget = NULL;
    if ( pPeer )
        pTarget = env->GetObjectField(pPeer, 
                                      WCachedIDs.PPCObjectPeer_targetFID);
    if ( pTarget )
    {
        jobject hJavaFont = GET_FONT( pTarget );
        int nCount = GetCount();
        for ( int i=0; i < nCount; i++ )
        {
            jstring hStr;
            hStr = GetItemString( pTarget, i );
            SIZE size = AwtFont::getMFStringSize( hDC, hJavaFont, hStr );
            if ( size.cx > m_nMaxWidth )
                m_nMaxWidth = size.cx;
        }
    }
    
    // free up the shared DC
    pDC->Unlock();
    // Now adjust the horizontal scrollbar extent
    AdjustHorizontalScrollbar();
}

MsgRouting AwtList::WmNotify(UINT notifyCode)
{
    //!CQ why not call SendEvent() to get event over to java???
    //!CQ need to get ITEM_CHANGED, ITEMS_SELECTED, ITEMS_DESELECTED over also
    JNIEnv *env;
    if (JVM -> AttachCurrentThread((void**) &env, 0) != 0) {
        return mrPassAlong; 
       }
    if (notifyCode == LBN_SELCHANGE) {
        env->CallVoidMethod(m_peerObject, 
                            WCachedIDs.PPCListPeer_handleListChangedMID, 
                            GetCurrentSelection());
    }
    else if (notifyCode == LBN_DBLCLK) {
        env->CallVoidMethod(m_peerObject, 
                            WCachedIDs.PPCListPeer_handleActionMID, 
                            GetCurrentSelection());
    }
    return mrDoDefault;
}

MsgRouting
AwtList::WmSize(int type, int w, int h)
{
    AdjustHorizontalScrollbar();
    return AwtComponent::WmSize(type, w, h);
}

MsgRouting
AwtList::WmDrawItem(UINT /*ctrlId*/, DRAWITEMSTRUCT far& drawInfo)
{
    // You get a -1 as the itemID if the listbox is empty -- this allows 
    // you to draw the focus rectangle if you want (we don't).  As it was, 
    // this was raising an ArrayIndexOutOfBoundsException
    // down the road when it tried to getItem on item index -1.
    if (drawInfo.itemID != -1) {
        AwtComponent::DrawListItem(drawInfo);
    }
    return mrConsume;
}

MsgRouting 
AwtList::WmMeasureItem(UINT /*ctrlId*/, MEASUREITEMSTRUCT far& measureInfo)
{
    AwtComponent::MeasureListItem(measureInfo);
    return mrConsume;
}


/**
 * JNI Functions
 */ 
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCListPeer_initIDs (JNIEnv *env, jclass cls)
{
    GET_METHOD_ID(PPCListPeer_preferredSizeMID, "preferredSize", 
                  "(I)Ljava/awt/Dimension;");
    GET_METHOD_ID(PPCListPeer_handleActionMID, "handleAction", "(I)V");
    GET_METHOD_ID(PPCListPeer_handleListChangedMID, "handleListChanged", 
                  "(I)V");
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCListPeer_create (JNIEnv *env, jobject thisObj, 
                                          jobject hParent)
{
    CHECK_PEER(hParent);
    AwtToolkit::CreateComponent(thisObj, hParent, 
                               (AwtToolkit::ComponentFactory)AwtList::Create);
    CHECK_PEER_CREATION(thisObj);
}

JNIEXPORT jint JNICALL
Java_sun_awt_pocketpc_PPCListPeer_getMaxWidth (JNIEnv *env, 
                                                jobject thisObj)
{
    CHECK_PEER_RETURN(thisObj);
    AwtList* l = PDATA(AwtList, thisObj); 
    return l->GetMaxWidth();

}


JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCListPeer_updateMaxItemWidth (JNIEnv *env, 
                                                      jobject thisObj)
{
    CHECK_PEER(thisObj);
    AwtList* l = PDATA(AwtList, thisObj); 
    l->UpdateMaxItemWidth();

}


JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCListPeer_addItemNative (JNIEnv *env, jobject thisObj,
                                                 jstring itemText, jint index,
                                                 jint width)
{
    CHECK_PEER(thisObj);
    // check for null string
    CHECK_NULL(itemText, "null list item");
    AwtList* l = PDATA(AwtList, thisObj);
    l->InsertString(index, JavaStringBuffer(env, itemText)); 
    l->CheckMaxWidth(width);  
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCListPeer_delItems (JNIEnv *env, jobject thisObj,
                                              jint start, jint end)
{
    CHECK_PEER(thisObj);

    AwtList* l = PDATA(AwtList, thisObj);
    int count = l->GetCount();

    if (start == 0 && end >= count-1) {
        l->SendMessage(LB_RESETCONTENT);
    }
    else for (int i = start; i <= end; i++) {
        l->SendMessage(LB_DELETESTRING, start);
    }

    l->UpdateMaxItemWidth();
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCListPeer_select (JNIEnv *env, jobject thisObj, 
                                            jint index)
{
    CHECK_PEER(thisObj);
    AwtList* l = PDATA(AwtList, thisObj);
    l->Select(index);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCListPeer_deselect (JNIEnv *env, jobject thisObj,
                                            jint index)
{
    CHECK_PEER(thisObj);
    AwtList* l = PDATA(AwtList, thisObj);
    l->Deselect(index);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCListPeer_makeVisible (JNIEnv *env, jobject thisObj,
                                                 jint index)
{
    AwtList* l = PDATA(AwtList, thisObj);
    l->SendMessage(LB_SETTOPINDEX, index);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCListPeer_setMultipleSelections(JNIEnv *env, 
                                                        jobject thisObj,
                                                        jboolean multipleMode)
{
    CHECK_PEER(thisObj);
    AwtList* l = PDATA(AwtList, thisObj);
    ::SendMessage(AwtToolkit::GetInstance().GetHWnd(), 
                  WM_AWT_LIST_SETMULTISELECT, (WPARAM)l, multipleMode);
}

JNIEXPORT jboolean JNICALL
Java_sun_awt_pocketpc_PPCListPeer_isSelected(JNIEnv *env, 
                                             jobject thisObj,
                                             jint index)
{
    CHECK_PEER_RETURN(thisObj);
    AwtList* l = PDATA(AwtList, thisObj);
    return l->IsItemSelected(index);
}
