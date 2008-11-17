/*
 * @(#)PPCTextAreaPeer.cpp	1.15 06/10/10
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
#include <string.h>

#include "jni.h"
#include "java_awt_TextArea.h"
#include "sun_awt_pocketpc_PPCTextAreaPeer.h"
#include "PPCTextAreaPeer.h"
#include "PPCTextComponentPeer.h"
#include "PPCGraphics.h"
#include "PPCCanvasPeer.h"

/**
 * Constructor
 */ 
AwtTextArea :: AwtTextArea()  //: AwtTextComponent() 
{

}

// Create a new AwtTextArea object and window.  
AwtTextArea* AwtTextArea::Create(jobject peer,
                                 jobject hParent)
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

    AwtTextArea* c = new AwtTextArea();
    CHECK_NULL_RETURN(c, "AwtTextArea() failed");

    // Adjust style for scrollbar visibility and word wrap  
    DWORD scroll_style;

    int scrollbarVisibility = (int) env->CallIntMethod(target, WCachedIDs.java_awt_TextArea_getScrollbarVisibilityMID);

    switch (scrollbarVisibility) {
    case java_awt_TextArea_SCROLLBARS_NONE:
        scroll_style = ES_AUTOVSCROLL;
        break;
    case java_awt_TextArea_SCROLLBARS_VERTICAL_ONLY:
        scroll_style = WS_VSCROLL | ES_AUTOVSCROLL;
        break;
    case java_awt_TextArea_SCROLLBARS_HORIZONTAL_ONLY:
        scroll_style = WS_HSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL;
        break;
    case java_awt_TextArea_SCROLLBARS_BOTH:
        scroll_style = WS_VSCROLL | WS_HSCROLL | 
            ES_AUTOVSCROLL | ES_AUTOHSCROLL;
        break;
    }

    DWORD style = WS_CHILD | WS_CLIPSIBLINGS | ES_LEFT | ES_MULTILINE | 
                  ES_WANTRETURN | ES_NOHIDESEL | scroll_style | 
                  (IS_WIN4X ? 0 : WS_BORDER);
    DWORD exStyle = IS_WIN4X ? WS_EX_CLIENTEDGE : 0;

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

// Count how many '\n's are there in jStr
int AwtTextArea::CountNewLines(jstring jStr, int maxlen) 
{ 
    int nNewlines = 0;
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return NULL;
    }
    WCHAR* string = TO_WSTRING(jStr);
    for (int i=0; i < maxlen && i < env->GetStringLength(jStr); i++) {
        if (string[i] == L'\n') {
            nNewlines++;
        }
    }
    return nNewlines;
}


/**
 * JNI Functions
 */

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCTextAreaPeer_initIDs (JNIEnv *env, jclass cls)
{
    GET_METHOD_ID (PPCTextAreaPeer_replaceText, "replaceText", 
                   "(Ljava/lang/String;II)V");
}


JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCTextAreaPeer_create (JNIEnv *env, jobject thisObj, 
                                              jobject hParent )
{
    CHECK_PEER(hParent);
    AwtToolkit::CreateComponent(thisObj, hParent, (AwtToolkit::ComponentFactory)
                                AwtTextArea::Create);

    CHECK_PEER_CREATION(thisObj);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCTextAreaPeer_replaceText (JNIEnv *env, 
                                                   jobject self,
                                                   jstring text, 
                                                   jint start,jint end)
{
    CHECK_PEER(self);
    CHECK_NULL(text, "null string");

    AwtTextComponent* c = PDATA(AwtTextComponent, self);
    jstring targetText = Java_sun_awt_pocketpc_PPCTextComponentPeer_getText(env,
                                                                            self);
#if defined(NETSCAPE)
	//Check for an exception above.  If yes don't go any further.
    if (exceptionOccurred(EE())) return;
#endif // Netscape.	
        
    // Need to be a little bit more careful about what values are
    // allowed for `start' and `end'. In the original implementation,
    // these were allowed to come through as arbitrary values, and
    // large ones would cause the buffer allocation in the Win95 code 
    // to crash. Here, we truncate their values to be at most the size 
    // of the text currently contained in the text area.
    int nLen = env->GetStringLength(targetText);
    start = (nLen < start) ? nLen : start;
    end = (nLen < end) ? nLen : end;

    // Adjust positions with the CRs going to be added.
    int cr_start = AwtTextArea::CountNewLines(targetText, start);
    int cr_end = AwtTextArea::CountNewLines(targetText, end);

    // After wrapper functions are made, it must be still remained to
    // distinguish Unicode TextArea (NT) and multibyte TextArea (95).  For 
    // multibyte TextArea, the start and end position must be recalculated.
#ifndef WINCE
    if (IS_NT) {
#endif
	start += cr_start;
	end += cr_end;

        int length = env->GetStringLength(text)+1;
        WCHAR* buffer = new WCHAR[length];
        env->GetStringRegion(text, 0, length-1, (jchar*)buffer);
        buffer[length-1] = L'\0';

        buffer = AwtTextComponent::AddCR(buffer, length);
        c->SendMessageW(EM_SETSEL, start, end);
        c->SendMessageW(EM_REPLACESEL, FALSE, (LPARAM)buffer);
        delete[] buffer;
#ifndef WINCE
    } else {
#ifdef WIN32
	ASSERT(IS_WIN95);
#endif
	// If the string in the target text area is multibyte,
	// the start and end position must be recalculate to get
	// the right positions.
        WCHAR* wStartbuffer = new WCHAR[start + 1];
        env->GetStringRegion(targetText, 0, start, wStartbuffer);
        wStartbuffer[start] = L'\0';

        WCHAR* wEndbuffer = new WCHAR[end + 1];
        env->GetStringRegion(targetText, 0, end, wEndbuffer);
        wEndbuffer[end] = L'\0';

        char* aStartbuffer = W2A(wStartbuffer);
	char* aEndbuffer = W2A(wEndbuffer);
	start = strlen(aStartbuffer) + cr_start;
	end = strlen(aEndbuffer) + cr_end;

	delete[] wStartbuffer;
	delete[] wEndbuffer;

        // Copy Java string into a buffer AddCR can replace if necessary.
	int length = env->GetStringLength(text);
        char* buffer = new char[length * 2 + 1];
        
        javaString2multibyte(text, buffer, length);
        buffer = AwtTextComponent::AddCR(buffer, length);
        
        c->SendMessage(EM_SETSEL, start, end);
        c->SendMessage(EM_REPLACESEL, FALSE, (LPARAM)buffer);

        delete[] buffer;
    }
    #endif /* ! WINCE */
}
