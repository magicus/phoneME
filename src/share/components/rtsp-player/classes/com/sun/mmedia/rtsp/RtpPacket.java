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

    RtpPacket(byte[] raw_data) {
        this.raw_data = raw_data;
    }

    int getVersion() {
        return raw_data[0] >> 6;
    }

    boolean hasPadding() {
        return 0 != (raw_data[0] & 0x20);
    }

    boolean hasExtension() {
        return 0 != (raw_data[0] & 0x10);
    }

    int getCsrcCount() {
        return raw_data[0] & 0x0F;
    }

    boolean hasMarker() {
        return 0 != (raw_data[1] & 0x80);
    }

    int getPayloadType() {
        return raw_data[1] & 0x7F;
    }

    int getSequenceNumber() {
        return (raw_data[2] << 8) |
                 raw_data[3];
    }

    int getTimestamp() {
        return (raw_data[4] << 24) |
               (raw_data[5] << 16) |
               (raw_data[6] << 8) |
                 raw_data[7];
    }

    int getSsrc() {
        return (raw_data[8] << 24) |
               (raw_data[9] << 16) |
               (raw_data[10] << 8) |
                 raw_data[11];
    }

    int getCsrc(int n) {
        int offs = 12 + 4 * n;
        return (raw_data[offs] << 24) |
               (raw_data[offs + 1] << 16) |
               (raw_data[offs + 2] << 8) |
                 raw_data[offs + 3];
    }

    int getPayloadOffset() {
        return 12 + 4 * getCsrcCount();
    }
}
