/*
 * 
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.mmedia.protocol;

/**
 * Parses a given locator or encoding string into its constituent properties.
 * Breaks it up into 3 main parts : the protocol, the device and the parameter
 * list of the form protocol://device?param1=value1&param2=value2...
 */
public final class LocatorParser {

    /**
     * Constructs a parser object and initializes its state.
     * @param s The string to be parsed
     */
    public LocatorParser(String s) {
	length = s.length();
	locator = s;
    }

    /**
     * Parses and returns the protocol part of the locator, if any.
     * @return The protocol string, or null if there isn't a protocol part
     */
    public String getProtocol() {
	int colonIndex = locator.indexOf("://");
	int oldIndex = index;
	if (colonIndex < 1)
	    return null;
	index = colonIndex + 3;
	return locator.substring(oldIndex, colonIndex);
    }

    /**
     * Parses and returns the device name part of the locator, if any.
     * @return The device name string, or null if there isn't one.
     */
    public String getDevice() {
	if (index >= length)
	    return null;
	int oldIndex = index;
	// Get the index of the question mark
	int queryIndex = locator.indexOf("?", index);
	// If its not found, then the whole string is assumed to be the device
	if (queryIndex < 0) {
	    index = length;
	    return locator.substring(oldIndex);
	} else if (queryIndex == 0) {
	    return null;
	}
	// Otherwise move the index forward past the ?
	index = queryIndex + 1;
	// return the device name portion
	return locator.substring(oldIndex, queryIndex);
    }

    /**
     * Parses and returns the next parameter key in a parameter
     * "key=value" pair.
     * @return The key part of the parameter "key=value" pair
     */
    public String getParameter() {
	if (index >= length)
	    return null;
	int ampIndex = locator.indexOf("&", index);
	// Assume last parameter
	if (ampIndex < 0)
	    ampIndex = length;

	// token = "abc=xyz"
	String token = locator.substring(index, ampIndex);

	int eq = token.indexOf("=");
	// If there's no = or nothing before the = or nothing after the =
	if (eq < 1 || eq == token.length() - 1)
	    return null;
	String key = token.substring(0, eq);
	// What's after the = is the value. Store it for later query thru
	// the getValue() method
	value = token.substring(eq + 1);
	index = ampIndex + 1;
	return key;
    }

    /**
     * Returns the value corresponding to the most recently parsed parameter
     * @return The value part of the "key=value" parameter pair.
     */
    public String getValue() {
	return value;
    }

    /**
     * Checks if there are any more characters in the string that haven't been
     * parsed yet.
     * @return true if there are more characters to be parsed, false otherwise
     */
    public boolean hasMore() {
	return index < length;
    }

    public static final String S_PROTOCOL_CAPTURE  = "capture";
    //public static final String S_PROTOCOL_HTTP     = "http";
    //public static final String S_PROTOCOL_RTP      = "rtp";

    public static final String S_DEVICE_AUDIO    = "audio";
    public static final String S_DEVICE_VIDEO    = "video";
    //public static final String S_DEVICE_AV       = "audio_video";
    public static final String S_DEVICE_RADIO    = "radio";

    //AUDIO,VIDEO,IMAGE encoding parameter keys and values
    public static final String S_ENCODING      = "encoding";
    public static final String S_ENCODING_JPEG   = "jpeg";
    public static final String S_ENCODING_PNG    = "png";
    public static final String S_ENCODING_PCM    = "pcm";
    
    public static final String S_TYPE          = "type";
    public static final String S_TYPE_JFIF       = "jfif";
    //public static final String S_TYPE_EXIF       = "exif";
    
    public static final String S_WIDTH         = "width";
    
    public static final String S_HEIGHT        = "height";
    
    public static final String S_QUALITY       = "quality";
    
    public static final String S_PROGRESSIVE   = "progressive";
    
    public static final String S_INTERLACED    = "interlaced";
    
    public static final String S_FPS           = "fps";
    
    public static final String S_COLORS        = "colors";
    
    public static final String S_JPEG          = "jpeg";

    public static final String S_PNG           = "png";
    
    public static final String S_JFIF          = "jfif";
    
    public static final String S_TRUE          = "true";
    
    public static final String S_FALSE         = "false";

    public static final String S_AUDIO         = "audio";

    public static final String S_VIDEO         = "video";

    public static final String S_RATE          = "rate";
    public static final String S_BITS          = "bits";
    public static final String S_CHANNELS      = "channels";
    public static final String S_ENDIAN        = "endian";
    public static final String S_ENDIAN_BIG      = "big";
    public static final String S_ENDIAN_LITTLE   = "little";

    public static final String S_SIGN            = "signed";
    public static final String S_SIGN_SIGNED     = "signed";
    public static final String S_SIGN_UNSIGNED   = "unsigned";

    //RADIO encoding parameter keys and values
    public static final String S_RADIO_FREQ      = "f";
    
    public static final String S_RADIO_MOD       = "mod";
    public static final String S_RADIO_MOD_FM    = "fm";
    public static final String S_RADIO_MOD_AM    = "am";

    public static final String S_RADIO_ST        = "st";
    public static final String S_RADIO_ST_MONO   = "mono";
    public static final String S_RADIO_ST_STEREO = "stereo";
    public static final String S_RADIO_ST_AUTO   = "auto";
    
    public static final String S_RADIO_PRGID     = "id";
    
    public static final String S_RADIO_PRESET    = "preset";
    
    /** The locator to be parsed */
    private String locator;

    /** The current pointer into the locator where parsing is to continue */
    private int index;

    /** The length of the locator */
    private int length;

    /** The latest value in a "key=value" pair */
    private String value;





    
// Usage:
//     public static void main(String [] args) {
// 	LocatorParser lp = new LocatorParser(args[0]);
// 	System.err.println("Protocol = " + lp.getProtocol());
// 	System.err.println("Device = " + lp.getDevice());
// 	while (lp.hasMore()) {
// 	    System.err.println(lp.getParameter() + " = " + lp.getValue());
// 	}
//     }


}
