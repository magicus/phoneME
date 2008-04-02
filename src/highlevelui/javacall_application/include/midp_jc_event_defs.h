/*
 *
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
#ifndef __MIDP_JC_EVENT_DEFS_H_
#define __MIDP_JC_EVENT_DEFS_H_


/**
 * @file
 *
 * @brief This file describes internal event message codes and structures
 *        used by MIDP javacall port implementation.
 */


#ifdef __cplusplus
extern "C" {
#endif

#ifdef ENABLE_JSR_120
#include <javacall_sms.h>
#include <javacall_cbs.h>
#endif
#ifdef ENABLE_JSR_205
#include <javacall_mms.h>
#endif
#include <javacall_events.h>
#include <javacall_time.h>
#include <javacall_socket.h>
#include <javacall_datagram.h>
#ifdef USE_VSCL
#include <javacall_vscl.h>
#endif
#include <javacall_network.h>

#ifdef ENABLE_JSR_135
#include <javacall_multimedia.h>
#include <javanotify_multimedia.h>
#endif
#include <javacall_keypress.h>
#include <javacall_penevent.h>
#include <javacall_input.h>
#include <javacall_lifecycle.h>
#ifdef ENABLE_JSR_177
#include "javacall_carddevice.h"
#endif /* ENABLE_JSR_177 */
#ifdef ENABLE_JSR_179
#include "javacall_location.h"
#endif /* ENABLE_JSR_179 */
#ifdef ENABLE_JSR_211
#include "jsr211_platform_invoc.h"
#endif /* ENABLE_JSR_211 */
#if ENABLE_JSR_234
#include <javacall_multimedia_advanced.h>
#endif /* ENABLE_JSR_234 */
#include <javacall_security.h>
#ifdef ENABLE_JSR_256
#include "javacall_sensor.h"
#endif /* ENABLE_JSR_256 */


#define MIDP_RUNMIDLET_MAXIMUM_ARGS 10

typedef enum {
    MIDP_JC_EVENT_KEY                  =100,
    MIDP_JC_EVENT_START_ARBITRARY_ARG  ,
    MIDP_JC_EVENT_END                  ,
    MIDP_JC_EVENT_KILL                 ,
    MIDP_JC_EVENT_SOCKET               ,
    MIDP_JC_EVENT_NETWORK              ,
    MIDP_JC_EVENT_TIMER                ,
    MIDP_JC_EVENT_PUSH                 ,
#ifdef ENABLE_JSR_120
    MIDP_JC_EVENT_SMS_SENDING_RESULT   ,
    MIDP_JC_EVENT_SMS_INCOMING         ,
    MIDP_JC_EVENT_CBS_INCOMING         ,
#endif
#ifdef ENABLE_JSR_205
    MIDP_JC_EVENT_MMS_SENDING_RESULT   ,
    MIDP_JC_EVENT_MMS_INCOMING         ,
#endif
    MIDP_JC_EVENT_MULTIMEDIA           ,
    MIDP_JC_EVENT_PAUSE                ,
    MIDP_JC_EVENT_RESUME               ,
    MIDP_JC_EVENT_INTERNAL_PAUSE       ,
    MIDP_JC_EVENT_INTERNAL_RESUME      ,
    MIDP_JC_EVENT_TEXTFIELD            ,
    MIDP_JC_EVENT_IMAGE_DECODER        ,
    MIDP_JC_EVENT_PEN                  ,
    MIDP_JC_EVENT_PERMISSION_DIALOG    ,
#ifdef ENABLE_JSR_179
    JSR179_LOCATION_JC_EVENT           ,
    JSR179_PROXIMITY_JC_EVENT          ,
#endif /* ENABLE_JSR_179 */
    MIDP_JC_EVENT_SPRINT_MASTER_VOLUME ,
    MIDP_JC_EVENT_SPRINT_STATE_CHANGE  ,
    MIDP_JC_EVENT_PHONEBOOK            ,
    MIDP_JC_EVENT_INSTALL_CONTENT      ,
    MIDP_JC_EVENT_SWITCH_FOREGROUND    ,
#ifdef ENABLE_JSR_177
    MIDP_JC_EVENT_CARDDEVICE           ,
#endif /* ENABLE_JSR_177 */
#if ENABLE_MULTIPLE_ISOLATES
    MIDP_JC_EVENT_SWITCH_FOREGOUND     ,
    MIDP_JC_EVENT_SELECT_APP           ,
#endif /*ENABLE_MULTIPLE_ISOLATES*/
#ifdef ENABLE_JSR_211
    JSR211_JC_EVENT_PLATFORM_FINISH      ,
    JSR211_JC_EVENT_JAVA_INVOKE          ,
#endif
#if ENABLE_JSR_234
    MIDP_JC_EVENT_ADVANCED_MULTIMEDIA  ,
#endif /*ENABLE_JSR_234*/
    JSR75_FC_JC_EVENT_ROOTCHANGED      ,
#if ENABLE_ON_DEVICE_DEBUG
    MIDP_JC_ENABLE_ODD_EVENT           ,
#endif /* ENABLE_ON_DEVICE_DEBUG */
    MIDP_JC_EVENT_ROTATION             ,
    MIDP_JC_EVENT_SET_VM_ARGS          ,
    MIDP_JC_EVENT_SET_HEAP_SIZE        ,
    MIDP_JC_EVENT_LIST_MIDLETS         ,
    MIDP_JC_EVENT_LIST_STORAGE_NAMES   ,
    MIDP_JC_EVENT_REMOVE_MIDLET
#if ENABLE_JSR_256
    ,JSR256_JC_EVENT_SENSOR_AVAILABLE  ,
    JSR256_JC_EVENT_SENSOR_OPEN_CLOSE  ,
    JSR256_JC_EVENT_SENSOR_DATA_READY
#endif /*ENABLE_JSR_256*/
} midp_jc_event_type;


typedef enum {
    MIDP_NETWORK_UP         = 1000,
    MIDP_NETWORK_DOWN       = 1001
} midp_network_event_type;

#ifdef ENABLE_JSR_177
typedef enum {
    MIDP_CARDDEVICE_RESET,
    MIDP_CARDDEVICE_XFER,
    MIDP_CARDDEVICE_UNLOCK
} midp_carddevice_event_type;
#endif /* ENABLE_JSR_177 */
typedef struct {
    javacall_key             key; /* '0'-'9','*','# */
    javacall_keypress_type  keyEventType; /* presed, released, repeated ... */
} midp_jc_event_key;

typedef struct {
    int   argc;
    char* argv[MIDP_RUNMIDLET_MAXIMUM_ARGS];
} midp_jc_event_start_arbitrary_arg;

typedef struct {
    int heap_size;
} midp_event_heap_size;

typedef struct {
    char* suiteID;
} midp_event_remove_midlet;

typedef struct {
    javacall_handle   handle;
    javacall_result   status;
    unsigned int      waitingFor;
    javacall_handle   extraData;
} midp_jc_event_socket;

typedef struct {
    midp_network_event_type netType;
} midp_jc_event_network;

typedef struct {
    int stub;
} midp_jc_event_timer;

typedef struct {
    int            alarmHandle;
} midp_jc_event_push;

#ifdef ENABLE_JSR_120
typedef struct {
    javacall_handle         handle;
    javacall_result result;
} midp_jc_event_sms_sending_result;

typedef struct {
    int stub;
} midp_jc_event_sms_incoming;

typedef struct {
    int stub;
} midp_jc_event_cbs_incoming;
#endif

#ifdef ENABLE_JSR_205
typedef struct {
    javacall_handle         handle;
    javacall_result result;
} midp_jc_event_mms_sending_result;

typedef struct {
    int stub;
} midp_jc_event_mms_incoming;
#endif

#ifdef ENABLE_JSR_135
typedef struct {
    javacall_media_notification_type mediaType;
    int appId;
    int playerId;
    int status;
    long data;
} midp_jc_event_multimedia;
#endif

typedef struct {
    javacall_textfield_status status;
} midp_jc_event_textfield;

typedef struct {
    javacall_handle handle;
    javacall_result result;
} midp_jc_event_image_decoder;

#ifdef ENABLE_JSR_179
typedef struct {
    javacall_location_callback_type event;
    javacall_handle provider;
    javacall_location_result operation_result;
} jsr179_jc_event_location;

typedef struct {
    javacall_handle provider;
    double latitude;
    double longitude;
    float proximityRadius;
    javacall_location_location location;
    javacall_location_result operation_result;
} jsr179_jc_event_proximity;
#endif /* ENABLE_JSR_179 */

#ifdef ENABLE_JSR_211
typedef struct {
    int invoc_id;
    jsr211_platform_event *jsr211event;
} jsr211_jc_event_platform_event;
#endif

#ifdef ENABLE_JSR_256
typedef struct {
    javacall_sensor_type sensor_type;
    javacall_bool is_available;
} jsr256_jc_event_sensor_available;

typedef struct {
    javacall_sensor_type sensor;
    javacall_bool isOpen;
    int errCode;
} jsr256_jc_event_sensor_t;
#endif /* ENABLE_JSR_256 */

typedef struct {
    javacall_penevent_type type;
    int x;
    int y;
} midp_jc_event_pen;

typedef struct {
    javacall_security_permission_type permission_level;
} midp_jc_event_permission_dialog;

typedef struct {
    char*               httpUrl;
    javacall_utf16*     descFilePath;
    int                 descFilePathLen;
    javacall_bool       isJadFile;
    javacall_bool       isSilent;
} midp_jc_event_install_content;


#ifdef ENABLE_JSR_177
typedef struct {
    midp_carddevice_event_type eventType;
    int handle;
} midp_jc_event_carddevice;
#endif /* ENABLE_JSR_177 */

typedef struct {
    char* phoneNumber;
} midp_jc_event_phonebook;

typedef struct {
    int data;
}jsr75_jc_event_root_changed;

typedef struct {
    midp_jc_event_type                     eventType;
    union {
        midp_jc_event_key                  keyEvent;
        midp_jc_event_start_arbitrary_arg  startMidletArbitraryArgEvent;
        midp_jc_event_socket               socketEvent;
        midp_jc_event_network              networkEvent;
        midp_jc_event_timer                timerEvent;
        midp_jc_event_push                 pushEvent;
#ifdef ENABLE_JSR_120
        midp_jc_event_sms_sending_result   smsSendingResultEvent;
        midp_jc_event_sms_incoming         smsIncomingEvent;
        midp_jc_event_cbs_incoming         cbsIncomingEvent;
#endif
#ifdef ENABLE_JSR_205
        midp_jc_event_mms_sending_result   mmsSendingResultEvent;
        midp_jc_event_mms_incoming         mmsIncomingEvent;
#endif
#ifdef ENABLE_JSR_135
        midp_jc_event_multimedia           multimediaEvent;
#endif
        midp_jc_event_textfield            textFieldEvent;
        midp_jc_event_image_decoder        imageDecoderEvent;
#ifdef ENABLE_JSR_179
        jsr179_jc_event_location           jsr179LocationEvent;
        jsr179_jc_event_proximity          jsr179ProximityEvent;
#endif /* ENABLE_JSR_179 */
        midp_jc_event_pen                  penEvent;
        midp_jc_event_permission_dialog    permissionDialog_event;
        midp_jc_event_install_content      install_content;
        midp_jc_event_phonebook            phonebook_event;
#ifdef ENABLE_JSR_177
        midp_jc_event_carddevice           carddeviceEvent;
#endif /* ENABLE_JSR_177 */
        jsr75_jc_event_root_changed        jsr75RootchangedEvent;
#ifdef ENABLE_JSR_211
        jsr211_jc_event_platform_event     jsr211PlatformEvent;
#endif

        midp_event_heap_size               heap_size;
        midp_event_remove_midlet           removeMidletEvent;
#ifdef ENABLE_JSR_256
        jsr256_jc_event_sensor_available   jsr256SensorAvailable;
        jsr256_jc_event_sensor_t           jsr256_jc_event_sensor;
#endif /* ENABLE_JSR_256 */
    } data;

} midp_jc_event_union;

#define BINARY_BUFFER_MAX_LEN 4096


#ifdef __cplusplus
}
#endif

#endif 



