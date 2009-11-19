/*
 * @(#)transportSPI.h	1.22 06/10/10
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
 *
 */
#ifndef TRANSPORTSPI_H
#define TRANSPORTSPI_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Data structs
 *
 * NOTE! Individual items in the Packet(s) do not need
 * to be adjusted for endianess. Every field *except* for
 * the PacketData.data field is automatically adjusted
 * by the underlying transport. However, there is no way
 * for the transport to know the format of the raw data,
 * so you must handle this. 
 */

#define FLAGS_None         ((jbyte)0x0)
#define FLAGS_Reply        ((jbyte)0x80)

#define REPLY_NoError      ((jshort)0x0)

typedef struct PacketData {
    int length;
    jbyte *data;
    struct PacketData *next;
} PacketData;

typedef struct CmdPacket {
    jint id;
    jbyte flags;
    jbyte cmdSet;
    jbyte cmd;
    PacketData data;
} CmdPacket;

typedef struct ReplyPacket {
    jint id;
    jbyte flags;
    jshort errorCode;
    PacketData data;
} ReplyPacket;

typedef struct Packet {
    union {
        CmdPacket cmd;
        ReplyPacket reply;
    } type;
} Packet;

/*
 * When the transport is loaded, a function named "JDWP_OnLoad" with the 
 * following type is called to initialize the transport
 */ 
struct Transport;
struct TransportCallback;

typedef jint (JNICALL *JDWP_OnLoad_t)(JavaVM *jvm,
                                      struct Transport **transport,
                                      struct TransportCallback *callback,
                                      void *reserved);

/*
 * Transport functions called by the back end. This structure should
 * be returned by JDWP_OnLoad
 */
typedef struct Transport {
    /*
     * TO DO: We'll need to come up with a set of error codes that can
     * be returned by transports for these functions. For now, 0 is 
     * success, non-0 is error. 
     */

    /*
     * TO DO: Also need a way to cancel a pending accept() if we are to
     * support multiple transports simultaneously. It might make sense
     * to break this up into Transport and Connection as it is on the 
     * front end and use the name "close" to cancel the accept.
     */
    jint (*listen)(char **);
    jint (*accept)(void);
    jint (*stopListening)(void);
    jint (*attach)(char *);
    jint (*sendByte)(jbyte);
    jint (*receiveByte)(jbyte *);
    jint (*sendPacket)(struct Packet *);
    jint (*receivePacket)(struct Packet *);
    void (*close)(void);
} Transport;

/*
 * Back end functions called by the transport.
 */
typedef struct TransportCallback {
    void *(*alloc)(jint numBytes);   /* Call this for all allocations */
    void (*free)(void *buffer);      /* Call this for all deallocations */
} TransportCallback;


#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* TRANSPORTSPI_H */
