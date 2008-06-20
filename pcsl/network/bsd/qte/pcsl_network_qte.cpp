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

/**
 * @file
 *
 * Methods to associate network events with the main Qt event loop.
 */

#include <qsocketnotifier.h>
#include <stdlib.h>
#include <stdio.h>

#include <qmetaobject.h>
#include <qapplication.h>
#include <pcsl_network.h>
#include <pcsl_network_qte.h>
#include <moc_pcsl_network_qte.cpp>

extern "C" void NotifySocketStatusChanged(long handle, int waitingFor); 

VMSocket::VMSocket(int fd, QObject *parent, const char* name):
    QObject(parent, name)
{
    sockFD = fd;
    readerCount = 0;
    writerCount = 0;
    vmSocketStatus = SOCKET_STATUS_IDLE;
    readNotifier = new QSocketNotifier(sockFD, QSocketNotifier::Read);
    writeNotifier = new QSocketNotifier(sockFD, QSocketNotifier::Write);
    connect(readNotifier, SIGNAL(activated(int)), 
	    this, SLOT(readableSlot(int)));
    connect(writeNotifier, SIGNAL(activated(int)), 
	    this, SLOT(writableSlot(int)));
    writeNotifier->setEnabled(false);
    readNotifier->setEnabled(false);
}

VMSocket::~VMSocket() {
    delete readNotifier;
    delete writeNotifier;
}

void VMSocket::readableSlot(int socket) {
    readNotifier->setEnabled(false);

    /* Make an up-call to unblock the thread */
    NotifySocketStatusChanged((long)this, SD_RECV);
       
    (void)socket; // avoid a compiler warning
}

void VMSocket::writableSlot(int socket) {
    writeNotifier->setEnabled(false);

    /* Make an up-call to unblock the thread */
    NotifySocketStatusChanged((long)this, SD_SEND);
 
    (void)socket; // avoid a compiler warning
}

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
addSocketNotifier(void *handle, int direction) {
    if (handle != 0) {
        VMSocket* s = (VMSocket*)(handle);

        switch(direction) {
            case SD_NONE: s->getReadNotifier()->setEnabled(false);
                          s->getWriteNotifier()->setEnabled(false);
                          break;

            case SD_RECV: s->getReadNotifier()->setEnabled(true);
                          break;

            case SD_SEND: s->getWriteNotifier()->setEnabled(true);
                          break;

            /* IMPL_NOTE: investigate who calls addSocketNotifier with SD_BOTH */
            case SD_BOTH: s->getWriteNotifier()->setEnabled(true);
                          s->getWriteNotifier()->setEnabled(true);
                          break;

                default : break;
        }

        /*
        if (direction == SD_NONE) {
            s->getReadNotifier()->setEnabled(false);
            s->getWriteNotifier()->setEnabled(false);
        }
        if ((direction == SD_RECV) || (direction == SD_BOTH)) {
            s->getReadNotifier()->setEnabled(true);
        }
        if ((direction == SD_SEND) || (direction == SD_BOTH)) {
            s->getWriteNotifier()->setEnabled(true);
        }
        */
    }
}

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
getRawSocketFD(void *handle) {
    if (handle != NULL) {
        //VMSocket* s = static_cast<VMSocket*>((void*)handle);
        VMSocket* s = (VMSocket*)handle;
        return s->getSockFD();
    }

    return -1;
}

/**
 * Return the VMSocketStatus
 */
pcsl_vmSocket_status
getVMSocketHandleStatus(void *handle) {
    VMSocket *s;
    s = (VMSocket*)(handle);
    return s->vmSocketStatus;
}

/**
 * Registers a VMSocket for read. 
 */
void 
register_vm_socket_read(void *handle) {
    VMSocket *s;
    s = (VMSocket*)(handle);

    /* Set the VMSocket status to SOCKET_STATUS_PENDING_READ */
    s->vmSocketStatus = SOCKET_STATUS_PENDING_READ;

    /* Increment the reader count */
    s->readerCount++;

    /* 
     * Enable the read socket notifier. This is analogous to
     * addSocketNotifier(handle, SD_RECV) 
     */
    s->getReadNotifier()->setEnabled(true);
}

/**
 * Unregisters a VMSocket for read. 
 */
void
Unregister_vm_socket_read(void *handle) {
    VMSocket *s;
    s = (VMSocket*)(handle);

    /* Set the VMSocket status to SOCKET_STATUS_IDLE */
    s->vmSocketStatus = SOCKET_STATUS_IDLE;

    /* Decrement the reader count */
    s->readerCount--;

    /* 
     * Re-enable the read socket notifier if reader count > 0. It is
     * necessary to do this as it will make sure that other threads 
     * which are blocked for read for the same socket receive callback
     * upon arrival of data at the socket.
     * The same effect could also have been achieved by not disabling 
     * the read notifier in readableSlot(), but it would have affected 
     * the push operation. Moreover Qt socket will keep on emitting 
     * read notifications until all data has been read at the socket in 
     * which case there is a chance of calling NotifyStatusChanged multiple
     * times.
     * The best bet is to re-enable the socket notifier when we know that
     * there are already other threads blocked for read operation. The 
     * read notifier will remain disabled when all threads are done 
     */
    if (s->readerCount > 0) {
        s->getReadNotifier()->setEnabled(true);
    }
}

/**
 * Registers a VMSocket for write. 
 */
void 
register_vm_socket_write(void *handle) {
    VMSocket *s;
    s = (VMSocket*)(handle);

    /* Set the VMSocket status to SOCKET_STATUS_PENDING_READ */
    s->vmSocketStatus = SOCKET_STATUS_PENDING_WRITE;

    /* Increment the writer count */
    s->writerCount++;

    /* 
     * Enable the write socket notifier. This is analogous to
     * addSocketNotifier(handle, SD_SEND) 
     */
    s->getWriteNotifier()->setEnabled(true);
}

/**
 * Unregisters a VMSocket for write. 
 */
void
Unregister_vm_socket_write(void *handle) {
    VMSocket *s;
    s = (VMSocket*)(handle);

    /* Set the VMSocket status to SOCKET_STATUS_IDLE */
    s->vmSocketStatus = SOCKET_STATUS_IDLE;

    /* Decrement the writer count */
    s->writerCount--;

    /* 
     * Re-enable the write socket notifier if writer count > 0. It is
     * necessary to do this as it will make sure that other threads 
     * which are blocked for write for the same socket will also be
     * able to receive callback
     * 
     * According to Qt documentation, the behaviour of a write notifier 
     * that is left in enabled state after having emitting the first 
     * activated() signal (and no "would block" error has occurred) 
     * is undefined. So it is necessary to disable the write notifiers 
     * whenever they are no longer required. We are re-enabling them 
     * only if there are other threads blocked for write to the same 
     * socket
     */
    if (s->writerCount > 0) {
        s->getWriteNotifier()->setEnabled(true);
    }
}

/**
 * Sets the VMSocket status.
 *
 * @param handle platform-specific handle to destroy
 * @param status Enumerated VMSocket status 
 *
 */
void
setVMSocketHandleStatus(void *handle, pcsl_vmSocket_status status) {
    VMSocket *s;

    s = (VMSocket*)(handle);
    s->vmSocketStatus = status;
}

/**
 * Creates a platform-specific handle.
 *
 * @param fd platform-specific file descriptor to be associated with
 *           the new handle
 *
 * @return the platform-specific handle; 0 if there was an error
 */
long
createSocketHandle(int fd) {
    VMSocket* s;

    s = new VMSocket(fd);
    return (long)s;
}

/**
 * Destroys a platform-specific handle and releases any resources used
 * by the handle.
 *
 * @param handle platform-specific handle to destroy
 *
 * @return 0 if successful; a non-zero value if there was an error
 */
void
destroySocketHandle(void *handle) {
    VMSocket *s;

    s = (VMSocket*)(handle);
    /*
    printf("destroying fd:%d\n", s->getSockFD());
	printf("Readercount : %d\n", s->readerCount); 
	printf("Writercount : %d\n", s->writerCount); 
    */

    delete s;
}

