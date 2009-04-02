/*
 * @(#)AudioStream.java	1.20 06/10/10
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
 * Convert an InputStream to an AudioStream. 
 *
 * @version 	1.16, 08/19/02
 */
public
class AudioStream extends FilterInputStream {
    NativeAudioStream audioIn;
    public AudioStream(InputStream in) throws IOException {
        super(in);
        try {
            audioIn = (new NativeAudioStream(in));
        } catch (InvalidAudioFormatException e) {
            // Not a native audio stream -- use a translator (if available).
            // If not, let the exception bubble up.
            audioIn = (new AudioTranslatorStream(in));
        }
        this.in = audioIn;
    }

    /**
     * A blocking read.
     */
    public int read(byte buf[], int pos, int len) throws IOException {
        int count = 0;
        while (count < len) {
            int n = super.read(buf, pos + count, len - count);
            if (n < 0) {
                return count;
            }
            count += n;
            Thread.currentThread().yield();
        }
        return count;
    }

    /**
     * Get the data.
     */
    public AudioData getData() throws IOException {
        byte buffer[] = new byte[audioIn.getLength()];
        int gotbytes = read(buffer, 0, audioIn.getLength());
        close();
        if (gotbytes != audioIn.getLength()) {
            throw new IOException("audio data read error");
        }
        return new AudioData(buffer);
    }
    
    public int getLength() {
        return audioIn.getLength();
    }
}
