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

package com.sun.mmedia.rtsp;

import javax.microedition.media.protocol.SourceStream;
import javax.microedition.media.protocol.ContentDescriptor;
import javax.microedition.media.Control;

public class RtspSS implements SourceStream {

    private String mediaControl;

    public RtspSS( String mediaControl ) {
        this.mediaControl = mediaControl;
    }

    public String getMediaControl() {
        return mediaControl;
    }

    // ===================== SourceStream methods =============

    public ContentDescriptor getContentDescriptor() {
        return null;
    }

    public long getContentLength() {
        return 0;
    }

    public int getSeekType() {
        return NOT_SEEKABLE;
    }

    public int getTransferSize() {
        return -1;
    }

    public int read( byte[] b, int off, int len )
        throws java.io.IOException {

        return -1;
    }

    public long seek( long where ) 
        throws java.io.IOException {
        return 0;
    }

    public long tell() {
        return 0;
    }

    // ===================== Controllable methods =============

    public Control getControl( String controlType ) {
        return null;
    }

    public Control[] getControls() {
        return new Control[] { null };
    }
}
