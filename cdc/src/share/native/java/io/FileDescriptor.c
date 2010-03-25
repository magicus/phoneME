/*
 * @(#)FileDescriptor.c	1.22 06/10/10
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
#include "jni_util.h"
#include "jvm.h"

#include "java_io_FileDescriptor.h"

/*******************************************************************/
/*  BEGIN JNI ********* BEGIN JNI *********** BEGIN JNI ************/
/*******************************************************************/

/* field id for jint 'fd' in java.io.FileDescriptor */

#include "jni_statics.h"

/**************************************************************
 * static methods to store field ID's in initializers
 */

JNIEXPORT void JNICALL 
Java_java_io_FileDescriptor_initIDs(JNIEnv *env, jclass fdClass) {
    JNI_STATIC(java_io_FileDescriptor, IO_fd_fdID) = (*env)->GetFieldID(env, fdClass, "fd", "I");
}

/**************************************************************
 * File Descriptor
 */

JNIEXPORT void JNICALL 
Java_java_io_FileDescriptor_sync(JNIEnv *env, jobject thisObj) {
    int fd = (*env)->GetIntField(env, thisObj, JNI_STATIC(java_io_FileDescriptor, IO_fd_fdID));
    if (JVM_Sync(fd) == -1) {
	JNU_ThrowByName(env, "java/io/SyncFailedException", "sync failed");
    }	
}
