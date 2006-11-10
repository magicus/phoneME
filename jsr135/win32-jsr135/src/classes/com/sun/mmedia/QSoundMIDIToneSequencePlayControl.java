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

class QSoundMIDIToneSequencePlayControl extends QSoundMIDIPlayBase {
         
    static private native int nInitVolumeCtrl(int peer);  
    static private native int nInitPitchCtrl(int peer);
    static private native int nInitRateCtrl(int peer);  
    static private native int nInitTempoCtrl(int peer);  
    
    private QSoundVolumeCtrl vc;
    private QSoundPitchCtrl pc;
    private QSoundRateCtrl rc;
    private QSoundTempoCtrl tc;
        
    QSoundMIDIToneSequencePlayControl(Player p)
    {
        super(p);
    }
    
    boolean open(boolean forceOpen)
    {
        boolean r = super.open(forceOpen);
        
        vc = new QSoundVolumeCtrl(nInitVolumeCtrl(peer()), player());
        pc = new QSoundPitchCtrl(nInitPitchCtrl(peer()));        
        rc = new QSoundRateCtrl(nInitRateCtrl(peer()));
        tc = new QSoundTempoCtrl(nInitTempoCtrl(peer()));
        
        return r;
    }
    
    Control getControl(String controlType)
    {
        Control r = null;

        if (controlType.equals(BasicPlayer.vocName)) {
            r = vc;
        } else if (controlType.equals(BasicPlayer.stcName)) {
            
            if((player() != null) && (player() instanceof StopTimeControl))
                r = (Control)player();

        } else if (controlType.equals(BasicPlayer.picName)) {
            r = pc;
        } else if (controlType.equals(BasicPlayer.racName)) {
            r = rc;
        } else if (controlType.equals(BasicPlayer.tecName)) {
            r = tc;
        } 
        
        return r;
    }
}

