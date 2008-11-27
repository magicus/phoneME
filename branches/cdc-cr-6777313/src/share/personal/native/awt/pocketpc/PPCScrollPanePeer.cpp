/*
 * @(#)PPCScrollPanePeer.cpp	1.4 03/01/22
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
#include "java_awt_Adjustable.h"
#include "java_awt_ScrollPane.h"
#include "sun_awt_pocketpc_PPCScrollPanePeer.h"
#include "PPCScrollPanePeer.h"


///////////////////////////////////////////////////////////////////////////
// AwtScrollPane class methods

AwtScrollPane::AwtScrollPane() {
}

const TCHAR* AwtScrollPane::GetClassName() {
    return TEXT("SunAwtScrollPane");
}


// Create a new AwtScrollPane object and window.  
AwtScrollPane* AwtScrollPane::Create(jobject self, jobject hParent)
{
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return NULL;
    }    
    CHECK_NULL_RETURN(hParent, "null hParent");
    AwtComponent* parent = PDATA(AwtComponent, hParent);
    CHECK_NULL_RETURN(parent, "null parent");
    jobject target = env->GetObjectField(self, 
                                         WCachedIDs.PPCObjectPeer_targetFID);
    
    CHECK_NULL_RETURN(target, "null target");

    AwtScrollPane* c = new AwtScrollPane();
    CHECK_NULL_RETURN(c, "AwtScrollPane() failed");

    DWORD style = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    if (!IS_WIN4X) {
        // It's been decided by the UI folks that 3.X ScrollPanes should have
        // borders...
        style |= WS_BORDER;
    }
    int scrollbarDisplayPolicy = env->GetIntField (target, 
                   WCachedIDs.java_awt_ScrollPane_scrollbarDisplayPolicyFID);
    if (scrollbarDisplayPolicy == java_awt_ScrollPane_SCROLLBARS_ALWAYS) {
        style |= WS_HSCROLL | WS_VSCROLL;
    }
    DWORD exStyle = IS_WIN4X ? WS_EX_CLIENTEDGE : 0;

    c->CreateHWnd(TEXT(""), style, exStyle,
             env->CallIntMethod(target, WCachedIDs.java_awt_Component_getXMID),
             env->CallIntMethod(target, WCachedIDs.java_awt_Component_getYMID),
             env->CallIntMethod(target,WCachedIDs.java_awt_Component_getWidthMID),
             env->CallIntMethod(target,WCachedIDs.java_awt_Component_getHeightMID),
                  parent->GetHWnd(),
                  (HMENU)parent->CreateControlID(),
                  ::GetSysColor(COLOR_WINDOWTEXT),
                  ::GetSysColor(COLOR_WINDOW),
                  self
        );

    return c;
}


void AwtScrollPane::SetPageIncrement(int value, int orient)
{
    SCROLLINFO * si = new SCROLLINFO;
    si->cbSize = sizeof(SCROLLINFO);
    si->fMask = SIF_PAGE;
    si->nPage = value;
    ::PostMessage(GetHWnd(), WM_AWT_SET_SCROLL_INFO, (WPARAM) orient,
		  (LPARAM) si);
}

void AwtScrollPane::SetInsets()
{
    RECT outside;
    RECT inside;
    ::GetWindowRect(GetHWnd(), &outside); /* is in screen coordinates */
    ::GetClientRect(GetHWnd(), &inside);  /* is in window coordinates */
    /* translate to screen coordinates */
    ::MapWindowPoints(GetHWnd(), 0, (LPPOINT)&inside, 2);

    JNIEnv *env;
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return;
    }
    jobject insets = env->GetObjectField(GetPeer(), 
                                         WCachedIDs.PPCPanelPeer_insets_FID);
    if (insets != NULL) {
	env->SetIntField(insets, WCachedIDs.java_awt_Insets_topFID, 
			 inside.top - outside.top);
	env->SetIntField(insets, WCachedIDs.java_awt_Insets_leftFID, 
			 inside.left - outside.left);
	env->SetIntField(insets, WCachedIDs.java_awt_Insets_bottomFID, 
			 outside.bottom - inside.bottom);
	env->SetIntField(insets, WCachedIDs.java_awt_Insets_rightFID,
			 outside.right - inside.right);
    }
}

void AwtScrollPane::ScrollTo(int x, int y)
{
#ifndef WINCE
    int dx = ::GetScrollPos(GetHWnd(), SB_HORZ) - x;
    int dy = ::GetScrollPos(GetHWnd(), SB_VERT) - y;
    ::SetScrollPos(GetHWnd(), SB_HORZ, x, TRUE);
    ::SetScrollPos(GetHWnd(), SB_VERT, y, TRUE);
    VERIFY(::ScrollWindow(GetHWnd(), dx, dy, NULL, NULL));
    VERIFY(::UpdateWindow(GetHWnd()));
#else
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_POS; 
    ::GetScrollInfo(GetHWnd(), SB_HORZ, &si);
    si.nPos = x;
    ::SetScrollInfo(GetHWnd(), SB_HORZ, &si, FALSE);

    ::GetScrollInfo(GetHWnd(), SB_VERT, &si);
    si.nPos = y;
    ::SetScrollInfo(GetHWnd(), SB_VERT, &si, TRUE); /* redraw = true */
#endif    
}

void AwtScrollPane::SetScrollInfo(int orient, int max, int page, 
                                  BOOL disableNoScroll)
{
    SCROLLINFO  * si = new SCROLLINFO;
    si->cbSize = sizeof(SCROLLINFO);
    si->nMin = 0;
    si->nMax = max;
    si->fMask = SIF_RANGE;
    if (disableNoScroll) {
        si->fMask |= SIF_DISABLENOSCROLL;
    }
    if (page > 0) {
        si->fMask |= SIF_PAGE;
        si->nPage = page;
    }
    ::PostMessage(GetHWnd(), WM_AWT_SET_SCROLL_INFO, (WPARAM) orient,
		  (LPARAM) si);
}

void AwtScrollPane::RecalcSizes(int parentWidth, int parentHeight,
				int childWidth, int childHeight)
{
    // Determine border width without scrollbars.
    int horzBorder;
    int vertBorder;
    if (IS_WIN4X) {
	horzBorder = ::GetSystemMetrics(SM_CXEDGE);
	vertBorder = ::GetSystemMetrics(SM_CYEDGE);
    } else {
        horzBorder = ::GetSystemMetrics(SM_CXBORDER);
        vertBorder = ::GetSystemMetrics(SM_CYBORDER);
    }
    
    parentWidth -= (horzBorder * 2);
    parentHeight -= (vertBorder * 2);

    // Enable each scrollbar as needed.
    JNIEnv *env;
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return;
    }
    jobject target = GetTarget(env);
    int policy = env->GetIntField (target, 
                   WCachedIDs.java_awt_ScrollPane_scrollbarDisplayPolicyFID);
    BOOL needsHorz =  (policy == java_awt_ScrollPane_SCROLLBARS_ALWAYS ||
                       (policy == java_awt_ScrollPane_SCROLLBARS_AS_NEEDED && 
                        childWidth > parentWidth));
    if (needsHorz) {
	parentHeight -= ::GetSystemMetrics(SM_CYHSCROLL);
    }
    BOOL needsVert =  (policy == java_awt_ScrollPane_SCROLLBARS_ALWAYS ||
                       (policy == java_awt_ScrollPane_SCROLLBARS_AS_NEEDED && 
                        childHeight > parentHeight));
    if (needsVert) {
	parentWidth -= ::GetSystemMetrics(SM_CXVSCROLL);
    }
    /*
     * Since the vertical scrollbar may have reduced the parent width
     * enough to now require a horizontal scrollbar, we need to
     * recalculate the horizontal metrics and scrollbar boolean.
     */
    if (!needsHorz) {
	needsHorz = (policy == java_awt_ScrollPane_SCROLLBARS_ALWAYS ||
		     (policy == java_awt_ScrollPane_SCROLLBARS_AS_NEEDED && 
		      childWidth > parentWidth));
	if (needsHorz) {
	    parentHeight -= ::GetSystemMetrics(SM_CYHSCROLL);
	}
    }

    // Now set ranges -- setting the min and max the same disables them.
    if (needsHorz) {
        jobject target = GetTarget(env);
        jobject hAdj = env->CallObjectMethod(target, 
                                 WCachedIDs.java_awt_ScrollPane_gethAdjustableMID);
        CHECK_OBJ(hAdj);
        env->CallVoidMethod(hAdj, 
                            WCachedIDs.java_awt_Adjustable_setBlockIncrementMID,
                            parentWidth);
        SetScrollInfo(SB_HORZ, childWidth - 1, parentWidth, 
                      (policy == java_awt_ScrollPane_SCROLLBARS_ALWAYS));
    } else {
        SetScrollInfo(SB_HORZ, 0, 0, 
                      (policy == java_awt_ScrollPane_SCROLLBARS_ALWAYS));
    }

    if (needsVert) {
        jobject target = GetTarget(env);
        jobject vAdj = env->CallObjectMethod(target, 
                                 WCachedIDs.java_awt_ScrollPane_getvAdjustableMID);
        CHECK_OBJ(vAdj);
        env->CallVoidMethod(vAdj, 
                            WCachedIDs.java_awt_Adjustable_setBlockIncrementMID,
                            parentHeight);
        SetScrollInfo(SB_VERT, childHeight - 1, parentHeight, 
                      (policy == java_awt_ScrollPane_SCROLLBARS_ALWAYS));

    } else {
        SetScrollInfo(SB_VERT, 0, 0, 
                      (policy == java_awt_ScrollPane_SCROLLBARS_ALWAYS));
    }
}

void AwtScrollPane::Reshape(int x, int y, int w, int h)
{
    AwtComponent::Reshape(x, y, w, h);
}

void AwtScrollPane::Show()
{
    SetInsets();
    AwtComponent::Show();
}


int AwtScrollPane::NewScrollPosition(UINT scrollCode, UINT orient, UINT pos,
                                     UINT unitIncrement, UINT blockIncrement)
{
    int newPos, minPos, maxPos, curPos;
#ifndef WINCE
    
    VERIFY(::GetScrollRange(GetHWnd(), orient, &minPos, &maxPos));
    curPos = ::GetScrollPos(GetHWnd(), orient);
#else /* WINCE */
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE | SIF_POS; /* gets min, max and positions */
    GetScrollInfo(GetHWnd(), orient, &si);
    minPos = si.nMin;
    maxPos = si.nMax;
    curPos = si.nPos;
#endif
    // Restrict range so window isn't displayed beyond the child window's
    // maximum size.
    RECT inside;
    ::GetClientRect(GetHWnd(), &inside);
    switch (orient) {
      case SB_HORZ:
          maxPos -= inside.right;  // GetClientRect always returns left as 0
          break;
      case SB_VERT:
          maxPos -= inside.bottom; // ditto for top
          break;
#ifdef DEBUG
      default:
          ASSERT(FALSE);
#endif
    }

    switch (scrollCode) {
      case SB_LINEUP:
          newPos = curPos - unitIncrement;
          return (newPos >= minPos) ? newPos : minPos;
      case SB_LINEDOWN:
          newPos = curPos + unitIncrement;
          return (newPos <= maxPos) ? newPos : maxPos;
      case SB_PAGEUP:
          newPos = curPos - blockIncrement;
          return (newPos >= minPos) ? newPos : minPos;
      case SB_PAGEDOWN:
          newPos = curPos + blockIncrement;
          return (newPos <= maxPos) ? newPos : maxPos;
      case SB_TOP:
          return minPos;
      case SB_BOTTOM:
          return maxPos;
      case SB_THUMBPOSITION:
      case SB_THUMBTRACK:
          return pos;
      default:
          return curPos;
    }
}



MsgRouting AwtScrollPane::WmHScroll(UINT scrollCode, UINT pos, 
                                    HWND hScrollPane) 
{
    JNIEnv* env;
    if (JVM -> AttachCurrentThread((void**) &env, 0) != 0) {
        return mrDoDefault;
    }
    jobject target = GetTarget(env);

    jobject hAdj = env->CallObjectMethod(target, 
                            WCachedIDs.java_awt_ScrollPane_gethAdjustableMID);
    int newPos = NewScrollPosition(scrollCode, SB_HORZ, pos, 
          (int)env->CallIntMethod(hAdj, WCachedIDs.java_awt_Adjustable_getUnitIncrementMID),
          (int)env->CallIntMethod(hAdj, WCachedIDs.java_awt_Adjustable_getBlockIncrementMID));

#ifndef WINCE
    int curPos = ::GetScrollPos(GetHWnd(), SB_HORZ);
    if (newPos == curPos) {
        return mrDoDefault;
    }

    ::SetScrollPos(GetHWnd(), SB_HORZ, newPos, TRUE);
    VERIFY(::ScrollWindow(GetHWnd(), curPos - newPos, 0, NULL, NULL));
    VERIFY(::UpdateWindow(GetHWnd()));
#else /* WINCE */
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_POS;
    ::GetScrollInfo(GetHWnd(), SB_HORZ, &si);
    if (si.nPos == newPos) {
	return mrDoDefault;
    }
    si.nPos = newPos;
    ::SetScrollInfo(GetHWnd(), SB_HORZ, &si, TRUE);
#endif /* WINCE */

    // Notify peer object.
    env->CallVoidMethod(GetPeer(), 
                        WCachedIDs.PPCScrollPanePeer_scrolledHorizontalMID,
                        newPos);
    ASSERT(!env->ExceptionCheck());                  
    return mrConsume;
}

MsgRouting AwtScrollPane::WmVScroll(UINT scrollCode, UINT pos, 
                                    HWND hScrollPane) 
{
    JNIEnv* env;
    if (JVM -> AttachCurrentThread((void**) &env, 0) != 0) {
        return mrDoDefault;
    }
    jobject target = GetTarget(env);
    jobject vAdj = env->CallObjectMethod(target, 
                            WCachedIDs.java_awt_ScrollPane_getvAdjustableMID);

    int newPos = NewScrollPosition(scrollCode, SB_VERT, pos,
        (int)env->CallIntMethod(vAdj, WCachedIDs.java_awt_Adjustable_getUnitIncrementMID),
        (int)env->CallIntMethod(vAdj, WCachedIDs.java_awt_Adjustable_getBlockIncrementMID));

#ifndef WINCE
    int curPos = ::GetScrollPos(GetHWnd(), SB_VERT);
    if (newPos == curPos) {
        return mrDoDefault;
    }

    ::SetScrollPos(GetHWnd(), SB_VERT, newPos, TRUE);
    VERIFY(::ScrollWindow(GetHWnd(), 0, curPos - newPos, NULL, NULL));
    VERIFY(::UpdateWindow(GetHWnd()));
#else
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_POS; 
    ::GetScrollInfo(GetHWnd(), SB_VERT, &si);
    if (si.nPos == newPos) {
	return mrDoDefault;
    }
    si.nPos = newPos;
    ::SetScrollInfo(GetHWnd(), SB_VERT, &si, TRUE); /* redraw = true */
#endif WINCE

    // Notify peer object.
    env->CallVoidMethod(GetPeer(), 
                        WCachedIDs.PPCScrollPanePeer_scrolledVerticalMID,
                        newPos);
    ASSERT(!env->ExceptionCheck());       
    return mrConsume;
}

#ifdef DEBUG
void AwtScrollPane::VerifyState()
{
    if (AwtToolkit::GetInstance().VerifyComponents() == FALSE) {
        return;
    }

    if (m_callbacksEnabled == FALSE) {
        // Component is not fully setup yet.
        return;
    }

    AwtComponent::VerifyState();
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return;
    }    

    jobject target = GetTarget(env);
    jobject hChild = env->CallObjectMethod(GetPeer(),
                             WCachedIDs.PPCScrollPanePeer_getScrollChildMID);
                                           
    ASSERT(!env->ExceptionCheck());
    if (hChild == NULL) {
        return;
    }

    jobject childPeer = env->CallStaticObjectMethod(
                                WCachedIDs.PPCObjectPeerClass,
                                WCachedIDs.PPCObjectPeer_getPeerForTargetMID,
                                hChild);
    CHECK_PEER(childPeer);
    AwtComponent* child = PDATA(AwtComponent, childPeer);
    
    // Verify child window is positioned correctly.
    RECT rect, childRect;
    ::GetClientRect(GetHWnd(), &rect);
    ::MapWindowPoints(GetHWnd(), 0, (LPPOINT)&rect, 2);
    ::GetWindowRect(child->GetHWnd(), &childRect);
    ASSERT(childRect.left <= rect.left && childRect.top <= rect.top);
}
#endif

/**
 * JNI Functions
 */ 
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCScrollPanePeer_initIDs (JNIEnv *env, jclass cls)
{
    GET_METHOD_ID(PPCScrollPanePeer_scrolledHorizontalMID, "scrolledHorizontal", 
                  "(I)V");
    GET_METHOD_ID(PPCScrollPanePeer_scrolledVerticalMID, "scrolledVertical", 
                  "(I)V");
    GET_METHOD_ID(PPCScrollPanePeer_getScrollChildMID, "getScrollChild", 
                  "()Ljava/awt/Component;");

    cls = env->FindClass ("java/awt/ScrollPane");
    if (cls == NULL)
        return;
    
    GET_FIELD_ID (java_awt_ScrollPane_scrollbarDisplayPolicyFID,
                  "scrollbarDisplayPolicy", "I");
    GET_METHOD_ID (java_awt_ScrollPane_gethAdjustableMID,
                  "getHAdjustable", "()Ljava/awt/Adjustable;");
    GET_METHOD_ID (java_awt_ScrollPane_getvAdjustableMID,
                  "getVAdjustable", "()Ljava/awt/Adjustable;");

    cls = env->FindClass( "java/awt/Adjustable" );
    if ( cls == NULL )
        return;

    GET_METHOD_ID( java_awt_Adjustable_setBlockIncrementMID,
                   "setBlockIncrement", "(I)V");
    GET_METHOD_ID( java_awt_Adjustable_getBlockIncrementMID,
                   "getBlockIncrement", "()I");
    GET_METHOD_ID( java_awt_Adjustable_getUnitIncrementMID,
    "getUnitIncrement", "()I");
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCScrollPanePeer_create (JNIEnv *env, jobject self,
                                                jobject hParent)
{
    AWT_TRACE((TEXT("%x: WScrollPanePeer.create(%x)\n"), self, hParent));
    CHECK_PEER(hParent);
    AwtToolkit::CreateComponent(
        self, hParent, (AwtToolkit::ComponentFactory)AwtScrollPane::Create);

    CHECK_PEER_CREATION(self);

    //FIX ((AwtScrollPane*)unhand(self)->pData)->VerifyState();	
}

JNIEXPORT jint JNICALL
Java_sun_awt_pocketpc_PPCScrollPanePeer_getOffset( JNIEnv* env, jobject self,
                                                   jint orient)
{
    AWT_TRACE((TEXT("%x: WScrollPanePeer.getOffset(%d)\n"), self, orient));
    CHECK_PEER_RETURN(self);
    AwtScrollPane* pane = PDATA(AwtScrollPane, self);
    pane->VerifyState();
    int nBar = (orient == java_awt_Adjustable_HORIZONTAL) ? SB_HORZ : SB_VERT;
#ifndef WINCE
    return ::GetScrollPos(pane->GetHWnd(), nBar);
#else
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_POS;
    ::GetScrollInfo(pane->GetHWnd(), nBar, &si);
    return si.nPos;
#endif /* WINCE */
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCScrollPanePeer_setInsets(JNIEnv *env, jobject self)
{
    AWT_TRACE((TEXT("%x: WScrollPanePeer.setInsets()\n"), self));
    CHECK_PEER(self);
    AwtScrollPane* pane = PDATA(AwtScrollPane, self);
    pane->SetInsets();
    pane->VerifyState();
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCScrollPanePeer_setScrollPosition(JNIEnv *env, 
                                                          jobject self,
                                                          jint x, jint y)
{
    AWT_TRACE((TEXT("%x: WScrollPanePeer.setScrollPosition(%d, %d)\n"), self, x, y));
    CHECK_PEER(self);
    AwtScrollPane* pane = PDATA(AwtScrollPane, self);
    pane->ScrollTo(x, y);
    pane->VerifyState();

}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCScrollPanePeer_setSpans(JNIEnv *env, jobject self,
                                                 jint parentWidth, 
                                                 jint parentHeight, 
                                                 jint childWidth, 
                                                 jint childHeight)
{
    AWT_TRACE((TEXT("%x: PPCScrollPanePeer.setSpans(%d, %d, %d, %d)\n"), self, 
	       parentWidth, parentHeight, childWidth, childHeight));
    CHECK_PEER(self);
    AwtScrollPane* pane = PDATA(AwtScrollPane, self);
    pane->RecalcSizes(parentWidth, parentHeight, childWidth, childHeight);
    pane->VerifyState();
}

JNIEXPORT jint JNICALL
Java_sun_awt_pocketpc_PPCScrollPanePeer_getHScrollbarHeightNative(JNIEnv *env,
                                                            jobject self)
{
    AWT_TRACE((TEXT("%x: PPCScrollPanePeer._getHScrollbarHeight()\n"), self));
    return ::GetSystemMetrics(SM_CYHSCROLL);
}

JNIEXPORT jint JNICALL
Java_sun_awt_pocketpc_PPCScrollPanePeer_getVScrollbarWidthNative(JNIEnv *env,
                                                            jobject self)
{
    AWT_TRACE((TEXT("%x: PPCScrollPanePeer._getVScrollbarWidth()\n"), self));
    return ::GetSystemMetrics(SM_CYVSCROLL);
}

// This routine is native to avoid having to grab the Component.LOCK --
// Required by HotJava
JNIEXPORT jobject JNICALL
Java_sun_awt_pocketpc_PPCScrollPanePeer_getScrollChild(JNIEnv *env, jobject self)
{
    AWT_TRACE((TEXT("%x: PPCScrollPanePeer.getScrollChild()\n"), self));
    CHECK_PEER_RETURN(self);
    jobject target = env->GetObjectField(self, WCachedIDs.PPCObjectPeer_targetFID);
    
    jint ncomponents = env->CallIntMethod(target, 
                             WCachedIDs.java_awt_Container_getComponentCountMID);

    if (ncomponents == 0) {
        env->ThrowNew(WCachedIDs.NullPointerExceptionClass, "Child is NULL");
        return NULL;
    }
    jobjectArray components = (jobjectArray)env->CallObjectMethod(target, 
                               WCachedIDs.java_awt_Container_getComponentsMID);
    return env->GetObjectArrayElement(components, 0);
}
