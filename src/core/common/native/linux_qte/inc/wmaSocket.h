/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
 * This source file is specific for Qt-based configurations.
 */

#ifndef QT_SOCKETS_H
#define QT_SOCKETS_H

#include <jsr120_types.h>
#include <stdlib.h>
#include <stdio.h>
#include <qsocket.h>
#include <qsocketnotifier.h>

/**
 * @file
 *
 * This file defines the WMASocket class and methods to associate
 * network events with the main Qt event loop.
 */

/* Socket status values. */
typedef enum {
    SOCKET_STATUS_IDLE = 0,
    SOCKET_STATUS_PENDING_READ,
    SOCKET_STATUS_PENDING_WRITE,
    SOCKET_STATUS_INTERRUPTED
} wma_vmSocket_status;

class WMASocket : public QObject
{

    Q_OBJECT

public:
    WMASocket(WMA_PROTOCOLS protocol, int fd, QObject *parent=0, const char* name=0);
    ~WMASocket();

    QSocketNotifier* getReadNotifier()	{ return readNotifier; }
    QSocketNotifier* getWriteNotifier()	{ return writeNotifier; }
    int	             getSockProtocol()	{ return sockProtocol; }
    int	             getSockFD()	{ return sockFD; }

public slots:
    void readableSlot(int);
    void writableSlot(int);

protected:
    QSocketNotifier* readNotifier;
    QSocketNotifier* writeNotifier;
    WMA_PROTOCOLS sockProtocol;
    int	sockFD;

};


/**
 * Creates a platform-specific handle.
 *
 * @param fd platform-specific file descriptor to be associated with
 *		the new handle
 *
 * @return the platform-specific handle; 0 if there was an error
 */
long wmaCreateSocketHandle(WMA_PROTOCOLS protocol, int fd);

/**
 * Gets the platform-specific file descriptor associated with the
 * given handle.
 *
 * @param handle platform-specific handle to the open connection
 *
 * @return the associated platform-specific file descriptor; a negative
 *	   number if there was an error
 */
int wmaGetRawSocketFD(void *handle);

/**
 * Destroys a platform-specific handle and releases any resources used
 * by the handle.
 *
 * @param handle platform-specific handle to destroy
 *
 * @return 0 if successful; a non-zero value if there was an error
 */
void wmaDestroySocketHandle(void *handle);

#endif
