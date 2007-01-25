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

/**
 * Event listener for events delivered from native layer
 */
class MMEventListener {

    /**
     * A thread waiting for incoming events.
     */
    private static Thread eventThread;
    /**
     * Performs OS specific checking for a file root mount/unmount.
     */
    private native static boolean waitEventFromNative();
    private native static int nativeEventInit();
    private native static int nativeEventDestroy();
    private native static boolean getNativeEvent(MMNativeEventImpl event);
    
    public MMEventListener() {
        if (eventThread == null) {
            // Create event thread
            eventThread = new Thread() {
                /**
                 * The main loop
                 */
                public void run() {
                    try {
                        nativeEventInit();
                        while (true) {
                            if (waitEventFromNative()) {
                                MMNativeEventImpl event = new MMNativeEventImpl();
                                if (getNativeEvent(event)) {
                                    BasicPlayer p = BasicPlayer.get(event.playerId);
                                    switch(event.eventId) {
                                        case MMNativeEventImpl.EOM_EVENT:
                                            p.sendEvent(PlayerListener.END_OF_MEDIA, new Long(event.value));
                                        break;
                                        case MMNativeEventImpl.RSL_EVENT:

                                            p.sendEvent(PlayerListener.RECORD_STOPPED, new Long(event.value));
                                            break;
                                    }
                                    Thread.yield();
                                }
                            } else {
                                sleep(10);
                            };
                        }
                    } catch (Exception ex) {
                    }
                }
                protected void finalize() {
                    nativeEventDestroy();
                }
            };
            // Start the thread
            eventThread.start();

        }
    }

    /**
     * Receiving anevents is Platform spevific. Will be defined later
     * Process of events shall be performed in the following way:
     *           BasicPlayer p;
     *           p = BasicPlayer.get(playerId_Inside_Isolate);
     *           p.sendEvent(PlayerListener.END_OF_MEDIA, (Object)new Long(nevt.intParam2 * 1000)); 
     */
}

class MMNativeEventImpl {
    static final int EOM_EVENT = 1;
    static final int RSL_EVENT = 2;
    int playerId;
    int eventId;
    long value;
    void MMNativeEventImpl() {
        playerId = -1;
        eventId = -1;
        value = -1;
    }
}

