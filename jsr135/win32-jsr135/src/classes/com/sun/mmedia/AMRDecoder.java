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

import javax.microedition.media.MediaException;

/* package private class */

class AMRDecoder extends AudioDecoder {

    private static final int AMR_NB = 1;
    private static final int AMR_WB = 2;

    private static final int MAX_FRAMES = 10; // Per channel
    private static final int BYTES_PER_SAMPLE = 2;

    private int frameSizeInSamples;
    
    private int decoderType;
    
    private int nativePeer;
    
    /**
     * Creates and opens the native decoder.
     * @param decoderType 1 for Narrow Band, 2 for Wide Band
     * @return 0 for failure, non-zero native peer pointer if success
     */
    private native int nOpen(int decoderType, int channels);
    
    /**
     * Decodes 1 or more frames of AMR audio and outputs PCM data.
     * @param nativePeer pointer to native data structure.
     * @param amrData compressed audio data, with 1 byte frame header
     *   prepended for each frame.
     * @param offset offset into the amrData array
     * @param length number of bytes to read from the amrData array
     * @param channels specifies how many channels of audio to decode. If 
     *   greater than 1, then each frame is a different channel. Which means
     *   at least "channels" frames should be passed in each time.
     * @param pcmData output PCM audio is written to this array, in interleaved
     *   fashion if channels > 1.
     * @return number of bytes successfully consumed from amrData array.
     */
    private native int nDecode(int nativePeer, 
    			       byte [] amrData, int offset, int length,
    			       int channels,
			       byte [] pcmData);
				
    /**
     * Closes the native decoder and frees up resources.
     * @param nativePeer pointer to native data structure.
     */
    private native void nClose(int nativePeer);
			
    /**
     * MediaException is throw if the decoder doesn't support any of the
     * 3 paramters.
     */
    AMRDecoder(String fourcc_encoding,
	       int sampleRate,
	       int channels)
	       throws MediaException {
		   
	this.sampleRate = sampleRate;
	this.channels = channels;
	if (fourcc_encoding == null)
	    throw new MediaException("Unknown fourcc");
	if ("amr".equals(fourcc_encoding.toLowerCase())) {
	    decoderType = AMR_NB;
	    frameSizeInSamples = 160;
	} else if ("amr-wb".equals(fourcc_encoding.toLowerCase())) {
	    decoderType = AMR_WB;
	    frameSizeInSamples = 320;
	} else {
	    throw new MediaException("Unknown fourcc");
	}
	// Create the native peer
	if ((nativePeer = nOpen(decoderType, channels)) == 0)
	    throw new MediaException("Unable to create native AMR decoder");
    }

    int getBitsPerSample() {
	return 8 * BYTES_PER_SAMPLE;
    }

    boolean isBigEndian() {
	return false;
    }

    boolean isSigned() {
	return getBitsPerSample() == 16;
    }

    synchronized void close() {
	if (nativePeer != 0)
	    nClose(nativePeer);
    }

    /**
     * Returns the number of 20 msec chunks of data
     */
    int getMaxBufferLength() {
	return MAX_FRAMES * channels;
    }

    /**
     * Decodes the compressed data.
     */
    void  process(Buffer input, Buffer output) throws MediaException {
	if (output == null)
	    throw new MediaException("output Buffer is null");
	// Upto MAX_FRAMES, accounting for NB/WB size and channel count.
	int maxBytes = BYTES_PER_SAMPLE * frameSizeInSamples * MAX_FRAMES * channels;
	if (output.data == null || output.data.length < maxBytes ) {
	    output.data = new byte[maxBytes];
	}
	int retVal = nDecode(nativePeer, input.data, input.offset, input.length,
			     channels, output.data);
	if (retVal == 0 || 
	    retVal % (frameSizeInSamples * BYTES_PER_SAMPLE * channels) != 0) {
	    throw new MediaException("Error decoding AMR frames");
	}
	output.offset = 0;
	output.length = retVal;
    }

}
