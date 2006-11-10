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

class QSoundMIDITuple
{
    private Object p;
    private int channel;   
    
    QSoundMIDITuple(Object pl, int ch)
    {
        p = pl;
        channel = ch;
    }
    
    Object getKey()
    {
        return p;
    }
    
    int getChannel()
    {
        return channel;
    }
    
    public boolean equals(Object o)
    {
        if(o instanceof QSoundMIDITuple)
        {
            QSoundMIDITuple t = (QSoundMIDITuple)o;
            
            if(t.getKey() == p)
            {
                int ch = t.getChannel();
                if((ch == -1) || (channel == -1))
                    return true;
                else if(ch == channel)
                    return true;
            }
        }

        return super.equals(o);
    }
    
    public String toString()
    {
        return new String("[key="+p+" channel="+channel+"]");
    }

}
