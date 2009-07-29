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

import java.io.IOException;
import javax.microedition.media.MediaException;
import javax.microedition.media.protocol.SourceStream;

class DirectInputThread extends Thread {

    //Caution: these fields are used by native (read-write) !!!
    private volatile long curPos = 0;
    private volatile long posToRead = 0;
    private volatile int lenToRead = 0;
    private volatile int nativePtr = 0;


    private boolean requestPending = false;
    private long    reqPos = 0;
    private int     reqLen = 0;
    private int     reqBufPtr = 0;

    final private HighLevelPlayer owner;
    private byte[] tmpBuf = new byte [ 0x1000 ];  // 4 Kbytes
    private volatile boolean isDismissed = false;
    private boolean isFrozen = false;

    private final Object dataWriteLock = new Object();
    private final Object requestLock = new Object();

    DirectInputThread(HighLevelPlayer p) throws MediaException {
        long len = p.stream.getContentLength();
        if( -1 != len ) {
            nNotifyStreamLen( p.getNativeHandle(), len );
        }
        else {
            throw new MediaException(
                    "Cannot playback stream with unknown length" );
        }
        this.owner = p;
    }
    

    private native void nWriteData( byte [] buf, int len, int handle );
    private native void nGetRequestParams( int handle );
    private native void nNotifyEndOfStream( int handle );
    private native void nNotifyStreamLen( int handle, long len );

    public void run(){

mainloop:
        for(;;) {
            if( isDismissed ) {
                return;
            }

            synchronized( requestLock ) {
                if( requestPending ) {
                    requestPending = false;
                }
                else {
                    try {
                        requestLock.wait();
                    } catch (InterruptedException ex) {
                        //owner.abort("Stream reading thread was interrupted");
                        return;
                    }
                    continue;
                }
            }

            nGetRequestParams( owner.getNativeHandle() );

            while( 0 < lenToRead ) {
                int read = 0;
                
                if( isDismissed ) {
                    return;
                }

                try {
                    seek();

                    if( isDismissed ) {
                        return;
                    }

                    int len = lenToRead > tmpBuf.length ?
                                    tmpBuf.length : lenToRead;
                    read = owner.stream.read(tmpBuf, 0, len);
                } catch ( MediaException ex) {
                    owner.abort( ex.getMessage() );
                    return;
                } catch ( IOException e ) {
                    owner.abort("Stream reading IOException: "
                            + e.getMessage() );
                    return;
                }

                synchronized( dataWriteLock ) {
                    if( isDismissed ) {
                        return;
                    }

                    if( isFrozen ) {
                        try {
                            dataWriteLock.wait();
                        } catch (InterruptedException ex) {}
                        continue mainloop;
                    }
                    else {
                        if( -1 == read ) {
                            nNotifyEndOfStream( owner.getNativeHandle() );
                        }
                        else {
                            nWriteData( tmpBuf, read, owner.getNativeHandle() );
                        }
                    }
                }
            }
        }
    }

    private void seek() throws IOException, MediaException {
        if( owner.stream.tell() == posToRead ) {
            return;
        }
        if( owner.stream.getSeekType() ==
                SourceStream.NOT_SEEKABLE ||
            ( posToRead != 0 &&
              owner.stream.getSeekType() !=
                SourceStream.RANDOM_ACCESSIBLE ) ) {
            throw new MediaException("The stream is not seekable");
        }

        owner.stream.seek(posToRead);
    }

    public void requestData() {
        synchronized( requestLock ) {
            requestPending = true;
            requestLock.notify();
            System.out.println( "DirectInputThread: data requested" );
        }
    }

    public void close() {
        isDismissed = true;
        synchronized (requestLock) {
            requestLock.notify();
        }
        synchronized( dataWriteLock ) {
            dataWriteLock.notify();
        }
        try {
            this.join();
        } catch (InterruptedException ex) {}
    }


    // this method may take some time to execute
    public void stopWritingAndFreeze() {
        synchronized( dataWriteLock ) {
            isFrozen = true;
        }
    }

    public void unfreeze() {
        synchronized( dataWriteLock ) {
            isFrozen = false;
            dataWriteLock.notify();
        }
    }
}
