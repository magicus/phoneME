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
    boolean lock = false;


    // get java buffer size to determine media format
    protected static native int nGetJavaBufferSize(int handle);
    // get first packet size to determine media format
    protected static native int nGetFirstPacketSize(int handle);
    // buffering media data
    protected static native int nBuffering(int handle, byte[] buffer, int size);
    // ask Native Player if it needs more data immediatelly
    protected static native boolean nNeedMoreDataImmediatelly(int hNative);    
    // Provide whole media content size, if known
    protected static native void nSetWholeContentSize(int hNative, long contentSize);

    synchronized boolean Lock() {
        if (lock) 
            return false;
        lock = true;    
        return true;
    }
    synchronized void Unlock() {
        lock = false;    
    }
    /**
     * The constructor
     *
     * @param  stream  the instance of stream.
     */
    MediaDownload(int hNative, SourceStream stream) {
        this.hNative = hNative;
        this.stream = stream;
        lock = false;
        eom = false;
        contLength = -1;
    }
    
    public void download() throws MediaException, IOException {
        if (contLength == -1) {
            contLength = stream.getContentLength();
            if (contLength > 0) {
                nSetWholeContentSize(hNative, contLength);
            }
        } 
        javaBufSize = nGetJavaBufferSize(hNative);
        packetSize  = nGetFirstPacketSize(hNative);

        if(packetSize > 0 && !eom) {
            do {
                int offset = 0;
                int num_read = packetSize;
                int ret;
                if (buffer == null || packetSize > buffer.length) {
                    buffer = new byte[(int)packetSize];
                }

                do {
                    ret = stream.read(buffer, offset, packetSize-offset);
                    if (ret == -1) {
                        eom = true;
                        break;
                    }
                    num_read -= ret;
                    offset += ret;
                }while(num_read>0);
                packetSize = nBuffering(hNative, buffer, packetSize-num_read);
                if (packetSize == -1) {
                    packetSize = 0;
                    throw new MediaException("Error data buffering or encoding");
                }
            }while (nNeedMoreDataImmediatelly(hNative) && !eom);
            if (eom) {
                packetSize = nBuffering(hNative, null, 0);
            }
        }
    }

    /**
     * Event handling thread.
     */
    public void run() {
        try {
            download();
        } catch(IOException ex1) {
        } catch(MediaException ex2) {
        } finally {
            Unlock();
        }
    }
    
    void deallocate() {
        eom = false;
        contLength = -1;
        buffer = null;
        javaBufSize = 0;
        packetSize = 0;
    }
    
}
