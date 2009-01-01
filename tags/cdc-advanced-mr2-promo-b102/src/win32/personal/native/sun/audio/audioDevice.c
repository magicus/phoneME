/*
 * @(#)audioDevice.c	1.6 06/10/10
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

/* FIXME: Need to port to Win32 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
//#include <sys/types.h>
//#include <sys/errno.h>

#if _MSC_VER < 1300
// inclusion of <unistd.h> breaks win32 build with MSVC8;
// although this header is most probably not needed for all win32 builds, we better
// use #if _MSC_VER and don't include for MSVC 8 and newer MS compilers, only
// To be removed for all win32 builds if not needed after testing.
 
#include <unistd.h>

#endif
//#include <stropts.h>

//#include "sun_audio_AudioDevice.h"

#include "jni.h"

//#include <linux/soundcard.h>

static jclass NullPtrExClass = 0;
jclass cls = 0;

int ntries = 0;
int maxtries = 10;

int sun_audio_audioFd = -1;
int audioDevice_check = 0;

#define AUBUFFERSIZE 8192

void ulawBufToLinearBuf(unsigned char *ulawBuf, short *linearBuf, int count);
short ulaw2linear(unsigned char ulawbyte);

/* Convert mono 8-bit ulaw encoded buffer to stereo16-bit linear encoded buffer */
void ulawBufToLinearBuf(unsigned char *ulawBuf, short *linearBuf, int count)
{    
    static int exp_lut[8] = {0,132,396,924,1980,4092,8316,16764};
    int i, sign, exponent, mantissa;

    for(i = 0; i < count; i++){
        ulawBuf[i] = ~ulawBuf[i];
        sign = (ulawBuf[i] & 0x80);
        exponent = (ulawBuf[i] >> 4) & 0x07;
        mantissa = ulawBuf[i] & 0x0F;
        /* Fill buffer with each byte twice - STEREO */
        linearBuf[2*i] = exp_lut[exponent] + (mantissa << (exponent + 3));
        linearBuf[2*i + 1] = exp_lut[exponent] + (mantissa << (exponent + 3));
        if (sign != 0) linearBuf[2*i] = -linearBuf[2*i];
        if (sign != 0) linearBuf[2*i + 1] = -linearBuf[2*i + 1];
    }
}

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
    int flags = 0;

    //  if (sun_audio_audioFd < 0) {
//          errno = 0;
//          /* Open /dev/dsp as sound sample will be converted to linear encoding */
//          if ((sun_audio_audioFd = open("/dev/dsp", O_SYNC | O_WRONLY | O_NONBLOCK)) < 0) {
//              switch (errno) {
//                case ENODEV:
//                case EACCES:
//                  /* Dont bother, there is no audio device */
//                  fprintf(stderr, "AUDIO ERROR - NO DEVICE / NO ACCESS\n");
//                  return -1;
//              }
//              /* Try again later, probably busy */
//              return 0;
//          }

//          /* Ensure device is set to 16-bit linear encoding format. */
//          flags = AFMT_S16_LE;
//          audioDevice_check = ioctl(sun_audio_audioFd, SNDCTL_DSP_SETFMT, &flags);
//          if (flags != AFMT_S16_LE){
//              fprintf(stderr, "Audio - could not set device to 16-bit linear encoding.\n");
//          }
//          if (audioDevice_check == -1){
//              perror("AUDIO ERROR - ioctl set format failed ");
//              audioDevice_check = 0;
//          }

//          /* Ensure device is set to stereo. */
//          flags = 1;
//          audioDevice_check = ioctl(sun_audio_audioFd, SNDCTL_DSP_STEREO, &flags);
//          if (flags != 1){
//              fprintf(stderr, "Audio - could not set device to stereo.\n");
//          }        
//          if (audioDevice_check == -1){
//              perror("AUDIO ERROR - ioctl set stereo failed ");
//              audioDevice_check = 0;
//          }

//          /* Ensure device is set to 8KHz. */     
//          flags = 0x00002000;
//          audioDevice_check = ioctl(sun_audio_audioFd, SNDCTL_DSP_SPEED, &flags);
//          if (flags != 0x00002000){
//              fprintf(stderr, "Audio - could not set device to 8Khz.\n");
//          }
//          if (audioDevice_check == -1){
//              perror("AUDIO ERROR - ioctl set speed failed ");
//              audioDevice_check = 0;
//          }

//          /* No longer need to reset device to be blocking - audio playback
//           * is non-blocking.*/
//  /*        audioDevice_check = fcntl(sun_audio_audioFd, F_SETFL,  O_SYNC | O_WRONLY);
//          if(audioDevice_check == -1){
//              perror("AUDIO ERROR - fcntl failed : ");
//              audioDevice_check = 0;
//          }
//  */    }
    return 1;
}

JNIEXPORT void JNICALL
Java_sun_audio_AudioDevice_audioClose(JNIEnv *env, jobject this)
{
//  audioDevice_check = close(sun_audio_audioFd);
//      if(audioDevice_check == -1){
//          perror("AUDIO ERROR - close failed ");
//          audioDevice_check = 0;
//      }
//      sun_audio_audioFd = -1;
    return;
}

JNIEXPORT void JNICALL
Java_sun_audio_AudioDevice_audioWrite(JNIEnv *env, jobject this, jbyteArray buffArray, jint len)
{

    /* 16-bit linear buffer */
    static short linBuf[AUBUFFERSIZE];

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

    /* For Linux - assuming device to play 16-bit linear encoded samples. */   
    /* Need to add other conversion functions for other devices and encoded  formats. */
    /* Convert from mono 8-bit ulaw to stereo 16-bit linear and write */
    while (len >= AUBUFFERSIZE) {
        ulawBufToLinearBuf(data, linBuf, AUBUFFERSIZE);
        
//          audioDevice_check = write(sun_audio_audioFd, (char *)linBuf, 4*AUBUFFERSIZE); 
        /* Make 10 attempts to write sound data to device if error encountered. */        
        if(++ntries <= maxtries){                        
            if(audioDevice_check == -1){
                /* perror("AUDIO ERROR - write1 failed ");*/

                /* Audio device is most likely unavailable as it is busy. */
                /* Sleep and try device again with same block of data. */

//                  sleep(1);

                /* Check that device is still open before attempting to write. */
                if(!(sun_audio_audioFd < 0)){
//                      audioDevice_check = write(sun_audio_audioFd, (char *)linBuf, 4*AUBUFFERSIZE);
                    if (audioDevice_check == -1) {
                        /* perror("AUDIO ERROR - write1 failed on further attempt "); */
                    }
                }
            }
        }
        ntries = 0;
        audioDevice_check = 0;       
        len -= AUBUFFERSIZE;
        data += AUBUFFERSIZE;
    } 
    if (len > 0) {
        ulawBufToLinearBuf(data, linBuf, len);
//          audioDevice_check = write(sun_audio_audioFd, (char *)linBuf, 4*len); 
        /* Make 10 attempts to write sound data to device if error encountered. */        
        if(++ntries <= maxtries){                        
            if(audioDevice_check == -1){
                /* perror("AUDIO ERROR - write2 failed ");*/

                /* Audio device is most likely unavailable as it is busy. */
                /* Sleep and try device again with same block of data. */

//                  sleep(1);

                /* Check that device is still open before attempting to write. */
                if(!(sun_audio_audioFd < 0)){
//                      audioDevice_check = write(sun_audio_audioFd, (char *)linBuf, 4*len);
                    if (audioDevice_check == -1) {
                        /* perror("AUDIO ERROR - write2 failed on further attempt "); */
                    }
                }
            }
        }
        ntries = 0;
        audioDevice_check = 0;          
     }

    (*env)->ReleaseByteArrayElements (env, buffArray, data, (int)NULL);

    return;
}


