/*
 * 
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

package com.sun.mmedia;

import java.util.*;
import java.lang.IllegalArgumentException;
import java.lang.IllegalStateException;
import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Item;
import javax.microedition.lcdui.Canvas;
import javax.microedition.lcdui.CustomItem;
import javax.microedition.media.Control;
import javax.microedition.media.MediaException;
import javax.microedition.media.control.VideoControl;
import javax.microedition.media.PlayerListener;

import com.sun.j2me.log.Logging;
import com.sun.j2me.log.LogChannels;

/**
 * Video direct player
 * it implements VideoControl
 */
public class DirectVideo extends DirectPlayer implements 
    VideoControl, MIDPVideoPainter {

/* Need to revisit: these can swap when device screen orientation changes,
 * so they cannot be final 
 */
/* Need to revisit: native methods return 0 */
    private final int SCREEN_WIDTH = 240;//nGetScreenWidth();
    private final int SCREEN_HEIGHT = 240;//nGetScreenHeight();
    private final int DEFAULT_WIDTH = 80;
    private final int DEFAULT_HEIGHT = 80;
    private final int ALPHA_COLOR = 1;

    // NOTE: You have to calibrate this value carefully
    //       If you increase this value, fake preview quality goes down but, system overhead decrease
    //       If you decrease this value, fake preview quality goes up but, system overhead increase
    //       If you set this value 0 or negative value, fake preview support still image only
    private final static int FAKE_PREVIEW_INTERVAL = 250;
    
    // Canvas and item reference
    private Canvas canvas;
    private DVItem item;

    // original video size    
    private int sw;
    private int sh;

    // Display position and size + temporary values for fullscreen mode
    private int dx, tmp_dx;
    private int dy, tmp_dy;
    private int dw, tmp_dw;
    private int dh, tmp_dh;

    // Fullscreen mode flag
    private boolean fsmode = false;

    // visible?
    private boolean visible = false;
    private boolean hidden = false;
    private boolean started = false;
    private boolean locationInited = false; // Is location initialized?
    
    // current display mode
    private int displayMode = -1;
    // MMHelper to communicate with Canvas
    private MMHelper mmh = null;
    // Lock
    private Object boundLock = new Object();

    // native functions /////////////////////////////////////////////

    // Get video width
    protected native int nGetWidth(int handle);
    // Get video height
    protected native int nGetHeight(int handle);
    // Set display location of video
    protected native boolean nSetLocation(int handle, int x, int y, int w, int h);
    // Get snapshot
    protected native byte[] nSnapShot(int handle, String imageType);
    // Set fullscreen
    protected native boolean nSetFullScreenMode(int handle, boolean fullscreen);
    // Set visible
    protected native boolean nSetVisible(int handle, boolean visible);
    // Get screen full width
    private native int nGetScreenWidth();
    // Get screen full height
    private native int nGetScreenHeight();
    // Turn on or off alpha channel
    private native int nSetAlpha(int handle, boolean on, int color);
    
    // member functions /////////////////////////////////////////////

    public DirectVideo() {
    }
    
    /**
     * Is in clipping area?
     */
    private boolean isInClippingArea(Graphics g, int x, int y, int w, int h) {
        int diffx = g.getTranslateX();
        int diffy = g.getTranslateY();
        int clipx = g.getClipX();
        int clipy = g.getClipY();
        int clipw = g.getClipWidth();
        int cliph = g.getClipHeight();

        x += diffx;
        y += diffy;
        clipx += diffx;
        clipy += diffy;

        if (x < clipx) return false;
        if (y < clipy) return false;
        if (x + w > clipx + clipw) return false;
        if (y + h > clipy + cliph) return false;

        if (debug) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI, 
                "isInClippingArea return true - No graphic outside of clipping area"); 
        }  

        return true;
    }

    /**
     *
     */
    private void setTranslatedVideoLocation(Graphics g, int x, int y, int w, int h) {
        int diffx = g.getTranslateX();
        int diffy = g.getTranslateY();
        int px, py, pw, ph;
 
        // Calculate positions
        // And, do a physical clipping
        // Currently, Zoran chipset does not support negative position and exceed position
        px = x + diffx;
        py = y + diffy;
        pw = w;
        ph = h;

        if (px + pw <= 0) {
            return;
        }
        if (py + ph <= 0) {
            return;
        }
        if (px >= SCREEN_WIDTH) {
            return;
        }
        if (py >= SCREEN_HEIGHT) {
            return;
        }
        if (px < 0) {
            pw += px;
            px = 0;
        }
        if (py < 0) {
            ph += py;
            py = 0;
        }
        if (px + pw > SCREEN_WIDTH) {
            pw = SCREEN_WIDTH - px;
        }
        if (py + ph > SCREEN_HEIGHT) {
            ph = SCREEN_HEIGHT - py;
        }

        if (hNative != 0) {
            nSetLocation(hNative, px, py, pw, ph);
        }
    }

    /**
     * Prepare direct video rendering surface
     */
    private void prepareVideoSurface(Graphics g, int x, int y, int w, int h) {
        if (debug) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI, 
                "prepareVideoSurface " + x + "," + y + "," + w + "," + h); 
        }    
 
        // Turn off alpha channel
        nSetAlpha(hNative, false, ALPHA_COLOR);
        setTranslatedVideoLocation(g, x, y, w, h);

        if (hNative != 0) {
            nSetVisible(hNative, !hidden);
        }
    }

    /**
     * Prepare clipped preview region by using alpha channel masking
     */
    private void prepareClippedPreview(Graphics g, int x, int y, int w, int h) {
        if (1 == nSetAlpha(hNative, true, ALPHA_COLOR)) {
            g.setColor(0, 0, 8);    // IMPL NOTE - Consider RGB565 conversion
            g.fillRect(x, y, w, h);
            setTranslatedVideoLocation(g, x, y, w, h);
            if (hNative != 0) {
                nSetVisible(hNative, !hidden);
            }
        } else {
            if (hNative != 0) {
                nSetVisible(hNative, false);
            }
        }
    }

    /**
     * request to repaint
     */
    private void repaint() {
        if (canvas != null) {
            canvas.repaint();
        } else if (item != null) {
            item.forcePaint();
        }
    }

    /**
     * Check mode value
     */
    protected void checkState() {
        if (displayMode == -1) {
            throw new IllegalStateException("initDisplayMode not called yet");
        }
    }

    /**
     * Override doGetControl
     * return VideoControl and GUIControl
     */
    protected Control doGetControl(String type) {
        Control c = super.doGetControl(type);

        if (c == null) {
            String prefix = "javax.microedition.media.control.";
            if (type.equals(prefix + vicName)) {        // VideoControl
                return this;
            } else if (type.equals(prefix + guiName)) {  // GUIControl
                return this;
            }
        }
        return c;
    }

    /**
     * Override doRealize
     * Prepare soure video informations
     */ 
    protected void doRealize() throws MediaException {
        super.doRealize();
        sw = nGetWidth(hNative);
        sh = nGetHeight(hNative);
        // initialize default rendering width and height
        if (sw <= 0) dw = DEFAULT_WIDTH;
        else dw = sw;
        if (sh <= 0) dh = DEFAULT_HEIGHT;
        else dh = sh;
    }

    protected boolean doStart() {
        started = true;
        repaint();
        return super.doStart();
    }

    protected void doStop() throws MediaException {
        started = false;
        super.doStop();
    }
    
    /**
     * Init display mode
     */
    public Object initDisplayMode(int mode, Object container) {
        if (debug) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI, 
                "initDisplayMode mode=" + mode + ", container=" + container); 
        }     
        
        Object ret = null;
        
        if (displayMode != -1) {
            throw new IllegalStateException("mode already set");
        }
        if (mode != USE_DIRECT_VIDEO && mode != USE_GUI_PRIMITIVE) {
            throw new IllegalArgumentException("unsupported mode");
        }
        if (mode == USE_DIRECT_VIDEO && !Canvas.class.isInstance(container)) {
            throw new IllegalArgumentException("container needs to be a Canvas");
        }
        if (mode == USE_GUI_PRIMITIVE && container != null) {
            if (!(container instanceof String)) {
                throw new IllegalArgumentException("container not valid");
            }
            if (!(container.equals("javax.microedition.lcdui.Item"))) {
                throw new IllegalArgumentException("container not valid");
            }
        }

        if (mode == USE_DIRECT_VIDEO) {
            canvas = (Canvas)container;
            if (mmh == null) {
                mmh = MMHelper.getMMHelper();
                if (mmh == null) {
                    throw new RuntimeException("initDisplayMode: unable to set the display mode");
                }
            }
            if (!canvas.isShown()) {
                hidden = true;
            }
            displayMode = mode;
            // register this direct video handler to MMH
            // MMH used to communicate with Canvas
            mmh.registerPlayer(canvas, this);
            setDisplayLocation(dx, dy);
        } else {
            displayMode = mode;
            item = new DVItem(null);
            ret = (Object)item;
            visible = true;
        }
        
        return ret;
    }

    /**
     * Override method in BasicPlayer to close
     * the <code>Player</code>.
     */
    protected void doClose() {
        if (mmh != null && canvas != null) {
            // unregister this direct video handler with MMH
            mmh.unregisterPlayer(canvas, this);
        }
        super.doClose();
    }
    
    /**
     * Set display location 
     */
    public void setDisplayLocation(int x, int y) {
        if (debug) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI, 
                "setDisplayLocation x=" + x + ",y=" + y); 
        }
        checkState();
        if (displayMode == USE_DIRECT_VIDEO) {

            boolean needRepaint = false;

            synchronized(boundLock) {
                if (fsmode) {
                    tmp_dx = x;
                    tmp_dy = y;
                } else {
                    dx = x;
                    dy = y;
                    needRepaint = ( dw != 0 && dh != 0 );
                }
            }

            if( needRepaint ) {
                repaint();
            }
        }
    }
    
    /**
     * Set display size
     */
    public void setDisplaySize(int width, int height) throws MediaException {
        if (debug) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI, 
                "setDisplaySize w=" + width + ",h=" + height); 
        }

        boolean sizeChanged = false;
        
        checkState();

        if (width < 1 || height < 1) {
            throw new IllegalArgumentException("invalid size ("+width+","+height+")");
        }

        synchronized(boundLock) {
            if (fsmode) {
                tmp_dw = width;
                tmp_dh = height;
            } else {
                sizeChanged = ( dw != width || dh != height );
                dw = width;
                dh = height;
            }
        }

        if (item != null) {
            // this will raise sizeChanged event
            // and sizeChanged shall raise paint event also
            item.setPreferredSize( width, height );
        }
        repaint();

        if (sizeChanged) {
            sendEvent(PlayerListener.SIZE_CHANGED, this);
        }
    }
    
    public int getDisplayX() {
        return dx;
    }
    
    public int getDisplayY() {
        return dy;
    }

    /**
     * Get actual width of rendering 
     */
    public int getDisplayWidth() {
        checkState();
        return dw;
    }
    
    /**
     * Get actual height of rendering
     */
    public int getDisplayHeight() {
        checkState();
        return dh;
    }

    /**
     * return source video width
     */
    public int getSourceWidth() {
        return sw;
    }
    
    /**
     * return source video height
     */
    public int getSourceHeight() {
        return sh;
    }

    /**
     * set visible or unvisible
     */
    public void setVisible(boolean visible) {
        boolean old = this.visible;
        checkState();
        this.visible = visible;

        if (old != visible) {
            repaint();
        }

        if (visible == false && hNative != 0) {
            nSetVisible(hNative, false);
        }
    }
    
    public void setDisplayFullScreen(boolean fullScreenMode) throws MediaException {

        checkState();

        synchronized( boundLock ) {
            if( fsmode != fullScreenMode ) {
                nSetFullScreenMode(hNative,fullScreenMode);
                fsmode = fullScreenMode;

                if( fsmode ) {
                    tmp_dx = dx;
                    tmp_dy = dy;
                    tmp_dw = dw;
                    tmp_dh = dh;
                } else {
                    if( tmp_dx != dx || tmp_dy != dy ) {
                        setDisplayLocation( tmp_dx, tmp_dy );
                    }
                    if( tmp_dw != dw || tmp_dh != dh ) {
                        setDisplaySize( tmp_dw, tmp_dh );
                    }
                }
            }
        }
    }

    /**
     * Check for the multimedia snapshot permission.
     *
     * @exception SecurityException if the permission is not
     *            allowed by this token
     */
    public void checkSnapshotPermission() {
        try {
            PermissionAccessor.checkPermissions( getLocator(), PermissionAccessor.PERMISSION_SNAPSHOT );
        } catch( InterruptedException e ) {
            throw new SecurityException( "Interrupted while trying to ask the user permission" );
        }
    }

    public byte[] getSnapshot( String imageType ) throws MediaException
    {
        checkSnapshotPermission();
        checkState();

        if (null == imageType)
        {
            imageType = System.getProperty("video.snapshot.encodings");
            if (null == imageType)
            {
                throw new MediaException( "No supported snapshot formats found" );
            }
            int spacePos = imageType.indexOf(' ');
            if (spacePos > 0)
            {
                imageType = imageType.substring(0, spacePos);
            }
        }
        else
        {
            String supported = System.getProperty( "video.snapshot.encodings" );
            int idx = supported.indexOf( imageType );
            if( -1 == idx )
            {
                throw new MediaException( "Snapshot format ('" +
                                          imageType +
                                          "')is not supported" );
            }
        }


        byte[] data = null;
        if (hNative != 0)
        {
            data = nSnapShot(hNative, imageType.toLowerCase());
        }
        if (null == data)
        {
            throw new MediaException( "Snapshot in '" + imageType + "' format failed." );
        }
        return data;
    }

    public void setSnapshotQuality( int quality )
    {
    }

    /**
     * called from Canvas.paint routine
     * We have to paint direct video region on Canvas
     * Notice: This have to be done before device painting action
     * Zoran ESDK use mask color to draw direct video
     */
    public void paintVideo(Graphics g) {
        int x, y, w, h;
        
        synchronized(boundLock) {
            x = dx;
            y = dy;
            w = dw;
            h = dh;
        }

        if (debug) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI, 
                "paintVideo x=" + x + ",y=" + y + ",w=" + w + ",h=" + h); 
        }

        if (canvas != null && !canvas.isShown()) {
            hidden = true;
        }

        if (hidden) {
            prepareClippedPreview(g, x, y, w, h);
        } else if (visible && started) {
            if (true == isInClippingArea(g, x, y, w, h)) {
                prepareVideoSurface(g, x, y, w, h);
            } else {
                int cx = g.getClipX();
                int cy = g.getClipY();
                int cw = g.getClipWidth();
                int ch = g.getClipHeight();
                g.setClip(x, y, w, h);
                prepareClippedPreview(g, x, y, w, h);
                g.setClip(cx, cy, cw, ch);
            }
        }
    }

    /**
     * Hide video preview (called from CanvasLFImpl)
     */
    public void hideVideo() {
        if (debug) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI, 
                "hideVideoPreview"); 
        }
        if (hNative != 0) {
            nSetVisible(hNative, false);
        }
        hidden = true;
        nSetAlpha(hNative, true, ALPHA_COLOR);
        repaint();
    }

    /**
     * Show video preview (called from CanvasLFImpl)
     */
    public void showVideo() {
        if (debug) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI, 
                "showVideoPreview"); 
        }
        hidden = false;
        repaint();
        // IMPL NOTE - we shouldn't show video immediately: it will appear after drawVideo()
    }
    
    // Inner class ///////////////////////////////////////////////////////////

    /**
     * Support USE_GUI_PRIMITIVE mode
     */
    class DVItem extends CustomItem {

        DVItem(String label) {
            super(label);
        }
        
        void forcePaint() {
            repaint();
        }
        
        protected void paint(Graphics g, int w, int h) {
            if (debug) {
                Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI, 
                    "DVItem.paint visible=" + visible); 
            }

            // Is in hidden state, then just draw fake preview
            if (hidden) {
                prepareClippedPreview(g, 0, 0, w, h);
            // Is out of hidden state, then check about clipping regions and
            // determind what to show
            } else if (visible) {
                if (true == isInClippingArea(g, 0, 0, w, h)) {
                    // Prepare video preview
                    prepareVideoSurface(g, 0, 0, w, h);
                } else {
                    prepareClippedPreview(g, 0, 0, w, h);
                }
            }
        }
        
        protected int getMinContentWidth() {
            return 1;
        }
        
        protected int getMinContentHeight() {
            return 1;
        }
        
        protected int getPrefContentWidth(int height) {
            return dw;
        }
        
        protected int getPrefContentHeight(int width) {
            return dh;
        }

        protected void sizeChanged(int w, int h) {
            synchronized(boundLock) {
                dw = w;
                dh = h;
            }
            repaint();
        }

        // Now this function used to control visible state of direct video preview
        // Called from MIDPWindow class
        protected void showNotify() {
            if (debug) {
                Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI, "showNotify"); 
            }        
            hidden = false;
            repaint();
        }

        // Now this function used to control visible state of direct video preview
        // Called from MIDPWindow class
        protected void hideNotify() {
            if (debug) {
                Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI, "hideNotify"); 
            }        
            hidden = true;
            repaint();
        }
    }
}
