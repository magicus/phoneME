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

import javax.microedition.io.Connector;
import javax.microedition.io.DatagramConnection;
import javax.microedition.io.Datagram;

import com.sun.j2me.log.Logging;
import com.sun.j2me.log.LogChannels;

public class RtpConnection extends Thread implements Runnable {
    DatagramConnection dc;
    int local_port;

    public RtpConnection(int local_port) {
        this.local_port = local_port;

        try {
            dc = (DatagramConnection)Connector.open("datagram://:" + local_port);
            start();
        } catch (java.io.IOException e) {
            dc = null;
        }
    }

    public void run() {
        while (null != dc) {
            try {
                Datagram d = dc.newDatagram(4096);
                dc.receive(d);
                int len = d.getLength();
                int off = d.getOffset();
                byte[] data = d.getData();
                RtpPacket pkt = new RtpPacket(data);
            } catch (IOException e) {
                if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
                    Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI,
                        "IOException in RtpConnection: " + e.getMessage());
                }
                break;
            }
        }
    }

    public boolean connectionIsAlive() {
        return (null != dc);
    }
}
