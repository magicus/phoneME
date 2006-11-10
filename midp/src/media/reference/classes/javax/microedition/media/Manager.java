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
package javax.microedition.media;


import java.io.InputStream;
import java.io.IOException;



/**
 * This class is defined by the JSR-118 specification
 * <em>MIDP 2,
 * Version 2.0.</em>
 */
// JAVADOC COMMENT ELIDED

public final class Manager {
    
    // JAVADOC COMMENT ELIDED
    public final static String TONE_DEVICE_LOCATOR = "device://tone";

    // JAVADOC COMMENT ELIDED
    public final static String MIDI_DEVICE_LOCATOR = "device://midi";

    // private final static String RADIO_CAPTURE_LOCATOR = "capture://radio";

    private final static String DS_ERR = "Cannot create a DataSource for: ";    

    private final static String PL_ERR = "Cannot create a Player for: ";
    
    private final static String REDIRECTED_MSG = " with exception message: ";


    // JAVADOC COMMENT ELIDED
    private Manager() { }


    // JAVADOC COMMENT ELIDED
    public static String[] getSupportedContentTypes(String protocol) {
        return new String[0];        
    }


    // JAVADOC COMMENT ELIDED
    public static String[] getSupportedProtocols(String content_type) {
        return new String[0];
    }

    // JAVADOC COMMENT ELIDED
    public static Player createPlayer(String locator)
         throws IOException, MediaException {

        if (locator == null) {
            throw new IllegalArgumentException();
        }

        throw new MediaException("Cannot create Player");
    }


    // JAVADOC COMMENT ELIDED
    public static Player createPlayer(InputStream stream, String type)
         throws IOException, MediaException {
        if (stream == null) {
            throw new IllegalArgumentException();
        }

        if (type == null) {
            throw new MediaException(PL_ERR + "NULL content-type");
        }
        
        throw new MediaException("Cannot create Player");

    }


    // JAVADOC COMMENT ELIDED
    public static void playTone(int note, int duration, int volume)
         throws MediaException {
        
        if (note < 0 || note > 127 || duration <= 0) {
            throw new IllegalArgumentException("bad param");
        }
    }
}
