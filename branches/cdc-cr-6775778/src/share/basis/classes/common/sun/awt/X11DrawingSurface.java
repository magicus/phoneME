/*
 * @(#)X11DrawingSurface.java	1.10 06/10/10
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
 *
 */

package sun.awt;

import sun.awt.PhysicalDrawingSurface;
import java.awt.image.ColorModel;

/**
 * The X11DrawingSurface interface defines the methods needed to
 * be able to render to an X11 window.  Only the commonly needed
 * attributes are returned, other attributes can be queried by
 * calling the X11 XGetWindowAttributes() function.
 *
 * Many of the methods return Java integers which must be cast to the
 * appropriate Windows data types using a cast similar to the pseudocode
 * indicated in the class comments of the various methods.
 *
 * @version 	1.6, 08/19/02
 * @author 	Jim Graham
 */
public interface X11DrawingSurface extends PhysicalDrawingSurface {
    /**
     * Return the X11 Display pointer for the window or pixmap
     * represented by this object.
     * <p>
     * Display *dpy = (Display *) getDisplay();
     */
    public int getDisplay();
    /**
     * Return the X11 Drawable ID for the window or pixmap
     * represented by this object.
     * <p>
     * Drawable d = (Drawable) getDrawable();
     */
    public int getDrawable();
    /**
     * Return the depth of the window or pixmap
     * represented by this object.
     * <p>
     * int depth = getDepth();
     */
    public int getDepth();
    /**
     * Return the X11 Visual ID of the window or pixmap
     * represented by this object.
     * <p>
     * VisualID vid = (VisualID) getVisualID();
     */
    public int getVisualID();
    /**
     * Return the X11 Colormap ID of the window or pixmap
     * represented by this object.
     * <p>
     * Colormap cmapid = (Colormap) getColormapID();
     */
    public int getColormapID();
    /**
     * Return the ColorModel of the window or pixmap
     * represented by this object.
     * <p>
     * Hjava_awt_image_ColorModel *cm =
     *     (Hjava_awt_image_ColorModel *) getColorModel();
     */
    public ColorModel getColorModel();
}
