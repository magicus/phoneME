/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#include <stdio.h> // NULL, fprintf()
#include <string.h> // strcmp()
#include <unistd.h> // strcmp()
//#include <JUMPMessages.h>
#include <shared_memory.h>
#include <javacall_multimedia.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include "jsr135_jumpdriver_impl.h"
#include "javavm/include/porting/sync.h"
#include "javavm/include/porting/threads.h"

#include <time.h>

static JSR135TunnelDescr tunnelDescr[MAX_SUPPORTED_ISOLATES];
int numTunnels = 0;

CVMThreadID MixerThreadId;
CVMMutex    MixerMutex;
CVMMutex    PlaybackMutex;
int         MixerRunning = 0;
int         MixerActive = 0;
int         MixerMutexInit = 0;

#define     LOCK_MIXER_MUTEX                    \
                if(0==MixerMutexInit) {         \
                    CVMmutexInit(&MixerMutex);  \
                    MixerMutexInit = 1;         \
                }                               \
                CVMmutexLock(&MixerMutex);

#define     UNLOCK_MIXER_MUTEX                  \
                CVMmutexUnlock(&MixerMutex);
            

#define MM_PCMOUT_STACK_SIZE     20*1024
#define MM_PCMOUT_PRIORITY 5 /* Normal priority */
static void pcmaudio_mixer(void *);

int jsr135_open_tunnel(int isolateId) {

    char name[MAX_SHMEM_NAME_LEN + 1];
    CVMSharedMemory shm;
    int idx = numTunnels;
    int i;
    int ret = -1;
    
    LOCK_MIXER_MUTEX
    
    if (numTunnels < MAX_SUPPORTED_ISOLATES) {
        /* Check if already attached */
        for (i=0; i<numTunnels; i++) {
            if (tunnelDescr[i].isolateId == isolateId) {
                CVMsharedMemClose(tunnelDescr[i].shMem);
                idx = i;
                break;
            }
        }
        /* open shared memory */
        sprintf(name, "MMAPI_AUDIO_%04x", isolateId);
        shm = CVMsharedMemOpen(name);
        if (shm != NULL) {
            /* add to descriptor */
            tunnelDescr[idx].isolateId = isolateId;
            tunnelDescr[idx].shMem = shm;
            tunnelDescr[idx].active = 0;
            tunnelDescr[idx].memPtr = CVMsharedMemGetAddress(shm);
            tunnelDescr[idx].writePtr = (int *)tunnelDescr[idx].memPtr;
            tunnelDescr[idx].memPtr += sizeof(int);
            tunnelDescr[idx].readPtr = (int *)tunnelDescr[idx].memPtr;
            tunnelDescr[idx].memPtr += sizeof(int);
            tunnelDescr[idx].playSize = (int *)tunnelDescr[idx].memPtr;
            tunnelDescr[idx].memPtr += sizeof(int);

            tunnelDescr[idx].size = CVMsharedMemGetSize(shm) - sizeof(int)*3;

            if (idx == numTunnels) {
                numTunnels++;
            }
            ret = 0;
        }
    }
    
    UNLOCK_MIXER_MUTEX
    return ret;
}

extern int close_driver;

int jsr135_close_tunnel(int isolateId) {

    int idx = MAX_SUPPORTED_ISOLATES;
    int i;
    int ret = -1;

    LOCK_MIXER_MUTEX
    
    if (numTunnels > 0) {
        /* Check if already attached */
        for (i=0; i<numTunnels; i++) {
            if (tunnelDescr[i].isolateId == isolateId) {
                CVMsharedMemClose(tunnelDescr[i].shMem);
                idx = i;
                break;
            }
        }
        if (idx < numTunnels) {
            for (i=idx; i<(numTunnels-1); i++) {
                tunnelDescr[i] = tunnelDescr[i+1];
            }
            numTunnels--;
            ret = 0;
        }
    }
    
    UNLOCK_MIXER_MUTEX

    close_driver = 1;
    return ret;
}

static int pcm_channels;
static int pcm_bits;
static int pcm_rate;
static int pcm_bytes;
int jsr135_get_pcmctl(int *channels, int* bits, int* rate) {

    int ret = -1;
    LOCK_MIXER_MUTEX

    if (javacall_media_get_pcmctl(channels, bits, rate) == JAVACALL_OK) {
        pcm_channels = *channels;
        pcm_bits = *bits;
        pcm_rate = *rate;
        pcm_bytes = (pcm_bits)/8;
        ret = 0;
    }
    
    UNLOCK_MIXER_MUTEX
    
    return ret;
}


int jsr135_mixer_start(int isolateId) {
    int i;
    int ret = -1;

    LOCK_MIXER_MUTEX
    for (i=0; i<numTunnels; i++) {
        if (tunnelDescr[i].isolateId == isolateId) {
            if (0 == MixerRunning) {
                CVMmutexInit(&PlaybackMutex);
                if (javacall_media_acquire_pcmaudio_device() == JAVACALL_OK) {
                    if (CVMthreadCreate(&MixerThreadId,
                        MM_PCMOUT_STACK_SIZE, MM_PCMOUT_PRIORITY,
                        pcmaudio_mixer, NULL)) {
                            tunnelDescr[i].active = 1;
                            MixerRunning = 1;
                            MixerActive = 1;
                            ret = 0;
                    } else {
                        javacall_media_release_pcmaudio_device();
                    }
                }
            } else {
                tunnelDescr[i].active = 1;
                if (MixerActive == 0) {
                    if (javacall_media_acquire_pcmaudio_device() == JAVACALL_OK) {
                        MixerActive = 1;
                    }
                }
                CVMmutexUnlock(&PlaybackMutex);
                ret = 0;
            }
            break;
        }
    }
    
    UNLOCK_MIXER_MUTEX
    
    return ret;
}

int jsr135_mixer_stop(int isolateId) {
    int i;
    int ret = -1;
    int numActive = 0;
    
    LOCK_MIXER_MUTEX
    
    if (1 == MixerRunning)  {
        for (i=0; i<numTunnels; i++) {
            if (tunnelDescr[i].isolateId == isolateId) {
                tunnelDescr[i].active = 0;
                ret = 0;
            }
            if (tunnelDescr[i].active) {
                numActive++;
            }
        }
        if (numActive == 0) {
            CVMmutexLock(&PlaybackMutex);
            javacall_media_release_pcmaudio_device();
            MixerActive = 0;
        }
    }
    
    UNLOCK_MIXER_MUTEX
    
    return ret;
}


int loop = 1;

static char buffer[MM_SHMEM_SIZE];
static short *bfr16 = (short *)buffer;
static char *bfr8 = (char *)buffer;

static void pcmaudio_mixer(void *args) {
    int numActivePlayers = 0;
    int i;
    int realLen;
    int firstPlayer;

    (void)args;
        while(loop) {
            CVMmutexLock(&PlaybackMutex);

            LOCK_MIXER_MUTEX

            if (numTunnels > 0) {
                numActivePlayers = 0;
                realLen = MM_SHMEM_SIZE;
                for (i=0; i<numTunnels; i++) {
                    CVMsharedMemLock(tunnelDescr[i].shMem);
                    if ((*tunnelDescr[i].playSize)>0) {
                        if (realLen>(*tunnelDescr[i].playSize)) {
                            realLen = (*tunnelDescr[i].playSize);
                        }
                        numActivePlayers++;
                    }
                }
                if (numActivePlayers > 0) {
                    firstPlayer = 1;
                    realLen = realLen/pcm_bytes;
                    for (i=0; i<numTunnels; i++) {
                        if ((*tunnelDescr[i].playSize)>0) {
                            int len = 0;
                            while(len < realLen) {
                                switch(pcm_bytes) {
                                    case 1:
                                        if (firstPlayer) {
                                            bfr8[len++] = tunnelDescr[i].memPtr[(*tunnelDescr[i].readPtr)]/numActivePlayers;
                                        } else {
                                            bfr8[len++] += tunnelDescr[i].memPtr[(*tunnelDescr[i].readPtr)]/numActivePlayers;
                                        }
                                        break;
                                    case 2:
                                        if (firstPlayer) {
                                            bfr16[len++] = (*(short *)(&tunnelDescr[i].memPtr[(*tunnelDescr[i].readPtr)]))/numActivePlayers;
                                        } else {
                                            bfr16[len++] += (*(short *)(&tunnelDescr[i].memPtr[(*tunnelDescr[i].readPtr)]))/numActivePlayers;
                                        }
                                        break;                                            
                                }
                                (*tunnelDescr[i].readPtr) +=pcm_bytes;
                                if ((*tunnelDescr[i].readPtr) >= tunnelDescr[i].size) {
                                    (*tunnelDescr[i].readPtr) = 0;
                                }
                                (*tunnelDescr[i].playSize)-=pcm_bytes;
                            }
                            firstPlayer = 0;
                        }
                        CVMsharedMemUnlock(tunnelDescr[i].shMem);
                    }
                } else {
                    realLen = 0;
                    for (i=0; i<numTunnels; i++) {
                        CVMsharedMemUnlock(tunnelDescr[i].shMem);
                    }
                }
                if (realLen > 0) {
                    long wait_ms = javacall_media_pcmaudio_playback(buffer, realLen*pcm_bytes);
                    if (wait_ms > 0) {
                        usleep(wait_ms/2);
                    }
                } else {
                    CVMthreadYield();
                }
            } 
            UNLOCK_MIXER_MUTEX
            
            CVMmutexUnlock(&PlaybackMutex);
            CVMthreadYield();
        }
    MixerRunning = 0;
}
