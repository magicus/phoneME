/*
 * @(#)DatagramPacket.c	1.11 01/11/13
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

#include "java_net_DatagramPacket.h"
#include "net_util.h"

/************************************************************************
 * DatagramPacket
 */

#include "jni_statics.h"

/*
 * Class:     java_net_DatagramPacket
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_DatagramPacket_init (JNIEnv *env, jclass cls) {
    JNI_STATIC(java_net_DatagramPacket, dp_addressID) = (*env)->GetFieldID(env, cls, "address", "Ljava/net/InetAddress;");
    CHECK_NULL(JNI_STATIC(java_net_DatagramPacket, dp_addressID));
    JNI_STATIC(java_net_DatagramPacket, dp_portID) = (*env)->GetFieldID(env, cls, "port", "I");
    CHECK_NULL(JNI_STATIC(java_net_DatagramPacket, dp_portID));
    JNI_STATIC(java_net_DatagramPacket, dp_bufID) = (*env)->GetFieldID(env, cls, "buf", "[B");
    CHECK_NULL(JNI_STATIC(java_net_DatagramPacket, dp_bufID));
    JNI_STATIC(java_net_DatagramPacket, dp_offsetID) = (*env)->GetFieldID(env, cls, "offset", "I");
    CHECK_NULL(JNI_STATIC(java_net_DatagramPacket, dp_offsetID));
    JNI_STATIC(java_net_DatagramPacket, dp_lengthID) = (*env)->GetFieldID(env, cls, "length", "I");
    CHECK_NULL(JNI_STATIC(java_net_DatagramPacket, dp_lengthID));
    JNI_STATIC(java_net_DatagramPacket, dp_bufLengthID) = (*env)->GetFieldID(env, cls, "bufLength", "I");
    CHECK_NULL(JNI_STATIC(java_net_DatagramPacket, dp_bufLengthID));
}
