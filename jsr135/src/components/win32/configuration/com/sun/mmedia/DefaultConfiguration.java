/*
 *
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

import javax.microedition.media.Manager;

/**
 *  The default configuration module for an MMAPI.
 *  implementation supporting the http and
 *  device protocols as well as the following
 *  content types:
 *
 *      Tones,
 *      Tone Sequences,
 *      AMR Narrowband
 *      AMR Wideband.
 *      GIF.
 *
 *  Please note:
 *
 *  Tone Sequences are supported over http in tone sequence
 *  file format (.jts).
 *
 *  Both single tones and tone sequence are also supported
 *  over the device protocol.
 *
 */
public class DefaultConfiguration extends Configuration {
    /**
     * Tone sequence mime type.
     */
    public final static String TONE = "audio/x-tone-seq";

    /**
     * WAV mime type.
     */
    public final static String WAV = "audio/x-wav";

    /**
     * AMR mime type
     */
    public final static String AMR = "audio/amr";

    /**
     * MIDI mime type.
     */
    public final static String MIDI = "audio/midi";

    /**
     * MP3 mime type
     */
    public final static String MP3 = "audio/mpeg";

    /**
     * 3GPP video mime type
     */
    public final static String V3GPP = "video/3gpp";

    /**
     * MPEG4 video mime type
     */
    public final static String MPEG4 = "video/mp4v-es";

    /**
     * Video capture locator
     */
    public final static String VIDEO_CAPTURE_LOCATOR = "capture://video";

    /**
     *Constructor for the DefaultConfiguration object
     */
    public DefaultConfiguration() {
        super();

        // Protocol handlers.
        handlers.put("http", "com.sun.mmedia.protocol.CommonDS");
        
        // Content handlers.

        handlers.put(TONE, "com.sun.mmedia.DirectTone");
        handlers.put(WAV, "com.sun.mmedia.DirectPlayer");
        handlers.put(MIDI, "com.sun.mmedia.DirectPlayer");
        handlers.put(MP3, "com.sun.mmedia.DirectPlayer");
        handlers.put(Manager.TONE_DEVICE_LOCATOR, "com.sun.mmedia.DirectTone");
        handlers.put(VIDEO_CAPTURE_LOCATOR, "com.sun.mmedia.DirectCamera");

        // Mime types
        mimeTypes.put("jts", TONE);
        mimeTypes.put("mid", MIDI);
        mimeTypes.put("midi", MIDI);
        mimeTypes.put("kar", MIDI);
        mimeTypes.put("wav", WAV);
        mimeTypes.put("mp3", MP3);

        // for converting
        mimeTypes.put("audio/tone", TONE);
    }


    /**
     *  Gets the supportedContentTypes attribute of the DefaultConfiguration object
     *
     * @param  protocol  Description of the Parameter
     * @return           The supportedContentTypes value
     */
    public String[] getSupportedContentTypes(String protocol) {
        if (protocol == null) {
            return new String[]{
            // Mandatory support
                TONE,
            // Optional formats
                MIDI,
                WAV,
                MP3,
                };
        }

        if (protocol.equals("device")) {
            return new String[]{
            // Mandatory support
                TONE,
            // Optional formats
                };
        }

        if (protocol.equals("http")) {
            return new String[]{
                TONE,
                MIDI,
                WAV,
                MP3,
                };
        }

        return new String[0];
    }


    /**
     *  Gets the supportedProtocols attribute of the DefaultConfiguration object
     *
     * @param  content_type  Description of the Parameter
     * @return               The supportedProtocols value
     */
    public String[] getSupportedProtocols(String content_type) {
        System.out.println("in config.getSupportedProtocols: " + content_type);

        if (content_type == null) {
            return new String[]{
                "device",
                "http",
                };
        }

        if (content_type.equals(TONE)) {
            return new String[]{
                "device",
                "http",
                };
        }

        if (content_type.equals(MIDI)) {
            return new String[]{
                "http",
                };
        }

        if (content_type.equals(WAV)) {
            return new String[]{
                "http",
                };
        }

        if (content_type.equals(MP3)) {
            return new String[]{
                "http",
                };
        }

        return new String[0];
    }

    /**
     *  Gets the videoRenderer attribute of the Configuration class
     *
     * @return    The videoRenderer value
     */
    public VideoRenderer getVideoRenderer() {
        return null;
    }

    /**
     *  Gets the audioRenderer attribute of the Configuration class
     *
     * @return    The audioRenderer value
     */
    public AudioRenderer getAudioRenderer() {
        return null;
    }

    /**
     *  Gets the tonePlayer attribute of the DefaultConfiguration object
     *
     * @return    The tonePlayer value
     */
    public TonePlayer getTonePlayer() {
        return new NativeTonePlayer();
    }
   
}

