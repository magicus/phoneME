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

class QSoundSynthPerformance
{
    private int qsmPeer;
    private int spPeer;
    private int gmPeer;
    
    private int tchnl;   // MIDI channel to play instant Tones on
    
    QSoundSynthPerformance()
    {
        gmPeer = QSoundHiddenManager.getMIDIGlobalPeer();
        qsmPeer = nInitSynth(gmPeer);
        spPeer = nInitSynthPerformance(qsmPeer);
        
        tchnl = 0;        
    }
    
    private native int nInitSynth(int gmPeer);
    private native int nInitSynthPerformance(int qsmPeer);
    
    private native void nEnableChannel(int peer, int channel, boolean enabled);
    
    void enableChannel(int channel, boolean enabled)
    {
        nEnableChannel(spPeer, channel, enabled);
    }
    
    
    private native void nWriteEventInt(int peer, int data, int tenthsMs);
    
    void writeEvent(int data, int tenthsMS)
    {
        nWriteEventInt(spPeer, data, tenthsMS);
    }
    
    void setChannel(int channel)
    {
        tchnl = channel;
    }
    
    private native void nPlayTone(int peer, int note, int dur, int vol, int channel);
    
    void playTone(int note, int duration, int volume)
         throws javax.microedition.media.MediaException {

        // Implementation Note: Need to play on a channel that is not being used or reserve a channel for instant playtones.
        
        nPlayTone(spPeer, note, duration, volume, tchnl);
    } 
}
