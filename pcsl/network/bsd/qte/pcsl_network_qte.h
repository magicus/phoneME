/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#include <qsocket.h>
#include <qsocketnotifier.h>

/**
 * @file
 *
 * This file defines the VMSocket class and methods to associate 
 * network events with the main Qt event loop.
 * It also defines QT interfaces that would be used by client sockets, 
 * as well as server sockets. The VMSocket object resides as a part of PCSL
 * library and the interfaces would be used by client sockets(which are part
 * of PCSL library) as well as server sockets and pushregistry 
 * (which would be part of MIDP stack)
 */

#define SD_NONE  -1

#define SD_RECV   0

#define SD_SEND   1

#define SD_BOTH   2

typedef enum {
    SOCKET_STATUS_IDLE = 0,
    SOCKET_STATUS_PENDING_READ,
    SOCKET_STATUS_PENDING_WRITE,
    SOCKET_STATUS_INTERRUPTED
} pcsl_vmSocket_status;

class VMSocket : public QObject
{
    Q_OBJECT

public:
    VMSocket(int fd, QObject *parent=0, const char* name=0);
    ~VMSocket();

    QSocketNotifier* getReadNotifier()  { return readNotifier; }
    QSocketNotifier* getWriteNotifier() { return writeNotifier; }
    int              getSockFD()        { return sockFD; }
    pcsl_vmSocket_status vmSocketStatus;
    int readerCount; 
    /* No of readers that are blocked for read operation to this socket */
    int writerCount; 
    /* No of writers that are blocked for write operation to this socket */
 
public slots:
    void readableSlot(int);
    void writableSlot(int);

protected:
    QSocketNotifier* readNotifier;
    QSocketNotifier* writeNotifier;
    int  sockFD;
};

/**
 * Return the VMSocketStatus
 *
 * @param handle platform-specific handle to the open connection
 *
 * @return pcsl_vmSocket_status 
 */
pcsl_vmSocket_status getVMSocketHandleStatus(void *handle); 

/**
 * Set the VMSocketStatus
 *
 * @param handle platform-specific handle to the open connection
 * @param pcsl_vmSocket_status 
 *
 */
void setVMSocketHandleStatus(void *handle, 
            pcsl_vmSocket_status status); 

/**
 * Registers a VMSocket for read. 
 * @param handle platform-specific handle to the open connection
 */
void register_vm_socket_read(void *handle); 

/**
 * Unregisters a VMSocket for read. 
 * @param handle platform-specific handle to the open connection
 */
void Unregister_vm_socket_read(void *handle);

/**
 * Registers a VMSocket for write. 
 * @param handle platform-specific handle to the open connection
 */
void register_vm_socket_write(void *handle);

/**
 * Unregisters a VMSocket for write. 
 * @param handle platform-specific handle to the open connection
 */
void Unregister_vm_socket_write(void *handle); 

/**
 * Gets the platform-specific file descriptor associated with the
 * given handle.
 *
 * @param handle platform-specific handle to the open connection
 *
 * @return the associated platform-specific file descriptor; a negative
 *         number if there was an error
 */
int
getRawSocketFD(void *handle);

/**
 * Creates a platform-specific handle.
 *
 * @param fd platform-specific file descriptor to be associated with
 *           the new handle
 *
 * @return the platform-specific handle; 0 if there was an error
 */
long
createSocketHandle(int fd);

/**
 * Destroys a platform-specific handle and releases any resources used
 * by the handle.
 *
 * @param handle platform-specific handle to destroy
 *
 * @return 0 if successful; a non-zero value if there was an error
 */
void
destroySocketHandle(void *handle);

/**
 * Registers a notification event with the platform-specific event loop.
 * Whenever network activity can occur in the given direction without
 * causing the system to block, a network event is posted to the event
 * queue.
 *  
 * @param handle platform-specific handle to the open connection
 * @param direction whether to notify on a read, write, or both; the value
 *                  of this parameter must be one of <tt>SD_RECV</tt> (for
 *                  reading), <tt>SD_SEND</tt> (for writing), or 
 *                  <tt>SD_BOTH</tt> (for both reading and writing)
 */
void
addSocketNotifier(void *handle, int direction); 

 
#endif


