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

#include <anc_indicators.h>
#include <midpError.h>
#include <push_server_export.h>
#include <push_server_resource_mgmt.h>
#include <midp_properties_port.h>
#include <midp_logging.h>
#include <midpResourceLimit.h>
#include <string.h>
#include <pcsl_network.h>
#include <midp_thread.h>
#include <midp_libc_ext.h>
#include <kni_globals.h>
#include <midpUtilKni.h>
#include <midp_net_events.h>

#define SOCK_ANC_INC_NETWORK_INDICATOR \
    ANC_INC_NETWORK_INDICATOR \
    getMidpSocketProtocolPtr(thisObject)->pendingConnections++;

#define SOCK_ANC_DEC_NETWORK_INDICATOR \
    ANC_DEC_NETWORK_INDICATOR \
    getMidpSocketProtocolPtr(thisObject)->pendingConnections--;

/**
 * @file
 * 
 * The default implementation of the native functions that are needed
 * for supporting the "socket:" Generic Connection protocols.
 */

/* Macro to retrieve C structure representation of an Object */
typedef struct Java_com_sun_midp_io_j2me_socket_Protocol _socketProtocol;
#define getMidpSocketProtocolPtr(handle) (unhand(_socketProtocol,(handle)))

/**
 * Opens a TCP connection to a server.
 * <p>
 * Java declaration:
 * <pre>
 *     open([BI)V
 * </pre>
 *
 * @param ipBytes Byte array that represents a raw IP address
 * @param port TCP port at host
 *
 * @return a native handle to the network connection.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_socket_Protocol_open0(void) {
    int   port;
    void *pcslHandle = INVALID_HANDLE;
    int status;
    void* context = NULL;
    MidpReentryData* info;

    port = (int)KNI_GetParameterAsInt(2);

    KNI_StartHandles(2);
    KNI_DeclareHandle(thisObject);
    KNI_DeclareHandle(bufferObject);
    KNI_GetThisPointer(thisObject);
    KNI_GetParameterAsObject(1, bufferObject);

    info = (MidpReentryData*)SNI_GetReentryData(NULL);
    if (info == NULL) {   /* First invocation */
       getMidpSocketProtocolPtr(thisObject)->handle = (jint)INVALID_HANDLE;

       /**
         * Verify that the resource is available well within limit as per 
         * the policy in ResourceLimiter
         */
        if (midpCheckResourceLimit(RSC_TYPE_TCP_CLI, 1) == 0) {
            REPORT_INFO(LC_PROTOCOL,
                "Resource limit exceeded for TCP client sockets"); 
            KNI_ThrowNew(midpIOException,
                "Resource limit exceeded for TCP client sockets");
        } else {
            SNI_BEGIN_RAW_POINTERS;
            status = pcsl_socket_open_start(
                     (unsigned char*)JavaByteArray(bufferObject),
                     port, &pcslHandle, &context);
            SNI_END_RAW_POINTERS;

            if (status == PCSL_NET_SUCCESS) {
                getMidpSocketProtocolPtr(thisObject)->handle = (jint)pcslHandle;
                if (midpIncResourceCount(RSC_TYPE_TCP_CLI, 1) == 0) {
                    REPORT_INFO(LC_PROTOCOL, "Resource limit update error"); 
                }
            } else if (status == PCSL_NET_IOERROR) {
                midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                        "IOError in socket::open = %d\n",
                        (int)pcsl_network_error(pcslHandle));
                REPORT_INFO1(LC_PROTOCOL, "%s\n", gKNIBuffer);
                KNI_ThrowNew(midpIOException, gKNIBuffer);
            } else if (status == PCSL_NET_CONNECTION_NOTFOUND) {
                midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE, 
                        "ConnectionNotFound error in socket::open :" 
                        " error = %d\n", (int)pcsl_network_error(pcslHandle));
                REPORT_INFO1(LC_PROTOCOL, "%s\n", gKNIBuffer); 
                KNI_ThrowNew(midpConnectionNotFoundException, gKNIBuffer);
            } else if (status == PCSL_NET_WOULDBLOCK) {
                SOCK_ANC_INC_NETWORK_INDICATOR;
                getMidpSocketProtocolPtr(thisObject)->handle = (jint)pcslHandle;
                if (midpIncResourceCount(RSC_TYPE_TCP_CLI, 1) == 0) {
                    REPORT_INFO(LC_PROTOCOL, "Resource limit update error"); 
                }
                REPORT_INFO1(LC_PROTOCOL, " handle = %d\n", pcslHandle);
                midp_thread_wait(NETWORK_WRITE_SIGNAL, (int)pcslHandle,
                    context);
            } else {
                REPORT_INFO(LC_PROTOCOL, "Unknown error during socket::open"); 
                KNI_ThrowNew(midpIOException, NULL);
            }
        }
    } else {  /* Reinvocation after unblocking the thread */
        pcslHandle = (void *) info->descriptor;
        context = (void *)info->status;

        if (getMidpSocketProtocolPtr(thisObject)->handle != (jint)pcslHandle) {
            REPORT_CRIT2(LC_PROTOCOL, 
                         "socket::open Handles mismatched 0x%x != 0x%x\n", 
                         pcslHandle,
                         getMidpSocketProtocolPtr(thisObject)->handle);
        }

        status = pcsl_socket_open_finish(pcslHandle, context);

        if (status == PCSL_NET_SUCCESS) {
            SOCK_ANC_DEC_NETWORK_INDICATOR;
        } else if (status == PCSL_NET_WOULDBLOCK) {
            midp_thread_wait(NETWORK_WRITE_SIGNAL, (int)pcslHandle, context);
        } else  {
            SOCK_ANC_DEC_NETWORK_INDICATOR;
            getMidpSocketProtocolPtr(thisObject)->handle = (jint)INVALID_HANDLE;
            if (midpDecResourceCount(RSC_TYPE_TCP_CLI, 1) == 0) {
                REPORT_INFO(LC_PROTOCOL, "Resource limit update error"); 
            }
            midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                    "error %d in socket::open",
                    (int)pcsl_network_error(pcslHandle));
            REPORT_INFO1(LC_PROTOCOL, "%s\n", gKNIBuffer); 
            KNI_ThrowNew(midpConnectionNotFoundException, gKNIBuffer);
        }
    }

    ANC_IND_NETWORK_INDICATOR;

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Reads from the open socket connection.
 * <p>
 * Java declaration:
 * <pre>
 *     read0([BII)I
 * </pre>
 *
 * @param b the buffer into which the data is read.
 * @param off the start offset in array <code>b</code>
 *            at which the data is written.
 * @param len the maximum number of bytes to read.
 *
 * @return the total number of bytes read into the buffer, or
 *         <tt>-1</tt> if there is no more data because the end of
 *         the stream has been reached.
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_socket_Protocol_read0(void) {
    int length;
    int offset;
    int iStreams;
    void *pcslHandle;
    int bytesRead = -1;
    int status = PCSL_NET_INVALID;
    void* context = NULL;
    MidpReentryData* info;
    
    length = (int)KNI_GetParameterAsInt(3);
    offset = (int)KNI_GetParameterAsInt(2);

    KNI_StartHandles(2);
    
    KNI_DeclareHandle(bufferObject);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);
    KNI_GetParameterAsObject(1, bufferObject);
    
    pcslHandle = (void *)(getMidpSocketProtocolPtr(thisObject)->handle);
    iStreams = (int)(getMidpSocketProtocolPtr(thisObject)->iStreams); 

    REPORT_INFO3(LC_PROTOCOL, "socket::read0 o=%d l=%d fd=%d\n",
                 offset, length, (int)pcslHandle);

    if (pcslHandle != INVALID_HANDLE) {
        int ipAddress;
        int port;

        /* Check the push cache for a waiting packet. */
        SNI_BEGIN_RAW_POINTERS;
        bytesRead = pushgetcachedpacket((int)pcslHandle, &ipAddress, &port,
            (char*)&(JavaByteArray(bufferObject)[offset]), length);
        SNI_END_RAW_POINTERS;
    }

    if (bytesRead <= 0) {
        info = (MidpReentryData*)SNI_GetReentryData(NULL);

        ANC_IND_NETWORK_INDICATOR;

        if (info == NULL) {   /* First invocation */
            if (INVALID_HANDLE == pcslHandle) {
                KNI_ThrowNew(midpIOException, "invalid handle during socket::read");
            } else {
                SOCK_ANC_INC_NETWORK_INDICATOR;
                SNI_BEGIN_RAW_POINTERS;
                status = pcsl_socket_read_start(pcslHandle,
                               (unsigned char*)&(JavaByteArray(bufferObject)[offset]),
                               length, &bytesRead, &context);
                SNI_END_RAW_POINTERS;
            }
        } else {  /* Reinvocation after unblocking the thread */
            if (INVALID_HANDLE == pcslHandle || iStreams == 0) {
                /* connection or its input streams are closed by another thread */
                KNI_ThrowNew(midpInterruptedIOException,
                             "Interrupted IO error during socket::read");
                if (INVALID_HANDLE == pcslHandle) {
                    SOCK_ANC_DEC_NETWORK_INDICATOR;
                }
            } else {
                if ((void *)info->descriptor != pcslHandle) {
                    REPORT_CRIT2(LC_PROTOCOL,
                                 "socket::read Handles mismatched 0x%x != 0x%x\n",
                                 pcslHandle,
                                 info->descriptor);
                }
                context = info->pResult;
                SNI_BEGIN_RAW_POINTERS;
                status = pcsl_socket_read_finish(pcslHandle,
                           (unsigned char*)&(JavaByteArray(bufferObject)[offset]),
                           length, &bytesRead, context);
                SNI_END_RAW_POINTERS;
            }
        }

        REPORT_INFO1(LC_PROTOCOL, "socket::read0 bytesRead=%d\n", bytesRead);

        if (INVALID_HANDLE != pcslHandle) {
            if (status == PCSL_NET_SUCCESS) {
                if (bytesRead == 0) {
                    /* end of stream */
                    bytesRead = -1;
                }
                SOCK_ANC_DEC_NETWORK_INDICATOR;
            } else {
                REPORT_INFO1(LC_PROTOCOL, "socket::read error=%d\n",
                             pcsl_network_error(pcslHandle));

                if (status == PCSL_NET_WOULDBLOCK) {
                    midp_thread_wait(NETWORK_READ_SIGNAL, (int)pcslHandle, context);
                } else if (status == PCSL_NET_INTERRUPTED) {
                    midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                            "Interrupted IO error %d during socket::read ",
                            pcsl_network_error(pcslHandle));
                    KNI_ThrowNew(midpInterruptedIOException, gKNIBuffer);
                    SOCK_ANC_DEC_NETWORK_INDICATOR;
                } else {
                    midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                            "Unknown error %d during socket::read ",
                            pcsl_network_error(pcslHandle));
                    KNI_ThrowNew(midpIOException, gKNIBuffer);
                    SOCK_ANC_DEC_NETWORK_INDICATOR;
                }
            }
        }

        ANC_IND_NETWORK_INDICATOR;
    }

    KNI_EndHandles();
    KNI_ReturnInt((jint)bytesRead);
}

/**
 * Writes to the open socket connection.
 * <p>
 * Java declaration:
 * <pre>
 *     write0([BII)I
 * </pre>
 *
 * @param b the buffer of the data to write
 * @param off the start offset in array <tt>b</tt>
 *            at which the data is written.
 * @param len the number of bytes to write.
 *
 * @return the total number of bytes written
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_socket_Protocol_write0(void) { 
    int length;
    int offset;
    int oStreams;
    void *pcslHandle;
    int bytesWritten = 0;
    int status = PCSL_NET_INVALID;
    void *context = NULL;
    MidpReentryData* info;

    length = (int)KNI_GetParameterAsInt(3);
    offset = (int)KNI_GetParameterAsInt(2);

    KNI_StartHandles(2);

    KNI_DeclareHandle(bufferObject);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);
    KNI_GetParameterAsObject(1, bufferObject);

    pcslHandle = (void *)(getMidpSocketProtocolPtr(thisObject)->handle);
    oStreams = (int)(getMidpSocketProtocolPtr(thisObject)->oStreams);

    REPORT_INFO3(LC_PROTOCOL, "socket::write0 o=%d l=%d fd=%d\n", 
                 offset, length, pcslHandle);

    info = (MidpReentryData*)SNI_GetReentryData(NULL);

    ANC_IND_NETWORK_INDICATOR;

    if (info == NULL) {   /* First invocation */
        if (INVALID_HANDLE == pcslHandle) {
            KNI_ThrowNew(midpIOException, 
                         "invalid handle during socket::write");
        } else {
            SOCK_ANC_INC_NETWORK_INDICATOR;
            SNI_BEGIN_RAW_POINTERS;
            status = pcsl_socket_write_start(pcslHandle, 
                           (char*)&(JavaByteArray(bufferObject)[offset]),
                           length, &bytesWritten, &context);
            SNI_END_RAW_POINTERS;
        }
    } else { /* Reinvocation after unblocking the thread */
        if (INVALID_HANDLE == pcslHandle || oStreams == 0) {
            /* connection or its output streams are closed by another thread */
            KNI_ThrowNew(midpInterruptedIOException, 
                         "Interrupted IO error during socket::write");
            SOCK_ANC_DEC_NETWORK_INDICATOR;
        } else {
            if ((void *)info->descriptor != pcslHandle) {
                REPORT_CRIT2(LC_PROTOCOL, 
                             "socket::write Handles mismatched 0x%x != 0x%x\n", 
                             pcslHandle,
                             info->descriptor);
            }
            context = info->pResult;
            SNI_BEGIN_RAW_POINTERS;
            status = pcsl_socket_write_finish(pcslHandle, 
                       (char*)&(JavaByteArray(bufferObject)[offset]),
                       length, &bytesWritten, context);
            SNI_END_RAW_POINTERS;
        }
    }

    if (INVALID_HANDLE != pcslHandle) {
        if (status == PCSL_NET_SUCCESS) {
            SOCK_ANC_DEC_NETWORK_INDICATOR;
        } else {
            REPORT_INFO1(LC_PROTOCOL, "socket::write error=%d\n", 
                         (int)pcsl_network_error(pcslHandle));

            if (status == PCSL_NET_WOULDBLOCK) {
                midp_thread_wait(NETWORK_WRITE_SIGNAL, (int)pcslHandle, context);
            } else if (status == PCSL_NET_INTERRUPTED) {
                midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                        "Interrupted IO error %d during socket::write ", 
                        pcsl_network_error(pcslHandle));
                KNI_ThrowNew(midpInterruptedIOException, gKNIBuffer);
                SOCK_ANC_DEC_NETWORK_INDICATOR;
            } else {
                midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                        "IOError %d during socket:: write \n", 
                        pcsl_network_error(pcslHandle));
                KNI_ThrowNew(midpIOException, gKNIBuffer);
                SOCK_ANC_DEC_NETWORK_INDICATOR;
            }
        }
    }

    REPORT_INFO1(LC_PROTOCOL, "socket::write0 bytesWritten=%d\n", 
                 bytesWritten);
   ANC_IND_NETWORK_INDICATOR;
    KNI_EndHandles();

    KNI_ReturnInt((jint)bytesWritten);
}

/**
 * Gets the number of bytes that can be read without blocking.
 * <p>
 * Java declaration:
 * <pre>
 *     available0(V)I
 * </pre>
 *
 * @return number of bytes that can be read without blocking
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_socket_Protocol_available0(void) {
    void *pcslHandle;
    int bytesAvailable = 0;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);
    
    pcslHandle = (void *)(getMidpSocketProtocolPtr(thisObject)->handle);

    KNI_EndHandles();

    REPORT_INFO1(LC_PROTOCOL, "socket::available0 fd=%d\n", (int)pcslHandle);

    if (INVALID_HANDLE == pcslHandle) {
        KNI_ThrowNew(midpIOException, "invalid handle during socket::available");
    } else {
        int status;

        /* Check the push cache for a waiting packet. */
        bytesAvailable = pushcacheddatasize((int)pcslHandle);
        if (bytesAvailable <= 0) {
            status = pcsl_socket_available(pcslHandle, &bytesAvailable);
            /* status is only PCSL_NET_SUCCESS or PCSL_NET_IOERROR */
            if (status == PCSL_NET_IOERROR) {
                bytesAvailable = 0;
                midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                        "IOError %d during socket::available0",
                        pcsl_network_error(pcslHandle));
                KNI_ThrowNew(midpIOException, gKNIBuffer);
            }
        }
    }

    REPORT_INFO1(LC_PROTOCOL, "socket::available0 bytesAvailable=%d\n", 
                 bytesAvailable);

    KNI_ReturnInt((jint)bytesAvailable);
}

/**
 * Closes the socket connection.
 * <p>
 * Java declaration:
 * <pre>
 *     close0(V)V
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_socket_Protocol_close0(void) {
    void *pcslHandle;
    int status = PCSL_NET_INVALID;
    void* context = NULL;
    MidpReentryData* info;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);


    info = (MidpReentryData*)SNI_GetReentryData(NULL);
    if (info == NULL) {
        /* initial invocation */
        pcslHandle = (void *)(getMidpSocketProtocolPtr(thisObject)->handle);

        if (INVALID_HANDLE == pcslHandle) {
            KNI_ThrowNew(midpIOException,
                "invalid handle during socket::close");
        } else {
            status = pcsl_socket_close_start(pcslHandle, &context);

            getMidpSocketProtocolPtr(thisObject)->handle =
                (jint)INVALID_HANDLE;

            midp_thread_signal(NETWORK_READ_SIGNAL, (int)pcslHandle, 0);
            midp_thread_signal(NETWORK_WRITE_SIGNAL, (int)pcslHandle, 0);
        }
    } else {
        /* reinvocation */
        pcslHandle = (void *)(info->descriptor);
        context = info->pResult;
        status = pcsl_socket_close_finish(pcslHandle, context);
    }
 
    REPORT_INFO1(LC_PROTOCOL, "socket::close handle=%d\n", pcslHandle);

    if (INVALID_HANDLE != pcslHandle) {
        if (status == PCSL_NET_SUCCESS) {
            if (midpDecResourceCount(RSC_TYPE_TCP_CLI, 1) == 0) {
                REPORT_INFO(LC_PROTOCOL, "Resource limit update error"); 
            }
        } else if (status == PCSL_NET_WOULDBLOCK) {
            REPORT_INFO1(LC_PROTOCOL, "socket::close = 0x%x blocked\n", 
                         pcslHandle);
            /* IMPL NOTE: unclear whether this is the right signal */
            midp_thread_wait(NETWORK_READ_SIGNAL, (int)pcslHandle, context);
        } else {
            /* must be PCSL_NET_IOERROR */
            midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                    "IOError in socket::close = %d\n", 
                    (int)pcsl_network_error(pcslHandle));
            REPORT_INFO1(LC_PROTOCOL, "%s", gKNIBuffer);
            KNI_ThrowNew(midpIOException, gKNIBuffer);
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Releases any native resources used by the socket connection.
 * <p>
 * Java declaration:
 * <pre>
 *     finalize(V)V
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_socket_Protocol_finalize(void) {
    void *pcslHandle;
    int status = PCSL_NET_INVALID;
    void* context = NULL;
    int i;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    pcslHandle = (void *)(getMidpSocketProtocolPtr(thisObject)->handle);

    REPORT_INFO1(LC_PROTOCOL, "socket::finalize handle=%d\n", pcslHandle);

    if (INVALID_HANDLE != pcslHandle) {
        status = pcsl_socket_close_start(pcslHandle, &context);

        getMidpSocketProtocolPtr(thisObject)->handle = (jint)INVALID_HANDLE;
        if (midpDecResourceCount(RSC_TYPE_TCP_CLI, 1) == 0) {
            REPORT_INFO(LC_PROTOCOL, "Resource limit update error"); 
        }

        if (status == PCSL_NET_IOERROR) { 
            midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                    "IOError in socket::finalize error=%d\n", 
                    (int)pcsl_network_error(pcslHandle));
            REPORT_ERROR1(LC_PROTOCOL, "%s", gKNIBuffer);
        } else if (status == PCSL_NET_WOULDBLOCK) {
            /* blocking during finalize is not supported */
            REPORT_CRIT1(LC_PROTOCOL, "socket::finalize = 0x%x blocked\n", 
                         pcslHandle);
        }
    }

    i = getMidpSocketProtocolPtr(thisObject)->pendingConnections;
    while (i > 0) {
        ANC_DEC_NETWORK_INDICATOR;
        i--;
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Gets a raw IPv4 address for the given hostname.
 * <p>
 * Java declaration:
 * <pre>
 *     getIpNumber([B)I
 * </pre>
 *
 * @param sHost the hostname to lookup
 * @param ipBytes Output parameter that represents an unsigned char
 *                array for an IP address
 * @return len Length of ipBytes
 * 
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_socket_Protocol_getIpNumber0(void) {
    int len = -1;
    int status = PCSL_NET_INVALID;
    unsigned char ipBytes[MAX_ADDR_LENGTH];
    void* context = NULL;
    void* pcslHandle;
    MidpReentryData* info;

    KNI_StartHandles(2);
    KNI_DeclareHandle(ipAddressObject);
    
    KNI_GetParameterAsObject(2, ipAddressObject);

    info = (MidpReentryData*)SNI_GetReentryData(NULL);
    if (info == NULL) {  /* First invocation */
        GET_PARAMETER_AS_PCSL_STRING(1, host)
            const jbyte * const host_bytes = pcsl_string_get_utf8_data(&host);
            status = pcsl_network_gethostbyname_start(
                    (char*)host_bytes,
                    ipBytes, MAX_ADDR_LENGTH, &len, &pcslHandle, &context);

            pcsl_string_release_utf8_data(host_bytes, &host);
        RELEASE_PCSL_STRING_PARAMETER
    } else {  /* Reinvocation after unblocking the thread */
        pcslHandle = (void*)info->descriptor;
        /* IMPL NOTE: Please see 6440539 for details. */
        /* All but windows implementations of pcsl_network_gethostbyname_finish */
        /*  ignore context parameter. Windows one expects status code there. */
        context = (void*)info->status;
        status = pcsl_network_gethostbyname_finish(ipBytes, MAX_ADDR_LENGTH,
                                                  &len, pcslHandle, context);
    }

    if (status == PCSL_NET_SUCCESS) {
        KNI_SetRawArrayRegion(ipAddressObject, 0, len, (jbyte *)ipBytes);
    } else if (status == PCSL_NET_WOULDBLOCK) {
        midp_thread_wait(HOST_NAME_LOOKUP_SIGNAL, (int)pcslHandle, context);
    } else { /* must be PCSL_NET_IOERROR or PCSL_NET_INVALID */
        len = -1; 
    }

    KNI_EndHandles();
    KNI_ReturnInt((jint)len);
}

/**
 * Gets the requested IP number.
 * <p>
 * Java declaration:
 * <pre>
 *     getHost(Z)Ljava/lang/String;
 * </pre>
 *
 * @param local <tt>true</tt> to get the local host IP address, or
 *              <tt>false</tt> to get the remote host IP address.
 *
 * @return the IP address as a dotted-quad <tt>String</tt>
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT 
Java_com_sun_midp_io_j2me_socket_Protocol_getHost0(void) {
    int local;
    void *pcslHandle;
    char value[MAX_HOST_LENGTH];
    int status = PCSL_NET_INVALID;

    local = (int)KNI_GetParameterAsBoolean(1);

    memset(value, '\0', MAX_HOST_LENGTH);

    KNI_StartHandles(2);
    KNI_DeclareHandle(result);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    pcslHandle = (void *)(getMidpSocketProtocolPtr(thisObject)->handle);

    if (INVALID_HANDLE == pcslHandle) {
        KNI_ThrowNew(midpIOException, 
                     "invalid handle during socket::getHost");
    } else {
        if (local) {
            status = pcsl_socket_getlocaladdr(pcslHandle, value);
        } else {
            status = pcsl_socket_getremoteaddr(pcslHandle, value);
        }

        if (status == PCSL_NET_SUCCESS) {
            KNI_NewStringUTF(value, result);
        } else {
            KNI_ThrowNew(midpIOException, NULL);
        }
    }

    KNI_EndHandlesAndReturnObject(result);
}

/**
 * Gets the requested port number.
 * <p>
 * Java declaration:
 * <pre>
 *     getPort(Z)I
 * </pre>
 *
 * @param local <tt>true</tt> to get the local port number, or
 *              <tt>false</tt> to get the remote port number
 *
 * @return the port number
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_socket_Protocol_getPort0(void) {
    int local;
    void *pcslHandle;
    int port = 0;
    int status = PCSL_NET_INVALID;

    local = (int)KNI_GetParameterAsInt(1);

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    pcslHandle = (void *)(getMidpSocketProtocolPtr(thisObject)->handle);

    KNI_EndHandles();

    if (INVALID_HANDLE == pcslHandle) {
        KNI_ThrowNew(midpIOException, 
                     "invalid handle during socket::getPort");
    } else {
        if (local == 1) {
            status = pcsl_network_getlocalport(pcslHandle, &port);
        } else {
            status = pcsl_network_getremoteport(pcslHandle, &port);
        }

        if (status == PCSL_NET_IOERROR) {
            KNI_ThrowNew(midpIOException, NULL);
        }
    }

    KNI_ReturnInt((jint)port);
}

/**
 * Gets the requested socket option.
 * <p>
 * Java declaration:
 * <pre>
 *     getSockOpt(I)I
 * </pre>
 *
 * @param option socket option to retrieve
 * @return the value of the socket option
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_socket_Protocol_getSockOpt0(void) {
    int option;
    int value = 0;
    void *pcslHandle;
    int status = PCSL_NET_INVALID;

    option = (int)KNI_GetParameterAsInt(1);

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    pcslHandle = (void *)(getMidpSocketProtocolPtr(thisObject)->handle);

    KNI_EndHandles();

    if (INVALID_HANDLE == pcslHandle) {
        KNI_ThrowNew(midpIOException, 
                     "invalid handle during socket::getPort");
    } else {
        status = pcsl_network_getsockopt(pcslHandle, option, &value);
        if (PCSL_NET_IOERROR == status) {
            KNI_ThrowNew(midpIllegalArgumentException, "Unsupported Socket Option");
        }
    }

    KNI_ReturnInt((jint)value);
}


/**
 * Sets the requested socket option.
 * <p>
 * Java declaration:
 * <pre>
 *     setSockOpt(II)V
 * </pre>
 *
 * @param option socket option to set
 * @param value the value to set <tt>option</tt> to
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_socket_Protocol_setSockOpt0(void) {
    int option;
    int value;
    void *pcslHandle;
    int status = PCSL_NET_INVALID;
    
    value  = (int)KNI_GetParameterAsInt(2);
    option = (int)KNI_GetParameterAsInt(1);

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    pcslHandle = (void *)(getMidpSocketProtocolPtr(thisObject)->handle);

    KNI_EndHandles();

    if (INVALID_HANDLE == pcslHandle) {
        KNI_ThrowNew(midpIOException, 
                     "invalid handle during socket::getPort");
    } else {
        status = pcsl_network_setsockopt(pcslHandle, option, value);
        if (status == PCSL_NET_IOERROR) {
            KNI_ThrowNew(midpIllegalArgumentException, "Unsupported Socket Option");
        } else if (PCSL_NET_INVALID == status) {
            KNI_ThrowNew(midpIllegalArgumentException, "Illegal Socket Option Value");
        }
    }

    KNI_ReturnVoid();
}

/**
 * Shuts down the output side of the connection. Any error that might
 * result from this operation is ignored.
 * Reads from the open socket connection.
 * <p>
 * Java declaration:
 * <pre>
 *     shutdownOutput(V)V
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_socket_Protocol_shutdownOutput0(void) {
    void *pcslHandle;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    pcslHandle = (void *)(getMidpSocketProtocolPtr(thisObject)->handle);

    KNI_EndHandles();

    REPORT_INFO1(LC_PROTOCOL, "socket::shutdownOutput handle=%d\n", pcslHandle);

    if (INVALID_HANDLE != pcslHandle) {
        /* Return value of shutdown() need not be checked */ 
        pcsl_socket_shutdown_output(pcslHandle);
    }

    KNI_ReturnVoid();
}

/**
 * Initializes the network.
 * <p>
 * Java declaration:
 * <pre>
 *     initializeInternal(V)V
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_NetworkConnectionBase_initializeInternal(void) {
    MidpReentryData* info;
    int status = PCSL_NET_SUCCESS;

    REPORT_INFO(LC_CORE,
        "Java_com_sun_midp_io_NetworkConnectionBase_initializeInternal start\n");

    info = (MidpReentryData*)SNI_GetReentryData(NULL);
    if (info == NULL) {
       status = pcsl_network_init_start(midp_network_status_event);
    } else { /* Reinvocation after unblocking the thread */
       status = pcsl_network_init_finish();
    }

   REPORT_INFO2(LC_CORE,
       "Java_com_sun_midp_io_NetworkConnectionBase_initializeInternal() "
       "pcsl_network_init(%d) returned %d\n",
       (int)(info != NULL), (int)status);

    switch(status){
    case PCSL_NET_SUCCESS:
        break;
    case PCSL_NET_WOULDBLOCK:
        midp_thread_wait(NETWORK_STATUS_SIGNAL, 0, NULL);
        break;
    default:
        REPORT_ERROR2(LC_CORE,
            "Java_com_sun_midp_io_NetworkConnectionBase_initializeInternal() "
            "pcsl_network_init(%d) failed with code %d\n",
            (int)(info != NULL), (int)status);
        break;
    }

    REPORT_INFO(LC_CORE,
        "Java_com_sun_midp_io_NetworkConnectionBase_initializeInternal end\n");

    KNI_ReturnVoid();
}

/**
 * Notify blocked threads thar all input streams of the socket
 * connection are closed by some other thread.
 * <p>
 * Java declaration:
 * <pre>
 *     notifyClosedInput0(V)V
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_socket_Protocol_notifyClosedInput0(void) {
    void *pcslHandle;
    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    pcslHandle = (void *)(getMidpSocketProtocolPtr(thisObject)->handle);
    /* Invalid handle means that entire socket connecton is closed
     * and all the blocked threads have alredy been notified.
     */
    if (INVALID_HANDLE != pcslHandle) {
        midp_thread_signal(NETWORK_READ_SIGNAL, (int)pcslHandle, 0);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Notify blocked threads thar all output streams of the socket
 * connection are closed by some other thread.
 * <p>
 * Java declaration:
 * <pre>
 *     notifyClosedOutput0(V)V
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_socket_Protocol_notifyClosedOutput0(void) {
    void *pcslHandle;
    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    pcslHandle = (void *)(getMidpSocketProtocolPtr(thisObject)->handle);
    /* Invalid handle means that entire socket connecton is closed
     * and all the blocked threads have alredy been notified.
     */
    if (INVALID_HANDLE != pcslHandle) {
        midp_thread_signal(NETWORK_WRITE_SIGNAL, (int)pcslHandle, 0);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}
