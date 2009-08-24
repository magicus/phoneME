/*
 *
 *  Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
 *  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License version
 *  2 only, as published by the Free Software Foundation. 
 *  
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  General Public License version 2 for more details (a copy is
 *  included at /legal/license.txt). 
 *  
 *  You should have received a copy of the GNU General Public License
 *  version 2 along with this work; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 *  02110-1301 USA 
 *  
 *  Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 *  Clara, CA 95054 or visit www.sun.com if you need additional
 *  information or have any questions. 
 */
package com.sun.mmedia;

import javax.microedition.media.*;
import com.sun.midp.events.*;

/**
 * Event listener for events delivered from native layer
 */
class MMEventListener implements EventListener {

    /**
     *  the following constants must be consistent 
     *  with javacall_media_notification_type enum values
     *  JAVACALL_EVENT_MEDIA_***, defined in javanotify_multimedia.h
     */
    private static final int EVENT_MEDIA_CREATE_FINISHED         =  1;
    private static final int EVENT_MEDIA_DESTROY_FINISHED        =  2;
    private static final int EVENT_MEDIA_STOP_FINISHED           =  3;
    private static final int EVENT_MEDIA_PAUSE_FINISHED          =  4;
    private static final int EVENT_MEDIA_RUN_FINISHED            =  5;
    private static final int EVENT_MEDIA_SET_MEDIA_TIME_FINISHED =  6;
    private static final int EVENT_MEDIA_DATA_REQUEST            =  7;
    private static final int EVENT_MEDIA_LENGTH_REQUEST          =  8;
    private static final int EVENT_MEDIA_DEVICE_AVAILABLE        =  9;
    private static final int EVENT_MEDIA_DEVICE_UNAVAILABLE      = 10;
    private static final int EVENT_MEDIA_ERROR                   = 11;
    private static final int EVENT_MEDIA_END_OF_MEDIA            = 12;
    private static final int EVENT_MEDIA_DURATION_UPDATED        = 13;
    private static final int EVENT_MEDIA_BUFFERING_STARTED       = 14;
    private static final int EVENT_MEDIA_BUFFERING_STOPPED       = 15;
    private static final int EVENT_MEDIA_RECORD_ERROR            = 16;
    private static final int EVENT_MEDIA_RECORD_SIZE_LIMIT       = 17;
    private static final int EVENT_MEDIA_VOLUME_CHANGED          = 18;
    private static final int EVENT_MEDIA_SYSTEM_VOLUME_CHANGED   = 19;
    private static final int EVENT_MEDIA_SNAPSHOT_FINISHED       = 20;

    MMEventListener() {
        MMEventHandler.setListener(this);
    }

    public boolean preprocess(Event event, Event waitingEvent) {
        return true;
    }

    /**
     * Process an event.
     * This method will get called in the event queue processing thread.
     *
     * @param event event to process
     */
    public void process(Event event) {
        NativeEvent nevt = (NativeEvent)event;
        HighLevelPlayer p = null;

                if( EventTypes.MMAPI_EVENT != nevt.getType() ) {
                    return;
                }

        switch ( nevt.intParam4 ) {
        case EVENT_MEDIA_END_OF_MEDIA:
            p = HighLevelPlayer.get(nevt.intParam1);
            if (p != null) {
                p.sendEvent(PlayerListener.END_OF_MEDIA, new Long(nevt.intParam2 * 1000));
            }
            break;

        case EVENT_MEDIA_DURATION_UPDATED:
            p = HighLevelPlayer.get(nevt.intParam1);
            if (p != null) {
                p.sendEvent(PlayerListener.DURATION_UPDATED, new Long(nevt.intParam2 * 1000));
            }
            break;

        case EVENT_MEDIA_VOLUME_CHANGED:
            p = HighLevelPlayer.get(nevt.intParam1);
            if (p != null) {
                if (nevt.intParam2 < 0) {
                    nevt.intParam2 = 0;
                }
                if (nevt.intParam2 > 100) {
                    nevt.intParam2 = 100;
                }
                p.sendEvent(PlayerListener.VOLUME_CHANGED, new Long(nevt.intParam2));
            }
            break;

        // System volume event handler - Send to the all players in this isolate
        case EVENT_MEDIA_SYSTEM_VOLUME_CHANGED:
            if (nevt.intParam2 < 0) {
                nevt.intParam2 = 0;
            }
            if (nevt.intParam2 > 100) {
                nevt.intParam2 = 100;
            }
            HighLevelPlayer.sendSystemVolumeChanged(nevt.intParam2);
            break;

        case EVENT_MEDIA_RECORD_SIZE_LIMIT:
            p = HighLevelPlayer.get(nevt.intParam1);
            if(p != null) {
                p.receiveRSL();
                        }       
            break;

        case EVENT_MEDIA_RECORD_ERROR:
            p = HighLevelPlayer.get(nevt.intParam1);
            if (p != null) {
                p.sendEvent(PlayerListener.RECORD_ERROR, new String("Unexpected Record Error"));
            }
            break;

        case EVENT_MEDIA_BUFFERING_STARTED:
            p = HighLevelPlayer.get(nevt.intParam1);
            if (p != null) {
                p.sendEvent(PlayerListener.BUFFERING_STARTED, new Long(nevt.intParam2 * 1000));
            }
            break;

        case EVENT_MEDIA_BUFFERING_STOPPED:
            p = HighLevelPlayer.get(nevt.intParam1);
            if (p != null) {
                p.sendEvent(PlayerListener.BUFFERING_STOPPED, new Long(nevt.intParam2 * 1000));
            }
            break;

        case EVENT_MEDIA_ERROR:
            p = HighLevelPlayer.get(nevt.intParam1);
            if (p != null) {
                p.sendEvent(PlayerListener.ERROR, new String("Unexpected Media Error"));
            }
            break;

        case EVENT_MEDIA_DATA_REQUEST:
            p = HighLevelPlayer.get(nevt.intParam1);
            DirectDebugOut.nDebugPrint("Java: MMEventListener: Data request");
            if (p != null) {
                p.continueDownload();
            }
            break;

        case EVENT_MEDIA_SET_MEDIA_TIME_FINISHED:
        case EVENT_MEDIA_CREATE_FINISHED:
        case EVENT_MEDIA_PAUSE_FINISHED:
        case EVENT_MEDIA_RUN_FINISHED:
        case EVENT_MEDIA_STOP_FINISHED:
        case EVENT_MEDIA_DESTROY_FINISHED:
            p = HighLevelPlayer.get(nevt.intParam1);
            AsyncExecutor ae = null;
            if ( null != p ) {
                ae = p.getAsyncExecutor();
                if (null != ae) {
                    ae.unblockOnEvent(nevt.intParam5, nevt.intParam2);
                }
            }
            break;

        case EVENT_MEDIA_SNAPSHOT_FINISHED:
            p = HighLevelPlayer.get( nevt.intParam1 );
            if( null != p )
            {
                p.notifySnapshotFinished();
            }
            break;
        }
    }
}
