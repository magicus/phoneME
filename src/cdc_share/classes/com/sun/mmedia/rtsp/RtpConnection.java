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

import java.io.IOException;
import java.net.DatagramSocket;
import java.net.DatagramPacket;
import java.net.SocketException;

public class RtpConnection extends Thread implements Runnable
{
    DatagramSocket ds;
    int            local_port;

    public RtpConnection( int local_port ) {
        this.local_port = local_port;

        try {
            ds = new DatagramSocket( local_port );
            start();
        } catch( SocketException e ) {
            ds = null;
        } catch( SecurityException e ) {
            ds = null;
        }
    }

    public void run() {
        while( null != ds ) {
            try {
                byte[] data = new byte[ 4096 ];
                DatagramPacket dp = new DatagramPacket( data, 4096 );
                ds.receive( dp );
                RtpPacket pkt = new RtpPacket( data );
            } catch( IOException e ) {
                e.printStackTrace();
                break;
            }
        }
    }

    public boolean connectionIsAlive() {
        return ( null != ds );
    }
}
