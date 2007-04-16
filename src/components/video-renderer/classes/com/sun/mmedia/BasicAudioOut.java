/*
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

/**
 * Parent interface that should be implemented by all DataSinks. 
 * It does not define any methods but only constants for the known
 * DataSink types
 */
public interface BasicAudioOut {
    /**
     * Default DataSink type 
     */
    public final int DATASINK_DEFAULT = 0;

    /**
     * Sound 3D DataSink type 
     */
    public final int DATASINK_SOUND3D = 1;

    /**
     * Media Processor DataSink
     */
    public final int DATASINK_EFFECTS = 2;

    /**
     * Open connection to the DataSink with the given parameters.
     *
     * @param sampleRate sample rate of the outpus stream
     * @param bits       number bits per channel
     * @param channels   number of channels in the stream, e.g. 1 for mono, 
     *                   2 for stereo, etc.
     */
    public boolean open(int sampleRate, int bits, int channels);

    public boolean open(int sampleRate, int bits, int channels, 
                        boolean isSigned, boolean isBigEndian);

    /**
     * Writes data to the data sink
     */
    public int write(byte [] data, int offset, int len);

    /**
     * Pauses the data processing by the data sink
     */
    public void pause();

    /**
     * Resumes the data processing by the data sink
     */
    public void resume();

    /**
     * Flashes any data buffered in the data sink buffers
     */
    public void flush();

    public int drain();

    public void drainLoop();

    public long getSamplesPlayed();

    public int getVolume();

    public void setVolume(int level);

    public long getMediaTime();

    public void setMediaTime(long mediaTime);
    
    public void setRate(int rate);
    
    public void close();
    
}
