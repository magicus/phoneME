/*
 * @(#)jni_statics_md.h	1.6 06/10/10
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

#ifndef INCLUDED_JNI_STATICS_MD_H
#define INCLUDED_JNI_STATICS_MD_H

/* Use this macro to access a variable in this structure */
#define JNI_STATIC_MD(CLASS, VARIABLE) \
    (CVMjniGlobalStatics.platformStatics.CLASS ## _ ## VARIABLE)

/* This macro is used internally to define the structure */
#define DECL_JNI_STATIC_MD(TYPE, CLASS, VARIABLE) \
    TYPE CLASS ## _ ## VARIABLE

struct _CVMJNIStatics_md {
    /* These are actually machine dependent (but should be identical on all platforms) */
    DECL_JNI_STATIC_MD(jfieldID, java_net_PlainDatagramSocketImpl, IO_fd_fdID);
    DECL_JNI_STATIC_MD(jfieldID, java_net_PlainDatagramSocketImpl, pdsi_fdID);
    DECL_JNI_STATIC_MD(jfieldID, java_net_PlainDatagramSocketImpl, pdsi_timeoutID);
    DECL_JNI_STATIC_MD(jfieldID, java_net_PlainDatagramSocketImpl, pdsi_localPortID);
    DECL_JNI_STATIC_MD(jclass,   java_net_PlainDatagramSocketImpl, ia_clazz);
    DECL_JNI_STATIC_MD(jmethodID,java_net_PlainDatagramSocketImpl, ia_ctor);
    DECL_JNI_STATIC_MD(jfieldID, java_net_PlainSocketImpl, IO_fd_fdID);
    DECL_JNI_STATIC_MD(jfieldID, java_net_PlainSocketImpl, psi_fdID);
    DECL_JNI_STATIC_MD(jfieldID, java_net_PlainSocketImpl, psi_addressID);
    DECL_JNI_STATIC_MD(jfieldID, java_net_PlainSocketImpl, psi_portID);
    DECL_JNI_STATIC_MD(jfieldID, java_net_PlainSocketImpl, psi_localportID);
    DECL_JNI_STATIC_MD(jfieldID, java_net_PlainSocketImpl, psi_timeoutID);
    DECL_JNI_STATIC_MD(int,      java_net_PlainSocketImpl, tcp_level); /* = -1 */
    DECL_JNI_STATIC_MD(jclass,   java_net_PlainSocketImpl, socketExceptionCls);
    DECL_JNI_STATIC_MD(jfieldID, java_net_SocketInputStream, IO_fd_fdID);
    DECL_JNI_STATIC_MD(jfieldID, java_net_SocketInputStream, fis_fdID);
    DECL_JNI_STATIC_MD(jfieldID, java_net_SocketInputStream, sis_implID);
    DECL_JNI_STATIC_MD(jfieldID, java_net_SocketOutputStream, fos_fdID);
    DECL_JNI_STATIC_MD(jfieldID, java_net_SocketOutputStream, IO_fd_fdID);
    DECL_JNI_STATIC_MD(jfieldID, java_io_VxWorksFileSystem, ids_path);
};

typedef struct _CVMJNIStatics_md CVMJNIStatics_md;

#endif /* INCLUDED_JNI_STATICS_MD_H */
