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
import javax.microedition.media.protocol.SourceStream;

class DirectInputThread extends Thread {

    private long curOffset = 0;
    private long offset = 0;
    private int requestedLen = 0;
    private int nativePtr = 0;
    private int hNative;
    private SourceStream stream;
    private boolean isClosed = false;
    private byte[] tmpBuf = new byte [ 1024 ];

    DirectInputThread(int hNative, SourceStream stream) {
        this.hNative = hNative;
        this.stream = stream;
    }
    


    public void run(){

        for(;;)
        {
            synchronized( this )
            {
                if( 0 < requestedLen )
                {
                    int len = requestedLen > tmpBuf.length ?
                                    tmpBuf.length : requestedLen;
                    try {
                        stream.read(tmpBuf, 0, len);
                    } catch (IOException ex) {
                    }
                    // call native copying + javacall_media_written()
                }
                try {
                    this.wait();
                } catch (InterruptedException ex) {
                }

                if( isClosed )
                {
                    break;
                }
            }
            
        }

    }

    public void requestData( long offset, int length, int bufPtr )
    {
        synchronized( this )
        {
            this.offset = offset;
            this.requestedLen = length;
            this.nativePtr = bufPtr;
            this.notify();
        }
    }

}
