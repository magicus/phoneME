/*
 * @(#)AudioPlayer.java	1.41 06/10/10
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

import java.util.Vector;
import java.util.Enumeration;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.security.AccessController;
import java.security.PrivilegedAction;

/**
 * This class provides an interface to play audio streams.
 *
 * To play an audio stream use:
 * <pre>
 *	AudioPlayer.player.start(audiostream);
 * </pre>
 * To stop playing an audio stream use:
 * <pre>
 *	AudioPlayer.player.stop(audiostream);
 * </pre>
 * To play an audio stream from a URL use:
 * <pre>
 *	AudioStream audiostream = new AudioStream(url.openStream());
 *	AudioPlayer.player.start(audiostream);
 * </pre>
 * To play a continuous sound you first have to
 * create an AudioData instance and use it to construct a
 * ContinuousAudioDataStream.
 * For example:
 * <pre>
 *	AudoData data = new AudioStream(url.openStream()).getData();
 *	ContinuousAudioDataStream audiostream = new ContinuousAudioDataStream(data);
 *	AudioPlayer.player.stop(audiostream);
 * </pre>
 *
 * @see AudioData
 * @see AudioDataStream
 * @see AudioDevice
 * @see AudioStream
 * @author Arthur van Hoff, Thomas Ball
 * @version 	1.37, 08/19/02
 */
public
class AudioPlayer extends Thread {
    private AudioDevice devAudio;
    /**
     * The default audio player. This audio player is initialized
     * automatically.
     */
    public static final AudioPlayer player = getAudioPlayer();
    private static ThreadGroup getAudioThreadGroup() {
        ThreadGroup g = currentThread().getThreadGroup();
        while ((g.getParent() != null) && (g.getParent().getParent() != null)) {
            g = g.getParent();
        }
        return g;
    }

    /**
     * Create an AudioPlayer thread in a privileged block.
     */
    private static AudioPlayer getAudioPlayer() {
        return (AudioPlayer) AccessController.doPrivileged(
                new PrivilegedAction() {
                    public Object run() {
                        return new AudioPlayer();
                    }
                }
            );
    }

    /**
     * Construct an AudioPlayer.
     */
    private AudioPlayer() {
        super(getAudioThreadGroup(), "Audio Player");
        devAudio = AudioDevice.device;
        setPriority(MAX_PRIORITY);
        setDaemon(true);
        start();
    }

    /**
     * Start playing a stream. The stream will continue to play
     * until the stream runs out of data, or it is stopped.
     * @see AudioPlayer#stop
     */
    public synchronized void start(InputStream in) {
        devAudio.openChannel(in);
        notify();
    }

    /**
     * Stop playing a stream. The stream will stop playing,
     * nothing happens if the stream wasn't playing in the
     * first place.
     * @see AudioPlayer#start
     */
    public synchronized void stop(InputStream in) {
        devAudio.closeChannel(in);
    }

    /**
     * Main mixing loop. This is called automatically when the AudioPlayer
     * is created.
     */
    public void run() {
        devAudio.open();
        devAudio.play();
        devAudio.close();
        System.out.println("audio player exit");
    }
}
