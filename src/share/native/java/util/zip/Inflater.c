/*
 * @(#)Inflater.c	1.29 06/10/10
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
 * Native method support for java.util.zip.Inflater
 */

#include "jlong.h"
#include "jni.h"
#include "jvm.h"
#include "jni_util.h"
#include "zlib.h"

#include "javavm/include/ansi2cvm.h"

#include "java_util_zip_Inflater.h"

#define ThrowDataFormatException(env, msg) \
	JNU_ThrowByName(env, "java/util/zip/DataFormatException", msg)

#include "jni_statics.h"

/* Work-around for Symbian tool bug.  No longer needed. */
static JNINativeMethod methods[] = {
    {"init", "(Z)J", (void *)&Java_java_util_zip_Inflater_init}
};

JNIEXPORT void JNICALL
Java_java_util_zip_Inflater_initIDs(JNIEnv *env, jclass cls)
{
    JNI_STATIC(java_util_zip_Inflater, strmID) = (*env)->GetFieldID(env, cls, "strm", "J");
    JNI_STATIC(java_util_zip_Inflater, needDictID) = (*env)->GetFieldID(env, cls, "needDict", "Z");
    JNI_STATIC(java_util_zip_Inflater, finishedID) = (*env)->GetFieldID(env, cls, "finished", "Z");
    JNI_STATIC(java_util_zip_Inflater, bufID) = (*env)->GetFieldID(env, cls, "buf", "[B");
    JNI_STATIC(java_util_zip_Inflater, offID) = (*env)->GetFieldID(env, cls, "off", "I");
    JNI_STATIC(java_util_zip_Inflater, lenID) = (*env)->GetFieldID(env, cls, "len", "I");
    /* Work-around for Symbian tool bug.  No longer needed. */
    (*env)->RegisterNatives(env, cls, methods,
	sizeof methods / sizeof methods[0]);
}

JNIEXPORT jlong JNICALL
Java_java_util_zip_Inflater_init(JNIEnv *env, jclass cls, jboolean nowrap)
{
    z_stream *strm = (z_stream *)calloc(1, sizeof(z_stream));

    if (strm == 0) {
	JNU_ThrowOutOfMemoryError(env, 0);
	return jlong_zero;
    } else {
	char *msg;
	switch (inflateInit2(strm, nowrap ? -MAX_WBITS : MAX_WBITS)) {
	  case Z_OK:
	    return ptr_to_jlong(strm);
	  case Z_MEM_ERROR:
	    free(strm);
	    JNU_ThrowOutOfMemoryError(env, 0);
	    return jlong_zero;
	  default:
	    msg = strm->msg;
	    free(strm);
	    JNU_ThrowInternalError(env, msg);
	    return jlong_zero;
	}
    }
}

JNIEXPORT void JNICALL
Java_java_util_zip_Inflater_setDictionary(JNIEnv *env, jclass cls, jlong strm,
					  jarray b, jint off, jint len)
{
    Bytef *buf = (Bytef *)(*env)->GetPrimitiveArrayCritical(env, b, 0);
    int res;
    if (buf == 0) /* out of memory */
        return;
    res = inflateSetDictionary((z_stream *)jlong_to_ptr(strm), buf + off, len);
    (*env)->ReleasePrimitiveArrayCritical(env, b, buf, 0);
    switch (res) {
    case Z_OK:
	break;
    case Z_STREAM_ERROR:
    case Z_DATA_ERROR:
	JNU_ThrowIllegalArgumentException(env, ((z_stream *)jlong_to_ptr(strm))->msg);
	break;
    default:
	JNU_ThrowInternalError(env, ((z_stream *)jlong_to_ptr(strm))->msg);
	break;
    }
}

JNIEXPORT jint JNICALL
Java_java_util_zip_Inflater_inflateBytes(JNIEnv *env, jobject thisObj, 
					 jarray b, jint off, jint len)
{
    z_stream *strm = (z_stream *)jlong_to_ptr((*env)->GetLongField(env, thisObj, JNI_STATIC(java_util_zip_Inflater, strmID)));

    if (strm == 0) {
	JNU_ThrowNullPointerException(env, 0);
	return 0;
    } else {
	jarray this_buf = (jarray)(*env)->GetObjectField(env, thisObj, JNI_STATIC(java_util_zip_Inflater, bufID));
	jint this_off = (*env)->GetIntField(env, thisObj, JNI_STATIC(java_util_zip_Inflater, offID));
	jint this_len = (*env)->GetIntField(env, thisObj, JNI_STATIC(java_util_zip_Inflater, lenID));
	Bytef *in_buf;
	Bytef *out_buf;
	int ret;
	in_buf  = (Bytef *)(*env)->GetPrimitiveArrayCritical(env, this_buf, 0);
	if (in_buf == 0) {
	    return 0;
	}
	out_buf = (Bytef *)(*env)->GetPrimitiveArrayCritical(env, b, 0);
	if (out_buf == 0) {
	    (*env)->ReleasePrimitiveArrayCritical(env, this_buf, in_buf, 0);
	    return 0;
	}
	strm->next_in  = in_buf + this_off;
	strm->next_out = out_buf + off;
	strm->avail_in  = this_len;
	strm->avail_out = len;
	ret = inflate(strm, Z_PARTIAL_FLUSH);
	(*env)->ReleasePrimitiveArrayCritical(env, b, out_buf, 0);
	(*env)->ReleasePrimitiveArrayCritical(env, this_buf, in_buf, 0);
	switch (ret) {
	case Z_STREAM_END:
	    (*env)->SetBooleanField(env, thisObj, JNI_STATIC(java_util_zip_Inflater, finishedID), JNI_TRUE);
	case Z_OK:
	    this_off += this_len - strm->avail_in;
	    (*env)->SetIntField(env, thisObj, JNI_STATIC(java_util_zip_Inflater, offID), this_off);
	    (*env)->SetIntField(env, thisObj, JNI_STATIC(java_util_zip_Inflater, lenID), strm->avail_in);
	    return len - strm->avail_out;
	case Z_NEED_DICT:
	    (*env)->SetBooleanField(env, thisObj, JNI_STATIC(java_util_zip_Inflater, needDictID), JNI_TRUE);
	    /* Might have consumed some input here! */
	    this_off += this_len - strm->avail_in;
	    (*env)->SetIntField(env, thisObj, JNI_STATIC(java_util_zip_Inflater, offID), this_off);
	    (*env)->SetIntField(env, thisObj, JNI_STATIC(java_util_zip_Inflater, lenID), strm->avail_in);
	case Z_BUF_ERROR:
	    return 0;
	case Z_DATA_ERROR:
	    ThrowDataFormatException(env, strm->msg);
	    return 0;
	case Z_MEM_ERROR:
	    JNU_ThrowOutOfMemoryError(env, 0);
	    return 0;
	default:
	    JNU_ThrowInternalError(env, strm->msg);
	    return 0;
	}
    }
}

JNIEXPORT jint JNICALL
Java_java_util_zip_Inflater_getAdler(JNIEnv *env, jclass cls, jlong strm)
{
    return ((z_stream *)jlong_to_ptr(strm))->adler;
}

JNIEXPORT jint JNICALL
Java_java_util_zip_Inflater_getTotalIn(JNIEnv *env, jclass cls, jlong strm)
{
    return ((z_stream *)jlong_to_ptr(strm))->total_in;
}

JNIEXPORT jint JNICALL
Java_java_util_zip_Inflater_getTotalOut(JNIEnv *env, jclass cls, jlong strm)
{
    return ((z_stream *)jlong_to_ptr(strm))->total_out;
}

JNIEXPORT void JNICALL
Java_java_util_zip_Inflater_reset(JNIEnv *env, jclass cls, jlong strm)
{
    if (inflateReset((z_stream *)jlong_to_ptr(strm)) != Z_OK) {
	JNU_ThrowInternalError(env, 0);
    }
}

JNIEXPORT void JNICALL
Java_java_util_zip_Inflater_end(JNIEnv *env, jclass cls, jlong strm)
{
    if (inflateEnd((z_stream *)jlong_to_ptr(strm)) == Z_STREAM_ERROR) {
	JNU_ThrowInternalError(env, 0);
    } else {
	free(jlong_to_ptr(strm));
    }
}

