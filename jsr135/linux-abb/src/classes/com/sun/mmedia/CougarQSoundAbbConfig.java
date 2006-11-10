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

import javax.microedition.media.Player;

/**
 * The configuration module for MIDP with QSound audio engine for MIDP subset of JSR135.
 *
 * @created    April 05, 2006
 */
public class CougarQSoundAbbConfig extends DefaultConfiguration {

    /**
     *  Handle for the Tone Player...
     */
    private static TonePlayer myTonePlayer;
    
    /**
     * Constructor for the Configuration object
     */
    public CougarQSoundAbbConfig() {
        super();
    }
    
    /** 
     * method that performs real initialization.
     * Called only from constructor.
     * Must be overwritten by derived classes 
     */
    protected void init() {
        // Protocol handlers.

        // Device handlers.
        handlers.put(TONE_DEVICE_LOCATOR, "com.sun.mmedia.QSoundABBToneSequencePlayer");
        
        // Content handlers.
        
        // MIDI/Tone Sequence content ...
        handlers.put(MIME_AUDIO_TONE, "com.sun.mmedia.QSoundABBToneSequencePlayer");
        
        supportedProtocols.put(MIME_AUDIO_TONE, dProtocols);
        
        // Mime types
        mimeTypes.put("jts", MIME_AUDIO_TONE);

        // for converting
        mimeTypes.put("audio/tone", MIME_AUDIO_TONE);
        
        // Create a Tone Player...
        myTonePlayer = new QSoundTonePlayer();
        
        /*
         * The peer value is actually not used. 
         * The only purpose of this call is to request loading 
         * if QSoundHiddenManager class with invokation of 
         * QSoundHiddenManager clas static initializer. 
         * As the result QSound audio system will be initialized for MMAPI. 
         */
        int peer = QSoundHiddenManager.getGlobalPeer();
    }

    /**
     *  Gets the supportedProtocols attribute of the DefaultConfiguration object
     *
     * @param  content_type  Description of the Parameter
     * @return               The supportedProtocols value
     */
    public String[] getSupportedProtocols(String content_type) {
        return (content_type == null)
            ? dProtocols
            : super.getSupportedProtocols(content_type);
    }

    /**
     * Gets the audio renderer.
     *
     * @return The audio renderer
     */
    public PCMAudioOut getAudioRenderer() {
        return null;
    }

    /**
     * Gets the video renderer.
     *
     * @return The video renderer
     */
    public VideoRenderer getVideoRenderer(Player player, 
                                          int sourceWidth, 
                                          int sourceHeight) {
        return null;
    }

    /**
     * Gets the image accessor.
     *
     * @return The image accessor
     */
    public ImageAccess getImageAccessor() {
        return null;
    }

    /**
     *  Gets the tonePlayer attribute of the DefaultConfiguration object
     *
     * @return    The tonePlayer value
     */
    public TonePlayer getTonePlayer() {
        return myTonePlayer;
    }

    public String[] getSupportedMediaProcessorInputTypes() {
        return new String[0];
    }

    public String[] getSupportedSoundSource3DPlayerTypes() {
        return new String[0];
    }
}
