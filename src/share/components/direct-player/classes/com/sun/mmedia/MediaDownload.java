/*
 * 
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
import  javax.microedition.media.*;
import  javax.microedition.media.protocol.SourceStream;
import java.io.IOException;

/**
 * The thread that's downloads media data
 *
 */
class MediaDownload implements Runnable {
    /**
     * the stream instance
     */
    private SourceStream stream;
    long contLength = -1;
    int packetSize = 0;
    int javaBufSize = 0; /* unused in current version */
    byte[] buffer = null;
    boolean eom = false;
    int hNative;
    boolean needMoreData = false;


    // get java buffer size to determine media format
    protected static native int nGetJavaBufferSize(int handle);
    // get first packet size to determine media format
    protected static native int nGetFirstPacketSize(int handle);
    // buffering media data
    protected static native int nBuffering(int handle, byte[] buffer, int offset, int size);
    // ask Native Player if it needs more data immediatelly
    protected static native boolean nNeedMoreDataImmediatelly(int hNative);    
    // Provide whole media content size, if known
    protected static native void nSetWholeContentSize(int hNative, long contentSize);

    /**
     * The constructor
     *
     * @param  stream  the instance of stream.
     */
    MediaDownload(int hNative, SourceStream stream) {
        this.hNative = hNative;
        this.stream = stream;
        eom = false;
        contLength = -1;
    }
    
    synchronized public void download(boolean inBackground) throws MediaException, IOException {
        int roffset = 0;
        int woffset = 0;

        if (contLength == -1) {
            contLength = stream.getContentLength();
            if (contLength > 0) {
                nSetWholeContentSize(hNative, contLength);
            }
        }
       
        int newJavaBufSize = nGetJavaBufferSize(hNative);
        packetSize  = nGetFirstPacketSize(hNative);
        
        if(packetSize > 0 && !eom) {
            
            if (newJavaBufSize < packetSize) {
                newJavaBufSize = packetSize;
            }
            
            if (buffer == null || newJavaBufSize > buffer.length) {
                buffer = new byte[(int)newJavaBufSize];
                javaBufSize = newJavaBufSize;
            }

            if (inBackground) {
                woffset = BgDownloadAndWait(roffset, woffset);
            }

            do {
                int num_read = woffset - roffset;
                int ret;
                if (num_read < packetSize && !eom) {
                    if ((roffset + packetSize) > javaBufSize) {
                        woffset = moveBuff(roffset, woffset);
                        roffset = 0;
                    }
                    do {
                        ret = stream.read(buffer, woffset, packetSize-num_read);
                        if (ret == -1) {
                            eom = true;
                            break;
                        }
                        num_read += ret;
                        woffset += ret;
                    }while(num_read<packetSize);
                }
                
                packetSize = nBuffering(hNative, buffer, roffset, num_read);
                roffset += num_read;
                if (packetSize == -1) {
                    packetSize = 0;
                    needMoreData = false;
                    throw new MediaException("Error data buffering or encoding");
                } else if (packetSize > javaBufSize){
                    if ((woffset - roffset)==0) {
                        javaBufSize = packetSize;
                        buffer = new byte[(int)javaBufSize];
                    } else {
                        javaBufSize = packetSize;
                        byte b[] = new byte[(int)javaBufSize];
                        for (int i=0, j=roffset; j<woffset; i++, j++) {
                            b[i] = buffer[j];
                        }
                        buffer = b;
                        woffset -= roffset;
                        roffset = 0;
                    }
                }
                if (roffset == woffset) {
                    roffset = 0;
                    woffset = 0;   
                }
                needMoreData = nNeedMoreDataImmediatelly(hNative);
                if (inBackground && !needMoreData) {
                    woffset = moveBuff(roffset, woffset);
                    roffset = 0;
                    woffset = BgDownloadAndWait(roffset, woffset);
                }
            }while (needMoreData && !eom);
            if (eom) {
                packetSize = nBuffering(hNative, null, 0, 0);
                needMoreData = false;
            }
        }
    }

    private int moveBuff(int roffset, int woffset) {
        for (int i=0, j=roffset; j<woffset; i++, j++) {
            buffer[i] = buffer[j];
        }
        return woffset-roffset;
    }

    private int BgDownloadAndWait(int roffset, int woffset) throws IOException {
        while (!needMoreData) {
            if (woffset<javaBufSize && !eom) {
                int num_read = packetSize;
                if (woffset + num_read >javaBufSize) {
                    num_read = javaBufSize - woffset;
                }
                int ret = stream.read(buffer, woffset, num_read);
                if (ret == -1) {
                    eom = true;
                } else {
                    woffset += ret;
                }
            } else {
                try {
                    wait(500);
                } catch (Exception e) {}
            }
        };
        return woffset;
    }
    
    /**
     * Event handling thread.
     */
    public void run() {
        try {
            download(true);
        } catch(IOException ex1) {
        } catch(MediaException ex2) {
        }
    }
    
    void deallocate() {
        eom = false;
        contLength = -1;
        buffer = null;
        javaBufSize = 0;
        packetSize = 0;
    }
    
    /**
     * 
     */
    public void fgDownload() throws IOException, MediaException {
        download(false);
    }

    /**
     * 
     */
    public boolean bgDownload() {
        if (!eom) {
            try {
                new Thread(this).start();
            } catch (Exception e) {
                return false;
            }
        }
        return true;
    }
    
    synchronized public void continueDownload() {
        needMoreData = true;
        notifyAll();
    }
}
