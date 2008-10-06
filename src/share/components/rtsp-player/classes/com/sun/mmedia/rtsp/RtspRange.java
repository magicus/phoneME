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

package com.sun.mmedia.rtsp;

public class RtspRange {

    public static final float NOW = -1;
    public static final float END = -1;

    private float startPos;
    private float endPos;

    public RtspRange( String str ) {
        int start = str.indexOf( '=' );
        int end = str.indexOf( '-' );

        String rangeType = str.substring( 0, start );
        String startPosStr = str.substring( start + 1, end );
        String endPosStr = str.substring( end + 1 );

        if( "npt".equals( rangeType ) ) {
            if( startPosStr.equals( "now" ) || startPosStr.length() == 0  ) {
                startPos = NOW;
            } else {
                startPos = Float.parseFloat( startPosStr );
            }

            if( endPosStr.length() == 0 ) {
                endPos = END;
            } else {
                endPos = Float.parseFloat( endPosStr );
            }
        } else {
            throw new NumberFormatException( "Cannot parse range '" 
                                             + str + "': Unknown range type" );
        }
    }

    public float getFrom() {
        return startPos;
    }

    public float getTo() {
        return endPos;
    }
}
