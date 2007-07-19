/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia.rtsp.sdp;

import java.io.*;
import java.util.*;

public class TimeDescription extends Parser {
    // Values:
    public String timeActive;
    public Vector repeatTimes;

    public TimeDescription(ByteArrayInputStream bin) {
        // Time the session is active:
        timeActive = getLine(bin);

        // Repeat Times:
        repeatTimes = new Vector();

	String tag= getTag( bin);

	while( tag != null && tag.length() > 0) {
	    if( tag.equals( "r=")) {	
                String repeatTime = getLine(bin);

                repeatTimes.addElement(repeatTime);
	    } else {
		ungetTag( tag);
		return;
	    }

	    tag= getTag( bin);
        }
    }
}
