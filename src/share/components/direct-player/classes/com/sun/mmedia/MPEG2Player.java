/*
 *  Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 *  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License version
 *  2 only, as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  General Public License version 2 for more details (a copy is
 *  included at /legal/license.txt).
 *
 *  You should have received a copy of the GNU General Public License
 *  version 2 along with this work; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 *  02110-1301 USA
 *
 *  Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 *  Clara, CA 95054 or visit www.sun.com if you need additional
 *  information or have any questions.
 */

package com.sun.mmedia;

import javax.microedition.media.Control;
import javax.microedition.media.MediaException;
import javax.microedition.media.Player;

class MPEG2Player extends LowLevelPlayer implements VideoSource {

    private int nativeHandle = 0;
    private DirectVideo directVideo = null;

    MPEG2Player( HighLevelPlayer owner )
    {
        super( owner );
    }

    protected void doNotifySnapshotFinished() {
    }

    protected void doReceiveRSL() {
    }

    protected void doRealize() throws MediaException {
    }

    protected void doPrefetch() throws MediaException {
    }

    private native boolean nStart( int handle );
    private native int nCreate( String locator );
    private native void nDestroy( int handle );
    private native boolean nSetVideoLocation( int handle,
            int x, int y, int w, int h );
    
    protected boolean doStart() {
        nativeHandle = nCreate( getOwner().source.getLocator() );
        return nStart( nativeHandle );
    }

    protected void doPostStart() {
    }

    protected void doStop() throws MediaException {
    }

    protected void doPreStop() {
    }

    protected void doDeallocate() {
    }

    protected void doClose() {
        nDestroy( nativeHandle );
        nativeHandle = 0;
        getOwner().setNativeHandleToNull();
    }

    protected long doSetMediaTime(long now) throws MediaException {
        throw new MediaException( "Not supported" );
    }

    protected long doGetMediaTime() {
        return Player.TIME_UNKNOWN;
    }

    protected long doGetDuration() {
        return Player.TIME_UNKNOWN;
    }

    protected Control doGetNewControl(String type) {
        Control ctl = null;
        if( type.endsWith( HighLevelPlayer.vicName ) )
        {
            directVideo = new DirectVideo( this, 0, 0 );
            ctl = directVideo;
        }
        return ctl;
    }

    protected void doSetStopTime(long time) {
    }

    protected String doGetContentType() {
        return "video/mpeg";
    }

    
    public boolean setVideoLocation(int x, int y, int w, int h) {
        return nSetVideoLocation( nativeHandle, x, y, w, h );
    }


    public byte[] getVideoSnapshot(String imageType) throws MediaException {
        throw new MediaException( "Not supported" );
    }


    public boolean setVideoFullScreen(boolean fullscreen) {
        return false;
    }


    public boolean setVideoVisible(boolean visible) {
        return true;
    }


    public boolean setColorKey(boolean on, int colorKey) {
        return false;
    }


    public void notifyDisplaySizeChange() {}

}