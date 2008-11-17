/*
 * @(#)ContinuousAudioDataStream.java	1.17 06/10/10
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
 * Create a continuous audio stream. This wraps a stream
 * around an AudioData object, the stream is restarted
 * at the beginning everytime the end is reached, thus
 * creating continuous sound.<p>
 * For example:
 * <pre>
 *	AudioData data = AudioData.getAudioData(url);
 *	ContinuousAudioDataStream audiostream = new ContinuousAudioDataStream(data);
 *	AudioPlayer.player.start(audiostream);
 * </pre>
 *
 * @see AudioPlayer
 * @see AudioData
 * @author Arthur van Hoff
 * @version 	1.13, 08/19/02
 */
public
class ContinuousAudioDataStream extends AudioDataStream {
    /**
     * Create a continuous stream of audio.
     */
    public ContinuousAudioDataStream(AudioData data) {
        super(data);
    }

    /**
     * When reaching the EOF, rewind to the beginning.
     */
    public int read() {
        int c = super.read();
        if (c == -1) {
            reset();
            c = super.read();
        }
        return c;
    }

    /**
     * When reaching the EOF, rewind to the beginning.
     */
    public int read(byte buf[], int pos, int len) {
        int count = 0;
        while (count < len) {
            int n = super.read(buf, pos + count, len - count);
            if (n >= 0) {
                count += n;
            } else {
                reset();
            }
        }
        return count;
    }
}
