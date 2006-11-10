/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
package com.sun.mmedia;

import java.io.IOException;
import com.sun.midp.security.*;

import com.sun.mmedia.WavPlayer;
import com.sun.mmedia.RecordCtrl;

/* IMPL_NOTE:
 * - move ownership of RateControl, VolumeControl, etc. to
 *   the audio renderer. That will enable sharing them with
 *   other players that use the renderer.
 */

public final class WavRecordCtrl extends RecordCtrl {
    private byte[] header;

    public WavRecordCtrl() {
    }

    public WavRecordCtrl(WavPlayer p) {
        player = p;
        headerSize = 44;
    }

    public final String getContentType() {
        return "audio/x-wav";
    }

    // IMPL_NOTE: avoid passing endian parameter
    // Have a method like setEndian and call it once.
    protected final byte[] getHeader(int dataSize) throws IOException {

        WavPlayer wPlayer = (WavPlayer) player;
        int channels = wPlayer.channels;
        int sampleRate = wPlayer.sampleRate;
        int sampleSizeInBits = wPlayer.sampleSizeInBits;
    
        if ( (header == null) || (header.length < 44) ) {
            header = new byte[44];
        }
    
        // Note: the header can be written directly using
        // malloc and fwrite  methods
        int avBytesPerSec = channels * sampleRate * (sampleSizeInBits / 8);
        int blockAlign = (sampleSizeInBits / 8) * channels;
        int offset = 0;
    
        writeInt(header, 0x46464952 /*RIFF*/, offset, /*bigEndian=*/false);
        offset +=4;
    
        // If we don't have any optional chunks,
        // fileSize is (dataSize + 44)
        writeInt(header, dataSize + 36, offset, /*bigEndian=*/false); // fileSize - 8
        offset +=4;
    
        writeInt(header, 0x45564157 /*WAVE*/, offset, /*bigEndian=*/false);
        offset +=4;
    
        writeInt(header, 0x20746D66 /*FMT*/, offset, /*bigEndian=*/false); // Format Chunk
        offset +=4;
    
        writeInt(header, 16, offset, /*bigEndian=*/false); // size of format chunk
        offset +=4;
    
        writeShort(header, 1, offset, /*bigEndian=*/false); // wFormatTag ==> uncompressed
        offset +=2;
    
        writeShort(header, (short) channels, offset, /*bigEndian=*/false);
        offset +=2;
    
        writeInt(header, sampleRate, offset, /*bigEndian=*/false);
        offset +=4;
    
        writeInt(header, avBytesPerSec, offset, /*bigEndian=*/false);
        offset +=4;
    
        writeShort(header, blockAlign, offset, /*bigEndian=*/false);
        offset +=2;
    
        writeShort(header, sampleSizeInBits, offset, /*bigEndian=*/false);
        offset +=2;
    
        writeInt(header, 0x61746164 /*DATA*/, offset, /*bigEndian=*/false); // Data chunk
        offset +=4;
    
        writeInt(header, dataSize, offset, /*bigEndian=*/false);
    
        return header;
    }
}
