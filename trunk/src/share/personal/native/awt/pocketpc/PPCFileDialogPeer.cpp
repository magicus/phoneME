/*
 * @(#)PPCFileDialogPeer.cpp	1.10 06/10/10
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
#include <winbase.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> 
#include <string.h>
#include <Ctype.h>

#include "awt.h"
#include "PPCToolkit.h"
#include "PPCComponentPeer.h"
#include "PPCDialogPeer.h"

#include "jni.h"
#include "sun_awt_pocketpc_PPCFileDialogPeer.h"
#include "PPCFileDialogPeer.h"

#include <java_awt_FileDialog.h>
#include <commdlg.h>

/* these corrupt non-ascii strings */
#define ASCII2UNICODE(astr, ustr, max) \
    MultiByteToWideChar(CP_ACP, 0, (astr), -1, (ustr), (max))

#define UNICODE2ASCII(ustr, astr, max) \
    WideCharToMultiByte(CP_ACP, 0, (ustr), -1, (astr), (max), NULL, NULL)


/* No getenv supported by WinCE. Mapped to the Windows Registry */
/* javai.c, properties_md.c */
char *AwtFileDialog::getenv(const char* name) {
    #define WINREG_MAX_NUM             10 /* temporary restriction */
    #define WINREG_HKEY_NAME_FOR_PJAVA TEXT("Software\\Sun\\PersonalJava\\")
    #define WINREG_MAX_NAME_LEN        64
    #define WINREG_MAX_VAL_LEN         1024
    #define PJAVA_SUBKEY               TEXT("default")

    static HKEY hKey = NULL;
    static int numVal = 0;
    static char* nameTable[WINREG_MAX_NUM];
    static char* valTable[WINREG_MAX_NUM];

    if (hKey == NULL) {
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			 WINREG_HKEY_NAME_FOR_PJAVA /*+*/ PJAVA_SUBKEY,
			 0, 0, &hKey) 
	    != ERROR_SUCCESS) {
          ASSERT(hKey == NULL);
	    return NULL;
	}
    } else {
	int i;
	for (i = 0; i < numVal; i++) {
	    if (strcmp(name, nameTable[i]) == 0) {
		return valTable[i];
	    }
	}
    }
    if (numVal >= WINREG_MAX_NUM) {
	return NULL;
    } else {
	WCHAR wName[WINREG_MAX_NAME_LEN];
	WCHAR wVal[WINREG_MAX_VAL_LEN];
	char  aVal[WINREG_MAX_VAL_LEN];
	DWORD dwSize = sizeof(wVal); /* size in bytes */
	DWORD dwType;

	ASCII2UNICODE(name, wName, WINREG_MAX_NAME_LEN);
	if (RegQueryValueEx(hKey, wName, NULL, &dwType, (BYTE*)wVal, &dwSize)
	    != ERROR_SUCCESS 
	    || dwType != REG_SZ) { /* null-terminated UNICODE string */
	    return NULL;
	}
	UNICODE2ASCII(wVal, aVal, WINREG_MAX_VAL_LEN);
	if ((nameTable[numVal] = strdup(name)) == NULL) { /* out of memory*/
	    return NULL;
	}
	if ((valTable[numVal] = strdup(aVal)) == NULL) { /* out of memory*/
	    delete(nameTable[numVal]);
	    return NULL;
	}
    }
    return valTable[numVal++];
}


DWORD AwtFileDialog::GetCurrentDirectoryA(DWORD nBufferLength, char *lpBuffer) {
    const char* cwd;
    char buf[MAX_PATH];

    if ((cwd = AwtFileDialog::getenv("PWD")) == NULL || cwd[0] == '\0') {
	/*Current Working Directory will be set to ROOT*/
	buf[0] = '\\';
	buf[1] = '\0';
	cwd = (buf[0] == '\0')?("\\."):(buf);
    }
    ASSERT(nBufferLength >= 1);
    strncpy(lpBuffer, cwd, nBufferLength - 1);
    /* strncpy'ed result may not be null-terminated */
    lpBuffer[nBufferLength - 1] = '\0';

    return strlen(lpBuffer);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCFileDialogPeer_initIDs (JNIEnv *env, jclass cls)
{
    GET_METHOD_ID( PPCFileDialogPeer_handleSelectedMID,
                   "handleSelected", "(Ljava/lang/String;)V" );
    GET_METHOD_ID( PPCFileDialogPeer_handleCancelMID,
                   "handleCancel", "()V" );
    GET_FIELD_ID( PPCFileDialogPeer_parentFID,
                 "parent", "Lsun/awt/pocketpc/PPCComponentPeer;");

    cls = env->FindClass( "java/awt/FileDialog" );
    if ( cls == NULL )
        return;

    GET_METHOD_ID (java_awt_FileDialog_getDirectoryMID, "getDirectory", 
                   "()Ljava/lang/String;");
    GET_METHOD_ID (java_awt_FileDialog_getFileMID, "getFile",
                   "()Ljava/lang/String;");
                   GET_METHOD_ID (java_awt_FileDialog_getModeMID, "getMode", "()I");
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCFileDialogPeer_show(JNIEnv *env, 
                                             jobject peer)
{
  
  ASSERT(peer != NULL);
  jobject target = 
    env->GetObjectField(peer,
                        WCachedIDs.PPCObjectPeer_targetFID);
    jobject hParent = env->GetObjectField(peer,
                                         WCachedIDs.PPCFileDialogPeer_parentFID);                              

    AwtComponent* parent = PDATA(AwtComponent, hParent);
    TCHAR* titlebuf = NULL;
    jstring title = (jstring)env->CallObjectMethod(target,
						   WCachedIDs.java_awt_Dialog_getTitleMID); 

    if (title) {
	int length = env->GetStringLength(title);
#ifndef UNICODE
	titlebuf = new TCHAR[length * 2 + 1];
        titlebuf = (TCHAR *) (char *)env->GetStringChars(title, NULL); 
#else
	titlebuf = new TCHAR[length + 1];
	env->GetStringRegion(title, 0, length, (jchar *) titlebuf);
        titlebuf[length] = L'\0';
#endif
    }


    // Save current directory, so we can reset if it changes.
    TCHAR* currentDirectory = new TCHAR[MAX_PATH+1];
#ifndef WINCE
    VERIFY(::GetCurrentDirectory(MAX_PATH, currentDirectory) > 0);
#endif


    TCHAR* directory = NULL;
    jstring dir = (jstring)env->CallObjectMethod(target,
						   WCachedIDs.java_awt_FileDialog_getDirectoryMID); 
    if (dir) {
      /*
      ** If a directory was specified then use it.
      */
	int length = env->GetStringLength(dir);
#ifndef UNICODE
      Note : add the cwd stuff to the non-unicode stuff, if ever needed.
	directory = new TCHAR[length * 2 + 1];
        directory = (TCHAR *) (char *)env->GetStringChars(dir, NULL); 
    }
#else
        directory = new TCHAR[MAX_PATH + 1];
	env->GetStringRegion(dir, 0, length, (jchar *) directory);
        directory[length] = L'\0';
	if ((directory[0] != TEXT('\\')) && (directory[0] != TEXT('/'))) 
        {
	  char achCwd[MAX_PATH] ;
	  AwtFileDialog::GetCurrentDirectoryA(MAX_PATH, achCwd) ;
          int cwdlen = strlen(achCwd) ;
	  MultiByteToWideChar(CP_ACP, 0, achCwd, cwdlen, directory, MAX_PATH) ;
	  if (directory[cwdlen-1] != TEXT('\\') && (directory[cwdlen-1] != TEXT('/')))
	  {
	      directory[cwdlen] = TEXT('\\') ;
	  } 
          env->GetStringRegion(dir, 0, length, (jchar *) (&directory[cwdlen+1]));
          directory[length+cwdlen+1] = L'\0';
        }

    }
    else {
      /*
      ** Otherwise use the current working directory.
      */
	char achCwd[MAX_PATH] ;
	AwtFileDialog::GetCurrentDirectoryA(MAX_PATH, achCwd) ;
        int cwdlen = strlen(achCwd) ;
	directory = new TCHAR[cwdlen * 2 + 1];

	MultiByteToWideChar(CP_ACP, 0, achCwd, cwdlen, directory, MAX_PATH) ;
        directory[cwdlen+1] = 0 ;
    }
#endif

    TCHAR* fileBuffer = new TCHAR[MAX_PATH+1];
    jstring file = (jstring)env->CallObjectMethod(target,
                                                  WCachedIDs.java_awt_FileDialog_getFileMID); 
    if (file) {
	int length = env->GetStringLength(file);
#ifndef UNICODE
        fileBuffer = (TCHAR *) (char *)env->GetStringChars(file, NULL); 
#else
        env->GetStringRegion(file, 0, length, (jchar *)fileBuffer);
        fileBuffer[length] = L'\0';
#endif
    } else {
        fileBuffer[0] = L'\0';
    }

    OPENFILENAME ofn;
    const TCHAR filter[] = TEXT("All Files (*.*)\0*.*\0\0");
    memset(&ofn, 0, sizeof(ofn));

    int fileFilterLen = 0;

    WCHAR *fileFilter = NULL;

    if(fileBuffer[0] != TEXT('\0')){
	    fileFilterLen = (wcslen(fileBuffer)*2 + 6);
	    fileFilter = new WCHAR[fileFilterLen];
        fileFilterLen = wcslen(fileBuffer);
        fileFilter[0] = TEXT('\0');
        wcscat(fileFilter, fileBuffer);
        fileFilter[fileFilterLen] = TEXT('\0');
        wcscat(fileFilter, fileBuffer);
        for(int i = fileFilterLen; i >= 0; i--){
            fileFilter[fileFilterLen+i+1]=fileFilter[fileFilterLen+i];
        }
        fileFilter[fileFilterLen] = TEXT('\0');

        fileFilter[(fileFilterLen*2)+1] = TEXT('\0');
        fileFilter[(fileFilterLen*2)+2] = TEXT('\0');
 
        ofn.lpstrFilter = fileFilter;

    }else{
        ofn.lpstrFilter = filter;
    }

    ofn.lStructSize = sizeof(ofn);
    ofn.nFilterIndex = 1;
    ofn.hwndOwner = parent ? parent->GetHWnd() : NULL;
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = titlebuf;
    ofn.lpstrInitialDir = directory;
    ofn.Flags = OFN_LONGNAMES | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;


    BOOL result;

    // disable all top-level windows
    AwtDialog::ModalDisable(NULL);
    // show the Win32 file dialog
    jint mode = env->CallIntMethod(target,
                                   WCachedIDs.java_awt_FileDialog_getModeMID);
    if (mode == java_awt_FileDialog_LOAD) {
        result = GetOpenFileName(&ofn);
    } else {
        result = GetSaveFileName(&ofn);
    }
    // re-enable top-level windows
    AwtDialog::ModalEnable(NULL);
    AwtDialog::ModalNextWindowToFront(ofn.hwndOwner);
#ifndef WINCE
    VERIFY(::SetCurrentDirectory(currentDirectory));
#endif

    // Report result to peer.
#ifndef WINCE
    if (result) {      
        env->CallVoidMethod(peer, 
                              WCachedIDs.PPCFileDialogPeer_handleSelectedMID, 
                              env->GetStringChars(ofn.lpstrFile));
    } else {
        env->CallVoidMethod(peer, 
                              WCachedIDs.PPCFileDialogPeer_handleCancelMID);
    }
#else 
    if (result) {
      jchar* javaArray;
	int wLen ;

	wLen = wcslen(ofn.lpstrFile) ;
 	javaArray = new jchar[wLen] ;

	memcpy((unsigned short *) javaArray, ofn.lpstrFile, wLen * sizeof (TCHAR)) ;

        jstring jcString = env->NewString(javaArray, wLen);

	//KEEP_POINTER_ALIVE(body) ;  PENDING : CHECK THIS ONE OUT???
        env->CallVoidMethod(peer, 
                            WCachedIDs.PPCFileDialogPeer_handleSelectedMID, 
                            jcString);
    } else {
        env->CallVoidMethod(peer, 
                            WCachedIDs.PPCFileDialogPeer_handleCancelMID);
    }

#endif
ASSERT(!env->ExceptionCheck());

    // Cleanup.
    delete[] fileBuffer;
    delete[] currentDirectory;
	if (fileFilter)
	{
		delete(fileFilter);
	}
    if (directory) {
        delete[] directory;
    }
    if (titlebuf) {
        delete[] titlebuf;
    }

}

/*
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCFileDialogPeer_create (JNIEnv *env, jobject thisobj)
{
	
}
*/

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCFileDialogPeer_setFileNative (JNIEnv *env, 
                                                         jobject thisobj,
                                                         jbyteArray file)
{
    
}



