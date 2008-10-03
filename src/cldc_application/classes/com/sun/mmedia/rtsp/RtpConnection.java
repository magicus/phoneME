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

package com.sun.mmedia.rtsp;

import java.io.IOException;

//import java.io.InputStream;
//import java.io.OutputStream;
//import java.io.ByteArrayOutputStream;

import javax.microedition.io.Connector;
import javax.microedition.io.DatagramConnection;
import javax.microedition.io.Datagram;

public class RtpConnection extends Thread implements Runnable
{
    DatagramConnection dc;
    int                local_port;

    public RtpConnection( int local_port ) {
        this.local_port = local_port;

        try {
            dc = (DatagramConnection)Connector.open( "datagram://:" + local_port );
            start();
        } catch( java.io.IOException e ) {
            dc = null;
        }
    }

    private String hex( byte b ) {

        String s = Integer.toHexString( b ).toUpperCase();
        int l = s.length();

        if( l == 1 ) {
            return "0" + s;
        } else if( l == 2 ) {
            return s;
        } else {
            return s.substring( l - 2 );
        }
    }

    public void run() {

        System.out.println( "*rtp* listening port " + local_port );

        while( null != dc ) {
            try {
                Datagram d = dc.newDatagram( 4096 );
                dc.receive( d );
                int len = d.getLength();
                int off = d.getOffset();
                byte[] data = d.getData();

                /*
                System.out.print( "*rtp* dgram received: " 
                    + off + "/" + len + ":\t"
                    + hex( data[ 0 ] ) + " "
                    + hex( data[ 1 ] ) + " "
                    + hex( data[ 2 ] ) + " "
                    + hex( data[ 3 ] ) + "  "
                    + hex( data[ 4 ] ) + " "
                    + hex( data[ 5 ] ) + " "
                    + hex( data[ 6 ] ) + " "
                    + hex( data[ 7 ] ) + "  "
                    + hex( data[ 8 ] ) + " "
                    + hex( data[ 9 ] ) + " "
                    + hex( data[ 10 ] ) + " "
                    + hex( data[ 11 ] ) + "  " );
                */

                RtpPacket pkt = new RtpPacket( data );

                System.out.println( "Pt="     + pkt.getPayloadType() +
                                    ", Seq="  + pkt.getSequenceNumber() + 
                                    ", Ssrc=" + pkt.getSsrc() );

            } catch( IOException e ) {
                e.printStackTrace();
                break;
            }
        }
    }

    public boolean connectionIsAlive() {
        return ( null != dc );
    }
}
