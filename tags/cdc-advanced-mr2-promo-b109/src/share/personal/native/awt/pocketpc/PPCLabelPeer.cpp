/*
 * @(#)PPCLabelPeer.cpp	1.9 06/10/10
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
#include "java_awt_Label.h"
#include "sun_awt_pocketpc_PPCLabelPeer.h"
#include "PPCCanvasPeer.h"
#include "PPCLabelPeer.h"
#include "PPCGraphics.h"

/*
 * Class methods
 */
AwtLabel::AwtLabel()
    : AwtComponent()
{
}

AwtLabel::~AwtLabel()
{
}

const TCHAR*
AwtLabel::GetClassName()
{
    return TEXT( "LABEL" );
}

AwtLabel* AwtLabel::Create( jobject self,
                            jobject hParent )
{
    AwtLabel *label = 0;

    JNIEnv *env;
    if ( JVM->AttachCurrentThread( (void **)&env, 0 ) != 0 ) {
        return 0;
    }
    CHECK_NULL_RETURN( hParent, "null hParent" );
    AwtCanvas *parent = PDATA( AwtCanvas, hParent );
    CHECK_NULL_RETURN( parent, "null parent" );
    jobject target = env->GetObjectField( self,
                          WCachedIDs.PPCObjectPeer_targetFID );
    CHECK_NULL_RETURN(target, "null target" );

    label = new AwtLabel();
    CHECK_NULL_RETURN( label, "AwtLabel() failed" );

    DWORD style = WS_CHILD | WS_CLIPSIBLINGS;

    label->CreateHWnd( TEXT(""), style, 0,
                       env->CallIntMethod( target,
                                  WCachedIDs.java_awt_Component_getXMID ),
                       env->CallIntMethod( target,
                                  WCachedIDs.java_awt_Component_getYMID ),
                       env->CallIntMethod( target,
                                  WCachedIDs.java_awt_Component_getWidthMID ),
                       env->CallIntMethod( target,
                                  WCachedIDs.java_awt_Component_getHeightMID ),
                       parent->GetHWnd(),
                       NULL,
                       ::GetSysColor( COLOR_WINDOWTEXT ),
                       ::GetSysColor( COLOR_BTNFACE ),
                       self
                       );
    return label;
}

// return true if caller should post a paint event back to java
BOOL
AwtLabel::DoPaint( HDC hDC, RECT& r )
{
    JNIEnv *env;
    if ( JVM->AttachCurrentThread( (void **)&env, 0 ) != 0 ) {
        return FALSE;
    }

    if ( (r.right-r.left ) > 0 && ( r.bottom-r.top ) > 0 &&
        m_peerObject != NULL && m_callbacksEnabled ) {

        long x,y;
        SIZE size;
        jobject javaFont;
        jstring javaString;
        jobject self = GetPeer();
        jobject target = env->GetObjectField( self,
                              WCachedIDs.PPCObjectPeer_targetFID );
        CHECK_NULL_RETURN(target, "null target" );

        javaFont = GET_FONT( target );
        javaString = (jstring)env->CallObjectMethod( target,
                                       WCachedIDs.java_awt_Label_getTextMID);

        size = AwtFont::getMFStringSize( hDC, javaFont, javaString );
    
        ::SetTextColor( hDC, GetColor() );

        // Redraw whole label to eliminate display noise during resizing.
        VERIFY( ::GetClientRect( GetHWnd(), &r ) );
        VERIFY( ::FillRect ( hDC, &r, GetBackgroundBrush() ) );
	
        y = ( r.top + r.bottom - size.cy ) / 2;
	
	jint alignment = env->CallIntMethod( target,
					     WCachedIDs.java_awt_Label_getAlignmentMID );
	
        switch (alignment) {
          case java_awt_Label_LEFT:
              x = 0;
              break;
          case java_awt_Label_CENTER:
              x = (r.left + r.right - size.cx) / 2;
              break;
          case java_awt_Label_RIGHT:
              x = r.right - size.cx;
              break;
        }

        //draw string
        if ( isEnabled() ) {
            AwtComponent::DrawWindowText( hDC, javaFont, javaString, x, y );
        } else {
            AwtComponent::DrawGrayText( hDC, javaFont, javaString, x, y );
        }

        return TRUE;
    }
    return FALSE;
}

MsgRouting
AwtLabel::WmPaint( HDC /* ignored */ )
{
    CriticalSection::Lock l( GetLock() );
    GetSharedDC()->Lock();
    GetSharedDC()->GrabDC( NULL );

    PAINTSTRUCT ps;
    HDC hDC = ::BeginPaint( GetHWnd(), &ps );// the passed-in HDC is ignored.
    ASSERT( hDC );
    RECT& r = ps.rcPaint;
    BOOL postPaint = DoPaint( hDC, r );
    // Netscape removed the next line for Bug # 81723
    // GetSharedDC()->Unlock();

    // if DoPaint indicated, callback here, now that the dc is unlocked
    if ( postPaint ) {
	JNIEnv *env;
	if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	    GetSharedDC()->Unlock();
	    return mrPassAlong;
	}
	
	env->CallVoidMethod(GetPeer(),
			    WCachedIDs.PPCComponentPeer_handlePaintMID,
			    r.left,
			    r.top,
			    r.right-r.left,
			    r.bottom-r.top);
    }
    
    VERIFY( ::EndPaint( GetHWnd(), &ps ) );
    //Netscape : Bug # 81723. Integrated by Lara Bunni. Should have 
    //moved the unlock also.
    //GDI calls were failing and were reporting invalid DC as
    //the error.  This was due to multiple threads racing for the 
    //use of an unprotected DC.
    GetSharedDC()->Unlock();

    return mrConsume;
}

MsgRouting
AwtLabel::WmPrintClient( HDC hDC, UINT )
{
    RECT r;

    // obtain valid DC from GDI stack
    ::RestoreDC( hDC, -1 );

    ::GetClipBox( hDC, &r );
    DoPaint( hDC, r );
    return mrConsume;
}

/**
 * JNI Functions
 */
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCLabelPeer_initIDs( JNIEnv *env, jclass cls )
{
    cls = env->FindClass( "java/awt/Label" );
    if ( cls == NULL )
        return;

    GET_METHOD_ID( java_awt_Label_getAlignmentMID, "getAlignment",
		   "()I" );
    GET_METHOD_ID( java_awt_Label_getTextMID, "getText",
		   "()Ljava/lang/String;" );
    return;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCLabelPeer_create( JNIEnv *env, jobject self,
                                           jobject hParent )
{
    CHECK_PEER( hParent );
    AwtToolkit::CreateComponent( self, hParent,
                         (AwtToolkit::ComponentFactory)AwtLabel::Create );
    CHECK_PEER_CREATION( self );
    return;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCLabelPeer_setText( JNIEnv *env,
                                            jobject self,
                                            jstring text )
{
    CHECK_PEER(self);
    AwtComponent *c = PDATA( AwtComponent, self );
    c->SetText( JavaStringBuffer( env, text ) );
    VERIFY( ::InvalidateRect( c->GetHWnd(), NULL, TRUE ) );
    return;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCLabelPeer_setAlignment( JNIEnv *env,
                                                 jobject self,
                                                 jint alignment )
{
    CHECK_PEER(self);
    // alignment of multifont label is referred to in WmDrawItem method
    AwtComponent *c = PDATA( AwtComponent, self );
    VERIFY( ::InvalidateRect( c->GetHWnd(), NULL, TRUE ) );
    return;
}
