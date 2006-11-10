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
package javax.microedition.media;

/**
 * This class is defined by the JSR-135 specification
 * <em>Mobile Media API,
 * Version 1.2.</em>
 */
// JAVADOC COMMENT ELIDED
public interface PlayerListener {

    // JAVADOC COMMENT ELIDED
    String STARTED = "started";

    // JAVADOC COMMENT ELIDED
    String STOPPED = "stopped";


    // JAVADOC COMMENT ELIDED
    String STOPPED_AT_TIME = "stoppedAtTime";


    // JAVADOC COMMENT ELIDED
    String END_OF_MEDIA = "endOfMedia";

    // JAVADOC COMMENT ELIDED
    String DURATION_UPDATED = "durationUpdated";

    // JAVADOC COMMENT ELIDED
    String DEVICE_UNAVAILABLE = "deviceUnavailable";

    // JAVADOC COMMENT ELIDED
    String DEVICE_AVAILABLE = "deviceAvailable";

    // JAVADOC COMMENT ELIDED
    String VOLUME_CHANGED = "volumeChanged";


    // JAVADOC COMMENT ELIDED
    String SIZE_CHANGED = "sizeChanged";


    // JAVADOC COMMENT ELIDED
    String ERROR = "error";

    // JAVADOC COMMENT ELIDED
    String CLOSED = "closed";


    // JAVADOC COMMENT ELIDED
    String RECORD_STARTED = "recordStarted";

    // JAVADOC COMMENT ELIDED
    String RECORD_STOPPED = "recordStopped";

    // JAVADOC COMMENT ELIDED
    String RECORD_ERROR = "recordError";

    // JAVADOC COMMENT ELIDED
    String BUFFERING_STARTED = "bufferingStarted";

    // JAVADOC COMMENT ELIDED
    String BUFFERING_STOPPED = "bufferingStopped";


    // JAVADOC COMMENT ELIDED
    void playerUpdate(Player player, String event, Object eventData);
}


