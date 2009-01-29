/*
 * @(#)AppletAudioClip.java	1.23 06/10/10
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

/* AppletAudioClip from Amber (PersonalJava 3.1) */

package sun.applet;

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.net.URLConnection;
import java.applet.AudioClip;
import sun.audio.*;

/**
 * Applet audio clip;
 *
 * @version 	1.12, 07/01/98
 * @author Arthur van Hoff
 */
public class AppletAudioClip implements AudioClip {
    URL url;
    AudioData data;
    InputStream stream;
    /**
     * This should really fork a seperate thread to
     * load the data.
     */
    public AppletAudioClip(URL url) {
        InputStream in = null;
        try {
            try {
                URLConnection uc = url.openConnection();
                uc.setAllowUserInteraction(true);
                in = uc.getInputStream();
                data = new AudioStream(in).getData();
            } finally {
                if (in != null) {
                    in.close();
                }
            }
        } catch (IOException e) {
            data = null;
        }
    }

    /* 
     * For constructing directly from Jar entries.  Or any other
     * raw Audio data.
     */

    public AppletAudioClip(byte[] data) {
        this.data = new AudioData(data);
    }

    public synchronized void play() {
        stop();
        if (data != null) {
            java.awt.Toolkit.getDefaultToolkit();
            stream = new AudioDataStream(data);
            AudioPlayer.player.start(stream);
        }
    }

    public synchronized void loop() {
        stop();
        if (data != null) {
            stream = new ContinuousAudioDataStream(data);
            AudioPlayer.player.start(stream);
        }
    }

    public synchronized void stop() {
        if (stream != null) {
            AudioPlayer.player.stop(stream);
            try {
                stream.close();
            } catch (IOException e) {}
        }
    }

    public String toString() {
        return getClass().toString() + "[" + url + "]";
    }
}
