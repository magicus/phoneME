/*
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

// @(#)Setup.cpp	1.8 06/10/10
// Setup.cpp : Defines the entry point for the DLL application.
//

#include "windows.h"
#include "ce_setup.h"
#include "winbase.h" 
#include "shellapi.h"
#include "mmsystem.h"

// Pocket PC or not Pocket PC ... that is the question?

WCHAR *ppcExe = TEXT("\\bin\\cvm.exe");
WCHAR *ppcDll = TEXT("\\lib\\wceCompat.dll");
WCHAR *wce3Dir = TEXT("\\bin");
WCHAR *genExe = TEXT("\\bin\\cvm.exe");
WCHAR *genDll = TEXT("\\lib\\wceCompat.dll");

WCHAR *wTempExe = NULL;
WCHAR *wTempDll = NULL;
WCHAR *wTempDir = NULL;
WCHAR *wPermExe = NULL;
WCHAR *wPermDll = NULL;

WCHAR* theAppender(const WCHAR*, const LPCTSTR);

WCHAR* theAppender(const WCHAR *wchString, const LPCTSTR instDir)
{
    WCHAR *wString = NULL;

    int instDirLen = wcslen(instDir);
    int wchLen = wcslen(wchString);
    int mallocLen = wchLen + instDirLen + 1; /* Null terminated */

    wString = (unsigned short*) malloc(mallocLen * sizeof(WCHAR));
    /* Copy INSTALL_DIR into wString */
    wcscpy(wString, instDir);
    /* Append the rest of string using wcsncpy */
    wcsncpy(wString + instDirLen, wchString, wchLen);
    /* Ensure string is null terminated */
    wString[mallocLen - 1] = TEXT('\0');
    return wString;
}


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

codeINSTALL_INIT Install_Init(HWND hwndParent,
                              BOOL fFirstCall,
                              BOOL fPreviouslyInstalled,
                              LPCTSTR pszInstallDir)
{
   return codeINSTALL_INIT_CONTINUE;
} 


codeINSTALL_EXIT Install_Exit(HWND hwndParent,
                              LPCTSTR pszInstallDir,
                              WORD cFailedDirs,
                              WORD cFailedFiles,
                              WORD cFailedRegKeys,
                              WORD cFailedRegVals,
                              WORD cFailedShortcuts)
{

	if(cFailedDirs||cFailedFiles||cFailedRegKeys||cFailedRegVals
		||cFailedShortcuts)
	{
		// The installation failed, so clean up and exit
		// TO DO: Undo any actions taken in Install_Init()

		return codeINSTALL_EXIT_UNINSTALL;
	}

	bool IsPocketPC;

	TCHAR szPlat[256];
	INT rc;

	/* Get platform type */

	rc = SystemParametersInfo(SPI_GETPLATFORMTYPE, sizeof(szPlat), szPlat, 0);

	/* Palm PC2 = PocketPC */

	if(lstrcmp(szPlat, TEXT("Palm PC2")) == 0) {
		IsPocketPC = true;
	} else {
		IsPocketPC = false;
	}
        
	/* Set up file paths */
	wTempExe = theAppender(ppcExe, pszInstallDir);        
	wTempDll = theAppender(ppcDll, pszInstallDir);
	wTempDir = theAppender(wce3Dir, pszInstallDir);


	if(IsPocketPC){		            
            /* POCKET PC VERSION */
//          MessageBox(NULL, TEXT("POCKET PC DETECTED"), TEXT("Detected"), MB_OK);
            /* Delete 2.11 EXE file */
            wPermExe = theAppender(genExe, pszInstallDir);
            DeleteFile(wPermExe);

            /* Move Pocket PC EXE file into INSTALL_DIR/bin directory */
            MoveFile(wTempExe, wPermExe);

            /* Delete 2.11 DLL file */
            wPermDll = theAppender(genDll, pszInstallDir);
            DeleteFile(wPermDll);

            /* Move Pocket PC DLL file into INSTALL_DIR/bin directory */
            MoveFile(wTempDll, wPermDll);

            /* Cleanup */
            free(wPermExe);
            free(wPermDll);
            
	} else {  /* NOT POCKET PC */		
            /* Delete Pocket PC EXE and DLL files */
            DeleteFile(wTempExe);
            DeleteFile(wTempDll);
	}

	/* Remove /Program Files/Java/bin/wce300 directory */
	RemoveDirectory(wTempDir);

        /* Cleanup */
        free(wTempExe);
        free(wTempDll);
        free(wTempDir);

	return codeINSTALL_EXIT_DONE;
} 

codeUNINSTALL_INIT Uninstall_Init(HWND hwndParent,
                              LPCTSTR pszInstallDir)
{
	/* Re-create wce300 directory - so uninstall does not complain about it */
	wTempDir = theAppender(wce3Dir, pszInstallDir);
	CreateDirectory(wTempDir, NULL);

        /* Cleanup */
        free(wTempDir);

   return codeUNINSTALL_INIT_CONTINUE;
} 

codeUNINSTALL_EXIT Uninstall_Exit(HWND hwndParent)
{
   // TO DO: Add your code here 

//  PlaySound(wSound, NULL, SND_LOOP);

   return codeUNINSTALL_EXIT_DONE;
} 





