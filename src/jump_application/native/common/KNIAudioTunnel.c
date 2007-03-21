/*
 * 
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

#include <string.h>
#include <poll.h>
#include "KNICommon.h"
#include "javavm/include/porting/sync.h"
#include "javavm/include/porting/threads.h"

#include "javavm/export/jni.h"
#include "native/common/jni_util.h"

/* Temporary solution */
#include <unistd.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <pthread.h>


#include <jump_messaging.h>
#include <shared_memory.h>
#include <jsr135_jumpdriver_impl.h>

//#define JSR135_KNI_LAYER
#include <jsr135_jumpdriver.h>

int jsr135_create_tunnel(int isolateId);
void jsr135_destroy_tunnel(int isolateId);

static JSR135TunnelDescr tunnelDescr;
static int pcm_channels;
static int pcm_bits;
static int pcm_rate;

static jint isolateId = -1;

static long sampletime(int sample_len) {
    return (sample_len / ((pcm_rate * pcm_channels * (pcm_bits/8)) / 1000));
}

static void my_sleep(CVMExecEnv *ee, long millis) {
    if (CVMlongEqz(millis)) {
        CVMthreadYield();
    } else {
        CVMWaitStatus st;
        CVMSysMonitor mon;
        if (CVMsysMonitorInit(&mon, NULL, 0)) {
            CVMsysMonitorEnter(ee, &mon);
            st = CVMsysMonitorWait(ee, &mon, millis);
            CVMsysMonitorExit(ee, &mon);
            CVMsysMonitorDestroy(&mon);
            if (st == CVM_WAIT_INTERRUPTED) {
                CVMthrowInterruptedException(ee, "operation interrupted");
            }
        } else {
            CVMthrowOutOfMemoryError(ee, "out of monitor resources");
        }
    }
}

/*  protected native int nInit (int isolatedId) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_AudioTunnel_nInit) {
    isolateId = KNI_GetParameterAsInt(1);
    jumpMessageStart();
    jsr135_create_tunnel(isolateId);
    jsr135_get_pcmctl(&pcm_channels, &pcm_bits, &pcm_rate);
    KNI_ReturnInt(0);
}

/*  protected native int nPlayBack () ; */
CNIResultCode
CNIcom_sun_mmedia_AudioTunnel_nPlayBack(CVMExecEnv* ee, CVMStackVal32 *arguments,
			       CVMMethodBlock **p_mb) {
    jint len, tlen;
    jint free_len;
    jint playSize;
    do {
        /* Wait free space in the tunnel */
        CVMsharedMemLock(tunnelDescr.shMem);
        while (*tunnelDescr.playSize >= tunnelDescr.size) {
            CVMsharedMemUnlock(tunnelDescr.shMem);
            CVMD_gcSafeExec(ee, {
                my_sleep(ee, sampletime((*tunnelDescr.playSize)/2));
            })
            CVMsharedMemLock(tunnelDescr.shMem);
        }
        
        /* calculate len of continuous free space */
        playSize = (*tunnelDescr.playSize);
        if ((*tunnelDescr.writePtr)<(*tunnelDescr.readPtr)) {
            free_len = (*tunnelDescr.readPtr) - (*tunnelDescr.writePtr);
        } else if ((*tunnelDescr.writePtr)>(*tunnelDescr.readPtr)) {
            free_len = tunnelDescr.size - (*tunnelDescr.writePtr);
        } else {
            if ((*tunnelDescr.playSize) == 0) {
                free_len = tunnelDescr.size;
                *tunnelDescr.writePtr = 0;
                *tunnelDescr.readPtr = 0;
            } else {
                free_len = 0;
            }
        }
        
        /* get Playback data */
        CVMD_gcSafeExec(ee, {
            LockAudioMutex();
            len = 0;
            do {
            tlen = javacall_media_get_pcmaudio(isolateId, tunnelDescr.memPtr+(*tunnelDescr.writePtr), free_len);
            if (tlen > 0) {
                (*tunnelDescr.writePtr) += tlen;
                (*tunnelDescr.playSize) += tlen;
                if ((*tunnelDescr.writePtr) == tunnelDescr.size) {
                    (*tunnelDescr.writePtr) = 0;
                }
                len += tlen;
                free_len -= tlen;
            }
            } while (tlen > 0 && free_len > 0);
            UnlockAudioMutex();
        })

        CVMsharedMemUnlock(tunnelDescr.shMem);
        CVMD_gcSafeExec(ee, {
            CVMthreadYield();
        })
        if ((len == 0) && (playSize > 0)) {
            CVMD_gcSafeExec(ee, {
                my_sleep(ee, sampletime(playSize/2));
            })
        }
    } while ((len > 0)||(playSize > 0));
    arguments[0].j.i = sampletime((tunnelDescr.size)/2);
    return CNI_SINGLE;
}

/*  protected native int nSTOP (int isolatedId) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_AudioTunnel_nStop) {
    isolateId = KNI_GetParameterAsInt(1);
    jsr135_destroy_tunnel(isolateId);
    jumpMessageShutdown();
    KNI_ReturnInt(0);
}

/*  protected native int nStartMixer (int isolatedId) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_AudioTunnel_nStartMixer) {
    isolateId = KNI_GetParameterAsInt(1);
    jsr135_mixer_start(isolateId);
    KNI_ReturnInt(0);
}
/*  protected native int nStopMixer (int isolatedId) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_AudioTunnel_nStopMixer) {
    isolateId = KNI_GetParameterAsInt(1);
    jsr135_mixer_stop(isolateId);
    KNI_ReturnInt(0);
}

int jsr135_create_tunnel(int isolateId) {

    char name[MAX_SHMEM_NAME_LEN + 1];
    
    /* create shared memory */
    sprintf(name, "MMAPI_AUDIO_%04x", isolateId);
    tunnelDescr.shMem = CVMsharedMemCreate(name, MM_SHMEM_SIZE);
    if (tunnelDescr.shMem == NULL) {
        return -1;
    }
    if (jsr135_open_tunnel(isolateId) == -1) {
        CVMsharedMemDestroy(tunnelDescr.shMem);
        return -1;
    }
    
    tunnelDescr.memPtr = CVMsharedMemGetAddress(tunnelDescr.shMem);
    tunnelDescr.writePtr = (int *)tunnelDescr.memPtr;
    *tunnelDescr.writePtr = 0;
    tunnelDescr.memPtr += sizeof(int);
    tunnelDescr.readPtr = (int *)tunnelDescr.memPtr;
    *tunnelDescr.readPtr = 0;
    tunnelDescr.memPtr += sizeof(int);
    tunnelDescr.playSize = (int *)tunnelDescr.memPtr;
    *tunnelDescr.playSize = 0;
    tunnelDescr.memPtr += sizeof(int);
    tunnelDescr.size = CVMsharedMemGetSize(tunnelDescr.shMem) - sizeof(int)*3;

    return 0;
}

void jsr135_destroy_tunnel(int isolateId) {
    jsr135_close_tunnel(isolateId);
    CVMsharedMemDestroy(tunnelDescr.shMem);
}
