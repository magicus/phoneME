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

abstract class QSoundMIDIPlayBase {
    
    private int qsMIDI;
    private int globMan;
        
    private Player player;
    
    private int loopCount;
    private int currentLoopCount;
    private int lastLoopCount;
    
    private native int nOpen(int gm);
    private native void nClose(int gm, int qsMpeer);
    
    private native boolean nFillBuffer(int gm, int qsMpeer, byte[] buffer);
    private native void nDeallocateBuffer(int qsMpeer);
    
    private native void nStart(int qsMpeer);
    private native void nStop(int qsMpeer);
    private native boolean nIsDone(int qsMpeer);
    private native int nLoopsDone(int qsMpeer);
    
    private native int nSetPosition(int peer, int now);
    private native int nGetPosition(int peer);        
    private native int nGetDuration(int peer);
    private native void nSetLoopCount(int peer, int count);
    
    private boolean opened;
    
    QSoundMIDIPlayBase()
    {
        globMan = QSoundHiddenManager.getMIDIGlobalPeer();
        opened = false;
    }
        
    QSoundMIDIPlayBase(Player p)
    {
        globMan = QSoundHiddenManager.getMIDIGlobalPeer();
        player = p;
        opened = false;
    }

    abstract Control getControl(String controlType);
    
    boolean open()
    {
        return open(false);
    }
    
    boolean open(boolean forceOpen)
    {
        if(opened && !forceOpen)
            return opened;
        
        qsMIDI = nOpen(globMan);        
           
        opened = true;
        return opened;
    }
    
    void close()
    {
        nDeallocateBuffer(qsMIDI);
        nClose(globMan, qsMIDI);
        qsMIDI = 0;    
    }
    
    boolean fillBuffer(byte[] b)
    {
        boolean r = nFillBuffer(globMan, qsMIDI, b);
        
        if(r)
        {
            if(loopCount != 0) nSetLoopCount(qsMIDI, loopCount);
        }
        
        return r;
    }
    
    void start()
    {   
        lastLoopCount = loopCount == -1 ? 0 : loopCount;
        nStart(qsMIDI);
    }
    
    void stop()
    {   
        nStop(qsMIDI);
    }
    
    boolean isDone()
    {
        boolean r = nIsDone(qsMIDI);
        
        if(!r && (loopCount != 0))
            currentLoopCount = nLoopsDone(qsMIDI);
        
        return r;
    }
    
    long setMediaTime(long now) throws MediaException
    {
        if(qsMIDI == 0) return 0;
        
        int pos = nSetPosition(qsMIDI, ((int)now/100));
        
        return ((long)pos) * 100L;
    }
    
    
    long getMediaTime()
    {        
        if(qsMIDI == 0) return Player.TIME_UNKNOWN;
        
        return ((long)(nGetPosition(qsMIDI))) * 100L;        
    }
        
    long getDuration()
    {
        if(qsMIDI == 0) return Player.TIME_UNKNOWN;
                
        return ((long)(nGetDuration(qsMIDI))) * 100L;
    }
        
       
    void setLoopCount(int count) {
        loopCount = count;
        nSetLoopCount(qsMIDI, loopCount);
    }
    
    
    int numLoopComplete()
    {
        int numLoops = currentLoopCount - lastLoopCount;
        
        lastLoopCount = currentLoopCount;
                        
        return numLoops;
    }
      
    Player player()
    {
        return player;
    }
    
    void setPlayer(Player p)
    {
        player = p;
    }
    
    int peer()
    {
        return qsMIDI;
    }
}
