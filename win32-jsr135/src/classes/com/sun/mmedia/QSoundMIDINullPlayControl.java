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

import javax.microedition.media.*;
import javax.microedition.media.control.*;

class QSoundMIDINullPlayControl extends QSoundMIDIPlayControl {    
    
    static private native int nInitVolumeCtrl(int peer);  
    static private native int nInitMIDICtrl(int peer);  
    
    private QSoundVolumeCtrl vc;
    private QSoundMIDICtrl mc;
    
    
    QSoundMIDINullPlayControl(Player p)
    {
        super(p);
    }   
    
    boolean open(boolean forceOpen)
    {
        boolean r = super.open(forceOpen);

        vc = new QSoundVolumeCtrl(nInitVolumeCtrl(peer()), player());
        mc = new QSoundMIDICtrl(nInitMIDICtrl(peer()), player());
        
        return r;
    }
    
    boolean fillBuffer(byte[] b)
    {
        return true;
    }
    
    
    long setMediaTime(long now) throws MediaException
    {
        throw new MediaException("MIDINullPlayer can't set MediaTime");
    }
    
       
    boolean isDone()
    {
        return true;
    }
    
    long getMediaTime()
    {
        return Player.TIME_UNKNOWN;
    }
        
    long getDuration()
    {
        return 0;
    }
    
       
    void setLoopCount(int count) {
    }
    
    Control getControl(String controlType)
    {
        Control r = null;
        
        if (controlType.equals(BasicPlayer.vocName)) {
            r = vc;
        } else if (controlType.equals(BasicPlayer.micName)) {
            r = mc;
        } else if (controlType.equals(BasicPlayer.stcName)) {
            if((player() != null) && (player() instanceof StopTimeControl))
                r = (Control)player();
        }
        
        return r;
    }
    
}
