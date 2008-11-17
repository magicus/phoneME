/*
 * @(#)jni_statics.h	1.8 06/10/10
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

#ifndef INCLUDED_JNI_STATICS_H
#define INCLUDED_JNI_STATICS_H

#include "jni_statics_md.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Use this macro to access a variable in this structure */
#define JNI_STATIC(CLASS, VARIABLE) \
    (CVMjniGlobalStatics.CLASS ## _ ## VARIABLE)

/* This macro is used internally to define the structure */
#define DECL_JNI_STATIC(TYPE, CLASS, VARIABLE) \
    TYPE CLASS ## _ ## VARIABLE

struct _CVMJNIStatics {
    /* These declarations are kept in separate namespaces according to 
     * the class name where they are used - this would expand to 
     * something like:
     * jfieldID    java_lang_ClassLoader_handleID;
     */
    DECL_JNI_STATIC(jfieldID, java_lang_ClassLoader, isXrunLibraryID);
    DECL_JNI_STATIC(jfieldID, java_lang_ClassLoader, handleID);
    DECL_JNI_STATIC(jfieldID, java_lang_ClassLoader, jniVersionID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_Deflater, strmID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_Deflater, levelID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_Deflater, strategyID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_Deflater, setParamsID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_Deflater, finishID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_Deflater, finishedID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_Deflater, bufID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_Deflater, offID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_Deflater, lenID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_Inflater, strmID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_Inflater, needDictID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_Inflater, finishedID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_Inflater, bufID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_Inflater, offID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_Inflater, lenID);
    DECL_JNI_STATIC(jfieldID, java_io_ObjectStreamClass, osf_field_id);
    DECL_JNI_STATIC(jfieldID, java_io_ObjectStreamClass, osf_typecode_id);
    DECL_JNI_STATIC(jfieldID, java_util_zip_ZipEntry, nameID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_ZipEntry, timeID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_ZipEntry, crcID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_ZipEntry, sizeID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_ZipEntry, csizeID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_ZipEntry, methodID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_ZipEntry, extraID);
    DECL_JNI_STATIC(jfieldID, java_util_zip_ZipEntry, commentID);
    DECL_JNI_STATIC(jfieldID, java_net_DatagramPacket, dp_addressID);
    DECL_JNI_STATIC(jfieldID, java_net_DatagramPacket, dp_portID);
    DECL_JNI_STATIC(jfieldID, java_net_DatagramPacket, dp_bufID);
    DECL_JNI_STATIC(jfieldID, java_net_DatagramPacket, dp_offsetID);
    DECL_JNI_STATIC(jfieldID, java_net_DatagramPacket, dp_lengthID);
    DECL_JNI_STATIC(jfieldID, java_net_DatagramPacket, dp_bufLengthID);
    DECL_JNI_STATIC(jfieldID, java_net_InetAddress, ia_addressID);
    DECL_JNI_STATIC(jfieldID, java_net_InetAddress, ia_familyID);
    DECL_JNI_STATIC(jfieldID, java_net_InetAddress, ia_preferIPv6AddressID);
    DECL_JNI_STATIC(jfieldID, java_io_FileDescriptor, IO_fd_fdID);
    DECL_JNI_STATIC(jfieldID, java_io_FileInputStream, fis_fd);
    DECL_JNI_STATIC(jfieldID, java_io_FileOutputStream, fos_fd);
    DECL_JNI_STATIC(jfieldID, java_io_RandomAccessFile, raf_fd);
    DECL_JNI_STATIC(jfieldID, java_lang_SecurityManager, initField);
    DECL_JNI_STATIC(jfieldID, java_util_zip_ZipFile, jzfileID);

    /* These follow a slightly different naming convention: because
     * they are from utility files (i.e. belong to no single Java
     * class), the filename is used as a namespace indicator instead
     * of the Java class name.  */

    DECL_JNI_STATIC(int,      jni_util, fastEncoding); /* = NO_ENCODING_YET */
    DECL_JNI_STATIC(jmethodID,jni_util, String_init_ID);
    DECL_JNI_STATIC(jmethodID,jni_util, String_getBytes_ID);
    DECL_JNI_STATIC(jmethodID,jni_util, Object_waitMID);
    DECL_JNI_STATIC(jmethodID,jni_util, Object_notifyMID);
    DECL_JNI_STATIC(jmethodID,jni_util, Object_notifyAllMID);
    DECL_JNI_STATIC(jmethodID,jni_util, mid);
    DECL_JNI_STATIC(void*    ,io_util, deletionList); /* should be struct dlEntry*, see io_util.c */
    DECL_JNI_STATIC(void*    ,zip_util, zfiles); /* should be jzfile*, see zip_util.c */
    DECL_JNI_STATIC(void*    ,zip_util, zfiles_lock);
    DECL_JNI_STATIC(jboolean ,zip_util, inited);
    DECL_JNI_STATIC(jclass   ,net_util, cls);
    DECL_JNI_STATIC(jclass   ,java_net_InetAddress, ia_class);
    DECL_JNI_STATIC(jclass   ,java_net_Inet4Address, ia4_class);
    DECL_JNI_STATIC(jclass   ,java_net_Inet6Address, ia6_class);
    DECL_JNI_STATIC(jmethodID,java_net_Inet4Address, ia4_ctrID);
    DECL_JNI_STATIC(jmethodID,java_net_Inet6Address, ia6_ctrID);
    DECL_JNI_STATIC(jfieldID ,java_net_Inet6Address, ia6_ipaddressID);
    /* The platform specific definitions of statics */
    CVMJNIStatics_md  platformStatics;
};

typedef struct _CVMJNIStatics CVMJNIStatics;

extern CVMJNIStatics CVMjniGlobalStatics;

#ifdef __cplusplus
}
#endif

#endif
