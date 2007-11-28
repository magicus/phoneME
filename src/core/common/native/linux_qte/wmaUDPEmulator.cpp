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

#include <fcntl.h>
#include <errno.h>
#include <values.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <wmaInterface.h>
#include <wmaSocket.h>
#include <midp_properties_port.h>
#include <wmaUDPEmulator.h>
#include <bytePackUnpack.h>
#include <pcsl_memory.h>
#include <pcsl_string.h>
#if ENABLE_JSR_205
#include <jsr205_mms_structs.h>
#endif
#if ENABLE_WMA_LOOPBACK
#include <jsr120_sms_structs.h>
#include <jsr120_sms_listeners.h>
#include <jsr120_sms_pool.h>
#if ENABLE_JSR_205
#include <jsr205_mms_pool.h>
#include <jsr120_mms_listeners.h>
#include <jsr205_mms_protocol.h>
#endif
#include <push_server_resource_mgmt.h>
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

#ifndef MAX_DATAGRAM_LENGTH
#define MAX_DATAGRAM_LENGTH 1500
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SMS protocol inbound/outbound physical ports and handles.
 * These are default values. Overriden with
 * com.sun.midp.io.j2me.sms.DatagramPortIn and
 * com.sun.midp.io.j2me.sms.DatagramPortOut properties
 * (see jsr120_wma11-gate/src/config/common/properties_jsr120.xml).
 * smsInPortNumber also can be overriden with JSR_120_SMS_PORT
 * environment variable which takes precedence over value of the
 * property.
 */
static jint smsInPortNumber = 11100;
static jint smsOutPortNumber = 11101;
static void *smsHandle = NULL;

/*
 * CBS protocol inbound physical ports and handles.
 * These are default values. Overriden with
 * com.sun.midp.io.j2me.cbs.DatagramPortIn property
 * (see jsr120_wma11-gate/src/config/common/properties_jsr120.xml).
 * Also can be overriden with JSR_120_CBS_PORT
 * environment variable which takes precedence over value of the
 * property.
 * No outbound CBS port required.
 */
static jint cbsInPortNumber = 22200;
static void *cbsHandle = NULL;

#if ENABLE_JSR_205
/*
 * MMS protocol inbound/outbound physical ports and handles.
 * These are default values. Overriden with
 * com.sun.midp.io.j2me.mms.DatagramPortIn and
 * com.sun.midp.io.j2me.mms.DatagramPortOut properties
 * (see jsr205_wma20-gate/src/config/common/properties_jsr205.xml).
 * mmsInPortNumber also can be overriden with JSR_205_MMS_PORT
 * environment variable which takes precedence over value of the
 * property.
 */
static jint mmsInPortNumber = 33300;
static jint mmsOutPortNumber = 33301;
static void *mmsHandle = NULL;
#endif

/*
 * Target host name to send outbound messages to (typically the host the
 * test suite is running on). Can be overriden with
 * com.sun.midp.io.j2me.wma.DatagramHost property
 * (see jsr120_wma11-gate/src/config/common/properties_jsr120.xml).
 */
static char* targetHost = NULL;
static char* targetHostDefault = "wmadatagramhost";

/*
 * Helper functions.
 */

#if ENABLE_JSR_205
/**
 * Create an MMS message from the given parameters.
 *
 * @param fromAddress The sender's MMS address.
 * @param appID The application ID corresponding to the message.
 * @param length The total number of bytes in the message.
 * @param buffer A pointer to the message bytes.
 *
 * @return The constructed MMS message.
 */
MmsMessage* createMmsMessage(char* fromAddress, char* appID, char* replyToAppID,
    jint length, char* buffer) {

    MmsMessage* mms = NULL;

    if (buffer != NULL) {

        mms = (MmsMessage*)pcsl_mem_malloc(sizeof(MmsMessage));
        memset(mms, 0, sizeof(MmsMessage));

        /* Copy the "from" address. */
        if (fromAddress == NULL) {
            mms->fromAddress = NULL;
        } else {
            mms->fromAddress = (char*)pcsl_mem_malloc(strlen(fromAddress)+1);
            strcpy(mms->fromAddress, fromAddress);
        }

        /* Copy the application ID string. */
        mms->appID = (char*)pcsl_mem_malloc(strlen(appID)+1);
        strcpy(mms->appID, appID);

        /* Copy the reply-to application ID string. */
        if (replyToAppID == NULL) {
            mms->replyToAppID = NULL;
        } else {
            mms->replyToAppID = (char*)pcsl_mem_malloc(strlen(replyToAppID)+1);
            strcpy(mms->replyToAppID, replyToAppID);
        }

        /* Copy the message length and the message bytes. */
        mms->msgLen = length;

        mms->msgBuffer = (char*)pcsl_mem_malloc(length);
        memcpy(mms->msgBuffer, buffer, length);
    }

    return mms;
}
#endif

/**
 * Fetch the network information for the host, based on its name.
 *
 * @param hostname the host name for which an ip address is needed,
 *	  a null-terminated string
 * @param pAddress base of byte array to receive the address
 * @param maxLen size of buffer at pAddress
 * @param pLen number of bytes returned to pAddress, 4 if it's an
 *	  IPv4 address, 16 if it is an IPv6 address
 *
 * @return WMA_NET_SUCCESS upon success;\n
 * WMA_NET_IOERROR if there is a network error;\n
 * WMA_NET_INVALID if <code>maxLen</code> is too small to receive the address.
 * 
 */
#if ENABLE_WMA_LOOPBACK == 0
static WMA_STATUS network_gethostbyname( char *hostname,
                                            unsigned char *pAddress,
                                            jint maxLen,
                                            jint *pLen)
{
    struct hostent *hp;
    jint realLen;

    hp = gethostbyname(hostname);

    if (NULL == hp) {
        return WMA_NET_IOERROR;
    }

    if (hp->h_addrtype == AF_INET) {
        realLen = 4;        /* IPv4 */
    } else if (hp->h_addrtype == AF_INET6) {
        realLen = 16;        /* IMPL NOTE: IPv6 not supported, yet */
    } else {
        return WMA_NET_IOERROR;
    }

    if (realLen > maxLen) {
        return WMA_NET_INVALID;
    }

    memcpy(pAddress, hp->h_addr_list[0], realLen);
    *pLen = realLen;

    return WMA_NET_SUCCESS;
}
#endif

/**
 * Open a datagram port for the given protocol in inbound or outbound mode.
 *
 * @param protocol  A protocol type.
 * @param port  The port to be bound to the datagram traffic.
 *
 * @return <code>WMA_NET_SUCCESS<code> if the port could be opened
 *     successfully; <code>WMA_NET_IOERROR</code>, otherwise.
 */
WMA_STATUS jsr120_datagram_open(WMA_PROTOCOLS protocol, jint port) {

    struct sockaddr_in addr;
    jint fd;
    jint res;
    jint i;
    jint flags;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == INVALID_SOCKET) {
        return WMA_NET_IOERROR;
    }

    i = port;

   /*
    * RFC: Here is a total mystery to me. If I give solaris a port number of zero
    * then it allocates one somewhere above 50000. The problem is that when I
    * do a recvfrom() on a socket that is bound to a port > 32767 it never sees
    * any data.
    *
    * Temporary solution (seems to work all the time):
    * Start at port 6000 and just go upwards until a free port is found
    */
    if (i <= 0) {
        i = 6000;
    }

    res = EADDRINUSE;
    do {
        addr.sin_family                = AF_INET;
        addr.sin_port                = htons((short)i++);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        res = bind(fd, (struct sockaddr*)&addr, sizeof( addr ));

    } while ((res == SOCKET_ERROR) && (errno == EADDRINUSE) &&
             (port == 0) && (i < 32767));

    if (res != SOCKET_ERROR) {


        /* Create WMASocket for receiving datagrams. */
        if (protocol == WMA_SMS_PROTOCOL) {
            smsHandle = (void *)wmaCreateSocketHandle(protocol, fd);
        } else if (protocol == WMA_CBS_PROTOCOL) {
            cbsHandle = (void *)wmaCreateSocketHandle(protocol, fd);
        } 
#if ENABLE_JSR_205
        if (protocol == WMA_MMS_PROTOCOL) {
            mmsHandle = (void *)wmaCreateSocketHandle(protocol, fd);
        }
#endif

        /* Set the datagram socket to non-blocking mode */
        flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        return WMA_NET_SUCCESS;
    }

    close(fd);

    return WMA_NET_IOERROR;
}

/**
 * Shut down a protocol's inbound/outbound datagram port.
 *
 * @param protocol  A protocol type.
 *
 * @return  <code>WMA_NET_SUCCESS</code> if the datagram port could be closed
 *          successfully; <code>WMA_NET_IOERROR</code> if the protocol was
 *          unknown, or if there was a problem when closing the port.
 */
WMA_STATUS jsr120_datagram_close(WMA_PROTOCOLS protocol) {
    jint status = 0;

    /* The socket descriptor. */
    jint fd = -1;

    /* Shut down the inbound datagram port. */
    if (protocol == WMA_SMS_PROTOCOL) {
        fd = wmaGetRawSocketFD(smsHandle);
        wmaDestroySocketHandle(smsHandle);
    } else if (protocol == WMA_CBS_PROTOCOL) {
        fd = wmaGetRawSocketFD(cbsHandle);
        wmaDestroySocketHandle(cbsHandle);
    } 
#if ENABLE_JSR_205
      else if (protocol == WMA_MMS_PROTOCOL) {
        fd = wmaGetRawSocketFD(mmsHandle);
        wmaDestroySocketHandle(mmsHandle);
    }
#endif

    if (fd >= 0) {
        status = close(fd);

        if (status == 0) {

            return WMA_NET_SUCCESS;
        }
    }

    return WMA_NET_IOERROR;
}

/**
 * Common implementation between pcsl_datagram_read_start()
 * and pcsl_datagram_read_finish().
 *
 * @param protocol     A protocol type (e.g.: <code>WMA_SMS_PROTOCOL</code>).
 * @param pAddress     The network address from the datagram. 
 * @param port         The network port from the datagram.
 * @param buffer       The buffer for the bytes to be read.
 * @param length       The number of bytes to be read.
 * @param pBytesRead   The number of bytes read.
 *
 * @return  <code>WMA_NET_SUCCESS</code> if the read was successful;
 *          <code>WMA_NET_WOULDBLOCK</code> if the read would block;
 *          <code>WMA_NET_INTERRUPTED</code> if the read was interrupted;
 *          <code>WMA_NET_IOERROR</code>, otherwise.
 */
WMA_STATUS jsr120_datagram_read(WMA_PROTOCOLS protocol,
                                   unsigned char *pAddress,
                                   jint *port,
                                   char *buffer,
                                   jint length,
                                   jint *pBytesRead)
{
    struct sockaddr_in addr;
    jint len = sizeof(struct sockaddr_in);
    jint status = 0;
    jint fd = -1;

    /* Pick up the socket descriptor for the given protocol. */
    if (protocol == WMA_SMS_PROTOCOL) {
        fd = wmaGetRawSocketFD(smsHandle);
    } else if (protocol == WMA_CBS_PROTOCOL) {
        fd = wmaGetRawSocketFD(cbsHandle);
    } 
#if ENABLE_JSR_205
      else if (protocol == WMA_MMS_PROTOCOL) {
        fd = wmaGetRawSocketFD(mmsHandle);
    }
#endif

    if (fd < 0) {
        /* Unknown protocol type. */
        return WMA_NET_IOERROR;
    }

    /* Read the datagram packet. */
    status = recvfrom(fd, buffer, length, 0, (struct sockaddr*)&addr,
                      (socklen_t *)&len);

    if (SOCKET_ERROR == status) {
        if (EWOULDBLOCK == errno || EINPROGRESS == errno) {
            return WMA_NET_WOULDBLOCK;
        } else if (EINTR == errno) {
            return WMA_NET_INTERRUPTED;
        } else {
            return WMA_NET_IOERROR;
        }
    }

    /* Pick up the network address and port number. */
    memcpy(pAddress, &addr.sin_addr.s_addr, sizeof(pAddress));
    *port = ntohs(addr.sin_port);

    /* Pick up the number of bytes that were actually read. */
    *pBytesRead = status;

    return WMA_NET_SUCCESS;
}

#if ENABLE_JSR_205
/*
 * Write a buffer of bytes using a series of datagram packets. This is
 * MMS-specific at this time, because of the mmsOutPortNumber use.
 *
 * @param outPort The datagram output port number.
 * @param handle A handle to the open network connection.
 * @param toAddr The address to which the datagrams will be sent (Unused).
 * @param length The number of bytes to be sent.
 * @param buf A pointer to the bytes to be sent.
 * @param bytesWritten A return parameter containing the number of bytes that
 *     were actually written.
 */
#if ENABLE_WMA_LOOPBACK == 0
static WMA_STATUS jsr205_datagram_write(jint outPort, void* handle, char *toAddr,
    char* fromAddr, jint length, char* buf, jint *bytesWritten) {

    /** The maximum amount of data allowed within a network packet. */
    jint PACKET_MAX_SIZE = MAX_DATAGRAM_LENGTH - 10;  /* 3 shorts + 1 int */

    /** The socket descriptor that corresponds to the handle. */
    jint socket_fd;

    /** The total number of datagram packets to be sent. */
    jshort totalPackets;

    /** The current packet number. */
    jshort packetNumber;

    /** The IP address used for datagram transfers. */
    unsigned char ipAddr[256];

    /** The length of the IP address to be retrieved. */
    int plen = 0;

    struct sockaddr_in addr;

    /** The buffer for all datagram data. */
    char* dgramBuffer = NULL;

    /** The writing index into the datagram buffer. */
    jint index = 0;

    /** The running pointer to the data to be sent, chunk by chunk. */
    char* p = buf;   /* pointer into buf */

    /** The count of bytes for the current chunk of data to be sent. */
    jint count = 0;

    jint status = WMA_NET_IOERROR;

    /*
     * The "to" and "from" addresses are not used in this implementation. In a
     * more sophisticated implementation that may involve more than one
     * recipient, the packets that get broadcast would need to be directed to
     * specific phones. The "to" address would then come into play and would
     * need to be for each specific phone in a phone list.
     */
    (void)toAddr;
    (void)fromAddr;

    /* Pick up the socket descriptor that corresponds to the handle. */
    socket_fd = wmaGetRawSocketFD(handle);

    *bytesWritten = 0;

    /* Compute the total number of datagrams required to send the message. */
    totalPackets =
        (jshort)((length + PACKET_MAX_SIZE - 1) / PACKET_MAX_SIZE);

    for (packetNumber = 1; packetNumber <= totalPackets; packetNumber++) {

        /** Reset the writing index into the datagram buffer. */
        index = 0;

        if (dgramBuffer == NULL) {
            /* Allocate datagram buffer */
            dgramBuffer = (char*)pcsl_mem_malloc(MAX_DATAGRAM_LENGTH);
        }

        if (dgramBuffer != NULL) {

            /* Initialize the datagram buffer. */
            memset(dgramBuffer, 0, MAX_DATAGRAM_LENGTH);

            /* Compute the number of bytes that can be stuffed into
             * this packet. 
             */
            count = length;
            if (count > PACKET_MAX_SIZE) {
                count = PACKET_MAX_SIZE;
            }

            /* Build the buffer to be written */
            putShort(dgramBuffer, &index, packetNumber);
            putShort(dgramBuffer, &index, totalPackets);
            /* Needed for: count < PACKET_MAX_SIZE */
            putShort(dgramBuffer, &index, count);
            /* Total length of message. */
            putInt(dgramBuffer, &index, length);
            /* Writes count bytes, starting at p. */
            putBytes(dgramBuffer, &index, p, count);

            memset(ipAddr, 0, 256);

            /* Send the datagram into the ether. */
            if (network_gethostbyname(targetHost, ipAddr, 256, &plen)
                != WMA_NET_SUCCESS) {

                return WMA_NET_IOERROR;
            }

            addr.sin_family = AF_INET;
            addr.sin_port   = htons((unsigned short)outPort);
            memcpy(&addr.sin_addr.s_addr, ipAddr,
                   sizeof(addr.sin_addr.s_addr));

            status = sendto(socket_fd, dgramBuffer, MAX_DATAGRAM_LENGTH,
                0, (struct sockaddr*)&addr, sizeof(addr));

            if (SOCKET_ERROR == status) {
                if (EWOULDBLOCK == errno || EINPROGRESS == errno) {
                    status = WMA_NET_WOULDBLOCK;
                } else if (EINTR == errno) {
                    status = WMA_NET_INTERRUPTED;
                } else {
                    status = WMA_NET_IOERROR;
                }
                break;
            }

            *bytesWritten += status;

            status = WMA_NET_SUCCESS;

            /* Move the pointer past the bytes and do next send */
            p += count;
            length -= count;
        }
    }

    /* If a datagram buffer was used, be sure to free its memory. */
    pcsl_mem_free(dgramBuffer);

    return (WMA_STATUS)status;
}
#endif
#endif

/**
 * Initialize string variable from environment or internal properties.
 * 
 * When environment setting is defined, variable receives the value from it.
 * Else it is initialized from property.
 *
 * @param ptrVar The variable address.
 * @param nameEnv The name of environment setting.
 * @param nameProp The name of internal property.
 */
static void load_var_char_env_prop(char **ptrVar, char *nameEnv, char *nameProp) {
    char *charName = getenv(nameEnv);
    if(charName != NULL) {
        *ptrVar = charName;
    } else {
        charName = (char*)getInternalProperty(nameProp);
        if (charName != NULL) {
            *ptrVar = charName;
        }
    }
}

/**
 * Initialize integer variable from environment or internal properties.
 * 
 * When environment setting is defined, variable receives the value from it.
 * Else it is initialized from property.
 *
 * @param ptrVar The variable address.
 * @param nameEnv The name of environment setting.
 * @param nameProp The name of internal property.
 */
static void load_var_jint_env_prop(jint *ptrVar, char *nameEnv, char *nameProp) {
    char *charName;
    load_var_char_env_prop(&charName, nameEnv, nameProp);
    *ptrVar = atoi(charName);
}

/**
 * Determine whether the given phone number is this device's phone number. 
 *
 * @param phoneList The list of phone numbers to be compared against the device's
 *     phone number. The list is preformatted and must be of the form:
 *     <code>phone1;phone2; phone3; ... ; phone_n</code>
 *
 * @return <code>WMA_NET_IOERROR</code> if the device's phone number cannot be
 *     determined. <code>WMA_OK</code> if there's a match;
 *     <code>WMA_ERR</code>, otherwise.
 */
WMA_STATUS is_device_phone_number(char* phoneList) {

    /** Device phone number from property list. */
    char *devPhoneNumber = NULL;

    /** Pointer into the phone number list. */
    char* p = phoneList;

    /** Length of the phone list. */
    jint len = 0;

    /** Length of a phone number in the phone number list. */
    jint phoneLen = 0;

    /** Pointer to the phone number separator. */
    char* sep = NULL;

    load_var_char_env_prop(&devPhoneNumber, "JSR_120_PHONE_NUMBER",
        "com.sun.midp.io.j2me.sms.PhoneNumber");
    if (devPhoneNumber == NULL) {
        /* Cannot determine device's phone number. Fatal error. */
        return WMA_NET_IOERROR;
    }

    if (phoneList != NULL) {

        len = strlen(phoneList);
        while (p < phoneList + len) {

            /* Skip blanks. */
            if (*p == ' ') {
                p++;
                continue;
            }

            /* Get length of phone number, assuming this is the last number. */
            phoneLen = len - (p - phoneList);

            /* Extract the phone number between separators. */
            sep = strchr(p, ';');
            if (sep != NULL) {
                phoneLen = sep - p;
            }

            /* If there's a match, exit now. */
            if (strncmp(p, devPhoneNumber, phoneLen) == 0) {
                return WMA_OK;  
            }

            /* Skip this phone number and try the next one. */
            p = p + phoneLen + 1;
        }
    }

    return WMA_ERR;
}

/**
 * Gets the phone number of device
 *
 * @return The phone number of device.
 * This function returns PCSL_STRING_NULL on pcsl error.
 * Note: the caller should free return value of this function 
 * after using.
 */
pcsl_string getInternalPhoneNumber(void) {
    pcsl_string retValue = PCSL_STRING_NULL;
    const char* phoneNumber = NULL;
    pcsl_string_status status = PCSL_STRING_ERR;
    load_var_char_env_prop((char**)&phoneNumber, "JSR_120_PHONE_NUMBER",
        "com.sun.midp.io.j2me.sms.PhoneNumber");
    if (phoneNumber != NULL) {
        status = pcsl_string_convert_from_utf8((const jbyte *)phoneNumber,
            strlen(phoneNumber), &retValue);
        if (status != PCSL_STRING_OK) {
            retValue = PCSL_STRING_NULL;
        }
    }
    return retValue;
}
/**
 * Write an SMS datagram.
 */
WMA_STATUS jsr120_sms_write(jchar msgType,
                               unsigned char address[],
                               unsigned char msgBuffer[],
                               jchar msgLen,
                               jchar sourcePort,
                               jchar destPort,
                               jint *bytesWritten,
                               void **pContext) {
  
    /** The buffer that will contain all data to be written. */
    char *buffer = NULL;

    /** The writing index. */
    jint index = 0;

    /** The return status. */
    int status = 0;

    /** Internal sms phone number property */
    const char *phNum = NULL;

    /** Physical port to be used to send SMS */
    jint sendPort = smsOutPortNumber; 

    /** Time related structs, variable */
    struct timeval tv;
    jlong timestamp = 0;
    jint tstat = -1;

    (void)sourcePort;
    (void)pContext;

    /* Allocate and initialize datagram buffer */
    buffer = (char*)pcsl_mem_malloc(MAX_DATAGRAM_LENGTH);
    if (buffer == NULL) {
        return WMA_NET_IOERROR;
    }
    memset(buffer, 0, MAX_DATAGRAM_LENGTH);

    /* Get phone number */
    load_var_char_env_prop((char**)&phNum, "JSR_120_PHONE_NUMBER",
        "com.sun.midp.io.j2me.sms.PhoneNumber");

    if (phNum == NULL) {
        /* Free resources and quit */
        pcsl_mem_free(buffer);
        /* Cannot determine device's phone number. Fatal error. */
        return WMA_NET_IOERROR;
    }

    /* Check if SMS is being sent to yourself */
    if (strcmp(phNum, (char *)address) == 0) {
        /*
         * Sending SMS to yourself, swicth port to the one you're 
         * listening on.
         */
        sendPort = smsInPortNumber;
    }

    /* Compute time of sending */
    tstat = gettimeofday(&tv, NULL);
    if (!tstat) { /* Success on getting time of day */
        /* We adjust to 1000 ticks per second */
        timestamp = (jlong)tv.tv_sec * 1000 + tv.tv_usec/1000;
    }

    /* Populate the datagram buffer */
    putInt(buffer, &index, (int)msgType);
    putInt(buffer, &index, (int)destPort);
    putLongLong(buffer, &index, timestamp);
    putString(buffer, &index, (char *)address);
    putString(buffer, &index, (char *)phNum);
    putInt(buffer, &index, (int)msgLen);
    if (msgLen > 0) {
        putBytes(buffer, &index, (char *)msgBuffer, msgLen);
    }

#if ENABLE_WMA_LOOPBACK
    {
        /*
         * This code is enabled mainly for unit tests
         * that want to test send/receive without the
         * network, i.e. message to be sent is put back
         * into the message pool and received by the sender.
         */
        SmsMessage *sms = (SmsMessage *)pcsl_mem_malloc(sizeof(SmsMessage));
        if (sms != NULL) {
            memset(sms, 0, sizeof(SmsMessage));

            sms->encodingType = msgType;
            sms->destPortNum = (unsigned short)destPort;
            sms->timeStamp = timestamp;
            sms->msgAddr = pcsl_mem_strdup((char *)phNum);
            sms->msgLen = msgLen;
            if (msgLen > 0) {
                jint i;
                char *msg = (char*)pcsl_mem_malloc(msgLen + 1);
                for (i = 0; i < msgLen; i++) {
                    msg[i] = msgBuffer[i];
		}
                msg[i] = '\0';
	        sms->msgBuffer = msg;
            }
            /* add message to pool */
           jsr120_sms_pool_add_msg(sms);
           /* Notify all listeners of the new message. */
           jsr120_sms_message_arrival_notifier(sms);

           status = msgLen;
        } else {
           return WMA_NET_IOERROR;
        }

    }
#else
    {
        unsigned char ipAddr[256];
        jint plen = 0;
        struct sockaddr_in addr;

        memset(ipAddr, 0, 256);

        /* Send the datagram into the ether. */
        if (network_gethostbyname(targetHost, ipAddr, 256, &plen) != WMA_NET_SUCCESS) {
            return WMA_NET_IOERROR;
        }

        addr.sin_family = AF_INET;
        addr.sin_port   = htons((unsigned short)sendPort);
        memcpy(&addr.sin_addr.s_addr, ipAddr, sizeof(addr.sin_addr.s_addr));

        status = sendto(wmaGetRawSocketFD(smsHandle), buffer, MAX_DATAGRAM_LENGTH, 0, 
                        (struct sockaddr*)&addr, sizeof(addr));
    }
#endif

    /* Message was sent, so free the buffer. */
    pcsl_mem_free(buffer);

    if (status == SOCKET_ERROR) {
        if ((errno == EWOULDBLOCK) || (errno == EINPROGRESS)) {
            return WMA_NET_WOULDBLOCK;
        } else if (errno == EINTR) {
            return WMA_NET_INTERRUPTED;
        } else {
            return WMA_NET_IOERROR;
        }
    }

    *bytesWritten = status;
    return WMA_NET_SUCCESS;
}

#if ENABLE_JSR_205
/**
 * Send an MMS message.
 *
 * @param sendingToSelf <code>1</code> if sending the message to this device;
 *      <code>0</code>, otherwise.
 * @param toAddr The recipient's MMS address.
 * @param fromAddr The sender's MMS address.
 * @param appID The application ID string associated with this message.
 * @param replyToAppID The reply-to application ID string associated with this
 *     message.
 * @param msgLen The total length, in bytes, of the MMS message.
 * @param msg A pointer to the MMS message, which contains both the message
 *     header and message body structures.
 * @param bytesWritten Returns the number of bytes written after successful
 *     write operation. This is only set if this function returns
 *     WMA_NET_SUCCESS.
 *
 * @return WMA_NET_SUCCESS for successful write operation;\n
 *	 WMA_NET_WOULDBLOCK if the operation would block,\n
 *	 WMA_NET_INTERRUPTED for an Interrupted IO Exception\n
 *	 WMA_NET_IOERROR for all other errors.
 */
WMA_STATUS jsr205_mms_write(jint sendingToSelf, char *toAddr, char* fromAddr, 
                               char* appID, char* replyToAppID, jint msgLen, char* msg, 
                               jint *bytesWritten) {

    /** The total length of all data to be written. */
    jint totalLength = 0;

    /** The buffer that will contain all data to be written. */
    char* buffer = NULL;

    /** The writing index. */
    jint index = 0;

    /** The return status. */
    WMA_STATUS status;

    if (appID == NULL) appID = "";
    if (replyToAppID == NULL) replyToAppID = "";

    /*
     * Create storage for the following:
     * - Application ID and its terminator (1 byte)
     * - The 1-byte terminator for the replyToAppID, or the 1-byte
     *   special flag when replyToAppID is NULL.
     * - The message length data
     * - The message data.
     */
    totalLength = strlen(fromAddr) + 1 +
                  strlen(appID) + 1 +
                  strlen(replyToAppID) + 1 +
                  sizeof(int) + msgLen;
    buffer = (char*)pcsl_mem_malloc(totalLength);
    if (buffer == NULL) {
        return WMA_NET_IOERROR;
    }
    memset(buffer, 0, totalLength);

    /* Populate the buffer to be written. */
    putString(buffer, &index, (char*)fromAddr);
    putString(buffer, &index, (char*)appID);
    putString(buffer, &index, (char*)replyToAppID);
    putInt(buffer, &index, msgLen);
    putBytes(buffer, &index, (char *)msg, msgLen);

#if ENABLE_WMA_LOOPBACK
    (void)sendingToSelf;
    (void)toAddr;

    MmsMessage* mms =
        createMmsMessage(fromAddr, appID, replyToAppID, msgLen, msg);

    /* Add the message to the pool. */
    jsr205_mms_pool_add_msg(mms);
    /* Notify all listeners of the new message. */
    jsr205_mms_message_arrival_notifier(mms);

    /* Fake the written count as well as the "sending" status. */
    *bytesWritten = totalLength;
    status = WMA_NET_SUCCESS;

#else
    /* The buffer to be written would have the reply-to address. */
    (void)replyToAppID;

    int outputPort = mmsOutPortNumber;

    if (sendingToSelf != 0) {
        /* Talking to ourselves; redirect output to the in-port. */
        outputPort = mmsInPortNumber;
    }
    status = jsr205_datagram_write(outputPort, mmsHandle, toAddr, fromAddr,
                                   totalLength, buffer, bytesWritten);

#endif

    /* Message was sent, so free the buffer. */
    pcsl_mem_free(buffer);

    return status;
}
#endif

/**
 * Initialize datagram support for the given protocol.
 */
WMA_STATUS init_jsr120() {

    WMA_STATUS status;

    /* Initialize  WMA_SMS_PROTOCOL */

    /* Check for the SMS-configured datagram input port number. */
    load_var_jint_env_prop(&smsInPortNumber, "JSR_120_SMS_PORT", 
        "com.sun.midp.io.j2me.sms.DatagramPortIn");

    status = jsr120_datagram_open(WMA_SMS_PROTOCOL, smsInPortNumber);
    if (status != WMA_NET_SUCCESS) {
        return WMA_NET_IOERROR;
    }

    /* Check for the SMS-configured datagram output port number. */
    load_var_jint_env_prop(&smsOutPortNumber, "JSR_120_SMS_OUT_PORT", 
        "com.sun.midp.io.j2me.sms.DatagramPortOut");

    /* Get WMA host name */
    load_var_char_env_prop(&targetHost, "JSR_120_DATAGRAM_HOST",
        "com.sun.midp.io.j2me.wma.DatagramHost");
    if (targetHost == NULL) {
        targetHost = targetHostDefault;
    }

    /* Initialize WMA_CBS_PROTOCOL */

    /* Check for the CBS-configured datagram input port number. */
    load_var_jint_env_prop(&cbsInPortNumber, "JSR_120_CBS_PORT", 
        "com.sun.midp.io.j2me.cbs.DatagramPortIn");

    status = jsr120_datagram_open(WMA_CBS_PROTOCOL, cbsInPortNumber);
    if (status != WMA_NET_SUCCESS) {
        return WMA_NET_IOERROR;
    }

#if ENABLE_JSR_205
    /* Initialize WMA_MMS_PROTOCOL */

    /* Check for the MMS-configured datagram input port number. */
    load_var_jint_env_prop(&mmsInPortNumber, "JSR_205_MMS_PORT", 
        "com.sun.midp.io.j2me.mms.DatagramPortIn");

    status = jsr120_datagram_open(WMA_MMS_PROTOCOL, mmsInPortNumber);
    if (status != WMA_NET_SUCCESS) {
        return WMA_NET_IOERROR;
    }

    /* Check for the MMS-configured datagram output port number. */
    load_var_jint_env_prop(&mmsOutPortNumber, "JSR_205_MMS_OUT_PORT", 
        "com.sun.midp.io.j2me.mms.DatagramPortOut");
#endif

    return WMA_NET_SUCCESS;
}

/**
 * Shuts down 
 */
void finalize_jsr120() {

    WMA_STATUS status;

    /* Shut down the inbound datagram ports. */
    status = jsr120_datagram_close(WMA_SMS_PROTOCOL);
    status = jsr120_datagram_close(WMA_CBS_PROTOCOL);
#if ENABLE_JSR_205
    status = jsr120_datagram_close(WMA_MMS_PROTOCOL);
#endif
}

#ifdef __cplusplus
}
#endif
