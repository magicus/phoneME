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
 */

#include <stdio.h>

#include <kni.h>
#include <sni.h>
#include <commonKNIMacros.h>
#include <ROMStructs.h>

#include <midp_libc_ext.h>
#include <kni_globals.h>
#include <midpError.h>
#include <pcsl_network.h>
#include <pcsl_serversocket.h>
#include <push_server_resource_mgmt.h>
#include <midp_properties_port.h>

#include <midp_logging.h>
#include <midpResourceLimit.h>
#include <midp_thread.h>
#include <suitestore_common.h>

/**
 * @file
 *
 * The default implementation of the native functions that are needed
 * for supporting the "serversocket:" Generic Connection protocol.
 */

/* Macro to retrieve C structure representation of an Object */
typedef struct Java_com_sun_midp_io_j2me_serversocket_Socket _serversocketProtocol;
#define getMidpServerSocketProtocolPtr(handle) (unhand(_serversocketProtocol,(handle)))

/* Macro to retrieve C structure representation of an Object */
typedef struct Java_com_sun_midp_io_j2me_socket_Protocol _socketProtocol;
#define getMidpSocketProtocolPtr(handle) (unhand(_socketProtocol,(handle)))

/**
 * Opens a server socket connection on the given port.  If successful,
 * stores a handle directly into the nativeHandle field.  If unsuccessful,
 * throws an exception.
 * <p>
 * Java declaration:
 * <pre>
 *     open0(I[B)V
 * </pre>
 *
 * @param port       TCP port to listen for connections on
 * @param suiteId    ID of current midlet suite, or null if there
 *                   is no current suite
 *
 * @exception IOException  if some other kind of I/O error occurs
 * or if reserved by another suite
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_serversocket_Socket_open0(void) {
    int port;
    jboolean tryOpen;
    SuiteIdType suiteId;
    void *pcslHandle = INVALID_HANDLE;
    int status = PCSL_NET_INVALID;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);

    KNI_GetThisPointer(thisObject);
    port = (int)KNI_GetParameterAsInt(1);
    suiteId = KNI_GetParameterAsInt(2);

    getMidpServerSocketProtocolPtr(thisObject)->nativeHandle = (jint)INVALID_HANDLE;

    /*
     * Determine whether to try opening a socket ourselves, or whether push
     * has made a determination for us.
     */

    if (suiteId == UNUSED_SUITE_ID) {
        tryOpen = KNI_TRUE;
    } else {
        int pushReturn;

        pushReturn = pushcheckout("socket", port,
                                  (char*)midp_suiteid2chars(suiteId));
                                  
        /*
         * pushcheckout() returns -1 if the handle wasn't found, -2 if it's
         * already in use by another suite, otherwise a valid checked-out
         * handle.
         */
        if (pushReturn == -1) {
            tryOpen = KNI_TRUE;
        } else if (pushReturn == -2) {
            KNI_ThrowNew(midpIOException, "socket already in use");
            tryOpen = KNI_FALSE;
        } else {
            /* IMPL NOTE: how to do resource accounting for this case? */
            getMidpServerSocketProtocolPtr(thisObject)->nativeHandle =
                pushReturn;
            tryOpen = KNI_FALSE;
        }
    }

    /*
     * At this point either tryOpen is true; or it's false and we have a valid
     * handle or we've thrown an exception.
     */

    if (tryOpen) {
        if (midpCheckResourceLimit(RSC_TYPE_TCP_SER, 1) == 0) {
            REPORT_INFO(LC_PROTOCOL,
                        "Resource limit exceeded for TCP server sockets");
            KNI_ThrowNew(midpIOException,
                         "Resource limit exceeded for TCP server sockets");
        } else {
            status = pcsl_serversocket_open(
                port, &pcslHandle);

            if (status == PCSL_NET_SUCCESS) {
                getMidpServerSocketProtocolPtr(thisObject)->nativeHandle = (jint)pcslHandle;
                REPORT_INFO2(LC_PROTOCOL,
                             "serversocket::open port = %d handle = %d\n",
                             port, pcslHandle);

                if (midpIncResourceCount(RSC_TYPE_TCP_SER, 1) == 0) {
                    REPORT_INFO(LC_PROTOCOL,
                                "TCP Server: Resource limit update error");
                }
            } else if (status == PCSL_NET_IOERROR) {
                midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                              "IOError in serversocket::open = %d\n",
                              pcsl_network_error(pcslHandle));
                REPORT_INFO1(LC_PROTOCOL, "%s\n", gKNIBuffer);
                KNI_ThrowNew(midpIOException, gKNIBuffer);
            } else {
                REPORT_INFO(LC_PROTOCOL, "Unknown error during serversocket::open");
                KNI_ThrowNew(midpIOException, NULL);
            }
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Closes the connection.
 * <p>
 * Java declaration:
 * <pre>
 *     close0(V)V
 * </pre>
 *
 * @exception  IOException  if an I/O error occurs when closing the
 *                          connection
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_serversocket_Socket_close0(void) {
    int serverSocketHandle;
    int status = PCSL_NET_INVALID;
    void* context = NULL;
    MidpReentryData* info;
    int resUpdate = 0;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    info = (MidpReentryData*)SNI_GetReentryData(NULL);
    if (info == NULL) {
        /* initial invocation */
        serverSocketHandle =
            getMidpServerSocketProtocolPtr(thisObject)->nativeHandle;

        /*
         * If serverSocketHandle is invalid, the socket has already been closed,
         * so per specification we do nothing and return.
         */
        if (serverSocketHandle == (int)INVALID_HANDLE) {
            REPORT_INFO(LC_PROTOCOL, "serversocket::close Invalid handle\n");
        } else {
            /*
             * The pushcheckin() function returns -1 on error.  If pushcheckin()
             * was successful, socket was checked back into the push registry, and
             * we needn't do anything further. Otherwise, we need to perform close
             * processing ourselves.
             *
             * IMPL NOTE: how to do resource accounting for the push case?
             */
            if (pushcheckin(serverSocketHandle) == -1) {
                status = pcsl_socket_close_start((void*)serverSocketHandle,
                                                 &context);
                resUpdate = 1;
            } else {
                status = PCSL_NET_SUCCESS;
            }
            /* Server socket should be monitored for read events only */
            midp_thread_signal(NETWORK_READ_SIGNAL, serverSocketHandle, 0);

            getMidpServerSocketProtocolPtr(thisObject)->nativeHandle =
                (jint)INVALID_HANDLE;
        }
    } else {
        /* reinvocation */
        serverSocketHandle = info->descriptor;
        context = info->pResult;
        status = pcsl_socket_close_finish((void*)serverSocketHandle, context);
    }

    REPORT_INFO1(LC_PROTOCOL, "serversocket::close handle=%d\n", serverSocketHandle);

    if (serverSocketHandle != (int)INVALID_HANDLE) {
        if (status == PCSL_NET_SUCCESS) {
          if (resUpdate) {
            if (midpDecResourceCount(RSC_TYPE_TCP_SER, 1) == 0) {
                REPORT_INFO(LC_PROTOCOL,
                            "TCP Server: Resource limit update error");
            }
          }
        } else if (status == PCSL_NET_WOULDBLOCK) {
            REPORT_INFO1(LC_PROTOCOL, "serversocket::close = 0x%x blocked\n",
                         serverSocketHandle);
            /* IMPL NOTE: is this the right signal? */
            midp_thread_wait(NETWORK_EXCEPTION_SIGNAL,
                             serverSocketHandle, context);
        } else {
            midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                          "IOError in serversocket::close = %d\n",
                          pcsl_network_error((void*)serverSocketHandle));
            REPORT_INFO1(LC_PROTOCOL, "%s", gKNIBuffer);
            KNI_ThrowNew(midpIOException, gKNIBuffer);
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Polls a server socket for an incoming TCP connection. If there is an
 * incoming connection, it is accepted and this method returns true. If
 * there is no incoming connection, this method returns false immediately.
 * This requires callers to call this method repeatedly (often in a
 * busy-wait loop) while awaiting an incoming connection.
 * <p>
 * The 'con' parameter must be a freshly created client socket connection
 * object.  If an incoming connection is accepted, the socket handle for
 * the newly accepted connection is stored into this object directly from
 * native code. This technique ensures that the acceptance of a new
 * connection and the storing of the native handle are performed
 * atomically.
 * <p>
 * Java declaration:
 * <pre>
 *     accept0(Lcom/sun/midp/io/j2me/socket/Protocol;)V
 * </pre>
 *
 * @param con the client socket connection object
 *
 * @return true if a connection was made, otherwise false
 *
 * @exception IOException if an I/O error has occurred
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_serversocket_Socket_accept0(void) {
    int serverSocketHandle;
    MidpReentryData* info;
    void* connectionHandle = INVALID_HANDLE;
    int status = PCSL_NET_INVALID;
    int processStatus = KNI_FALSE;
    void *context = NULL;

    KNI_StartHandles(2);
    KNI_DeclareHandle(thisObject);
    KNI_DeclareHandle(socketObject);

    KNI_GetThisPointer(thisObject);
    KNI_GetParameterAsObject(1, socketObject);

    serverSocketHandle =
        getMidpServerSocketProtocolPtr(thisObject)->nativeHandle;

    if (serverSocketHandle == (int)INVALID_HANDLE) {
        KNI_ThrowNew(midpIOException, "Socket was closed");
    } else {
        info = (MidpReentryData*)SNI_GetReentryData(NULL);
        if (info == NULL) {   /* First invocation */
            REPORT_INFO1(LC_PROTOCOL, "serversocket::accept handle=%d\n",
                         serverSocketHandle);

            /*
             * pushcheckoutaccept() returns -1 if nothing was checked out, so we
             * have to do the accept operation ourselves.
             *
             * If a connection was checked out of the push registry, we needn't do
             * anything else.
             *
             * IMPL NOTE: how to do resource accounting for the push case?
             */
            connectionHandle = (void*)pushcheckoutaccept(serverSocketHandle);
            if (connectionHandle == (void*)-1) {
                /*
                 * An incoming socket connection counts against the client socket
                 * resource limit.
                 */
                if (midpCheckResourceLimit(RSC_TYPE_TCP_CLI, 1) == 0) {
                    REPORT_INFO(LC_PROTOCOL,
                                "Resource limit exceeded for TCP client sockets");
                    KNI_ThrowNew(midpIOException,
                                 "Resource limit exceeded for TCP client sockets");
                } else {
                    status = pcsl_serversocket_accept_start(
                        (void*)serverSocketHandle, &connectionHandle, &context);

                    processStatus = KNI_TRUE;
                }
            }
        } else {  /* Reinvocation after unblocking the thread */
            if (info->descriptor != serverSocketHandle) {
                midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                              "serversocket::accept Handles mismatched 0x%x != 0x%x\n",
                              serverSocketHandle,
                              info->descriptor);
                REPORT_CRIT(LC_PROTOCOL, gKNIBuffer);
                KNI_ThrowNew(midpIllegalStateException, gKNIBuffer);
            } else {
                if (midpCheckResourceLimit(RSC_TYPE_TCP_CLI, 1) == 0) {
                    REPORT_INFO(LC_PROTOCOL,
                                "Resource limit exceeded for TCP client sockets");
                    KNI_ThrowNew(midpIOException,
                                 "Resource limit exceeded for TCP client sockets");
                } else {
                    status = pcsl_serversocket_accept_finish(
                        (void*)serverSocketHandle, &connectionHandle, &context);

                    processStatus = KNI_TRUE;
                }
            }
        }

        if (processStatus) {
            REPORT_INFO1(LC_PROTOCOL,
                         "serversocket::accept connection handle=%d\n",
                         connectionHandle);
            if (status == PCSL_NET_SUCCESS) {
                if (midpIncResourceCount(RSC_TYPE_TCP_CLI, 1) == 0) {
                    REPORT_INFO(LC_PROTOCOL,
                                "serversocket: Resource limit update error");
                }
            } else if (status == PCSL_NET_WOULDBLOCK) {
                midp_thread_wait(NETWORK_READ_SIGNAL,
                                 serverSocketHandle, context);
            } else if (status == PCSL_NET_IOERROR) {
                midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                              "IOError in serversocket::accept = %d\n",
                              pcsl_network_error((void*)serverSocketHandle));
                REPORT_INFO1(LC_PROTOCOL, "%s\n", gKNIBuffer);
                KNI_ThrowNew(midpIOException, gKNIBuffer);
            } else {
                REPORT_INFO(LC_PROTOCOL, "Unknown error during serversocket::accept");
                KNI_ThrowNew(midpIOException, NULL);
            }
        }

        if (connectionHandle != (void*)-1) {
            /*
             * We got a valid connection, either by checking it out of the
             * push registry, or by accepting an incoming connection from the
             * platform.
             */
            (getMidpSocketProtocolPtr(socketObject))->handle =
                (jint)connectionHandle;
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}


/**
 * Releases any native resources used by the server socket connection.
 * <p>
 * Java declaration:
 * <pre>
 *     finalize(V)V
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_serversocket_Socket_finalize(void) {
    int serverSocketHandle;
    void* context = NULL;
    int status = PCSL_NET_INVALID;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    serverSocketHandle =
        getMidpServerSocketProtocolPtr(thisObject)->nativeHandle;

    REPORT_INFO1(LC_PROTOCOL, "serversocket::finalize handle=%d",
        serverSocketHandle);

    if (serverSocketHandle != (int)INVALID_HANDLE) {
        if (pushcheckin(serverSocketHandle) == -1) {
            status = pcsl_socket_close_start(
                (void*)serverSocketHandle, &context);
            if (midpDecResourceCount(RSC_TYPE_TCP_SER, 1) == 0) {
                REPORT_INFO(LC_PROTOCOL,
                    "TCP Server : Resource limit update error");
            }

            if (status == PCSL_NET_IOERROR) {
                midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                              "IOError in serversocket::finalize error=%d\n",
                              pcsl_network_error((void*)serverSocketHandle));
                REPORT_ERROR1(LC_PROTOCOL, "%s", gKNIBuffer);
            } else if (status == PCSL_NET_WOULDBLOCK) {
                /* blocking during finalize is not supported */
                REPORT_CRIT1(LC_PROTOCOL, "serversocket::finalize = 0x%x blocked\n",
                             serverSocketHandle);
            }
        }
        getMidpServerSocketProtocolPtr(thisObject)->nativeHandle =
            (jint)INVALID_HANDLE;
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Gets the local IP number.
 * <p>
 * Java declaration:
 * <pre>
 *     getLocalAddress0(V)Ljava/lang/String;
 * </pre>
 *
 * @return the IP address as a dotted-quad <tt>String</tt>
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_io_j2me_serversocket_Socket_getLocalAddress0(void) {
    int serverSocketHandle;
    char value[MAX_HOST_LENGTH];
    int status;

    KNI_StartHandles(2);
    KNI_DeclareHandle(thisObject);
    KNI_DeclareHandle(result);
    KNI_GetThisPointer(thisObject);

    serverSocketHandle =
        getMidpServerSocketProtocolPtr(thisObject)->nativeHandle;

    if (serverSocketHandle != (int)INVALID_HANDLE) {
        status = pcsl_network_getLocalIPAddressAsString(value);

        if (status == PCSL_NET_SUCCESS) {
            KNI_NewStringUTF(value, result);
        } else {
            KNI_ReleaseHandle(result);
        }
    } else {
        KNI_ThrowNew(midpIOException, NULL);
    }

    KNI_EndHandlesAndReturnObject(result);
}

/**
 * Gets the local port to which this socket connection is bound.
 * <p>
 * Java declaration:
 * <pre>
 *     getLocalPort0(V)I
 * </pre>
 *
 * @param handle the native handle to the network connection.
 *
 * @return the local port number for this socket connection
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_serversocket_Socket_getLocalPort0(void) {
    int port = -1;
    int serverSocketHandle;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    serverSocketHandle =
        getMidpServerSocketProtocolPtr(thisObject)->nativeHandle;

    if (serverSocketHandle != (int)INVALID_HANDLE) {
        int status;

        /*
         * IMPL NOTE: even though there is currently no PCSL serversocket
         * implementation, it HAPPENS to work to call PCSL's getlocalport
         * because on Linux and Win32, the old porting layer and PCSL both use
         * a socket descriptor as the handle.
         */
        status = pcsl_network_getlocalport((void *) serverSocketHandle, &port);
        if (status != PCSL_NET_SUCCESS) {
            KNI_ThrowNew(midpIOException, "I/O error");
        }
    } else {
        KNI_ThrowNew(midpIOException, "socket closed");
    }

    KNI_EndHandles();
    KNI_ReturnInt((jint)port);
}
