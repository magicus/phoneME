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

class AduqDepacketizer extends DefaultDepacketizer
{
    private static final int READ_TIMEOUT = 2000; // ms to wait before returning 0
    private static final int OUT_QUEUE_INITIAL_SIZE = 100; // mp3 frames

    private Vector out_queue = new Vector(OUT_QUEUE_INITIAL_SIZE);
    private int head_offs = 0; // offset in head packet
    private int bytes_ready = 0;

    public int read(byte[] b, int off, int len) throws java.io.IOException {

        synchronized (out_queue) {

            int bytes_moved = 0;

            try {
                int s = out_queue.size();
                while (bytes_ready < len) {
                    out_queue.wait(READ_TIMEOUT);
                    if (s == out_queue.size()) {
                        // if no new packets arrived within timeout period,
                        // return all data we already have
                        break;
                    }
                }
            } catch (InterruptedException ie) {
                return -1;
            }

            byte[] head;
            int n;

            while (bytes_moved < len && 0 != out_queue.size()) {

                head = (byte[])out_queue.elementAt(0);
                n = Math.min(len - bytes_moved, head.length - head_offs);

                System.arraycopy(head, head_offs, b, off + bytes_moved, n);

                head_offs += n;
                bytes_moved += n;

                if (0 == head.length - head_offs) {
                    out_queue.removeElementAt(0);
                    head_offs = 0;
                }
            }

            bytes_ready -= bytes_moved;
            return bytes_moved;
        }
    }

    public boolean processPacket(RtpPacket pkt) {

        if (!enqueuePacket(pkt)) {
            return false;
        }

        RtpPacket p;
        byte[] adu;
        int adu_offs;
        int adu_size;

        while( 0 != pkt_queue.size() ) {

            int i = 0;

            p = (RtpPacket)pkt_queue.elementAt(0);
            adu = p.raw();
            adu_offs = p.payloadOffs();
            adu_size = p.payloadSize();

            int h = (0xFF & (int)adu[adu_offs + 0]) << 24 |
                    (0xFF & (int)adu[adu_offs + 1]) << 16 |
                    (0xFF & (int)adu[adu_offs + 2]) << 8 |
                    (0xFF & (int)adu[adu_offs + 3]);

            int sfs = framesize(h);//source frame size (side info+frame data, without header)
            int ssis = sideinfosize(h);//source side info size
            int sfds = sfs - ssis;//source frame data size
            int dfs = sfs;//destination frame size
            int dsis = ssis;//destination side info size
            int dfds = sfds;//destination frame data size

            byte[] f = new byte[4+dfs];
            System.arraycopy(adu, adu_offs, f, 0, 4 + dsis);
            for (int j = 0; j < dfds; j++) {
                f[4 + dsis + j] = 0;
            }

            int base = 0;
            boolean complete = false;

            while (true) {
                int bp = ((0xFF & (int)adu[adu_offs + 4]) << 1) |
                         ((0xFF & (int)adu[adu_offs + 5]) >> 7) & 1;

                int src, dst, len;

                if (bp > base) {
                    src = bp - base;
                    dst = 0;
                    if (src < adu_size - (4 + ssis)) {
                        len = adu_size - (4 + ssis) - src;
                    } else {
                        len = 0;
                    }
                } else {
                    src = 0;
                    dst = base - bp;
                    len = adu_size - (4 + ssis);
                }

                if (dst >= dfds) {
                    complete = true;
                    break;
                }

                if (dst + len >= dfds) {
                    complete = true;
                    len = dfds - dst;
                }

                if (0 != len) {
                    System.arraycopy(adu, adu_offs + 4 + ssis + src, f, 4 + dsis + dst, len);
                }

                if (complete || i + 1 >= pkt_queue.size()) {
                    break;
                }

                base += sfds;
                i++;
                p = (RtpPacket)pkt_queue.elementAt(i);
                adu = p.raw();
                adu_offs = p.payloadOffs();
                adu_size = p.payloadSize();

                h = (0xFF & (int)adu[adu_offs + 0]) << 24 |
                    (0xFF & (int)adu[adu_offs + 1]) << 16 |
                    (0xFF & (int)adu[adu_offs + 2]) << 8 |
                    (0xFF & (int)adu[adu_offs + 3]);

                sfs = framesize(h);
                ssis = sideinfosize(h);
                sfds = sfs - ssis;
            } // while(true)

            if (!complete) {
                break;
            }

            synchronized (out_queue) {
                bytes_ready += f.length;
                out_queue.addElement(f);
                out_queue.notify();
            }

            pkt_queue.removeElementAt(0);

        } // while(0 != pkt_queue.size())

        return true;
    }

    private static int getsync(int hdr) { return (hdr >> 21) & 2047; }
    private static int getversion(int hdr) { return (hdr >> 19) & 3; }
    private static int getlayer(int hdr) { return (hdr >> 17) & 3; }
    private static int getprotection(int hdr) { return (hdr >> 16) & 1; }
    private static int getbitrate(int hdr) { return (hdr >> 12) & 15; }
    private static int getsampling(int hdr) { return (hdr >> 10) & 3; }
    private static int getpadding(int hdr) { return (hdr >> 9) & 1; }
    private static int getprivatebit(int hdr) { return (hdr >> 8) & 1; }
    private static int getmode(int hdr) { return (hdr >> 6) & 3; }
    private static int getmodeext(int hdr) { return (hdr >> 4) & 3; }
    private static int getcopyright(int hdr) { return (hdr >> 3) & 1; }
    private static int getoriginal(int hdr) { return (hdr >> 2) & 1; }
    private static int getemphasis(int hdr) { return (hdr & 3); }

    private static boolean ismpeg2(int hdr) { return (getversion(hdr) & 1) == 0; }
    private static boolean ismpeg2_5(int hdr) { return getversion(hdr) == 0; }
    private static boolean hascrc(int hdr) { return getprotection(hdr) == 0; }
    private static boolean isstereo(int hdr) { return getmode(hdr) != 3; }

    private static int layer(int hdr) { return 4 - getlayer(hdr); }

    private static int bitrate(int hdr) {
        return br[(ismpeg2(hdr) ? 1 : 0)][ layer(hdr) - 1][getbitrate(hdr)];
    }

    private static int sampling(int hdr) {
        return sf[getversion(hdr)][getsampling(hdr)];
    }

    private static int framesize(int hdr) {
        int r = 0;
        int s = sampling(hdr);
        if (0 != s) {
            r = bitrate(hdr);
            if (3 == getlayer(hdr)) {
                r *= 48000;
            } else {
                r *= 144000;
            }

            if (ismpeg2(hdr)) {
                s <<= 1;
            }

            r /= s;
            if (1 == getpadding(hdr)) {
                r++;
            }
            r -= 4;
        }
        return r;
    }

    private static int sideinfosize(int hdr) {
        int r = 0;
        if (hascrc(hdr)) {
            r += 2;
        }
        if (3 == getversion(hdr)) {
            r += isstereo(hdr) ? 32 : 17;
        } else {
            r += isstereo(hdr) ? 17 : 9;
        }
        return r;
    }

    private static final int[][][] br = {
        {
            {1,32,64,96,128,160,192,224,256,288,320,352,384,416,448,0},
            {1,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,0},
            {1,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,0}
        },{
            {1,32,48,56, 64, 80, 96,112,128,144,160,176,192,224,256,0},
            {1, 8,16,24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160,0},
            {1, 8,16,24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160,0}
        }
    };

    private static final int sf[][] = {
        {11025,12000,8000,0},
        {0,0,0,0},
        {22050,24000,16000,0},
        {44100,48000,32000,0}
    };
}
