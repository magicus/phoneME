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

import java.util.Vector;
import com.sun.j2me.log.Logging;
import com.sun.j2me.log.LogChannels;

class DefaultDepacketizer implements Depacketizer
{
    protected static final int PACKET_TIMEOUT = 30000; // ms to wait for packet arrival
    protected static final int INITIAL_QUEUE_SIZE = 100; // packets

    protected RtpPacket cur_pkt = null;
    protected Vector pkt_queue = new Vector(INITIAL_QUEUE_SIZE);

    public int read(byte[] b, int off, int len) throws java.io.IOException {

        if (null == cur_pkt || 0 == cur_pkt.payloadSize()) {
            try {
                cur_pkt = dequeuePacket();
            } catch (InterruptedException e) {
                return -1;
            }
        }

        if (null == cur_pkt || 0 == cur_pkt.payloadSize()) return -1;

        int bytes_moved = cur_pkt.getPayload(b, off, len);

        return bytes_moved;
    }

    public boolean processPacket(RtpPacket pkt) {
        return enqueuePacket(pkt);
    }

    protected boolean enqueuePacket(RtpPacket pkt) {
        try {
            synchronized (pkt_queue) {
                pkt_queue.addElement(pkt);
                pkt_queue.notify();
            }
            return true;
        } catch (OutOfMemoryError e) {
            if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
                Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI,
                    "OutOfMemoryError in DefaultDepacketizer.enqueuePacket()");
            }
            return false;
        }
    }

    protected RtpPacket dequeuePacket() throws InterruptedException {
        synchronized (pkt_queue) {
            if (0 == pkt_queue.size()) {
                pkt_queue.wait(PACKET_TIMEOUT);
            }
            if (0 != pkt_queue.size()) {
                RtpPacket p = (RtpPacket)pkt_queue.elementAt(0);
                pkt_queue.removeElementAt(0);
                return p;
            } else {
                return null;
            }
        }
    }
}
