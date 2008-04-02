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

package javax.microedition.media.control;

import javax.microedition.media.MediaException;


/**
 * <code>VideoControl</code> controls the display of video.
 * A <code>Player</code> which supports the playback of video 
 * must provide a <code>VideoControl</code> via its
 * <code>getControl</code> and <code>getControls</code>
 * method.
 *
 */
public interface VideoControl extends GUIControl {

    /**
     * This defines a mode on how the video is displayed.
     * It is used in conjunction with 
     * <a href="#initDisplayMode(int, java.lang.Object)">
     * <code>initDisplayMode</code></a>.
     * <p>
     * <code>USE_DIRECT_VIDEO</code> mode can only be used
     * on platforms with LCDUI support.
     * <p>
     * When <code>USE_DIRECT_VIDEO</code> is specified for
     * <code>initDisplayMode</code>, the <code>arg</code>
     * argument must not be null and must be a
     * <code>javax.microedition.lcdui.Canvas</code> or a subclass of it.
     * In this mode, the video is directly
     * rendered onto the canvas.  The region where the video
     * is rendered can be set by the <code>setDisplayLocation</code>
     * method.  By default, the location is (0, 0).
     * Drawing any graphics or rendering other video at the same
     * region on the canvas may not be supported.
     * <p>
     * <code>initDisplayMode</code>
     * returns <code>null</code> in this mode.
     * <p>
     * Here is one sample usage scenario:
     * <pre>
     * <code>
     *   javax.microedition.lcdui.Canvas canvas;
     *   // canvas must be created before being used in the following code.
     *
     *   try {
     *       Player p = Manager.createPlayer("http://mymachine/abc.mpg");
     *       p.realize();
     *       VideoControl vc;
     *       if ((vc = (VideoControl)p.getControl("VideoControl")) != null) {
     *           vc.initDisplayMode(VideoControl.USE_DIRECT_VIDEO, canvas);
     *           vc.setVisible(true);
     *       }
     *       p.start();
     *   } catch (MediaException pe) {
     *   } catch (IOException ioe) {
     *   }
     * </code>
     * </pre>
     * <p>
     * Value 1 is assigned to <code>USE_DIRECT_VIDEO</code>.
     */
    int USE_DIRECT_VIDEO = 1;


    /**
     * Initialize the mode on how the video is displayed.
     * This method must be called before video can be displayed.
     * <p>
     * Two modes are defined:
     * <ul>
     * <li> <a href="GUIControl.html#USE_GUI_PRIMITIVE"><code>USE_GUI_PRIMITIVE</code></a> (inherited from GUIControl)
     * <li> <a href="#USE_DIRECT_VIDEO"><code>USE_DIRECT_VIDEO</code></a>
     * </ul>
     * On platforms with LCDUI support, both modes must be supported.
     *
     * @param mode The video mode that determines how video is
     * displayed.  It can be <code>USE_GUI_PRIMITIVE</code>,
     * <code>USE_DIRECT_VIDEO</code> or an implementation-
     * specific mode.
     *
     * @param arg The exact semantics of this argument is
     * specified in the respective mode definitions.
     *
     * @return The exact semantics and type of the object returned
     * are specified in the respective mode definitions.
     * 
     * @exception IllegalStateException Thrown if 
     * <code>initDisplayMode</code> is called again after it has 
     * previously been called successfully.
     *
     * @exception IllegalArgumentException Thrown if
     * the <code>mode</code> or <code>arg</code> 
     * argument is invalid.   <code>mode</code> must be
     * <code>USE_GUI_PRIMITIVE</code>, 
     * <code>USE_DIRECT_VIDEO</code>, or a custom mode
     * supported by this implementation.
     * <code>arg</code> must conform to the 
     * conditions defined by the 
     * respective mode definitions.  
     * Refer to the mode definitions for the required type
     * of <code>arg</code>.
     */
    Object initDisplayMode(int mode, Object arg);

/* JAVADOC ELIDED */
    void setDisplayLocation(int x, int y);

/* JAVADOC ELIDED */
    int getDisplayX();

/* JAVADOC ELIDED */
    int getDisplayY();

/* JAVADOC ELIDED */
    void setVisible(boolean visible);

/* JAVADOC ELIDED */
    void setDisplaySize(int width, int height) 
	throws MediaException;

/* JAVADOC ELIDED */
    void setDisplayFullScreen(boolean fullScreenMode) throws MediaException;

    /**
     * Return the width of the source video.  The
     * width must be a positive number.
     * @return the width of the source video
     */ 
    int getSourceWidth();

    /**
     * Return the height of the source video.  The
     * height must be a positive number.
     * @return the height of the source video
     */ 
    int getSourceHeight();

    /**
     * Return the actual width of the current render video.
     *
     * @return width of the display video
     * @exception IllegalStateException Thrown if 
     * <code>initDisplayMode</code> has not been called.
     */
    int getDisplayWidth();

    /**
     * Return the actual height of the current render video.
     *
     * @return height of the display video
     * @exception IllegalStateException Thrown if 
     * <code>initDisplayMode</code> has not been called.
     */
    int getDisplayHeight();

    /**
     * Get a snapshot of the displayed content. Features and format 
     * of the captured image are specified by <code>imageType</code>. 
     * Supported formats can be queried from <code>System.getProperty</code> 
     * with
     * <a href="../../../../overview-summary.html#video.snapshot.encodings">
     * <code>video.snapshot.encodings</code></a> as the key.
     * The first format in the supported list is the default capture format.
     *
     * @param imageType Format and resolution of the returned image.
     * If <code>null</code> is given, the default capture format is used.
     * @return image as a byte array in required format.
     * @exception IllegalStateException Thrown if 
     * <code>initDisplayMode</code> has not been called.
     * @exception MediaException Thrown if the requested format is 
     * not supported or the <code>Player</code> does not support
     * snapshots.
     * @exception SecurityException Thrown if the caller does
     * not have the security permission to take the snapshot.
     */
    byte[] getSnapshot(String imageType) throws MediaException;
}

