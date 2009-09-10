/*
 * @(#)jni_io.h	1.59 06/10/23
 *
 * Portions Copyright  2000-2006 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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

#ifndef _INCLUDED_JNI_IO_H
#define _INCLUDED_JNI_IO_H

#include "javavm/include/porting/net.h"

typedef struct CVMIOVector CVMIOVector;

#define JVMIOVEC_VERSION_1 0x40000100

struct CVMIOVector {
    CVMInt32       (*CVMjniIOOpen)(const char *name, CVMInt32 mode,
                                   CVMInt32 perm);
    CVMInt32       (*CVMjniIOClose)(CVMInt32 filedes);
    CVMInt32       (*CVMjniIORead)(CVMInt32 filedes, void *buf, CVMUint32 nbyte);
    CVMInt32       (*CVMjniIOWrite)(CVMInt32 filedes, const void *buf,
                               CVMUint32 nbyte);
    CVMInt64       (*CVMjniIOSeek)(CVMInt32 filedes, CVMInt64 cur,
                                   CVMInt32 whence);
    CVMInt32       (*CVMjniIOFlush)(FILE *f);
    CVMInt32       (*CVMjniIORemove)(const char *name);

    CVMInt32       (*CVMjniIOSocket)(CVMInt32 domain, CVMInt32 type, CVMInt32 proto);
    CVMInt32       (*CVMjniIOSocketClose)(CVMInt32 fd);
    CVMInt32       (*CVMjniIOBind)(CVMInt32 fd, struct sockaddr *name,
                              CVMInt32 namelen);
    CVMInt32       (*CVMjniIOGetSockName)(CVMInt32 fd, struct sockaddr *name,
                                    CVMInt32 *namelen);
    CVMInt32       (*CVMjniIOPoll)(CVMInt32 fd, CVMBool isWrite, CVMInt32 timeout);
    CVMInt32       (*CVMjniIOConnect)(CVMInt32 fd, struct sockaddr *name,
                                 CVMInt32 namelen);
    CVMInt32       (*CVMjniIOListen)(CVMInt32 fd, CVMInt32 count);
    CVMInt32       (*CVMjniIOAccept)(CVMInt32 fd, struct sockaddr *name,
                                CVMInt32 *namelen);
    CVMInt32       (*CVMjniIOSetSockOpt)(CVMInt32 fd, CVMInt32 level,
                                    CVMInt32 optName,
                                    const void *optVal, CVMInt32 optsize);
    CVMInt32       (*CVMjniIOSend)(CVMInt32 s, char *msg, CVMInt32 len,
                              CVMInt32 flags);
    CVMInt32       (*CVMjniIORecv)(CVMInt32 f, char *buf, CVMInt32 len,
                              CVMInt32 option);
    struct hostent *(*CVMjniIOGetHostByName)(char *hostname);
    CVMInt32       (*CVMjniIOShutdown)(CVMInt32 filedes, CVMInt32 option);

    CVMBool        (*CVMjniIOThreadCreate)(CVMThreadID *thread,
                                           CVMSize stackSize,
                                           CVMInt32 priority,
                                           void (*func)(void *), void *arg);
    CVMInt64       (*CVMjniIOGetTimeMillis)(void);
    void           (*CVMjniIOSleep)(unsigned millis);
    CVMBool        (*CVMjniIOMutexInit)( CVMMutex *m);
    void           (*CVMjniIOMutexDestroy)(CVMMutex *m);
    void           (*CVMjniIOMutexLock)(CVMMutex *m);
    void           (*CVMjniIOMutexUnlock)(CVMMutex *m);
    CVMBool        (*CVMjniIOCondvarInit)(CVMCondVar *c, CVMMutex* m);
    void           (*CVMjniIOCondvarDestroy)(CVMCondVar *c);
    CVMBool        (*CVMjniIOCondvarWait)(CVMCondVar *c, CVMMutex *m,
                                       CVMJavaLong millis);
    void           (*CVMjniIOCondvarNotify)(CVMCondVar *c);
    void           (*CVMjniIOCondvarNotifyAll)(CVMCondVar *c);
    void           (*CVMjniIOExit)(CVMInt32 exitStatus);
};


#endif /*_INCLUDED_JNI_IO_H */
