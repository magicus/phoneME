/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

import javax.microedition.media.protocol.SourceStream;
import javax.microedition.media.protocol.ContentDescriptor;
import javax.microedition.media.Control;

public class RtspSS implements SourceStream {

    private RtpConnection conn;
    private RtpPacket cur_pkt = null;
    public ContentDescriptor cdescr;

    public RtspSS(RtpConnection conn) {
        this.conn = conn;
        conn.setSS(this);
    }

    void setContentDescriptor(String descr) {
        cdescr = new ContentDescriptor(descr);
        //System.out.println("**** MIME:" + cdescr.getContentType());
    }

    void packetArrived(RtpPacket pkt) {
        if (null == cdescr) {
            RtpPayloadType pt = RtpPayloadType.get(pkt.getPayloadType());
            if (null != pt) {
                setContentDescriptor(pt.getDescr());
            } else {
                // unsupported content type
                // conn.stopListening();
            }
        }
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
                cur_pkt = conn.dequeuePacket();
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
}
