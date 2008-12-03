/*
 * @(#)PPCWindowPeer.cpp	1.6 06/10/10
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
#include "sun_awt_pocketpc_PPCWindowPeer.h"
#include "java_awt_event_WindowEvent.h"
#include "PPCToolkit.h"
#include "PPCWindowPeer.h"
#include "PPCGraphics.h"

#include "java_awt_Insets.h"
#include "java_awt_Container.h"
#include "java_awt_event_ComponentEvent.h"

#ifdef WINCE
#include <commctrl.h>
#include <wceCompat.h>
#endif /* WINCE */


///////////////////////////////////////////////////////////////////////////
// AwtWindow class methods

AwtWindow::AwtWindow() {
    m_iconic = FALSE;
    m_eraseBackground = FALSE;
    m_resizing = FALSE;
    m_cmdBar = NULL;
    m_sizePt.x = m_sizePt.y = 0;
    VERIFY(::SetRectEmpty(&m_insets));
    VERIFY(::SetRectEmpty(&m_old_insets));
    VERIFY(::SetRectEmpty(&m_warningRect));
#ifndef WINCE // Pocket PC 2002 doesn't seem to have this    
    SetCursor(::LoadCursor(NULL, IDC_ARROW));
#endif
}

AwtWindow::~AwtWindow() {
  AwtToolkit::UnregisterTopLevelWindow(GetHWnd());
}

const TCHAR* AwtWindow::GetClassName() {
  return TEXT("SunAwtWindow");
}

// Create a new AwtWindow object and window.  
AwtWindow* AwtWindow::Create(jobject self,
                             jobject hParent)
{
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return NULL;
    }

    CHECK_NULL_RETURN(hParent, "null hParent");
    AwtWindow* parent = PDATA(AwtWindow, hParent);
    CHECK_NULL_RETURN(parent, "null parent");
    jobject target = env->GetObjectField(self, WCachedIDs.PPCObjectPeer_targetFID);
    CHECK_NULL_RETURN(target, "null target");

    AwtWindow* window = new AwtWindow();
    CHECK_NULL_RETURN(window, "AwtWindow() failed");

    DWORD style = WS_CLIPCHILDREN | WS_POPUP;
    window->CreateHWnd(TEXT(""),
                       style, 0,
                       0, 0, 0, 0,
                       parent->GetHWnd(),
                       NULL,
                       ::GetSysColor(COLOR_WINDOWTEXT),
                       ::GetSysColor(COLOR_WINDOW),
                       self
                      );

    //  here instead of during create, so that a WM_NCCALCSIZE is sent.
    window->Reshape(env->CallIntMethod(target, 
				       WCachedIDs.java_awt_Component_getXMID),
                    env->CallIntMethod(target,
				       WCachedIDs.java_awt_Component_getYMID),
		    env->CallIntMethod(target, 
				       WCachedIDs.java_awt_Component_getWidthMID),
		    env->CallIntMethod(target, 
				       WCachedIDs.java_awt_Component_getHeightMID));

    /* also done for awtFrame and awtDialog */
    AwtToolkit::RegisterTopLevelWindow(window->GetHWnd());
    return window;
}

//
// Override AwtComponent's Reshape to handle minimized/maximized
// windows, fixes 4065534 (robi.khan@eng).
// NOTE: See also AwtWindow::WmSize
//
void AwtWindow::Reshape(int x, int y, int width, int height) 
{
    HWND    hWndSelf = GetHWnd();
#ifndef WINCE
    if ( IsIconic(hWndSelf)) {
    // normal AwtComponent::Reshape will not work for iconified windows so...
	WINDOWPLACEMENT wp;
	POINT	    ptMinPosition = {x,y};
	POINT	    ptMaxPosition = {0,0};
	RECT	    rcNormalPosition = {x,y,x+width,y+height};
	RECT	    rcWorkspace;
	HWND	    hWndDesktop = GetDesktopWindow();

	// SetWindowPlacement takes workspace coordinates, but
	// if taskbar is at top of screen, workspace coords !=
	// screen coords, so offset by workspace origin
	VERIFY(::SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&rcWorkspace, 0));
	::OffsetRect(&rcNormalPosition, -rcWorkspace.left, -rcWorkspace.top);

	// set the window size for when it is not-iconified
	wp.length = sizeof(wp);
	wp.flags = WPF_SETMINPOSITION;
	wp.showCmd = SW_SHOWNA;
	wp.ptMinPosition = ptMinPosition;
	wp.ptMaxPosition = ptMaxPosition;
	wp.rcNormalPosition = rcNormalPosition;
	SetWindowPlacement(hWndSelf, &wp);
	return;
    }
    
    if (IsZoomed(hWndSelf)) {
    // setting size of maximized window, we remove the
    // maximized state bit (matches Motif behaviour)
    // (calling ShowWindow(SW_RESTORE) would fire an
    //  activation event which we don't want)
	LONG	style = GetStyle();
	ASSERT(style & WS_MAXIMIZE);
	style ^= WS_MAXIMIZE;
	SetStyle(style);
    }
#endif /* !WINCE */
    AwtComponent::Reshape(x, y, width, height);
}

//
// Get and return the insets for this window (container, really).
// Calculate & cache them while we're at it, for use by AwtGraphics
// return if insets have changed
//
BOOL AwtWindow::UpdateInsets(jobject insets)
{
    JNIEnv *env;

    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return FALSE;
    }

    ASSERT(GetPeer() != NULL);
    jobject target = GetTarget();
    CHECK_NULL_RETURN(target, "null target");

    // fix 4167248 : don't update insets when frame is iconified
    // to avoid bizarre window/client rectangles
    if (m_iconic)
	return FALSE;

    // Code to calculate insets. Stores results in frame's data
    // members, and in the peer's Inset object.
    RECT outside;
    RECT inside;
    ::GetWindowRect(GetHWnd(), &outside);
    ::GetClientRect(GetHWnd(), &inside);
#ifdef WINCE
    /* In WINCE, the command bar is included in the Client rect */
    /* In WINCE, nobody can hear you scream */
    if (env->IsInstanceOf(GetTarget(), WCachedIDs.java_awt_FrameClass)
	&& m_cmdBar) {
        int h = wceGetCommandBarHeight(m_cmdBar);
	inside.top += h;
    }
#endif
    // Update our inset member
    if (outside.right - outside.left > 0 && outside.bottom - outside.top > 0) { 
        ::MapWindowPoints(GetHWnd(), 0, (LPPOINT)&inside, 2);
        m_insets.top = inside.top - outside.top;
        m_insets.bottom = outside.bottom - inside.bottom;
        m_insets.left = inside.left - outside.left;
        m_insets.right = outside.right - inside.right;
    } else {
        // This window hasn't been sized yet -- use system metrics.
	if (env->IsInstanceOf(GetTarget(), WCachedIDs.java_awt_FrameClass) ||
	    env->IsInstanceOf(GetTarget(), WCachedIDs.java_awt_DialogClass)) {
	    // Get outer frame sizes.
#ifndef WINCE
	    long style = GetStyle();
	    if (style & WS_THICKFRAME) {
		m_insets.left = m_insets.right = 
		    ::GetSystemMetrics(SM_CXSIZEFRAME);
		m_insets.top = m_insets.bottom = 
		    ::GetSystemMetrics(SM_CYSIZEFRAME);
	    } else {
		m_insets.left = m_insets.right = 
		    ::GetSystemMetrics(SM_CXDLGFRAME);
		m_insets.top = m_insets.bottom = 
		    ::GetSystemMetrics(SM_CYDLGFRAME);
	    }
#else /* WINCE */
	    m_insets.left = m_insets.right = 
		::GetSystemMetrics(SM_CXDLGFRAME);
	    m_insets.top = m_insets.bottom = 
		::GetSystemMetrics(SM_CYDLGFRAME);
#endif /* WINCE */
	    
	    // Add in title.
	    m_insets.top += ::GetSystemMetrics(SM_CYCAPTION);
	    
	    // Add in warning string, if needed
	    if (env->CallObjectMethod(target, WCachedIDs.java_awt_Window_getWarningStringMID) != NULL) {
		m_insets.bottom += ::GetSystemMetrics(SM_CYCAPTION);
	    }
	    
	    // Add in menuBar, if any.
#ifndef WINCE
	    if (env->IsInstanceOf(GetTarget(), WCachedIDs.java_awt_FrameClass) &&
		((AwtFrame*)this)->GetMenuBar()) {
		m_insets.top += ::GetSystemMetrics(SM_CYCAPTION);
	    }

#else /* WINCE */
	    if (env->IsInstanceOf(GetTarget(),
				  WCachedIDs.java_awt_FrameClass) && 
		m_cmdBar) {
		int h = wceGetCommandBarHeight(m_cmdBar);
		m_insets.top +=  h;
	    }
#endif /* WINCE */

	}
    }
    
    // Get insets into our peer directly
    jobject peerInsets = env->GetObjectField(GetPeer(), WCachedIDs.PPCPanelPeer_insets_FID);
    if (peerInsets != NULL) { // may have been called during creation
        env->SetIntField(peerInsets,
			 WCachedIDs.java_awt_Insets_topFID, m_insets.top);
        env->SetIntField(peerInsets,
			 WCachedIDs.java_awt_Insets_bottomFID, m_insets.bottom);
        env->SetIntField(peerInsets,
			 WCachedIDs.java_awt_Insets_leftFID, m_insets.left);
        env->SetIntField(peerInsets,
			 WCachedIDs.java_awt_Insets_rightFID, m_insets.right);
    }
    
    // Get insets into the Inset object (if any) that was passed
    if (insets != NULL) {
	env->SetIntField(insets, 
			 WCachedIDs.java_awt_Insets_topFID,
			 m_insets.top);
	env->SetIntField(insets,
			 WCachedIDs.java_awt_Insets_bottomFID,
			 m_insets.bottom);
	env->SetIntField(insets,
			 WCachedIDs.java_awt_Insets_leftFID,
			 m_insets.left);
	env->SetIntField(insets,
			 WCachedIDs.java_awt_Insets_rightFID,
			 m_insets.right);
    }

    BOOL	insetsChanged = !::EqualRect( &m_old_insets, &m_insets );	
    ::CopyRect( &m_old_insets, &m_insets );
    return insetsChanged;
}

/*
 * Although this function sends ComponentEvents, it needs to be defined
 * here because only top-level windows need to have move and resize
 * events fired from native code.  All contained windows have these events
 * fired from common Java code.
 */
void AwtWindow::SendComponentEvent(jint eventId)
{
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return;
    }
    
    jobject hEvent =
	env->NewObject(WCachedIDs.java_awt_event_ComponentEventClass,
		       WCachedIDs.java_awt_event_ComponentEvent_constructorMID,
		       GetTarget(), 
		       eventId);
    ASSERT(!env->ExceptionCheck());
    ASSERT(hEvent != NULL);
    SendEvent(hEvent);
}

void AwtWindow::SendWindowEvent(jint eventId)
{
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return;
    }
    
    jobject hEvent =
	env->NewObject(WCachedIDs.java_awt_event_WindowEventClass,
		       WCachedIDs.java_awt_event_WindowEvent_constructorMID,
		       GetTarget(), 
		       eventId);
    ASSERT(!env->ExceptionCheck());
    ASSERT(hEvent != NULL);
    SendEvent(hEvent);
}

MsgRouting AwtWindow::WmCreate()
{
    return mrDoDefault;  
}

MsgRouting AwtWindow::WmClose()
{
    SendWindowEvent(java_awt_event_WindowEvent_WINDOW_CLOSING);

    // Rely on above notification to handle quitting as needed
    return mrConsume;  
}

MsgRouting AwtWindow::WmDestroy()
{
    SendWindowEvent(java_awt_event_WindowEvent_WINDOW_CLOSED);
    return AwtComponent::WmDestroy();
}

//
// Override AwtComponent's move handling to first update the java AWT target's position
// fields directly, since Windows and below can be resized from outside of java (by user)
//
MsgRouting AwtWindow::WmMove(int x, int y)
{
#ifndef WINCE
    if ( IsIconic(GetHWnd()) ) {
    // fixes 4065534 (robi.khan@eng)
    // if a window is iconified we don't want to update
    // it's target's position since minimized Win32 windows
    // move to -32000, -32000 for whatever reason
    // NOTE: See also AwtWindow::Reshape
	return mrDoDefault;
    }
#endif /* WINCE */
    // Update the java AWT target component's fields directly
    //
    
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return AwtComponent::WmMove(x, y);
    }

    jobject target = GetTarget();
    jint targetX = env->CallIntMethod(target,
				      WCachedIDs.java_awt_Component_getXMID);
    jint targetY = env->CallIntMethod(target,
				      WCachedIDs.java_awt_Component_getYMID);
	
    RECT rect;
    GetSharedDC()->Lock();
    ::GetWindowRect(GetHWnd(), &rect);
    GetSharedDC()->Unlock();

    if (targetX != rect.left || targetY != rect.top) {
	env->SetIntField(target,
			 WCachedIDs.java_awt_Component_xFID,
			 rect.left);
	env->SetIntField(target,
			 WCachedIDs.java_awt_Component_yFID,
			 rect.top);
        SendComponentEvent(java_awt_event_ComponentEvent_COMPONENT_MOVED);
    }

    return AwtComponent::WmMove(x, y);
}

//
// Override AwtComponent's size handling to first update the java AWT target's dimension
// fields directly, since Windows and below can be resized from outside of java (by user)
//
MsgRouting AwtWindow::WmSize(int type, int w, int h)
{
    if (type == SIZE_MINIMIZED) {
        SendWindowEvent(java_awt_event_WindowEvent_WINDOW_ICONIFIED);
        m_iconic = TRUE;
#ifndef WINCE
	ASSERT(IsIconic(GetHWnd()));
#endif
    } else if (type == SIZE_RESTORED && m_iconic) {
        SendWindowEvent(java_awt_event_WindowEvent_WINDOW_DEICONIFIED);
        m_iconic = FALSE;
    } else if (type == SIZE_MAXIMIZED && m_iconic) {
        SendWindowEvent(java_awt_event_WindowEvent_WINDOW_DEICONIFIED);
        m_iconic = FALSE;
    } else {
	JNIEnv *env;
	
	if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	    return AwtComponent::WmSize(type, w, h);
	}
	
        jobject target = GetTarget();
	// fix 4167248 : ensure the insets are up-to-date before using
	BOOL insetsChanged = UpdateInsets(NULL);
        int newWidth = w + m_insets.left + m_insets.right;
        int newHeight = h + m_insets.top + m_insets.bottom;

#ifdef WINCE
	/* The cmdbar is part of the insets, but is also part of client */
	/* area */
	if (m_cmdBar) {
	    int cmdHeight = wceGetCommandBarHeight(m_cmdBar);
	    newHeight -= cmdHeight;

	    /* Command bar does not automatically resize with frame */
	    ::SendMessage(m_cmdBar, TB_AUTOSIZE, 0, 0);
	    /* If we have "Adornments" on the command bar, they */
	    /* will not be right aligned after the resize and need */
	    /* to be added again. MS documents a function for doing */
	    /* this (:CommandBar_AlignAdornments) which does not exist */
	    /* CommandBar_AddAdornments(m_cmdBar, 0, 0); */
	}
#endif /* WINCE */

	jint targetWidth = env->CallIntMethod(target,
					      WCachedIDs.java_awt_Component_getWidthMID);
	jint targetHeight = env->CallIntMethod(target,
					       WCachedIDs.java_awt_Component_getHeightMID);

        if (targetWidth != newWidth || targetHeight != newHeight ||
	    insetsChanged) {
	    env->SetIntField(target,
			     WCachedIDs.java_awt_Component_widthFID,
			     newWidth);
	    env->SetIntField(target,
			     WCachedIDs.java_awt_Component_heightFID,
			     newHeight);
            if (!m_resizing) {
                SendComponentEvent(java_awt_event_ComponentEvent_COMPONENT_RESIZED);
            }
        }
        return AwtComponent::WmSize(type, w, h);
    }
    
    return mrDoDefault;
}

MsgRouting AwtWindow::WmPaint(HDC)
{
    MsgRouting retVal = mrDoDefault;
    GetSharedDC()->Lock();
    // Get the rectangle that covers all update regions, if any exist.
    RECT r;
    if (::GetUpdateRect(GetHWnd(), &r, FALSE)) {

        // Now remove all update regions from this window -- do it here instead
        // of after the Java upcall, in case any new updating is requested.
        // If we don't do this any components that appear off the 
	// visable screen will not be drawn, and will throw is into a loop of 
	// WmPaint.
	// paulsh@ireland Bug ID 4114208.
 
	::ValidateRect(GetHWnd(), NULL);
	
	::ValidateRect(GetHWnd(), &r);

#ifndef WINCE
	::OffsetRect(&r, m_insets.left, m_insets.top);
#else
	/* Commandbar is part of client rect */
	{
	    int topOffset = m_insets.top;
	    if (m_cmdBar) {
		topOffset -= wceGetCommandBarHeight(m_cmdBar);
	    }
	    ::OffsetRect(&r, m_insets.left, topOffset);
	}
	
#endif
	GetSharedDC()->Unlock();

        if ((r.right-r.left) > 0 && (r.bottom-r.top) > 0 &&
            m_peerObject != NULL && m_callbacksEnabled) {
	    JNIEnv *env;
	    
	    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
		return mrPassAlong;
	    }
	    
	    env->CallVoidMethod(GetPeer(),
				WCachedIDs.PPCComponentPeer_handleExposeMID,
				r.left,
				r.top,
				r.right-r.left,
				r.bottom-r.top);
        } else {
#ifdef WINCE
	    /* If we do not paint anything, we need to call the default
	     * procedure, on wince, as that is where the non-client areas
	     * are painted. Furthemore, the system will keep sending us
	     * WM_PAINT untile we do something
	     */
	    retVal = mrPassAlong;
#else
	    ;
#endif
	}
    } else {
        GetSharedDC()->Unlock();
    }
    
    //Netscape : Bug # 82231 : NT3.51 -When you have 2 dialogs ovelapping 
    //each other, WM_ERASEBKGND msgs are not sent to the back window when the
    //front one is dragged accross it and the front windows borders are 
    //completely within those of the back dialog.  So force "handleExpose"
    //always on versions < 4.0
    
    if (IS_WIN4X) {
	m_eraseBackground = FALSE;
    }
    return retVal;
}

MsgRouting AwtWindow::WmSysCommand(UINT uCmdType, UINT xPos, UINT yPos)
{
#ifndef WINCE
    if (uCmdType == SC_SIZE) {
        m_resizing = TRUE;
    }
#endif /* WINCE: TODO? */
    return mrDoDefault;
}

MsgRouting AwtWindow::WmExitSizeMove()
{
    if (m_resizing) {
        SendComponentEvent(java_awt_event_ComponentEvent_COMPONENT_RESIZED);
        m_resizing = FALSE;
    }
    return mrDoDefault;
}

MsgRouting AwtWindow::WmSettingChange(WORD wFlag, LPCTSTR pszSection)
{
#ifndef WINCE
    if (wFlag == SPI_SETNONCLIENTMETRICS) {
    // user changed window metrics in
    // Control Panel->Display->Appearance
    // which may cause window insets to change
	UpdateInsets(NULL);
	return mrConsume;
    }
#endif /* WINCE */ /* TODO ?*/
    return mrDoDefault;
}

#ifndef WINCE
MsgRouting AwtWindow::WmNcCalcSize(BOOL fCalcValidRects, 
                                   LPNCCALCSIZE_PARAMS lpncsp, UINT& retVal)
{
    if (fCalcValidRects == FALSE) {
        return mrDoDefault;
    }

    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return mrDoDefault;
    }

    jobject target = GetTarget();
    if (env->CallObjectMethod(target,
			      WCachedIDs.java_awt_Window_getWarningStringMID) != NULL) {

	RECT r;
	::CopyRect(&r, &lpncsp->rgrc[0]);
	retVal = DefWindowProc(WM_NCCALCSIZE, fCalcValidRects, (LPARAM)lpncsp);

	// Adjust non-client area for warning banner space.
	m_warningRect.left = lpncsp->rgrc[0].left;
	m_warningRect.right = lpncsp->rgrc[0].right;
	m_warningRect.bottom = lpncsp->rgrc[0].bottom;
	m_warningRect.top = m_warningRect.bottom - ::GetSystemMetrics(SM_CYCAPTION);
        if (GetStyle() & WS_THICKFRAME) {
	    m_warningRect.top -= 2;
        } else {
	    m_warningRect.top += 2;
        }

	lpncsp->rgrc[0].bottom = m_warningRect.top;

	// Convert to window-relative coordinates.
	::OffsetRect(&m_warningRect, -r.left, -r.top);

        // Notify target of Insets change.
        UpdateInsets(NULL);

	return mrConsume;
	// NB:  AwtDialog calls us, and relies on the fact that
	// DefWindowProc() is called if and only if mrDoDefault is
	// not returned by us.
    } else {
	// WM_NCCALCSIZE is usually in response to a resize, but
	// also can be triggered by SetWindowPos(SWP_FRAMECHANGED),
	// which means the insets will have changed - rnk 4/7/1998
	retVal = DefWindowProc(WM_NCCALCSIZE, fCalcValidRects, (LPARAM)lpncsp);
 	UpdateInsets(NULL);
	return mrConsume;
    }
}

MsgRouting AwtWindow::WmNcPaint(HRGN hrgn)
{
    DefWindowProc(WM_NCPAINT, (WPARAM)hrgn, 0);
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return mrConsume;
    }

    jobject target = GetTarget();
    if (env->CallObjectMethod(target,
			      WCachedIDs.java_awt_Window_getWarningStringMID) != NULL) {
	RECT r;
        ::CopyRect(&r, &m_warningRect);
	HDC hDC = ::GetWindowDC(GetHWnd());
	ASSERT(hDC);
	int iSaveDC = ::SaveDC(hDC);
	VERIFY(::SelectClipRgn(hDC, NULL) != NULL);
	VERIFY(::FillRect(hDC, &m_warningRect, ::GetStockObject(BLACK_BRUSH)));

        if (GetStyle() & WS_THICKFRAME) {
            // draw edge
            VERIFY(::DrawEdge(hDC, &r, EDGE_RAISED, BF_TOP));
            r.top += 2;
            VERIFY(::DrawEdge(hDC, &r, EDGE_SUNKEN, BF_RECT));
            ::InflateRect(&r, -2, -2);
        }

	// draw warning text
	LPWSTR text = GET_WSTRING(env->CallObjectMethod(target,
							WCachedIDs.java_awt_Window_getWarningStringMID));
	VERIFY(::SetBkColor(hDC, RGB(255, 225, 0)) != CLR_INVALID);
	VERIFY(::SetTextColor(hDC, RGB(0, 0, 0)) != CLR_INVALID);
	VERIFY(::SelectObject(hDC, ::GetStockObject(DEFAULT_GUI_FONT)) != NULL);
	VERIFY(::SetTextAlign(hDC, TA_LEFT | TA_BOTTOM) != GDI_ERROR);
        VERIFY(::ExtTextOutW(hDC, r.left, r.bottom, ETO_CLIPPED | ETO_OPAQUE, 
	                     &r, text, wcslen(text), NULL));
	VERIFY(::RestoreDC(hDC, iSaveDC));
	::ReleaseDC(GetHWnd(), hDC);
    }
    return mrConsume;
}

MsgRouting AwtWindow::WmNcHitTest(UINT x, UINT y, UINT& retVal)
{
    POINT pt = { x, y };
    if (::PtInRect(&m_warningRect, pt)) {
        retVal = HTNOWHERE;
    } else {
        retVal = DefWindowProc(WM_NCHITTEST, 0, MAKELPARAM(x, y));
    }
    return mrConsume;
}
#endif /* ! WINCE */

BOOL CALLBACK InvalidateChildRect(HWND hWnd, LPARAM)
{
    ::InvalidateRect(hWnd, NULL, TRUE);
    return TRUE;
}

void AwtWindow::Invalidate(RECT* r)
{
    ::InvalidateRect(GetHWnd(), NULL, TRUE);
#ifndef WINCE
    ::EnumChildWindows(GetHWnd(), (WNDENUMPROC)InvalidateChildRect, 0);
#else /* WINCE */
    /* no EnumChildWindows, so we traverse them ourself */
    HWND hwnd=::GetWindow(GetHWnd(), GW_CHILD);
    if (hwnd) {
       InvalidateChildRect(hwnd, NULL);
       while (hwnd=::GetWindow(hwnd, GW_HWNDNEXT)) {
            InvalidateChildRect(hwnd, NULL);
        }
    }
#endif /* WINCE */
}

void AwtWindow::SetResizable(BOOL isResizable)
{
    if (IsEmbedded()) {
	return;
    }
#ifndef WINCE
    long style = GetStyle();
    static const long SizeStyles = WS_THICKFRAME|WS_MAXIMIZEBOX|WS_MINIMIZEBOX;

    if (isResizable) {
	style |= SizeStyles;
    } else {
	style &= ~SizeStyles;
    }
    SetStyle(style);
    RedrawNonClient();
#endif /* WINCE */
}

// SetWindowPos flags to cause frame edge to be recalculated
static const UINT SwpFrameChangeFlags =
    SWP_FRAMECHANGED | /* causes WM_NCCALCSIZE to be called */
    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
    SWP_NOACTIVATE | 
    SWP_NOREPOSITION
#ifndef WINCE
    | SWP_NOSENDCHANGING | SWP_NOCOPYBITS
#endif
    ;


//
// Forces WM_NCCALCSIZE to be called to recalculate
// window border (updates insets) without redrawing it
//
void AwtWindow::RecalcNonClient()
{
    ::SetWindowPos(GetHWnd(), (HWND) NULL, 0, 0, 0, 0, SwpFrameChangeFlags
#ifndef WINCE
		   |SWP_NOREDRAW
#endif
		   );
}

//
// Forces WM_NCCALCSIZE to be called to recalculate
// window border (updates insets) and redraws border to match
//
void AwtWindow::RedrawNonClient()
{
    ::SetWindowPos(GetHWnd(), (HWND) NULL, 0, 0, 0, 0, SwpFrameChangeFlags
#ifndef WINCE
		   |SWP_ASYNCWINDOWPOS
#endif
		   );
}

/////////////////////////////////////////////////////////////////////////////////////////////
// WindowPeer native methods
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCWindowPeer_initIDs(JNIEnv *env,
					    jclass cls) 
{
    GET_METHOD_ID (PPCWindowPeer_postFocusOnActivateMID,
		   "postFocusOnActivate", "()V");

    cls = env->FindClass ("sun/awt/pocketpc/PPCPanelPeer");
	
    if (cls == NULL)
	return;

    GET_FIELD_ID (PPCPanelPeer_insets_FID, "insets_",
		  "Ljava/awt/Insets;");
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCWindowPeer_toFront(JNIEnv *env,
					    jobject self)
{
    CHECK_PEER(self);
    AwtWindow* w = PDATA(AwtWindow, self);

    ::SetWindowPos(w->GetHWnd(), HWND_TOP, 0,0,0,0,
                   SWP_NOMOVE|SWP_NOSIZE);
    ::SetForegroundWindow(w->GetHWnd()) ;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCWindowPeer_toBack(JNIEnv *env,
					   jobject self)
{
    CHECK_PEER(self);
    AwtWindow* w = PDATA(AwtWindow, self);

    ::SetWindowPos(w->GetHWnd(), HWND_BOTTOM, 0,0,0,0,
                   SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCWindowPeer__1setTitle(JNIEnv *env,
					       jobject self,
					       jstring title)
{
    CHECK_PEER(self);
    CHECK_NULL(title, "null title");

    AwtWindow* w = PDATA(AwtWindow, self);

#ifndef WINCE
    if (IS_NT) {
#else
    {
#endif
	int length = env->GetStringLength(title);
	WCHAR *buffer = new WCHAR[length + 1];
	env->GetStringRegion(title, 0, length, (jchar *) buffer);
	buffer[length] = L'\0';
	VERIFY(::SetWindowTextW(w->GetHWnd(), buffer));
	delete[] buffer;
#ifndef WINCE
    } else {
	int length = env->GetStringLength(title);
	char *buffer = new char[length * 2 + 1];
	AwtFont::javaString2multibyte(env, title, buffer, length);	
	VERIFY(::SetWindowText(w->GetHWnd(), buffer));
	delete[] buffer;
    }
#else
    }
#endif /* WINCE */
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCWindowPeer__1setResizable(JNIEnv *env,
						  jobject self,
						  jboolean resizable)
{
    CHECK_PEER(self);
    AwtWindow* w = PDATA(AwtWindow, self);
    ASSERT(!IsBadReadPtr(w, sizeof(AwtWindow)));
    w->SetResizable(resizable != 0);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCWindowPeer_create(JNIEnv *env,
					   jobject self, 
					   jobject hParent)
{
    CHECK_PEER(hParent);
    AwtToolkit::CreateComponent(self, hParent, (AwtToolkit::ComponentFactory)AwtWindow::Create);

    CHECK_PEER_CREATION(self);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCWindowPeer_updateInsets(JNIEnv *env,
						 jobject self,
						 jobject insets)
{
    CHECK_PEER(self);
    CHECK_NULL(insets, "null insets");

    AwtWindow* w = PDATA(AwtWindow, self);
    w->UpdateInsets(insets);
}

#ifdef WINCE
/* TranslateToClient Coordinates: same as SubtractInsets, except
 * that a Frame can override this as it needs to when there are
 * menubars involved.
 */
void AwtWindow::TranslateToClient(int &x, int &y)
{
    x -= m_insets.left;
    y -= m_insets.top;
}

void AwtWindow::TranslateToClient(long &x, long &y)
{
    x -= m_insets.left;
    y -= m_insets.top;
}
#endif /* WINCE */

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCWindowPeer_requestComponentFocusIfActive(JNIEnv *env,
								  jobject self,
								  jobject peer)
{
    CHECK_PEER(self);
    AwtWindow * window = PDATA(AwtWindow, self);
    ASSERT(peer != NULL);
    AwtComponent * component = PDATA(AwtComponent, peer);

    // Request the focus, passing the containing window as the wParam so
    // SetFocus() will not be called (causing unwanted activation) if
    // the components ancestor top-level window is not active
    // (see AwtComponent handling of WM_AWT_COMPONENT_SETFOCUS)
    WPARAM	wParam = (WPARAM)window->GetHWnd();
    VERIFY(::PostMessage(component->GetHWnd(), WM_AWT_COMPONENT_SETFOCUS, wParam, 0));
}
