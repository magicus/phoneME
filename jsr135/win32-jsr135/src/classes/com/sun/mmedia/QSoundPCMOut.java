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

// Wrapper around QSound IWaveStreamer interface...

public class QSoundPCMOut implements PCMAudioOut, QSoundConnectable {
    
    private int peer = 0;  // RingBuffer connected to channel
    private int em_peer = 0; // Effect Module with which to get controls to simulate jsr135
    private QSoundConnectable qc;
    
    private long bytesPerMilliSecond = 0;
    private long byteCount = 0;
    
    private QSoundVolumeCtrl vc;
    private QSoundRateCtrl rc;
    
    static private native int nInitEffectModule(int gm_peer);
    static private native int nInitVolumeCtrl(int peer);
    static private native int nInitRateCtrl(int peer);
    
    private native int nOpen(int sampleRate, int bits, int channels);
    private native int nClose(int peer);
    
    private native int nWrite(int peer, int offset, int len, byte [] data);
    
    /*
     * public access is needed for Class.forName() based instantiation ...
     */
    public QSoundPCMOut() {
        em_peer = nInitEffectModule(QSoundHiddenManager.getGlobalPeer());
        
        vc = new QSoundVolumeCtrl(nInitVolumeCtrl(em_peer));
        rc = new QSoundRateCtrl(nInitRateCtrl(em_peer));
        
        qc = this;
    }
    
    QSoundPCMOut(QSoundConnectable con) {
        em_peer = nInitEffectModule(QSoundHiddenManager.getGlobalPeer());
        
        vc = new QSoundVolumeCtrl(nInitVolumeCtrl(em_peer));
        rc = new QSoundRateCtrl(nInitRateCtrl(em_peer));

        qc = con;
    }
    
    /*
     * PCMAudioOut I/F method
     */
    public boolean open(int sampleRate, int bits, int channels) {
        // IMPL_NOTE: The values of the 2 boolean arguments should be verify
        // may cause bugs in the future !!
        return open(sampleRate, bits, channels, false, false);
    }
    
    /*
     * PCMAudioOut I/F method
     */
    public boolean open(int sampleRate, int bits, int channels,
            boolean signed, boolean bigEndian) {
        
        peer = nOpen(sampleRate, bits, channels);
        
        bytesPerMilliSecond = (sampleRate * (bits >> 3) * channels) / 1000;
        
        if(peer != 0) qc.connect(peer);
        
        return peer != 0;
    }
    
    /*
     * PCMAudioOut I/F method
     */
    public int write(byte [] data, int offset, int len) {
        // System.out.println("PCMOut: off:" + offset + " len: " + len);
        int w = nWrite(peer, offset, len, data);
        byteCount += w;
        
        return w;
    }
    
    /*
     * PCMAudioOut I/F method
     */
    public void pause() {
    }
    
    /*
     * PCMAudioOut I/F method
     */
    public void resume() {
    }
    
    /*
     * PCMAudioOut I/F method
     */
    public void flush() {
        byteCount = 0;
    }
    
    /*
     * PCMAudioOut I/F method
     */
    public int drain() {
        return -1;
    }
    
    /*
     * PCMAudioOut I/F method
     */
    public void drainLoop() {
    }
    
    /*
     * PCMAudioOut I/F method
     */
    public synchronized void close() {
        
        qc.disconnect(peer);
        qc = null;
        
        nClose(peer);
        peer = 0;
    }
    
    
    void impose(QSoundConnectable qcon) {
        if(peer != 0)   /* We were closed while added to a module or the connection is opened */
            qc.disconnect(peer);
        
        qc = qcon;
        
        if(peer != 0)
            qc.connect(peer);
    }
    
    void dispose() {
        if(peer != 0)  /* We were closed while added to a module or the connection is opened */
            qc.disconnect(peer);
        
        qc = this;
        
        if(peer != 0)
            qc.connect(peer);
    }
    
    /*
     * PCMAudioOut I/F method
     */
    public synchronized long getSamplesPlayed() {
        return 0;
    }
    
    /*
     * PCMAudioOut I/F method
     */
    public void setVolume(int level) {
        vc.setLevel(level);
    }
    
    /*
     * PCMAudioOut I/F method
     */
    public int getVolume() {        
        return vc.getLevel();
    }
    
    /*
     * returns media time in milliseconds
     *
     * PCMAudioOut I/F method
     */
    public synchronized long getMediaTime() {
        long millSecs = byteCount / bytesPerMilliSecond;
        
        return millSecs;
    }
    
    /*
     * mediaTime in milliseconds
     *
     * PCMAudioOut I/F method
     */
    public synchronized void setMediaTime(long mediaTime) {

        byteCount = mediaTime * bytesPerMilliSecond;
        
    }
    
    /*
     * PCMAudioOut I/F method
     */
    public void setRate(int rate) {
        rc.setRate(rate);
    }
    
    
    private native void nAddPlayer(int peer, int ciPeer);
    private native void nRemovePlayer(int peer, int ciPeer);
    
    public void connect(int inStreamPeer) {
        nAddPlayer(em_peer, inStreamPeer);
    }
    
    public void disconnect(int inStreamPeer) {
        nRemovePlayer(em_peer, inStreamPeer);
    }
    
}
