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

class RtpPacket {

    byte[] raw_data;
    int raw_data_size;

    int payload_offs;
    int payload_size;

    RtpPacket(byte[] raw_data, int raw_data_size) {
        this.raw_data = raw_data;
        this.raw_data_size = raw_data_size;

        payload_offs = 12 + 4 * csrcCount();
        payload_size = raw_data_size - payload_offs;
    }

    byte[] raw() {
        return raw_data;
    }

    int rawSize() {
        return raw_data_size;
    }

    int version() {
        return raw_data[0] >> 6;
    }

    boolean hasPadding() {
        return 0 != (raw_data[0] & 0x20);
    }

    boolean hasExtension() {
        return 0 != (raw_data[0] & 0x10);
    }

    int csrcCount() {
        return raw_data[0] & 0x0F;
    }

    boolean hasMarker() {
        return 0 != (raw_data[1] & 0x80);
    }

    int payloadType() {
        return raw_data[1] & 0x7F;
    }

    short sequenceNumber() {
        return (short)(((raw_data[2] & 0xFF) << 8) |
                       (raw_data[3] & 0xFF) );
    }

    int timestamp() {
        return ((raw_data[4] & 0xFF) << 24) |
               ((raw_data[5] & 0xFF) << 16) |
               ((raw_data[6] & 0xFF) << 8) |
                 (raw_data[7] & 0xFF);
    }

    int ssrc() {
        return ((raw_data[8] & 0xFF) << 24) |
               ((raw_data[9] & 0xFF) << 16) |
               ((raw_data[10] & 0xFF) << 8) |
                 (raw_data[11] & 0xFF);
    }

    int csrc(int n) {
        int offs = 12 + 4 * n;
        return ((raw_data[offs] & 0xFF) << 24) |
               ((raw_data[offs + 1] & 0xFF) << 16) |
               ((raw_data[offs + 2] & 0xFF) << 8) |
                 (raw_data[offs + 3] & 0xFF);
    }

    int getPayload(byte[] buf, int buf_offs, int buf_size) {
        int len = Math.min(buf_size, payload_size);
        System.arraycopy(raw_data, payload_offs, buf, buf_offs, len);
        payload_size -= len;
        payload_offs += len;
        return len;
    }

    int payloadOffs() {
        return payload_offs;
    }

    int payloadSize() {
        return payload_size;
    }
}
