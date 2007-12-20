/*
 *    
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

#include <javacall_events.h>
#include <javanotify_multimedia.h>

/**
 * Post native media event to Java event handler
 * 
 * @param type          Event type
 * @param isolateId     Isolate ID that came from javacall_media_create function
 * @param playerId      Player ID that came from javacall_media_create function
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
                                      void *data) {
    struct {
    	int     event_type;
        int     isolateId;
        int     type;
        int     playerId;
        long    data;
    }event = {135, isolateId, type, playerId, *((long *)data)};

    javacall_event_send((unsigned char *)&event, sizeof(event));
}
