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
#include <push_server_resource_mgmt.h>
#include <midp_logging.h>
#include <midpResourceLimit.h>
#include <midpMalloc.h>
#include <string.h>
#include <pcsl_network.h>
#include <midp_thread.h>
#include <midp_libc_ext.h>
#include <kni_globals.h>
#include <pcsl_memory.h>
#include <suitestore_common.h>
#include <midpUtilKni.h>

/**
 * @file
 *
 * The default implementation of the native functions that are needed
 * for supporting the "datagram:" Generic Connection protocol.
 */

/* Macro to retrieve C structure representation of an Object */
typedef struct Java_com_sun_midp_io_j2me_datagram_Protocol _datagramProtocol;
#define getMidpDatagramProtocolPtr(handle) (unhand(_datagramProtocol,(handle)))

/**
 * Opens a datagram connection on the given port.
 * <p>
 * Java declaration:
 * <pre>
 *     open0(I[B)V
 * </pre>
 *
 * @param port port to listen on, or 0 to have one selected
 * @param suiteId the ID of the current MIDlet suite
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_datagram_Protocol_open0(void) {
    int port;
    SuiteIdType suiteId;
    jboolean tryOpen = KNI_TRUE;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    port = (int)KNI_GetParameterAsInt(1);
    suiteId = KNI_GetParameterAsInt(2);

    if (getMidpDatagramProtocolPtr(thisObject)->nativeHandle
            != (jint)INVALID_HANDLE) {
        KNI_ThrowNew(midpIOException, "already open");
        tryOpen = KNI_FALSE;
    }

    if (tryOpen) {
        int pushReturn;
        pushReturn = pushcheckout("datagram", port,
                                  (char*)midp_suiteid2chars(suiteId));
        /*
         * pushcheckout() returns -1 if the handle wasn't found, -2 if it's
         * already in use by another suite, otherwise a valid checked-out
         * handle.
         */

        if (pushReturn == -1) {
            /* leave tryOpen == KNI_TRUE and try again below */
        } else if (pushReturn == -2) {
            KNI_ThrowNew(midpIOException, "already in use");
            tryOpen = KNI_FALSE;
        } else {
            /* IMPL NOTE: need to do resource accounting for this case */
            getMidpDatagramProtocolPtr(thisObject)->nativeHandle
                = (jint)pushReturn;
            tryOpen = KNI_FALSE;
        }
    }

    if (tryOpen) {
        if (midpCheckResourceLimit(RSC_TYPE_UDP, 1) == 0) {
            KNI_ThrowNew(midpIOException, "resource limit exceeded");
        } else {
            MidpReentryData* info;
            int status;
            void *socketHandle;
            void *context;

            info = (MidpReentryData*)SNI_GetReentryData(NULL);
            if (info == NULL) {
                /* initial invocation */
                ANC_INC_NETWORK_INDICATOR;
                status = pcsl_datagram_open_start(port, &socketHandle,
                    &context);
            } else {
                /* reinvocation */
                socketHandle = (void *)info->descriptor;
                context = info->pResult;
                status = pcsl_datagram_open_finish(socketHandle, context);
            }

            if (status == PCSL_NET_SUCCESS) {
                if (midpIncResourceCount(RSC_TYPE_UDP, 1) == 0) {
                    REPORT_INFO(LC_PROTOCOL,
                        "Datagrams: resource limit update error");
                }
                getMidpDatagramProtocolPtr(thisObject)->nativeHandle
                    = (jint)socketHandle;
                ANC_DEC_NETWORK_INDICATOR;
            } else if (status == PCSL_NET_WOULDBLOCK) {
                midp_thread_wait(NETWORK_WRITE_SIGNAL, (int)socketHandle,
                    context);
            } else {
                /* status == PCSL_NET_IOERROR */
                midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                    "error code %d", pcsl_network_error(socketHandle));
                REPORT_INFO1(LC_PROTOCOL, "datagram::open0 %s", gKNIBuffer);
                KNI_ThrowNew(midpIOException, gKNIBuffer);
                ANC_DEC_NETWORK_INDICATOR;
            }
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Sends a datagram.
 * <p>
 * Java declaration:
 * <pre>
 *     send0(II[BII)I
 * </pre>
 *
 * @param ipNumber raw IPv4 address of the remote host
 * @param port UDP port of the remote host
 * @param buf the data buffer to send
 * @param off the offset into the data buffer
 * @param len the length of the data in the buffer
 * @return number of bytes sent
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_datagram_Protocol_send0(void) {
    int ipAddress;
    int port;
    int offset;
    int length;
    int bytesSent = 0;
    void *socketHandle;
    MidpReentryData* info;
    unsigned char ipBytes[MAX_ADDR_LENGTH];

    KNI_StartHandles(2);

    KNI_DeclareHandle(bufferObject);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    ipAddress = (int)KNI_GetParameterAsInt(1);
    port      = (int)KNI_GetParameterAsInt(2);
    KNI_GetParameterAsObject(3, bufferObject);
    offset    = (int)KNI_GetParameterAsInt(4);
    length    = (int)KNI_GetParameterAsInt(5);

    socketHandle =
        (void *)getMidpDatagramProtocolPtr(thisObject)->nativeHandle;

    REPORT_INFO5(LC_PROTOCOL,
        "datagram::send0 off=%d len=%d port=%d ip=0x%x handle=0x%x",
        offset, length, port, ipAddress, (int)socketHandle);

    /* Convert ipAddress(integer) to ipBytes */
    memcpy(ipBytes, &ipAddress, sizeof(ipBytes));

    info = (MidpReentryData*)SNI_GetReentryData(NULL);

    if (socketHandle != INVALID_HANDLE) {
        int status;
        void *context;

        if (info == NULL) {
            /* initial invocation */
            ANC_INC_NETWORK_INDICATOR;
            SNI_BEGIN_RAW_POINTERS;
            status = pcsl_datagram_write_start(
                socketHandle, ipBytes, port,
                (char*)&(JavaByteArray(bufferObject)[offset]),
                length, &bytesSent, &context);
            SNI_END_RAW_POINTERS;
        } else {
            /* reinvocation */
            if ((void *)info->descriptor != socketHandle) {
                REPORT_CRIT2(LC_PROTOCOL,
                    "datagram::send0 handle mismatch 0x%x != 0x%x\n",
                        socketHandle, info->descriptor);
            }
	    context = info->pResult;
            SNI_BEGIN_RAW_POINTERS;
	    status = pcsl_datagram_write_finish(
                socketHandle, ipBytes, port,
                (char*)&(JavaByteArray(bufferObject)[offset]),
                length, &bytesSent, context);
            SNI_END_RAW_POINTERS;
        }

        if (status == PCSL_NET_SUCCESS) {
            ANC_DEC_NETWORK_INDICATOR;
        } else if (status == PCSL_NET_WOULDBLOCK) {
            midp_thread_wait(NETWORK_WRITE_SIGNAL, (int)socketHandle, context);
        } else if (status == PCSL_NET_INTERRUPTED) {
            KNI_ThrowNew(midpInterruptedIOException, NULL);
            ANC_DEC_NETWORK_INDICATOR;
        } else {
            /* status == PCSL_NET_IOERROR */
            midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                "error code %d", pcsl_network_error(socketHandle));
            KNI_ThrowNew(midpIOException, gKNIBuffer);
            ANC_DEC_NETWORK_INDICATOR;
        }

    } else {
        if (info == NULL) {
            /* initial invocation */
            KNI_ThrowNew(midpIOException, "socket closed");
        } else {
            /* reinvocation */
            KNI_ThrowNew(midpInterruptedIOException, NULL);
            ANC_DEC_NETWORK_INDICATOR;
        }
    }

    KNI_EndHandles();
    KNI_ReturnInt((jint)bytesSent);
}

/**
 * Packs an IP address, port number, and the number of bytes received into a
 * jlong value suitable for returning from the receive0 native method.
 */
static jlong
pack_recv_retval(int ipAddress, int port, int bytesReceived) {
    return
        (((jlong)ipAddress) << 32) +
        (unsigned)((port & 0xFFFF) << 16) +
        (bytesReceived & 0xFFFF);
}

/**
 * Receives a datagram.
 * <p>
 * Java declaration:
 * <pre>
 *     receive0([BII)J
 * </pre>
 *
 * @param buf the data buffer
 * @param off the offset into the data buffer
 * @param len the length of the data in the buffer
 * @return The upper 32 bits contain the raw IPv4 address of the
 *         host the datagram was received from. The next 16 bits
 *         contain the port. The last 16 bits contain the number
 *         of bytes received.
 */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_com_sun_midp_io_j2me_datagram_Protocol_receive0(void) {
    int offset;
    int length;
    void *socketHandle;
    jlong lres = 0;
    MidpReentryData* info;

    KNI_StartHandles(2);
    KNI_DeclareHandle(bufferObject);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    KNI_GetParameterAsObject(1, bufferObject);
    offset = (int)KNI_GetParameterAsInt(2);
    length = (int)KNI_GetParameterAsInt(3);

    socketHandle =
        (void *)getMidpDatagramProtocolPtr(thisObject)->nativeHandle;

    info = (MidpReentryData*)SNI_GetReentryData(NULL);

    REPORT_INFO3(LC_PROTOCOL,
        "datagram::receive0 off=%d len=%d handle=0x%x",
	offset, length, (int)socketHandle);

    if (socketHandle != INVALID_HANDLE) {
        int ipAddress;
        int port;
        int bytesReceived;

        /*
         * Check the push cache for a waiting datagram.
         *
         * If pushgetcachedpacket() returns -1 [IMPL NOTE: the code checks for
         * less than zero; which is correct?], we need to read a datagram
         * ourselves. Otherwise, pushgetcachedpacket() has returned a waiting
         * datagram and has set ipAddress and port to valid values.
         */
        SNI_BEGIN_RAW_POINTERS;
        bytesReceived = pushgetcachedpacket((int)socketHandle, &ipAddress,
            &port, (char*)&(JavaByteArray(bufferObject)[offset]), length);
        SNI_END_RAW_POINTERS;

        if (bytesReceived < 0) {
            int status;
            unsigned char ipBytes[MAX_ADDR_LENGTH];
            void *context;

            if (info == NULL) {
                /* initial invocation */
                ANC_INC_NETWORK_INDICATOR;
                SNI_BEGIN_RAW_POINTERS;
                status = pcsl_datagram_read_start(
                           socketHandle, ipBytes, &port,
                           (char*)&(JavaByteArray(bufferObject)[offset]),
                           length, &bytesReceived, &context);
                SNI_END_RAW_POINTERS;
            } else {
                /* reinvocation */
                if ((void *)info->descriptor != socketHandle) {
                    REPORT_CRIT2(LC_PROTOCOL,
                        "datagram::send0 handle mismatch 0x%x != 0x%x\n",
                            socketHandle, info->descriptor);
                }
                context = info->pResult;
                SNI_BEGIN_RAW_POINTERS;
                status = pcsl_datagram_read_finish(
                           socketHandle, ipBytes, &port,
                           (char*)&(JavaByteArray(bufferObject)[offset]),
                           length, &bytesReceived, context);
                SNI_END_RAW_POINTERS;
            }

            if (status == PCSL_NET_SUCCESS) {
                memcpy(&ipAddress, ipBytes, MAX_ADDR_LENGTH);
                lres = pack_recv_retval(ipAddress, port, bytesReceived);
                ANC_DEC_NETWORK_INDICATOR;
            } else if (status == PCSL_NET_WOULDBLOCK) {
                midp_thread_wait(NETWORK_READ_SIGNAL, (int)socketHandle, context);
            } else if (status == PCSL_NET_INTERRUPTED) {
                KNI_ThrowNew(midpInterruptedIOException, NULL);
                ANC_DEC_NETWORK_INDICATOR;
            } else {
                /* status == PCSL_NET_IOERROR */
                midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                    "error code %d", pcsl_network_error(socketHandle));
                KNI_ThrowNew(midpIOException, gKNIBuffer);
                ANC_DEC_NETWORK_INDICATOR;
            }
        } else {
            /* push gave us a datagram */
            lres = pack_recv_retval(ipAddress, port, bytesReceived);
        }
    } else {
        if (info == NULL) {
            /* initial invocation */
            KNI_ThrowNew(midpIOException, "socket closed");
        } else {
            /* reinvocation */
            KNI_ThrowNew(midpInterruptedIOException, NULL);
            ANC_DEC_NETWORK_INDICATOR;
        }
    }

    KNI_EndHandles();
    KNI_ReturnLong(lres);
}


/**
 * Closes the datagram connection.
 * <p>
 * Java declaration:
 * <pre>
 *     close0(V)V
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_datagram_Protocol_close0(void) {
    void *socketHandle;
    MidpReentryData *info;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    socketHandle =
        (void *)getMidpDatagramProtocolPtr(thisObject)->nativeHandle;

    info = (MidpReentryData*)SNI_GetReentryData(NULL);

    REPORT_INFO1(LC_PROTOCOL, "datagram::close handle=0x%x",
        (int)socketHandle);

    if (socketHandle != INVALID_HANDLE) {
        int status;

        status = pushcheckin((int)socketHandle);
        if (status == -1) {
            void *context = NULL;

            if (info == NULL) {
                /* first invocation */
                ANC_INC_NETWORK_INDICATOR;
                status = pcsl_datagram_close_start(socketHandle, &context);

                getMidpDatagramProtocolPtr(thisObject)->nativeHandle =
                    (jint)INVALID_HANDLE;

                midp_thread_signal(NETWORK_READ_SIGNAL, (int)socketHandle, 0);
                midp_thread_signal(NETWORK_WRITE_SIGNAL, (int)socketHandle, 0);
            } else {
                /* reinvocation */
                socketHandle = (void *)(info->descriptor);
                context = info->pResult;
                status = pcsl_datagram_close_finish(socketHandle, context);
            }

            if (status == PCSL_NET_WOULDBLOCK) {
                /* IMPL NOTE: unclear whether this is the right signal */
                midp_thread_wait(NETWORK_READ_SIGNAL, (int)socketHandle,
                    context);
            } else {
                /* PCSL_NET_SUCCESS or PCSL_NET_IOERROR */
                ANC_DEC_NETWORK_INDICATOR;
                if (midpDecResourceCount(RSC_TYPE_UDP, 1) == 0) {
                    REPORT_INFO(LC_PROTOCOL,
                        "datagram::close0 resource limit update error");
                }
                if (status != PCSL_NET_SUCCESS) {
                    midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                        "error code %d", pcsl_network_error(socketHandle));
                    REPORT_INFO1(LC_PROTOCOL, "datagram::close %s",
                        gKNIBuffer);
                    KNI_ThrowNew(midpIOException, gKNIBuffer);
                }
            }
        } else {
            /* it was checked into push; don't really close the socket
               but notify a possible listeners to be unblocked */
            getMidpDatagramProtocolPtr(thisObject)->nativeHandle =
                                                   (jint)INVALID_HANDLE;
            midp_thread_signal(NETWORK_READ_SIGNAL, (int)socketHandle, 0);
            midp_thread_signal(NETWORK_WRITE_SIGNAL, (int)socketHandle, 0);
        }
    } else {
        if (info == NULL) {
            /* first invocation */
            /* already closed, do nothing */
        } else {
            /* reinvocation */
            ANC_DEC_NETWORK_INDICATOR;
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Get a hostname for the given raw IPv4 address.
 * <p>
 * Java declaration:
 * <pre>
 *     static addrToString(I)Ljava/lang/String;
 * </pre>
 *
 * @param ipn raw IPv4 address
 * @return The hostname or <tt>ipn</tt> as a dotted-quad if
 *         no hostname was found
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_io_j2me_datagram_Protocol_addrToString(void) {
    jint ipn;
    jchar* result;
    int resultLen;
    int status;

    ipn = KNI_GetParameterAsInt(1);

    status = pcsl_network_addrToString((unsigned char*)&ipn, &result,
                                       &resultLen);
    KNI_StartHandles(1);
    KNI_DeclareHandle(resultObj);

    if (PCSL_NET_SUCCESS == status) {
        KNI_NewString(result, (jsize)resultLen, resultObj);
        pcsl_mem_free(result);
    } else {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_EndHandlesAndReturnObject(resultObj);
}

/**
 * Gets a raw IPv4 address for the given hostname.
 * <p>
 * Java declaration:
 * <pre>
 *     static getIpNumber([B)I
 * </pre>
 *
 * @param szHost the hostname to lookup as a 'C' string
 * @return raw IPv4 address or <tt>-1</tt> if there was an error
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_datagram_Protocol_getIpNumber(void) {
    int len;
    int status = PCSL_NET_NOSTATUS;
    int ipn = -1;
    unsigned char ipBytes[MAX_ADDR_LENGTH];
    void* context = NULL;
    void* handle;
    MidpReentryData* info;

    info = (MidpReentryData*)SNI_GetReentryData(NULL);

    if (info == NULL) {  /* First invocation */
        KNI_StartHandles(1);
        GET_PARAMETER_AS_PCSL_STRING(1, host)
            const jbyte * const host_bytes = pcsl_string_get_utf8_data(&host);

            status = pcsl_network_gethostbyname_start(
                   (char*)host_bytes,
                    ipBytes, MAX_ADDR_LENGTH, &len, &handle, &context);

            pcsl_string_release_utf8_data(host_bytes, &host);
        RELEASE_PCSL_STRING_PARAMETER
        KNI_EndHandles();
    } else {  /* Reinvocation after unblocking the thread */
        handle = (void*)info->descriptor;
        /* IMPL NOTE: Please see 6440539 for details. */
        /* All but windows implementations of pcsl_network_gethostbyname_finish */
        /*  ignore context parameter. Windows one expects status code there. */
        context = (void*)info->status;
        status = pcsl_network_gethostbyname_finish(ipBytes, MAX_ADDR_LENGTH,
                                                  &len, handle, context);
    }

    if (status == PCSL_NET_SUCCESS) {
        /*
         * Convert the unsigned char ip bytes array into an integer
         * that represents a raw IP address.
         */
        //ipn = pcsl_network_getRawIpNumber(ipBytes);
        memcpy(&ipn, ipBytes, MAX_ADDR_LENGTH);
    } else if (status == PCSL_NET_WOULDBLOCK) {
        midp_thread_wait(HOST_NAME_LOOKUP_SIGNAL, (int)handle, context);
    } else {
        /* status is either PCSL_NET_IOERROR or PCSL_NET_INVALID */
        /* or (in the case of out-of-memory) PCSL_NET_NOSTATUS */
        ipn = -1;
        REPORT_INFO1(LC_PROTOCOL,
            "datagram::getIpNumber returns PCSL error code %d", status);
        /*
         * IOException is thrown at the Java layer when return value
         * is -1
         */
        //KNI_ThrowNew(midpIOException, "Host name could not be resolved");
    }

    KNI_ReturnInt((jint)ipn);
}

/**
 * Get the maximum length of a datagram.
 * <p>
 * Java declaration:
 * <pre>
 *     getMaximumLength0(V)I
 * </pre>
 *
 * @return maximum length of a datagram
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_datagram_Protocol_getMaximumLength0(void) {
    void *socketHandle;
    int len = -1;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    socketHandle =
        (void *)getMidpDatagramProtocolPtr(thisObject)->nativeHandle;

    if (socketHandle != INVALID_HANDLE) {
        int status;

        /*
         * IMPL NOTE:
         * Option=3 represents SO_RCVBUF
         * The SO_RCVBUF option is used by the the network implementation
         * as a hint to size the underlying network I/O buffers.
         */
        status = pcsl_network_getsockopt(socketHandle, 3, &len);
        if (status != PCSL_NET_SUCCESS) {
            KNI_ThrowNew(midpIOException, NULL);
        }
    } else {
        KNI_ThrowNew(midpIOException, "socket closed");
    }

    REPORT_INFO2(LC_PROTOCOL,
        "datagram::getMaximumLength0 handle=%d len=%d %d\n",
        (int)socketHandle, len);

    KNI_EndHandles();
    KNI_ReturnInt((jint)len);
}

/**
 * Gets the nominal length of a datagram.
 * <p>
 * Java declaration:
 * <pre>
 *     getNominalLength0(V)I
 * </pre>
 *
 * @return nominal length of a datagram
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_datagram_Protocol_getNominalLength0(void) {
    /*
     * IMPL NOTE:
     * This implementation is identical to getMaximumLength0().
     * Is this useful? Should this just call getMaxiumumLength0()?
     */

    void *socketHandle;
    int len = -1;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    socketHandle =
        (void *)getMidpDatagramProtocolPtr(thisObject)->nativeHandle;

    if (socketHandle != INVALID_HANDLE) {
        int status;

        /*
         * IMPL NOTE:
         * Option=3 represents SO_RCVBUF
         * The SO_RCVBUF option is used by the the network implementation
         * as a hint to size the underlying network I/O buffers.
         */
        status = pcsl_network_getsockopt(socketHandle, 3, &len);
        if (status != PCSL_NET_SUCCESS) {
            KNI_ThrowNew(midpIOException, NULL);
        }
    } else {
        KNI_ThrowNew(midpIOException, "socket closed");
    }

    REPORT_INFO2(LC_PROTOCOL,
        "datagram::getNominalLength0 handle=%d len=%d\n",
        (int)socketHandle, len);

    KNI_EndHandles();
    KNI_ReturnInt((jint)len);
}

/**
 * Releases any native resources used by the datagram connection.
 * <p>
 * Java declaration:
 * <pre>
 *     finalize(V)V
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_datagram_Protocol_finalize(void) {
    void *handle;
    int status;
    void *context = NULL;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    handle = (void *)getMidpDatagramProtocolPtr(thisObject)->nativeHandle;

    if (handle != INVALID_HANDLE) {
        if (pushcheckin((int)handle) == -1) {
            status = pcsl_datagram_close_start(handle, &context);
            if (status == PCSL_NET_SUCCESS) {
                if (midpDecResourceCount(RSC_TYPE_UDP, 1) == 0) {
                    REPORT_INFO(LC_PROTOCOL,
                        "Datagrams: resource limit update error");
                }
            } else if (status == PCSL_NET_IOERROR) {
                REPORT_INFO2(LC_PROTOCOL,
                    "datagram::finalize handle 0x%x I/O error code %d",
                    (int)handle, pcsl_network_error(handle));
            } else if (status == PCSL_NET_WOULDBLOCK) {
                REPORT_ERROR1(LC_PROTOCOL,
                    "datagram::finalize handle 0x%x WOULDBLOCK not supported",
                    (int)handle);
            }
            getMidpDatagramProtocolPtr(thisObject)->nativeHandle =
                (jint)INVALID_HANDLE;
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Gets the local IP number.
 * <p>
 * Java declaration:
 * <pre>
 *     static getHost0(V)Ljava/lang/String;
 * </pre>
 *
 * @return the local IP address as a dotted-quad <tt>String</tt>
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_io_j2me_datagram_Protocol_getHost0(void) {
    char value[MAX_HOST_LENGTH];
    int status;

    KNI_StartHandles(1);
    KNI_DeclareHandle(result);

    status = pcsl_network_getLocalIPAddressAsString(value);

    if ((status == PCSL_NET_SUCCESS) && (value != NULL)) {
        KNI_NewStringUTF(value, result);
    } else {
        KNI_ReleaseHandle(result);
    }

    KNI_EndHandlesAndReturnObject(result);
}

/**
 * Gets the local port number of a datagram connection.
 * <p>
 * Java declaration:
 * <pre>
 *     getPort0(V)I
 * </pre>
 *
 * @return the local port number of the given datagram connection
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_io_j2me_datagram_Protocol_getPort0(void) {
    void *socketHandle;
    int port = -1;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    socketHandle =
        (void *)getMidpDatagramProtocolPtr(thisObject)->nativeHandle;

    if (socketHandle != INVALID_HANDLE) {
        int status;
        status = pcsl_network_getlocalport(socketHandle, &port);
        if (status != PCSL_NET_SUCCESS) {
            KNI_ThrowNew(midpIOException, NULL);
        }
    } else {
        KNI_ThrowNew(midpIOException, "socket closed");
    }

    KNI_EndHandles();
    KNI_ReturnInt((jint)port);
}
