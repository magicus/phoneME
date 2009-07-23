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

    private volatile long curPos = 0;
    private volatile long posToRead = 0;
    private volatile int sizeToRead = 0;
    private volatile int nativePtr = 0;
    private int read;
    private HighLevelPlayer owner;
    private byte[] tmpBuf = new byte [ 1024 ];
    private final Object dismissLock = new Object();
    private boolean isDismissed = false;

    DirectInputThread(HighLevelPlayer p) {
        this.owner = p;
    }
    

    private native void nWriteData();

    public void run(){

        for(;;)
        {
            synchronized( this )
            {
                while( 0 < sizeToRead )
                {
                    try {
                        seek();
                        int len = sizeToRead > tmpBuf.length ?
                                        tmpBuf.length : sizeToRead;
                        read = owner.stream.read(tmpBuf, 0, len);
                    } catch ( MediaException ex) {
                        owner.abort( ex.getMessage() );
                        break;
                    } catch ( IOException e )
                    {
                        owner.abort("Stream reading IOException: "
                                + e.getMessage() );
                        break;
                    }

                    synchronized( dismissLock )
                    {
                        if( isDismissed )
                        {
                            break;
                        }
                    }
                    // call native copying + javacall_media_written()
                    nWriteData();
                }

                synchronized( dismissLock )
                {
                    if( isDismissed )
                    {
                        break;
                    }
                }
                try {
                    this.wait();
                } catch (InterruptedException ex) {
                    owner.abort("Stream reading thread was interrupted");
                    break;
                }

            }
            
            synchronized( dismissLock )
            {
                if( isDismissed )
                {
                    break;
                }
            }
        }

    }

    private void seek() throws IOException, MediaException
    {
        if( owner.stream.tell() == posToRead )
        {
            return;
        }
        if( owner.stream.getSeekType() ==
                SourceStream.NOT_SEEKABLE ||
            ( posToRead != 0 &&
              owner.stream.getSeekType() !=
                SourceStream.RANDOM_ACCESSIBLE ) )
        {
            throw new MediaException("The stream is not seekable");
        }

        owner.stream.seek(posToRead);
    }

    public void requestData( long position, int length, int bufPtr )
    {
        synchronized( this )
        {
            this.posToRead = position;
            this.sizeToRead = length;
            this.nativePtr = bufPtr;
            this.notify();
        }
    }

    public void dismiss()
    {
        synchronized( dismissLock )
        {
            isDismissed = true;
        }
    }
}
