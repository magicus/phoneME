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

package com.sun.mmedia.rtsp.protocol;

public class RtspIncomingMessage {

    private byte[] bytes;

    private RtspMessageType type;

    public RtspIncomingMessage( byte[] bytes ) {

        System.out.println( "----- incoming message -----\n"
                            + new String( bytes ) +
                            "----------------------------\n" );

        this.bytes = bytes;

        int n = 0;
        while( n < bytes.length && ' ' != bytes[ n ] ) n++;
        String strType = new String( bytes, 0, n );
        type = new RtspMessageType( strType );

        System.out.println( "- type: " + type );

        int offs = 0;

        while( offs < bytes.length ) {

            int len = 0;
            while( offs + len + 1 < bytes.length && 
                   ( '\r' != bytes[ offs + len ] || '\n' != bytes[ offs + len + 1 ] ) ) { 
                len++;
            }

            String line = new String( bytes, offs, len );

            offs += len + 2;

            if( 0 != line.length() ) {
                parseLine( line );
            } else {
                if( offs < bytes.length ) {
                    System.out.println( "- data: [[[" 
                                        + new String( bytes, offs, bytes.length - offs )
                                        + "]]]" );
                }
                break;
            }
        }

        System.out.println( "----------------------------\n\n" );
    }

    private void parseLine( String line ) {
        System.out.println( "- line: '" + line + "'" );
    }
}



