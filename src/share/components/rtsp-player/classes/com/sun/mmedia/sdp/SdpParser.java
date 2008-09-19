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

package com.sun.mmedia.sdp;

import java.io.*;
import java.util.*;

public class SdpParser extends Parser {

    private SessionDescription sessionDescription = null;

    public SdpParser( byte data[] ) {
        ByteArrayInputStream bin = new ByteArrayInputStream( data );
        parseData( bin );
    }

    public SdpParser( byte data[], int offs, int len ) {
        ByteArrayInputStream bin = new ByteArrayInputStream( data, offs, len );
        parseData( bin );
    }

    public void parseData( ByteArrayInputStream bin ) {
        if( getTag( bin ).equals( "v=" ) ) {
            try {
                sessionDescription = new SessionDescription( bin );
            } catch ( Exception e ) {
                e.printStackTrace();
            }
        }
    }

    public MediaAttribute getSessionAttribute( String name ) {
        if( null != sessionDescription ) {
            return sessionDescription.getSessionAttribute( name );
        } else {
            return null;
        }
    }

    public MediaDescription getMediaDescription( String name ) {
        if( null != sessionDescription ) {
            return sessionDescription.getMediaDescription( name );
        } else {
            return null;
        }
    }

    public Vector getMediaDescriptions() {
        if( null != sessionDescription ) {
            return sessionDescription.getMediaDescriptions();
        } else {
            return null;
        }
    }
}
