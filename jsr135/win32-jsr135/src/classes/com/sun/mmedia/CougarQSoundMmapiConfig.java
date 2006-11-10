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
 * The configuration module for MIDP with QSound audio engine for JSR135.
 *
 * @created    February 27, 2006
 */
public class CougarQSoundMmapiConfig extends DefaultConfiguration {
    /**
     *  Handle for the Image Access Environment...
     */
    private static ImageAccess imageAccessor;

    /**
     *  Handle for the Tone Player...
     */
    private static TonePlayer myTonePlayer;
    
    /**
     * Constructor for the Configuration object
     */
    public CougarQSoundMmapiConfig() {
        super();
    }
    
    /** 
     * method that performs real initialization.
     * Called only from constructor.
     * Must be overwritten by derived classes 
     */
    protected void init() {
        // Protocol handlers.
        handlers.put("http", "com.sun.mmedia.protocol.CommonDS");
        handlers.put("file", "com.sun.mmedia.protocol.CommonDS");
        handlers.put("capture", "com.sun.mmedia.protocol.WavCapture");

        // Device handlers.
        handlers.put(TONE_DEVICE_LOCATOR, "com.sun.mmedia.QSoundToneSequencePlayer");
        handlers.put(MIDI_DEVICE_LOCATOR, "com.sun.mmedia.QSoundMIDIPlayer");
        
        // Content handlers.
        
        // RGB565 content ... - internal image capture format
        //supportedProtocols.put(RGB565, cProtocols);
                
        // AMR content ... 
        // this one is licensed full functional AMR player
        handlers.put(MIME_AUDIO_AMR, "com.sun.mmedia.AMRPlayer");
        handlers.put(MIME_AUDIO_AMR_WB, "com.sun.mmedia.AMRPlayer");
        supportedProtocols.put(MIME_AUDIO_AMR, hfProtocols);
        //TO PASS MMAPI-TCK 1.1: supportedProtocols.put(MIME_AUDIO_AMR_WB, hfProtocols);
        
        // MIDI/Tone Sequence content ...
        handlers.put(MIME_AUDIO_TONE, "com.sun.mmedia.QSoundToneSequencePlayer");
        handlers.put(MIME_AUDIO_MIDI, "com.sun.mmedia.QSoundMIDIPlayer");
        handlers.put(MIME_AUDIO_SP_MIDI, "com.sun.mmedia.QSoundMIDIPlayer");
        
        supportedProtocols.put(MIME_AUDIO_TONE, hfdProtocols);
        supportedProtocols.put(MIME_AUDIO_MIDI, hfdProtocols);
        supportedProtocols.put(MIME_AUDIO_SP_MIDI, hfProtocols);
        
        // Other multimedia content ...
        handlers.put(MIME_IMAGE_GIF, "com.sun.mmedia.GIFPlayer");
        handlers.put(MIME_AUDIO_WAV, "com.sun.mmedia.WavPlayer");
        
        supportedProtocols.put(MIME_IMAGE_GIF, hfProtocols);
        supportedProtocols.put(MIME_AUDIO_WAV, hfcProtocols);

        // Mime types
        mimeTypes.put("jts", MIME_AUDIO_TONE);
        mimeTypes.put("amr", MIME_AUDIO_AMR);
        mimeTypes.put("awb", MIME_AUDIO_AMR_WB);
        mimeTypes.put("gif", MIME_IMAGE_GIF);
        mimeTypes.put("wav", MIME_AUDIO_WAV);
        //mimeTypes.put("mpg", MIME_VIDEO_MPEG);
        //mimeTypes.put("mpeg", MIME_VIDEO_MPEG);
        mimeTypes.put("mid", MIME_AUDIO_MIDI);
        mimeTypes.put("midi", MIME_AUDIO_SP_MIDI);

        // for converting
        mimeTypes.put("audio/tone", MIME_AUDIO_TONE);
        mimeTypes.put("audio/wav", MIME_AUDIO_WAV);
        mimeTypes.put("audio/x-wav", MIME_AUDIO_WAV); // do we need this duplication ?
        mimeTypes.put("audio/x-midi", MIME_AUDIO_MIDI);
        mimeTypes.put("audio/amr", MIME_AUDIO_AMR); // do we need this duplication ?
        mimeTypes.put("audio/amr-wb", MIME_AUDIO_AMR_WB); // do we need this duplication ?
        mimeTypes.put("audio/sp-midi", MIME_AUDIO_SP_MIDI); // do we need this duplication ?
        mimeTypes.put("audio/x-pcmu", MIME_AUDIO_PCMU); // do we need this duplication ?
        //mimeTypes.put("video/x-jpeg", MIME_IMAGE_JPEG);

        // Create ImageAccessor ("ImageRenderer")
        imageAccessor = new MIDPImageAccessor();
        
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
     * Gets the audio renderer.
     *
     * @return The audio renderer
     */
    public PCMAudioOut getAudioRenderer() {
        return (PCMAudioOut)new QSoundPCMOut();
    }

    /**
     * Gets the video renderer.
     *
     * @return The video renderer
     */
    public VideoRenderer getVideoRenderer(Player player, 
                                          int sourceWidth, 
                                          int sourceHeight) {
        return new MIDPVideoRenderer(player, sourceWidth, sourceHeight);
    }

    /**
     * Gets the image accessor.
     *
     * @return The image accessor
     */
    public ImageAccess getImageAccessor() {
        return imageAccessor;
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
