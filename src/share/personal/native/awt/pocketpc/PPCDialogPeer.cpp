/*
 * @(#)PPCDialogPeer.cpp	1.14 06/10/10
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
#include "sun_awt_pocketpc_PPCDialogPeer.h"
#include "PPCDialogPeer.h"

#if defined(DEBUG)
// counts how many nested modal dialogs are open, a sanity
// check to ensure the somewhat complicated disable/enable
// code is working properly
int AwtModalityNestCounter = 0;
#endif
// property name tagging windows disabled by modality
static const char * ModalDisableProp = "SunAwtModalDisableProp";

///////////////////////////////////////////////////////////////////////////
// AwtDialog class methods

AwtDialog::AwtDialog()
  : AwtFrame()
{
    m_modalWnd = NULL;
    disabledLevel = 0;
}

AwtDialog::~AwtDialog()
{
    if ( m_modalWnd != NULL ) {
	WmEndModal();
    }
}

static TCHAR *dialogClassName = TEXT( "DIALOG" );

const TCHAR*
AwtDialog::GetClassName()
{
  return dialogClassName;
}

// Override RegisterClass so dialogs do not have icon by default
void
AwtDialog::RegisterClass()
{
    const TCHAR* className = GetClassName();
    WNDCLASS  wc;

    if ( !::GetClassInfo( ::GetModuleHandle( NULL ), className, &wc ) ) {
#ifndef WINCE
        wc.style         = CS_OWNDC | CS_SAVEBITS;
#else
	wc.style         = 0;
#endif
	wc.lpfnWndProc   = (WNDPROC)::DefWindowProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = DLGWINDOWEXTRA;
	wc.hInstance     = ::GetModuleHandle( NULL );
	wc.hIcon         = NULL; // Dialogs do not have an icon by default
	wc.hCursor       = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = className;

        ATOM ret = ::RegisterClass( &wc );
        ASSERT( ret != 0 );
    }
    return;
}

// Create a new AwtDialog object and window.  
AwtDialog *
AwtDialog::Create( jobject self,
                   jobject hParent )
{
    JNIEnv *env;
    if ( JVM->AttachCurrentThread( (void **)&env, 0 ) != 0 ) {
        return 0;
    }

    jobject target = env->GetObjectField( self,
                                          WCachedIDs.PPCObjectPeer_targetFID );
    CHECK_NULL_RETURN( target, "null target" );

    HWND hwndParent = 0;
    if ( hParent != 0 ) {
	CHECK_PEER_RETURN( hParent );
	AwtFrame* parent = PDATA( AwtFrame, hParent );
	hwndParent = parent->GetHWnd();
    }

    AwtDialog* dialog = new AwtDialog();
    CHECK_NULL_RETURN( dialog, "AwtDialog() failed" );
	
    //Netscape : Bug # 79863 : Make sure the size of the dialog is at 
    //least the minimim tracking size because making a dialog of
    //zero size renders it invisible.
#ifndef WINCE
    #define MIN_DIALOG_WIDTH ::GetSystemMetrics(SM_CXMINTRACK)
    #define MIN_DIALOG_HEIGHT (::GetSystemMetrics(SM_CYMINTRACK) + \
			     ::GetSystemMetrics(SM_CYSIZEFRAME))
#else
      /* TODO: find proper mins */
    #define MIN_DIALOG_WIDTH 32
    #define MIN_DIALOG_HEIGHT 32
#endif
    if ( env->CallIntMethod( target,
                             WCachedIDs.java_awt_Component_getWidthMID ) <
         MIN_DIALOG_WIDTH ) {
	env->SetIntField( target,
                          WCachedIDs.java_awt_Component_widthFID,
                          MIN_DIALOG_WIDTH );
    }
    if ( env->CallIntMethod( target,
                             WCachedIDs.java_awt_Component_getWidthMID ) <
         MIN_DIALOG_HEIGHT ) {
	env->SetIntField( target,
                          WCachedIDs.java_awt_Component_heightFID,
                          MIN_DIALOG_HEIGHT );
    }	

    int colorId = IS_WIN4X ? COLOR_3DFACE : COLOR_WINDOW;
    DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN;
    style &= ~( WS_MINIMIZEBOX | WS_MAXIMIZEBOX );
    //DWORD exStyle = IS_WIN4X ? WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME: 0;
    DWORD exStyle = IS_WIN4X ? WS_EX_WINDOWEDGE : 0; // WS_EX_DLGMODALFRAME gets added in SetResizable now...
    
    dialog->CreateHWnd( TEXT( "" ),
                        style, exStyle,
                        env->CallIntMethod( target,
                                 WCachedIDs.java_awt_Component_getXMID ), 
                        env->CallIntMethod( target,
                                 WCachedIDs.java_awt_Component_getYMID ),
                        env->CallIntMethod( target,
                                 WCachedIDs.java_awt_Component_getWidthMID ),
                        env->CallIntMethod( target,
                                 WCachedIDs.java_awt_Component_getHeightMID ),
                       hwndParent,
                       NULL,
                       ::GetSysColor( COLOR_WINDOWTEXT ),
                       ::GetSysColor( colorId ),
                       self
                      );

    //Netscape: Force the WM_NCALCSIZE to be sent to the window which 
    //in turn makes the window compute its insets.  This has to happen
    //now so that subsequent calls to getInsets() will return proper
    //values.
    dialog->RecalcNonClient();
    dialog->UpdateDialogIcon();
    dialog->UpdateSystemMenu();
 
    if ( env->CallObjectMethod( target,
              WCachedIDs.java_awt_Component_getForegroundMID ) == 0 ) {
        env->CallVoidMethod( self,
                             WCachedIDs.PPCDialogPeer_setDefaultColorMID,
                             (jboolean)( IS_WIN4X != 0 ) );
    }
    AwtToolkit::RegisterTopLevelWindow( dialog->GetHWnd() );
    return dialog;
}

//
// Disables top-level windows for a modal dialog
//	hWndDlg		Modal dialog 
//
void
AwtDialog::ModalDisable( HWND hWndDlg )
{
#if defined(DEBUG)
    ASSERT( AwtModalityNestCounter >= 0 &&
            AwtModalityNestCounter <= 1000); // sanity check
    AwtModalityNestCounter++;
#endif
    AwtDialog::ResetDisabledLevel( hWndDlg );
#ifndef WINCE
    ::EnumThreadWindows( AwtToolkit::MainThread(),
                         (WNDENUMPROC)DisableTopLevelsCallback,
                         (LPARAM)hWndDlg );
#else
    AwtToolkit::EnumTopLevelWindows(
		   (WNDENUMPROC)DisableTopLevelsCallback,
                   (LPARAM)hWndDlg );
#endif
    return;
}

// 
// Re-enables top-level windows for a modal dialog
//	hWndDlg		Modal dialog
//
void
AwtDialog::ModalEnable( HWND hWndDlg )
{
#if defined(DEBUG)
    ASSERT(AwtModalityNestCounter > 0 &&
           AwtModalityNestCounter <= 1000); // sanity check
    AwtModalityNestCounter--;
#endif
    int disabledLevel = AwtDialog::GetDisabledLevel( hWndDlg );
#ifndef WINCE
    ::EnumThreadWindows( AwtToolkit::MainThread(),
                         (WNDENUMPROC)EnableTopLevelsCallback,
                         (LPARAM)disabledLevel );
#else
    AwtToolkit::EnumTopLevelWindows( (WNDENUMPROC)EnableTopLevelsCallback, 
                                     (LPARAM)disabledLevel );
#endif
    AwtDialog::ResetDisabledLevel( hWndDlg );
    return;
}

//
// Brings a window to foreground when modal dialog goes away
// (the Win32 Z-order is very screwy if another app is active
//  when the modal dialog goes away, so we explicitly activate 
//  the first enabled, visible, window we find)
//
//	hWndParent	Parent window of the dialog
//
void
AwtDialog::ModalNextWindowToFront( HWND hWndParent )
{
    if ( ::IsWindowEnabled( hWndParent ) && ::IsWindowVisible( hWndParent ) ) {
    // always give parent first shot at coming to the front
	::SetForegroundWindow( hWndParent );
    } else {
    // otherwise, activate the first enabled, visible window we find
#ifndef WINCE
	::EnumThreadWindows( AwtToolkit::MainThread(),
                             (WNDENUMPROC)NextWindowToFrontCallback,
                             0L );
#else
	AwtToolkit::EnumTopLevelWindows(
                                      (WNDENUMPROC)NextWindowToFrontCallback,
                                      0L);
#endif
    }
    return;
}

//
// Disables top-level windows, incrementing a counter on each disabled
// window to indicate how many nested modal dialogs need the window disabled
//
BOOL CALLBACK
AwtDialog::DisableTopLevelsCallback( HWND hWnd, LPARAM lParam )
{
    HWND hWndDlg = (HWND)lParam;

    if ( hWnd == hWndDlg || hWnd == AwtToolkit::GetInstance().GetHWnd() ) {
	return TRUE; // ignore myself and toolkit when disabling
    }

    // the drop-down list portion of combo-boxes are actually top-level
    // windows with WS_CHILD, so ignore these guys...
    if ( ( ::GetWindowLong( hWnd, GWL_STYLE ) & WS_CHILD ) != 0) {
	return TRUE;
    }

    ASSERT( hWndDlg == NULL || IsWindow( hWndDlg ) );

    AwtDialog::IncrementDisabledLevel( hWnd, 1 );
    return TRUE;
}

//
// Enables top-level window if no modal dialogs need it disabled
//
BOOL CALLBACK
AwtDialog::EnableTopLevelsCallback( HWND hWnd, LPARAM lParam ) 
{
    HWND hWndDlg = (HWND)lParam;
    int	disabledLevel = AwtDialog::GetDisabledLevel( hWnd );
    int	dlgDisabledLevel = (int)lParam;

    if ( disabledLevel == 0 || disabledLevel  < dlgDisabledLevel ) {
        // skip enabled windows or windows disabled after the 
        // given modal dialog became modal
	return TRUE;
    }

    AwtDialog::IncrementDisabledLevel( hWnd, -1 );
    return TRUE;
}

/*
 * Brings the first enabled, visible window to the foreground
 */
BOOL CALLBACK
AwtDialog::NextWindowToFrontCallback( HWND hWnd, LPARAM lParam )
{
    if ( ::IsWindowVisible( hWnd ) && ::IsWindowEnabled( hWnd ) ) {
	::SetForegroundWindow( hWnd );
	return FALSE; // stop callbacks
    }
    return TRUE;
}

int
AwtDialog::GetDisabledLevel( HWND hWnd )
{
#ifndef WINCE
    int	disabledLevel = (int)::GetProp( hWnd, ModalDisableProp );
    ASSERT( disabledLevel >= 0 && disabledLevel <= 1000 );
    return disabledLevel;
#else
    return AwtToolkit::GetDisabledLevel( hWnd );
#endif // !WINCE
}

#if 0 // defined(DEBUG)
// debugging-only function; useful for tracking down disable/enable problems
static void
DbgShowDisabledLevel( HWND hWnd, int disabledLevel )
{    
    // put disabled counter in title bar text
    char szDisabled[256];
    wsprintf( szDisabled, "disabledLevel = %d", disabledLevel );
    ::SetWindowText( hWnd, szDisabled );
    return;
}

    #define DBG_SHOW_DISABLED_LEVEL( _win, _dl ) \
	    DbgShowDisabledLevel( (_win), (_dl) )

#else    

    #define DBG_SHOW_DISABLED_LEVEL( _win, _dl )

#endif // 0

void
AwtDialog::IncrementDisabledLevel( HWND hWnd, int increment )
{
    int	disabledLevel = GetDisabledLevel( hWnd );
    ASSERT( increment == -1 || increment == 1); // only allow increment/decrement
    disabledLevel += increment;
#ifndef WINCE
    if ( disabledLevel == 0 ) {
	// don't need the property any more...
	ResetDisabledLevel( hWnd );
    } else {
	::SetProp( hWnd, ModalDisableProp, (HANDLE)disabledLevel );
    }
#else // WINCE
    AwtToolkit::SetDisabledLevel( hWnd, disabledLevel );
#endif // !WINCE
    ::EnableWindow( hWnd, disabledLevel == 0 );
    DBG_SHOW_DISABLED_LEVEL( hWnd, GetDisabledLevel( hWnd ) );
    return;
}

void
AwtDialog::ResetDisabledLevel( HWND hWnd )
{
#ifndef WINCE
    ::RemoveProp( hWnd, ModalDisableProp );
#else
    AwtToolkit::ResetDisabledLevel( hWnd );
#endif // !WINCE
    DBG_SHOW_DISABLED_LEVEL( hWnd, GetDisabledLevel( hWnd ) );
    return;
}

MsgRouting
AwtDialog::WmShowModal()
{
    ASSERT( ::GetCurrentThreadId() == AwtToolkit::MainThread() );

    // disable top-level windows
    AwtDialog::ModalDisable( GetHWnd() );
    // enable myself (could have been explicitly disabled via setEnabled)
    ::EnableWindow( GetHWnd(),
                    TRUE); // is this the right behaviour? (rnk 6/4/98)
    // show window and set focus to it
    SendMessage( WM_AWT_COMPONENT_SHOW, SW_SHOW );
    // normally done by DefWindowProc(WM_ACTIVATE),
    // but we override that message so we do it explicitly
    ::SetFocus( GetHWnd() );

    m_modalWnd = GetHWnd();

    return mrConsume;
}

MsgRouting
AwtDialog::WmEndModal()
{

#ifdef WINCE
    // There is a condition where we can be called twice
    // needs some further investigation, but this seems to fix it
    // may still be a thread timing problem
    if ( !m_modalWnd ) {
	return mrConsume;
    }
#endif // WINCE
    ASSERT( ::GetCurrentThreadId() == AwtToolkit::MainThread() );
    ASSERT( ::IsWindowVisible( GetHWnd() ) );
    ASSERT( ::IsWindow( m_modalWnd ) );

    // re-enable top-level windows
    AwtDialog::ModalEnable( GetHWnd() );

    HWND hWndParent = ::GetParent( GetHWnd() );
    BOOL isEnabled = ::IsWindowEnabled( hWndParent );

    // Need to temporarily enable the dialog's owner window
    // or Windows could activate another application. This
    // happens when another app was activated previous to
    // a nested dialog being activated/closed (in which case the
    // owner wouldn't be re-enabled in ModalEnable).
    ::EnableWindow( hWndParent, TRUE );
    // hide the dialog
    SendMessage( WM_AWT_COMPONENT_SHOW, SW_HIDE );
    // restore the owner's true enabled state
    ::EnableWindow( hWndParent, isEnabled );

    // bring the next window in the stack to the front
    AwtDialog::ModalNextWindowToFront( hWndParent );
    m_modalWnd = NULL;
    
    return mrConsume;
}

void
AwtDialog::SetResizable( BOOL isResizable )
{
    // call superclass
    AwtFrame::SetResizable( isResizable );

    long style = GetStyle();
    long xstyle = GetStyleEx();
    if ( isResizable ) {
        // remove modal frame
	xstyle &= ~WS_EX_DLGMODALFRAME;
    } else {
        // add modal frame
	xstyle |= WS_EX_DLGMODALFRAME;
    }
    // dialogs are never minimizable/maximizable, so remove those bits
    style &= ~( WS_MINIMIZEBOX|WS_MAXIMIZEBOX );
    SetStyle( style );
    SetStyleEx( xstyle) ;
    RedrawNonClient();
    return;
}

// Adjust system menu so that:
//  Non-resizable dialogs only have Move and Close items
//  Resizable dialogs only have Move/Close/Size
// Normally, Win32 dialog system menu handling is done via
// CreateDialog/DefDlgProc, but our dialogs are using DefWindowProc
// so we handle the system menu ourselves
void
AwtDialog::UpdateSystemMenu()
{
#ifndef WINCE
    HWND hWndSelf = GetHWnd();

    BOOL isResizable = ( (GetStyle() & WS_THICKFRAME ) != 0 );

    // first of all, restore the default menu
    ::GetSystemMenu( hWndSelf, TRUE );
    // now get a working copy of the menu
    HMENU hMenuSys = GetSystemMenu( hWndSelf, FALSE );
    // remove inapplicable sizing commands
    ::DeleteMenu( hMenuSys, SC_MINIMIZE, MF_BYCOMMAND );
    ::DeleteMenu( hMenuSys, SC_RESTORE, MF_BYCOMMAND );
    ::DeleteMenu( hMenuSys, SC_MAXIMIZE, MF_BYCOMMAND );
    if ( !isResizable ) {
	::DeleteMenu( hMenuSys, SC_SIZE, MF_BYCOMMAND );
    }
    // remove separator if only 3 items left (Move, Separator, and Close)
    if ( ::GetMenuItemCount( hMenuSys ) == 3 ) {
	MENUITEMINFO mi;
	mi.cbSize = sizeof( MENUITEMINFO);
	mi.fMask = MIIM_TYPE;
	::GetMenuItemInfo( hMenuSys, 1, TRUE, &mi );
	if ( mi.fType & MFT_SEPARATOR ) {
	    ::DeleteMenu( hMenuSys, 1, MF_BYPOSITION );
	}
    }
#endif // !WINCE
    return;
}

// Call this when dialog isResizable property changes, to 
// hide or show the small icon in the upper left corner
void
AwtDialog::UpdateDialogIcon()
{
    HWND hWndSelf = GetHWnd();
#ifndef WINCE
    BOOL isResizable = ( (GetStyle() & WS_THICKFRAME ) != 0 );

    HWND hWndOwner = GetWindow( hWndSelf, GW_OWNER) ;
    
    if ( isResizable ) {
        // if we're resizable, use the icon from our owner window
	HICON hIconOwnerBig;
	HICON hIconOwnerSmall;
	HICON hIconClassBig;
	HICON hIconClassSmall;
	HICON hIconDialog;

	// try getting owner's small icon
	hIconOwnerSmall = (HICON)::SendMessage( hWndOwner,
                                                WM_GETICON, ICON_SMALL, NULL );
        hIconOwnerBig = (HICON)::SendMessage( hWndOwner,
                                              WM_GETICON, ICON_BIG, NULL );
	hIconClassBig = (HICON)::GetClassLong( hWndOwner, GCL_HICON );
	hIconClassSmall= (HICON)::GetClassLong( hWndOwner, GCL_HICONSM );
	if ( hIconOwnerSmall != NULL ) {
	    hIconDialog = hIconOwnerSmall;
	} else if ( hIconOwnerBig != NULL ) {
	    hIconDialog = hIconOwnerBig;
	} else if ( hIconClassSmall != NULL ) {
	    hIconDialog = hIconClassSmall;
	} else {
	    hIconDialog = hIconClassBig;
	}
	::PostMessage( hWndSelf, WM_SETICON, ICON_SMALL, (LPARAM)hIconDialog );
	::PostMessage( hWndSelf, WM_SETICON, ICON_BIG, (LPARAM)hIconDialog );
    } else {
#endif // !WINCE
        // It's not clear why these calls are necessary. Currently
        // they cause a hang when they're received in
        // AwtComponent::WindowProc(), even though we merely pass them
        // on to DefWindowProc() rather than handling them ourselves.
        // Comment them out for the time being.
        // if we're not sizable remove the icon
        // ::PostMessage( hWndSelf, WM_SETICON, ICON_SMALL, NULL );
        // ::PostMessage( hWndSelf, WM_SETICON, ICON_BIG, NULL );
#ifndef WINCE
    }
#endif // !WINCE
    return;
}

// Override WmStyleChanged to adjust system menu for sizable/non-resizable dialogs
MsgRouting
AwtDialog::WmStyleChanged( WORD wStyleType, LPSTYLESTRUCT lpss )
{
    UpdateSystemMenu();
    UpdateDialogIcon();
    return mrConsume;
}

MsgRouting
AwtDialog::WmSize( int type, int w, int h )
{
    UpdateSystemMenu(); // adjust to reflect restored vs. maximized state
    return AwtFrame::WmSize( type, w, h );
}

LRESULT
AwtDialog::WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
{
    MsgRouting mr = mrDoDefault;
    LRESULT retValue = 0L;

    switch(message) {
	case WM_AWT_DLG_SHOWMODAL:
	    mr = WmShowModal();
	    break;
	case WM_AWT_DLG_ENDMODAL:
	    mr = WmEndModal();
	    break;
    }

    if ( mr != mrConsume ) {
        retValue = AwtFrame::WindowProc( message, wParam, lParam );
    }
    return retValue;
}

///////////////////////////////////////////////////////////////////////////
// JNI methods

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCDialogPeer_initIDs( JNIEnv *env, jclass cls )
{
    GET_METHOD_ID( PPCDialogPeer_setDefaultColorMID,
                   "setDefaultColor", "(Z)V" );
    return;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCDialogPeer_create( JNIEnv *env,
                                            jobject self,
                                            jobject hParent )
{
    AwtToolkit::CreateComponent(
        self, hParent, (AwtToolkit::ComponentFactory)AwtDialog::Create );

    CHECK_PEER_CREATION( self );
    return;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCDialogPeer_showModal( JNIEnv *env,
                                              jobject self )
{
    CHECK_PEER( self );
    AwtDialog* d = PDATA( AwtDialog, self );
    ::PostMessage( d->GetHWnd(), WM_AWT_DLG_SHOWMODAL, 0, 0L );
    return;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCDialogPeer_endModal( JNIEnv *env,
                                             jobject self )
{
    CHECK_PEER( self );
    AwtDialog* d = PDATA( AwtDialog, self );
    ::SendMessage( d->GetHWnd(), WM_AWT_DLG_ENDMODAL, 0, 0L );
    return;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCDialogPeer_setTitleNative( JNIEnv *env, 
                                                    jobject thisobj,
                                                    jbyteArray title )
{
    // Leave temporarily; not used in Pjava implementation.
    return;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCDialogPeer_setResizable( JNIEnv *env, 
                                                  jobject thisobj,
                                                  jboolean resizable )
{
    // TODO
    return;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCDialogPeer_setModal (JNIEnv *env,
                                              jobject thisobj,
                                              jboolean modal )
{
    // TODO
    return;
}
