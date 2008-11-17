/*
 * @(#)AudioData.java	1.26 06/10/10
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

/**
 * A clip of audio data, contains ulaw 8bit, 8000hz data.
 * This data can be used to construct and AudioDataStream,
 * which can be played. <p>
 *
 * @author  Arthur van Hoff
 * @version 1.22, 08/19/02
 * @see     AudioDataStream
 * @see     AudioPlayer
 */
public
class AudioData {
    /**
     * The data
     */
    byte buffer[];
    /**
     * Constructor
     */
    public AudioData(byte buffer[]) {
        this.buffer = buffer;
    }
}
