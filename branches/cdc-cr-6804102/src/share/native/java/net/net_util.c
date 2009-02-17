/*
 * @(#)net_util.c	1.19 06/10/10
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

#include "jni.h"
#include "jvm.h"
#include "jni_util.h"
#include "net_util.h"

int IPv6_supported() ;

static int IPv6_available;

jint ipv6_available() 
{
    return IPv6_available ;
}


void init_IPv6Available(JNIEnv* env)
{
    jclass iCls;
    jmethodID mid;
    jint preferIPv4Stack;
    
    /* Call the private method java.net.PlainSocketImpl.preferIPv4Stack()
     * to access the property. Accessing the property is a privileged
     * action.
     */
    iCls = (*env)->FindClass(env, "java/net/InetAddress"); 
    CHECK_NULL(iCls);
    mid = (*env)->GetStaticMethodID(env, iCls, "preferIPv4Stack", "()Z");
    CHECK_NULL(mid);
    preferIPv4Stack = (*env)->CallStaticBooleanMethod(env, iCls, mid);
    
    /* 
       Since we have initialized and loaded the Socket library we will 
       check now to whether we have IPv6 on this platform and if the 
       supporting socket APIs are available 
    */
    IPv6_available = IPv6_supported() & (!preferIPv4Stack);
}
