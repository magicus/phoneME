/*
 * @(#)InetAddress.c	1.24 06/10/10 
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

#include "java_net_InetAddress.h"
#include "net_util.h"

/************************************************************************
 * InetAddress
 */

#include "jni_statics.h"

/*
 * Class:     java_net_InetAddress
 * Method:    init
 * Signature: ()V 
 */
JNIEXPORT void JNICALL
Java_java_net_InetAddress_init(JNIEnv *env, jclass cls) {
    jclass c;
    init_IPv6Available(env);
    c = (*env)->FindClass(env,"java/net/InetAddress");
    CHECK_NULL(c);
    JNI_STATIC(java_net_InetAddress, ia_class) = (*env)->NewGlobalRef(env, c);
    CHECK_NULL(JNI_STATIC(java_net_InetAddress, ia_class));
    JNI_STATIC(java_net_InetAddress, ia_addressID) = (*env)->GetFieldID(env, JNI_STATIC(java_net_InetAddress, ia_class), "address", "I");
    CHECK_NULL(JNI_STATIC(java_net_InetAddress, ia_addressID));
    JNI_STATIC(java_net_InetAddress, ia_familyID) = (*env)->GetFieldID(env, JNI_STATIC(java_net_InetAddress, ia_class), "family", "I");
    CHECK_NULL(JNI_STATIC(java_net_InetAddress, ia_familyID));
    JNI_STATIC(java_net_InetAddress, ia_preferIPv6AddressID) = (*env)->GetStaticFieldID(env, JNI_STATIC(java_net_InetAddress, ia_class), "preferIPv6Address", "Z"); 
    CHECK_NULL(JNI_STATIC(java_net_InetAddress, ia_preferIPv6AddressID));
}
