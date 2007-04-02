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

import java.util.Vector;
import java.io.IOException;
import java.io.InputStream;

import java.util.Enumeration;
import java.util.Hashtable;
import com.sun.mmedia.TonePlayer;
import com.sun.mmedia.Configuration;

import com.sun.mmedia.ABBBasicPlayer;

/**
 * This class is defined by the JSR-135 specification
 * <em>Mobile Media API,
 * Version 1.2.</em>
 */
// JAVADOC COMMENT ELIDED

public final class Manager {
    private static Configuration config = Configuration.getConfiguration();
    private static TonePlayer tonePlayer;
    private static Object createLock = new Object();
    
    // JAVADOC COMMENT ELIDED
    public final static String TONE_DEVICE_LOCATOR = "device://tone";

    private final static String DS_ERR = "Cannot create a DataSource for: ";    

    private final static String PL_ERR = "Cannot create a Player for: ";
    
    private final static String REDIRECTED_MSG = " with exception message: ";


    /**
     * This private constructor keeps anyone from actually
     * getting a <CODE>Manager</CODE>.
     */
    private Manager() { }


    // JAVADOC COMMENT ELIDED
    public static String[] getSupportedContentTypes(String protocol) {
        return config.getSupportedContentTypes(protocol);        
    }


    // JAVADOC COMMENT ELIDED
    public static String[] getSupportedProtocols(String content_type) {
        return config.getSupportedProtocols(content_type);
    }

  
    // JAVADOC COMMENT ELIDED
    public static Player createPlayer(String locator)
         throws IOException, MediaException {

        if (locator == null) {
            throw new IllegalArgumentException();
        }

        String locStr = locator.toLowerCase();
        String validLoc;
	
        if (locStr.equals(validLoc = TONE_DEVICE_LOCATOR)) {
                
            ABBBasicPlayer p;
            
            // separate device & encodings
            int encInd = locator.indexOf('?');
            String encStr = null;
            if (encInd > 0) {
                encStr = locator.substring(encInd + 1);
                locStr = locStr.substring(0, encInd);
                /* 
                 * TBD: check case when '?' is the last Locator symbol:
                 *Will encStr be == "", or it will be == null ?
                 */
            } else {
                /*
                 * detect invalid locator case: 
                 * if no '?' found then locStr & validLoc shall be 
                 * equivalent strings, but since they have already passed 
                 * char-to-char comparison, we to check lengths only...
                 */
                if (locStr.length() != validLoc.length())
                    throw new MediaException("Malformed locator");
                encStr = "";
            }
            String className = config.getHandler(locStr);
            try {
                // Try and instance the player ....
                Class protoClass = Class.forName(className);
                p = (ABBBasicPlayer) protoClass.newInstance();
                // Pass encStr to created Player as argument
                if (!p.initFromURL(encStr)) {
                    throw new MediaException("Invalid locator media encodings");
                };
                //System.out.println("DEBUG: Manager.createPlayer(" + locator + ") returned#1 " + p);
                return p;
            } catch (Exception e) {
                throw new MediaException(PL_ERR + locator + 
                        REDIRECTED_MSG + e.getMessage());
            }
        } else { 
            // not in the list of predefined locators, 
            // find handler by extension

            String theProtocol = null;
            int idx = locStr.indexOf(':');

            if (idx != -1) {
                theProtocol = locStr.substring(0, idx);
            } else {
                throw new MediaException("Malformed locator");
            }

            String[] supported = getSupportedProtocols(config.ext2Mime(locStr));
            boolean found = false;
            for (int i = 0; i < supported.length; i++) {
                if (theProtocol.equals(supported[i])) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                throw new MediaException(PL_ERR + locator);
            }

            Player pp = null;

            throw new MediaException("Not supported");
            
            //System.out.println("DEBUG: Manager.createPlayer(" + locator + ") returned#2 " + pp);
            // return pp;
        }
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

        throw new MediaException("Not supported");
    }


    // JAVADOC COMMENT ELIDED
    public static void playTone(int note, int duration, int volume)
         throws MediaException {
        
        if (note < 0 || note > 127 || duration <= 0) {
            throw new IllegalArgumentException("bad param");
        }

        if (duration == 0 || volume == 0) {
            return;
        }

        synchronized(createLock) {
            if (tonePlayer == null) {
                tonePlayer = config.getTonePlayer();
            }
        }
        
        if (tonePlayer != null) {
            tonePlayer.playTone(note, duration, volume);
        } else {
            throw new MediaException("no tone player");
        }
    }
}
