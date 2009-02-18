/*
 * @(#)WinCEDatagramSocketImpl_md.c	1.5 06/10/10
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

#include "java_net_WinCEDatagramSocketImpl.h"

#include "jni_statics.h"

#define ia_addressID	 JNI_STATIC(java_net_InetAddress, ia_addressID)
#define ia_familyID	 JNI_STATIC(java_net_InetAddress, ia_familyID)

JNIEXPORT void JNICALL
Java_java_net_WinCEDatagramSocketImpl_copyInetAddress(JNIEnv *env,
    jclass cls, jobject dst, jobject src)
{
    jint family = (*env)->GetIntField(env, src, ia_familyID);
    jint address = (*env)->GetIntField(env, src, ia_addressID);
    (*env)->SetIntField(env, dst, ia_familyID, family);
    (*env)->SetIntField(env, dst, ia_addressID, address);
}
