/*
 * @(#)PPCClipboard.cpp	1.9 06/10/10
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
 */

#include <string.h>

#include "sun_awt_pocketpc_PPCClipboard.h"
#include "PPCClipboard.h"
#include "PPCFontPeer.h"
#include "PPCTextComponentPeer.h"
#include "java_awt_datatransfer_StringSelection.h"

const TCHAR *StringFormatName = TEXT("JavaString");
static int stringFormat = 0;

jobject AwtClipboard::theCurrentClipboard = NULL;
BOOL AwtClipboard::ignoreWmDestroyClipboard = FALSE;

#ifndef WINCE
#define GALLOCFLG (GMEM_MOVEABLE | GMEM_DDESHARE | GMEM_ZEROINIT)
#else
#define LALLOCFLG (LPTR)
#endif

void AwtClipboard::LostOwnership() 
{
    if (theCurrentClipboard != NULL) {
	JNIEnv *env;
	
	if (JVM->AttachCurrentThread((void **) &env, 0) == 0) {
	    env->CallVoidMethod(theCurrentClipboard,
				WCachedIDs.PPCClipboard_lostSelectionOwnershipMID);
	    env->DeleteGlobalRef(theCurrentClipboard);
	    ASSERT(!env->ExceptionCheck());
	}
        theCurrentClipboard = NULL;
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCClipboard_initIDs(JNIEnv *env,
				      jclass cls) 
{
    GET_METHOD_ID (PPCClipboard_lostSelectionOwnershipMID,
		   "lostSelectionOwnership", "()V");
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCClipboard_init(JNIEnv *env,
				   jclass cls)
{
    stringFormat = ::RegisterClipboardFormat(StringFormatName);
    if (stringFormat == 0) {
        char buf[80];
        sprintf(buf, "couldn't register clipboard format: %d", 
                GetLastError());
	env->ThrowNew(WCachedIDs.java_awt_AWTExceptionClass, buf);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCClipboard_setClipboardText(JNIEnv *env,
					       jobject self, 
					       jobject ss)
{
    CHECK_NULL(ss, "null StringSelection");

    // Convert text into WCHAR array with CRLF line endings.
    jstring text = (jstring) 
	env->GetObjectField(ss, 
			    WCachedIDs.java_awt_datatransfer_StringSelection_dataFID);

    int length = env->GetStringLength(text)+1;
    WCHAR* ucText = new WCHAR[length];
    env->GetStringRegion(text, 0, length-1, (jchar *) ucText);
    ucText[length-1] = L'\0';
    ucText = AwtTextComponent::AddCR(ucText, length);

    length = wcslen(ucText) + 1;

    // Open clipboard.
    HWND hwndToolkit = AwtToolkit::GetInstance().GetHWnd();
    VERIFY(::OpenClipboard(hwndToolkit));

    // Set clipboard ownership.  We must call ::EmptyClipboard() to
    // purge stale data in synthesized formats (4141464) - but if we
    // already own the clipboard we should not call LostOwnership upon
    // receiving WM_DESTROYCLIPBOARD (4060810).
    AwtClipboard::ignoreWmDestroyClipboard = TRUE; 
    VERIFY(::EmptyClipboard());
    AwtClipboard::ignoreWmDestroyClipboard = FALSE;

    if (AwtClipboard::theCurrentClipboard != NULL) {
	env->DeleteGlobalRef(AwtClipboard::theCurrentClipboard);
    }
    AwtClipboard::theCurrentClipboard = env->NewGlobalRef(self);

#ifdef WINCE
     {
        // Create a global UNICODE copy of the StringSelector.
        WCHAR* buffer = (WCHAR*) ::LocalAlloc(LALLOCFLG, length * sizeof(WCHAR));
        wcscpy(buffer, ucText);
        VERIFY(::SetClipboardData(CF_UNICODETEXT, buffer));
    }
#else
    if (IS_NT) {
        // Create a global UNICODE copy of the StringSelector.
        HANDLE hGlobalMemory = GlobalAlloc(GALLOCFLG, length * sizeof(WCHAR));
        WCHAR* buffer = (WCHAR*)GlobalLock(hGlobalMemory);
        wcscpy(buffer, ucText);
        GlobalUnlock(hGlobalMemory);
	// fixed 4177171 : pass handle instead of buffer as per API docs
	VERIFY(::SetClipboardData(CF_UNICODETEXT, hGlobalMemory));
    } else {
        // Create a multibyte copy of the StringSelector.
        char* multibytes = W2A(ucText);
        DWORD length = strlen(multibytes) + 1;
        HANDLE hGlobalMemory = GlobalAlloc(GALLOCFLG, length);
        char* buffer = (char*)GlobalLock(hGlobalMemory);
        strcpy(buffer, multibytes);
        GlobalUnlock(hGlobalMemory);
	// fixed 4177171 : pass handle instead of buffer as per API docs
	VERIFY(::SetClipboardData(CF_TEXT, hGlobalMemory));
    }
#endif /* !WINCE */
    VERIFY(::CloseClipboard());

}

JNIEXPORT jstring JNICALL
Java_sun_awt_pocketpc_PPCClipboard_getClipboardText(JNIEnv *env,
					       jobject self)
{
    WCHAR* ucText = NULL;
    jchar* harr = NULL;
    HANDLE hClipMemory = NULL;
    DWORD length;

    VERIFY(::OpenClipboard(AwtToolkit::GetInstance().GetHWnd()));

#ifdef WINCE
    {
	hClipMemory = ::GetClipboardData(CF_UNICODETEXT);
	if (hClipMemory == NULL) {
	    VERIFY(::CloseClipboard());
	    return NULL;
	}
	length = (::LocalSize(hClipMemory) / sizeof(WCHAR));
	ucText = new WCHAR[length];
	wcscpy(ucText, (WCHAR*)hClipMemory);
    }
#else /* !WINCE */
    if (IS_NT) {
        hClipMemory = GetClipboardData(CF_UNICODETEXT);
        if (hClipMemory == NULL) {
            VERIFY(CloseClipboard());
            return NULL;
        }
        length = (GlobalSize(hClipMemory) / sizeof(WCHAR));
        ucText = new WCHAR[length];
        WCHAR* pClipMemory = (WCHAR*)GlobalLock(hClipMemory);
        wcscpy(ucText, pClipMemory);
    } else {
        HANDLE hClipMemory = GetClipboardData(CF_TEXT);
        if (hClipMemory == NULL) {
            VERIFY(CloseClipboard());
            return NULL;
        }
        WCHAR* pText = A2W((char*)GlobalLock(hClipMemory));
        length = wcslen(pText);
        ucText = new WCHAR[length+1];
        wcscpy(ucText, pText);
    }

    GlobalUnlock(hClipMemory);
#endif /* !WINCE */
    VERIFY(::CloseClipboard());

    // Convert WCHAR array to Java String and return it.
    length = AwtTextComponent::RemoveCR(ucText);
    harr = new jchar[length];
    ASSERT(harr != NULL);
    wcsncpy((wchar_t *) harr, ucText, length);
    delete[] ucText;
    jstring hStr = env->NewString(harr, length);
    ASSERT(!env->ExceptionCheck());
    return hStr;
}
