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

    private long streamLen = -1;

    DirectInputThread(HighLevelPlayer p) throws MediaException {
        wrPlayer = new WeakReference( p );
        streamLen = p.stream.getContentLength();
    }
    

    private native void nWriteData( byte [] buf, int len, int handle );
    private native void nGetRequestParams( int handle );
    private native void nNotifyEndOfStream( int handle, long pos );

    public void run(){

mainloop:
        for(;;) {
            HighLevelPlayer owner = (HighLevelPlayer) wrPlayer.get();
            if (null == owner || isDismissed ) {
                //System.out.println( "DirectInputThread: exit 1" );
                return;
            }

            synchronized( requestLock ) {
                while ( !requestPending )
                {
                    try {
                        requestLock.wait(1000);
                    } catch (InterruptedException ex) {}
                    if( isDismissed ) {
                        //System.out.println( "DirectInputThread: exit 2" );
                        return;
                    }

                    owner = (HighLevelPlayer) wrPlayer.get();
                    if (null == owner) {
                        //System.out.println( "DirectInputThread: exit 3" );
                        return;
                    }

                }
                requestPending = false;
            }

            nGetRequestParams( owner.getNativeHandle() );

            while( 0 < lenToRead ) {
                int read = 0;
                
                if( isDismissed ) {
                    //System.out.println( "DirectInputThread: exit 4" );
                    return;
                }

                try {
                    if( !seek( owner ) ) {
                        continue mainloop;
                    }

                    if( isDismissed ) {
                        //System.out.println( "DirectInputThread: exit 5" );
                        return;
                    }

                    int len = lenToRead > tmpBuf.length ?
                                    tmpBuf.length : lenToRead;
                    read = owner.stream.read(tmpBuf, 0, len);
                } catch ( MediaException ex) {
                    if (!isDismissed) {
                        owner.doOnDirectInputError( ex.getMessage() );
                    }
                    System.out.println( "DirectInputThread: exit 6" );
                    return;
                } catch ( IOException e ) {
                    if( !isDismissed ) {
                        owner.doOnDirectInputError("Stream reading IOException: " + e.getMessage());
                    }
                    System.out.println( "DirectInputThread: exit 7" );
                    return;
                } catch ( RuntimeException t ) {
                    System.out.println( "DirectInputThread: unexpected RuntimeException ");
                    t.printStackTrace();
                    throw t;
                } catch ( Error e ) {
                    System.out.println( "DirectInputThread: unexpected Error ");
                    e.printStackTrace();
                    throw e;
                }

                owner = (HighLevelPlayer) wrPlayer.get();
                if ( null == owner || isDismissed ) {
                    //System.out.println( "DirectInputThread: exit 8" );
                    return;
                }
                
                if( -1 == read ) {
                    nNotifyEndOfStream( owner.getNativeHandle(), posToRead );
                    streamLen = posToRead;
                    continue mainloop;
                }
                else {
                    nWriteData( tmpBuf, read, owner.getNativeHandle() );
                }
            }
        }
    }

    // returns false if failed to seek because end of stream was reached
    //                          (and the stream length was being unknown)
    // returns true otherwise
    private boolean seek( HighLevelPlayer p ) throws IOException, MediaException {
        SourceStream stream = p.stream;

        if( stream.tell() == posToRead ) {
            return true;
        }

        if( posToRead < 0 || ( streamLen != -1 && posToRead > streamLen ) ) {
            throw new MediaException( "Requested position is negative or " +
                    "beyond the stream length");
        }
        
        if( stream.getSeekType() ==
                SourceStream.NOT_SEEKABLE ||
            ( posToRead != 0 &&
              stream.getSeekType() !=
                SourceStream.RANDOM_ACCESSIBLE ) ) {
            throw new MediaException("The stream is not seekable");
        }

        final long seekResult = stream.seek( posToRead );
        if( seekResult != posToRead ) {
            byte [] buf = new byte[ 1 ];
            if( -1 == stream.read(buf, 0, 1) ) {
                streamLen = seekResult;
                nNotifyEndOfStream( p.getNativeHandle(), seekResult );
                return false;
            } else {
                throw new MediaException( "Failed to seek to the specified " +
                        "position" );
            }
        }
        return true;
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
    }

}
