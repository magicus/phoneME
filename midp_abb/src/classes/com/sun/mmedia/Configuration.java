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
package com.sun.mmedia;

import java.util.Hashtable;

import javax.microedition.media.Player;


/**
 *  The configuration module for MMAPI.
 *
 * @created    January 13, 2005
 */
public abstract class Configuration {
    
    /**
     * A hash table of the protocol and content handlers.
     */
    protected Hashtable handlers;

    /**
     * A hash table of media processors.
     */
    protected Hashtable processors;

    /**
     * A table of mime types.
     */
    protected Hashtable mimeTypes;

    /**
     *  The current configuration object.
     */
    private static Configuration config;

    /**
     *  True if players loop in native code,
     *  otherwise false
     */
    protected static boolean nativeLooping = false;
    
    public final static String TONE_DEVICE_LOCATOR = "device://tone"; //javax.microedition.media.Manager.TONE_DEVICE_LOCATOR;
    public final static String MIDI_DEVICE_LOCATOR = "device://midi"; //javax.microedition.media.Manager.MIDI_DEVICE_LOCATOR;
    public final static String RADIO_CAPTURE_LOCATOR = "capture://radio";
    public final static String AUDIO_CAPTURE_LOCATOR = "capture://audio";
    public final static String VIDEO_CAPTURE_LOCATOR = "capture://video";
    
    protected Hashtable properties;
    
    private static Object singletonLock = new Object();
    /**
     * Constructor for the Configuration object
     */
    public Configuration() {
        handlers = new Hashtable();
        processors = new Hashtable();
        mimeTypes = new Hashtable();
        properties = new Hashtable();
    }


    /**
     *  Gets the supportedContentTypes attribute of the Configuration object
     *
     * @param  protocol  Description of the Parameter
     * @return           The supportedContentTypes value
     */
    public abstract String[] getSupportedContentTypes(String protocol);


    /**
     *  Gets the supportedProtocols attribute of the Configuration class
     *
     * @param  content_types  Description of the Parameter
     * @return                The supportedProtocols value
     */
    public abstract String[] getSupportedProtocols(String content_types);

    public abstract String[] getSupportedMediaProcessorInputTypes();

    public abstract String[] getSupportedSoundSource3DPlayerTypes();

    public abstract String getProperty(String key);

    public abstract void setProperty(String key, String value);

    /**
     * Gets the audio renderer.
     *
     * @return The audio renderer
     */
    public abstract PCMAudioOut getAudioRenderer();
    
    /**
     * Gets the video renderer.
     *
     * @return The video renderer
     */
    public abstract VideoRenderer getVideoRenderer(Player player,
                                                   int sourceWidth,
                                                   int sourceHeight);

    /**
     *  Gets the tonePlayer attribute of the Configuration object
     *
     * @return    The tonePlayer value
     */
    public abstract TonePlayer getTonePlayer();
    
    /**
     * Convert from the name of a file to its corresponding Mime
     * type based on the extension.
     *
     * @param  name  Description of the Parameter
     * @return       Description of the Return Value
     */
    public String ext2Mime(String name) {
        int idx = name.lastIndexOf('.');
        String ext;
        if (idx != -1) {
            ext = name.substring(idx + 1).toLowerCase();
        } else {
            ext = name.toLowerCase();
        }
        return (String) mimeTypes.get(ext);
    }


    /**
     *  Gets the handler attribute of the Configuration object
     *
     * @param  type  The content type
     * @return       The handler value
     */
    public String getHandler(String type) {
        return (String) handlers.get(type);
    }

    /**
     *  Gets the processor attribute of the Configuration object
     *
     * @param  type  The content type
     * @return       The processor value
     */
    public String getMediaProcessor(String type) {
        return (String) processors.get(type);
    }

    /**
     *  Gets Accessor to platform specific Image classes
     *  To be defined in derived classes.
     *
     * @return instance of ImageAccess class
     */
    public abstract ImageAccess getImageAccessor();

    /**
     *  Gets the configuration attribute of the Configuration class
     *
     * @return    The configuration value
     */
    public static Configuration getConfiguration() {
        synchronized (singletonLock) {
            if (config != null) return config;

            String className = System.getProperty("mmapi-configuration");

            if (className != null) {        
                try {
                    // ... try and instantiate the configuration class ...
                    Class handlerClass = Class.forName(className);
                    config = (Configuration) handlerClass.newInstance();
                } catch (Exception e) {
                    // return DefaultConfiguration 
                    config = new DefaultConfiguration();
                }
            } else {            
                config = new DefaultConfiguration();
            }

            return config;
        }
    }

    /**
     *  Description of the Method
     *
     * @return    Description of the Return Value
     */
    public static boolean nativeLoopMode() {
        return nativeLooping;
    }
}

