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
import com.sun.mmedia.BasicPlayer;
import java.io.*;
import java.util.Vector;
import java.util.Enumeration;

public class QSoundToneSequencePlayer extends BasicPlayer implements Runnable, QSoundMIDIOut, StopTimeControl {
    
    private QSoundMIDIPlayBase qsmc;
    
    private Object playLock = new Object();
    private Thread playThread;
    private boolean stopped;
    
    private QSoundMIDIConnectable qmc;
    private Vector channels;
    
    private QSoundToneCtrl tctrl;
    
    private final int bufferSize = 2048;
        
    public QSoundToneSequencePlayer()
    {
        qsmc = new QSoundMIDIToneSequencePlayControl(this);
        channels = new Vector(10);
        hasDataSource = false;
        
        tctrl = new QSoundToneCtrl(this);
    }

    QSoundToneSequencePlayer(QSoundMIDIPlayBase pb)
    {
        qsmc = pb;
        qsmc.setPlayer(this);
        channels = new Vector(10);
        hasDataSource = false;
        
        tctrl = new QSoundToneCtrl(this);
    }    
    
       /**
     * Subclasses need to implement this to realize
     * the <code>Player</code>.
     *
     * @exception  MediaException  Description of the Exception
     */
    protected void doRealize() throws MediaException
    {   
        qsmc.open();
        
        stopped = true;

        Enumeration e = channels.elements();
        int p = qsmc.peer();
        
        while(e.hasMoreElements())
        {
            QSoundMIDITuple mt = (QSoundMIDITuple)e.nextElement();
            ((QSoundMIDIConnectable)mt.getKey()).connectMIDI(p, mt.getChannel());        
        } 

        if(source != null)
        {
            int count;
            byte[] b = new byte[bufferSize];

            // make use of BAOS, since it takes care of growing buffer 
            ByteArrayOutputStream baos = new ByteArrayOutputStream(bufferSize);        

            try {
                if(stream.tell()> 0)
                    stream.seek(0);

                while ((count = stream.read(b, 0, bufferSize)) > 0)
                    baos.write(b, 0, count);

                boolean r = qsmc.fillBuffer(baos.toByteArray());

                baos.close();

                if(!r) throw new MediaException("Bad Tone Format");
            }
            catch (IOException ioe)
            {
                throw new MediaException("Failure occured with read stream");
            }

            
            baos = null;
        }
    }
    
    /**
     * Subclasses need to implement this to prefetch
     * the <code>Player</code>.
     *
     * @exception  MediaException  Description of the Exception
     */
    protected void doPrefetch() throws MediaException
    {
     
    }
        
    /**
     * Subclasses need to implement this start
     * the <code>Player</code>.
     *
     * @return    Description of the Return Value
     */
    protected boolean doStart()
    {
        /* 
         * TBD: wait until previous thread finishes before starting new one ...
         * Does it make sense ? Or doStop() makes code below redundant ?
        synchronized (playLock) {
            if (playThread != null && playThread.isAlive()) {
                //request thread to stop ex. stopped=true or doStop() ?
                try { playThread.join(); } catch (InterruptedException ie) {};
            }
        }
         */
        
        if(!stopped) 
            try{ doStop(); } catch (MediaException me) {}; // Make sure the we were stopped.
        
        stopped = false;

        synchronized(playLock)
        {
            playThread = new Thread(this);
            playThread.start();
            try { playLock.wait(); } catch (InterruptedException ie) {};
        }
        
        return true;
    }
    
    
    /**
     * Subclasses need to implement this to realize
     * the <code>Player</code>.
     *
     * @exception  MediaException  Description of the Exception
     */
    protected void doStop() throws MediaException
    {
        qsmc.stop();
        stopped = true;
        synchronized(playLock)
        {
            try { playThread.join(); } catch (InterruptedException ie) {};
        }
    }
    
    /**
     * Subclasses need to implement this to deallocate
     * the <code>Player</code>.
     */
    protected void doDeallocate()
    {   

    }
    
    /**
     * Subclasses need to implement this to close
     * the <code>Player</code>.
     */
    protected void doClose()
    {
        if(state != Player.UNREALIZED)
        {   
            if(!stopped) 
                try{ doStop(); } catch (MediaException me) {}; // Make sure the we were stopped.
            
            Enumeration e = channels.elements();
            int p = qsmc.peer();
                
            while(e.hasMoreElements())
            {
                QSoundMIDITuple mt = (QSoundMIDITuple)e.nextElement();
                ((QSoundMIDIConnectable)mt.getKey()).disconnectMIDI(p, mt.getChannel());        
            }
            
            qsmc.close();

            channels.removeAllElements();
            channels = null;
        
            qsmc = null;
        }  
    }
    
    /**
     * Subclasses need to implement this to set the media time
     * of the <code>Player</code>.
     *
     * @param  now                 Description of the Parameter
     * @return                     Description of the Return Value
     * @exception  MediaException  Description of the Exception
     */
    protected long doSetMediaTime(long now) throws MediaException
    {
        return qsmc.setMediaTime(now);
    }
        
        /**
     * Subclasses need to implement this to get the media time
     * of the <code>Player</code>
     *
     * @return    Description of the Return Value
     */
    protected long doGetMediaTime()
    {
        return qsmc.getMediaTime();
    }
    
    /**
     * Subclasses need to implement this to get the duration
     * of the <code>Player</code>.
     *
     * @return    Description of the Return Value
     */
    
    protected long doGetDuration()
    {
        return qsmc.getDuration();
    }
    
    /**
     * The worker method to actually obtain the control.
     *
     * @param  type  the class name of the <code>Control</code>.
     * @return       <code>Control</code> for the class or interface
     * name.
     */
    protected Control doGetControl(String controlType)
    {  
        Control r = null;
        
        if ((getState() != UNREALIZED) && controlType.startsWith(BasicPlayer.pkgName)) {
            
            controlType = controlType.substring(BasicPlayer.pkgName.length());
            
            if (controlType.equals(BasicPlayer.tocName)) {
                r = tctrl;
            } else 
            {
                r = qsmc.getControl(controlType);
            }
        }
        
        return r;                
    }
    
    void doNextLoopIteration() { }
    void doFinishLoopIteration() { }
        
    protected void doSetLoopCount(int count) {
        qsmc.setLoopCount(count);
    }
    
    public void run() {
        qsmc.start();
 
        synchronized(playLock) { playLock.notify(); }
        
        boolean done = false;
        boolean stoppedAtTime = false;
        int numLoopComplete = 0;
        
        while(!stopped)
        {
            try { Thread.sleep(500); } catch (InterruptedException ie) {};
            
            done = qsmc.isDone();
            numLoopComplete = qsmc.numLoopComplete();
            
            if(!done && (numLoopComplete > 0))
            {
                Long medt = new Long(qsmc.getMediaTime());
                while(numLoopComplete-- > 0)
                {
                    updateTimeBase(false);
                    sendEvent(PlayerListener.END_OF_MEDIA, medt);
                }
            }
            
            stoppedAtTime = ((stopTime != StopTimeControl.RESET) && 
                             (stopTime < doGetMediaTime()));
            
            if (done || stoppedAtTime)
                stopped = true;
        }
            
        if(done) {
            updateTimeBase(false);
            state = Player.PREFETCHED;
            sendEvent(PlayerListener.END_OF_MEDIA, new Long(qsmc.getMediaTime()));
        }
        else if (stoppedAtTime) {
            qsmc.stop(); 
            satev();
        } else {
            // stopped due to user request: caller ( BasicPlayer.stop() )
            // will call appropriate sentEvent(STOPPED, ...);
        }
        
    }
    
    public boolean impose(QSoundMIDIConnectable qcon, int ch)
    {
        QSoundMIDITuple mt = new QSoundMIDITuple(qcon, ch);
        
        if(channels.contains(mt))
            return false;
        
        channels.addElement(mt);
         
        if(state != Player.UNREALIZED)
            qcon.connectMIDI(qsmc.peer(), ch);  
      
        return true;
    }
    
    
    public boolean dispose(QSoundMIDIConnectable qcon)
    {
        if(qsmc == null)
            return true;
        
        int p = qsmc.peer();
        
        if(p == 0)
            return true;

        QSoundMIDITuple tmptuple = new QSoundMIDITuple(qcon, -1);

        int l;
        while((l = channels.lastIndexOf(tmptuple)) >= 0)
        {               
            QSoundMIDITuple mt = (QSoundMIDITuple)channels.elementAt(l);
            ((QSoundMIDIConnectable)mt.getKey()).disconnectMIDI(p, mt.getChannel());        
            channels.removeElement(mt);
        }

        return true;                
    }
    
    public boolean dispose(QSoundMIDIConnectable qcon, int ch)
    {
        if(qsmc == null)
            return true;
        
        int p = qsmc.peer();
        
        if(p == 0)
            return true;

        QSoundMIDITuple tmptuple = new QSoundMIDITuple(qcon, ch);

        int l;
        while((l = channels.lastIndexOf(tmptuple)) >= 0)
        {               
            QSoundMIDITuple mt = (QSoundMIDITuple)channels.elementAt(l);
            ((QSoundMIDIConnectable)mt.getKey()).disconnectMIDI(p, mt.getChannel());        
            channels.removeElement(mt);
        }

        return true;                
    }
    
    
    public int getAudioType() {
        return AUDIO_MIDI;
    }
    
    public Object getOutput() { return this; }


    boolean setSequence(byte []seq) throws IllegalArgumentException
    {        
        source = null;
        
        if(state == UNREALIZED)
            qsmc.open();
        
        return qsmc.fillBuffer(seq);

    }

    public String getContentType() {
        chkClosed(true);
        return ((source != null) 
            ? source.getContentType() 
            : DefaultConfiguration.MIME_AUDIO_TONE);
    }
    
    public boolean equals(Object o)
    {
        if(o instanceof QSoundMIDITuple)
        {
            QSoundMIDITuple t = (QSoundMIDITuple)o;
            
            return channels.contains(t);
        }

        return super.equals(o);
    }
    
    
}

