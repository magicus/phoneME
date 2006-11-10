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

import javax.microedition.media.control.VolumeControl;
import javax.microedition.media.PlayerListener;
import com.sun.mmedia.BasicPlayer;


class VolCtrl implements VolumeControl {
    private int level;
    private boolean muted;
    private BasicPlayer player;

    public  VolCtrl(BasicPlayer p) {
        level = -1;
        muted = false;
        player = p;
    }

    public synchronized void setMute(boolean m) {
        if (m && !this.muted) {
            player.doSetLevel(0);
            this.muted = true;
            player.sendEvent(PlayerListener.VOLUME_CHANGED, this);
        } else if(!m && muted) {
            this.level = player.doSetLevel(level);
            this.muted = false;
            player.sendEvent(PlayerListener.VOLUME_CHANGED, this);
        }
    }

    public synchronized boolean isMuted() {
        return this.muted;
    }

    public synchronized int setLevel(int ll) {
        if (ll < 0) {
            ll = 0;
        } else if (ll > 100) {
            ll = 100;
        } 
    
        if (!this.muted) {
            int newl = player.doSetLevel(ll);
            if (newl != level) {
                this.level = newl;
                player.sendEvent(PlayerListener.VOLUME_CHANGED, this);
            }
        }
        return this.level;
    }

    public synchronized int getLevel() {
        return this.level;
    }
    
    // method needs to be package private, it is used in MIDIPlayer
    final synchronized void setLevelImpl(int l1) {
        level = l1;
    }
}
