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
package com.sun.mmedia.rtsp.sdp;

import java.io.*;
import java.util.*;

public class SessionDescription extends Parser {
    public Vector sessionAttributes;
    public Vector timeDescriptions;
    public Vector mediaDescriptions;
    public boolean connectionIncluded;

    // Values:
    public String version;
    public String origin;
    public String sessionName;
    public String sessionInfo;
    public String uri;
    public String email;
    public String phone;
    public String connectionInfo;
    public String bandwidthInfo;
    public String timezoneAdjustment;
    public String encryptionKey;


    public SessionDescription(ByteArrayInputStream bin) {
        // Protocol Version:
        version = getLine(bin);

        connectionIncluded = false;

	timeDescriptions = new Vector();
        sessionAttributes = new Vector();
	mediaDescriptions = new Vector();
	
	String tag= getTag( bin);

	while( tag != null && tag.length() > 0) {
	    if( tag.equals( "o=")) {
                origin = getLine(bin);
                debug( "origin: " + origin);
	    } else if( tag.equals( "s=")) {
                sessionName = getLine(bin);
                debug( "session name: " + sessionName);
	    } else if( tag.equals( "i=")) {
                sessionInfo = getLine(bin);
		debug( "session info: " + sessionInfo);
	    } else if( tag.equals( "u=")) {
                uri = getLine(bin);
                debug( "uri: " + uri);
	    } else if( tag.equals( "e=")) {
                email = getLine(bin);
                debug( "email: " + email);
	    } else if( tag.equals( "p=")) {	    
                phone = getLine(bin);
                debug( "phone: " + phone);
	    } else if( tag.equals( "c=")) {	 	    
                connectionIncluded = true;

                connectionInfo = getLine(bin);
                debug( "connection info: " + connectionInfo);
	    } else if( tag.equals( "b=")) {		    
                bandwidthInfo = getLine(bin);
                debug("bandwidth info: " + bandwidthInfo);	    
	    } else if( tag.equals( "t=")) {
		TimeDescription timeDescription= new TimeDescription( bin);

		timeDescriptions.addElement( timeDescription);
	    } else if( tag.equals( "z=")) {
                timezoneAdjustment = getLine(bin);
                debug( "timezone adjustment: " + timezoneAdjustment);
	    } else if( tag.equals( "k=")) {
                encryptionKey = getLine(bin);
                debug( "encryption key: " + encryptionKey);		
	    } else if( tag.equals( "a=")) {
                String sessionAttribute = getLine(bin);
                debug( "session attribute: " + sessionAttribute);

                int index = sessionAttribute.indexOf(':');

                if (index > 0) {
                    String name = sessionAttribute.substring(0, index);
                    String value = sessionAttribute.substring(index + 1);

                    MediaAttribute attribute = new MediaAttribute(name, value);

                    sessionAttributes.addElement(attribute);
		}
	    } else if( tag.equals( "m=")) {
		MediaDescription mediaDescription= new MediaDescription( bin, connectionIncluded);

		mediaDescriptions.addElement( mediaDescription);
	    }
	    
	    tag= getTag( bin);
	}
    }

    public MediaAttribute getSessionAttribute(String name) {
        MediaAttribute attribute = null;

        if (sessionAttributes != null) {
            for (int i = 0; i < sessionAttributes.size(); i++) {
                MediaAttribute entry =
                        (MediaAttribute) sessionAttributes.elementAt(i);

                if (entry.getName().equals(name)) {
                    attribute = entry;
                    break;
                }
            }
        }

        return attribute;
    }

    public MediaDescription getMediaDescription(String name) {
        MediaDescription description = null;

        if (mediaDescriptions != null) {
            for (int i = 0; i < mediaDescriptions.size(); i++) {
                MediaDescription entry =
                        (MediaDescription) mediaDescriptions.elementAt(i);

                if (entry.name.equals(name)) {
                    description = entry;
                    break;
                }
            }
        }

        return description;	
    }

    public Vector getMediaDescriptions() {
        return mediaDescriptions;
    }    
}

