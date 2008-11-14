/*
 *
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

/*
 * This file implements the native method of the classes that are related
 * to the network monitor utility.
 * The classes are:
 * com.sun.kvem.netmon.NetMonProxy;
 * com.sun.kvem.netmon.HttpAgent
 * com.sun.kvem.netmon.HttpsAgent
 * com.sun.kvem.io.socket.Protocol
 * com.sun.kvem.io.datagram.Protocol
 * com.sun.kvem.io.comm.Protocol
 * com.sun.kvem.io.ssl.Protocol
 * com.sun.kvem.io.j2me.sms.DatagramImpl
 *
 * See the classes javadoc for more info.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kni.h>
#include <lime.h>
#include <stdlib.h>
#include <stdio.h>
#include <midpMalloc.h>

#define BOOL_DEFINED

#if 0
#define DEBUG_PRINT(s) puts(s); fflush(stdout)
#else
#define DEBUG_PRINT(s)
#endif

#define FIELDOFFSET(cl, n, t) \
(lookupField((cl), getNameAndTypeKey((n), (t)))->u.offset)

#define NETMON_HTTP_PACKAGE "com.sun.kvem.netmon.http"
#define HTTP_RECEIVER "HttpMsgReceiver"
#define HTTPS_RECEIVER "HttpsMsgReceiver"

#define NETMON_SOCKET_PACKAGE "com.sun.kvem.netmon.socket"
#define SOCKET_RECEIVER "SocketMsgReceiver"

#define NETMON_SSL_PACKAGE "com.sun.kvem.netmon.ssl"
#define SSL_RECEIVER "SSLMsgReceiver"

#define NETMON_DATAGRAM_PACKAGE "com.sun.kvem.netmon.datagram"
#define DATAGRAM_RECEIVER "DatagramMsgReceiver"

#define NETMON_COMM_PACKAGE "com.sun.kvem.netmon.comm"
#define COMM_RECEIVER "CommMsgReceiver"

#define NETMON_SMS_PACKAGE "com.sun.kvem.netmon.sms"
#define SMS_RECEIVER "SMSMsgReceiver"

#define NETMON_MMS_PACKAGE "com.sun.kvem.netmon.mms"
#define MMS_RECEIVER "MMSMsgReceiver"

#define NETMON_OBEX_PACKAGE "com.sun.kvem.netmon.obex"
#define OBEX_RECEIVER "ObexMsgReceiver"

#define NETMON_BLUETOOTH_PACKAGE "com.sun.kvem.netmon.bluetooth"
#define BLUETOOTH_RECEIVER "BluetoothMsgReceiver"

#define NETMON_APDU_PACKAGE "com.sun.kvem.netmon.apdu"
#define APDU_RECEIVER "APDUMsgReceiver"
#define NETMON_JCRMI_PACKAGE "com.sun.kvem.netmon.jcrmi"
#define JCRMI_RECEIVER "JCRMIMsgReceiver"

#define NETMON_SIP_PACKAGE "com.sun.kvem.netmon.sip"
#define SIP_RECEIVER "SipMsgReceiver"

#define LIME_PACKAGE "com.sun.kvem.midp"
#define LIME_GRAPHICS_CLASS "GraphicsBridge"
#define LIME_MEDIA_CLASS "MediaBridge"
#define LIME_NETWORK_CLASS "NetworkBridge"
#define LIME_HTTP_CLASS "NetworkBridge"
#define LIME_HTTPS_CLASS "SSLNetworkBridge"
#define LIME_EVENT_CLASS "EventBridge"
#define LIME_COMMAND_CLASS "CommandManager"
#define LIME_DEBUG_CLASS "DebugBridge"
#define LIME_TEXT_CLASS "TextManager"
#define LIME_WIDGET_CLASS "WidgetManager"
#define LIME_API_CLASS "APIManager"
#define LIME_DEVICE_CLASS "DeviceBridge"
#define LIME_POPUP_CLASS  "PopupChoiceGroupManager"
#define LIME_RESOURCE_CLASS  "ResourceBridge"

/*
 * boolean isNetworkMonitorActive();
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_io_Connector_isNetworkMonitorActive() {
    KNI_ReturnBoolean(isNetworkMonitorActive());
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_kvem_netmon_NetMonProxy_isNetworkMonitorActive0() {
    KNI_ReturnBoolean(isNetworkMonitorActive());
}

/* HTTP Agent Methods ----------------------------------------------------*/

/*
 * int newStream(String url, int direction, long groupid)
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_kvem_netmon_HttpAgent_newStream0() {
    jlong groupid;
    jint direction;
    jint md;
    jchar* urlData;
    int urlLength;

    KNI_StartHandles(1);
    KNI_DeclareHandle(url);

    static LimeFunction *f = NULL;

    KNI_GetParameterAsObject(1, url);
    direction = KNI_GetParameterAsInt(2);
    groupid = KNI_GetParameterAsLong(3);

    DEBUG_PRINT("in HttpAgent_newStream\n");

    urlLength = KNI_GetStringLength(url);
    urlData = (jchar*)midpMalloc(urlLength * sizeof(jchar));

    if (urlData == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        KNI_GetStringRegion(url, 0, urlLength, urlData);

        if(f == NULL) {
	        /* int newStream(String url, int direction, int groupid) */
	        f = NewLimeFunction(NETMON_HTTP_PACKAGE,
	        HTTP_RECEIVER,
	        "newStream");
        }
        f->call(f, &md, urlData , urlLength, direction, groupid);
        midpFree(urlData);
    }
    KNI_EndHandles();
    KNI_ReturnInt(md);
}

/**
 * void write(int md, int _byte)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_netmon_HttpAgent_write() {
    int byte, md;

    static LimeFunction *f = NULL;

    md = KNI_GetParameterAsInt(1);
    byte = KNI_GetParameterAsInt(2);

    DEBUG_PRINT("in HttpAgent_write\n");

    if(f == NULL) {
	    /* void updateMsgOneByte(int md, int b) */
	    f = NewLimeFunction(NETMON_HTTP_PACKAGE,
	    HTTP_RECEIVER,
	    "updateMsgOneByte");
    }
    f->call(f,NULL,md,byte);
    KNI_ReturnVoid();
}

/**
 * void write(int md, byte[] buff, int off, int len)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_netmon_HttpAgent_writeBuff() {
    int md, off, len;

    KNI_StartHandles(1);
    KNI_DeclareHandle(buffer);

    static LimeFunction *f = NULL;
    jbyte* bufferData = NULL;
    int bufferLength = 0;

    md = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, buffer);
    off = KNI_GetParameterAsInt(3);
    len = KNI_GetParameterAsInt(4);

    DEBUG_PRINT("in HttpAgent_writeBuff\n");

    bufferLength = KNI_GetArrayLength(buffer);
    bufferData = (jbyte*) midpMalloc(bufferLength * sizeof(jbyte));

    if (bufferData == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {

        KNI_GetRawArrayRegion(buffer, 0, bufferLength * sizeof(jbyte),
                bufferData);

	if(f == NULL) {
		/* void updateMsgBuff(int md , byte[] buffer, int off, int len) */
		f = NewLimeFunction(NETMON_HTTP_PACKAGE,
		HTTP_RECEIVER,
		"updateMsgBuff");
	}
	f->call(f, NULL, md, bufferData, bufferLength, off, len);
        midpFree(bufferData);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}


/**
 * void close(int md)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_netmon_HttpAgent_close() {
    int md = KNI_GetParameterAsInt(1);
    static LimeFunction *f = NULL;

    DEBUG_PRINT("in HttpAgent_close\n");

    if(f == NULL) {
	    f = NewLimeFunction(NETMON_HTTP_PACKAGE,
	    HTTP_RECEIVER,
	    "close");
    }
    f->call(f, NULL, md);
    KNI_ReturnVoid();
}

/* HTTPS Agent Methods ---------------------------------------------------*/

/**
 * int newStream(String url, int direction, long groupid)
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_kvem_netmon_HttpsAgent_newStream() {
    int direction;
    jlong groupid;
    jchar* urlData;
    int urlLength;
    int md;

    KNI_StartHandles(1);
    KNI_DeclareHandle(url);

    static LimeFunction *f = NULL;

    KNI_GetParameterAsObject(1, url);
    direction = KNI_GetParameterAsInt(2);
    groupid = KNI_GetParameterAsLong(3);

    DEBUG_PRINT("in HttpsAgent_newStream\n");

    urlLength = KNI_GetStringLength(url);
    urlData = (jchar*)midpMalloc(urlLength * sizeof(jchar));

    if (urlData == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        KNI_GetStringRegion(url, 0, urlLength, urlData);

	if(f == NULL) {
		/* int newStream( String url, int direction, long groupid) */
		f = NewLimeFunction(NETMON_HTTP_PACKAGE,
		HTTPS_RECEIVER,
		"newStream");
	}

	f->call(f, &md, urlData , urlLength, direction, groupid);
        midpFree(urlData);
    }
    KNI_EndHandles();
    KNI_ReturnInt(md);
}

/**
 * void write(int md, int _byte)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_netmon_HttpsAgent_write() {
    int md, byte;
    static LimeFunction *f = NULL;

    md = KNI_GetParameterAsInt(1);
    byte = KNI_GetParameterAsInt(2);

    DEBUG_PRINT("in HttpsAgent_write\n");

    if(f == NULL) {
	    /* void updateOneByte( int md, int b) */
	    f = NewLimeFunction(NETMON_HTTP_PACKAGE,
	    HTTPS_RECEIVER,
	    "updateMsgOneByte");
    }
    f->call( f, NULL, md, byte);
    KNI_ReturnVoid();
}

/**
 * void writeBuff(int md, byte[] buff, int off, int len)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_netmon_HttpsAgent_writeBuff() {
    int md, off, len;

    KNI_StartHandles(1);
    KNI_DeclareHandle(buffer);

    static LimeFunction *f = NULL;
    jbyte* bufferData;
    int bufferLength;

    md = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, buffer);
    off = KNI_GetParameterAsInt(3);
    len = KNI_GetParameterAsInt(4);

    DEBUG_PRINT("in HttpsAgent_write\n");

    bufferLength = KNI_GetArrayLength(buffer);
    bufferData = (jbyte*) midpMalloc(bufferLength * sizeof(jbyte));

    if (bufferData == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {

        KNI_GetRawArrayRegion(buffer, 0, bufferLength * sizeof(jbyte),
                bufferData);

	if(f == NULL) {
		/* void updateMsgBuff(int md,byte[] buff, int off, int len) */
		f = NewLimeFunction(NETMON_HTTP_PACKAGE,
		HTTPS_RECEIVER,
		"updateMsgBuff");
	}
	f->call(f, NULL, md, bufferData, bufferLength, off, len);
        midpFree(bufferData);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * void close(int md)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_netmon_HttpsAgent_close() {
	int md;

	static LimeFunction *f = NULL;

        md = KNI_GetParameterAsInt(1);

	DEBUG_PRINT("in HttpsAgent_close\n");

	if(f == NULL) {
		/* void close(int md) */
		f = NewLimeFunction(NETMON_HTTP_PACKAGE,
		HTTPS_RECEIVER,
		"close");
	}
	f->call(f,NULL,md);
        KNI_ReturnVoid();
}

/* Socket Agent Methods ---------------------------------------------------*/

/**
 * int connect0(String url, int mode, long groupid)
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_kvem_io_j2me_socket_Protocol_connect0() {
    int md;
    jint mode;
    jlong groupid;
    jchar* urlData;
    int urlLength;

    KNI_StartHandles(1);
    KNI_DeclareHandle(url);

    static LimeFunction *f = NULL;

    KNI_GetParameterAsObject(1, url);
    mode = KNI_GetParameterAsInt(2);
    groupid = KNI_GetParameterAsLong(3);

    DEBUG_PRINT("in socket_Protocol_connect0\n");

    urlLength = KNI_GetStringLength(url);
    urlData = (jchar*)midpMalloc(urlLength * sizeof(jchar));

    if (urlData == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        KNI_GetStringRegion(url, 0, urlLength, urlData);

        if(f == NULL) {
          /* int connect(String name, int mode, long groupid) */
          f = NewLimeFunction(NETMON_SOCKET_PACKAGE,
		             SOCKET_RECEIVER,
		             "connect");
        }
        f->call(f, &md, urlData, urlLength, mode, groupid);
        midpFree(urlData);
    }
    KNI_EndHandles();
    KNI_ReturnInt(md);
}

/**
 * void disconnect0(int md)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_socket_Protocol_disconnect0() {
    int md;

    static LimeFunction *f = NULL;

    md = KNI_GetParameterAsInt(1);

    DEBUG_PRINT("in socket_Protocol_disconnect0\n");

    if(f == NULL) {
        f = NewLimeFunction(NETMON_SOCKET_PACKAGE,
		           SOCKET_RECEIVER,
		           "disconnect");
    }
    f->call(f, NULL, md);
    KNI_ReturnVoid();
}

/**
 * void read0(int md, byte b[], int off, int len)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_socket_Protocol_read0() {
    int md, off, len;

    jbyte* data;
    int length;

    KNI_StartHandles(1);
    KNI_DeclareHandle(b);

    md = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, b);
    off = KNI_GetParameterAsInt(3);
    len = KNI_GetParameterAsInt(4);

    DEBUG_PRINT("in socket_Protocol_read0\n");

    length = KNI_GetArrayLength(b);
    data = (jbyte*) midpMalloc(length * sizeof(jbyte));

    if (data == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        static LimeFunction *f = NULL;

        KNI_GetRawArrayRegion(b, 0, length * sizeof(jbyte), data);

        if(f == NULL) {
            f = NewLimeFunction(NETMON_SOCKET_PACKAGE,
		               SOCKET_RECEIVER,
		               "read");
        }
        f->call(f,NULL, md, data, length, off, len);
        midpFree(data);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * void write0(int md, byte b[], int off, int len)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_socket_Protocol_write0() {
    int md, off, len;

    jbyte* data;
    int length;

    KNI_StartHandles(1);
    KNI_DeclareHandle(b);

    md = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, b);
    off = KNI_GetParameterAsInt(3);
    len = KNI_GetParameterAsInt(4);

    DEBUG_PRINT("in socket_Protocol_write0\n");

    length = KNI_GetArrayLength(b);
    data = (jbyte*) midpMalloc(length * sizeof(jbyte));

    if (data == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        static LimeFunction *f = NULL;

        KNI_GetRawArrayRegion(b, 0, length * sizeof(jbyte), data);

        if(f == NULL) {
            f = NewLimeFunction(NETMON_SOCKET_PACKAGE,
		               SOCKET_RECEIVER,
		               "write");
        }
        f->call(f, NULL, md, data, length, off, len);
        midpFree(data);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * void setSocketOption(int md, byte option,  int value)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_socket_Protocol_setSocketOption0() {
    jint md, value;
    jbyte option;

    static LimeFunction *f = NULL;

    md = KNI_GetParameterAsInt(1);
    option = KNI_GetParameterAsByte(2);
    value = KNI_GetParameterAsInt(3);

    DEBUG_PRINT("in socket_Protocol_setSocketOption0\n");

    if(f == NULL) {
        f = NewLimeFunction(NETMON_SOCKET_PACKAGE,
		           SOCKET_RECEIVER,
		           "setSocketOption");
    }
    f->call(f,NULL, md, option, value);
    KNI_ReturnVoid();
}

/* SSL Agent Methods ---------------------------------------------------*/

/**
 * int connect0(String url, int mode, long groupid)
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_kvem_io_j2me_ssl_Protocol_connect0() {
    jlong groupid;
    jint mode;
    jint md;
    jchar* urlData;
    int urlLength;

    static LimeFunction *f = NULL;

    KNI_StartHandles(1);
    KNI_DeclareHandle(url);

    KNI_GetParameterAsObject(1, url);
    mode = KNI_GetParameterAsInt(2);
    groupid = KNI_GetParameterAsLong(3);

    DEBUG_PRINT("in ssl_Protocol_connect0\n");

    urlLength = KNI_GetStringLength(url);
    urlData = (jchar*)midpMalloc(urlLength * sizeof(jchar));

    if (urlData == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        KNI_GetStringRegion(url, 0, urlLength, urlData);

        if(f == NULL) {
            f = NewLimeFunction(NETMON_SSL_PACKAGE,
		               SSL_RECEIVER,
		               "connect");
            }
        f->call(f, &md, urlData, urlLength, mode, groupid);
        midpFree(urlData);
    }
    KNI_EndHandles();
    KNI_ReturnInt(md);
}

/**
 * void disconnect0(int md)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_ssl_Protocol_disconnect0() {
    int md;
    static LimeFunction *f = NULL;

    md = KNI_GetParameterAsInt(1);

    DEBUG_PRINT("in ssl_Protocol_disconnect0\n");

    if(f == NULL) {
        f = NewLimeFunction(NETMON_SSL_PACKAGE,
		           SSL_RECEIVER,
		           "disconnect");
    }
    f->call(f,NULL, md);
    KNI_ReturnVoid();
}

/**
 * void read0(int md, byte b[], int off, int len)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_ssl_Protocol_read0() {
    int md, off, len;

    jbyte* data;
    int length;

    KNI_StartHandles(1);
    KNI_DeclareHandle(b);

    md = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, b);
    off = KNI_GetParameterAsInt(3);
    len = KNI_GetParameterAsInt(4);

    DEBUG_PRINT("in ssl_Protocol_read0\n");

    length = KNI_GetArrayLength(b);
    data = (jbyte*) midpMalloc(length * sizeof(jbyte));

    if (data == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        static LimeFunction *f = NULL;

        KNI_GetRawArrayRegion(b, 0, length * sizeof(jbyte), data);

        if(f == NULL) {
        f = NewLimeFunction(NETMON_SSL_PACKAGE,
		           SSL_RECEIVER,
		           "read");
        }
        f->call(f,NULL, md, data, length, off, len);
        midpFree(data);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * void write0(int md, byte b[], int off, int len)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_ssl_Protocol_write0() {
    int md, off, len;

    jbyte* data;
    int length;

    KNI_StartHandles(1);
    KNI_DeclareHandle(b);

    md = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, b);
    off = KNI_GetParameterAsInt(3);
    len = KNI_GetParameterAsInt(4);


    length = KNI_GetArrayLength(b);
    data = (jbyte*) midpMalloc(length * sizeof(jbyte));

    if (data == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        static LimeFunction *f = NULL;

        KNI_GetRawArrayRegion(b, 0, length * sizeof(jbyte), data);

        if(f == NULL) {
            f = NewLimeFunction(NETMON_SSL_PACKAGE,
		               SSL_RECEIVER,
		               "write");
        }
        f->call(f, NULL, md, data, length, off, len);
        midpFree(data);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * void setSocketOption(int md, byte option,  int value)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_ssl_Protocol_setSocketOption0() {
    int md, value;
    jbyte option;

    static LimeFunction *f = NULL;

    md = KNI_GetParameterAsInt(1);
    option = KNI_GetParameterAsByte(2);
    value = KNI_GetParameterAsInt(3);

    if(f == NULL) {
        f = NewLimeFunction(NETMON_SSL_PACKAGE,
		           SSL_RECEIVER,
		           "setSocketOption");
    }
    f->call(f,NULL, md, option, value);
    KNI_ReturnVoid();
}

/* Datagram Agent Methods ---------------------------------------------------*/

/**
 * int open0(String url, int mode, long groupid)
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_kvem_io_j2me_datagram_Protocol_open0() {
    jlong groupid;
    jint mode;
    jint md;
    jchar* urlData;
    int urlLength;

    static LimeFunction *f = NULL;

    KNI_StartHandles(1);
    KNI_DeclareHandle(url);

    KNI_GetParameterAsObject(1, url);
    mode = KNI_GetParameterAsInt(2);
    groupid = KNI_GetParameterAsLong(3);

    DEBUG_PRINT("in datagram_Protocol_open0\n");

    urlLength = KNI_GetStringLength(url);
    urlData = (jchar*)midpMalloc(urlLength * sizeof(jchar));

    if (urlData == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        KNI_GetStringRegion(url, 0, urlLength, urlData);

        if(f == NULL) {
            f = NewLimeFunction(NETMON_DATAGRAM_PACKAGE,
		               DATAGRAM_RECEIVER,
		               "open");
        }
        f->call(f, &md, urlData, urlLength, mode, groupid);
        midpFree(urlData);
    }
    KNI_EndHandles();
    KNI_ReturnInt(md);
}

/**
 * void close0(int md)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_datagram_Protocol_close0() {
    int md;
    static LimeFunction *f = NULL;

    md = KNI_GetParameterAsInt(1);

    DEBUG_PRINT("in datagram_Protocol_close0\n");

    if(f == NULL) {
        f = NewLimeFunction(NETMON_DATAGRAM_PACKAGE,
		           DATAGRAM_RECEIVER,
		           "close");
    }
    f->call(f,NULL, md);
    KNI_ReturnVoid();
}

/**
 * void receive0(int md)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_datagram_Protocol_receive0() {
    int md;
    static LimeFunction *f = NULL;

    md = KNI_GetParameterAsInt(1);

    if(f == NULL) {
        f = NewLimeFunction(NETMON_DATAGRAM_PACKAGE,
		           DATAGRAM_RECEIVER,
		           "receive");
    }
    f->call(f, NULL, md);
    KNI_ReturnVoid();
}

/**
 * void received0(int md, byte b[], int off, int len)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_datagram_Protocol_received0() {
    int md, off, len;

    jbyte* data;
    int length;

    KNI_StartHandles(1);
    KNI_DeclareHandle(b);

    md = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, b);
    off = KNI_GetParameterAsInt(3);
    len = KNI_GetParameterAsInt(4);

    DEBUG_PRINT("in datagram_Protocol_received0\n");

    length = KNI_GetArrayLength(b);
    data = (jbyte*) midpMalloc(length * sizeof(jbyte));

    if (data == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        static LimeFunction *f = NULL;

        KNI_GetRawArrayRegion(b, 0, length * sizeof(jbyte), data);

        if(f == NULL) {
            f =NewLimeFunction(NETMON_DATAGRAM_PACKAGE,
		               DATAGRAM_RECEIVER,
		               "received");
        }
        f->call(f, NULL, md, data, length, off, len);
        midpFree(data);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * void send0(int md, byte b[], int off, int len)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_datagram_Protocol_send0() {
    int md, off, len;

    jbyte* data;
    int length;

    KNI_StartHandles(1);
    KNI_DeclareHandle(b);

    md = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, b);
    off = KNI_GetParameterAsInt(3);
    len = KNI_GetParameterAsInt(4);

    DEBUG_PRINT("in datagram_Protocol_send0\n");

    length = KNI_GetArrayLength(b);
    data = (jbyte*) midpMalloc(length * sizeof(jbyte));

    if (data == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        static LimeFunction *f = NULL;

        KNI_GetRawArrayRegion(b, 0, length * sizeof(jbyte), data);

        if(f == NULL) {
            f =NewLimeFunction(NETMON_DATAGRAM_PACKAGE,
		               DATAGRAM_RECEIVER,
		               "send");
        }
        f->call(f, NULL, md, data, length, off, len);
        midpFree(data);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/* Comm Agent Methods ---------------------------------------------------*/

/**
 * int connect0(String url, int mode, long groupid)
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_kvem_io_j2me_comm_Protocol_connect0() {
    jlong groupid;
    jint mode;
    jint md;
    jchar* urlData;
    int urlLength;

    static LimeFunction *f = NULL;

    KNI_StartHandles(1);
    KNI_DeclareHandle(url);

    KNI_GetParameterAsObject(1, url);
    mode = KNI_GetParameterAsInt(2);
    groupid = KNI_GetParameterAsLong(3);

    DEBUG_PRINT("in comm_Protocol_connect0\n");

    urlLength = KNI_GetStringLength(url);
    urlData = (jchar*)midpMalloc(urlLength * sizeof(jchar));

    if (urlData == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        KNI_GetStringRegion(url, 0, urlLength, urlData);

        if(f == NULL) {
            f =NewLimeFunction(NETMON_COMM_PACKAGE,
		               COMM_RECEIVER,
		               "connect");
        }
        f->call(f, &md, urlData, urlLength, mode, groupid);
        midpFree(urlData);
    }
    KNI_EndHandles();
    KNI_ReturnInt(md);
}

/**
 * void disconnect0(int md)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_comm_Protocol_disconnect0() {
    int md;

    static LimeFunction *f = NULL;

    md = KNI_GetParameterAsInt(1);

    DEBUG_PRINT("in comm_Protocol_disconnect0\n");

    if(f == NULL) {
        f =NewLimeFunction(NETMON_COMM_PACKAGE,
		           COMM_RECEIVER,
		           "disconnect");
    }
    f->call(f,NULL, md);
    KNI_ReturnVoid();
}

/**
 * void read0(int md, byte b[], int off, int len)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
 Java_com_sun_kvem_io_j2me_comm_Protocol_read0() {
    int md, off, len;

    jbyte* data;
    int length;

    KNI_StartHandles(1);
    KNI_DeclareHandle(b);

    md = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, b);
    off = KNI_GetParameterAsInt(3);
    len = KNI_GetParameterAsInt(4);

    DEBUG_PRINT("in comm_Protocol_read0\n");

    length = KNI_GetArrayLength(b);
    data = (jbyte*) midpMalloc(length * sizeof(jbyte));

    if (data == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        static LimeFunction *f = NULL;

        KNI_GetRawArrayRegion(b, 0, length * sizeof(jbyte), data);

        if(f == NULL) {
            f =NewLimeFunction(NETMON_COMM_PACKAGE,
		               COMM_RECEIVER,
		               "read");
        }
        f->call(f, NULL, md, data, length, off, len);
        midpFree(data);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * void write0(int md, byte b[], int off, int len)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_comm_Protocol_write0() {
    int md, off, len;

    jbyte* data;
    int length;

    KNI_StartHandles(1);
    KNI_DeclareHandle(b);

    md = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, b);
    off = KNI_GetParameterAsInt(3);
    len = KNI_GetParameterAsInt(4);

    DEBUG_PRINT("in comm_Protocol_write0\n");

    length = KNI_GetArrayLength(b);
    data = (jbyte*) midpMalloc(length * sizeof(jbyte));

    if (data == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        static LimeFunction *f = NULL;

        KNI_GetRawArrayRegion(b, 0, length * sizeof(jbyte), data);

        if(f == NULL) {
            f =NewLimeFunction(NETMON_COMM_PACKAGE,
		               COMM_RECEIVER,
		               "write");
        }
        f->call(f, NULL, md, data, length, off, len);
        midpFree(data);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * void setBaudRate0(int md, int baudrate)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_comm_Protocol_setBaudRate0() {
    int md, baudrate;

    static LimeFunction *f = NULL;

    md = KNI_GetParameterAsInt(1);
    baudrate = KNI_GetParameterAsInt(2);

    DEBUG_PRINT("in comm_Protocol_setBaudRate0\n");

    if(f == NULL) {
        f = NewLimeFunction(NETMON_COMM_PACKAGE,
		           COMM_RECEIVER,
		           "setBaudRate");
    }
    f->call(f, NULL, md, baudrate);
    KNI_ReturnVoid();
}

/* SMS Agent Methods ---------------------------------------------------*/
/**
 * int open0(String name, int mode, long groupid)
 */


KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_kvem_io_j2me_sms_DatagramImpl_open0() {
    jlong groupid;
    jint mode;
    jint md;
    jchar* nameData;
    int nameLength;

    static LimeFunction *f = NULL;

    KNI_StartHandles(1);
    KNI_DeclareHandle(name);

    KNI_GetParameterAsObject(1, name);
    mode = KNI_GetParameterAsInt(2);
    groupid = KNI_GetParameterAsLong(3);

    DEBUG_PRINT("in sms_DatagramImpl_open0\n");

    nameLength = KNI_GetStringLength(name);
    nameData = (jchar*)midpMalloc(nameLength * sizeof(jchar));

    if (nameData == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        KNI_GetStringRegion(name, 0, nameLength, nameData);

        if (f==NULL) {
            f = NewLimeFunction(NETMON_SMS_PACKAGE,
		           SMS_RECEIVER,
		           "open");
        }
        f->call(f, &md, nameData, nameLength, mode, groupid);
        midpFree(nameData);
    }
    KNI_EndHandles();
    KNI_ReturnInt(md);
}

/**
 * void close0(int md)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_sms_DatagramImpl_close0() {
    int md;
    static LimeFunction *f = NULL;

    md = KNI_GetParameterAsInt(1);

    DEBUG_PRINT("in sms_DatagramImpl_close0\n");

    if (f==NULL)
    {
        f =NewLimeFunction(NETMON_SMS_PACKAGE,
		       SMS_RECEIVER,
		       "close");
    }
    f->call(f, NULL, md);
    KNI_ReturnVoid();
}

/**
 * void send0(int md, byte datagram[])
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_sms_DatagramImpl_send0() {
    int md;

    jbyte* data;
    int length;

    KNI_StartHandles(1);
    KNI_DeclareHandle(datagram);

    md = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, datagram);

    DEBUG_PRINT("in sms_DatagramImpl_send0\n");

    length = KNI_GetArrayLength(datagram);
    data = (jbyte*) midpMalloc(length * sizeof(jbyte));

    if (data == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        static LimeFunction *f = NULL;

        KNI_GetRawArrayRegion(datagram, 0, length * sizeof(jbyte), data);

        if (f==NULL)
        {
             f = NewLimeFunction(NETMON_SMS_PACKAGE,
                    SMS_RECEIVER,
                    "send");
        }
        f->call(f, NULL, md, data, length);
        midpFree(data);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * void received0(int md, byte datagram[], int length)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_sms_DatagramImpl_received0() {
    int md, len;

    jbyte* data;
    int length;

    KNI_StartHandles(1);
    KNI_DeclareHandle(b);

    md = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, b);
    len = KNI_GetParameterAsInt(3);

    DEBUG_PRINT("in sms_DatagramImpl_received0\n");

    length = KNI_GetArrayLength(b);
    data = (jbyte*) midpMalloc(length * sizeof(jbyte));

    if (data == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        static LimeFunction *f = NULL;

        KNI_GetRawArrayRegion(b, 0, length * sizeof(jbyte), data);

        if (f==NULL)
        {
            f = NewLimeFunction(NETMON_SMS_PACKAGE,
                    SMS_RECEIVER,
                    "received");
        }

        f->call(f, NULL, md, data, length, len);
        midpFree(data);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/* MMS Agent Methods ---------------------------------------------------*/
/**
 * void send0(byte buf[])
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_mms_Protocol_send0() {
    jbyte* data;
    int length;

    KNI_StartHandles(1);
    KNI_DeclareHandle(buf);

    KNI_GetParameterAsObject(1, buf);

    DEBUG_PRINT("in mms_Protocol_send0\n");

    length = KNI_GetArrayLength(buf);
    data = (jbyte*) midpMalloc(length * sizeof(jbyte));

    if (data == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        static LimeFunction *f = NULL;

        KNI_GetRawArrayRegion(buf, 0, length * sizeof(jbyte), data);

        if (f==NULL)
        {
             f = NewLimeFunction(NETMON_MMS_PACKAGE,
                    MMS_RECEIVER,
                    "send");
        }
        f->call(f, NULL, data, length);
        midpFree(data);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * void received0(byte buf[])
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_mms_Protocol_received0() {

    jbyte* data;
    int length;

    KNI_StartHandles(1);
    KNI_DeclareHandle(b);

    KNI_GetParameterAsObject(1, b);

    DEBUG_PRINT("in mms_Protocol_received0\n");

    length = KNI_GetArrayLength(b);
    data = (jbyte*) midpMalloc(length * sizeof(jbyte));

    if (data == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        static LimeFunction *f = NULL;

        KNI_GetRawArrayRegion(b, 0, length * sizeof(jbyte), data);

        if (f==NULL)
        {
            f = NewLimeFunction(NETMON_MMS_PACKAGE,
                    MMS_RECEIVER,
                    "received");
        }

        f->call(f, NULL, data, length);
        midpFree(data);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * void warnPotentialBlocking0()
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_security_SecurityToken_warnPotentialBlocking0(void) {
    static LimeFunction *f = NULL;

    if (f==NULL)
    {
        /* void warnPotentialBlocking() */
        f = NewLimeFunction("com.sun.kvem.midp.lcdui",
                "DisplayBlocking",
                "warnPotentialBlocking");
    }

    f->call(f, NULL);
    KNI_ReturnVoid();
}

/************************************************************************
 *
 * This block contains the native methods that are used by JSR082 Netmon.
 *
 * FIXME: after Netmon for Bluetooth is ready and the netmon structure
 *        is known, more accurate work may be done here to get rid of
 *        unused code.
 *
 ************************************************************************/
#if ENABLE_JSR_82

/**
 * void sendArray(int md, byte b[], int offset, int len)
 * Sending only specified data length.
 */
static void netmon_obex_sendArray(LimeFunction *f) {
    int md, len, offset;

    jbyte* data;
    int length;

    KNI_StartHandles(1);
    KNI_DeclareHandle(b);

    md = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, b);
    offset = KNI_GetParameterAsInt(3);
    len = KNI_GetParameterAsInt(4);

    length = KNI_GetArrayLength(b);

    /* sanity checks */
    if (offset < 0) offset = 0;
    if (len < 0) len = 0;
    if (offset + len > length) {

        /* silently correct bad parameters. */
        offset = 0;
        len = length;
    }

    data = (jbyte*) midpMalloc(len * sizeof(jbyte));

    if (data == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        KNI_GetRawArrayRegion(b, offset * sizeof(jbyte),
                len * sizeof(jbyte), data);

        f->call(f,NULL, md, data, len);
        midpFree(data);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * void connect(String url, long groupid)
 * Sending only specified data length.
 */
static void netmon_obex_connect(LimeFunction *f) {
    int md;
    jlong groupid;
    jchar* urlData;
    int urlLength;

    KNI_StartHandles(1);
    KNI_DeclareHandle(url);

    KNI_GetParameterAsObject(1, url);
    groupid = KNI_GetParameterAsLong(2);

    urlLength = KNI_GetStringLength(url);
    urlData = (jchar*)midpMalloc(urlLength * sizeof(jchar));

    if (urlData == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        KNI_GetStringRegion(url, 0, urlLength, urlData);

        f->call(f, &md, urlData, urlLength, groupid);
        midpFree(urlData);
    }
    KNI_EndHandles();
    KNI_ReturnInt(md);
}

/**
 * int connect0(String url, long groupid)
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_jsr082_bluetooth_kvem_impl_NetmonCommon_connect0() {
    static LimeFunction *f = NULL;

    if (f == NULL) {
        f = NewLimeFunction(NETMON_OBEX_PACKAGE, OBEX_RECEIVER, "connect");
    }
    netmon_obex_connect(f);
}

/**
 * void disconnect0(int md)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr082_bluetooth_kvem_impl_NetmonCommon_disconnect0() {
    static LimeFunction *f = NULL;
    int md = KNI_GetParameterAsInt(1);

    if (f == NULL) {
        f = NewLimeFunction(NETMON_OBEX_PACKAGE, OBEX_RECEIVER, "disconnect");
    }
    f->call(f, NULL, md);
    KNI_ReturnVoid();
}



/**
 * void read0(int md, byte b[], int offset, int len)
 * Sending only specified data length.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr082_bluetooth_kvem_impl_NetmonCommon_read0() {
    static LimeFunction *f = NULL;

    if (f == NULL) {
        f = NewLimeFunction(NETMON_OBEX_PACKAGE, OBEX_RECEIVER, "read");
    }
    netmon_obex_sendArray(f);
}

/**
 * void write0(int md, byte b[], int offset, int len)
 * Sending only specified data length.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr082_bluetooth_kvem_impl_NetmonCommon_write0() {
    static LimeFunction *f = NULL;

    if (f == NULL) {
        f = NewLimeFunction(NETMON_OBEX_PACKAGE, OBEX_RECEIVER, "write");
    }
    netmon_obex_sendArray(f);
}

/* BT SPP netmon functions */

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_jsr082_bluetooth_kvem_btspp_BTSPPNetmonNotifier_notifierConnect0() {
    int md;
    jlong groupid;
    jchar* urlData;
    int urlLength;

    KNI_StartHandles(1);
    KNI_DeclareHandle(url);

    static LimeFunction *f = NULL;

    KNI_GetParameterAsObject(1, url);
    groupid = KNI_GetParameterAsLong(2);

    urlLength = KNI_GetStringLength(url);
    urlData = (jchar*)midpMalloc(urlLength * sizeof(jchar));

    if (urlData == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        KNI_GetStringRegion(url, 0, urlLength, urlData);

        if(f == NULL) {
          /* int connect(String name, long groupid) */
          f = NewLimeFunction(NETMON_BLUETOOTH_PACKAGE,
		              BLUETOOTH_RECEIVER,
		              "notifierConnect");
        }
        f->call(f, &md, urlData, urlLength, groupid);
        midpFree(urlData);
    }
    KNI_EndHandles();
    KNI_ReturnInt(md);
}



KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr082_bluetooth_kvem_btspp_BTSPPNetmonNotifier_updateServiceRecord() {
    static LimeFunction *f = NULL;
    DEBUG_PRINT("in impl_bluetooth_NetmonBluetooth_updateServiceRecord\n");

    if (f == NULL) {
        f = NewLimeFunction(NETMON_BLUETOOTH_PACKAGE,
                BLUETOOTH_RECEIVER, "updateServiceRecord");
    }
    netmon_obex_sendArray(f);
}





/**
 * int connect0(String url, long groupid)
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_jsr082_bluetooth_kvem_btspp_BTSPPNetmonConnection_connect0() {
    static LimeFunction *f = NULL;
    DEBUG_PRINT("in btspp_BTSPPNetmonConnection_connect0\n");

    if (f == NULL) {
        f = NewLimeFunction(NETMON_BLUETOOTH_PACKAGE,
                BLUETOOTH_RECEIVER, "connect");
    }
    netmon_obex_connect(f);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr082_bluetooth_kvem_btspp_BTSPPNetmonConnection_disconnect0() {
    static LimeFunction *f = NULL;
    int md = KNI_GetParameterAsInt(1);
    DEBUG_PRINT("in btspp_BTSPPNetmonConnection_disconnect0\n");

    if (f == NULL) {
        f = NewLimeFunction(NETMON_BLUETOOTH_PACKAGE,
                BLUETOOTH_RECEIVER, "disconnect");
    }
    f->call(f, NULL, md);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr082_bluetooth_kvem_btspp_BTSPPNetmonNotifier_disconnect0() {
    Java_com_sun_jsr082_bluetooth_kvem_btspp_BTSPPNetmonConnection_disconnect0();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr082_bluetooth_kvem_btspp_BTSPPNetmonConnection_read0() {
    static LimeFunction *f = NULL;
    DEBUG_PRINT("in btspp_BTSPPNetmonConnection_read0\n");

    if (f == NULL) {
        f = NewLimeFunction(NETMON_BLUETOOTH_PACKAGE,
                BLUETOOTH_RECEIVER, "read");
    }
    netmon_obex_sendArray(f);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr082_bluetooth_kvem_btspp_BTSPPNetmonConnection_write0() {
    static LimeFunction *f = NULL;
    DEBUG_PRINT("in btspp_BTSPPNetmonConnection_write0\n");

    if (f == NULL) {
        f = NewLimeFunction(NETMON_BLUETOOTH_PACKAGE,
                BLUETOOTH_RECEIVER, "write");
    }
    netmon_obex_sendArray(f);
}

/**
 * int connect0(String url, long groupid)
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_jsr082_bluetooth_kvem_btl2cap_L2CAPNetmonConnection_connect0() {
    static LimeFunction *f = NULL;
    DEBUG_PRINT("in btl2cap_L2CAPNetmonConnection_connect0\n");

    if (f == NULL) {
        f = NewLimeFunction(NETMON_BLUETOOTH_PACKAGE,
                BLUETOOTH_RECEIVER, "connect");
    }
    netmon_obex_connect(f);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr082_bluetooth_kvem_btl2cap_L2CAPNetmonConnection_disconnect0() {
    static LimeFunction *f = NULL;
    int md = KNI_GetParameterAsInt(1);
    DEBUG_PRINT("in btl2cap_L2CAPNetmonConnection_disconnect0\n");

    if (f == NULL) {
        f = NewLimeFunction(NETMON_BLUETOOTH_PACKAGE,
                BLUETOOTH_RECEIVER, "disconnect");
    }
    f->call(f, NULL, md);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr082_bluetooth_kvem_btl2cap_L2CAPNetmonConnection_read0() {
    static LimeFunction *f = NULL;
    DEBUG_PRINT("in btl2cap_L2CAPNetmonConnection_read0\n");

    if (f == NULL) {
        f = NewLimeFunction(NETMON_BLUETOOTH_PACKAGE,
                BLUETOOTH_RECEIVER, "read");
    }
    netmon_obex_sendArray(f);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr082_bluetooth_kvem_btl2cap_L2CAPNetmonConnection_write0() {
    static LimeFunction *f = NULL;
    DEBUG_PRINT("in btl2cap_L2CAPNetmonConnection_read0\n");

    if (f == NULL) {
        f = NewLimeFunction(NETMON_BLUETOOTH_PACKAGE,
                BLUETOOTH_RECEIVER, "read");
    }
    netmon_obex_sendArray(f);
}

/**
 * int connect0(String url, long groupid)
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_jsr082_bluetooth_kvem_impl_NetmonCommon_notifierConnect0() {
    int md;
    jlong groupid;
    jchar* urlData;
    int urlLength;

    KNI_StartHandles(1);
    KNI_DeclareHandle(url);

    static LimeFunction *f = NULL;

    KNI_GetParameterAsObject(1, url);
    groupid = KNI_GetParameterAsLong(2);

    urlLength = KNI_GetStringLength(url);
    urlData = (jchar*)midpMalloc(urlLength * sizeof(jchar));

    if (urlData == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        KNI_GetStringRegion(url, 0, urlLength, urlData);

        if(f == NULL) {
          /* int connect(String name, long groupid) */
          f = NewLimeFunction(NETMON_BLUETOOTH_PACKAGE,
		              BLUETOOTH_RECEIVER,
		              "notifierConnect");
        }
        f->call(f, &md, urlData, urlLength, groupid);
        midpFree(urlData);
    }
    KNI_EndHandles();
    KNI_ReturnInt(md);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr082_bluetooth_kvem_impl_NetmonBluetooth_updateServiceRecord() {
    static LimeFunction *f = NULL;
    DEBUG_PRINT("in impl_bluetooth_NetmonBluetooth_updateServiceRecord\n");

    if (f == NULL) {
        f = NewLimeFunction(NETMON_BLUETOOTH_PACKAGE,
                BLUETOOTH_RECEIVER, "updateServiceRecord");
    }
    netmon_obex_sendArray(f);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr082_bluetooth_kvem_impl_NetmonCommon_notifierDisconnect0() {
    static LimeFunction *f = NULL;
    int md = KNI_GetParameterAsInt(1);
    DEBUG_PRINT("in impl_NetmonCommon_notifierDisconnect0\n");

    if (f == NULL) {
        f = NewLimeFunction(NETMON_BLUETOOTH_PACKAGE,
                BLUETOOTH_RECEIVER, "notifierDisconnect");
    }
    f->call(f, NULL, md);
    KNI_ReturnVoid();
}

#endif /* EXCLUDE_JSR082 */

/*
 * boolean isNetworkMonitorActive0();
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_kvem_environment_NetMon_isNetworkMonitorActive0() {
    KNI_ReturnBoolean(isNetworkMonitorActive());
}

/************************************************************************
 *
 * This block contains the native methods that are used by JSR177 Netmon.
 *
 ************************************************************************/
#if ENABLE_JSR_177


/**
 * void open0(String buf, int groupid)
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_kvem_io_j2me_apdu_Protocol_open0() {
	int md;
    jlong groupid;
	static LimeFunction *f = NULL;

    jsize bufUnicodeLength = -1;
    jchar * bufUnicodeBuffer = 0;

    jint returnValue = 0;

    KNI_StartHandles(1);
    KNI_DeclareHandle(bufHandle);

    KNI_GetParameterAsObject(1, bufHandle);
	groupid = KNI_GetParameterAsLong(2);

    if (f == NULL) {
        f = NewLimeFunction(NETMON_APDU_PACKAGE,
                    APDU_RECEIVER,
                    "open");
    }

    bufUnicodeLength = KNI_GetStringLength(bufHandle);
	bufUnicodeBuffer = (jchar*)malloc(bufUnicodeLength * sizeof (jchar));
	KNI_GetStringRegion(bufHandle, 0, bufUnicodeLength, bufUnicodeBuffer);
	f->call(f, &md,
		bufUnicodeBuffer, bufUnicodeLength, groupid);
	free(bufUnicodeBuffer);

    KNI_EndHandles();
    KNI_ReturnInt(md);
}



/**
 * void exchangeAPDU0(int md, byte buf[])
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_apdu_Protocol_exchangeAPDU0() {
    int md;
	jbyte* data;
    int length;

    KNI_StartHandles(1);

    KNI_DeclareHandle(buf);
	md = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, buf);

    DEBUG_PRINT("in apdu_Protocol_exchangeAPDU0\n");

    length = KNI_GetArrayLength(buf);
    data = (jbyte*) midpMalloc(length * sizeof(jbyte));

    if (data == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        static LimeFunction *f = NULL;

        KNI_GetRawArrayRegion(buf, 0, length * sizeof(jbyte), data);

        if (f == NULL) {
			f = NewLimeFunction(NETMON_APDU_PACKAGE,
						APDU_RECEIVER,
						"exchangeAPDU");
		}
        f->call(f, NULL, md, data, length, -1);
        midpFree(data);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}


/**
 * void open0(String buf, int groupid)
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_kvem_io_j2me_jcrmi_Protocol_open0() {
	int md;
    jlong groupid;
	static LimeFunction *f = NULL;

    jsize bufUnicodeLength = -1;
    jchar * bufUnicodeBuffer = 0;

    jint returnValue = 0;

    KNI_StartHandles(1);
    KNI_DeclareHandle(bufHandle);

    KNI_GetParameterAsObject(1, bufHandle);
	groupid = KNI_GetParameterAsLong(2);

    if (f == NULL) {
        f = NewLimeFunction(NETMON_JCRMI_PACKAGE,
                    JCRMI_RECEIVER,
                    "open");
    }

    bufUnicodeLength = KNI_GetStringLength(bufHandle);
	bufUnicodeBuffer = (jchar*)malloc(bufUnicodeLength * sizeof (jchar));
	KNI_GetStringRegion(bufHandle, 0, bufUnicodeLength, bufUnicodeBuffer);
	f->call(f, &md,
		bufUnicodeBuffer, bufUnicodeLength, groupid);
	free(bufUnicodeBuffer);

    KNI_EndHandles();
    KNI_ReturnInt(md);
}



/**
 * void invoke0(int md, byte buf[], String method)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_jcrmi_Protocol_invoke0() {
    int md;
	jbyte* data;
    int length;
	jchar * methodName;
	int methodNameLength = -1;

    KNI_StartHandles(2);

    KNI_DeclareHandle(buf);
	KNI_DeclareHandle(methodNameHandle);
	md = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, buf);
    KNI_GetParameterAsObject(3, methodNameHandle);

    DEBUG_PRINT("in jcrmi_Protocol_invoke0\n");

    length = KNI_GetArrayLength(buf);
	methodNameLength = KNI_GetStringLength(methodNameHandle);
	methodName = (jchar*)malloc(methodNameLength * sizeof (jchar));
	KNI_GetStringRegion(methodNameHandle, 0, methodNameLength, methodName);
    data = (jbyte*) midpMalloc(length * sizeof(jbyte));

    if (data == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        static LimeFunction *f = NULL;

        KNI_GetRawArrayRegion(buf, 0, length * sizeof(jbyte), data);

        if (f == NULL) {
			f = NewLimeFunction(NETMON_JCRMI_PACKAGE,
						JCRMI_RECEIVER,
						"invoke");
		}
        f->call(f, NULL, methodName, methodNameLength, md, data, length, -1);
        midpFree(data);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * void response0(int md, byte buf[])
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_kvem_io_j2me_jcrmi_Protocol_response0() {
    int md;
	jbyte* data;
    int length;;

    KNI_StartHandles(1);

    KNI_DeclareHandle(buf);
	md = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, buf);

    DEBUG_PRINT("in jcrmi_Protocol_invoke1\n");

    length = KNI_GetArrayLength(buf);
	if (length > 0) {
        data = (jbyte*) midpMalloc(length * sizeof(jbyte));

        if (data == NULL) {
            KNI_ThrowNew("java/lang/OutOfMemoryError", "");
        } else {
            static LimeFunction *f = NULL;

            KNI_GetRawArrayRegion(buf, 0, length * sizeof(jbyte), data);

            if (f == NULL) {
			    f = NewLimeFunction(NETMON_JCRMI_PACKAGE,
						JCRMI_RECEIVER,
						"response");
		    }
            f->call(f, NULL, md, data, length, -1);
            midpFree(data);
        }
	}
    KNI_EndHandles();
    KNI_ReturnVoid();
}
#endif /* EXCLUDE_JSR177 */

/************************************************************************
 *
 * This block contains native methods that are used by JSR180 Netmon.
 *
 ************************************************************************/
#if ENABLE_JSR_180


/**
 * void send0(int md, String method, String serializedHeader, String msg);
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_gov_nist_microedition_sip_SipClientConnectionImpl_send0() {
	int methodLen, headerLen, msgLen;
	jchar *method, *header, *msg;
	jint md;

	md = KNI_GetParameterAsInt(1);

    KNI_StartHandles(3);

    KNI_DeclareHandle(methodHandle);
    KNI_DeclareHandle(headerHandle);
	KNI_DeclareHandle(msgHandle);

    KNI_GetParameterAsObject(2, methodHandle);
    KNI_GetParameterAsObject(3, headerHandle);
    KNI_GetParameterAsObject(4, msgHandle);

    methodLen = KNI_GetStringLength(methodHandle);
    headerLen = KNI_GetStringLength(headerHandle);
    msgLen = KNI_GetStringLength(msgHandle);

    method = (jchar*) malloc(methodLen * sizeof(jchar));
	KNI_GetStringRegion(methodHandle, 0, methodLen, method);

    header = (jchar*) malloc(headerLen * sizeof(jchar));
    msg = (jchar*) malloc(msgLen * sizeof(jchar));

    if (msg == NULL || header == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        static LimeFunction *f = NULL;
        KNI_GetStringRegion(headerHandle, 0, headerLen, header);
        KNI_GetStringRegion(msgHandle, 0, msgLen, msg);

        if (f == NULL) {
			f = NewLimeFunction(NETMON_SIP_PACKAGE,
						SIP_RECEIVER,
						"sdpSend");
		}

        f->call(f, NULL, md, method, methodLen, header, headerLen, msg, msgLen);

        free(header);
        free(msg);
        free(method);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}



/**
 * void send0(int md, String method, String uri, String header, String msg);
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_gov_nist_microedition_sip_SipServerConnectionImpl_send0() {
	int methodLen, uriLen, headerLen, msgLen;
	jchar *method, *uri, *header, *msg;
	jint md;

	md = KNI_GetParameterAsInt(1);

    KNI_StartHandles(4);

    KNI_DeclareHandle(methodHandle);
    KNI_DeclareHandle(uriHandle);
    KNI_DeclareHandle(headerHandle);
	KNI_DeclareHandle(msgHandle);

    KNI_GetParameterAsObject(2, methodHandle);
    KNI_GetParameterAsObject(3, uriHandle);
    KNI_GetParameterAsObject(4, headerHandle);
    KNI_GetParameterAsObject(5, msgHandle);

    methodLen = KNI_GetStringLength(methodHandle);
    uriLen = KNI_GetStringLength(uriHandle);
    headerLen = KNI_GetStringLength(headerHandle);
    msgLen = KNI_GetStringLength(msgHandle);

    method = (jchar*) malloc(methodLen * sizeof(jchar));
	KNI_GetStringRegion(methodHandle, 0, methodLen, method);

	uri = (jchar*) malloc(uriLen * sizeof(jchar));
	KNI_GetStringRegion(uriHandle, 0, uriLen, uri);

    header = (jchar*) malloc(headerLen * sizeof(jchar));
    msg = (jchar*) malloc(msgLen * sizeof(jchar));

    if (msg == NULL || header == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        static LimeFunction *f = NULL;
        KNI_GetStringRegion(headerHandle, 0, headerLen, header);
        KNI_GetStringRegion(msgHandle, 0, msgLen, msg);

        if (f == NULL) {
			f = NewLimeFunction(NETMON_SIP_PACKAGE,
						SIP_RECEIVER,
						"send");
		}

        f->call(f, NULL, md, method, methodLen, uri, uriLen,
            header, headerLen, msg, msgLen);

        free(header);
        free(msg);
        free(method);
        free(uri);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 *  void acceptAndOpen0(int md, String method, String header, String msg);
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_gov_nist_microedition_sip_SipConnectionNotifierImpl_acceptAndOpen0() {
	int methodLen, headerLen, msgLen;
	jchar *method, *header, *msg;
	jint md;

	md = KNI_GetParameterAsInt(1);

    KNI_StartHandles(3);

    KNI_DeclareHandle(methodHandle);
    KNI_DeclareHandle(headerHandle);
	KNI_DeclareHandle(msgHandle);

    KNI_GetParameterAsObject(2, methodHandle);
    KNI_GetParameterAsObject(3, headerHandle);
    KNI_GetParameterAsObject(4, msgHandle);

    methodLen = KNI_GetStringLength(methodHandle);
    headerLen = KNI_GetStringLength(headerHandle);
    msgLen = KNI_GetStringLength(msgHandle);

    method = (jchar*) malloc(methodLen * sizeof(jchar));
	KNI_GetStringRegion(methodHandle, 0, methodLen, method);

    header = (jchar*) malloc(headerLen * sizeof(jchar));
    msg = (jchar*) malloc(msgLen * sizeof(jchar));

    if (msg == NULL || header == NULL) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", "");
    } else {
        static LimeFunction *f = NULL;
        KNI_GetStringRegion(headerHandle, 0, headerLen, header);
        KNI_GetStringRegion(msgHandle, 0, msgLen, msg);

        if (f == NULL) {
			f = NewLimeFunction(NETMON_SIP_PACKAGE,
						SIP_RECEIVER,
						"acceptAndOpen");
		}

        f->call(f, NULL, md, method, methodLen, header, headerLen, msg, msgLen);

        free(header);
        free(msg);
        free(method);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}


/**
 * void responseReceived0(int md, String uri, String header, int statusCode,
 *                 String responsePhrase);
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_gov_nist_microedition_sip_SipClientConnectionImpl_responseReceived0() {
	static LimeFunction *f = NULL;
	int headerLen, uriLen, phraseLen;
	jint md, status;
	jchar *header, *uri, *phrase;

	KNI_StartHandles(3);
	KNI_DeclareHandle(phraseHandle);
	KNI_DeclareHandle(uriHandle);
	KNI_DeclareHandle(headerHandle);

	KNI_GetParameterAsObject(2, uriHandle);
	KNI_GetParameterAsObject(3, headerHandle);
	KNI_GetParameterAsObject(5, phraseHandle);

	uriLen = KNI_GetStringLength(uriHandle);
	phraseLen = KNI_GetStringLength(phraseHandle);
	headerLen = KNI_GetStringLength(headerHandle);

	phrase = (jchar *) malloc(phraseLen * sizeof(jchar));
	KNI_GetStringRegion(phraseHandle, 0, phraseLen, phrase);

	uri = (jchar *) malloc(uriLen * sizeof(jchar));
	KNI_GetStringRegion(uriHandle, 0, uriLen, uri);

	header = (jchar *) malloc(headerLen * sizeof(jchar));
	KNI_GetStringRegion(headerHandle, 0, headerLen, header);

	md = KNI_GetParameterAsInt(1);
	status = KNI_GetParameterAsInt(4);

	if (f == NULL) {
		f = NewLimeFunction(NETMON_SIP_PACKAGE,
					SIP_RECEIVER,
					"responseReceived");
	}

    f->call(f, NULL, md, uri, uriLen, header, headerLen,
        status, phrase, phraseLen);

	free(uri);
	free(header);
    free(phrase);

	KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * int openServerConn0(String name, int portNum);
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_gov_nist_microedition_sip_StackConnector_openServerConn0() {
	static LimeFunction *f = NULL;
	int nameLen;
	jint port, md;
	jchar *name;

	KNI_StartHandles(1);
	KNI_DeclareHandle(nameHandle);
	KNI_GetParameterAsObject(1, nameHandle);
	nameLen = KNI_GetStringLength(nameHandle);
	name = (jchar *) malloc(nameLen * sizeof(jchar));
	KNI_GetStringRegion(nameHandle, 0, nameLen, name);

	port = KNI_GetParameterAsInt(2);
	if (f == NULL) {
		f = NewLimeFunction(NETMON_SIP_PACKAGE,
					SIP_RECEIVER,
					"openServerConn");
	}

    f->call(f, &md, name, nameLen, port);

    free(name);

	KNI_EndHandles();
	KNI_ReturnInt(md);
}

/**
 * int openClientConn0(String name, String uri, int portNum);
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_gov_nist_microedition_sip_StackConnector_openClientConn0() {
	static LimeFunction *f = NULL;
	int nameLen, uriLen;
	jint port, md;
	jchar *name, *uri;

	KNI_StartHandles(2);
	KNI_DeclareHandle(nameHandle);
	KNI_DeclareHandle(clientURIHandle);

	KNI_GetParameterAsObject(1, nameHandle);
	nameLen = KNI_GetStringLength(nameHandle);
	name = (jchar *) malloc(nameLen * sizeof(jchar));
	KNI_GetStringRegion(nameHandle, 0, nameLen, name);

	KNI_GetParameterAsObject(2, clientURIHandle);
	uriLen = KNI_GetStringLength(clientURIHandle);
	uri = (jchar *) malloc(uriLen * sizeof(jchar));
	KNI_GetStringRegion(clientURIHandle, 0, uriLen, uri);

	port = KNI_GetParameterAsInt(3);
	if (f == NULL) {
		f = NewLimeFunction(NETMON_SIP_PACKAGE,
					SIP_RECEIVER,
					"openClientConn");
	}

    f->call(f, &md, name, nameLen, uri, uriLen, port);

    free(name);
	free(uri);

	KNI_EndHandles();
	KNI_ReturnInt(md);
}

#endif /* EXCLUDE_JSR180 */
