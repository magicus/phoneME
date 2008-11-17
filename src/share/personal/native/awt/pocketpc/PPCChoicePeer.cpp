/*
 * @(#)PPCChoicePeer.cpp	1.20 06/10/10
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
#include "sun_awt_pocketpc_PPCChoicePeer.h"
#include "PPCChoicePeer.h"
#include "PPCCanvasPeer.h"

AwtChoice::AwtChoice()
  : AwtComponent()
{
}

AwtChoice::~AwtChoice()
{
}

const TCHAR*
AwtChoice::GetClassName()
{
    return TEXT( "COMBOBOX" ); 
}

// Create a new AwtChoice object and window.  
AwtChoice*
AwtChoice::Create( jobject peer, jobject hParent )
{
    JNIEnv *env;
    if ( JVM->AttachCurrentThread( (void **)&env, 0 ) != 0 ) {
        return 0;
    }
    CHECK_NULL_RETURN( hParent, "null hParent" );
    AwtCanvas *parent = PDATA( AwtCanvas, hParent );
    CHECK_NULL_RETURN( parent, "null parent" );
    jobject target = env->GetObjectField( peer,
                                          WCachedIDs.PPCObjectPeer_targetFID );
    CHECK_NULL_RETURN( target, "null target" );

    AwtChoice *c = new AwtChoice();
    CHECK_NULL_RETURN( c, "AwtChoice() failed" );

#ifndef WINCE
    DWORD style = WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL | 
        CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED;
#else
    // WINCE does not support OWNERDRAW combo boxes.
    DWORD style = WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL | 
        CBS_DROPDOWNLIST;
#endif // !WINCE

    // In OWNER_DRAW, the size of the edit control part of the choice
    // must be determinded in its creation, when the parent cannot get
    // the choice's instance from its handle.  So record the pair of 
    // the ID and the instance of the choice.
    UINT myId = parent->CreateControlID();
    ASSERT( myId > 0 );
    c->m_myControlID = myId;
    parent->PushChild( myId, c );

    c->CreateHWnd( TEXT(""), style, 0,
                   env->CallIntMethod( target,
                                WCachedIDs.java_awt_Component_getXMID ), 
                   env->CallIntMethod( target,
				WCachedIDs.java_awt_Component_getYMID ),
                   env->CallIntMethod( target,
				WCachedIDs.java_awt_Component_getWidthMID ),
                   env->CallIntMethod( target,
				WCachedIDs.java_awt_Component_getHeightMID ),
                   parent->GetHWnd(),
                   (HMENU)myId,
                   ::GetSysColor( COLOR_WINDOWTEXT ),
                   ::GetSysColor( COLOR_WINDOW ),
                   peer
                   );
    c->m_backgroundColorSet = TRUE;  // suppress inheriting parent's color.
    return c;
}

// calculate height of drop-down list part of the combobox
// to show all the items up to a maximum of eight
int
AwtChoice::GetDropDownHeight()
{
    int itemHeight =(int)::SendMessage( GetHWnd(),
                                        CB_GETITEMHEIGHT, (UINT)0, 0 );
    int numItemsToShow = (int)::SendMessage( GetHWnd(), CB_GETCOUNT, 0, 0 );
    numItemsToShow = numItemsToShow > 8 ? 8 : numItemsToShow;
    // drop-down height snaps to nearest line, so add a 
    // fudge factor of 1/2 line to ensure last line shows
    return itemHeight*numItemsToShow + itemHeight/2;
}

// get the height of the field portion of the combobox
int
AwtChoice::GetFieldHeight()
{
    int fieldHeight = 0;
    int borderHeight;
    fieldHeight =(int)::SendMessage( GetHWnd(), CB_GETITEMHEIGHT,
                                     (UINT)-1, 0 );
    // add top and bottom border lines; border size is different for
    // Win 4.x (3d edge) vs 3.x (1 pixel line)
#ifndef WINCE
    borderHeight = ::GetSystemMetrics( IS_WIN4X ? SM_CYEDGE : SM_CYBORDER );
#else
    borderHeight = ::GetSystemMetrics( SM_CYEDGE );
#endif // !WINCE
    fieldHeight += ( borderHeight * 2);
    return fieldHeight;
}

// gets the total height of the combobox, including drop down
int
AwtChoice::GetTotalHeight()
{
    int dropHeight = GetDropDownHeight();
    int fieldHeight = GetFieldHeight();

    // border on drop-down portion is always non-3d (so don't use SM_CYEDGE)
    int borderHeight = ::GetSystemMetrics( SM_CYBORDER );
    // total height = drop down height + field height + top +
    //                bottom drop down border lines 
    return dropHeight + fieldHeight + borderHeight * 2;
}

// Recalculate and set the drop-down height for the Choice.
void
AwtChoice::ResetDropDownHeight()
{
    JNIEnv *env;
    if ( JVM->AttachCurrentThread( (void **)&env, 0 ) != 0 ) {
        return;
    }
    jobject target = env->GetObjectField( GetPeer(),
                                          WCachedIDs.PPCObjectPeer_targetFID );
    CHECK_NULL( target, "null target" );
    RECT rcWindow;

    ::GetWindowRect( GetHWnd(), &rcWindow );
    // resize the drop down to accomodate added/removed items
    ::SetWindowPos( GetHWnd(), NULL,
		    0, 0, rcWindow.right - rcWindow.left, GetTotalHeight(),
		    SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER );
    return;
}

void
AwtChoice::Reshape( int x, int y, int w, int h )
{
    // Choice component height is fixed (when rolled up)
    // so vertically center the choice in it's bounding box
    int fieldHeight = GetFieldHeight();
    if ( fieldHeight > 0 && fieldHeight < h ) {
        y += (h - fieldHeight) / 2;
    }
    AwtComponent::Reshape( x, y, w, GetTotalHeight() );
    return;
}

//Netscape : Override the AwtComponent method so we can set the item height
//for the editfield and for each item in the list.
void
AwtChoice::SetFont( AwtFont* font )
{
    ASSERT( font != NULL );
    if ( font->GetAscent() < 0 ) {
        AwtFont::SetupAscent( font );
    }
    HANDLE hFont = font->GetHFont();
    SendMessage( WM_SETFONT, (WPARAM)hFont, MAKELPARAM( FALSE, 0 ) );

    // Get the text metrics and change the height of each item.
    HDC hDC = ::GetDC( GetHWnd() );
    ASSERT( hDC != NULL );
    TEXTMETRIC tm;
	
    VERIFY( ::SelectObject( hDC, hFont ) != NULL );
    VERIFY( ::GetTextMetrics( hDC, &tm ) );

    long h = tm.tmHeight + tm.tmExternalLeading;
    
    VERIFY( ::ReleaseDC( GetHWnd(), hDC ) != 0 );
#ifndef WINCE
    int nCount = (int)::SendMessage( GetHWnd(), CB_GETCOUNT, 0, 0 );
    for( int i=0 ; i< nCount ; ++i ) {
        VERIFY( ::SendMessage( GetHWnd(), CB_SETITEMHEIGHT, i,
                               MAKELPARAM( h, 0 ) ) != CB_ERR);
    }

    h += AwtToolkit::sm_nComboHeightOffset;

    // Change the height of the Edit Box.
    VERIFY( ::SendMessage( GetHWnd(), CB_SETITEMHEIGHT, (UINT)-1,
                           MAKELPARAM( h, 0 ) ) != CB_ERR );
#else
    // Change the height of all the items */
    VERIFY( ::SendMessage(GetHWnd(), CB_SETITEMHEIGHT, 0,
                          MAKELPARAM( h, 0 ) ) != CB_ERR);
    // WinCE can't change the height of the Edit Box 
    // without effecting the items
#endif // !WINCE

    // notify Java that the height has changed
    JNIEnv *env;
    if ( JVM->AttachCurrentThread( (void **)&env, 0 ) != 0 ) {
        return;
    }
    jobject peer = GetPeer();
    jobject target = env->GetObjectField( peer,
                                          WCachedIDs.PPCObjectPeer_targetFID );
    CHECK_NULL( target, "null target" );

    // Is this a good idea? Originally, call
    // resize( int, int ). This is deprecated and replaced
    // by setSize( int, int ) so use that if we want anything
    // at all. Will there be side-effects as we saw in our
    // original porting?
    env->CallVoidMethod( target,
                         WCachedIDs.java_awt_Component_setSizeMID,
                         env->CallIntMethod( target,
                                  WCachedIDs.java_awt_Component_getWidthMID ),
                         h + 6 );
    ASSERT( !env->ExceptionCheck() );
    return;
}

jobject
AwtChoice::PreferredItemSize(JNIEnv* env)
{
    jobject peer = GetPeer();
    jobject target = env->GetObjectField( peer,
                                          WCachedIDs.PPCObjectPeer_targetFID);
    CHECK_NULL_RETURN( target, "null target" );
    jobject dimension = env->CallObjectMethod( target,
                             WCachedIDs.java_awt_Component_getPreferredSizeMID
                                             );
    ASSERT( !env->ExceptionCheck() );

    // This size is window size of choice and it's too big for
    // each drop down item height.
    // Rather than invoking the setSize() method, just
    // set the height field directly.
    env->SetIntField( dimension,
                      WCachedIDs.java_awt_Dimension_heightFID,
                      GetFontHeight() );
    return dimension;
}

MsgRouting
AwtChoice::WmNotify( UINT notifyCode )
{
    if ( notifyCode == CBN_SELCHANGE ) {
        JNIEnv *env;
        if ( JVM->AttachCurrentThread( (void **)&env, 0 ) != 0 ) {
          return mrDoDefault; // ?
        }
        env->CallVoidMethod( GetPeer(),
                             WCachedIDs.PPCChoicePeer_handleActionMID,
                             SendMessage( CB_GETCURSEL ) );
        ASSERT( !env->ExceptionCheck() );
    }
    return mrDoDefault;
}

MsgRouting
AwtChoice::WmDrawItem( UINT /*ctrlId*/, DRAWITEMSTRUCT far& drawInfo )
{
    DrawListItem( drawInfo );
    return mrConsume;
}

MsgRouting
AwtChoice::WmMeasureItem( UINT /*ctrlId*/, MEASUREITEMSTRUCT far& measureInfo )
{
    MeasureListItem( measureInfo );
    return mrConsume;
}

/////////////////////////////////////////////////////////////////////////////
// Diagnostic routines
#ifdef DEBUG

void
AwtChoice::VerifyState()
{
    if ( AwtToolkit::GetInstance().VerifyComponents() == FALSE ) {
        return;
    }

    if ( m_callbacksEnabled == FALSE ) {
        // Component is not fully setup yet.
        return;
    }

    AwtComponent::VerifyState();

    // Compare number of items.
    
    JNIEnv *env;
    if ( JVM->AttachCurrentThread( (void **)&env, 0 ) != 0 ) {
        return;
    }
    jobject target = GetTarget();

    int nTargetItems = env->CallIntMethod( target,
                            WCachedIDs.java_awt_Choice_getItemCountMID );
    int nPeerItems = (int)::SendMessage( GetHWnd(), CB_GETCOUNT, 0, 0 );
    ASSERT( nTargetItems == nPeerItems );

    // Compare selection
    int targetIndex = env->CallIntMethod( target,
                           WCachedIDs.java_awt_Choice_getSelectedIndexMID );
    ASSERT( !env->ExceptionCheck() );
    int peerCurSel = (int)::SendMessage( GetHWnd(), CB_GETCURSEL, 0, 0 );
    ASSERT( targetIndex == peerCurSel );
    return;
}
#endif //DEBUG

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCChoicePeer_initIDs( JNIEnv *env, jclass cls )
{
    GET_METHOD_ID( PPCChoicePeer_handleActionMID,
                   "handleAction", "(I)V" );
    return;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCChoicePeer_create( JNIEnv *env,
                                            jobject self,
                                            jobject hParent )
{
    CHECK_PEER( hParent );
    AwtToolkit::CreateComponent( self, hParent,
                      (AwtToolkit::ComponentFactory)AwtChoice::Create );
    CHECK_PEER_CREATION( self );
    return;
}


JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCChoicePeer_addItem( JNIEnv *env,
                                             jobject self,
                                             jstring itemText,
                                             jint index )
{
    CHECK_PEER( self );
    CHECK_NULL( itemText, "null itemText" );

    AwtChoice *c = PDATA( AwtChoice, self );
    c->SendMessage( CB_INSERTSTRING, index, JavaStringBuffer( env, itemText ) );
    c->ResetDropDownHeight();
#ifdef DEBUG
    c->VerifyState();
#endif // DEBUG
    return;
}


JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCChoicePeer_remove( JNIEnv *env,
                                            jobject self,
                                            jint index )
{
    CHECK_PEER( self );
    AwtChoice* c = PDATA( AwtChoice, self );
    c->SendMessage( CB_DELETESTRING, index, 0 );
    c->ResetDropDownHeight();
#ifdef DEBUG
    c->VerifyState();
#endif // DEBUG
    return;
}


JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCChoicePeer_select( JNIEnv *env,
                                            jobject self,
                                            jint index )
{
    CHECK_PEER( self );
    AwtChoice *c = PDATA( AwtChoice, self );
    c->SendMessage( CB_SETCURSEL, index );
    return;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCChoicePeer_reshape( JNIEnv *env,
                                             jobject self,
                                             jint x,
                                             jint y,
                                             jint width,
                                             jint height )
{
    CHECK_PEER( self );
    AwtChoice *c = PDATA( AwtChoice, self );
    c->Reshape( x, y, width, height );
    c->VerifyState();
    return;
}
