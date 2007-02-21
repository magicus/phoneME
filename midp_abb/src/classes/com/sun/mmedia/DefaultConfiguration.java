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
import java.util.Vector;
import java.util.Enumeration;

import javax.microedition.media.Player;

/**
 * The default configuration module for MIDP.
 *
 * @created    January 13, 2005
 */
public class DefaultConfiguration extends Configuration {
    /**
     * Tone sequence mime type.
     */
    public final static String MIME_AUDIO_TONE = "audio/x-tone-seq";

    /**
     * AMR NB mime type.
     */
    public final static String MIME_AUDIO_AMR = "audio/amr";

    /**
     * GIF mime type.
     */
    public final static String MIME_IMAGE_GIF = "image/gif";

    /**
     * PNG mime type.
     */
    public final static String MIME_IMAGE_PNG = "image/png";

    /**
     * JPEG mime type.
     */
    public final static String MIME_IMAGE_JPEG = "image/jpeg";

    /**
     * Raw image mime type.
     */
    public final static String MIME_IMAGE_RAW = "image/raw";

    /**
     * AMR WB mime type.
     */
    public final static String MIME_AUDIO_AMR_WB = "audio/amr-wb";

    /**
     * WAV mime type.
     */
    public static final String MIME_AUDIO_WAV = "audio/x-wav";

    /**
     * MIDI mime type.
     */
    public static final String MIME_AUDIO_MIDI = "audio/midi";

    /**
     * SP-MIDI mime type.
     */
    public static final String MIME_AUDIO_SP_MIDI = "audio/sp-midi";
    
    /**
     * MPEG mime type.
     */
    public static final String MIME_VIDEO_MPEG = "video/mpeg";
    
    /**
     * MPEG4 mime type.
     */
    public final static String MIME_VIDEO_MPEG4 = "video/mpeg4";

    /**
     * PCMU mime type.
     */
    public static final String MIME_AUDIO_PCMU = "audio/x-pcmu";
    
    /**
     * RGB 565, Sun specific type.
     */
    public static final String RGB565 = "video/vnd.sun.rgb565";


    /**
     *  Handle for the Image Access Environment...
     */
    private static ImageAccess imageAccessor;

    /**
     *  Handle for the Tone Player...
     */
    private static TonePlayer myTonePlayer;
    
    /**
     * defines whether to process jsr234-specific operations or not 
     * (jsr135 only).
     */
    private boolean needAMMS;
    
    /**
     * defines which audio subsystem to use - MAP or QSound.
     */
    private boolean needQSound;


    protected Hashtable supportedProtocols;
    private Hashtable supportedContentTypes;
    
    private Vector nullContentTypes;
    private Vector captureContentTypes;
    private Vector deviceContentTypes;
    private Vector fileContentTypes;
    private Vector httpContentTypes;

    /**
     * Constructor for the DefaultConfiguration object
     */
    public DefaultConfiguration() {
        super();
        supportedProtocols = new Hashtable();
        
        init();
        
        nullContentTypes = new Vector(9);
        captureContentTypes = new Vector(2);
        deviceContentTypes = new Vector(2);
        fileContentTypes = new Vector(8);
        httpContentTypes = new Vector(8);
        
        supportedContentTypes = new Hashtable();
        
        supportedContentTypes.put("capture", captureContentTypes); 
        supportedContentTypes.put("device", deviceContentTypes); 
        supportedContentTypes.put("file", fileContentTypes); 
        supportedContentTypes.put("http", httpContentTypes); 
        
        for (Enumeration keys = supportedProtocols.keys(); keys.hasMoreElements() ;) {
            // one of supported mime-types, added during init()
            String mime = (String)keys.nextElement();
            // array of supported protocols for this mime-type
            // can be a combination of {"capture", "device", "file", "http"}
            String values[] = (String[])supportedProtocols.get(mime);
            for (int i = 0; i < values.length; ++i) {
                //values[i] is one of {"capture", "device", "file", "http"}
                Vector vectorContentTypes = 
                        (Vector)supportedContentTypes.get(values[i]);
                //vectorContentTypes is one of: 
                //{captureContentTypes, deviceContentTypes, fileContentTypes, httpContentTypes}
                //TBD: add "if (vectorContentTypes != null)" to support removal of "file://" URLs
                vectorContentTypes.addElement(mime);
            }
            //this one is a merge of all other *ContentTypes
            nullContentTypes.addElement(mime);
        }
        /*
        System.out.println("DEBUG: Supported properties: " + properties.toString());
        System.out.println("DEBUG: Supported handlers: " + handlers.toString());
        System.out.println("DEBUG: Supported processors: " + processors.toString());
        
        System.out.println("DEBUG: Supported mime-types: " + mimeTypes.toString());
        System.out.println("DEBUG: Supported content-types: " + supportedContentTypes.toString());
        System.out.println("DEBUG: Supported protocols: " + supportedProtocols.toString());
        
        System.out.println("DEBUG: null content-types: " + nullContentTypes.toString());
        System.out.println("DEBUG: capture content-types: " + captureContentTypes.toString());
        System.out.println("DEBUG: device content-types: " + deviceContentTypes.toString());
        System.out.println("DEBUG: file content-types: " + fileContentTypes.toString());
        System.out.println("DEBUG: http content-types: " + httpContentTypes.toString());
         */
    }
    
    /** 
     * method that performs real initialization.
     * Called only from constructor.
     * Must be overwritted by derived classes 
     */
    protected void init() {
        System.err.println("Failed to find configuration object - DefaultConfiguration is used!");
        
        try {
            String value = System.getProperty("microedition.amms.version");
            needAMMS = (value != null);
        } catch (Exception e) {
            needAMMS = false;
        }
        try {
            String value = System.getProperty("supports.mediacapabilities");
            needQSound = (value.indexOf("audio3d") != -1);
        } catch (Exception e) {
            needQSound = false;
        }

        // Protocol handlers.
        handlers.put("http", "com.sun.mmedia.protocol.CommonDS");
        handlers.put("file", "com.sun.mmedia.protocol.CommonDS");
        handlers.put("capture", "com.sun.mmedia.protocol.WavCapture");

        // Device handlers.
        if (needQSound) {
            handlers.put(TONE_DEVICE_LOCATOR, "com.sun.mmedia.QSoundToneSequencePlayer");
            handlers.put(MIDI_DEVICE_LOCATOR, "com.sun.mmedia.QSoundMIDIPlayer");
        } else {
            handlers.put(TONE_DEVICE_LOCATOR, "com.sun.mmedia.ToneSequencePlayer");
            handlers.put(MIDI_DEVICE_LOCATOR, "com.sun.mmedia.MIDIPlayer");
        }
        
        if (needAMMS) {
            handlers.put(RADIO_CAPTURE_LOCATOR, "com.sun.amms.AmmsTuner");
            handlers.put(VIDEO_CAPTURE_LOCATOR, "com.sun.amms.AmmsCameraPlayer");
        } else {
            handlers.put(RADIO_CAPTURE_LOCATOR, "com.sun.mmedia.MmapiTuner");
            handlers.put(VIDEO_CAPTURE_LOCATOR, "com.sun.mmedia.MmapiCameraPlayer");
        }
        
        // Content handlers.
        
        // RGB565 content ... - internal image capture format
        supportedProtocols.put(RGB565, cProtocols);
                
        // AMR content ... - disabled, can't determine implementation to use
        /*
        // this one is licensed full functional AMR player
        //handlers.put(MIME_AUDIO_AMR, "com.sun.mmedia.AMRPlayer");
        //handlers.put(MIME_AUDIO_AMR_WB, "com.sun.mmedia.AMRPlayer");
         *
        //this one is emulation player provided by Toolkit+QuickTime
        //handlers.put(MIME_AUDIO_AMR, "com.sun.mmedia.AMR3DConnectablePlayer2");
        //QuickTime doesn't support amr-wb: handlers.put(MIME_AUDIO_AMR_WB, "");
         *
        //supportedProtocols.put(MIME_AUDIO_AMR, hfProtocols);
        ////TO PASS MMAPI-TCK 1.1: supportedProtocols.put(MIME_AUDIO_AMR_WB, hfProtocols);
        */
        // MIDI/Tone Sequence content ...
        if (needQSound) {
            handlers.put(MIME_AUDIO_TONE, "com.sun.mmedia.QSoundToneSequencePlayer");
            handlers.put(MIME_AUDIO_MIDI, "com.sun.mmedia.QSoundMIDIPlayer");
            handlers.put(MIME_AUDIO_SP_MIDI, "com.sun.mmedia.QSoundMIDIPlayer");
        } else {
            handlers.put(MIME_AUDIO_TONE, "com.sun.mmedia.ToneSequencePlayer");
            handlers.put(MIME_AUDIO_MIDI, "com.sun.mmedia.MIDIPlayer");
            handlers.put(MIME_AUDIO_SP_MIDI, "com.sun.mmedia.MIDIPlayer");
        }
        supportedProtocols.put(MIME_AUDIO_TONE, hfdProtocols);
        supportedProtocols.put(MIME_AUDIO_MIDI, hfdProtocols);
        supportedProtocols.put(MIME_AUDIO_SP_MIDI, hfProtocols);
        
        // Other multimedia content ...
        handlers.put(MIME_IMAGE_GIF, "com.sun.mmedia.GIFPlayer");
        handlers.put(MIME_AUDIO_WAV, "com.sun.mmedia.WavPlayer");
        handlers.put(MIME_VIDEO_MPEG, "com.sun.mmedia.JavaMPEG1Player2");
        
        supportedProtocols.put(MIME_IMAGE_GIF, hfProtocols);
        supportedProtocols.put(MIME_AUDIO_WAV, hfcProtocols);
        supportedProtocols.put(MIME_VIDEO_MPEG, hfProtocols);

        if (needAMMS) {
            // Media Processors.
            processors.put(MIME_IMAGE_RAW, "com.sun.amms.RAWImageProcessor");
            processors.put(MIME_IMAGE_JPEG, "com.sun.amms.JPEGImageProcessor");
            processors.put(MIME_IMAGE_PNG, "com.sun.amms.PNGImageProcessor");
        }
        
        // define MIDI Tone Player class
        if (needQSound) {
            setProperty("com.sun.mmedia.TonePlayer", "com.sun.mmedia.QSoundTonePlayer");
        } else {
            setProperty("com.sun.mmedia.TonePlayer", "com.sun.mmedia.MIDIOut");
        }
        
       // define audio renderer class
        if (needQSound) {
            setProperty("com.sun.mmedia.PCMAudioOut", "com.sun.mmedia.QSoundPCMOut");
        } else {
            setProperty("com.sun.mmedia.PCMAudioOut", "com.sun.mmedia.MapPCMOut");
        }
        
        if (needAMMS) {
            // define class that implement AMMS GlobalManager object
            if (needQSound)
                setProperty("javax.microedition.amms.GlobalManager", "com.sun.mmedia.QSoundGlobalManager");
            else
                setProperty("javax.microedition.amms.GlobalManager", "com.sun.mmedia.MapGlobalManager");
        }

        // Mime types
        mimeTypes.put("jts", MIME_AUDIO_TONE);
        mimeTypes.put("amr", MIME_AUDIO_AMR);
        mimeTypes.put("awb", MIME_AUDIO_AMR_WB);
        mimeTypes.put("gif", MIME_IMAGE_GIF);
        mimeTypes.put("wav", MIME_AUDIO_WAV);
        mimeTypes.put("mpg", MIME_VIDEO_MPEG);
        mimeTypes.put("mpeg", MIME_VIDEO_MPEG);
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
        mimeTypes.put("video/x-jpeg", MIME_IMAGE_JPEG);

        initTunerProperties();

        /**
         * will return NULL until issue with "classByName" creatio 
         * of MIDPImageAccessor will be fixed.
         * MIDPImageAccessor will be returneed by derived classes ...
         */
        // Create ImageAccessor ("ImageRenderer")
        //imageAccessor = new MIDPImageAccessor();
        imageAccessor = null;
        
        // Create a Tone Player...
        myTonePlayer = (TonePlayer)createInstanceOf("com.sun.mmedia.TonePlayer");
    }

    /** 
     * sets group of tuner properties. 
     * Depending on real config, propery values may have different prefixes. 
     *
     * Should not be overwritten by derived classes
     */
    protected void initTunerProperties() {
        // define Tuner radio channel emulation data
        setProperty("com.sun.tuner.presets", "5"); // number of presets (same array for FM & AM)
        setProperty("com.sun.tuner.play", "2"); // default preset to play on startup

        setProperty("com.sun.tuner.fm.stations", "3"); //number of FM stations

        setProperty("com.sun.tuner.fm.station_1", "885000");
        setProperty("com.sun.tuner.fm.name_1", "KCQW");
        setProperty("com.sun.tuner.fm.text_1", "Hour of Opera");
        setProperty("com.sun.tuner.fm.url_1", "tuner1.wav");
        //setProperty("com.sun.tuner.fm.preset_1", "4");

        setProperty("com.sun.tuner.fm.station_2", "932000");
        setProperty("com.sun.tuner.fm.name_2", "Rock 932");
        setProperty("com.sun.tuner.fm.text_2", "Drive Rock");
        setProperty("com.sun.tuner.fm.url_2", "tuner2.wav");
        setProperty("com.sun.tuner.fm.preset_2", "3");

        setProperty("com.sun.tuner.fm.station_3", "1015000");
        setProperty("com.sun.tuner.fm.name_3", "NNNERD");
        setProperty("com.sun.tuner.fm.text_3", "Module Prime");
        setProperty("com.sun.tuner.fm.url_3", "tuner4.wav");
        setProperty("com.sun.tuner.fm.preset_3", "2");

        setProperty("com.sun.tuner.am.stations", "2"); //number of AM stations

        setProperty("com.sun.tuner.am.station_1", "5800");
        setProperty("com.sun.tuner.am.name_1", "FUNR");
        setProperty("com.sun.tuner.am.text_1", "Funny News");
        setProperty("com.sun.tuner.am.url_1", "tuner3.wav");
        setProperty("com.sun.tuner.am.preset_1", "4");

        setProperty("com.sun.tuner.am.station_2", "12110");
        setProperty("com.sun.tuner.am.name_2", "TLKB");
        setProperty("com.sun.tuner.am.text_2", "Talk Back");
        setProperty("com.sun.tuner.am.url_2", "tuner5.wav");
        setProperty("com.sun.tuner.am.preset_2", "1");
    }

    /**
     * Gets the supportedContentTypes attribute of the DefaultConfiguration object
     *
     * @param  protocol  Description of the Parameter
     * @return           The supportedContentTypes value
     */
    public String[] getSupportedContentTypes(String protocol) {
        Vector vectorContentTypes = (protocol == null)
            ? (Vector)nullContentTypes
            : (Vector)supportedContentTypes.get(protocol);
        
        if (vectorContentTypes == null) {
            return new String[0];
        } else {
            String[] array = new String[vectorContentTypes.size()];
            vectorContentTypes.copyInto(array);
            return array;
        }
    }

    protected final static String hfdcProtocols[] = { "http", "file", "device", "capture" };
    
    protected final static String hfcProtocols[] = { "http", "file", "capture" };
    
    protected final static String hfdProtocols[] = { "http", "file", "device" };
        
    protected final static String hfProtocols[] = { "http", "file" };
    
    protected final static String cProtocols[] = { "capture" };
    
    protected final static String dProtocols[] = { "device" };
    
    /*
     * Shall be used if "file://" is not available:
     *
    protected final static String hdcProtocols[] = { "http", "device", "capture" };
    
    protected final static String hcProtocols[] = { "http", "capture" };
    
    protected final static String hdProtocols[] = { "http", "device" };
        
    protected final static String hProtocols[] = { "http" };
     */
    /**
     *  Gets the supportedProtocols attribute of the DefaultConfiguration object
     *
     * @param  content_type  Description of the Parameter
     * @return               The supportedProtocols value
     */
    public String[] getSupportedProtocols(String content_type) {
        String[] array = (content_type == null)
            ? hfdcProtocols
            : (String[])supportedProtocols.get(content_type);
        return (array == null) ? new String[0] : array;
    }

    /**
     * Gets the audio renderer.
     *
     * @return The audio renderer
     */
    public PCMAudioOut getAudioRenderer() {
        return (PCMAudioOut)createInstanceOf("com.sun.mmedia.PCMAudioOut");
    }

    /**
     * Gets the video renderer.
     *
     * @return The video renderer
     */
    public VideoRenderer getVideoRenderer(Player player, 
                                          int sourceWidth, 
                                          int sourceHeight) {
        /**
         * will return NULL until issue with "classByName" creatio 
         * of VideoRenderer/MIDPVideoRenderer will be fixed.
         * MIDPVideoRenderer will be returneed by derived classes ...
         */
        //return new MIDPVideoRenderer(player, sourceWidth, sourceHeight);
        return null;
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
    
    protected final static String supportedMPInputTypes[] = { 
        MIME_IMAGE_PNG, MIME_IMAGE_JPEG, MIME_IMAGE_RAW
    };
            
    public String[] getSupportedMediaProcessorInputTypes() {
        return (needAMMS)
            ? supportedMPInputTypes
            : new String[0];
    }

    protected final static String supportedSS3DPlayerTypes[] = {
        MIME_AUDIO_WAV, 
        //MIME_AUDIO_AMR, //MIME_AUDIO_AMR_WB, 
        MIME_AUDIO_MIDI, MIME_AUDIO_SP_MIDI, 
        MIME_AUDIO_TONE 
    };
            
    public String[] getSupportedSoundSource3DPlayerTypes() {
        return (needAMMS && needQSound)
            ? supportedSS3DPlayerTypes
            : new String[0];
    }

    public String getProperty(String key) {
        return (String) properties.get(key);
    }

    public void setProperty(String key, String value) {
        properties.put(key, value);
    }       
    
    private Object createInstanceOf(String propertyName) {
        try {
            String propertyValue = getProperty(propertyName);
            Class propertyClass = Class.forName(propertyValue);
            Object propertyInstance = propertyClass.newInstance();
            return propertyInstance;
        } catch (Exception e) {
            return null;
        }
    }
}
