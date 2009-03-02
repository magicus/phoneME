/*
 * @(#)net_util.h	1.28 01/11/13
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

#ifndef NET_UTILS_H
#define NET_UTILS_H

#include "jvm.h"
#include "jni_util.h"
#include "net_util_md.h"

#include "jdknet2cvm.h"

/************************************************************************
 * Macros and misc constants
 */

#define MAX_PACKET_LEN 65536

#define IPv4 1
#define IPv6 2

#define NET_ERROR(env, ex, msg) \
{ if (!(*env)->ExceptionOccurred(env)) JNU_ThrowByName(env, ex, msg) }

#define CHECK_NULL(x) if ((x) == NULL) return;
#define CHECK_NULL_RETURN(x, y) if ((x) == NULL) return y;

/************************************************************************
 * Cached field IDs
 *
 * The naming convention for field IDs is
 *      <class abbrv>_<fieldName>ID
 * i.e. psi_timeoutID is PlainSocketImpl's timeout field's ID.
 */

/* PlainSocketImpl fields */
/*
 * Defined in platform specific PlainSocketImpl
 *
extern jfieldID psi_timeoutID;
extern jfieldID psi_fdID;
extern jfieldID psi_addressID;
extern jfieldID psi_portID;
extern jfieldID psi_localportID;
*/

/* DatagramPacket fields */
/*
extern jfieldID dp_addressID;
extern jfieldID dp_portID;
extern jfieldID dp_bufID;
extern jfieldID dp_offsetID;
extern jfieldID dp_lengthID;
extern jfieldID dp_bufLengthID;
*/

/************************************************************************
 *  Utilities
 */

JNIEXPORT void JNICALL NET_ThrowNew(JNIEnv *env, int errorNum, char *msg);
int NET_GetError();

void NET_ThrowCurrent(JNIEnv *env, char *msg);

jfieldID NET_GetFileDescriptorID(JNIEnv *env);

#ifdef __cplusplus
extern "C" {
#endif

jint ipv6_available() ;
void init_IPv6Available(JNIEnv* env);

#ifdef __cplusplus
}
#endif

void 
NET_AllocSockaddr(struct sockaddr **him, int *len);

void
NET_InetAddressToSockaddr(JNIEnv *env, jobject iaObj, int port, struct sockaddr *him, int *len);

jobject
NET_SockaddrToInetAddress(JNIEnv *env, struct sockaddr *him, int *port);

void
NET_SetTrafficClass(struct sockaddr *him, int trafficClass);

jint
NET_GetPortFromSockaddr(struct sockaddr *him);

jint
NET_SockaddrEqualsInetAddress(JNIEnv *env,struct sockaddr *him, jobject iaObj);

int 
NET_IsIPv4Mapped(jbyte* caddr);

int
NET_IPv4MappedToIPv4(jbyte* caddr);

int 
NET_IsEqual(jbyte* caddr1, jbyte* caddr2);

/* Socket operations 
 *
 * These work just like the JVM_* procedures, except that they may do some 
 * platform-specific pre/post processing of the arguments and/or results.
 */

JNIEXPORT int JNICALL
NET_GetSockOpt(int fd, int level, int opt, void *result, int *len);

JNIEXPORT int JNICALL
NET_SetSockOpt(int fd, int level, int opt, const void *arg, int len);

JNIEXPORT int JNICALL
NET_Bind(int fd, struct sockaddr *him, int len);

JNIEXPORT int JNICALL
NET_MapSocketOption(jint cmd, int *level, int *optname);

void NET_ThrowByNameWithLastError(JNIEnv *env, const char *name,
                                  const char *defaultDetail);

#endif /* NET_UTILS_H */
