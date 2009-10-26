/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.mmedia.rtsp;

import java.io.ByteArrayOutputStream;
import javax.microedition.media.protocol.SourceStream;
import javax.microedition.media.protocol.ContentDescriptor;
import javax.microedition.media.Control;

public class SeekableWrapper implements SourceStream {

    private SourceStream ss;
    private int cur_pos        = 0;

    private ByteArrayOutputStream baos = new ByteArrayOutputStream();

    public SeekableWrapper(SourceStream ss) {
        this.ss = ss;
    }

    // ===================== SourceStream methods =============

    public ContentDescriptor getContentDescriptor() {
        return ss.getContentDescriptor();
    }

    public long getContentLength() {
        return ss.getContentLength();
    }

    public int getSeekType() {
        return RANDOM_ACCESSIBLE;
    }

    public int getTransferSize() {
        return ss.getTransferSize();
    }

    public int read(byte[] b, int off, int len) throws java.io.IOException {

        int copy_amount = len;
        int read_amount = 0;
        int n_read      = 0; 

        if( cur_pos + len > baos.size() ) {
            copy_amount = baos.size() - cur_pos;
            read_amount = cur_pos + len - baos.size();
        }

        if( copy_amount > 0 ) {
            byte[] bytes = baos.toByteArray();
            System.arraycopy( bytes, cur_pos, b, off, copy_amount );
            cur_pos += copy_amount;
        }

        if( read_amount > 0 )
        {
            n_read = ss.read(b, off + copy_amount, read_amount); 
            baos.write(b, off + copy_amount, n_read);
            cur_pos += n_read;
        }
        return copy_amount + n_read;
    }

    public long seek(long where) throws java.io.IOException {

        int w = (int)where;

        if( w < 0 ) w = 0;
        if( w <= cur_pos ) {
            cur_pos = w;
        } else {
            byte[] b = new byte[ w - cur_pos ];
            while( cur_pos < w ) {
                int n_read = ss.read(b, 0, w - cur_pos ); 
                baos.write(b, 0, n_read);
                cur_pos += n_read;
            }
        }
        return cur_pos;
    }

    public long tell() {
        return (long)cur_pos;
    }

    // ===================== Controllable methods =============

    public Control getControl(String controlType) {
        return ss.getControl(controlType);
    }

    public Control[] getControls() {
        return ss.getControls();
    }
}
