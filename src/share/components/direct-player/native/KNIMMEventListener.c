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
#include "KNICommon.h"
#include "javavm/include/porting/sync.h"
#include "javacall_multimedia.h"

typedef struct {
    int type;
    int playerId;
    long value;
}NativeEvent;

#define MM_MAX_EVENTS  10
NativeEvent qEvt[MM_MAX_EVENTS];
int qhead = 0;
int qtail = 0;

#define EVENT_TYPE_NAME "eventId"
#define PLAYER_ID_NAME  "playerId"
#define VALUE_NAME      "value"

#define EOM_EVENT_NAME "EOM_EVENT"
#define RSL_EVENT_NAME "RSL_EVENT"

CVMMutex        nEventMutex;
CVMCondVar      nCondVar;

/*  protected native int nStart (int isolatedId) ; */
CNIResultCode
CNIcom_sun_mmedia_MMEventListener_nativeEventInit(CVMExecEnv* ee, CVMStackVal32 *arguments,
			       CVMMethodBlock **p_mb) {
    jboolean ret;
    (void)p_mb;
CVMD_gcUnsafeExec(ee, {
    ret = CVMmutexInit(&nEventMutex);
    ret = CVMcondvarInit(&nCondVar,&nEventMutex);
})
    arguments[0].j.i = 0;
    return CNI_SINGLE;
}

/*  protected native int nStart (int isolatedId) ; */
CNIResultCode
CNIcom_sun_mmedia_MMEventListener_nativeEventDestroy(CVMExecEnv* ee, CVMStackVal32 *arguments,
			       CVMMethodBlock **p_mb) {
    (void)p_mb;
CVMD_gcUnsafeExec(ee, {
    CVMcondvarDestroy(&nCondVar);
    CVMmutexDestroy(&nEventMutex);
})
    arguments[0].j.i = 0;
    return CNI_SINGLE;
}

/*  protected native boolean waitEventFromNative () ; */
CNIResultCode
CNIcom_sun_mmedia_MMEventListener_waitEventFromNative(CVMExecEnv* ee, CVMStackVal32 *arguments,
			       CVMMethodBlock **p_mb) {
    jboolean ret = CVM_FALSE;
    (void)p_mb;
CVMD_gcUnsafeExec(ee, {
    CVMmutexLock(&nEventMutex);
})
//CVMD_gcUnsafeExec(ee, {
    CVMthreadYield();
    while(qhead == qtail) {
        ret = CVMcondvarWait(&nCondVar, &nEventMutex, 0);
    }        
//})
    if(qhead != qtail) {
        ret = CVM_TRUE;
    }
CVMD_gcUnsafeExec(ee, {
    CVMmutexUnlock(&nEventMutex);
    CVMthreadYield();
})        
    arguments[0].j.i = ret;
    return CNI_SINGLE;
}

/*  protected native boolean getNativeEvent (com/sun/mmedia/MMNativeEventImpl) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_MMEventListener_getNativeEvent) {
    jboolean ret = KNI_FALSE;
    CVMmutexLock(&nEventMutex);
        if (qhead != qtail) {
            jfieldID fieldID;
            int type = -1;
            KNI_StartHandles(2);
            KNI_DeclareHandle(classHandle);
            KNI_DeclareHandle(event);
            
            KNI_GetParameterAsObject(1, event);
            KNI_GetObjectClass(event, classHandle);
            
            switch(qEvt[qhead].type) {
                case JAVACALL_EVENT_MEDIA_END_OF_MEDIA:
                    fieldID = KNI_GetStaticFieldID(classHandle, EOM_EVENT_NAME, "I");
                    type = KNI_GetStaticIntField(classHandle, fieldID);
                    break;
                case JAVACALL_EVENT_MEDIA_RECORD_SIZE_LIMIT:
                    fieldID = KNI_GetStaticFieldID(classHandle, RSL_EVENT_NAME, "I");
                    type = KNI_GetStaticIntField(classHandle, fieldID);
                    break;
                default:
                    break;
            }
            if (type != -1) {
                fieldID = KNI_GetFieldID(classHandle, EVENT_TYPE_NAME, "I");
                KNI_SetIntField(event, fieldID, type);

                fieldID = KNI_GetFieldID(classHandle, PLAYER_ID_NAME, "I");
                KNI_SetIntField(event, fieldID, qEvt[qhead].playerId);

                fieldID = KNI_GetFieldID(classHandle, VALUE_NAME, "J");
                KNI_SetLongField(event, fieldID, qEvt[qhead].value);

                /* move head pointer */
                qhead++;
                if (qhead == MM_MAX_EVENTS) {
                    qhead = 0;
                }
                if (qhead == qtail) {
                    qhead = qtail = 0;
                }
                ret = KNI_TRUE;
            }
            KNI_EndHandles();
        }
    CVMmutexUnlock(&nEventMutex);
    KNI_ReturnBoolean(ret);
}


/**
 * Post native media event to Java event handler
 * 
 * @param type          Event type
 * @param playerId      Player ID that came from javacall_media_create function
 * @param data          Data that will be carried with this notification
 *                      - JAVACALL_EVENT_MEDIA_END_OF_MEDIA
 *                          data = Media time when the Player reached end of media and stopped.
 *                      - JAVACALL_EVENT_MEDIA_DURATION_UPDATED
 *                          data = The duration of the media.
 *                      - JAVACALL_EVENT_MEDIA_RECORD_SIZE_LIMIT
 *                          data = The media time when the recording stopped.
 *                      - JAVACALL_EVENT_MEDIA_BUFFERING_STARTED
 *                          data = Designating the media time when the buffering is started.
 *                      - JAVACALL_EVENT_MEDIA_BUFFERING_STOPPED
 *                          data = Designating the media time when the buffering stopped.
 *                      - JAVACALL_EVENT_MEDIA_VOLUME_CHANGED
 *                          data = volume value.
 *                      - JAVACALL_EVENT_MEDIA_SNAPSHOT_FINISHED
 *                          data = None.
 */
void javanotify_on_media_notification(javacall_media_notification_type type,
                                      int isolateId,
                                      int playerId, 
                                      void* data) {
    (void)isolateId;
    CVMmutexLock(&nEventMutex);
        qEvt[qtail].type = type;
        qEvt[qtail].playerId = playerId;
        qEvt[qtail].value = (long)data;
        qtail++;
        if (qtail == MM_MAX_EVENTS) {
            qtail = 0;
        }
        if (qhead == qtail) {
            qhead++;
            if (qhead == MM_MAX_EVENTS) {
                qhead = 0;
            }
        }
        CVMcondvarNotify(&nCondVar);
    CVMmutexUnlock(&nEventMutex);
}
