/*
 * @(#)PeerBasedToolkit.c	1.9 06/10/10
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

JNIEXPORT jobject JNICALL
Java_sun_awt_PeerBasedToolkit_getComponentPeer (JNIEnv * env, jclass cls, jobject component)
{
        jfieldID java_awt_Component_peerFID;
        jclass clz;

	if (component == NULL)
		return NULL;

        clz = (*env)->GetObjectClass(env, component);
 
        if (clz == NULL)
            return NULL;
 
        java_awt_Component_peerFID = (*env)->GetFieldID(env, clz, "peer", "Lsun/awt/peer/ComponentPeer;");
  
        if (java_awt_Component_peerFID == 0) 
                return NULL;
 
 	return (*env)->GetObjectField (env, component, java_awt_Component_peerFID);
}

JNIEXPORT jobject JNICALL
Java_sun_awt_PeerBasedToolkit_getMenuComponentPeer (JNIEnv * env, jclass cls, jobject menuComponent)
{
        jfieldID java_awt_MenuComponent_peerFID;
        jclass clz;

	if (menuComponent == NULL)
		return NULL;

        clz = (*env)->GetObjectClass(env, menuComponent);
 
        if (clz == NULL)
            return NULL;
 
        java_awt_MenuComponent_peerFID = (*env)->GetFieldID(env, clz, "peer", "Lsun/awt/peer/MenuComponentPeer;");
  
        if (java_awt_MenuComponent_peerFID == 0) 
                return NULL;

	return (*env)->GetObjectField (env, menuComponent, java_awt_MenuComponent_peerFID);
}
