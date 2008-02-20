/*
 *  Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 *  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License version
 *  2 only, as published by the Free Software Foundation.
 *  
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  General Public License version 2 for more details (a copy is
 *  included at /legal/license.txt).
 *  
 *  You should have received a copy of the GNU General Public License
 *  version 2 along with this work; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 *  02110-1301 USA
 *  
 *  Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 *  Clara, CA 95054 or visit www.sun.com if you need additional
 *  information or have any questions.
 */
package com.sun.mmedia;

import javax.microedition.media.*;
import javax.microedition.media.control.VolumeControl;

/**
 *  Description of the Class
 *
 * @created    January 17, 2005
 */
public abstract class AudioRenderer implements VolumeControl {
    /**
     *  Description of the Field
     */
    protected boolean muted;

    /**
     *  Description of the Field
     */
    protected int level;
    
    private BasicPlayer player;


    /**
     *  Description of the Method
     *
     * @param  player  Description of the Parameter
     */
    public void init(BasicPlayer player) {
        this.player = player;
    }


    /**
     *  Sets the mute attribute of the AudioRenderer object
     *
     * @param  mute  The new mute value
     */
    public void setMute(boolean mute) {
        if (mute && !muted) {
            player.doSetLevel(0);
            muted = true;
            player.sendEvent(PlayerListener.VOLUME_CHANGED, this);
        } else if (!mute && muted) {
            level = player.doSetLevel(level);
            muted = false;
            player.sendEvent(PlayerListener.VOLUME_CHANGED, this);
        }
    }


    /**
     *  Gets the muted attribute of the AudioRenderer object
     *
     * @return    The muted value
     */
    public boolean isMuted() {
        return muted;
    }


    /**
     *  Sets the level attribute of the AudioRenderer object
     *
     * @param  ll  The new level value
     * @return     Description of the Return Value
     */
    public int setLevel(int ll) {
        int newl;

        if (ll < 0) {
            ll = 0;
        } else if (ll > 100) {
            ll = 100;
        }

        if (!muted) {
            newl = player.doSetLevel(ll);
            if (newl != level) {
                level = newl;
                player.sendEvent(PlayerListener.VOLUME_CHANGED, this);
            }
        }
        
        return level;
    }


    /**
     *  Gets the level attribute of the AudioRenderer object
     *
     * @return    The level value
     */
    public int getLevel() {
        return level;
    }
}

