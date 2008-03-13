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
#include "javanotify_multimedia.h"

typedef struct {
    int type;
    int playerId;
    long value;
}NativeEvent;

#define EVENT_TYPE_NAME "eventId"
#define PLAYER_ID_NAME  "playerId"
#define VALUE_NAME      "value"

#define EOM_EVENT_NAME "EOM_EVENT"
#define RSL_EVENT_NAME "RSL_EVENT"

/*  protected native boolean getNativeEvent (byte[], com/sun/mmedia/MMNativeEventImpl) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_MMEventListener_parseNativeEvent) {
    jboolean ret = KNI_FALSE;
    jfieldID fieldID;
    int type = -1;
    NativeEvent nEvent;

    KNI_StartHandles(2);
    KNI_DeclareHandle(bufferHandle);
    KNI_GetParameterAsObject(1, bufferHandle);
    KNI_GetRawArrayRegion(bufferHandle, 0, sizeof(NativeEvent), (jbyte*)&nEvent);

    KNI_DeclareHandle(classHandle);
    KNI_DeclareHandle(event);
    
    KNI_GetParameterAsObject(2, event);
    KNI_GetObjectClass(event, classHandle);

    switch(nEvent.type) {
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
        KNI_SetIntField(event, fieldID, nEvent.playerId);

        fieldID = KNI_GetFieldID(classHandle, VALUE_NAME, "J");
        KNI_SetLongField(event, fieldID, nEvent.value);

        ret = KNI_TRUE;
    }
    KNI_EndHandles();
    KNI_ReturnBoolean(ret);
}
