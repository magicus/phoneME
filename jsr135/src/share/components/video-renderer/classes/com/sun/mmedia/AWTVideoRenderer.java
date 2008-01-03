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
package com.sun.mmedia;

import java.awt.*;
import java.awt.image.*;
import javax.microedition.media.MediaException;
import javax.microedition.media.Control;
import javax.microedition.media.control.VideoControl;

public class AWTVideoRenderer extends VideoRenderer 
        implements VideoControl {
    /**
     * The video canvas object.
     */
    private VideoCanvas canvas;

    /**
     * The display mode
     */
    private int mode = -1;

    /**
     * Display X
     */
    private int dx;
    /**
     * Display Y
     */
    private int dy;
    /**
     * Display Width
     */
    private int dw;
    /**
     * Display Height
     */
    private int dh;

    /**
     * Source width
     */
    private int sw;
    /**
     * Source height
     */
    private int sh;


    /*
     * Locator string used to open the Player instance which this VideoControl is assoicated with. 
     */
    private String locatorString;

    /**
     *Constructor for the VideoRenderer object
     */
    public AWTVideoRenderer(BasicPlayer player, int sourceWidth, int sourceHeight) { 
        locatorString = ((GIFPlayer)player).getLocator();
    }

    public Control getVideoControl() {
        return (VideoControl)this;
    }

    /**
     *  Description of the Method
     *
     * @param  mode          Description of the Parameter
     * @param  sourceWidth   Description of the Parameter
     * @param  sourceHeight  Description of the Parameter
     */
    public void initRendering(int mode, int sourceWidth, int sourceHeight) {
		setSourceSize(sourceWidth, sourceHeight);
    }


    /**
     *  Sets the sourceSize attribute of the AWTVideoRenderer object
     *
     * @param  sourceWidth   The new sourceSize value
     * @param  sourceHeight  The new sourceSize value
     */
    private void setSourceSize(int sourceWidth, int sourceHeight) {
        sw = sourceWidth;
        sh = sourceHeight;

        // Default display width and height
        dw = sw;
        dh = sh;
    }


    /**
     *  Description of the Method
     *
     * @param  rgbData  Description of the Parameter
     */
    public void render(int[] rgbData) {
		// csaito: adding this if. 
        if (canvas != null) {

           Image img = Toolkit.getDefaultToolkit().createImage(
               new MemoryImageSource(sw, sh, rgbData, 0, sw));

           canvas.updateImage(img);
        }

    }


    /**
     *  Gets the snapshot attribute of the VideoRenderer object
     *
     * @param  imageType           Description of the Parameter
     * @return                     The snapshot value
     * @exception  MediaException  Description of the Exception
     */

    public byte[] getSnapshot(String imageType) throws MediaException {
        checkPermission();
        throw new MediaException("getSnapshot not implemented!");
        // return null;
    }

    /**
     * Check for the image snapshot permission.
     *
     * @exception SecurityException if the permission is not
     *            allowed by this token
     */
    private void checkPermission() throws SecurityException {
        try {
            PermissionAccessor.checkPermissions(locatorString, PermissionAccessor.PERMISSION_SNAPSHOT);
    	} catch (InterruptedException e) {
    	    throw new SecurityException("Interrupted while trying to ask the user permission");
    	}
    }

    /**
     *  Gets the displayWidth attribute of the VideoRenderer object
     *
     * @return    The displayWidth value
     */
    public int getDisplayWidth() {
        checkState();
        return dw;
    }


    /**
     *  Gets the displayHeight attribute of the VideoRenderer object
     *
     * @return    The displayHeight value
     */
    public int getDisplayHeight() {
        checkState();
        return dh;
    }


    /**
     *  Gets the sourceWidth attribute of the VideoRenderer object
     *
     * @return    The sourceWidth value
     */
    public int getSourceWidth() {
        return sw;
    }


    /**
     *  Gets the sourceHeight attribute of the VideoRenderer object
     *
     * @return    The sourceHeight value
     */
    public int getSourceHeight() {
        return sh;
    }


    /**
     *  Sets the displaySize attribute of the VideoRenderer object
     *
     * @param  width                                        The new displaySize value
     * @param  height                                       The new displaySize value
     * @exception  javax.microedition.media.MediaException  Description of the Exception
     */
    public void setDisplaySize(int width, int height)
         throws javax.microedition.media.MediaException {
        checkState();

        if (width <= 0 || height <= 0) 
            throw new IllegalArgumentException("Width and Height must be positive");

        if (mode == USE_GUI_PRIMITIVE) { // should be, as USE_DIRECT_VIDEO is unsupported
            if (canvas != null) { 
               canvas.setSize(width, height);
            }
        }
         
    }


    /**
     *  Sets the displayFullScreen attribute of the VideoRenderer object
     *
     * @param  fullScreenMode                               The new displayFullScreen value
     * @exception  javax.microedition.media.MediaException  Description of the Exception
     */
    public void setDisplayFullScreen(boolean fullScreenMode)
         throws javax.microedition.media.MediaException {
        if (fullScreenMode) {  // csaito: may want to implement this properly
            throw new MediaException("No Fullscreen mode");
        }
    }


    /**
     *  Sets the visible attribute of the VideoRenderer object
     *
     * @param  visible  The new visible value
     */
    public void setVisible(boolean visible) {
        checkState();
    }


    /**
     *  Gets the displayX attribute of the VideoRenderer object
     *
     * @return    The displayX value
     */
    public int getDisplayX() {
        return dx;
    }


    /**
     *  Gets the displayY attribute of the VideoRenderer object
     *
     * @return    The displayY value
     */
    public int getDisplayY() {
        return dy;
    }


    /**
     * Initializes the display mode.
     *
     * Currently only USE_GUI_PRIMITIVE is supported!
     *
     * @param  mode       the display mode
     * @param  container  the container (an AWT component)
     * @return            returns the AWT component
     */
    public Object initDisplayMode(int mode, Object container) {
		if (this.mode != -1)
		{
            throw new IllegalStateException("mode already set");
        }

        if (mode == USE_DIRECT_VIDEO && mode != USE_GUI_PRIMITIVE) {
            throw new IllegalArgumentException("unsupported mode");
        }

        // csaito: the spec seems to require "container" to be a String obj.
        // thus this is a wrong logic...
        //if (mode == USE_DIRECT_VIDEO && !(container instanceof Canvas)) {
        //    throw new IllegalArgumentException("container needs to be a Canvas");
        //}

        if (mode == USE_GUI_PRIMITIVE && 
            container != null &&
            !(container instanceof String)) {
            throw new IllegalArgumentException("Invalid Object arg parameter");
        }

        if (container != null) {
           try {
               Class awtClazz = Class.forName((String)container);
               if (!(awtClazz.isAssignableFrom(java.awt.Component.class))) {
                  throw new IllegalArgumentException("String does not represent a valid AWT class"); 
               }
           } catch (Exception e) {
               throw new IllegalArgumentException("String does not represent a valid AWT class"); 
           }
        }

        this.mode = mode;

        canvas = new VideoCanvas();
   
        return (Object) canvas;
    }


    /**
     *  Sets the displayLocation attribute of the VideoRenderer object
     *
     * @param  x  The new displayLocation value
     * @param  y  The new displayLocation value
     */
    public void setDisplayLocation(int x, int y) {
        checkState();
        
        if (mode == USE_DIRECT_VIDEO) {
            dx = x;
            dy = y;
            canvas.repaint();
        }        
    }


    /**
     *  Description of the Method
     */
    private void checkState() {
        if (mode == -1) {
            throw new IllegalStateException("initDisplayMode not called yet");
        }
    }


    /**
     *  Description of the Method
     */
    public void close() {
        // release all resources
        if (canvas != null) {
            canvas.close();
            canvas = null;
        }
    }


    /**
     *  Description of the Class
     *
     */
    //class VideoCanvas extends Canvas {
    class VideoCanvas extends Component {
        Image img;

        int preferredWidth = -1, preferredHeight = -1;

        /**
         *  Description of the Method
         *
         * @param  g  Description of the Parameter
         */
        public void paint(Graphics g) {
			if (img != null)
			{
                   g.drawImage(img, dx, dy, dw, dh, this);
            }
		}


        /**
         *  Description of the Method
         *
         * @param  img  Description of the Parameter
         */
        public void updateImage(Image img) {
					if (img != null) {
						if (preferredWidth == -1  || preferredHeight == -1) {
						   preferredWidth  = img.getWidth(this);
						   preferredHeight = img.getHeight(this);
						   this.invalidate();
						   this.getParent().validate();
						}
						this.img = img;
						repaint();
					}
		}
        
        public void close() {
            img = null;
        }        

        public Dimension getPreferredSize() { 
            return new Dimension(preferredWidth, preferredHeight);
        }

        public Dimension getMinimumSize() {
            return getPreferredSize();
        }
    }
}

