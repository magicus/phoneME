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

/**
 * @file
 *
 * Methods to associate network events with the platform event loop.
 */
#include <midp_logging.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <kni.h>

#include <jsr120_sms_structs.h>
#include <jsr120_sms_pool.h>
#include <jsr120_sms_listeners.h>
#include <jsr120_cbs_structs.h>
#include <jsr120_cbs_listeners.h>
#include <jsr120_cbs_pool.h>
#if ENABLE_JSR_205
#include <jsr205_mms_structs.h>
#include <jsr205_mms_pool.h>
#include <jsr205_mms_listeners.h>
#include <jsr205_mms_protocol.h>
#endif
#include <wmaInterface.h>
#include <wmaUDPEmulator.h>
#include <bytePackUnpack.h>
#include <wmaSocket.h>

#include <pcsl_memory.h>
#include <push_server_resource_mgmt.h>
#include <midpMalloc.h>
#include <java_types.h>
#include <midpServices.h>

#ifndef MAX_DATAGRAM_LENGTH
#define MAX_DATAGRAM_LENGTH 1500
#endif /* MAX_DATAGRAM_LENGTH */

#define MAX_ADDR_LENGTH 4

#if ENABLE_JSR_205
/* private MMS variables */
static int packetNumber = 1;
static int mmsOffset = 0;
static char* mmsBuffer = NULL;
static jboolean assemble_frags(char *packet);
#endif

static int checkfilter(char *filter, char *ip);
static jboolean checkReadSignal(int socket);
static jboolean checkWriteSignal(int socket);
static WMA_STATUS wmaGetProtocolByFD(int fd, WMA_PROTOCOLS *pProtocol);

typedef struct _jsr120_socket {
    WMA_PROTOCOLS sockProtocol;
    int	sockFD;
    struct _jsr120_socket *next;
} jsr120_socket;

jsr120_socket *wmaSocketRoot = NULL;

#if ENABLE_JSR_205
/**
 * Create an MMS message structure, containing unformatted message data.
 *
 * @param length The total number of bytes in the buffer.
 * @param buffer The buffer that contains the entire MMS message, which is
 *     composed of both the unformatted message header and message body.
 *
 * @return An <code>MmsMessage</code> structure, containing the unformatted
 *     MMS message data.
 */
MmsMessage* createMmsMessageFromBuffer(char* buffer) {

    MmsMessage* mms = NULL;

    if (buffer != NULL) {

        char* p;
        int index = 0;
        int length;

        mms = (MmsMessage *)pcsl_mem_malloc(sizeof(MmsMessage));
        memset(mms, 0, sizeof(MmsMessage));
        p = buffer;

        /* Get the sender's address. */
        mms->fromAddress = getString(p, &index);

        /* Get the application ID string. */
        mms->appID = getString(p, &index);

        /* Get the reply-to application ID string. */
        mms->replyToAppID = getString(p, &index);

        /* Get the message length and data. */
        length = getInt(p, &index);
        mms->msgLen = length;
        mms->msgBuffer = getBytes(p, &index, length);
    }

    return mms;
}

/**
 * Get a formatted MMS header from unformatted MMS message data..
 *
 * @param message An MMS message, containing unformatted data.
 *
 * @return An <code>MmsHeader</code> structure, containing formatted MMS header
 *     data.
 */
MmsHeader* createMmsHeader(MmsMessage* message) {

    MmsHeader* header = NULL;

    if (message != NULL) {

        /* Later, read header length, then data to be formatted. */
        header = (MmsHeader *)pcsl_mem_malloc(sizeof(MmsHeader));
        memset(header, 0, sizeof(MmsHeader));
        /* Populate the header with data extracted from message fields. */
    }

    return header;
}

/**
 * Release all memory used by an MMS header.
 *
 * @param header The MMS header whose memory will be freed.
 */
void destroyMMSHeader(MmsHeader* header) {

    if (header != NULL) {
        /* Use pcsl_mem_free(header->part) to free each part. */

        /* Free the header itself. */
        pcsl_mem_free(header);
    }
}

#endif

/**
 *
 */
jboolean jsr120_check_signal(midpSignalType signalType, int fd, int status) {
    jboolean ret = KNI_FALSE;
    
    switch (signalType) {
    case NETWORK_READ_SIGNAL:
        ret = checkReadSignal(fd);
        break;
    case HOST_NAME_LOOKUP_SIGNAL:
    case NETWORK_WRITE_SIGNAL:
        ret = checkWriteSignal(fd);
        break;
    default:
        /* Silently fail for unknown signals. */
        break;
    }

    (void) status;
    return ret;
}

jboolean checkWriteSignal(int socket) {
    jboolean ret = KNI_FALSE;
    WMA_PROTOCOLS protocol;

    if (wmaGetProtocolByFD(socket, &protocol) == WMA_OK) {
    
        /* Make an up-call to unblock the thread */
        switch (protocol) {

        case WMA_SMS_PROTOCOL:
            jsr120_sms_message_sent_notifier(0, WMA_OK);
            ret = KNI_TRUE;
            break;

        case WMA_CBS_PROTOCOL:
            /* No write signal supported for CBS */
            break;

        case WMA_MMS_PROTOCOL:
#if ENABLE_JSR_205
            jsr205_mms_message_sent_notifier(0, WMA_OK);
            ret = KNI_TRUE;
#endif
            break;
        }
    }

    return ret;
}

jboolean checkReadSignal(int socket) {

    WMA_STATUS status;
    unsigned char ipBytes[MAX_ADDR_LENGTH];
    jint ipPort;
    jint datagramLength;
    jint length;
    char* msg = NULL;
    char* recipientPhone = NULL;
    char *filter = NULL;
    jint i;
    WMA_PROTOCOLS sockProtocol;
    char* p;
    jint index;

    /* Message structures for the protocols SMS and CBS. */
    SmsMessage* sms = NULL;
    CbsMessage* cbs = NULL;

#if ENABLE_JSR_205
    /* Message structures for MMS. */
    MmsMessage* mms = NULL;
#endif

    if (wmaGetProtocolByFD(socket, &sockProtocol) != WMA_OK)
        return KNI_FALSE;

    /* Read the datagram. */
    p = (char *)pcsl_mem_malloc(MAX_DATAGRAM_LENGTH);
    if (p != NULL) {
        memset(p, 0, MAX_DATAGRAM_LENGTH);

        status = jsr120_datagram_read(sockProtocol, ipBytes, &ipPort, p, MAX_DATAGRAM_LENGTH,
                                      &datagramLength);  

        if (status == WMA_NET_SUCCESS) {
            index = 0;

            switch (sockProtocol) {

            case WMA_SMS_PROTOCOL:
                sms = (SmsMessage *)pcsl_mem_malloc(sizeof(SmsMessage));
                if (sms != NULL) {
                    memset(sms, 0, sizeof(SmsMessage));

                    sms->encodingType = getInt(p, &index);
                    sms->sourcePortNum = ipPort;
                    sms->destPortNum = (unsigned short)getInt(p, &index);
                    sms->timeStamp = getLongLong(p, &index);
                    /* The next field is the recipient's phone number */
                    recipientPhone = getString(p, &index);
                    pcsl_mem_free(recipientPhone);
                    /* The next field is the sender's phone number */
                    sms->msgAddr = getString(p, &index);

                    length = getInt(p, &index);
                    sms->msgLen = length;
                    if (length > 0) {
                        msg = (char*)pcsl_mem_malloc(length + 1);
                        for (i = 0; i < length; i++) {
                            msg[i] = p[index++];
                        }
                        msg[i] = '\0';
                        sms->msgBuffer = msg;
                    }

                    /*
                     * Check for a push entry for this connection.
                     */
                    filter = pushgetfilter("sms://:", sms->destPortNum);

                    /*
                     * File this message, if there is no push entry for it
                     * i.e. filter is NULL or if there is a push entry for it
                     * and it passes the check filter test. The sender's phone
                     * number is checked against the filter.
                     */
                    if (filter == NULL || checkfilter(filter, sms->msgAddr)) {
                        /* add message to pool */
                        jsr120_sms_pool_add_msg(sms);
                        /* Notify all listeners of the new message. */
                        jsr120_sms_message_arrival_notifier(sms);
                    }

                    if (filter != NULL) {
                        pcsl_mem_free(filter);
                    }
                }

                break;

            case WMA_CBS_PROTOCOL:
                cbs = (CbsMessage*)pcsl_mem_malloc(sizeof(CbsMessage));
                if (cbs != NULL) {
                    memset(cbs, 0, sizeof(CbsMessage));

                    cbs->encodingType = (ENCODING_TYPE)getInt(p, &index);
                    cbs->msgID = getInt(p, &index);

                    /* Pick up the message length and the message data. */
                    length = getInt(p, &index);
                    cbs->msgLen = length;
                    if (length > 0) {
                        msg = (char*)pcsl_mem_malloc(length);
                        for (i = 0; i < length; i++) {
                            msg[i] = p[index++];
                        }
                        cbs->msgBuffer = (unsigned char*)msg;
                    }

                    /* Add message to pool. */
                    jsr120_cbs_pool_add_msg(cbs);
		    /* Notify all listeners of the new message. */
		    jsr120_cbs_message_arrival_notifier(cbs);
                }
                break;

#if ENABLE_JSR_205

            case WMA_MMS_PROTOCOL:

                if (assemble_frags(p) == KNI_TRUE) {

                    /*
                     * Note:
                     *
                     * The message is currently received in whole. That is, the
                     * message header and body are contained in the single
                     * MmsMessage. Normally, the header would be received on its
                     * own via (in this case, for example), some sort of state
                     * machine that processes the header, 
                     */
                    MmsHeader* mmsHeader;

                    /* Create a new MMS message */
                    mms = createMmsMessageFromBuffer(mmsBuffer);
                    pcsl_mem_free(mmsBuffer);
                    mmsBuffer = NULL;

                    /* Notify the platform that a message has arrived. */
                    mmsHeader = createMmsHeader(mms);
                    destroyMMSHeader(mmsHeader);

                    /*
                     * Check for a push entry for this connection.
                     */
                    filter = pushgetfiltermms("mms://:", mms->appID);

                    /*
                     * File this message, if there is no push entry for it
                     * i.e. filter is NULL or if there is a push entry for it
                     * and it passes the check filter test
                     */
                    if (filter == NULL || checkfilter(filter, mms->replyToAppID)) {

                        jsr205_mms_pool_add_msg(mms);
                        /* Notify all listeners of the new message. */
                        jsr205_mms_message_arrival_notifier(mms);
                    }

                    pcsl_mem_free(filter);
                }

                break;
#endif

            default:
                /* RFC: Silently fail for unknown protocols. */
                break;

            }  /* switch*/
        }
        pcsl_mem_free(p);
    }

    return KNI_TRUE;
}

/**
 * Creates a platform-specific handle.
 *
 * @param fd platform-specific file descriptor to be associated with
 *		the new handle
 *
 * @return the platform-specific handle; NULL if there was an error
 */
void *
wmaCreateSocketHandle(WMA_PROTOCOLS protocol, int fd) {
    jsr120_socket* ws;

    ws = (jsr120_socket*)pcsl_mem_malloc(sizeof(jsr120_socket));
    ws->sockProtocol = protocol;
    ws->sockFD = fd;

    ws->next = wmaSocketRoot;
    wmaSocketRoot = ws;

    return ws;
}

/**
 * Gets the platform-specific file descriptor associated with the
 * given handle.
 *
 * @param handle platform-specific handle to the open connection
 *
 * @return the associated platform-specific file descriptor; a negative
 *	   number if there was an error
 */
int
wmaGetRawSocketFD(void *handle) {
    if (handle != NULL) {
	jsr120_socket* ws = (jsr120_socket *)handle;
	return ws->sockFD;
    }

    return -1;
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
wmaDestroySocketHandle(void *handle) {
    jsr120_socket *ws;

    ws = (jsr120_socket*)(handle);

    if (ws == wmaSocketRoot) {
        wmaSocketRoot = ws->next;
    } else {
        jsr120_socket *wsPrev;

        for (wsPrev = wmaSocketRoot; wsPrev != NULL && wsPrev->next != ws; wsPrev = wsPrev->next)
            ;
        if (wsPrev != NULL) {
            wsPrev->next = ws->next;
        }
    }

    pcsl_mem_free(ws);
}

/**
 *
 */
jsr120_socket *
wmaGetWMASocketByFD(int fd) {
    jsr120_socket *ws;

    for (ws = wmaSocketRoot; ws != NULL && ws->sockFD != fd; ws = ws->next)
        ;

    return ws;
}

/**
 *
 */
static WMA_STATUS
wmaGetProtocolByFD(int fd, WMA_PROTOCOLS *pProtocol) {
    jsr120_socket *ws = wmaGetWMASocketByFD(fd);
    WMA_STATUS status = WMA_ERR;

    if (ws != NULL) {
        *pProtocol = ws->sockProtocol;
        status = WMA_OK;
    }

    return status;
}

/**
 * check the SMS header against the push filter.
 * @param filter The filter string to be used
 * @param cmsidn The caller's MSIDN number to be tested by the filter
 * @return <code>1</code> if the comparison is successful; <code>0</code>,
 *     otherwise.
 */
static int checkfilter(char *filter, char *cmsidn) {
    char *p1 = NULL;
    char *p2 = NULL;

#if REPORT_LEVEL <= LOG_INFORMATION
    if (filter != NULL && cmsidn != NULL) {
        reportToLog(LOG_INFORMATION, LC_PROTOCOL,
                    "in checkfilter[%s , %s]",
                    filter, cmsidn);
    }
#endif
    if ((cmsidn == NULL) || (filter == NULL)) return 0;

    /* Filter is exactly "*", then all MSIDN numbers are allowed. */
    if (strcmp(filter, "*") == 0) return 1;

    /*
     * Otherwise walk through the filter string looking for character
     * matches and wildcard matches.
     * The filter pointer is incremented in the main loop and the
     * MSIDN pointer is incremented as characters and wildcards
     * are matched. Checking continues until there are no more filter or
     * MSIDN characters available.
     */
    for (p1=filter, p2=cmsidn; *p1 && *p2; p1++) {
        /*
         * For an asterisk, consume all the characters up to
         * a matching next character.
         */
        if (*p1 == '*') {
            /* Initialize the next two filter characters. */
            char f1 = *(p1+1);
            char f2 = '\0';
            if (f1 != '\0') {
                f2 = *(p1+2);
            }

            /* Skip multiple wild cards. */
            if (f1 == '*') {
                continue;
            }

            /*
             * Consume all the characters up to a match of the next
             * character from the filter string. Stop consuming
             * characters, if the address is fully consumed.
             */
            while (*p2) {
                /*
                 * When the next character matches, check the second character
                 * from the filter string. If it does not match, continue
                 * consuming characters from the address string.
                 */
                if(*p2 == f1 || f1 == '?') {
                    if (*(p2+1) == f2 || f2 == '?' || f2 == '*') {
                        /* Always consume an address character. */
                        p2++;
                        if (f2 != '?' || *(p2+1) == '.' || *(p2+1) == '\0') {
                            /* Also, consume a filter character. */
                            p1++;
                        }
                        break;
                    }
                }
                p2++;
            }
        } else if (*p1 == '?') {
            p2 ++;
        } else if (*p1 != *p2) {
            /* If characters do not match, filter failed. */
            return 0;
        } else {
            p2 ++;
 	}
    }

    if (!(*p1)  && !(*p2) ) {
        /* 
         * All available filter and MSIDN characters were checked.
         */
        return 1;
    } else {
        /*
         * Mismatch in length of filter and MSIDN string
         */
        return 0;
    }
}

#if ENABLE_JSR_205
/**
 * Assemble fragments, one packet a time, into a data buffer.
 *
 * @param packet The packet containing a fragment to be assembled.
 * @param length A return parameter containing the total number of bytes in the
 *     data buffer.
 *
 * @return <code>KNI_TRUE</code> if the packet contained the last fragment in the
 *     collection; <code>KNI_FALSE</code>, otherwise.
 */
static jboolean assemble_frags(char *packet) {

    int index = 0;
    short packNum = -1;
    short numPackets = 0;
    short count = 0;
    int totalLen = 0;

    /*
     * Extract the packet number, # packets,
     * no of bytes in this packet and total # of bytes
     */
    packNum = getShort(packet, &index);
    numPackets = getShort(packet, &index);
    count = getShort(packet, &index);
    totalLen = getInt(packet, &index);

    if (packNum != packetNumber) {
        /*
         * Mismatch in packet number. Packets have
         * either arrived out of order or a packet
         * has been dropped.
         */
        fprintf(stderr, "ERROR: Datagram packets have been dropped\n");
        if (mmsBuffer != NULL) {
            pcsl_mem_free(mmsBuffer);
            mmsBuffer = NULL;
        }
        return KNI_FALSE;
    }
                
    /*
     * For the first packet, release any storage from a previous assembly and
     * prepare the main storage buffer for assembling data fragments.
     */
    if (packNum == 1) {
        if (mmsBuffer != NULL) {
            pcsl_mem_free(mmsBuffer);
            mmsBuffer = NULL;
        }
        mmsBuffer = (char *)pcsl_mem_malloc(totalLen);
        memset(mmsBuffer, 0, totalLen);
        mmsOffset = 0;
    }

    /* Append the data fragment. */
    memcpy(mmsBuffer + mmsOffset, packet + index, count);

    /* Check if this is the last packet */
    if (packNum >= numPackets) {

        /* Reset the expected packet number and storage offset. */
        packetNumber = 1;
        mmsOffset = 0;

        /* This packet was the last fragment. */
        return KNI_TRUE;
    }

    /* Update the expected packet number and storage offset. */
    packetNumber++;
    mmsOffset += count;

    /* This packet was not the last fragment. */
    return KNI_FALSE;
}

#endif
