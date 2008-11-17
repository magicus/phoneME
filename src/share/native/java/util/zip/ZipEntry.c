/*
 * @(#)ZipEntry.c	1.18 06/10/10
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

/*
 * Native method support for java.util.zip.ZipEntry
 */

#include "jlong.h"
#include "jvm.h"
#include "jni.h"
#include "jni_util.h"
#include "zip_util.h"

#include "java_util_zip_ZipEntry.h"

#define DEFLATED 8
#define STORED 0

#include "jni_statics.h"

JNIEXPORT void JNICALL
Java_java_util_zip_ZipEntry_initIDs(JNIEnv *env, jclass cls)
{
    JNI_STATIC(java_util_zip_ZipEntry, nameID) = (*env)->GetFieldID(env, cls, "name", "Ljava/lang/String;");
    JNI_STATIC(java_util_zip_ZipEntry, timeID) = (*env)->GetFieldID(env, cls, "time", "J");
    JNI_STATIC(java_util_zip_ZipEntry, crcID) = (*env)->GetFieldID(env, cls, "crc", "J");
    JNI_STATIC(java_util_zip_ZipEntry, sizeID) = (*env)->GetFieldID(env, cls, "size", "J");
    JNI_STATIC(java_util_zip_ZipEntry, csizeID) = (*env)->GetFieldID(env, cls, "csize", "J");
    JNI_STATIC(java_util_zip_ZipEntry, methodID) = (*env)->GetFieldID(env, cls, "method", "I");
    JNI_STATIC(java_util_zip_ZipEntry, extraID) = (*env)->GetFieldID(env, cls, "extra", "[B");
    JNI_STATIC(java_util_zip_ZipEntry, commentID) = (*env)->GetFieldID(env, cls, "comment", "Ljava/lang/String;");
}

JNIEXPORT void JNICALL
Java_java_util_zip_ZipEntry_initFields(JNIEnv *env, jobject obj, jlong zentry)
{
    jzentry *ze = (jzentry *)jlong_to_ptr(zentry);
    jstring name = (*env)->GetObjectField(env, obj, JNI_STATIC(java_util_zip_ZipEntry, nameID));
    jlong long_ffffffff = CVMlongUshr(CVMint2Long(-1), 32);

    if (name == 0) {
        name = (*env)->NewStringUTF(env, ze->name);
	if (name == 0) {
	    return;
	}
	(*env)->SetObjectField(env, obj, JNI_STATIC(java_util_zip_ZipEntry, nameID), name);
    }
    (*env)->SetLongField(env, obj, JNI_STATIC(java_util_zip_ZipEntry, timeID),
			 CVMlongAnd(jint_to_jlong(ze->time), long_ffffffff));
    (*env)->SetLongField(env, obj, JNI_STATIC(java_util_zip_ZipEntry, crcID),
			 CVMlongAnd(jint_to_jlong(ze->crc), long_ffffffff));
    (*env)->SetLongField(env, obj, JNI_STATIC(java_util_zip_ZipEntry, sizeID),
			CVMint2Long(ze->size));
    if (ze->csize == 0) {
	(*env)->SetLongField(env, obj,
			     JNI_STATIC(java_util_zip_ZipEntry, csizeID),
			     CVMint2Long(ze->size));
	(*env)->SetIntField(env, obj,
			    JNI_STATIC(java_util_zip_ZipEntry, methodID),
			    STORED);
    } else {
	(*env)->SetLongField(env, obj,
			     JNI_STATIC(java_util_zip_ZipEntry, csizeID),
			     CVMint2Long(ze->csize));
	(*env)->SetIntField(env, obj,
			    JNI_STATIC(java_util_zip_ZipEntry, methodID),
			    DEFLATED);
    }
    if (ze->extra != 0) {
	unsigned char *bp = (unsigned char *)&ze->extra[0];
	jsize len = (bp[0] | (bp[1] << 8));
	jbyteArray extra = (*env)->NewByteArray(env, len);
	if (extra == 0) {
	    return;
	}
	(*env)->SetByteArrayRegion(env, extra, 0, len, &ze->extra[2]);
	(*env)->SetObjectField(env, obj, JNI_STATIC(java_util_zip_ZipEntry, extraID), extra);
    }
    if (ze->comment != 0) {
	jstring comment = (*env)->NewStringUTF(env, ze->comment);
	if (comment == 0) {
	    return;
	}
	(*env)->SetObjectField(env, obj, JNI_STATIC(java_util_zip_ZipEntry, commentID), comment);
    }
}
