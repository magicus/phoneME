/*
 * @(#)NativeAudioStream.java	1.11 06/10/10
 *
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
 *
 */

package sun.audio;

import java.io.InputStream;
import java.io.DataInputStream;
import java.io.FilterInputStream;
import java.io.IOException;

/**
 * A Sun-specific AudioStream that supports the .au file format. 
 *
 * @version 	1.7, 08/19/02
 */
public
class NativeAudioStream extends FilterInputStream {
    private final int SUN_MAGIC = 0x2e736e64;
    private final int DEC_MAGIC = 0x2e736400;
    private final int MINHDRSIZE = 24;
    private final int TYPE_ULAW = 1;
    private int length = 0;
    /**
     * Read header, only sun 8 bit, ulaw encoded, single channel,
     * 8000hz is supported
     */
    public NativeAudioStream(InputStream in) throws IOException {
        super(in);
        DataInputStream data = new DataInputStream(in);
        int magic = data.readInt();
        if (magic != SUN_MAGIC && magic != DEC_MAGIC) {
            System.out.println("NativeAudioStream: invalid file type.");
            throw new InvalidAudioFormatException();
        }
        int hdr_size = data.readInt(); // header size
        if (hdr_size < MINHDRSIZE) {
            System.out.println("NativeAudioStream: wrong header size of " +
                hdr_size + ".");
            throw new InvalidAudioFormatException();
        }
        length = data.readInt();
        int encoding = data.readInt();
        if (encoding != TYPE_ULAW) {
            System.out.println("NativeAudioStream: invalid audio encoding.");
            throw new InvalidAudioFormatException();
        }
        int sample_rate = data.readInt();
        if ((sample_rate / 1000) != 8) {	// allow some slop
            System.out.println("NativeAudioStream: invalid sample rate of " +
                sample_rate + ".");
            throw new InvalidAudioFormatException();
        }
        int channels = data.readInt();
        if (channels != 1) {
            System.out.println("NativeAudioStream: wrong number of channels. "
                + "(wanted 1, actual " + channels + ")");
            throw new InvalidAudioFormatException();
        }
        in.skip(hdr_size - MINHDRSIZE);
    }

    public int getLength() {
        return length;
    }
}
