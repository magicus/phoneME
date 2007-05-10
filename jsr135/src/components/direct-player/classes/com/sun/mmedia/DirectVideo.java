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

import com.sun.midp.midlet.Scheduler;
import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.configurator.Constants;
import com.sun.midp.security.*;
import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;


/**
 * Video direct player
 * it implements VideoControl
 */
public class DirectVideo extends DirectPlayer implements 
    VideoControl, MIDPVideoPainter, ImplicitlyTrustedClass {

    private final int SCREEN_WIDTH = nGetScreenWidth();
    private final int SCREEN_HEIGHT = nGetScreenHeight();
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

    // Display position and size
    private int dx;
    private int dy;
    private int dw;
    private int dh;

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

    /** This class has a different security domain than the MIDlet suite */
    private static SecurityToken classSecurityToken;
    
    // native functions /////////////////////////////////////////////

    // Get video width
    protected native int nGetWidth(int handle);
    // Get video height
    protected native int nGetHeight(int handle);
    // Set display location of video
    protected native boolean nSetLocation(int handle, int x, int y, int w, int h);
    // Get snapshot
    protected native byte[] nSnapShot(int handle, String imageType);
    // Set visible
    protected native boolean nSetVisible(int handle, boolean visible);
    // Get screen full width
    private native int nGetScreenWidth();
    // Get screen full height
    private native int nGetScreenHeight();
    // Turn on or off alpha channel
    private native int nSetAlpha(boolean on, int color);

    // member functions /////////////////////////////////////////////

    public DirectVideo() {
    }
    
    /**
     * Initializes the security token for this class, so it can
     * perform actions that a normal MIDlet Suite cannot.
     *
     * @param token security token for this class.
     */
    public final void initSecurityToken(SecurityToken token) {
        if (classSecurityToken != null) {
            return;
        }
        classSecurityToken = token;
    }

    /**
     * Check snapshot permission
     */
    protected void checkPermission() {
        try {
            Scheduler scheduler = Scheduler.getScheduler();
            MIDletSuite midletSuite = scheduler.getMIDletSuite();
            midletSuite.checkForPermission(Permissions.MM_IMAGE_CAPTURING, null);
        } catch (InterruptedException e) {
            throw new SecurityException(
                    "Interrupted while trying to ask the user permission");
        }
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
 
        // Calcurate positions
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
        nSetAlpha(false, ALPHA_COLOR);
        setTranslatedVideoLocation(g, x, y, w, h);

        // set location and size of display region
        if (hNative != 0) {
            nSetVisible(hNative, true);
        }
    }

    /**
     * Prepare clipped preview region by using alpha channel masking
     */
    private void prepareClippedPreview(Graphics g, int x, int y, int w, int h) {
        if (1 == nSetAlpha(true, ALPHA_COLOR)) {
            g.setColor(0, 0, 8);    // IMPL NOTE - Consider RGB565 conversion
            g.fillRect(x, y, w, h);
            setTranslatedVideoLocation(g, x, y, w, h);
            if (hNative != 0) {
                nSetVisible(hNative, true);
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
     * request to repaint canvas and wait until that processed
     */
    private void repaintAndWait() {
        if (canvas != null) {
            canvas.repaint();
            canvas.serviceRepaints();
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
        repaintAndWait();
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
        if (mode == USE_DIRECT_VIDEO && !(container instanceof Canvas)) {
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
     * Set display location 
     */
    public void setDisplayLocation(int x, int y) {
        if (debug) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI, 
                "setDisplayLocation x=" + x + ",y=" + y); 
        }
        checkState();
        if (displayMode == USE_DIRECT_VIDEO) {
            synchronized(boundLock) {
                dx = x;
                dy = y;
            }
            if (dw != 0 && dh !=0) {
                repaintAndWait();
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
            throw new IllegalArgumentException("invalid size");
        }

        synchronized(boundLock) {
            if (dw != width && dh != height) sizeChanged = true;
            dw = width;
            dh = height;
        }
        if (item != null) {
            // this will raise sizeChanged event
            // and sizeChanged shall raise paint event also
            item.setPreferredSize(width, height);
        }
        repaintAndWait();
        
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
            repaintAndWait();
        }

        if (visible == false && hNative != 0) {
            nSetVisible(hNative, false);
        }
    }
    
    /**
     * There is no full screen mode now
     */
    public void setDisplayFullScreen(boolean fullScreenMode) throws MediaException {
        checkState();
        if (fullScreenMode) {
            throw new MediaException("No Fullscreen mode");
        }
    }
    
    /**
     * There is no snap shot support now
     */
    public byte[] getSnapshot(String imageType) throws MediaException {
        checkState();
        throw new MediaException("No snapshot support");
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
        hidden = true;
        nSetAlpha(true, ALPHA_COLOR);
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
        nSetAlpha(false, ALPHA_COLOR);        
        repaint();
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

