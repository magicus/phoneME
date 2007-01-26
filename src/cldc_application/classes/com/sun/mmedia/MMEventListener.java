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
        BasicPlayer p;

        switch (nevt.getType()) {
        case EventTypes.MM_EOM_EVENT:
            p = BasicPlayer.get(nevt.intParam1);
            if (p != null) {
                p.sendEvent(PlayerListener.END_OF_MEDIA, new Long(nevt.intParam2 * 1000));
            }
            break;

        case EventTypes.MM_DURATION_EVENT:
            p = BasicPlayer.get(nevt.intParam1);
            if (p != null) {
                p.sendEvent(PlayerListener.DURATION_UPDATED, new Long(nevt.intParam2 * 1000));
            }
            break;

        /* Extern volume event handler - Send to the all players in this isolate */
        case EventTypes.MM_VOLUME_CHANGED_EVENT:
            if (nevt.intParam2 < 0) {
                nevt.intParam2 = 0;
            }
            if (nevt.intParam2 > 100) {
                nevt.intParam2 = 100;
            }
            BasicPlayer.sendExternalVolumeChanged(PlayerListener.VOLUME_CHANGED, nevt.intParam2);
            break;

        case EventTypes.MM_RECORD_LIMIT_EVENT:
            // Need revisit
            break;
        
        case EventTypes.MM_RECORD_ERROR_EVENT:
            p = BasicPlayer.get(nevt.intParam1);
            if (p != null) {
                p.sendEvent(PlayerListener.RECORD_ERROR, new String("Unexpected Record Error"));
            }
            break;

        case EventTypes.MM_BUFFERING_START_EVENT:
            p = BasicPlayer.get(nevt.intParam1);
            if (p != null) {
                p.sendEvent(PlayerListener.BUFFERING_STARTED, new Long(nevt.intParam2 * 1000));
            }
            break;
        
        case EventTypes.MM_BUFFERING_STOP_EVENT:
            p = BasicPlayer.get(nevt.intParam1);
            if (p != null) {
                p.sendEvent(PlayerListener.BUFFERING_STOPPED, new Long(nevt.intParam2 * 1000));
            }
            break;

        case EventTypes.MM_GENERAL_ERROR_EVENT:
            p = BasicPlayer.get(nevt.intParam1);
            if (p != null) {
                p.sendEvent(PlayerListener.RECORD_ERROR, new String("Unexpected Media Error"));
            }
            break;
        }
    }
}
