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

import com.sun.j2me.log.Logging;
import com.sun.j2me.log.LogChannels;

public class RtpConnection extends RtpConnectionBase {

    DatagramSocket ds = null;

    public RtpConnection(int local_port) {
        super(local_port);
    }

    public boolean connectionIsAlive() {
        return (null != ds);
    }

    public void startListening() throws IOException {
        try {
            ds = new DatagramSocket(local_port);
            start();
        } catch (SocketException e) {
            throw new IOException("Cannot start listening on port "
                + local_port + ": " + e);
        } catch (SecurityException e) {
            throw new IOException("Cannot start listening on port "
                + local_port + ": " + e);
        }
    }

    public void stopListening() {
        ds.close();
        ds = null;
    }

    public RtpPacket receivePacket() {
        try {
            byte[] data = new byte[MAX_DATAGRAM_SIZE];
            DatagramPacket dp = new DatagramPacket(data, MAX_DATAGRAM_SIZE);
            ds.receive(dp);
            RtpPacket pkt = new RtpPacket(data,dp.getLength());
            return pkt;
        } catch (IOException e) {
            if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
                Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI,
                    "IOException in RtpConnection.receivePacket(): " + e.getMessage());
            }
            return null;
        }
    }
}
