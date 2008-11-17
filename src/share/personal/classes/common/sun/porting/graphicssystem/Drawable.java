/*
 * @(#)Drawable.java	1.19 06/10/10
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

package sun.porting.graphicssystem;

/**
 * The <code>Drawable</code> interface specifies operations that are available
 * on all drawable surfaces (either a screen or an offscreen memory region).
 *
 * @version 1.14, 08/19/02
 */
public interface Drawable extends sun.awt.PhysicalDrawingSurface {
    /**
     * Get the size of this <code>Drawable</code>.
     * @return The size in pixels, as a java.awt.Dimension object.
     */
    java.awt.Dimension getSize();
    /**
     * Get the color model of this <code>Drawable</code>.  May be null if the
     * object's color model is indeterminate.
     * @return The color model, as a <code>java.awt.image.ColorModel</code> object.
     */
    java.awt.image.ColorModel getColorModel();
    /**
     * Get a <code>Graphics</code> object associated with this <code>Drawable</code>.
     * The <code>Graphics</code> object must consult the <code>GeometryProvider</code>
     * on every draw, in order to properly clip and translate.
     * The <code>GeometryProvider</code> is what allows the underlying graphics
     * library to properly draw into a window.
     * @see GeometryProvider
     */
    java.awt.Graphics getGraphics(GeometryProvider provider);
}
