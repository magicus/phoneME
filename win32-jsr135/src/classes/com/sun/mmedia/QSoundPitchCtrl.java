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

import javax.microedition.media.control.PitchControl;

public class QSoundPitchCtrl implements PitchControl
{
    private int peer;

    public QSoundPitchCtrl(int pc)
    {
        peer = pc;
    }

    private native int nGetMaxPitch(int peer);
    
    public int getMaxPitch()
    {
        return nGetMaxPitch(peer); 
    }

    private native int nGetMinPitch(int peer);
    
    public int getMinPitch()
    {
        return nGetMinPitch(peer); 
    }

    private native int nGetPitch(int peer);
    
    public int getPitch()
    {
        return nGetPitch(peer); 
    }

    private native int nSetPitch(int peer, int pitch);
    
    public int setPitch(int millisemitones)
    {
        return nSetPitch(peer, millisemitones); 
    }
}

