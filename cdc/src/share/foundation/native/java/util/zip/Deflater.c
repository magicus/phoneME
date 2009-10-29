/*
 * @(#)Deflater.c	1.23 06/10/10
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
 * Native method support for java.util.zip.Deflater
 */

#include "javavm/include/porting/ansi/stdio.h"
#include "javavm/include/porting/ansi/stdlib.h"
#include "jlong.h"
#include "jni.h"
#include "jni_util.h"
#include "zlib.h"

#include "java_util_zip_Deflater.h"

#define DEF_MEM_LEVEL 8

#include "jni_statics.h"

/* Work-around for Symbian tool bug.  No longer needed. */
static const JNINativeMethod methods[] = {
    {"init", "(IIZ)J", (void *)&Java_java_util_zip_Deflater_init}
};

JNIEXPORT void JNICALL
Java_java_util_zip_Deflater_initIDs(JNIEnv *env, jclass cls)
{
    JNI_STATIC(java_util_zip_Deflater, strmID) = (*env)->GetFieldID(env, cls, "strm", "J");
    JNI_STATIC(java_util_zip_Deflater, levelID) = (*env)->GetFieldID(env, cls, "level", "I");
    JNI_STATIC(java_util_zip_Deflater, strategyID) = (*env)->GetFieldID(env, cls, "strategy", "I");
    JNI_STATIC(java_util_zip_Deflater, setParamsID) = (*env)->GetFieldID(env, cls, "setParams", "Z");
    JNI_STATIC(java_util_zip_Deflater, finishID) = (*env)->GetFieldID(env, cls, "finish", "Z");
    JNI_STATIC(java_util_zip_Deflater, finishedID) = (*env)->GetFieldID(env, cls, "finished", "Z");
    JNI_STATIC(java_util_zip_Deflater, bufID) = (*env)->GetFieldID(env, cls, "buf", "[B");
    JNI_STATIC(java_util_zip_Deflater, offID) = (*env)->GetFieldID(env, cls, "off", "I");
    JNI_STATIC(java_util_zip_Deflater, lenID) = (*env)->GetFieldID(env, cls, "len", "I");
    /* Work-around for Symbian tool bug.  No longer needed. */
    (*env)->RegisterNatives(env, cls, methods,
			    sizeof methods / sizeof methods[0]);
}

JNIEXPORT jlong JNICALL
Java_java_util_zip_Deflater_init(JNIEnv *env, jclass cls, jint level,
				 jint strategy, jboolean nowrap)
{
    z_stream *strm = calloc(1, sizeof(z_stream));

    if (strm == 0) {
	JNU_ThrowOutOfMemoryError(env, 0);
	return jlong_zero;
    } else {
	char *msg;
	switch (deflateInit2(strm, level, Z_DEFLATED,
			     nowrap ? -MAX_WBITS : MAX_WBITS,
			     DEF_MEM_LEVEL, strategy)) {
	  case Z_OK:
	    return ptr_to_jlong(strm);
	  case Z_MEM_ERROR:
	    free(strm);
	    JNU_ThrowOutOfMemoryError(env, 0);
	    return jlong_zero;
	  case Z_STREAM_ERROR:
	    free(strm);
	    JNU_ThrowIllegalArgumentException(env, 0);
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
Java_java_util_zip_Deflater_setDictionary(JNIEnv *env, jclass cls, jlong strm,
					  jarray b, jint off, jint len)
{
    Bytef *buf = (*env)->GetPrimitiveArrayCritical(env, b, 0);
    int res;
    if (buf == 0) {/* out of memory */
        return;
    }
    res = deflateSetDictionary((z_stream *)jlong_to_ptr(strm), buf + off, len);
    (*env)->ReleasePrimitiveArrayCritical(env, b, buf, 0);
    switch (res) {
    case Z_OK:
	break;
    case Z_STREAM_ERROR:
	JNU_ThrowIllegalArgumentException(env, 0);
	break;
    default:
	JNU_ThrowInternalError(env, ((z_stream *)jlong_to_ptr(strm))->msg);
	break;
    }
}

JNIEXPORT jint JNICALL
Java_java_util_zip_Deflater_deflateBytes(JNIEnv *env, jobject this,
					 jarray b, jint off, jint len)
{
    z_stream *strm = jlong_to_ptr((*env)->GetLongField(env, this, JNI_STATIC(java_util_zip_Deflater, strmID)));

    if (strm == 0) {
	JNU_ThrowNullPointerException(env, 0);
	return 0;
    } else {
	jarray this_buf = (*env)->GetObjectField(env, this, JNI_STATIC(java_util_zip_Deflater, bufID));
	jint this_off = (*env)->GetIntField(env, this, JNI_STATIC(java_util_zip_Deflater, offID));
	jint this_len = (*env)->GetIntField(env, this, JNI_STATIC(java_util_zip_Deflater, lenID));
	Bytef *in_buf;
	Bytef *out_buf;
	int res;
	if ((*env)->GetBooleanField(env, this, JNI_STATIC(java_util_zip_Deflater, setParamsID))) {
	    int level = (*env)->GetIntField(env, this, JNI_STATIC(java_util_zip_Deflater, levelID));
	    int strategy = (*env)->GetIntField(env, this, JNI_STATIC(java_util_zip_Deflater, strategyID));
	    in_buf  = (*env)->GetPrimitiveArrayCritical(env, this_buf, 0);
	    if (in_buf == 0) {
	        return 0;
	    }
	    out_buf = (*env)->GetPrimitiveArrayCritical(env, b, 0);
	    if (out_buf == 0) {
	        (*env)->ReleasePrimitiveArrayCritical(env, this_buf, in_buf, 0);
		return 0;
	    }
	    strm->next_in = in_buf + this_off;
	    strm->next_out = out_buf + off;
	    strm->avail_in = this_len;
	    strm->avail_out = len;
	    res = deflateParams(strm, level, strategy);
	    (*env)->ReleasePrimitiveArrayCritical(env, b, out_buf, 0);
	    (*env)->ReleasePrimitiveArrayCritical(env, this_buf, in_buf, 0);
	    switch (res) {
	    case Z_OK:
		(*env)->SetBooleanField(env, this, JNI_STATIC(java_util_zip_Deflater, setParamsID), JNI_FALSE);
		this_off += this_len - strm->avail_in;
		(*env)->SetIntField(env, this, JNI_STATIC(java_util_zip_Deflater, offID), this_off);
		(*env)->SetIntField(env, this, JNI_STATIC(java_util_zip_Deflater, lenID), strm->avail_in);
		return len - strm->avail_out;
	    case Z_BUF_ERROR:
		(*env)->SetBooleanField(env, this, JNI_STATIC(java_util_zip_Deflater, setParamsID), JNI_FALSE);
		return 0;
	      default:
		JNU_ThrowInternalError(env, strm->msg);
		return 0;
	    }
	} else {
	    jboolean finish = (*env)->GetBooleanField(env, this, JNI_STATIC(java_util_zip_Deflater, finishID));
	    in_buf  = (*env)->GetPrimitiveArrayCritical(env, this_buf, 0);
	    if (in_buf == 0) {
	        return 0;
	    }
	    out_buf = (*env)->GetPrimitiveArrayCritical(env, b, 0);
	    if (out_buf == 0) {
	        (*env)->ReleasePrimitiveArrayCritical(env, this_buf, in_buf, 0);
		return 0;
	    }
	    strm->next_in = in_buf + this_off;
	    strm->next_out = out_buf + off;
	    strm->avail_in = this_len;
	    strm->avail_out = len;
	    res = deflate(strm, finish ? Z_FINISH : Z_NO_FLUSH);
	    (*env)->ReleasePrimitiveArrayCritical(env, b, out_buf, 0);
	    (*env)->ReleasePrimitiveArrayCritical(env, this_buf, in_buf, 0);
	    switch (res) {
	    case Z_STREAM_END:
		(*env)->SetBooleanField(env, this, JNI_STATIC(java_util_zip_Deflater, finishedID), JNI_TRUE);
	    case Z_OK:
		this_off += this_len - strm->avail_in;
		(*env)->SetIntField(env, this, JNI_STATIC(java_util_zip_Deflater, offID), this_off);
		(*env)->SetIntField(env, this, JNI_STATIC(java_util_zip_Deflater, lenID), strm->avail_in);
		return len - strm->avail_out;
	    case Z_BUF_ERROR:
		return 0;
	    default:
		JNU_ThrowInternalError(env, strm->msg);
		return 0;
	    }
	}
    }
}

JNIEXPORT jint JNICALL
Java_java_util_zip_Deflater_getAdler(JNIEnv *env, jclass cls, jlong strm)
{
    return ((z_stream *)jlong_to_ptr(strm))->adler;
}

JNIEXPORT jint JNICALL
Java_java_util_zip_Deflater_getTotalIn(JNIEnv *env, jclass cls, jlong strm)
{
    return ((z_stream *)jlong_to_ptr(strm))->total_in;
}

JNIEXPORT jint JNICALL
Java_java_util_zip_Deflater_getTotalOut(JNIEnv *env, jclass cls, jlong strm)
{
    return ((z_stream *)jlong_to_ptr(strm))->total_out;
}

JNIEXPORT void JNICALL
Java_java_util_zip_Deflater_reset(JNIEnv *env, jclass cls, jlong strm)
{
    if (deflateReset((z_stream *)jlong_to_ptr(strm)) != Z_OK) {
	JNU_ThrowInternalError(env, 0);
    }
}

JNIEXPORT void JNICALL
Java_java_util_zip_Deflater_end(JNIEnv *env, jclass cls, jlong strm)
{
    if (deflateEnd((z_stream *)jlong_to_ptr(strm)) == Z_STREAM_ERROR) {
	JNU_ThrowInternalError(env, 0);
    } else {
	free((z_stream *)jlong_to_ptr(strm));
    }
}
