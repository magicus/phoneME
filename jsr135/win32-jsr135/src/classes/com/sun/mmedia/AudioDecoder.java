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

/* abstract package private class for compressed audio decoders */
abstract class AudioDecoder {

    /**
     * The fourcc_encoding can be used when a decoder can decode a variety
     * of encodings within a family. For e.g a decoder that can decode both
     * AMR-NB and AMR-WB
     */
    protected String fourcc_encoding;
    protected int sampleRate;
    protected int channels;

    /**
     * Get bits per sample of the decoded audio data
     * Should be available after the class is instantiated.
     */
    abstract int getBitsPerSample();

    /**
     * Should be available after the class is instantiated.
     * Get endian of the decoded audio data
     */
    abstract boolean isBigEndian();

    /**
     * Should be available after the class is instantiated.
     * Return true if the decoded audio data is signed
     */
    abstract boolean isSigned();


    /**
     * If necessary, the data for the output buffer will be allocated
     * by the decoder.
     */
     
    abstract void  process(Buffer input, Buffer output) throws MediaException;

    abstract void close() throws MediaException;

    void reset() {
    }
}
