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

#ifndef _wceCompat_h_
#define _wceCompat_h_

#ifdef INSIDE_WCE_COMPAT
#define DECLLIXPORT dllexport
#else
#define DECLLIXPORT dllimport
#endif

#ifdef __cplusplus
#define LANGPREFIX extern "C"
#else
#define LANGPREFIX
#endif /* __cplusplus */

#ifndef CVM_STATICLINK_LIBS
#define WCECOMPAT_API LANGPREFIX _declspec(DECLLIXPORT)
#else 
#define WCECOMPAT_API LANGPREFIX
#endif

WCECOMPAT_API HMENU __cdecl wceCreateMenuBarMenu();

WCECOMPAT_API HWND __cdecl
    wceSetMenuBar(HWND frame, HWND cmdBarWnd, HMENU menu);

WCECOMPAT_API int __cdecl wceGetCommandBarHeight(HWND hWndCmdBar);

WCECOMPAT_API int __cdecl wceIsCmdBarWnd(HWND hwnd);

WCECOMPAT_API HWND __cdecl wceGetFrameWnd(HWND cmdBarWnd);

WCECOMPAT_API int __cdecl wceWasMenuGesture(HWND hwnd, int x, int y,
					     int button, int down, UINT flags);
#ifdef _WIN32_WCE_EMULATION
WCECOMPAT_API int __cdecl wceConnect(unsigned int s, struct sockaddr *name, int namelen) ;
WCECOMPAT_API int __cdecl wceGetsockname(unsigned int s, struct sockaddr *name, int *namelen) ;
WCECOMPAT_API int __cdecl wceGethostname(char *name, int namelen ); 
WCECOMPAT_API struct hostent * __cdecl wceGethostbyname(char *name);
WCECOMPAT_API struct hostent * __cdecl wceGethostbyaddr(char *addr, int len, int type );
WCECOMPAT_API unsigned long __cdecl wceHtonl(unsigned long hostlong) ;
WCECOMPAT_API unsigned long __cdecl wceNtohl(unsigned long netlong) ;
WCECOMPAT_API int __cdecl wceBind(unsigned int s, const struct sockaddr *addr, int namelen ) ;
WCECOMPAT_API unsigned short __cdecl wceNtohs(unsigned short netshort );
WCECOMPAT_API int __cdecl wceGetsockopt(unsigned int s, int level, int optname, char *optval, int *optlen ) ;
WCECOMPAT_API int __cdecl wceSetsockopt(unsigned int s, int level, int optname, char *optval, int optlen ) ;

WCECOMPAT_API unsigned short __cdecl wceHtons(unsigned short hostshort) ;
WCECOMPAT_API int __cdecl wceSendto(unsigned int s, char *buf, int len, int flags, struct sockaddr *to, int tolen) ;
WCECOMPAT_API int __cdecl wceRecvfrom(unsigned int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen) ;
WCECOMPAT_API unsigned int __cdecl wceSocket(int af, int type, int protocol) ;
WCECOMPAT_API int __cdecl wceListen(unsigned int s, int backlog) ;
WCECOMPAT_API unsigned int __cdecl wceAccept(unsigned int s, struct sockaddr *addr, int *addrlen ) ;
WCECOMPAT_API int __cdecl wceIoctlsocket(unsigned int s, long cmd, unsigned long *argp ) ; 
WCECOMPAT_API int __cdecl wceRecv(unsigned int s, char *buf, int len, int flags ) ; 
WCECOMPAT_API int __cdecl wceSend(unsigned int s, char *buf, int len, int flags ) ; 
WCECOMPAT_API int __cdecl wceClosesocket(unsigned int s ) ; 
WCECOMPAT_API int __cdecl wceShutdown(unsigned int s, int how ) ; 
WCECOMPAT_API int __cdecl wceSelect(int nfds, struct fd_set *readfds, struct fd_set *writefds, struct fd_set *exceptfds, struct timeval *timeout) ;
WCECOMPAT_API int __cdecl wceWSAGetLastError(void) ;
WCECOMPAT_API int __cdecl wceWSACleanup(void) ;
WCECOMPAT_API int __cdecl wceWSAStartup(WORD wVersionRequested, void *lpWSAData);
#endif

#endif _wceCompat_h_
