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

import javax.microedition.media.control.MIDIControl;
import javax.microedition.media.Player;
import javax.microedition.media.MediaException;



public class QSoundMIDICtrl implements MIDIControl
{
    private int peer;
    private Player player;
    
    public QSoundMIDICtrl(int mc)
    {
        peer = mc;
    }

    public QSoundMIDICtrl(int mc, Player p)
    {
        peer = mc;
        player = p;
    }
            
    private native int nGetBankList(int peer, boolean custom, int[] bankList);
    
    public int[] getBankList(boolean custom)
    {
        if((player != null) && (player.getState() < Player.PREFETCHED))
            throw new IllegalStateException();
        
        int[] bl = new int[200];
        
        int len = nGetBankList(peer, custom, bl); 

        if(len == -1)
            throw new IllegalArgumentException();
        
        int[] rbl = new int[len];

        for(int i = 0; i<len; i++)
            rbl[i] = bl[i];
        
        return rbl;
    }

    private native int nGetChannelVolume(int peer, int channel);
    
    public int getChannelVolume(int channel)
    {
        if((player != null) && (player.getState() < Player.PREFETCHED))
            throw new IllegalStateException();

        int v = nGetChannelVolume(peer, channel); 
        
        if(v == -2)
            throw new IllegalArgumentException();
        
        return v;
    }

    private native int nGetKeyName(int peer, int bank, int prog, int key, byte[] s);
    
    public String getKeyName(int bank, int prog, int key) throws MediaException
    {
        if((player != null) && (player.getState() < Player.PREFETCHED))
            throw new IllegalStateException();

        byte[] st = new byte[50];
        
        int len = nGetKeyName(peer, bank, prog, key, st);
        
        if(len == -2)
            throw new MediaException();
        
        if(len == -1)
            throw new IllegalArgumentException();
        
        return new String(st, 0, len);
    }

    private native boolean nGetProgram(int peer, int channel, int[] program);
    
    public int[] getProgram(int channel)
    {
        if((player != null) && (player.getState() < Player.PREFETCHED))
            throw new IllegalStateException();

        int[] program = new int[2];

        if(!nGetProgram(peer, channel, program))
            throw new IllegalArgumentException();
        
        return program;
    }
    
    private native int nGetProgramList(int peer, int bank, int[] programList);
    
    public int[] getProgramList(int bank)
    {
        if((player != null) && (player.getState() < Player.PREFETCHED))
            throw new IllegalStateException();

        int[] pl = new int[200];

        int len = nGetProgramList(peer, bank, pl);
        
        if(len == -1)
            throw new IllegalArgumentException();
        
        int[] rpl = new int[len];

        for(int i = 0; i<len; i++)
            rpl[i] = pl[i];
        
        return rpl;
    }
    
    private native int nGetProgramName(int peer, int bank, int prog, byte[] s);
    
    public String getProgramName(int bank, int prog) throws MediaException
    {
        if((player != null) && (player.getState() < Player.PREFETCHED))
            throw new IllegalStateException();

        byte[] st = new byte[50];
        
        int len = nGetProgramName(peer, bank, prog, st);
        
        if(len == -2)
            throw new MediaException();
        
        if(len == -1)
            throw new IllegalArgumentException();
        
        return new String(st, 0, len);
    }   
    
    private native boolean nIsBankQuerySupported(int peer);
    
    public boolean isBankQuerySupported()
    {
        return nIsBankQuerySupported(peer);
    }
    
    private native int nLongMidiEvent(int peer, byte[] data, int offset, int length);
    
    public int longMidiEvent(byte[] data, int offset, int length)
    {
        if((player != null) && (player.getState() < Player.PREFETCHED))
            throw new IllegalStateException();
        
        int l = nLongMidiEvent(peer, data, offset, length);
        
        if(l == -2)
            throw new IllegalArgumentException();
        
        return l;
    }
    
    private native boolean nSetChannelVolume(int peer, int channel, int volume);
    
    public void setChannelVolume(int channel, int volume)
    {
        if((player != null) && (player.getState() < Player.PREFETCHED))
            throw new IllegalStateException();

        int tries = 5;
        boolean noError = true;
        while((tries-- > 0) && (noError = nSetChannelVolume(peer, channel, volume)))
        {   
            /* This is needed because an mQ234_Read is needed to
             * cause this command to executed. The mQ234_Read
             * will happned on the next poll in the runRenderThread
             */
            
            if(getChannelVolume(channel) != volume) 
            {
                try {Thread.sleep(20); } catch (InterruptedException ie) {};
            }
            else
            {
                break;
            }
        }
        
        if(!noError)
            throw new IllegalArgumentException();
    }
    
    private native boolean nSetProgram(int peer, int channel, int bank, int program);
    
    public void setProgram(int channel, int bank, int program)
    {
        if((player != null) && (player.getState() < Player.PREFETCHED))
            throw new IllegalStateException();

        if(!nSetProgram(peer, channel, bank, program))
            throw new IllegalArgumentException();
        
        // The mQCore engine requires an mQ234_Read to pull the setProgram
        // so a slight pause is necessary for this to take place.
        int[] data = new int[2]; 
        for(int i=0; i < 5; i++)
        {
            try {Thread.sleep(15);} catch (InterruptedException ie){};
            
            if(nGetProgram(peer, channel, data) && (data[0] == bank) && (data[1] == program))
                break;
        }
    }
    
    private native boolean nShortMidiEvent(int peer, int type, int data1, int data2);
    
    public void shortMidiEvent(int type, int data1, int data2)
    {
        if((player != null) && (player.getState() < Player.PREFETCHED))
            throw new IllegalStateException();
        
        if(!nShortMidiEvent(peer, type, data1, data2))
            throw new IllegalArgumentException();

        // The mQCore engine requires an mQ234_Read to pull the shortMIDIEvent
        // so a slight pause is necessary for this to take place.
        try {Thread.sleep(30);} catch (InterruptedException ie) {};
    }

}

