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
import javax.microedition.media.protocol.SourceStream;
import javax.microedition.media.protocol.ContentDescriptor;
import javax.microedition.media.Control;
import com.sun.j2me.log.Logging;
import com.sun.j2me.log.LogChannels;

public class RtspSS implements SourceStream {

    protected static final int PACKET_TIMEOUT = 30000; // ms to wait for packet arrival
    protected static final int INITIAL_QUEUE_SIZE = 100; // packets

    private RtpPacket cur_pkt = null;
    public ContentDescriptor cdescr;

    public RtspSS() {
    }

    void setContentDescriptor(String descr) {
        cdescr = new ContentDescriptor(descr);
        //System.out.println("**** MIME:" + cdescr.getContentType());
    }

    // ===================== SourceStream methods =============

    public ContentDescriptor getContentDescriptor() {
        return cdescr;
    }

    public long getContentLength() {
        return 0;
    }

    public int getSeekType() {
        return NOT_SEEKABLE;
    }

    public int getTransferSize() {
        return -1;
    }

    public int read(byte[] b, int off, int len)
        throws java.io.IOException {

        if (null == cur_pkt || 0 == cur_pkt.getPayloadSize()) {
            try {
                cur_pkt = dequeuePacket();
            } catch (InterruptedException e) {
                return -1;
            }
        }

        if (null == cur_pkt || 0 == cur_pkt.getPayloadSize()) return -1;

        int bytes_moved = cur_pkt.getPayload(b, off, len);

        return bytes_moved;
    }

    public long seek(long where) 
        throws java.io.IOException {
        return 0;
    }

    public long tell() {
        return 0;
    }

    // ===================== Controllable methods =============

    public Control getControl(String controlType) {
        return null;
    }

    public Control[] getControls() {
        return new Control[] { null };
    }

    // ===================== RTP packet queue =================

    Vector pkt_queue = new Vector(INITIAL_QUEUE_SIZE);

    public synchronized boolean enqueuePacket(RtpPacket pkt) {
        try {
            pkt_queue.addElement(pkt);
            if (null == cdescr) {
                RtpPayloadType pt = RtpPayloadType.get(pkt.getPayloadType());
                if (null != pt) {
                    setContentDescriptor(pt.getDescr());
                } else {
                    // unsupported content type
                    // conn.stopListening();
                }
            }
            notify();
            return true;
        } catch (OutOfMemoryError e) {
            if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
                Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI,
                    "OutOfMemoryError in RtpConnection.enqueuePacket()");
            }
            return false;
        }
    }

    public synchronized RtpPacket dequeuePacket() throws InterruptedException {
        if (0 == pkt_queue.size()) {
            wait(PACKET_TIMEOUT);
        }
        if (0 != pkt_queue.size()) {
            RtpPacket p = (RtpPacket)pkt_queue.elementAt(0);
            pkt_queue.removeElementAt(0);
            return p;
        } else {
            return null;
        }
    }

    public synchronized int getNumPackets() {
        return pkt_queue.size();
    }

}
