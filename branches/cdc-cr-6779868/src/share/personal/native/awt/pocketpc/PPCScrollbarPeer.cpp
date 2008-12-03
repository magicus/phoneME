/*
 * @(#)PPCScrollbarPeer.cpp	1.9 06/10/10
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
#include "java_awt_Scrollbar.h"
#include "sun_awt_pocketpc_PPCScrollbarPeer.h"
#include "PPCScrollbarPeer.h"
#include "PPCCanvasPeer.h"

////////////////////////////////////////////////////////////////////////////
// AwtScrollbar class methods

AwtScrollbar::AwtScrollbar() {
    m_orientation = SB_HORZ;
    m_lineIncr = 0;
    m_pageIncr = 0;
}

const TCHAR* AwtScrollbar::GetClassName() {
    return TEXT("SCROLLBAR");  // System provided scrollbar class
}

// Create a new AwtScrollbar object and window.  
AwtScrollbar* AwtScrollbar::Create(jobject peer, jobject hParent)
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

    AwtScrollbar* c = new AwtScrollbar();
    CHECK_NULL_RETURN(c, "AwtScrollbar() failed");

    int orient = (int)env->CallIntMethod(target, 
                                 WCachedIDs.java_awt_Scrollbar_getOrientationMID);
    c->m_orientation = (orient == java_awt_Scrollbar_VERTICAL) ? SB_VERT : SB_HORZ;
    c->m_lineIncr = (int)env->CallIntMethod(target, WCachedIDs.java_awt_Scrollbar_getLineIncrementMID);
    c->m_pageIncr = (int)env->CallIntMethod(target, WCachedIDs.java_awt_Scrollbar_getPageIncrementMID);

    DWORD style = WS_CHILD | WS_CLIPSIBLINGS | 
        c->m_orientation;// Note: SB_ and SBS_ are the same here
    c->CreateHWnd(TEXT(""), style, 0,
              (int)env->CallIntMethod(target, WCachedIDs.java_awt_Component_getXMID),
              (int)env->CallIntMethod(target, WCachedIDs.java_awt_Component_getYMID),
              (int)env->CallIntMethod(target,WCachedIDs.java_awt_Component_getWidthMID),
              (int)env->CallIntMethod(target,WCachedIDs.java_awt_Component_getHeightMID),
                  parent->GetHWnd(),
                  (HMENU)parent->CreateControlID(),
                  ::GetSysColor(COLOR_SCROLLBAR),
                  ::GetSysColor(COLOR_SCROLLBAR),
                  peer
                 );
    c->m_backgroundColorSet = TRUE;  // suppress inheriting parent's color.
/*	Not in WINCE */
/*    c->UpdateBackground((Hsun_awt_windows_WComponentPeer*)peer);*/

    c->m_ignoreFocusEvents = FALSE;
    c->m_prevCallback = NULL;
    c->m_prevCallbackPos = 0;

    return c;
}

// 4063897: Work around a windows bug, see KB article Q73839.  Reset
// focus on scrollbars to update focus indicator.  The article advises
// to disable/enable the scrollbar, but simply resetting the focus
// is sufficient.
void AwtScrollbar::UpdateFocusIndicator()
{
    ASSERT(::GetFocus() == GetHWnd());
    m_ignoreFocusEvents = TRUE;
    ::SetFocus(NULL);		// give up focus
    ::SetFocus(GetHWnd());	// force windows to update focus indicator
    m_ignoreFocusEvents = FALSE;
}



LRESULT AwtScrollbar::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    // Delegate real work to super
    LRESULT retValue = AwtComponent::WindowProc(message, wParam, lParam);

    // Post hooks for workarounds
    switch (message) {

      // 4063897: Work around a windows bug.  See KB article Q73839.
      // Need to update focus indicator on scrollbar if thumb
      // proportion or thumb position was changed after processing
      // event.

      case WM_SIZE:
      case SBM_SETSCROLLINFO:
#ifndef WINCE
      case SBM_SETRANGE:
      case SBM_SETRANGEREDRAW:
#endif /* WINCE */
	  if (::GetFocus() == GetHWnd())
	      UpdateFocusIndicator();
          break;


      // Scrollbar internal loop releases mouse capture when user
      // releases the button.  Request a redraw at this point to work
      // around problems with page up/down area not being redrawn
      // correctly after multiple sequential scroll events, in a case
      // when Java listeners take long time to complete.
      // (Alternatively, we can do this in response to WM_[HV]SCROLL
      // with SB_ENDSCROLL code, that is sent right after the capture
      // is released).

      case WM_CAPTURECHANGED:
	  ::InvalidateRect(GetHWnd(), NULL, TRUE);
	  break;
    }

    return retValue;
}

MsgRouting AwtScrollbar::WmKillFocus(HWND hWndGot)
{
    if (m_ignoreFocusEvents) {
	// We are voluntary giving up focus and will get it back
	// immediately.  This is necessary to force windows to update
	// the focus indicator.
	return mrDoDefault;
    }
    else {
	return AwtComponent::WmKillFocus(hWndGot);
    }
}

MsgRouting AwtScrollbar::WmSetFocus(HWND hWndLost)
{
    if (m_ignoreFocusEvents) {
	// We have voluntary gave up focus and are getting it back
	// now.  This is necessary to force windows to update the
	// focus indicator.
	return mrDoDefault;
    }
    else {
	return AwtComponent::WmSetFocus(hWndLost);
    }
}


// Fix for a race condition when the WM_LBUTTONUP is picked by the AWT
// message loop before(!) the windows internal message loop for the
// scrollbar is started in response to WM_LBUTTONDOWN.  See KB article
// Q102552.
//
// The weird effects of this race are described in at least 4028490,
// 4028516, 4060678 (all unjustly closed), 4063897, 4121712 and most
// likely some others.
//
// Since WM_LBUTTONUP is processed by the windows internal message
// loop it's *impossible* to deliver the MOUSE_RELEASED event to Java.
// May be we can synthesize a MOUSE_RELEASED event but that does not 
// seem good so we'd better left this as is for now.  See bug reports
// 4063897 4087087, 4088022.

MsgRouting AwtScrollbar::WmMouseDown(UINT flags, int x, int y, int button)
{
    // We pass the WM_LBUTTONDOWN up to Java, but we process it
    // immediately as well to avoid the race.  Later when this press
    // event returns to us wrapped into a WM_AWT_HANDLE_EVENT we
    // ignore it in the HandleEvent below.

    MsgRouting usualRoute = AwtComponent::WmMouseDown(flags, x, y, button);

    if (button == LEFT_BUTTON)
	// Force immediate processing to avoid the race.  See also
	// HandleEvent below, where we prevent this event from being
	// processed twice.
	return mrDoDefault;
    else
	return usualRoute;
}

MsgRouting AwtScrollbar::HandleEvent(MSG* msg)
{
    if (msg->message == WM_LBUTTONDOWN || msg->message == WM_LBUTTONDBLCLK) {
	// Left button press(es) was already routed to default
	// window procedure in the WmMouseDown above.
	delete msg;
	return mrConsume;
    }
    else {
	return AwtComponent::HandleEvent(msg);
    }
}

void AwtScrollbar::SetValues(int value, int visible, int minimum, int maximum)
{
    // Fix for 4072398: if scrollbar is disabled, disable the update
    // after SetScroll*, to prevent it from silently enabling the peer
    // scrollbar while Java still thinks the scrollbar is disabled.
    BOOL update_p = ::IsWindowEnabled(GetHWnd());

/*    if (IS_WINVER_ATLEAST(3,51)) { */
#ifdef WINCE
	SCROLLINFO si;
	si.cbSize = sizeof si;
	si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
	si.nMin = minimum;
	si.nMax = maximum - 1;
	si.nPage = visible;
	si.nPos = value;
	::SetScrollInfo(GetHWnd(), SB_CTL, &si, update_p);
/*    }
    else {			// Do we still support NT 3.50? */
#else
	// NOTE: Old scrollbars are not proportional, shouldn't we
	// adjust the maximum accordingly by substracting visbile as
	// well?  (This seems like a dead code anyway, though).
	::SetScrollRange(GetHWnd(), SB_CTL, minimum, maximum - 1, FALSE);
	::SetScrollPos(GetHWnd(), SB_CTL, value, update_p);
/*   } */
#endif
}

const char * const AwtScrollbar::SbNdragAbsolute = "dragAbsolute";
const char * const AwtScrollbar::SbNdragBegin    = "dragBegin";
const char * const AwtScrollbar::SbNdragEnd      = "dragEnd";
const char * const AwtScrollbar::SbNlineDown     = "lineDown";
const char * const AwtScrollbar::SbNlineUp       = "lineUp";
const char * const AwtScrollbar::SbNpageDown     = "pageDown";
const char * const AwtScrollbar::SbNpageUp       = "pageUp";

inline void
AwtScrollbar::DoScrollCallbackCoalesce(const char* methodName, int newPos)
{
    if (!(methodName == m_prevCallback && newPos == m_prevCallbackPos)) {
        JNIEnv *env;
        if (JVM -> AttachCurrentThread((void**) &env, 0) != 0) {
                return; //mrPassAlong; 
        }
        jclass cls = env->FindClass("sun/awt/pocketpc/PPCScrollbarPeer");
        jmethodID mid  = env->GetMethodID(cls, methodName,  "(I)V" );
        env->CallVoidMethod(m_peerObject, mid, newPos);
	m_prevCallback = methodName;
	m_prevCallbackPos = newPos;
    }
}

MsgRouting AwtScrollbar::WmHScroll(UINT scrollCode, UINT pos, HWND hScrollbar)
{ 
    int minVal, maxVal;    // scrollbar range
    int minPos, maxPos;    // thumb positions (max depends on visible amount)
    int curPos, newPos;

    // For drags we have old (static) and new (dynamic) thumb positions
    int dragP = scrollCode == SB_THUMBPOSITION || scrollCode == SB_THUMBTRACK;
    int thumbPos;

/*    if (IS_WINVER_ATLEAST(3,51)) { // New scrollbar API */
#ifdef WINCE
	SCROLLINFO si;
	si.cbSize = sizeof si;
	si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;

	// 4027759, 4078338, 4094993: ignore 16 bit `pos' parameter.
	// From, _Win32 Programming_, by Rector and Newcommer, p. 185:
	// "In some of the older documentation on Win32 scroll bars,
	// including that published by Microsoft, you may read that
	// you *cannot* obtain the scroll position while in a handler.
	// The SIF_TRACKPOS flag was added after this documentation
	// was published.  Beware of this older documentation; it may
	// have other obsolete features."
	if (dragP)
	  si.fMask |= SIF_TRACKPOS;

	VERIFY(::GetScrollInfo(GetHWnd(), SB_CTL, &si));
	curPos = si.nPos;
	minPos = minVal = si.nMin;

	// Upper bound of the range.  Note that adding 1 here is safe
	// and won't cause a wrap, since we have substracted 1 in the
	// SetValues above.
	maxVal = si.nMax + 1;

	// 4034521: Maximum position that can be usefully set is
	// (maximum - visible).  We must take this into account or
	// false lineDown, pageDown will be fired at the bottom of the
	// scrollbar.
	maxPos = maxVal - si.nPage;

	// Documentation for SBM_SETRANGE says that scrollbar range is
	// limited by MAXLONG, which is 2**31.  When a scroll range is
	// greater than that - thumbPos is reported incorrectly due to
	// integer arithmetic wrap(s).
	if (dragP)
	    thumbPos = si.nTrackPos;
/*    else {			// Do we still support NT 3.50? */
#else
	VERIFY(::GetScrollRange(GetHWnd(), SB_CTL, &minVal, &maxVal));
	curPos = ::GetScrollPos(GetHWnd(), SB_CTL);
	minPos = minVal;
	maxPos = maxVal;	// NOTE: see the comment in SetValues above

	// 4027759, 4078338, 4094993:
	// Try to work around 16 bit limit of the `pos'
	if (dragP) {
	    if (minVal >= 0 && maxVal <= 0xffff) {
		// If the range is within [0,65535] - pos is correct
		thumbPos = pos;
	    }
	    else if (minVal + 0xffff < maxVal) {
		// If the range does not fit in 16 bit we loose...

		// One possible solution is to keep track of SB_THUMBTRACK
		// events and note when the `pos' wraps since track events
		// are generated contiguously.  But windows defeats this:
		// if we're dragging the thumb and mouse goes away from
		// the window while the button is still pressed, the thumb
		// is warped back to its original position, when the mouse
		// (button is still pressed) enters the window again the
		// thumb is warped to this new position.

		thumbPos = pos;	// NOTE: wrong - but can't do anything about it
	    }
	    else {
		// The range fits into 16 bit so we can unambiguously
		// calculate the real thumb position.
		thumbPos = minPos + LOWORD(pos - LOWORD(minPos));
	    }
	}
#endif /*WINCE */    

    switch (scrollCode) {
      case SB_LINEUP:
	  if (minPos + m_lineIncr >= curPos) {
	      newPos = minPos;
	  }
	  else {
	      newPos = curPos - m_lineIncr;
	  }
          if (newPos != curPos) {
              DoScrollCallbackCoalesce(SbNlineUp, newPos);
	  }
          break;
      case SB_LINEDOWN:
	  if (maxPos - m_lineIncr <= curPos) {
	      newPos = maxPos;
	  }
	  else {
	      newPos = curPos + m_lineIncr;
	  }
          if (newPos != curPos) {
              DoScrollCallbackCoalesce(SbNlineDown, newPos);
          }
          break;
      case SB_PAGEUP:
	  if (minPos + m_pageIncr >= curPos) {
	      newPos = minPos;
	  }
	  else {
	      newPos = curPos - m_pageIncr;
	  }
          if (newPos != curPos) {
              DoScrollCallbackCoalesce(SbNpageUp, newPos);
          }
          break;
      case SB_PAGEDOWN:
	  if (maxPos - m_pageIncr <= curPos) {
	      newPos = maxPos;
	  }
	  else {
	      newPos = curPos + m_pageIncr;
	  }
          if (newPos != curPos) {
              DoScrollCallbackCoalesce(SbNpageDown, newPos);
          }
          break;
      case SB_THUMBPOSITION:
	  // In ideal world we should use different callbacks for
	  // SB_THUMBTRACK and SB_THUMBPOSITION.  Application usually
	  // processes SB_THUMBTRACK (real time scrolling) if it can
	  // scroll fast.  If redraw required for scrolling is
	  // expensive, application will process SB_THUMBPOSITION.
	  // But java program cannot control this choice via AWT API.

	  // Since we do process SB_THUMBTRACK events and since the
	  // position reported in the SB_THUMBPOSITION events is the
	  // same as in the last SB_THUMBTRACK event, the guard
	  // expression below will *always* be false and dragAbsolute
	  // shall never be called.  The code below is left here just
	  // in case.

	  if (thumbPos != curPos) {
	      DoScrollCallbackCoalesce(SbNdragAbsolute, thumbPos);
	  }
          break;
      case SB_THUMBTRACK:
	  if (thumbPos != curPos) {
	      DoScrollCallbackCoalesce(SbNdragAbsolute, thumbPos);
	  }
          break;
      case SB_TOP:
	  if (curPos != minPos) {
	      DoScrollCallbackCoalesce(SbNdragAbsolute, minPos);
	  }
          break;
      case SB_BOTTOM:
	  if (curPos != maxPos) {
	      DoScrollCallbackCoalesce(SbNdragAbsolute, maxPos);
	  }
          break;
      case SB_ENDSCROLL:
          //FIX DoCallback(SbNdragEnd, "(I)V", curPos);
	  m_prevCallback = NULL;
          break;
      case 9 /*SB_BEGINSCROLL*/:
          //FIX DoCallback(SbNdragBegin, "(I)V", curPos);
	  m_prevCallback = NULL;
          break;
    }
    return mrDoDefault;
}

MsgRouting AwtScrollbar::WmVScroll(UINT scrollCode, UINT pos, HWND hScrollbar) {
    return WmHScroll(scrollCode, pos, hScrollbar);
}


/**
 * JNI Functions
 */
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCScrollbarPeer_initIDs (JNIEnv *env, jclass cls)
{
    cls = env->FindClass ("java/awt/Scrollbar");
    if (cls == NULL)
        return;
    GET_METHOD_ID (java_awt_Scrollbar_getLineIncrementMID,
                  "getLineIncrement", "()I");
    GET_METHOD_ID (java_awt_Scrollbar_getPageIncrementMID,
                   "getPageIncrement", "()I");
    GET_METHOD_ID (java_awt_Scrollbar_getOrientationMID,
                   "getOrientation", "()I");
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCScrollbarPeer_setValuesNative (JNIEnv *env, 
                                                        jobject self,
                                                        jint value, jint visible,
                                                        jint minimum, jint maximum)
{
    CHECK_PEER(self);
    AwtScrollbar* c = PDATA(AwtScrollbar, self);
    c->SetValues(value, visible, minimum, maximum);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCScrollbarPeer_setLineIncrement (JNIEnv *env, 
                                                         jobject self,
                                                         jint increment)
{
    CHECK_PEER(self);
    AwtScrollbar* c = PDATA(AwtScrollbar, self);
    c->SetLineIncrement((int)increment);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCScrollbarPeer_setPageIncrement (JNIEnv *env, jobject self,
                                                         jint increment)
{
    CHECK_PEER(self);
    AwtScrollbar* c = PDATA(AwtScrollbar, self);
    c->SetPageIncrement((int)increment);
}


JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCScrollbarPeer_create (JNIEnv *env, jobject thisobj, 
                                               jobject parent)
{
    CHECK_PEER(parent);
    AwtToolkit::CreateComponent(
            thisobj, parent, (AwtToolkit::ComponentFactory)AwtScrollbar::Create);
    CHECK_PEER_CREATION(thisobj);
}
