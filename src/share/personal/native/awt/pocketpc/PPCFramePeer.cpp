/*
 * @(#)PPCFramePeer.cpp	1.14 06/10/10
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
#include "sun_awt_pocketpc_PPCFramePeer.h"
#include "java_awt_event_WindowEvent.h"
#include "PPCFramePeer.h"
#include "awt.h"

#ifdef WINCE

#include <commctrl.h> /* For command bar */
#include <wceCompat.h>

#endif /* WINCE */

#define MAX( x, y )     ( ( x ) > ( y )  ? ( x ) : ( y ) )

// g_bMenuLoop is here because wince 3.0 is not
// generating the WM_ENTERMENULOOP and WM_EXITMENULOOP 
// messages... as a result, we have to set g_bMenuLoop
// ourselves. Extra attention may need to be given to
// g_bMenuLoop.  If you experience a crash in
// AwtToolkit::PreProcessMsg, it may have to do with this.
extern BOOL g_bMenuLoop;

enum FrameExecIds {
    FRAME_SETMENUBAR
};

AwtFrame::AwtFrame()
  :AwtWindow()
{
    m_hIcon = NULL;
    m_parentWnd = NULL;
    menuBar = NULL;
    m_isEmbedded = FALSE;
    m_extraHeight = 0;
    m_ignoreWmSize = FALSE;
    m_isMenuDropped = FALSE;
}

AwtFrame::~AwtFrame()
{
  if ( m_hIcon != 0 ) {
    ::DestroyIcon( m_hIcon );
  }

  g_bMenuLoop = FALSE;
}

const TCHAR *
AwtFrame::GetClassName()
{
    return TEXT( "FRAME" );
}

// Create a new AwtFrame object and window.  
AwtFrame*
AwtFrame::Create( jobject self, jobject hParent )
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
        AwtFrame* parent = PDATA(AwtFrame, hParent);
        hwndParent = parent->GetHWnd();
    }

    AwtFrame* frame = new AwtFrame();
    if ( frame == 0 ) {
        return 0;
    }

    frame->m_isEmbedded = FALSE;
#if 0
    // the client area 
    // of the browser is a Java Frame for parenting purposes, but really a 
    // Windows child window
    frame->m_isEmbedded = 
        (IsInstanceOf((HObject*)target, "sun/awt/EmbeddedFrame") &&
         unhand((Hsun_awt_windows_WEmbeddedFrame*)target)->handle != 0);

    if (frame->m_isEmbedded) {
        hwndParent = 
            (HWND)(unhand((Hsun_awt_windows_WEmbeddedFrame*)target)->handle);
        RECT rect;
        ::GetClientRect(hwndParent, &rect);
        frame->RegisterClass();
        frame->m_hwnd = ::CreateWindowEx(
#ifndef WINCE
                     WS_EX_NOPARENTNOTIFY, 
#else
                     0,
#endif // WINCE
                                         frame->GetClassName(), TEXT(""),
                                         WS_CHILD | WS_CLIPCHILDREN, 0, 0,
                                         rect.right, rect.bottom,
                                         hwndParent, NULL, 
                                         ::GetModuleHandle(NULL), NULL);

        frame->LinkObjects((Hsun_awt_windows_WComponentPeer*)self);
        frame->SubclassWindow();

        // Update target's dimensions to reflect this embedded window.
        ::GetClientRect(frame->m_hwnd, &rect);
        ::MapWindowPoints(frame->m_hwnd, hwndParent, (LPPOINT)&rect, 2);
        unhand(target)->x = rect.left;
        unhand(target)->y = rect.top;
        unhand(target)->width = rect.right - rect.left;
        unhand(target)->height = rect.bottom - rect.top;
    } else {
#endif // 0
        frame->CreateHWnd( frame->GetClassName(),
#ifndef WINCE
                           WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
#else
                           WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
#endif // WINCE
                           WS_EX_WINDOWEDGE,
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
                           ::GetSysColor( COLOR_WINDOWFRAME ),
                           self);
#ifdef WINCE
        /* command bar will be added later, if necessary */
        frame->m_cmdBar= NULL;

        /* icon is set in the window class, but if we don't send */
        /* the WM_SETICON message, it won't appear in the task bar */
        {
            HICON hIcon = (HICON) ::SendMessage(frame->m_hwnd,
                                                WM_GETICON,
                                                ICON_SMALL, (LPARAM)0);
            if (!hIcon) {
              hIcon = AwtToolkit::GetInstance().GetAwtIcon();
            }
            if (hIcon) {
              ::SendMessage(frame->m_hwnd, WM_SETICON, ICON_SMALL,
                            (LPARAM)hIcon);
            }
        }
#endif // WINCE

        //Netscape: Force the WM_NCALCSIZE to be sent to the window which 
        //in turn makes the window compute its insets.  This has to happen
        //now so that subsequent calls to getInsets() will return proper
        //values.
        // Comment out call to RecaclNonClient() for the nonce,
        // so that Indra can get further in debugging.
        // frame->RecalcNonClient();
#if 0
    }
#endif // 0 (end of "if isEmbedded" statement
    
    AwtToolkit::RegisterTopLevelWindow( frame->GetHWnd() );
    return frame;
}

/*
 * Override AwtComponent's Reshape to keep this frame in place if it is
 * an embedded 'frame'
 */
void
AwtFrame::Reshape( int x, int y, int width, int height ) 
{
    if ( IsEmbedded() ) {
        x = y = 0;
    }
    AwtWindow::Reshape( x, y, width, height );
    return;
}

// Show the frame, and activate it.
void
AwtFrame::Show()
{
    VERIFY( ::PostMessage( GetHWnd(),
                          WM_AWT_COMPONENT_SHOW,
                          SW_SHOW, 0) );
    return;
}

MsgRouting
AwtFrame::WmSize( int type, int w, int h )
{
    if ( ! m_ignoreWmSize ) {
        return AwtWindow::WmSize( type, w, h );
    }
    return mrDoDefault;
}
    
MsgRouting
AwtFrame::WmSetCursor( HWND hWnd, UINT hitTest, UINT message, 
                       BOOL& retVal )
{
    if( IsEmbedded() ) {
        // Don't pass WM_SETCURSOR messages on to an embedded frame's parent
        // unless EmbeddedFrame.isCursorAllowed() is true
        JNIEnv *env;
        if ( JVM->AttachCurrentThread( (void **)&env, 0 ) != 0 ) {
          return mrConsume; //?
        }
        jobject target = env->GetObjectField( GetPeer(),
                                       WCachedIDs.PPCObjectPeer_targetFID );
        if ( target == 0 ) {
          return mrConsume;
        }

        BOOL isCursorAllowed = env->CallBooleanMethod( target,
                          WCachedIDs.sun_awt_EmbeddedFrame_isCursorAllowedMID,
                          "()Z" );
        if ( isCursorAllowed ) {
          ::SetCursor( GetCursor( TRUE ) );
          return mrDoDefault;
        } else {
          retVal = FALSE;
          return mrConsume;
        }
    }
    return AwtWindow::WmSetCursor(hWnd, hitTest, message, retVal);
}

MsgRouting
AwtFrame::WmActivate( UINT nState, BOOL fMinimized )
{
    if (nState == WA_INACTIVE) {
        SendWindowEvent(java_awt_event_WindowEvent_WINDOW_DEACTIVATED);
        return mrDoDefault;
    } 
       
    SendWindowEvent(java_awt_event_WindowEvent_WINDOW_ACTIVATED);
    
    JNIEnv *env;
    if ( JVM->AttachCurrentThread( (void **)&env, 0 ) != 0 ) {
        return mrConsume; //?
    }

    env->CallVoidMethod(GetPeer(),
			WCachedIDs.PPCWindowPeer_postFocusOnActivateMID);
    
    return mrConsume;
}

MsgRouting
AwtFrame::WmEnterMenuLoop( BOOL isTrackPopupMenu )
{
    if ( !isTrackPopupMenu ) {
        m_isMenuDropped = TRUE;
    }
    return mrDoDefault;
}

MsgRouting
AwtFrame::WmExitMenuLoop( BOOL isTrackPopupMenu )
{
    if ( !isTrackPopupMenu ) {
        m_isMenuDropped = FALSE;
    }
    return mrDoDefault;
}

AwtMenuBar*
AwtFrame::GetMenuBar()
{
    return menuBar;
}

void
AwtFrame::SetMenuBar( AwtMenuBar* mb )
{
    menuBar = mb;
    if ( mb == 0 ) {
        // Remove existing menu bar, if any.

        if ( m_extraHeight != 0 ) {
            RECT outside;
            ::GetWindowRect( GetHWnd(), &outside );

            JNIEnv *env;
            if ( JVM->AttachCurrentThread( (void **)&env, 0 ) != 0 ) {
                return;
            }
            jobject target = env->GetObjectField( GetPeer(),
                               WCachedIDs.PPCObjectPeer_targetFID );
            CHECK_NULL( target, "null target" );

//          m_ignoreWmSize = TRUE;
            ::MoveWindow( GetHWnd(),
                          outside.left,
                          outside.top,
                          outside.right - outside.left,
                          outside.bottom - m_extraHeight - outside.top,
                          TRUE );
//          m_ignoreWmSize = FALSE;
/*
            execute_java_dynamic_method(EE(), frame, "resize", "(II)V",
                                        outside.right - outside.left,
                                        outside.bottom - m_extraHeight -
                                        outside.top);
            ASSERT(!exceptionOccurred(EE()));
*/
            m_extraHeight = 0;
        }
    } else {
        if ( menuBar->GetHMenu() != NULL ) {
            // Save the size of original window and client rectangle
            RECT outside;
            RECT inside;
            ::GetWindowRect( GetHWnd(), &outside );
            ::GetClientRect( GetHWnd(), &inside );
            ::MapWindowPoints( GetHWnd(), 0, (LPPOINT)&inside, 2 );

            CriticalSection& l = menuBar->GetLock();
            l.Enter();

            m_cmdBar = wceSetMenuBar( m_hwnd, m_cmdBar, menuBar->GetHMenu() );

            // Get the size of the new client rectangle with the menu bar
            RECT insidewithmenu;
            ::GetClientRect( GetHWnd(), &insidewithmenu );
            ::MapWindowPoints( GetHWnd(), 0, (LPPOINT)&insidewithmenu, 2 );

            // If the top of the client rectangle intrudes below the
            // bottom of the window rectangle, we have no choice but to
            // make the window bigger so that insets are computed correctly.
            if ( insidewithmenu.top > inside.top && m_extraHeight == 0 ) {
                JNIEnv *env;
                if ( JVM->AttachCurrentThread( (void **)&env, 0 ) != 0 ) {
                    return;
                }
                jobject target = env->GetObjectField( GetPeer(),
                                        WCachedIDs.PPCObjectPeer_targetFID );
                CHECK_NULL( target, "null target" );
        
                RECT exposed = { outside.left, outside.bottom, outside.right,
                                 outside.bottom + insidewithmenu.top - 
                                 inside.top };

//              m_ignoreWmSize = TRUE;
                ::MoveWindow( GetHWnd(),
                              outside.left,
                              outside.top,
                              outside.right - outside.left,
                              exposed.bottom - outside.top,
                              TRUE);
//              m_ignoreWmSize = FALSE;

                m_extraHeight =  insidewithmenu.top - inside.top;

                l.Leave();
/*        
                execute_java_dynamic_method( EE(), frame, "resize", "(II)V",
                                             outside.right - outside.left,
                                             exposed.bottom - outside.top);
                ASSERT(!exceptionOccurred(EE()));
*/        
            }
            else {
              l.Leave();
            }
            g_bMenuLoop = TRUE;
        } // if (menuBar->GetHMenu() != NULL )
        else {
          g_bMenuLoop = FALSE;
        }
    }
    return;
}

MsgRouting
AwtFrame::WmDrawItem( UINT ctrlId, DRAWITEMSTRUCT far& drawInfo )
{
    // if the item to be redrawn is the menu bar, then do it
    AwtMenuBar* awtMenubar = GetMenuBar();

    if ( drawInfo.CtlType == ODT_MENU && ( awtMenubar != NULL ) && 
        ( awtMenubar->GetHMenu() == (HMENU)drawInfo.hwndItem ) )
    {
        awtMenubar->DrawItem( drawInfo );
        return mrConsume;
    } 

    return AwtComponent::WmDrawItem(ctrlId, drawInfo);
}

MsgRouting
AwtFrame::WmMeasureItem( UINT ctrlId, MEASUREITEMSTRUCT far& measureInfo )
{
    AwtMenuBar* awtMenubar = GetMenuBar();
    if ( ( measureInfo.CtlType == ODT_MENU ) && ( awtMenubar != NULL ) ) {
        // AwtMenu instance is stored in itemData. Use it to check if this 
        // menu is the menu bar.
        AwtMenu * pMenu = (AwtMenu *) measureInfo.itemData;
        ASSERT( pMenu != NULL );
        if ( pMenu == awtMenubar )
        {
            HWND hWnd = GetHWnd();
            HDC hDC = ::GetDC( hWnd );
            ASSERT( hDC != NULL );
            awtMenubar->MeasureItem( hDC, measureInfo );
            VERIFY( ::ReleaseDC( hWnd, hDC ) );
            return mrConsume;
        }
    }
    return AwtComponent::WmMeasureItem( ctrlId, measureInfo );
}
  
void
AwtFrame::SetIcon(AwtImage *pImage)
{
#ifndef WINCE
    if ( ::IsIconic( GetHWnd() ) ) {
        ::InvalidateRect( GetHWnd(), NULL, TRUE );
    }
#endif
    if ( m_hIcon != NULL ) {
        // Don't verify DestroyIcon() as NT 4.0 returns 0 on success or failure
        ::DestroyIcon( m_hIcon );
        m_hIcon = NULL;
    }

    if ( pImage != NULL ) {
        // need AwtImage::ToIcon()
        // m_hIcon = pImage->ToIcon();

        if ( m_hIcon == NULL ) {
            AWT_TRACE( ( TEXT( "WFramePeer.setIcon: failed converting image to icon.\n" ) ) );
        }
    }

    // Save in a local variable, so the toolkit's icon doesn't get deleted.
    HICON hIcon = m_hIcon;

    if ( hIcon == NULL ) {
        // Get AWT icon.
        hIcon = AwtToolkit::GetInstance().GetAwtIcon();
    }

    // If hIcon is NULL, Windows will use the WNDCLASS icon
#ifndef WINCE
    ::PostMessage( GetHWnd(), WM_SETICON, ICON_BIG, (long)hIcon );
    ::PostMessage( GetHWnd(), WM_SETICON, ICON_SMALL, (long)hIcon );

#else // WINCE
    // icon does not appear on task bar
    ::SendMessage( GetHWnd(), WM_SETICON, ICON_SMALL,  (LPARAM)hIcon );
#endif // !WINCE

    // set icon on owned windows
#ifndef WINCE
    ::EnumThreadWindows(
        AwtToolkit::MainThread(),
        (WNDENUMPROC)OwnedSetIcon, 
            (LPARAM)this );
#endif // !WINCE
    return;
}

// set icon on windows owned by this one
BOOL CALLBACK
AwtFrame::OwnedSetIcon( HWND hWndDlg, LPARAM lParam )
{
    AwtFrame * frame = (AwtFrame *)lParam;
    HWND    hwndDlgOwner = ::GetWindow( hWndDlg, GW_OWNER );

    while ( hwndDlgOwner != NULL ) {
        if ( hwndDlgOwner == frame->GetHWnd() ) {
            break;
        }
        hwndDlgOwner = ::GetWindow( hwndDlgOwner, GW_OWNER );
    }

    if ( hwndDlgOwner == NULL ) {
        return TRUE;
    }
    

    // if owned window has an icon, change it to match the frame
    if ( ::SendMessage( hWndDlg, WM_GETICON, ICON_SMALL, NULL ) != NULL ) {
#ifndef WINCE
        ::PostMessage( hWndDlg, WM_SETICON, ICON_BIG, (long)frame->m_hIcon );
#endif // WINCE
        ::PostMessage( hWndDlg, WM_SETICON, ICON_SMALL, (long)frame->m_hIcon );
    }
    return TRUE;
}

LONG
AwtFrame::WinThreadExecProc(ExecuteArgs * args)
{
    switch( args->cmdId ) {
    case FRAME_SETMENUBAR:
    {
        jobject mbPeer = (jobject)args->param1;
        if ( mbPeer == 0 ) {
            return -1L;
        }
  
        JNIEnv *env;
        if ( JVM->AttachCurrentThread( (void **)&env, 0 ) != 0 ) {
            return -1L;
        }
        
        // cancel any currently dropped down menus
        if ( m_isMenuDropped ) {
            SendMessage( WM_CANCELMODE );
        }

        if ( mbPeer == NULL ) {
            // Remove existing menu bar, if any
            SetMenuBar( NULL );
        } else {
            AwtMenuBar* menuBar = PDATA( AwtMenuBar, mbPeer );
            SetMenuBar( menuBar );
        }

        if ( menuBar && IsWindowVisible( m_hwnd ) ) {
            // check for existence and visibilty or get an exception on 'CE
            DrawMenuBar();
        }
        
        break;
    }

    default:
        AwtWindow::WinThreadExecProc( args );
        break;
    }

    return 0L;
}

#ifdef WINCE
void
AwtFrame::TranslateToClient( int &x, int &y )
{
    // The command bar is included in window's CE's client area
    // so we add its height back in (so 0,0 ends up below it).
    if ( m_cmdBar) {
        y -= m_insets.top - wceGetCommandBarHeight(m_cmdBar);
    } else {
	y -= m_insets.top;
    }
    x -= m_insets.left;
    return;
}

void
AwtFrame::TranslateToClient( long &x, long &y )
{
    /* The command bar is included in window's CE's client area */
    /* so we add it's height back in (so 0,0 ends up below it) */
    if ( m_cmdBar ) {
        y -= m_insets.top - wceGetCommandBarHeight( m_cmdBar );
    } else {
	y -= m_insets.top;
    }
    x -= m_insets.left;
    return;
}
 
#endif // WINCE

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCFramePeer_setMenuBarNative( JNIEnv *env,
                                                     jobject self,
                                                     jobject mbPeer )
{
    CHECK_PEER( self )
    AwtObject::WinThreadExec( self, FRAME_SETMENUBAR, (LPARAM)mbPeer );
    /*
    AwtFrame* frame = PDATA( AwtFrame, self );
    if ( mbPeer == NULL ) {
        // Remove existing menu bar, if any.
	frame->SetMenuBar( NULL );
    } else {
        CHECK_PEER( mbPeer );
        AwtMenuBar *menuBar = PDATA( AwtMenuBar, mbPeer );
	frame->SetMenuBar( menuBar );
    }
    frame->DrawMenuBar();
    return;
    */
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCFramePeer_initIDs (JNIEnv *env, jclass cls)
{
    cls = env->FindClass( "sun/awt/EmbeddedFrame" );
    if ( cls == NULL ) {
        return;
    }
    GET_METHOD_ID( sun_awt_EmbeddedFrame_isCursorAllowedMID, "isCursorAllowed",
                   "()Z" );
    return;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCFramePeer_create( JNIEnv *env, jobject self,
                                           jobject hParent )
{
    AwtToolkit::CreateComponent( self, hParent,
                           (AwtToolkit::ComponentFactory)AwtFrame::Create );
    CHECK_PEER_CREATION( self );
    return;
}

// Fix for for 1251439: like WComponentPeer.reshape, but checks
// SM_C[XY]MIN and enforces these tresholds.  The additional code is
// marked as such.
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCFramePeer_reshape( JNIEnv *env,
                                            jobject self,
                                            jint x, jint y,
                                            jint w, jint h )
{
    CHECK_PEER( self );
    AwtComponent* p = PDATA( AwtComponent, self );
    RECT* r = new RECT;
    // enforce tresholds before posting the event
    jobject target = p->GetTarget();
    CHECK_NULL( target, "null target" );
#ifndef WINCE
    int minWidth = ::GetSystemMetrics( SM_CXMIN );
    int minHeight = ::GetSystemMetrics( SM_CYMIN );
#else
    int minHeight = ::GetSystemMetrics( SM_CYCAPTION ) + 6;
    int minWidth = minHeight;
#endif
    if ( w < minWidth || h < minHeight ) {
        w = MAX( w, minWidth );
        h = MAX( h, minHeight );
	env->SetIntField(target,
			 WCachedIDs.java_awt_Component_widthFID, w);
	env->SetIntField(target,
			 WCachedIDs.java_awt_Component_heightFID, h);
    }
    // end of fix proper
    ::SetRect( r, x, y, x + w, y + h );
    ::PostMessage( p->GetHWnd(), WM_AWT_RESHAPE_COMPONENT, 0, (LPARAM)r );
    return;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCFramePeer__1setIconImage( JNIEnv *env,
                                                  jobject self,
                                                  jobject ir )
{
    CHECK_PEER( self );
    CHECK_NULL( ir, "null ImageRepresentation" );

    AwtFrame *frame = PDATA( AwtFrame, self );
    if ( ir == NULL ) {
        frame->SetIcon( NULL );
    } else {
        // If GetpImage fails (bad graphic file, for instance), pImage will
        // be NULL.  This gets passed through anyway, because NULL causes
        // the class's icon to be reset to the Windows default.
        // GetpImage currently unimplemented
        //AwtImage *pImage = AwtImage::GetpImage( ir );
        AwtImage *pImage = 0;
        frame->SetIcon( pImage );
    }
    return;
}
