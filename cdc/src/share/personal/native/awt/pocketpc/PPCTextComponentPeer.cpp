/*
 * @(#)PPCTextComponentPeer.cpp	1.19 06/10/10
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
#include "sun_awt_pocketpc_PPCTextComponentPeer.h"
#include "PPCTextComponentPeer.h"


/**
 * Constructor
 */

AwtTextComponent ::AwtTextComponent()  //      : AwtComponent()
{
}

const TCHAR* AwtTextComponent::GetClassName() {
    return TEXT("EDIT");  // System provided edit control class
}

// Set a suitable font to IME against the component font.
void AwtTextComponent::SetFont(AwtFont* font)
{
    ASSERT(font != NULL);
    if (font->GetAscent() < 0) {
	AwtFont::SetupAscent(font);
    }

    int index = font->GetInputHFontIndex();
    if (index < 0)
        index = 0; //In this case, user cannot get any suitable font for input.

    SendMessage(WM_SETFONT, (WPARAM)font->GetHFont(index), MAKELPARAM(FALSE, 0));
    VERIFY(::InvalidateRect(GetHWnd(), NULL, TRUE));
}

// Returns a copy of pStr where \n have been converted to \r\n.
// If the result != 'pStr', 'pStr' has been freed.

WCHAR *AwtTextComponent::AddCR(WCHAR *pStr, int nStrLen)
{
    int nNewlines = 0;
    int i, nResultIx, nLen;
    WCHAR *result;

    // check to see if there are any LF's
    if (wcschr(pStr, L'\n') == NULL) {
        return pStr;
    }

    // count newlines
    for (i=0; pStr[i] != 0; i++) {
        if (pStr[i] == L'\n') {
            nNewlines++;
        }
    }
    nLen = i;

    if (nLen + nNewlines + 1 > nStrLen) {
        result = new WCHAR[nLen + nNewlines + 1];
    } else {
        result = pStr;
    }
    nResultIx = nLen + nNewlines;
    for (i=nLen; i>=0; i--) {
        result[nResultIx--] = pStr[i];
        if (pStr[i] == L'\n') {
            result[nResultIx--] = L'\r';
        }
    }
    ASSERT(nResultIx == -1);

    if (pStr != result) {
        // We realloc'd, so delete old buffer.
        delete[] pStr;
    }
    return result;
}

char *AwtTextComponent::AddCR(char *pStr, int nStrLen)
{
    int nNewlines = 0;
    int i, nResultIx, nLen;
    char *result;

    // check to see if there are any LF's
    if (strchr(pStr, '\n') == NULL) {
        return pStr;
    }

    // count newlines
    for (i=0; pStr[i] != 0; i++) {
        if (pStr[i] == '\n') {
            nNewlines++;
        }
    }
    nLen = i;

    if (nLen + nNewlines + 1 > nStrLen) {
        result = new char[nLen + nNewlines + 1];
    } else {
        result = pStr;
    }
    nResultIx = nLen + nNewlines;
    for (i=nLen; i>=0; i--) {
        result[nResultIx--] = pStr[i];
        if (pStr[i] == '\n') {
            result[nResultIx--] = '\r';
        }
    }
    ASSERT(nResultIx == -1);

    if (pStr != result) {
        // We realloc'd, so delete old buffer.
        delete[] pStr;
    }
    return result;
}

// Converts \r\n pairs to \n.
int AwtTextComponent::RemoveCR(WCHAR *pStr)
{
    int i, nLen = 0;

    if (pStr) {
        // check to see if there are any CR's
        if (wcschr(pStr, L'\r') == NULL) {
            return wcslen(pStr);
        }

        for (i=0; pStr[i] != 0; i++) {
            if (pStr[i] == L'\r' && pStr[i + 1] == L'\n') {
                i++;
            }
            pStr[nLen++] = pStr[i];
        }
        pStr[nLen] = 0;
    }
    return nLen;
}

/* This routine is safe for converting multibyte strings, because
 * control characters such as \n and \r are never part of a multibyte
 * character sequence. */
int AwtTextComponent::RemoveCR(char *pStr)
{
    int i, nLen = 0;

    if (pStr) {
        // check to see if there are any CR's
        if (strchr(pStr, '\r') == NULL) {
            return strlen(pStr);
        }

        for (i=0; pStr[i] != 0; i++) {
            if (pStr[i] == '\r' && pStr[i + 1] == '\n') {
                i++;
            }//////////;;;;/
            pStr[nLen++] = pStr[i];
        }
        pStr[nLen] = 0;
    }
    return nLen;
}

MsgRouting
AwtTextComponent::WmNotify(UINT notifyCode)
{
    if (notifyCode == EN_CHANGE) {
        JNIEnv *env;
        if (JVM->AttachCurrentThread((void **) &env, 0) == 0) {
            env->CallVoidMethod(GetPeer(), 
                                WCachedIDs.PPCTextComponentPeer_valueChangedMID);
        }
    }
    return mrDoDefault;
}


/**
 * JNI Functions
 */
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCTextComponentPeer_initIDs (JNIEnv *env, jclass cls)
{
    GET_METHOD_ID(PPCTextComponentPeer_valueChangedMID, "valueChanged", "()V");
    cls = env->FindClass( "java/awt/TextArea" );
    if ( cls == NULL )
        return;
    WCachedIDs.java_awt_TextAreaClass = (jclass) env->NewGlobalRef (cls);
    GET_METHOD_ID( java_awt_TextArea_getScrollbarVisibilityMID, 
                   "getScrollbarVisibility", "()I");
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCTextComponentPeer_enableEditing (JNIEnv *env, 
                                                          jobject thisObj,
                                                          jboolean editable)
{
    CHECK_PEER(thisObj);
    AwtTextComponent* c = PDATA(AwtTextComponent, thisObj);
    c->SendMessage(EM_SETREADONLY, !editable);
}

JNIEXPORT jstring JNICALL
Java_sun_awt_pocketpc_PPCTextComponentPeer_getText (JNIEnv *env, 
                                                    jobject thisObj)
{
    CHECK_PEER_RETURN(thisObj);
    int len;
    AwtTextComponent* c = PDATA(AwtTextComponent, thisObj);

    len = c->GetTextLength();
    jstring js;
    if (len == 0) {
        // Make java null string
        jchar *jc = new jchar[0];
	js = env->NewString(jc, 0);
	delete [] jc;
        ASSERT(!env->ExceptionCheck());
    } else {
#ifndef UNICODE
       char* buffer = new char[len+1];
       c->GetText(buffer, len+1);
       AwtTextComponent::RemoveCR(buffer);
       js = env->NewString(buffer, wcslen(buf)); // convert to unicode. right???
       delete[] buffer;
#else
       TCHAR *buffer = new TCHAR[len+1];
       c->GetText(buffer, len+1);
       AwtTextComponent::RemoveCR(buffer);
       js = TO_JSTRING((LPWSTR)buffer);
       //js = env->NewString(buffer, wcslen(buffer));
       delete[] buffer;
#endif
    }
    return js;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCTextComponentPeer_setText (JNIEnv *env, 
                                                    jobject thisObj,
                                                    jstring text)
{
    CHECK_PEER(thisObj);
    AwtTextComponent* c = PDATA(AwtTextComponent, thisObj);

    int length = env->GetStringLength(text);
#ifndef UNICODE
    TCHAR* buffer = new TCHAR[length * 2 + 1];
    buffer = TO_WSTRING(text);
#else
    WCHAR* buffer = new WCHAR[length  + 1];
    env->GetStringRegion(text, 0, length, (jchar*)buffer);
    buffer[length] = 0; //or it should be TEXT('\0') or L'\0'???
#endif

    buffer = AwtTextComponent::AddCR(buffer, length);
    c->SetText(buffer);
    /* Fix for TCK bug: setText() does not generate valueChangedEvent */
    if (env->IsInstanceOf(c->GetTarget(), WCachedIDs.java_awt_TextAreaClass)) {
        c->WmNotify(EN_CHANGE);
    }
    delete[] buffer;
}

JNIEXPORT jint JNICALL
Java_sun_awt_pocketpc_PPCTextComponentPeer_getSelectionStart (JNIEnv *env,
                                                              jobject thisObj)
{
    CHECK_PEER_RETURN(thisObj);
    AwtComponent* c = PDATA(AwtComponent, thisObj);
    long start;
    c->SendMessage(EM_GETSEL, (WPARAM)&start);
    return getJavaSelPos((AwtTextComponent*)c, start);

}

JNIEXPORT jint JNICALL
Java_sun_awt_pocketpc_PPCTextComponentPeer_getSelectionEnd (JNIEnv *env, 
                                                            jobject self)
{
     CHECK_PEER_RETURN(self);
     AwtComponent* c = PDATA(AwtComponent, self);
     long end;
     c->SendMessage(EM_GETSEL, 0, (LPARAM)&end);
     return getJavaSelPos((AwtTextComponent*)c, end);    
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCTextComponentPeer_select (JNIEnv *env, 
                                                   jobject thisObj,
                                                   jint start, jint end)
{
    CHECK_PEER(thisObj);
    AwtComponent* c = PDATA(AwtComponent, thisObj);

    c->SendMessage(EM_SETSEL, 
		   getWin32SelPos((AwtTextComponent*)c, start),
		   getWin32SelPos((AwtTextComponent*)c, end));

    c->SendMessage(EM_SCROLLCARET);    
}

static long getJavaSelPos(AwtTextComponent *c, long orgPos)
{
    long mblen;
    long pos = 0;
    long cur = 0;
    TCHAR *mbbuf;

    if ((mblen = c->GetTextLength()) == 0)
	   return 0;
    mbbuf = new TCHAR[mblen + 1];
    c->GetText(mbbuf, mblen + 1);
    while (cur < orgPos && pos++ < mblen) {
#ifndef UNICODE
	if (IsDBCSLeadByte(mbbuf[cur])) {
	    cur++;
	} else if (mbbuf[cur] == '\r' && mbbuf[cur + 1] == '\n') {
	    cur++;
	}
#else
	if (mbbuf[cur] == TEXT('\r') && mbbuf[cur + 1] == TEXT('\n')) {
	    cur++;
	}
#endif
	cur++;
    }
    delete[] mbbuf;
	return pos;
}

static long getWin32SelPos(AwtTextComponent *c, long orgPos)
{
    long mblen;
    long pos = 0;
    long cur = 0;
    TCHAR *mbbuf;

    if ((mblen = c->GetTextLength()) == 0)
       return 0;
    mbbuf = new TCHAR[mblen + 1];
    c->GetText(mbbuf, mblen + 1);

    while (cur < orgPos && pos < mblen) {
#ifndef UNICODE
       if (IsDBCSLeadByte(mbbuf[pos])) {
	      pos++;
       } else if (mbbuf[pos] == '\r' && mbbuf[pos + 1] == '\n') {
	   pos++;
       }
#else
       if (mbbuf[pos] == TEXT('\r') && mbbuf[pos + 1] == TEXT('\n')) {
	   pos++;
       }
#endif
       pos++;
       cur++;
    }
    delete[] mbbuf;
    return pos;
}
