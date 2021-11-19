/*
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

#ifndef __JAVANOTIFY_JSR135_H_
#define __JAVANOTIFY_JSR135_H_

/**
 * @file javanotify_multimedia.h
 * @ingroup JSR135 
 * @brief Multimedia related external events notification
 */

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
 ******************************************************************************
 ******************************************************************************

  NOTIFICATION FUNCTIONS
  - - - -  - - - - - - -  
  The following functions are implemented by Sun.
  Platform is required to invoke these function for each occurence of the
  underlying event.
  The functions need to be executed in platform's task/thread

 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/

/* Event queue ID for MMAPI. */
#define JSR135_EVENT_QUEUE_ID 135

/**
 * @enum javacall_media_notification_type
 * 
 * @brief Multimedia notification type.
 */
typedef enum {
    /** Posted when a Player has reached the end of the media. */
    JAVACALL_EVENT_MEDIA_END_OF_MEDIA = 1,      
    /** Posted when the duration of a Player is updated. */    
    JAVACALL_EVENT_MEDIA_DURATION_UPDATED, 
    /** Record size limit is reached or no more space is available */
    JAVACALL_EVENT_MEDIA_RECORD_SIZE_LIMIT,
    /** Posted when an error occurs during the recording. */
    JAVACALL_EVENT_MEDIA_RECORD_ERROR,          
    /** Posted when the system or another higher priority application has released 
        an exclusive device which is now available to the Player. */
    JAVACALL_EVENT_MEDIA_DEVICE_AVAILABLE,      
    /** Posted when the system or another higher priority application has temporarily 
        taken control of an exclusive device which was previously available to the Player. */
    JAVACALL_EVENT_MEDIA_DEVICE_UNAVAILABLE,    
    /** Posted when the native player needs more media content from Java side. */
    JAVACALL_EVENT_MEDIA_NEED_MORE_MEDIA_DATA,
    /** Posted when the Player enters into a buffering mode. */
    JAVACALL_EVENT_MEDIA_BUFFERING_STARTED,     
    /** Posted when the Player leaves the buffering mode. */
    JAVACALL_EVENT_MEDIA_BUFFERING_STOPPED,
    /** Posted when the volume changed from external action. */
    JAVACALL_EVENT_MEDIA_VOLUME_CHANGED,
    /** Posted when the blocked snapshot finished */
    JAVACALL_EVENT_MEDIA_SNAPSHOT_FINISHED,
    /** Posted when an error had occurred. */
    JAVACALL_EVENT_MEDIA_ERROR,
    /** Posted when the System Volume level has changed. */
    JAVACALL_EVENT_MEDIA_SYSTEM_VOLUME_CHANGED,
    /** Posted when an ACM error has occurred. */
    JAVACALL_EVENT_MEDIA_ACM_ERROR,
    /** Posted when the video size changed. */
    JAVACALL_EVENT_MEDIA_SIZE_CHANGED,
    /** 
     * All events that should be sent to Java listeners must be defined
     * above this marker.
     */
    JAVACALL_EVENT_MEDIA_JAVA_EVENTS_MARKER,

    /** Posted when the blocked start finished */
    JAVACALL_EVENT_MEDIA_STARTED,
    /** Posted when the blocked stop finished */
    JAVACALL_EVENT_MEDIA_STOPPED,    
    /** Posted when the blocked realize finished */
    JAVACALL_EVENT_MEDIA_REALIZED,        
    /** Posted when the blocked set media time  finished */
    JAVACALL_EVENT_MEDIA_TIME_SET,
    /** Posted when the blocked get duration finished */
    JAVACALL_EVENT_MEDIA_DURATION_GOTTEN,
    /** Posted when the blocked seek to frame finished */
    JAVACALL_EVENT_MEDIA_FRAME_SOUGHT,  
    /** Posted when the blocked skip frames finished */
    JAVACALL_EVENT_MEDIA_FRAMES_SKIPPED,
    /** Posted when the blocked get metadata key finished */
    JAVACALL_EVENT_MEDIA_METADATA_KEY_GOTTEN,  
    /** Posted when the blocked get metadata finished */
    JAVACALL_EVENT_MEDIA_METADATA_GOTTEN,
    /** Posted when the blocked get pitch finished */
    JAVACALL_EVENT_MEDIA_PITCH_GOTTEN,
    /** Posted when the blocked set pitch finished */
    JAVACALL_EVENT_MEDIA_PITCH_SET,  
    /** Posted when the blocked get rate finished */
    JAVACALL_EVENT_MEDIA_RATE_GOTTEN,          
    /** Posted when the blocked set rate finished */
    JAVACALL_EVENT_MEDIA_RATE_SET,   
    /** Posted when the blocked start recording finished */
    JAVACALL_EVENT_MEDIA_RECORD_STARTED,
    /** Posted when the blocked stop recording finished */
    JAVACALL_EVENT_MEDIA_RECORD_STOPPED,
    /** Posted when the blocked pause recording finished */
    JAVACALL_EVENT_MEDIA_RECORD_PAUSED,       
    /** Posted when the blocked reset recording finished */
    JAVACALL_EVENT_MEDIA_RECORD_RESET,       
    /** Posted when the blocked commit recording finished */
    JAVACALL_EVENT_MEDIA_RECORD_COMMITTED,           
    /** Posted when the blocked get tempo finished */
    JAVACALL_EVENT_MEDIA_TEMPO_GOTTEN,
    /** Posted when the blocked set tempo finished */
    JAVACALL_EVENT_MEDIA_TEMPO_SET,
    /** Posted when the blocked get video size finished */
    JAVACALL_EVENT_MEDIA_VIDEO_SIZE_GOTTEN,        
    /** Posted when the blocked set video location finished */
    JAVACALL_EVENT_MEDIA_VIDEO_LOCATION_SET,        
    /** Posted when the blocked set video visibility finished */
    JAVACALL_EVENT_MEDIA_VIDEO_VISIBILITY_SET,
    /** Posted when finished a short MIDI event */
    JAVACALL_EVENT_MEDIA_SHORT_MIDI_EVENT_FINISHED,
    /** Posted when finished a long MIDI event */
    JAVACALL_EVENT_MEDIA_LONG_MIDI_EVENT_FINISHED,
    
    /** Posted when preset stop time reached */
    JAVACALL_EVENT_MEDIA_STOP_TIME_REACHED,
    /** Posted when native video player finished rendering a frame */
    JAVACALL_EVENT_MEDIA_FRAME_RENDERED,

    /** Posted when the blocked do buffering finished */
    JAVACALL_EVENT_MEDIA_BUFFERING_UNBLOCKED,
    /** Posted when the blocked acquire device finished */
    JAVACALL_EVENT_MEDIA_DEVICE_ACQUIRED,
    /** Posted when the blocked release device finished */
    JAVACALL_EVENT_MEDIA_DEVICE_RELEASED,
    
    /** Posted when the blocked get volume finished */
    JAVACALL_EVENT_MEDIA_VOLUME_GOTTEN,
    /** Posted when the blocked set volume finished */
    JAVACALL_EVENT_MEDIA_VOLUME_SET,
    /** Posted when the blocked set mute finished */
    JAVACALL_EVENT_MEDIA_MUTE_SET,
    /** Posted when the blocked encode finished */
    JAVACALL_EVENT_MEDIA_ENCODE_COMPLETE    
} javacall_media_notification_type;

/** 
 * @defgroup MediaNotification Notification API for Multimedia
 * @ingroup JSR135 
 * @{
 */ 

/**
 * Post native media event to Java event handler
 * 
 * @param type          Event type
 * @param appID         Application ID that came from javacall_media_create function
 * @param playerId      Player ID that came from javacall_media_create function
 * @param status        Status of completed operation
 * @param data          Data that will be carried with this notification
 *                      - JAVACALL_EVENT_MEDIA_END_OF_MEDIA
 *                          data = Media time when the Player reached end of media and stopped.
 *                      - JAVACALL_EVENT_MEDIA_DURATION_UPDATED
 *                          data = The duration of the media.
 *                      - JAVACALL_EVENT_MEDIA_RECORD_SIZE_LIMIT
 *                          data = The media time when the recording stopped.
 *                      - JAVACALL_EVENT_MEDIA_DEVICE_AVAILABLE
 *                          data = None.
 *                      - JAVACALL_EVENT_MEDIA_DEVICE_UNAVAILABLE   
 *                          data = None.
 *                      - JAVACALL_EVENT_MEDIA_NEED_MORE_MEDIA_DATA
 *                          data = None.
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
                                      int appID,
                                      int playerId, 
                                      javacall_result status,
                                      void *data);

/** @} */

#ifdef __cplusplus
}
#endif

#endif 


