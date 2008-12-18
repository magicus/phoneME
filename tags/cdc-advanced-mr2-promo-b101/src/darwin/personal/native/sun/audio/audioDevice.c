/*
 * @(#)audioDevice.c	1.5 06/10/10
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
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <unistd.h>

#include "sun_audio_AudioDevice.h"

#include "jni.h"

static jclass NullPtrExClass = 0;
jclass cls = 0;

int ntries = 0;
int maxtries = 10;

int sun_audio_audioFd = -1;
int check = 0;


JNIEXPORT void JNICALL
Java_sun_audio_AudioDevice_initIDs(JNIEnv *env, jobject obj)
{
    if (NullPtrExClass == 0) {
        cls = (*env)->FindClass(env, "java/lang/NullPointerException");
        if (cls == 0) {
            /* error */
            fprintf(stderr, "AUDIO ERROR- find class failed.\n");
        }

        NullPtrExClass = (*env)->NewGlobalRef(env, cls);
        if (NullPtrExClass == 0) {
            /* error */
            fprintf(stderr, "AUDIO ERROR - global ref assignment failed.\n");
        }
    }
}

JNIEXPORT jint JNICALL
Java_sun_audio_AudioDevice_audioOpen(JNIEnv *env, jobject this)
{
    extern int errno;

    char *ad = NULL;

    if (sun_audio_audioFd < 0) {
	errno = 0;      
        ad = getenv("AUDIODEV");
        if (ad == NULL){
            sun_audio_audioFd = open("/dev/audio", O_FSYNC | O_WRONLY | O_NONBLOCK);
        } else {
            sun_audio_audioFd = open(ad, O_FSYNC | O_WRONLY | O_NONBLOCK);
        }
        if (sun_audio_audioFd < 0) {
            switch (errno) {
              case ENODEV:
              case EACCES:
                /* Dont bother, there is no audio device */
                fprintf(stderr, "AUDIO ERROR - NO DEVICE / NO ACCESS\n");
                return -1;
            }
            /* Try again later, probably busy */
            return 0;
        } 
    }
    return 1;
}

JNIEXPORT void JNICALL
Java_sun_audio_AudioDevice_audioClose(JNIEnv *env, jobject this)
{
    check = close(sun_audio_audioFd);
    if(check == -1){
        perror("AUDIO ERROR - close failed ");
        check = 0;
    }
    sun_audio_audioFd = -1;
    return;
}

JNIEXPORT void JNICALL
Java_sun_audio_AudioDevice_audioWrite(JNIEnv *env, jobject this, jbyteArray buffArray, jint len)
{

    jint dataLen;
    jbyte *data;

    if (buffArray == 0) {
        (*env)->ThrowNew(env, cls, NULL);
	/*SignalError(0, JAVAPKG "NullPointerException", 0);*/
	return;
    }
    if (len <= 0 || sun_audio_audioFd < 0) {
	return;
    }

    /* 8-bit ulaw encoded buffer */
    data =  (*env)->GetByteArrayElements (env, buffArray, NULL);

    if (data == NULL) {
        return;
    }

    dataLen = (*env)->GetArrayLength (env, buffArray);   
 
    if (dataLen < len) {
	len = dataLen;
    }


    check = write(sun_audio_audioFd, (char *)data, len);
    if(check == -1){
        perror("AUDIO ERROR - write3 failed ");
        check = 0;
    }

    (*env)->ReleaseByteArrayElements (env, buffArray, data, (int)NULL);

    return;
}


