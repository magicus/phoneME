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
import com.sun.mmedia.BasicPlayer;
import com.sun.mmedia.TonePlayer;
import com.sun.mmedia.Configuration;
import com.sun.mmedia.PermissionAccessor;
import com.sun.mmedia.protocol.BasicDS;
import com.sun.mmedia.protocol.CommonDS;

import javax.microedition.media.protocol.DataSource;
import javax.microedition.media.protocol.SourceStream;


/**
 * This class is defined by the JSR-135 specification
 * <em>Mobile Media API,
 * Version 1.2.</em>
 */
// JAVADOC COMMENT ELIDED

public final class Manager {
    private static Configuration config = Configuration.getConfiguration();
    private static TonePlayer tonePlayer;
    private static TimeBase sysTimeBase = new SystemTimeBase();
    private static Object createLock = new Object();
    
    // JAVADOC COMMENT ELIDED
    public final static String TONE_DEVICE_LOCATOR = "device://tone";

    // JAVADOC COMMENT ELIDED
    public final static String MIDI_DEVICE_LOCATOR = "device://midi";

    // private final static String RADIO_CAPTURE_LOCATOR = "capture://radio";

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
        BasicPlayer p;
	
        /*
         * TBD: in general all capture devices 
         * shall be checked for 
         * 1). capture://<device>
         * 2). capture://<device>?<media_encodings>
         * 3). all other combinations are invalid
         *
         * These is second type of URIs: device://<name>. 
         *
         * According to RFC2396 (Generic Syntax of URIs), 
         * <scheme>://<device> part is case incensitive,
         * but others (media encodings in our case) are case sensitive...
         */
        if (locStr.equals(validLoc = TONE_DEVICE_LOCATOR) || 
            locStr.equals(validLoc = MIDI_DEVICE_LOCATOR) ||
            locStr.startsWith(validLoc = config.RADIO_CAPTURE_LOCATOR) || 
            locStr.startsWith(validLoc = config.VIDEO_CAPTURE_LOCATOR)) {
                
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
                p = (BasicPlayer) protoClass.newInstance();
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

            DataSource ds = createDataSource(locator);

            Player pp = null;

            try {
                pp = createPlayer(ds);
            } catch (MediaException ex) {
                ds.disconnect();
                throw ex;
            } catch (IOException ex) {
                ds.disconnect();
                throw ex;
            }
            
            //System.out.println("DEBUG: Manager.createPlayer(" + locator + ") returned#2 " + pp);
            return pp;
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

        type = type.toLowerCase();

        // Wrap the input stream with a CommonDS where the input
        // can be handled in a generic way.
        
        CommonDS ds = new CommonDS();
        
        ds.setInputStream(stream);
        ds.setContentType(type);
        //ds.locator is NULL
     
        try {
            return createPlayer(ds);
        } catch (IOException ex) {
            throw new MediaException(PL_ERR + ex.getMessage());
        }
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
    
    /**
     *  Gets the playerFromType attribute of the Manager class
     *
     * @param  type                Description of the Parameter
     * @return                     The playerFromType value
     * @exception  IOException     Description of the Exception
     * @exception  MediaException  Description of the Exception
     */
    private static BasicPlayer getPlayerFromType(String type) 
        throws IOException, MediaException {
        String className;      
        
        if (type == null) {
            throw new MediaException(PL_ERR + "NULL content-type");
        }
        if ((className = config.getHandler(type)) == null) {
            throw new MediaException(PL_ERR + type);
        }

        BasicPlayer p = null;

        try {
            // ... try and instantiate the handler ...
            Class handlerClass = Class.forName(className);
            
            p = (BasicPlayer) handlerClass.newInstance();
        } catch (Exception e) {
            throw new MediaException(PL_ERR + type + 
                    REDIRECTED_MSG + e.getMessage());
        }
        return p;
    }

    /**
     * MMAPI full specific methods.
     *
     * @param  source              Description of the Parameter
     * @return                     Description of the Return Value
     * @exception  IOException     Description of the Exception
     * @exception  MediaException  Description of the Exception
     */

    // JAVADOC COMMENT ELIDED
    public static Player createPlayer(DataSource source)
         throws IOException, MediaException {
        
        if (source == null) {
            throw new IllegalArgumentException();
        }
        
        source.connect();

        String contentType = source.getContentType();
        String locator = source.getLocator(); // can be null
        PermissionAccessor.checkContentTypePermissions(locator, contentType);

        BasicPlayer p = getPlayerFromType(contentType);

        p.setSource(source);

        return p;
    }

    /**
     * Create a <code>DataSource</code> for the specified media
     * identified by a locator.  The <code>DataSource</code>
     * returned can be used to read the media data from the input
     * source.
     * <p>
     * The returned data source is <i>connected</i>;
     * <code>DataSource.connect</code> has been invoked.
     * <p>
     * If no suitable <code>DataSource</code> can be found to
     * handle the input, a <CODE>MediaException</CODE>
     * is thrown.
     *
     * @param  locator             The source protocol for the media data.
     * @return                     A connected <CODE>DataSource</CODE>.
     * @exception  MediaException  Thrown if no <CODE>DataSource</CODE>
     * can be found that supports the given protocol as specified by
     * the locator.
     * @exception  IOException     Thrown if there was a problem connecting
     * with the source (e.g. the source media does not exist).
     */
    private static DataSource createDataSource(String locator)
         throws IOException, MediaException {

        String className = config.getHandler(BasicDS.getProtocol(locator));

        /* invalid locator - throw MediaException */
        if (className == null) {
            throw new MediaException(DS_ERR + locator);
        }

        /* check valid locator  permissions */
        PermissionAccessor.checkLocatorPermissions(locator);
        
        try {
            // ... Try and instance a DataSource ....
            Class protoClass = Class.forName(className);
            DataSource source = (DataSource) protoClass.newInstance();
            // ... and get it connected ....
            ((BasicDS) source).setLocator(locator);
            source.connect();

            return source;
        } catch (IOException e) {
            throw e;
        } catch (MediaException e) {
            throw e;
        } catch (Exception e) {
            throw new MediaException(DS_ERR + locator + 
                    REDIRECTED_MSG + e.getMessage());
        }
    }

    // JAVADOC COMMENT ELIDED
    public static TimeBase getSystemTimeBase() {
        return sysTimeBase;
    }
}


/**
 * SystemTimeBase is the implementation of the default <CODE>TimeBase</CODE>
 * based on the system clock.
 *
 * @created    January 13, 2005
 * @see        TimeBase
 */
class SystemTimeBase implements TimeBase {
    /*
     *  Pick some offset (start-up time) so the system time won't be
     *  so huge.  The huge numbers overflow floating point operations
     *  in some cases.
     */
    private long offset;
    
    SystemTimeBase() {
        offset = System.currentTimeMillis() * 1000L;
    }

    /**
     * This is a straight-forward implementation of a
     * system time base using the system clock.
     *
     * @return    The time value
     */
    public long getTime() {
        return (System.currentTimeMillis() * 1000L) - offset;
    }
}
