/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/** 
 * @file connector.c
 * Interface between the MM RTP Player and the native RTP stack.
 */

#include <stdio.h>

#ifdef CDC
#include "com_sun_mmedia_RTPPlayer.h"
#endif

#include "rtp.h"
#include "mni.h"

#include "jpeg_depacketizer.h"
#include "h263_decoder.h"
#include "mpa_decoder.h"

#ifdef WIN32
#include <winsock.h>
#else
#include <unistd.h>
#define closesocket close
#define INVALID_SOCKET -1
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

extern void mpa_reset(void *decoder);

#define RTP_USABLE_WINSOCK_DLL_NOT_FOUND 1


/** 
 * Opens the RTP connector.
 * 
 * @param connector   Pointer to the RTP_CONNECTOR structure.
 * @param host        The hostname.
 * @param port        The RTP data port.
 * @param multicast   True, if this is a multicast session.
 */
static int connector_open(RTP_CONNECTOR *connector, const char *host, 
			  int port, RTP_BOOL multicast) {
    struct sockaddr_in rtp_client;
    struct sockaddr_in rtcp_client; 
    struct timeval tv;
    struct ip_mreq mreq;
    int rc;

#ifdef WIN32
    u_long nAsync = 1;
    WORD wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD(1, 1);

    rc = WSAStartup(wVersionRequested, &wsaData);

    if (rc != 0) {
        /* a usable WinSock DLL was not found */
        return RTP_USABLE_WINSOCK_DLL_NOT_FOUND;
    }
#endif    

    /* create rtp socket */
    if ((connector->rtp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) 
	== INVALID_SOCKET) { 
        /* the UDP Socket could not be created */
        return RTP_CREATE_UDP_SOCKET_FAILURE;
    }
    
    memset((char *) &connector->rtp_socket, sizeof(connector->rtp_socket), 0);
    
    /* make socket asynchronous */
#ifdef WIN32
    ioctlsocket(connector->rtp_socket, FIONBIO, (u_long FAR *) &nAsync);
#else
    tv.tv_sec = 0;
    tv.tv_usec = 5000; /* 5ms */
      
    setsockopt(connector->rtp_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)); 
#endif

    /* bind port */
    rtp_client.sin_family = AF_INET;
    rtp_client.sin_port = htons((u_short) port);
    rtp_client.sin_addr.s_addr = INADDR_ANY; /* inet_addr(host); */
    
    if (bind(connector->rtp_socket, 
	      (struct sockaddr *)&rtp_client, sizeof(rtp_client)) == -1) {
        /* can't bind to rtp socket */
        return RTP_BIND_UDP_SOCKET_FAILURE;
    }

    if (multicast) {	
        /* join multicast group */
        mreq.imr_multiaddr.s_addr = inet_addr(host);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);

        rc = setsockopt(connector->rtp_socket,IPPROTO_IP,IP_ADD_MEMBERSHIP,
                        (void *) &mreq, sizeof(mreq));
        if (rc < 0) {
	    /* cannot join multicast group */
	    return RTP_JOIN_MULTICAST_GROUP_FAILURE;
        }
    }
    
    /* create rtcp socket */
    if ((connector->rtcp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) 
	== INVALID_SOCKET) { 
        /* failed to create RTCP socket */
        return RTP_CREATE_UDP_SOCKET_FAILURE;
    }

    memset((char *) &connector->rtcp_socket, sizeof(connector->rtcp_socket), 0);

    /* make socket asynchronous */
#ifdef WIN32
    ioctlsocket(connector->rtcp_socket, FIONBIO, (u_long FAR *) &nAsync);
#else
    tv.tv_sec = 0;
    tv.tv_usec = 5000; /* 5ms */
        
    setsockopt(connector->rtcp_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)); 
#endif

    /* bind port */
    rtcp_client.sin_family = AF_INET;
    rtcp_client.sin_port = htons((u_short)(port + 1));
    rtcp_client.sin_addr.s_addr = INADDR_ANY; /* inet_addr(host); */
    
    if (bind(connector->rtcp_socket, 
	      (struct sockaddr *)&rtcp_client, sizeof(rtcp_client)) == -1) {
#ifdef RTP_DEBUG
        printf("Error: can't bind to rtcp socket.\n");
#endif
        return RTP_BIND_UDP_SOCKET_FAILURE;
    }

    if (multicast) {	
        /* join multicast group */
        mreq.imr_multiaddr.s_addr = inet_addr(host);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);

        rc = setsockopt(connector->rtcp_socket,IPPROTO_IP,IP_ADD_MEMBERSHIP,
                        (void *) &mreq, sizeof(mreq));
        if (rc < 0) {
#ifdef RTP_DEBUG
            printf("%s : cannot join multicast group.\n", host);
#endif
	    return RTP_JOIN_MULTICAST_GROUP_FAILURE;
        }
    }

    return RTP_SUCCESS;
}

/**
 * Closes the RTP connector.
 * 
 * @param connector   A pointer to the RTP_CONNECTOR structure.
 */ 
static void connector_close(RTP_CONNECTOR *connector) {
    if (connector->rtp_socket != -1) {
        closesocket(connector->rtp_socket);
	connector->rtp_socket = -1;
    }

    if (connector->rtcp_socket != -1) {
        closesocket(connector->rtcp_socket);
	connector->rtcp_socket = -1;
    }
}

/**
 * Receives RTP packets.
 * 
 * @param rtp_memory  A pointer to the RTP_MEMORY structure.
 * @param read        Contains the number of bytes read when
 *                    this function returns.
 *
 * @return            RTP_SUCCESS, if the call returns successfully,
 *                    RTP_OUT_OF_MEMORY, RTP_INVALID_VERSION, RTP_BAD_FORMAT,
 *                    RTP_WRITE_AUDIO_FAILURE or other error codes from
 *                    the stack in case of failure.
 */
static int receive_rtp_data(RTP_MEMORY *rtp_memory, int *read) {
    int ret = RTP_SUCCESS;

    RTP_CONNECTOR *connector= rtp_memory->connector;

    if (rtp_memory->audio_ready) {
        *read = recv(connector->rtp_socket, connector->recv_buf, RTP_MTU_SIZE, 0);

        if (*read > 0) {
#ifdef RTP_DEBUG
  	    printf("RTP - received %d bytes\n", *read);
#endif
            ret = rtpr_parse_packet(rtp_memory, connector->recv_buf, *read);
        }
    } else {
        ret = rtp_write_audio(rtp_memory);
    }

    return ret;
}

/**
 * Receives RTCP packets.
 * 
 * @param rtp_memory  A pointer to the RTP_MEMORY structure.
 */
static void receive_rtcp_data(RTP_MEMORY *rtp_memory) {
    RTP_CONNECTOR *connector = rtp_memory->connector;

    int read = recv(connector->rtcp_socket, connector->recv_buf, RTP_MTU_SIZE, 0);

    if (read > 0) {
        rtcpr_parse_packet(rtp_memory, connector->recv_buf, read);
    }
}

/*------------------------------------------------------------------*/
/*                  Java-to-Native Interface                        */
/*------------------------------------------------------------------*/

/**
 * Opens the RTP connector.
 * 
 * @param host        The local interface.
 * @param port        The RTP data port.
 * @param multicast   True, if this is a multicast session.
 *
 * @return            Sets the rtp_error variable in RTPPlayer
 *                    to the return code from the native functions.
 *
 * @return            A pointer to the RTP_MEMORY structure.
 */
MNI_RET_TYPE_INT
Java_com_sun_mmedia_RTPPlayer_nConnectorOpen(
MNI_FUNCTION_PARAMS_3(jstring jhost, jint jport, jboolean jmulticast)) {
    RTP_MEMORY *rtp_memory;
    const char *hostname;
    int port, ret;
    RTP_BOOL multicast;
    
    MNI_GET_STRING_PARAM(hostname, jhost, 1);
    MNI_GET_INT_PARAM(port, jport, 2);
    MNI_GET_BOOL_PARAM(multicast, jmulticast, 3);

    if ((ret = rtp_init_memory( &rtp_memory)) == RTP_SUCCESS) {
        ret = connector_open(rtp_memory->connector, hostname, port, multicast);
    }

    MNI_RELEASE_STRING_PARAM(hostname, jhost);

    /* return the error code */
    MNI_SET_INT_FIELD("rtp_error", ret);
#ifdef NATIVE_VIDEO_TRANSFERS
    MNI_SET_BOOL_FIELD("nativeVideoTransfers", MNI_TRUE);
#endif
    /* return a pointer to RTP_MEMORY */
    MNI_RET_VALUE_INT(rtp_memory);
}


/**
 * Receives RTP packets.
 * 
 * @param peer        A pointer to the RTP_MEMORY structure.
 *
 * @return            Sets the rtp_error variable in RTPPlayer
 *                    to the return code from the native functions.
 *
 * @return            The number of bytes of RTP data received.
 */

MNI_RET_TYPE_INT
Java_com_sun_mmedia_RTPPlayer_nReceiveRTP
(MNI_FUNCTION_PARAMS_1(jint jpeer)) {
    int read = 0;
    int peer, ret;

    MNI_GET_INT_PARAM(peer, jpeer, 1);

    ret = receive_rtp_data((RTP_MEMORY *) peer, &read);

    MNI_SET_INT_FIELD("rtp_error", ret);

    MNI_RET_VALUE_INT(read);
}

/**
 * Receives RTCP packets.
 * 
 * @param peer        A pointer to the RTP_MEMORY structure.
 */

MNI_RET_TYPE_VOID
Java_com_sun_mmedia_RTPPlayer_nReceiveRTCP
(MNI_FUNCTION_PARAMS_1(jint jpeer)) {
    int peer;

    MNI_GET_INT_PARAM(peer, jpeer, 1);

    receive_rtcp_data((RTP_MEMORY *) peer);

    MNI_RET_VALUE_VOID;
}


/**
 * Closes the RTP connector.
 * 
 * @param peer        A pointer to the RTP_MEMORY structure.
 */ 

MNI_RET_TYPE_VOID
Java_com_sun_mmedia_RTPPlayer_nConnectorClose
(MNI_FUNCTION_PARAMS_2(jint jpeer, jint jnativeRGB)) {
    int peer;
    int nativeRGB;
    MNI_GET_INT_PARAM(peer, jpeer, 1);
    MNI_GET_INT_PARAM(nativeRGB, jnativeRGB, 2);
    
    connector_close(((RTP_MEMORY *) peer)->connector);
#ifdef NATIVE_VIDEO_TRANSFERS
    MNI_FREE((char*) nativeRGB);
#endif
    rtp_free_memory((RTP_MEMORY *) peer);

    MNI_RET_VALUE_VOID;
}


/** @todo Needs documentation. */

MNI_RET_TYPE_INT
Java_com_sun_mmedia_RTPPlayer_nGetPayloadType
(MNI_FUNCTION_PARAMS_1(jint jpeer)) {
    RTP_SSRC *ssrc_ptr;
    int payload_type = -1;
    int peer;
    RTP_MEMORY *rtp_memory;

    MNI_GET_INT_PARAM(peer, jpeer, 1);

    rtp_memory = (RTP_MEMORY *) peer;

    if (rtp_memory != NULL) {
        ssrc_ptr = rtp_memory->ssrc_table;

        if (ssrc_ptr != NULL) {
	    payload_type = ssrc_ptr->payload_type;  
        }
    }

    MNI_RET_VALUE_INT(payload_type);    
}


/** @todo Needs documentation. */

MNI_RET_TYPE_BOOLEAN
Java_com_sun_mmedia_RTPPlayer_nCopyRGBBuffer
(MNI_FUNCTION_PARAMS_3(jint jpeer, jbyteArray jrgbBuffer, jint jnativeRGB)) {
    RTP_MEMORY *rtp_memory;
    RTP_DECODER *decoder;
    JPEG_DEPACKETIZER *depacketizer;
    H263_DECODER *h263_decoder;
    u_char *data = NULL;
    int peer;
#ifndef NATIVE_VIDEO_TRANSFERS
    int array_len;
#endif
    char *source = NULL;
    int copyLength = 0;
    int nativeRGB;
    
    MNI_GET_INT_PARAM(peer, jpeer, 1);
    MNI_GET_INT_PARAM(nativeRGB, jnativeRGB, 3);

    rtp_memory = (RTP_MEMORY *) peer;

    if (rtp_memory->frame_ready) {
#ifndef NATIVE_VIDEO_TRANSFERS
        MNI_GET_BYTE_ARRAY_PARAM(data, jrgbBuffer, array_len, 2);
#endif
	decoder = rtp_memory->ssrc_table->decoder;

	if (rtp_memory->ssrc_table->payload_type == 26) {

	    depacketizer = (JPEG_DEPACKETIZER *)decoder->state;
	    
	    copyLength = depacketizer->rgbDataLength;
	    source = (char*) depacketizer->rgbData;
	} else if (rtp_memory->ssrc_table->payload_type == 34) {
	    h263_decoder = (H263_DECODER *)decoder->state;
	    copyLength = h263_decoder->rgbBufferLength;
	    source = h263_decoder->rgbBuffer;
	}

	if (copyLength > 0) {
#ifndef NATIVE_VIDEO_TRANSFERS
	    memcpy(data, source, copyLength);
	    MNI_RELEASE_BYTE_ARRAY_PARAM(data, jrgbBuffer, 0);
#else
	    if (nativeRGB == 0) {
		data = (char *) MNI_MALLOC(copyLength);
		MNI_SET_INT_FIELD("nativeRGB", (int) data);
	    } else {
		data = (char *) nativeRGB;
	    }
	    memcpy(data, source, copyLength);
#endif
	}
	
	rtp_memory->frame_ready = FALSE;

	MNI_RET_VALUE_BOOLEAN(1);
    }

    MNI_RET_VALUE_BOOLEAN(0);
}


/** @todo Needs documentation. */

MNI_RET_TYPE_BOOLEAN
Java_com_sun_mmedia_RTPPlayer_nGetFrameSize
(MNI_FUNCTION_PARAMS_1(jint jpeer)) {
    RTP_MEMORY *rtp_memory;
    RTP_DECODER *decoder;
    JPEG_DEPACKETIZER *depacketizer;
    H263_DECODER *h263_decoder;
    int peer;

    /* printf("nGetFrameSize\n");*/
    MNI_GET_INT_PARAM(peer, jpeer, 1);

    rtp_memory = (RTP_MEMORY *) peer;

    if (rtp_memory->frame_ready) {
        decoder = rtp_memory->ssrc_table->decoder;

	if (rtp_memory->ssrc_table->payload_type == RTP_PT_JPEG) {
	    depacketizer = (JPEG_DEPACKETIZER *)decoder->state;    

	    MNI_SET_INT_FIELD("frameWidth", depacketizer->width);
	    MNI_SET_INT_FIELD("frameHeight", depacketizer->height);
	} else if (rtp_memory->ssrc_table->payload_type == RTP_PT_H263) {
	    h263_decoder = (H263_DECODER *)decoder->state;

	    MNI_SET_INT_FIELD("frameWidth", h263_decoder->width);
	    MNI_SET_INT_FIELD("frameHeight", h263_decoder->height);
	}

	MNI_RET_VALUE_BOOLEAN(1);
    }

    MNI_RET_VALUE_BOOLEAN(0);
}


/**
 * Resets the  the RTP connector.
 * 
 * @param peer  A pointer to the RTP_MEMORY structure.
 */ 

MNI_RET_TYPE_VOID
Java_com_sun_mmedia_RTPPlayer_nReset
(MNI_FUNCTION_PARAMS_1(jint jpeer)) {
    int peer;
    RTP_MEMORY *rtp_memory;
    RTP_DECODER *decoder;
    MPA_DECODER *mpa_decoder;
 
    MNI_GET_INT_PARAM(peer, jpeer, 1);

    rtp_memory = (RTP_MEMORY *) peer;

    if (rtp_memory->ssrc_table->payload_type == RTP_PT_MPA) {
        decoder = rtp_memory->ssrc_table->decoder;

	mpa_decoder = (MPA_DECODER *)decoder->state;    

        mpa_decoder->inBufferLength = 0;
        mpa_decoder->inBufferOffset = 0;

        mpa_decoder->outBufferLength = 0;
        mpa_decoder->outBufferOffset = 0;
	
	mpa_reset(mpa_decoder->state);
    }

    MNI_RET_VALUE_VOID;
}

MNI_RET_TYPE_BOOLEAN
Java_com_sun_mmedia_RTPPlayer_nNativeRendering
(MNI_FUNCTION_PARAMS_0()) {
#ifdef ZAURUS
    MNI_RET_VALUE_BOOLEAN(1);    
#else
    MNI_RET_VALUE_BOOLEAN(0);
#endif 
}

