/*
 * @(#)jcov_java.c	1.8 06/10/10
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

#include "javavm/include/porting/ansi/string.h"
#include "javavm/include/porting/ansi/stdio.h"
#include "javavm/include/porting/ansi/stdlib.h"

#include "jni.h"
#include "jcov_error.h"
#include "jcov_types.h"

int jcov_java_init_done = 0;
jmethodID mid_get_stream;
jmethodID mid_available;
jmethodID mid_read;

#define READ_CHUNK_SIZE 1024
#define NAME_BUF_SIZE 255
#define RET_IF_(cond) if (cond) return FALSE

Bool get_class_binary_data(JNIEnv *env, const char *class_name, UINT8 **buf, jint *buf_len) {
    jstring name;
    jobject istream;
    jbyteArray arr;
    jint len;
    jint res = 0;
    jint n = 0;
    char name_buf[NAME_BUF_SIZE];
    jclass cls_class_loader;
    jclass cls_input_stream;
    char *suff = ".class";

    cls_class_loader = (*env)->FindClass(env, "java/lang/ClassLoader");
    RET_IF_(cls_class_loader == NULL);
    cls_input_stream  = (*env)->FindClass(env, "java/io/InputStream");
    RET_IF_(cls_input_stream == NULL);

    if (strlen(class_name) + strlen(suff) >= NAME_BUF_SIZE) {
        printf("*** Jcov errror: class name too long: %s (skipped)\n", class_name);
        return FALSE;
    }
    sprintf(name_buf, "%s%s", class_name, suff);
    name = (*env)->NewStringUTF(env, name_buf);
    RET_IF_(name == NULL);
    istream = (*env)->CallStaticObjectMethod(env, cls_class_loader, mid_get_stream, name);
    RET_IF_(istream == NULL);

    len = (*env)->CallIntMethod(env, istream, mid_available);
    arr = (*env)->NewByteArray(env, (jsize)len);

    do {
        jint i;
        res += n;
        i = len - res; 
        if (i > READ_CHUNK_SIZE)
            i = READ_CHUNK_SIZE;
        n = (*env)->CallIntMethod(env, istream, mid_read, arr, res, i);
    } while (n > 0);
    RET_IF_(res != len);
    *buf = (UINT8*)malloc(len);
    RET_IF_(*buf == NULL);
    (*env)->GetByteArrayRegion(env, arr, 0, len, (jbyte*)(*buf));
    *buf_len = len;

    return TRUE;
}

Bool jcov_java_init(JNIEnv *env) {
    jclass cls_class_loader;
    jclass cls_input_stream;
    char *sig = "(Ljava/lang/String;)Ljava/io/InputStream;";

    cls_class_loader = (*env)->FindClass(env, "java/lang/ClassLoader");
    RET_IF_(cls_class_loader == NULL);
    mid_get_stream = (*env)->GetStaticMethodID(env, cls_class_loader, "getSystemResourceAsStream", sig);
    RET_IF_(mid_get_stream == NULL);
    cls_input_stream  = (*env)->FindClass(env, "java/io/InputStream");
    RET_IF_(cls_input_stream == NULL);
    mid_available = (*env)->GetMethodID(env, cls_input_stream, "available", "()I");
    RET_IF_(mid_available == NULL);
    mid_read = (*env)->GetMethodID(env, cls_input_stream, "read", "([BII)I");
    RET_IF_(mid_read == NULL);

    jcov_java_init_done = 1;
    return TRUE;
}

#undef RET_IF_
