/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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


#ifndef __kvm_emulator_limeio_h
#define __kvm_emulator_limeio_h

#ifdef WIN32

#include <windows.h>
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;

#ifndef LONG64_DEFINED
typedef __int64 long64;
#endif

#elif defined SOLARIS2

#include <sys/types.h>
#ifndef LONG64_DEFINED
#define LONG64_DEFINED
typedef long long           long64;
#endif

#elif defined(LINUX) || defined(UNIX)

#include <sys/types.h>
#include <stdint.h>

#ifndef LONG64_DEFINED
#define LONG64_DEFINED
typedef u_int64_t           long64;
#endif

#endif

typedef struct __Socket {
    struct InternalSocketData *data;
    int (*open) (struct __Socket *s, int port, const char *server);
    int (*close) (struct __Socket *s);
    int (*read) (struct __Socket *s, char *buffer, int buflen, int *bytes);
    int (*write) (struct __Socket *s, char *buffer, int bytes);
    int (*printf) (struct __Socket *s, char *format, ...);
    int (*readInt32) (struct __Socket *s, int32_t *result);
    int (*readLong64) (struct __Socket *s, long64 *value);
    int (*writeInt32) (struct __Socket *s, int32_t value);
    int (*writeLong64) (struct __Socket *s, long64 value);
    int (*flush) (struct __Socket *s);
    int (*ready) (struct __Socket *s);
    int (*block) (struct __Socket *s);
    void (*setDelayMode) (struct __Socket *s, int mode);
    void (*setPeekMode) (struct __Socket *s, int mode);
    void (*clearError) (struct __Socket *s);
    int (*getError) (struct __Socket *s);
} Socket;

typedef struct __ServerSocket {
    struct InternalServerSocketData *data;
    int (*listen) (struct __ServerSocket *s, int port);
    int (*listenAny) (struct __ServerSocket *s, int *port);
    int (*accept) (struct __ServerSocket *s, Socket **sp);
    int (*close) ( struct __ServerSocket *s);
    void (*clearError) (struct __ServerSocket *s);
    int (*getError) (struct __ServerSocket *s);
} ServerSocket;

Socket *NewSocket(void);
void DeleteSocket(Socket *s);

ServerSocket *NewServerSocket(void);
void DeleteServerSocket(ServerSocket *s);

#endif
