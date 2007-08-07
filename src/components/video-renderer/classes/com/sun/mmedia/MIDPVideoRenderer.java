/*
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

import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.Canvas;
import javax.microedition.lcdui.Graphics;


import javax.microedition.media.Control;
import javax.microedition.media.Player;
import javax.microedition.media.PlayerListener;
import javax.microedition.media.MediaException;
import javax.microedition.media.control.VideoControl;

import com.sun.mmedia.PermissionAccessor;

import com.sun.mmedia.protocol.LocatorParser;

//import com.sun.mmedia.ImageEncoder;

import com.sun.mmedia.MMCustomItem;

/**
 * VideoControl implementation for MIDP
 */
public final class MIDPVideoRenderer extends VideoRenderer
    implements VideoControl, MIDPVideoPainter {

    /** If the application requests an Item */
    private MMItem mmItem;
    /** If the application requests to draw in a Canvas */
    private Canvas canvas;
    /** Full screen mode flag */
    private boolean fsmode;
    /** Is the player closed */
    private boolean closed;
    /** The display mode */
    private int mode = -1;
    /** Container visible flag. True if the Canvas is visible */
    private boolean cvis;
    /** Application specified visibility flag. True if setVisible(true) */
    private boolean pvis;
    /** Player which is being controlled */
    private BasicPlayer player;

    /** Display X */
    private int dx, tmpdx;
    /** Display Y */
    private int dy, tmpdy;
    /** Display Width */
    private int dw, tmpdw;
    /** Display Height */
    private int dh, tmpdh;

    /** Source width */
    private int videoWidth;
    /** Source height */
    private int videoHeight;

    /** Storage for the snapshot */
    private byte [] tempSnapData;

    /** To check the frame rate */
    private static final boolean TRACE_FRAMERATE = false;
    /** To check the frame rate */
    private int frameCount;
    /** To check the frame rate */
    private long frameStartTime = 0;

    private static final String UNSUP_PARAMS = "Unsupported parameters";

    /** used to protect dx, dy, dw, dh set & read */
    private Object dispBoundsLock = new Object();
    
    /****************************************************************
     * VideoControl implementation
     ****************************************************************/

    MIDPVideoRenderer(Player p, int sourceWidth, int sourceHeight) {
        setSourceSize(sourceWidth, sourceHeight);
        if (p instanceof BasicPlayer) {
            this.player = (BasicPlayer)p;
        } else {
            System.err.println("video renderer can't work with Players of this class: " + p.toString());
        }
    }

    void setSourceSize(int sourceWidth, int sourceHeight) {
        
        videoWidth = sourceWidth;
        videoHeight = sourceHeight;
        
        // Default display width and height
        synchronized (dispBoundsLock) {
            dw = videoWidth;
            dh = videoHeight;
        }
    }

    private void checkState() {
        if (mode == -1)
            throw new IllegalStateException("initDisplayMode not called yet");
    }

    public Object initDisplayMode(int mode, Object container) {
        if (this.mode != -1)
            throw new IllegalStateException("mode is already set");
        
        if (mode == USE_DIRECT_VIDEO) {
            if (!(container instanceof Canvas))
                throw new IllegalArgumentException(
                    "container needs to be a Canvas for USE_DIRECT_VIDEO mode");
            
            if (mmh == null) {
                mmh = MMHelper.getMMHelper();
                if (mmh == null)
                    throw new IllegalArgumentException(
                            "unable to set USE_DIRECT_VIDEO mode");
            }

            this.mode = mode;
            fsmode = false;
            cvis = true;
            canvas = (Canvas) container;
            mmh.registerPlayer(canvas, this);
            setVisible(false); // By default video is not shown in USE_DIRECT_VIDEO mode
            return null;
            
        } else if (mode == USE_GUI_PRIMITIVE) {
            if (container != null && 
                (!(container instanceof String) ||
                 !(container.equals("javax.microedition.lcdui.Item"))))
                throw new IllegalArgumentException("container needs to be a javax.microedition.lcdui.Item for USE_GUI_PRIMITIVE mode");

            this.mode = mode;
            fsmode = false;
            cvis = true;
            mmItem = new MMItem();
            setVisible(true);
            return mmItem;
            
        } else {
            throw new IllegalArgumentException("unsupported mode");
        }
    }

    public void setDisplayLocation(int x, int y) {
        checkState();
        // Applicable only in USE_DIRECT_VIDEO mode
        if (mode == USE_DIRECT_VIDEO) {
            if (fsmode) { // Just store location in fullscreen mode
                synchronized (dispBoundsLock) {
                    tmpdx = x;
                    tmpdy = y;
                }
            } else {
                synchronized (dispBoundsLock) {
                    dx = x;
                    dy = y;
                }
                if (pvis && cvis)
                    canvas.repaint();
            }
        }
    }


    public int getDisplayX() {
        return dx;
    }
        
    public int getDisplayY() {
        return dy;
    }

    /**
     * Check for the image snapshot permission.
     *
     * @exception SecurityException if the permission is not
     *            allowed by this token
     */
    private static void checkPermission() throws SecurityException {
        PermissionAccessor.checkPermissions(PermissionAccessor.PERMISSION_VIDEO_SNAPSHOT);
    }

    public void setVisible(boolean visible) {
        checkState();
        pvis = visible;
        if (canvas != null) // USE_DIRECT_VIDEO
            canvas.repaint();
        else if (mmItem != null) // USE_GUI_PRIMITIVE
            mmItem.forcePaint(null);
    }

    public void setDisplaySize(int width, int height)
        throws javax.microedition.media.MediaException {
        checkState();
        if (width < 1 || height < 1)
            throw new IllegalArgumentException("Invalid size");
        
        boolean sizeChanged = (dw != width || dh != height);
        
        if (fsmode) { // Just store sizes in fullscreen mode
            synchronized (dispBoundsLock) {
                tmpdw = width;
                tmpdh = height;
            }
        } else {
            synchronized (dispBoundsLock) {
                dw = width;
                dh = height;
            }
            if (pvis)
                if (mmItem != null)
                    mmItem.forcePaint(null);
                else if (cvis)                   
                    canvas.repaint();
        }
        // Makes sense only if NOT in Full Screen mode
        if (sizeChanged && !fsmode)
            player.sendEvent(PlayerListener.SIZE_CHANGED, this);
    }

    public void setDisplayFullScreen(boolean fullScreenMode)
        throws javax.microedition.media.MediaException {
        checkState();
        if (fsmode != fullScreenMode) {
            fsmode = fullScreenMode;
            if (fsmode) { //switching from Normal to Full Screen
                synchronized (dispBoundsLock) {
                    tmpdx = dx;
                    tmpdy = dy;
                    tmpdw = dw;
                    tmpdh = dh;
                }
                if (mode == USE_GUI_PRIMITIVE) {
                    canvas = mmItem.toFullScreen(this, this);
                    if (canvas == null) {
                        // No owner or no display - thus invisible
                        // Do nothing, but simulate fullscreen (lock sizes - for compliance)
                        return;
                    }                        
                }
                synchronized (dispBoundsLock) {
                    dx = 0;
                    dy = 0;
                                        
                    // Keep aspect ratio
                    int scrw = canvas.getWidth();
                    int scrh = canvas.getHeight();
                    dw = scrh * videoWidth / videoHeight;
                    if (dw > scrw) {
                        dw = scrw;
                        dh = scrw * videoHeight / videoWidth;
                        dy = (scrh - dh) / 2;
                    } else {
                        dh = scrh;
                        dx = (scrw - dw) / 2;
                    }
                }
                if (cvis)
                    canvas.repaint();

            } else { //switching from Full to Normal Screen
                synchronized (dispBoundsLock) {
                    dx = tmpdx;
                    dy = tmpdy;
                    dw = tmpdw;
                    dh = tmpdh;
                }
                if (mode == USE_DIRECT_VIDEO) {
                    if (pvis && cvis)
                        canvas.repaint();
                } else {
                    mmItem.toNormal();
                    canvas = null;
                    if (pvis)
                        mmItem.forcePaint(null);
                }
            }
            player.sendEvent(PlayerListener.SIZE_CHANGED, this);
        }
    }

    public int getDisplayWidth() {
        checkState();
        return dw;
    }

    public int getDisplayHeight() {
        checkState();
        return dh;
    }
    
    public int getSourceWidth() {
        return videoWidth;
    }
        
    public int getSourceHeight() {
        return videoHeight;
    }

    public byte[] getSnapshot(String imageType)
        throws MediaException, SecurityException {
        throw new MediaException("Not supported");
    }

    private int tryParam(String tok, String prop, int def) {
        if (tok.startsWith(prop)) {
            tok = tok.substring(prop.length(), tok.length());
            try {
                return Integer.parseInt(tok);
            } catch (NumberFormatException nfe) {
            }
        }
        return def;
    }
    
    synchronized public void close() {
        if (!closed && canvas != null)
            mmh.unregisterPlayer(canvas, this);
        rgbData = null;
        scaledRGB = null;
        pngData = null;
        closed = true;
    }

    /****************************************************************
     * Rendering interface
     ****************************************************************/
    
    // Frame types
    public static final int RGB565 = 1; // short [] 
    public static final int RGB888 = 2; // byte []
    public static final int XRGB888 = 3; // int []
    public static final int XBGR888 = 4; // int []
    public static final int RGBX888 = 5; // int []
    public static final int YUV420_PLANAR = 6; // byte []
    public static final int YUV422_PLANAR = 7; // byte []
    public static final int YUYV = 8; // byte []
    public static final int UYVY = 9; // byte []
    public static final int YVYU = 10; // byte []
    public static final int NATIVE_RENDER = 128; // to be ORed with above
    public static final int USE_ALPHA = 256;

    int rgbMode;
    int pWidth;
    int pHeight;
    int [] rgbData;
    int [] scaledRGB;
    byte [] pngData;
    int pngDataLength;
    boolean nativeRender;
    boolean useAlpha;
    private Image image;
    private MMHelper mmh = null;

    public Control getVideoControl() {
        return (VideoControl)this;
    }
    
    public void initRendering(int mode, int width, int height) {
        rgbMode = mode & 0x7F; // mask out NATIVE_RENDER
        nativeRender = (mode & NATIVE_RENDER) > 0;
        useAlpha = (mode & USE_ALPHA) > 0;
        pWidth = width;
        pHeight = height;
    }

    void setMode(int mode) {
        rgbMode = mode & 0x7F;
        nativeRender = (mode >= 128);
    }

    int getPreferredRGBMode() {
        return RGB888;
    }

    /**
     * Public render method
     */
    public void render(int[] data) {
        render((Object)data);
    }

    /**
     * Renders the data to the screen at the component's location
     * and size, if component is visible.
     * Returns true if displayed, false if not.
     */
    synchronized boolean render(Object data) {
        if (data == null)
            return false;
        if (data instanceof int[])
            update((int[]) data);
        else
            return false;
        
        return true;
    }

    synchronized boolean renderImage(byte [] imageData, int imageLength) {
        // Keep these values, in case snapshot is requested
        pngData = imageData;
        pngDataLength = imageLength;

        if (!pvis)
            return false;

        if (canvas != null) {
            if (cvis)
                canvas.repaint(dx, dy, dw, dh);
        } else if (mmItem != null) {
            mmItem.renderImage(imageData, imageLength);
        }
        return true;
    }

    private void update(int [] frame) {
        if (rgbMode != XBGR888)
            return;

        rgbData = frame;

        if (!pvis)
            return;
                
        if (canvas != null) {
            if (cvis) {
                canvas.repaint(dx, dy, dw, dh);
            }
        } else if (mmItem != null) {
            mmItem.forcePaint(frame);
        }
    }

    /**
     * Scales an input rgb image to the destination size.
     */
    private int [] scaleToDest(int [] source) {
        int ldw = 0;
        int ldh = 0;
        synchronized (dispBoundsLock) {
            ldw = dw;
            ldh = dh;
        }
        synchronized (this) { // To avoid interference with close()
            if (scaledRGB == null || scaledRGB.length < ldw * ldh)
                scaledRGB = new int[ldw * ldh];
            // Scale using nearest neighbor
            int dp = 0;
            for (int y = 0; y < ldh; y++) {
                for (int x = 0; x < ldw; x++) {
                    scaledRGB[dp++] = source[((y * videoHeight) / ldh) * videoWidth +
                                            ((x * videoWidth) / ldw)];
                }
            }
            return scaledRGB;
        }
    }

    /**
     * Scale an image to the destination size. This first gets the
     * pixels from the image and then uses the other scaleToDist()
     * to do the scaling.
     */
    private int [] scaleToDest(Image img) {
        if (rgbData == null)
            rgbData = new int[videoWidth * videoHeight];
        int width = img.getWidth();
        int height = img.getHeight();
        img.getRGB(rgbData, 0, videoWidth, 0, 0, width, height);
        return scaleToDest(rgbData);
    }

    /**
     * Paint video into canvas - in USE_DIRECT_VIDEO mode
     */
    public void paintVideo(Graphics g) {
        // Don't paint if Canvas visible flag is false
        if (!pvis || !cvis)
            return;
        
        // Save the clip region
        int cx = g.getClipX();
        int cy = g.getClipY();
        int cw = g.getClipWidth();
        int ch = g.getClipHeight();
        // Change the clip to clip the video area
        g.clipRect(dx, dy, dw, dh);
        
        // Check if its within our bounds
        if (g.getClipWidth() > 0 && g.getClipHeight() > 0 && pvis) {
            int w = dw, h = dh;
            if (w > videoWidth) w = videoWidth;
            if (h > videoHeight) h = videoHeight;
            try {
                synchronized (this) {
                    if (pngData != null) {
                        if (image != null) {

                        }
                        image = Image.createImage(pngData, 0, pngDataLength);
                        // We're rendering an image
                        if (dw != videoWidth || dh != videoHeight) {
                            // Scale first and display
                            int [] scaledRGB = scaleToDest(image);
                            g.drawRGB(scaledRGB, 0, dw, dx, dy, dw, dh, useAlpha);
                        } else {
                            // No scaling
                            g.drawImage(image, dx, dy,
                                        Graphics.LEFT | Graphics.TOP);
                        }
                    } else if (rgbData != null) {
                        // We're rendering an RGB array
                        if (dw != videoWidth || dh != videoHeight) {
                            // Scale first and display
                            int [] scaledRGB = scaleToDest(rgbData);
                            g.drawRGB(scaledRGB, 0, dw, dx, dy, dw, dh, useAlpha);
                        } else {
                            // No scaling
                            g.drawRGB(rgbData, 0, videoWidth, dx, dy, w, h, useAlpha);
                        }
                    }
                }
            } finally {
                // Revert the clip region
                g.setClip(cx, cy, cw, ch);
            }
        } else {
            g.setClip(cx, cy, cw, ch);
        }
        if (TRACE_FRAMERATE) {
            if (frameStartTime == 0) {
                frameStartTime = System.currentTimeMillis();
            } else {
                frameCount++;
                if ((frameCount % 30) == 0) {
                    int frameRate = (int) ( (frameCount * 1000) / (System.currentTimeMillis() - frameStartTime + 1));
                    //System.err.println("Frame Rate = " + frameRate);
                }
            }
        }
    }

    /**
     * Enable/disable rendering for canvas (USE_DIRECT_VIDEO mode)
     */
    public void showVideo() {
        if (canvas != null && !cvis) {
            cvis = true;
            canvas.repaint();
        }
    }
       
    public void hideVideo() {
        if (canvas != null && cvis) {
            cvis = false;
            canvas.repaint();
        }
    }

    /****************************************************************
     * MMItem (CustomItem) - USE_GUI_PRIMITIVE mode
     ****************************************************************/
    
    final class MMItem extends MMCustomItem {

        int ody, odh, odw;
        int [] frame;
        Image image;
        Object imageLock = new Object();
        
        public MMItem() {
            super("");
        }

        void forcePaint(int [] frame) {
            if (frame != null)
                this.frame = frame;
            else
                invalidate();
            repaint();
        }
        
        void renderImage(byte [] imageData, int imageLength) {
            synchronized (imageLock) {
                image = Image.createImage(imageData, 0, imageLength);
            }
            repaint();
        }

        protected void paint(Graphics g, int w, int h) {
            // Don't paint if VideoControl visible flag is false
            if (!pvis)
                return;

            if (frame != null) {
                if (dw != videoWidth || dh != videoHeight) {
                    // Scale first
                    int [] scaledRGB = scaleToDest(frame);
                    g.drawRGB(scaledRGB, 0, dw, 0, 0, dw, dh, useAlpha);
                } else {
                    // No scaling
                    g.drawRGB(frame, 0, videoWidth, 0, 0, videoWidth, videoHeight, useAlpha);
                }
            } else {
                synchronized (imageLock) {
                    if (image != null) {
                        if (dw != videoWidth || dh != videoHeight) {
                            // Scale first
                            int [] scaledRGB = scaleToDest(image);
                            g.drawRGB(scaledRGB, 0, dw, 0, 0, dw, dh, useAlpha);
                        } else {
                            // No scaling
                            g.drawImage(image, 0, 0, Graphics.LEFT | Graphics.TOP);
                        }
                    }
                }
            }
        }

        protected int getMinContentWidth() {
            return 1;
        }

        protected int getMinContentHeight() {
            return 1;
        }

        protected int getPrefContentWidth(int h) {
            return dw;
        }
        
        protected int getPrefContentHeight(int w) {
            return dh;
        }

        protected void hideNotify() {
            super.hideNotify();
        }
    }
}
