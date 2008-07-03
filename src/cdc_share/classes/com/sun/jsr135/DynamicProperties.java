/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.jsr135;

import com.sun.cdc.config.PropertyProvider;
import java.util.Hashtable;

/**
 * This class provides values for the following dynamic properties:
 * <ul>
 *   <li>supports.mixing</li>
 *   <li>supports.audio.capture</li>
 *   <li>supports.video.capture</li>
 *   <li>supports.recording</li>
 *   <li>audio.encodings</li>
 *   <li>video.encodings</li>
 *   <li>video.snapshot.encodings</li>
 *   <li>streamable.contents</li>
 * </ul>
 */
public class DynamicProperties implements PropertyProvider {

    final String propertySupportsMixing="supports.mixing";
    final String propertySupportsAudioCapture="supports.audio.capture";
    final String propertySupportsVideoCapture="supports.video.capture";
    final String propertySupportsRecording="supports.recording";
    final String propertyAudioEncodings="audio.encodings";
    final String propertyVideoEncodings="video.encodings";
    final String propertyVideoSnapshotEncodings="video.snapshot.encodings";
    final String propertyStreamableContents="streamable.contents";
    
    private static Hashtable properties = new Hashtable();

    /** The only instance of this class. */
    private static DynamicProperties instance = null;

    /**
     * Does not let anyone instantiate this class.
     */
    private DynamicProperties() { }

    /**
     * Returns one and only instance of this class.
     * This method does not need to be synchronized because it will be called
     * only sequentially during isolate initialization.
     *
     * @return <code>DynamicProperties</code> instance
     */
    public static DynamicProperties getInstance() {
        if (instance == null) {
            instance = new DynamicProperties();
        }
        return instance;
    }

    /**
     * Returns current value for the dynamic property corresponding to the
     * given key. This method is called upon retrieval of any of the properties
     * supported by this class.
     *
     * @param key key for the property being retrieved.
     * @param fromCache indicates whether property value should be taken from
     *        internal cache. It can be ignored if properties caching is not
     *        supported by underlying implementation.
     * @return current property value
     */
    public String getValue(String key, boolean fromCache) {
        String val = null;
        if (fromCache) {
            return (String)properties.get(key);
        }
        if (key.equals(propertySupportsMixing)) {
            val = nGetPropertyValueSupportsMixing();
        } else if (key.equals(propertySupportsAudioCapture)) {
            val = nGetPropertyValueSupportsAudioCapture();
        } else if (key.equals(propertySupportsVideoCapture)) {
            val = nGetPropertyValueSupportsVideoCapture();
        } else if (key.equals(propertySupportsRecording)) {
            val = nGetPropertyValueSupportsRecording();
        } else if (key.equals(propertyAudioEncodings)) {
            val = nGetPropertyValueAudioEncodings();
        } else if (key.equals(propertyVideoEncodings)) {
            val = nGetPropertyValueVideoEncodings();
        } else if (key.equals(propertyVideoSnapshotEncodings)) {
            val = nGetPropertyValueVideoSnapshotEncodings();
        } else if (key.equals(propertyStreamableContents)) {
            val = nGetPropertyValueStreamableContents();
        }
        return val;
    }

    /**
     * Returns current value for the dynamic property
     *
     * @return current property value
     */
    private static native String nGetPropertyValueSupportsMixing();
    private static native String nGetPropertyValueSupportsAudioCapture();
    private static native String nGetPropertyValueSupportsVideoCapture();
    private static native String nGetPropertyValueSupportsRecording();
    private static native String nGetPropertyValueAudioEncodings();
    private static native String nGetPropertyValueVideoEncodings();
    private static native String nGetPropertyValueVideoSnapshotEncodings();
    private static native String nGetPropertyValueStreamableContents();
    

    /**
     * Tells underlying implementation to cache values of all the properties
     * corresponding to this particular class. This call can be ignored if
     * property caching is not supported.
     *
     * @return <code>true</code> on success, <code>false</code> otherwise
     */
    public boolean cacheProperties() {
        properties.put(propertySupportsMixing, 
                       nGetPropertyValueSupportsMixing());
        properties.put(propertySupportsAudioCapture,
                       nGetPropertyValueSupportsAudioCapture());
        properties.put(propertySupportsVideoCapture,
                       nGetPropertyValueSupportsVideoCapture());
        properties.put(propertySupportsRecording,
                       nGetPropertyValueSupportsRecording());
        String value = nGetPropertyValueAudioEncodings();
        if (value != null) {
            properties.put(propertyAudioEncodings, value);
        }
        value = nGetPropertyValueVideoEncodings();
        if (value != null) {
            properties.put(propertyVideoEncodings, value);
        }
        value = nGetPropertyValueVideoSnapshotEncodings();
        if (value != null) {
            properties.put(propertyVideoSnapshotEncodings, value);
        }
        value = nGetPropertyValueStreamableContents();
        if (value != null) {
            properties.put(propertyStreamableContents, value);
        }
        return true;
    };

}
