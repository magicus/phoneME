/*
 * @(#)socketTransport.c	1.31 06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.  
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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

#include "transportSPI.h"
#include "sysSocket.h"
#include "JDWP.h"

static int serverSocketFD = -1;
static int socketFD = -1;
static TransportCallback *callback;
static JavaVM *jvm;

static int 
setOptions(int fd) 
{
    jvalue dontcare = {0};
    int err; 

    
    err = dbgsysSetSocketOption(fd, SO_REUSEADDR, JNI_TRUE, dontcare);                     
    if (err < 0) {                                                              
        fprintf(stderr,"Error [%d] in SO_REUSEADDR setsockopt() call!\n",errno);
        perror("err:");                                                         
        fprintf(stderr,"Socket transport failed to init.\n");                   
        return err;                                                             
    }                                                                           
    
    err = dbgsysSetSocketOption(fd, TCP_NODELAY, JNI_TRUE, dontcare);
    if (err < 0) {
        fprintf(stderr,"Error [%d] in tcpnodelay() call!\n",errno);
        perror("err:");
        fprintf(stderr,"Socket transport failed to init.\n");
        return err;
    }

    return err;
}

static jint 
socketTransport_listen(char **address)
{
    jint portNum;
    struct sockaddr_in serverSocket;
    int err;

    if ((*address == NULL) || (*address[0] == '\0')) {
        portNum = 0;
    } else {
        char *p;
        for (p = *address; *p != '\0'; p++) {
            if (!isdigit(*p)) {
                fprintf(stderr,"Invalid listen port number: %s\n", *address);
                return SYS_ERR;
            }
        }
        portNum = atoi(*address);
    }

    serverSocketFD = dbgsysSocket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketFD < 0) {
        fprintf(stderr,"Error [%d] in socket() call!\n",errno);
        perror("err:");
        fprintf(stderr,"Socket transport failed to init.\n");
        return serverSocketFD;
    }

    err = setOptions(serverSocketFD);
    if (err < 0) {
        return err;
    }

    memset((void *)&serverSocket,0,sizeof(struct sockaddr_in));
    serverSocket.sin_family = AF_INET;
    serverSocket.sin_port = dbgsysHostToNetworkShort((u_short)portNum);
    serverSocket.sin_addr.s_addr = dbgsysHostToNetworkLong(INADDR_ANY);

    err = dbgsysBind(serverSocketFD, (struct sockaddr *)&serverSocket, 
                  sizeof(serverSocket));
    if (err < 0) {
        fprintf(stderr,"Error [%d] in bind() call!\n",errno);
        perror("err:");
        fprintf(stderr,"Socket transport failed to init.\n");
        return err;
    }

    err = dbgsysListen(serverSocketFD, 1);
    if (err < 0) {
        fprintf(stderr,"Error [%d] in listen() call!\n",errno);
        perror("err:");
        fprintf(stderr,"Socket transport failed to init.\n");
        return err;
    }

    if ((*address == NULL) || (*address[0] == '\0')) {
        char buf[20];
        int len = sizeof(serverSocket);
        err = dbgsysGetSocketName(serverSocketFD, 
                               (struct sockaddr *)&serverSocket, &len);
        portNum = dbgsysNetworkToHostShort(serverSocket.sin_port);
        sprintf(buf, "%d", portNum);
        *address = (*callback->alloc)(strlen(buf) + 1);
        if (*address == NULL) {
            return SYS_NOMEM;
        } else {
            strcpy(*address, buf);
        }
    }

    return SYS_OK;
}

static jint
socketTransport_accept(void) 
{
    int socketLen;
    struct sockaddr_in socket;

    memset((void *)&socket,0,sizeof(struct sockaddr_in));
    socketLen = sizeof(socket);
    socketFD = dbgsysAccept(serverSocketFD,
                               (struct sockaddr *)&socket,
                                &socketLen);

    if (socketFD < 0) {
	/* Silence error message if we closed the socket */
	if (errno != EBADF && errno != ECONNABORTED) {
	    fprintf(stderr,"Error [%d] in accept() call!\n",errno);
	    perror("err:");
	    fprintf(stderr,"Socket transport failed to init.\n");
	    return JDWP_ERROR(INTERNAL);
	}
	return JDWP_ERROR(VM_DEAD);
    }

    return SYS_OK;
}

static jint
socketTransport_stopListening(void)
{
    if (serverSocketFD != -1) {
	dbgsysSocketClose(serverSocketFD);
	serverSocketFD = -1;
    }

    return SYS_OK;
}

static jint
parseAddress(char *addressString, struct sockaddr_in *addr) 
{
    char *hostname;
    char *buffer = NULL;
    int port;
    struct hostent *hp;


    /*
     * TO DO: Could use some more error checking
     */
    char *colon = strchr(addressString, ':');
    if (colon == NULL) {
        hostname = "localhost";
        port = atoi(addressString);    
    } else {
        buffer = (*callback->alloc)(strlen(addressString)+1);
        if (buffer == NULL) {
            return SYS_NOMEM;
        }
        strcpy(buffer, addressString);
        buffer[colon - addressString] = '\0';
        hostname = buffer;
        port = atoi(colon + 1);
    }
    hp = dbgsysGetHostByName(hostname);
    if (hp == NULL) {
        fprintf(stderr,"Error [%d] in gethostbyname() call!\n",errno);
        perror("err:");
        fprintf(stderr,"Socket transport failed to init.\n");
        return SYS_ERR;
    }

    memset(addr, 0, sizeof(*addr));
    addr->sin_port = dbgsysHostToNetworkShort((short)port);
    memcpy(&addr->sin_addr, hp->h_addr_list[0], hp->h_length);
    addr->sin_family = AF_INET;

    (*callback->free)(buffer);

    return SYS_OK;
}

static jint
socketTransport_attach(char *addressString) 
{
    struct sockaddr_in socket;
    int err;
     
    err = parseAddress(addressString, &socket);
    if (err < 0) {
        return err;
    }

    socketFD = dbgsysSocket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0) {
        fprintf(stderr,"Error [%d] in socket() call!\n",errno);
        perror("err:");
        fprintf(stderr,"Socket transport failed to init.\n");
        return socketFD;
    }

    err = setOptions(socketFD);
    if (err < 0) {
        return err;
    }

    err = dbgsysConnect(socketFD, (struct sockaddr *)&socket, 
                                sizeof(socket));
    if (err < 0) {
        fprintf(stderr,"Error [%d] in connect() call!\n",errno);
        perror("err:");
        fprintf(stderr,"Socket transport failed to init.\n");
        return err;
    }

    return SYS_OK;
}

static void 
socketTransport_close(void)
{
    if (socketFD != -1) {
	dbgsysSocketClose(socketFD);
	socketFD = -1;
    }
}

static jint 
socketTransport_sendByte(jbyte data)
{
    jbyte d = data;

    if (dbgsysSend(socketFD,(char *)&d,sizeof(jbyte),0) != sizeof(jbyte))
        return SYS_ERR;

    return SYS_OK;
}

static jint 
socketTransport_receiveByte(jbyte *data)
{
    if (dbgsysRecv(socketFD,(char *)data,sizeof(jbyte),0) != sizeof(jbyte))
        return SYS_ERR;

    return SYS_OK;
}

static jint 
socketTransport_sendPacket(struct Packet *packet)
{
    jint len = 11; /* size of packet header */
    struct PacketData *data;

    data = &(packet->type.cmd.data);
    do {
        len += data->length;
        data = data->next;
    } while (data != NULL);

    len = (jint)dbgsysHostToNetworkLong(len);

    if (dbgsysSend(socketFD,(char *)&len,sizeof(jint),0) != sizeof(jint))
        return SYS_ERR;

    packet->type.cmd.id = (jint)dbgsysHostToNetworkLong(packet->type.cmd.id);

    if (dbgsysSend(socketFD,(char *)&(packet->type.cmd.id)
                 ,sizeof(jint),0) != sizeof(jint))
        return SYS_ERR;

    if (dbgsysSend(socketFD,(char *)&(packet->type.cmd.flags)
                 ,sizeof(jbyte),0) != sizeof(jbyte))
        return SYS_ERR;

    if (packet->type.cmd.flags & FLAGS_Reply) {
        jshort errorCode = dbgsysHostToNetworkShort(packet->type.reply.errorCode);
        if (dbgsysSend(socketFD,(char *)&(errorCode)
                     ,sizeof(jshort),0) != sizeof(jshort))
            return SYS_ERR;
    } else {
        if (dbgsysSend(socketFD,(char *)&(packet->type.cmd.cmdSet)
                     ,sizeof(jbyte),0) != sizeof(jbyte))
            return SYS_ERR;
    
        if (dbgsysSend(socketFD,(char *)&(packet->type.cmd.cmd)
                     ,sizeof(jbyte),0) != sizeof(jbyte))
            return SYS_ERR;
    }

    data = &(packet->type.cmd.data);
    do {
        if (dbgsysSend(socketFD,(char *)data->data,data->length,0) != data->length)
            return SYS_ERR;

        data = data->next;
    } while (data != NULL);

    return SYS_OK;
}

static jint 
recv_fully(int f, char *buf, int len)
{
    int nbytes = 0;
    while (nbytes < len) {
        int res = dbgsysRecv(f, buf + nbytes, len - nbytes, 0);
        if (res < 0) {
	    /* close called on socket */
            return res;
        } else if (res == 0) {
	    /* shutdown called on socket */
	    errno = EBADF;
            return res;
	}
        nbytes += res;
    }
    return nbytes;
}

static jint 
socketTransport_receivePacket(struct Packet *packet) {
    jint length;

    /*
     * This function espicially should take note of
     * the differences between cmd and reply packets!
     */

    /* %comment gordonh024 */
    if (recv_fully(socketFD,(char *)&length,sizeof(jint)) != sizeof(jint))
        return SYS_ERR;

    length = (jint)dbgsysNetworkToHostLong(length);

    if (recv_fully(socketFD,(char *)&(packet->type.cmd.id),sizeof(jint)) != sizeof(jint))
        return SYS_ERR;

    packet->type.cmd.id = (jint)dbgsysNetworkToHostLong(packet->type.cmd.id);

    if (recv_fully(socketFD,(char *)&(packet->type.cmd.flags),sizeof(jbyte)) != sizeof(jbyte))
        return SYS_ERR;

    if (packet->type.cmd.flags & FLAGS_Reply) {
        if (recv_fully(socketFD,(char *)&(packet->type.reply.errorCode),sizeof(jshort)) != sizeof(jshort))
            return SYS_ERR;
    } else {
        if (recv_fully(socketFD,(char *)&(packet->type.cmd.cmdSet),sizeof(jbyte)) != sizeof(jbyte))
            return SYS_ERR;
    
        if (recv_fully(socketFD,(char *)&(packet->type.cmd.cmd),sizeof(jbyte)) != sizeof(jbyte))
            return SYS_ERR;
    }

    length -= ((sizeof(jint) * 2) + (sizeof(jbyte) * 3));

    if (length < 0) {
        return SYS_ERR;
    } else if (length == 0) {
        packet->type.cmd.data.length = 0;
        packet->type.cmd.data.data = NULL;
        packet->type.cmd.data.next = NULL;
    } else {
        packet->type.cmd.data.length = length;
        packet->type.cmd.data.next = NULL;

        packet->type.cmd.data.data= (*callback->alloc)(length);

        if (packet->type.cmd.data.data == NULL)
            return SYS_ERR;

        if (recv_fully(socketFD,(char *)packet->type.cmd.data.data
                       ,length) != length) {
            (*callback->free)(packet->type.cmd.data.data);
            return SYS_ERR;
        }
    }

    return SYS_OK;
}

static struct Transport functions = {
    socketTransport_listen,
    socketTransport_accept,
    socketTransport_stopListening,
    socketTransport_attach,
    socketTransport_sendByte,
    socketTransport_receiveByte,
    socketTransport_sendPacket,
    socketTransport_receivePacket,
    socketTransport_close
};

void 
exitTransportWithError(char *message, char *fileName, 
                       char *date, int lineNumber)
{
    JNIEnv *env;
    jint error;
    char buffer[500];

    sprintf(buffer, "Socket Transport \"%s\" (%s), line %d: %s\n",
            fileName, date, lineNumber, message); 
    error = (*jvm)->GetEnv(jvm, (void **)(void *)&env, JNI_VERSION_1_2);
    if (error != JNI_OK) {
        /*
         * We're forced into a direct call to exit()
         */
        fprintf(stderr, "%s", buffer);
        exit(-1);
    } else {
        (*env)->FatalError(env, buffer);
    }
}

JNIEXPORT jint JNICALL 
JDWP_OnLoad(JavaVM *vm, Transport **tablePtr, 
            TransportCallback *cbTablePtr, void *reserved) 
{
    jvm = vm;
    callback = cbTablePtr;
    *tablePtr = &functions;
    return 0;
}


