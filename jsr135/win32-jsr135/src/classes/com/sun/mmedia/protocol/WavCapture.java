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
package com.sun.mmedia.protocol;

import javax.microedition.media.*;
import javax.microedition.media.protocol.*;
import com.sun.mmedia.DefaultConfiguration;
import com.sun.mmedia.protocol.*;
import java.io.IOException;

/* Audio Capture Class, uses wave-in API on Win32
 */
public class WavCapture extends BasicDS implements SourceStream {

    // Defaults
    private int sampleRate = 8000;
    private int sampleSize = 16;
    private int channels = 1;
    private int endian; // 0 for any, 1 for "little", 2 for "big"
    private int signedvalue; // 0 for any, 1 for "signed", 2 for "unsigned"

    private int peer; // Holds the native data
    private long bytesRead;

    // The WAVe header
    private byte [] hdr = new byte[44];
    private int hdrSize;
    private int hdrOfs;
    
    /****************************************************************
     * DataSource implementation
     ****************************************************************/
    public WavCapture() {
        // Captures audio and wraps it into audio/x-wav content type
        contentType = DefaultConfiguration.MIME_AUDIO_WAV;
    }

    /* Parse the URL, extract and validate the properties */
    public void setLocator(String locator) throws MediaException {
        String key = null;
        String value = null;
        try {
            LocatorParser lp = new LocatorParser(locator.toLowerCase());
            if (!lp.getProtocol().equals(lp.S_PROTOCOL_CAPTURE) ||
                !lp.getDevice().equals(lp.S_DEVICE_AUDIO)) {
                throw new MediaException("Illegal locator " + locator);
            }
            while ((key = lp.getParameter()) != null) {
                value = lp.getValue(); 
                if (key.equals(lp.S_ENCODING)) {
                    if (!value.equals(lp.S_ENCODING_PCM)) {
                        throw new MediaException("Only pcm encoding is supported");
                    }
                } else if (key.equals(lp.S_RATE)) {
                    sampleRate = Integer.parseInt(value);
                } else if (key.equals(lp.S_BITS)) {
                    sampleSize = Integer.parseInt(value);
                } else if (key.equals(lp.S_CHANNELS)) {
                    channels = Integer.parseInt(value);
                } else if (key.equals(lp.S_ENDIAN)) {
                    if (value.equals(lp.S_ENDIAN_LITTLE)) {
                        endian = 1; // LITTLE_ENDIAN
                    } else if (value.equals(lp.S_ENDIAN_BIG)) {
                        endian = 2; // BIG_ENDIAN
                    } else {
                        throw new MediaException(
                            "Illegal value for endian " + value);
                    }
                } else if (key.equals(lp.S_SIGN)) {
                    if (value.equals(lp.S_SIGN_SIGNED))
                        signedvalue = 1; // SIGNED
                    else if (value.equals(lp.S_SIGN_UNSIGNED))
                        signedvalue = 2; // UNSIGNED
                    else
                        throw new MediaException("Illegal value for signed " + value);
                } else {
                    throw new MediaException("Illegal key/value pair " + key +
                                 " : " + value);
                }
            }
            if (lp.hasMore()) {
                throw new MediaException("Malformed locator. Parameters should be specified in <key>=<value> format");
            }
        } catch (MediaException e) {
            throw e;
        } catch (Exception e) {
            throw new MediaException("Illegal value for parameter " + key +
                         " : " + value);
        }
    
        // Check for bad params
        if (channels > 2 || channels < 1 ||
            (sampleSize != 8 && sampleSize != 16) ||
            (sampleSize == 8 && signedvalue == 1 /*signed*/) ||
            (sampleSize == 16 && (signedvalue == 2 || endian == 2 /*big*/))) {
            throw new MediaException("Requested format is not supported " + locator);
        }
        this.locator = locator;
    }

    void getConnection() throws IOException {
        // Create the native interface and open the sound card for input
        peer = nOpen(sampleRate, sampleSize, channels, 6 /* Number of native buffers */);
    
        if (peer == 0)
            throw new IOException("Couldn't open audio input");
        /*
        { // Workaround for a Zaurus bug where the audio device doesn't
          // capture data in the requested audio format but captures it in the format
          // that had been successfully set before the device was opened.
          // The problem happens the first time the device is opened with a new audio
          // format. Subsequently if the device is opened with the same format, audio
          // capture works correctly until the format is changed.
          // Closing and opening the audio device in the native code doesn't solve
          // the problem. However, closing the device after setting the desired audio format
          // and reopening the device works. 

          // IMPL_NOTE: This section should be under #ifdef ZAURUS which can be done
          // if this file is made into a jpp.
    
            close();
            peer = nOpen(sampleRate, sampleSize, channels, 6 );
            if (peer == 0)
            throw new IOException("Couldn't open audio input");
        }*/
        // Generate the WAV header
        createHeader(sampleRate, sampleSize, channels);
    }

    public void close() {
        stop();
        if (peer != 0) {
            nCommand(peer, 3 /*CLOSE*/, 0);
            peer = 0;
        }
    }

    public void start() throws IOException {  
        if (nCommand(peer, 1 /*START*/, 0) == 0) {
            throw new IOException("Couldn't start audio input");
        }
    }
    
    public void stop() {
        nCommand(peer, 2 /*STOP*/, 0);
    }

    public void connect() throws IOException {
        getConnection();
    }

    public synchronized void disconnect() {
        close();
    }

    public SourceStream[] getStreams() {
        return new SourceStream[] { this };
    }

    
    /****************************************************************
     * SourceStream implementation
     ****************************************************************/

    public ContentDescriptor getContentDescriptor() {
        return new ContentDescriptor(getContentType());
    }

    public long getContentLength() {
        return -1;
    }

    public int read(byte[] buffer, int offset, int length) throws IOException {
        // First transfer the header
        if (hdrSize > 0) {
            int toCopy = hdrSize;
            if (length < toCopy)
            toCopy = length;
            System.arraycopy(hdr, hdrOfs,
                     buffer, offset,
                     toCopy);
            hdrSize -= toCopy;
            hdrOfs += toCopy;
            return toCopy;
        }
        // Read a chunk of captured audio from the device
        int got = nRead(peer, buffer, offset, length);
        bytesRead += (long) got;
        // If we didn't get any audio, sleep and try again.
        while (got == 0) {
            try {
            Thread.sleep(16); // Half the time of each capture chunk
            } catch (InterruptedException ie) {
            }
            got = nRead(peer, buffer, offset, length);
            bytesRead += (long) got;
        }
        return got;
    }

    // Cant really seek, throw an IOException.
    public long seek(long where) throws IOException {
        throw new IOException("Cannot seek");
    }

    // Return the bytes read from audio device so far
    public long tell() {
    	return bytesRead;
    }

    // Live input, not streaming
    public int getSeekType() {
    	return NOT_SEEKABLE;
    }

    public int getTransferSize() {
        // Divide by 8 and multiply by 8 to make it a multiple of 8
        // Overall divide by 8 to make it 125 millisecond chunks
        return 8 * (((sampleSize * sampleRate * channels) / 8) / 64);
    }

    /****************************************************************
     * Native Methods
     ****************************************************************/
    
    // Open the device for the given parameters and number of chunks
    private native int nOpen(int sampleRate, int sampleSize, int channels,
			        int chunks);

    // General purpose command function to start/stop/etc.
    // Next available command number is 4
    private native int nCommand(int peer, int command, int param);

    // Read available bytes into the buf
    private native int nRead(int peer, byte [] buf,
			        int offset, int length);

    /****************************************************************
     * Create Wave Header
     ****************************************************************/

    // Write an int in LITTLE_ENDIAN
    private void writeInt(int value) {
        hdr[hdrSize++] = (byte)(value & 0xFF);
        hdr[hdrSize++] = (byte)((value >>>  8) & 0xFF);
        hdr[hdrSize++] = (byte)((value >>> 16) & 0xFF);
        hdr[hdrSize++] = (byte)((value >>> 24) & 0xFF);
    }
    
    // Write a short in LITTLE_ENDIAN
    private void writeShort(int value) {
        hdr[hdrSize++] = (byte)((value >> 0) & 0xFF);
        hdr[hdrSize++] = (byte)((value >> 8) & 0xFF);	
    }

    // Write the WAVe header into the hdr array
    private void createHeader(int samplesPerSec,
			        int sampleSizeInBits,
			        int channels) {
        hdrSize = 0;
        int avBytesPerSec = channels * samplesPerSec * (sampleSizeInBits / 8);
        writeInt(0x46464952 /*RIFF*/);
        writeInt(-1);	// Length unbounded
        writeInt(0x45564157 /*WAVE*/);
        writeInt(0x20746D66 /*FMT*/);  // Format Chunk
        writeInt(16);   // size of format chunk
        writeShort(1);  // wFormatTag ==> uncompressed
        writeShort((short) channels);
        writeInt(samplesPerSec);
        writeInt(avBytesPerSec);
        writeShort(channels * sampleSizeInBits / 8); // BlockAlign
        writeShort(sampleSizeInBits);
        writeInt(0x61746164 /*DATA*/); // Data chunk
        writeInt(-1);	// Length unbounded
    }
}
