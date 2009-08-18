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
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.microedition.media.MediaException;
import javax.microedition.media.protocol.SourceStream;
import java.lang.ref.WeakReference;

class DirectInputThread extends Thread {

    //Caution: these fields are used by native (read-write) !!!
    private volatile long curPos = 0;
    private volatile long posToRead = 0;
    private volatile int lenToRead = 0;

    private boolean requestPending = false;

    private WeakReference wrPlayer;

    private byte[] tmpBuf = new byte [ 0x1000 ];  // 4 Kbytes
    private volatile boolean isDismissed = false;

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
        wrPlayer = new WeakReference( p );
    }
    

    private native void nWriteData( byte [] buf, int len, int handle );
    private native void nGetRequestParams( int handle );
    private native void nNotifyEndOfStream( int handle );
    private native void nNotifyStreamLen( int handle, long len );

    public void run(){

mainloop:
        for(;;) {
            HighLevelPlayer owner = null;

            synchronized( requestLock ) {
                while ( !requestPending )
                {
                    try {
                        requestLock.wait(1000);
                    } catch (InterruptedException ex) {}
                    if( isDismissed ) {
                        return;
                    }

                    owner = (HighLevelPlayer) wrPlayer.get();
                    if (null == owner) {
                        return;
                    }

                }
                requestPending = false;
            }

            nGetRequestParams( owner.getNativeHandle() );

            while( 0 < lenToRead ) {
                int read = 0;
                
                if( isDismissed ) {
                    return;
                }

                try {
                    seek( owner.stream );

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

                owner = (HighLevelPlayer) wrPlayer.get();
                if ( null == owner || isDismissed ) {
                    return;
                }
                
                if( -1 == read ) {
                    nNotifyEndOfStream( owner.getNativeHandle() );
                }
                else {
                    nWriteData( tmpBuf, read, owner.getNativeHandle() );
                }
            }
        }
    }

    private void seek( SourceStream stream ) throws IOException, MediaException {
        if( stream.tell() == posToRead ) {
            return;
        }
        if( stream.getSeekType() ==
                SourceStream.NOT_SEEKABLE ||
            ( posToRead != 0 &&
              stream.getSeekType() !=
                SourceStream.RANDOM_ACCESSIBLE ) ) {
            throw new MediaException("The stream is not seekable");
        }

        stream.seek(posToRead);
    }

    public void requestData() {
        synchronized( requestLock ) {
            requestPending = true;
            requestLock.notify();
        }
    }

    public void close() {
        isDismissed = true;
        synchronized (requestLock) {
            requestLock.notify();
        }
        try {
            this.join();
        } catch (InterruptedException ex) {}
    }

}
