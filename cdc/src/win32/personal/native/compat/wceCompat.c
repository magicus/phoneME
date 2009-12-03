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

#include <windows.h>

#include <commctrl.h>
#include <winsock.h>
#include "wceCompat.h"
#include "wceUtil.h"
#include "resource.h"

/*
 * WCE Compatibility library, to make things run across all WCE platforms
 */


#if _WIN32_WCE >= 300

#include <aygshell.h>

#define BRAIN_DEAD

#endif


static HINSTANCE theInstance; /* instance of this library */

#ifdef POCKETPC_MENUS
static HWND theCmdBarWnd;
#endif

#ifdef OBSOLOETE
static HWND theFrameWnd;
#endif

#ifndef CVM_STATICLINK_LIBS
/* Dll Main, called when library in loaded, used to get instance handle
 * for loading resources later
 */
BOOL WINAPI DllMain(HANDLE hInstDll, DWORD reason, LPVOID resevered)
{

    switch (reason) {
        case DLL_PROCESS_ATTACH:
	    theInstance = (HINSTANCE)hInstDll;
	    break;
        default:
	    break;
    }
    return TRUE;
}
#endif

#ifdef POCKETPC_MENUS
#define SHGetMenu(hWndMB) \
    (HMENU)SendMessage((hWndMB), SHCMBM_GETMENU, (WPARAM)0, (LPARAM)0)

#define SHGetSubMenu(hWndMB, MenuId) \
    (HMENU)SendMessage((hWndMB), SHCMBM_GETSUBMENU, (WPARAM) 0, (LPARAM)MenuId)

#define SHSetSubMenu(hWndMB, MenuId, hMenu) \
    (void)SendMessage((hWndMB), SHCMBM_SETSUBMENU, (WPARAM)MenuId, (LPARAM)hMenu)
#endif


WCECOMPAT_API HMENU __cdecl wceCreateMenuBarMenu()
{
#ifdef POCKETPC_MENUS
    return CreatePopupMenu();
#else
    return CreateMenu();
#endif
}

WCECOMPAT_API int __cdecl
wceIsCmdBarWnd(HWND hwnd)
{
#ifdef POCKETPC_MENUS
    if (hwnd == theCmdBarWnd) {
	return 1;
    } else {
	return 0;
    }
#else
    return 0;
#endif
}

/* Set a "Menu Bar" on the given frame.
 * PocketPC places menubars at the bottom, which are non-dynamic 
 * so we have a place holder menu item, which takes one to the menu-bar 
 */
WCECOMPAT_API HWND __cdecl
wceSetMenuBar(HWND frame, HWND cmdBarWnd, HMENU menu)
{
#ifdef POCKETPC_MENUS 
    static HMENU javaMenu = NULL;
    HMENU hMenuBar, subMenu;
    SHMENUBARINFO mb;

    /* What we have to do here is pretty bad */
    if (!theInstance) {
	return NULL;
    } 
    if (!frame) {
	return NULL;
    }
    if (1)  {
	/* Only way of creating a menubar is by loading it from a resouce */
	/* This load is a sanity check */
	javaMenu = LoadMenu(theInstance, MAKEINTRESOURCE(IDR_MENUBAR1));
        if (!javaMenu) {
	    return NULL;
	}

	memset(&mb, 0, sizeof(SHMENUBARINFO));
	mb.cbSize = sizeof(SHMENUBARINFO);
	mb.hwndParent = frame;
	mb.dwFlags = 0;
	mb.nToolBarId = IDR_MENUBAR1;
	mb.nBmpId = 0;
	mb.hInstRes =  theInstance;
	mb.cBmpImages =  0;
	/* If this fails,  we're toast */
	if (!SHCreateMenuBar(&mb)) {
	    return NULL;
	}
	cmdBarWnd = mb.hwndMB;
	if (!cmdBarWnd) {
	    return NULL;
	}
    }

    theCmdBarWnd = cmdBarWnd;
    javaMenu = SHGetMenu(cmdBarWnd);
    subMenu = GetSubMenu(javaMenu, 0);
    if (!subMenu) {
	return NULL;
    }
    
    /* Now, we simply cannot set the commandbar menu */
    /* but we can set its submenu, by sending it the */
    /* SHCMBM_SETSUBMENU message */

    SHSetSubMenu(cmdBarWnd, JAVA_MENU_ID, menu); /* this is a marco, defined above */
#else  /* ! POCKETPC_MENUS */
    #define ID_CMDBAR 199
    cmdBarWnd = CommandBar_Create(GetModuleHandle(NULL), frame, ID_CMDBAR);
    if (!CommandBar_InsertMenubarEx(cmdBarWnd,
				    NULL,// No instance implies handle
				    (LPTSTR) menu,
				    0)) {
	// Failure
	return (NULL);
    }
#endif POCKETPC_MENUS
#ifdef OBSOLOLETE
    theFrameWnd = frame;
#endif
    return cmdBarWnd;
    
}


/* return the (added) height of the command bar */
/* for pocket PC, this is zero, as it is not in the client area */
WCECOMPAT_API int __cdecl
wceGetCommandBarHeight(HWND hWndCmdBar)
{
#ifdef POCKETPC_MENUS
    return 0;
#else 
    return CommandBar_Height(hWndCmdBar);
#endif
}

#ifdef OBSOLETE
WCECOMPAT_API HWND __cdecl wceGetFrameWnd(HWND cmdBarWnd)
{
    return theFrameWnd; /* FIXME: multiple frames! */
}
#endif

#include <dbgapi.h>

WCECOMPAT_API int __cdecl
wceWasMenuGesture(HWND hwnd, int x, int y, int button, int down, UINT flags)
{
#ifdef POCKETPC_MENUS

    SHRGINFO shrginfo;
    int result;
    static int doit = 0 ;

    shrginfo.cbSize = sizeof(SHRGINFO);
    shrginfo.hwndClient = hwnd;
    shrginfo.ptDown.x = x;
    shrginfo.ptDown.y = y;
    shrginfo.dwFlags = SHRG_RETURNCMD;
    NKDbgPrintfW(TEXT("entering recognize\n"));
    if (doit) {
	result = SHRecognizeGesture(&shrginfo);
    }
    NKDbgPrintfW(TEXT("got %d\n"), result);
    return (result);// SHRecognizeGesture(&shrginfo));
#else

    return 0; /* handled elsewhere, for now */

#endif

}


#ifdef _WIN32_WCE_EMULATION

WCECOMPAT_API int __cdecl 
wceConnect (SOCKET s, struct sockaddr *name, int namelen)
{
    return(connect(s, name, namelen)) ;
}

WCECOMPAT_API int __cdecl 
wceGetsockname(SOCKET s, struct sockaddr *name, int *namelen)

{
    return(getsockname(s, name, namelen)) ;
}

WCECOMPAT_API int __cdecl 
wceGethostname(char *name, int namelen )
{
    return(gethostname(name, namelen )) ;
}

WCECOMPAT_API struct hostent * __cdecl 
wceGethostbyname(char *name)
{
    return(gethostbyname(name)) ;
}

WCECOMPAT_API struct hostent * __cdecl 
wceGethostbyaddr(char *addr, int len, int type )
{
    return(gethostbyaddr(addr, len, type )) ;
}

WCECOMPAT_API u_long __cdecl wceHtonl(u_long hostlong)
{
    return(htonl(hostlong)) ;
}

WCECOMPAT_API u_long __cdecl wceNtohl(u_long netlong)
{
    return(ntohl(netlong)) ;
}

WCECOMPAT_API int __cdecl wceBind(u_int s, const struct sockaddr *addr, int namelen )
{
    return(bind(s, addr, namelen)) ;
}
WCECOMPAT_API u_short __cdecl wceNtohs(u_short netshort )
{
    return(ntohs(netshort)) ;
}

WCECOMPAT_API int __cdecl wceGetsockopt(u_int s, int level, int optname, char *optval, int *optlen )
{
    return(getsockopt(s, level, optname, optval, optlen)) ;
}

WCECOMPAT_API int __cdecl wceSetsockopt(u_int s, int level, int optname, char *optval, int optlen )
{
    return(setsockopt(s, level, optname, optval, optlen)) ;
}


WCECOMPAT_API u_short __cdecl wceHtons(u_short hostshort)
{
    return(htons(hostshort)) ;
}

WCECOMPAT_API int __cdecl wceSendto(u_int s, char *buf, int len, int flags, struct sockaddr *to, int tolen)
{
    return(sendto(s, buf, len, flags, to, tolen)) ;
}

WCECOMPAT_API int __cdecl wceRecvfrom(u_int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen)
{
    return(recvfrom(s, buf, len, flags, from, fromlen)) ;
}

WCECOMPAT_API u_int __cdecl wceSocket(int af, int type, int protocol)
{
    return(socket(af, type, protocol)) ;
}

WCECOMPAT_API int __cdecl wceListen(u_int s, int backlog)
{
    return(listen(s, backlog)) ;
}

WCECOMPAT_API u_int __cdecl wceAccept(u_int s, struct sockaddr *addr, int *addrlen )
{
    return(accept(s, addr, addrlen)) ; 
}

WCECOMPAT_API int __cdecl wceIoctlsocket(u_int s, long cmd, u_long *argp )
{
    return(ioctlsocket(s, cmd, argp)) ;
}

WCECOMPAT_API int __cdecl wceRecv(u_int s, char *buf, int len, int flags )
{
    return(recv(s, buf, len, flags)) ; 
}

WCECOMPAT_API int __cdecl wceSend(u_int s, char *buf, int len, int flags )
{
    return(send(s, buf, len, flags)) ; 
}

WCECOMPAT_API int __cdecl wceClosesocket(u_int s)
{
    return(closesocket(s)); 
}

WCECOMPAT_API int __cdecl wceShutdown(u_int s, int how)
{
    return(shutdown(s, how)) ;
}

WCECOMPAT_API int __cdecl wceSelect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
    return(select(nfds, readfds, writefds, exceptfds, timeout)) ;
}

WCECOMPAT_API int __cdecl wceWSAGetLastError(void)
{
	return(WSAGetLastError()) ;
}

WCECOMPAT_API int __cdecl wceWSACleanup(void)
{
	return(WSACleanup()) ;
}

WCECOMPAT_API int __cdecl wceWSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData )
{
	return(WSAStartup(wVersionRequested, lpWSAData)) ;
}

#endif

/* these corrupt non-ascii strings */
#define ASCII2UNICODE(astr, ustr, max) \
    MultiByteToWideChar(CP_ACP, 0, (astr), -1, (ustr), (max))

#define UNICODE2ASCII(ustr, astr, max) \
    WideCharToMultiByte(CP_ACP, 0, (ustr), -1, (astr), (max), NULL, NULL)

#ifndef CVM_STATICLINK_LIBS
WCECOMPAT_API char * __cdecl
getenv(const char* name)
{
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
	    free(nameTable[numVal]);
	    return NULL;
	}
    }
    return valTable[numVal++];
}
#endif

