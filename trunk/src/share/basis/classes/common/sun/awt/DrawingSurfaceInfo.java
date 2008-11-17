/*
 * @(#)DrawingSurfaceInfo.java	1.10 06/10/10
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

import java.awt.Rectangle;
import java.awt.Shape;

/**
 * The DrawingSurfaceInfo object provides a common interface to
 * access all information about how to gain access to the drawing
 * surface controlled by some Java object.  The information can
 * be divided into two different categories, the logical drawing
 * boundaries and the physical surface handle.  The physical handle
 * is represented by an implementation-specific object which the
 * caller must be able to recognize in order to render directly to
 * the surface.  Many of the physical handles will be usable only
 * by native code which accesses platform-specific drawing interfaces.
 *
 * @version 	1.6, 08/19/02
 * @author 	Jim Graham
 */
public interface DrawingSurfaceInfo {
    /**
     * Lock the current state of the drawing surface in preparation
     * for drawing to it and return an integer "change identifier"
     * which can be used to determine when changes have occured to
     * the drawing surface specification and when to retrieve new
     * extent and physical handle data.  If the identifier has not
     * changed since the previous time lock and unlock were called
     * then the consumer of this information is free to reuse clipping
     * and handle information cached during the previous rendering
     * operation.  If the identifier changes, then all extent,
     * clipping, and handle information must be retrieved again.
     * The unlock() method must be called when rendering is complete
     * or the component may be permanently locked down on the screen
     * and other rendering threads may block indefinately.
     */
    public int			lock();
    /**
     * Unlock the drawing surface and allow other threads to
     * access or change its composition.
     */
    public void			unlock();
    /**
     * Returns a rectangle describing the logical bounds of the given
     * screen object relative to the "drawing handle" information
     * returned by getSurface().  Note that there may be portions
     * of this bounding rectangle that are obscured.  This value
     * defines the location of the logical upper-left origin of
     * the component or screen object and its desired extents
     * (which may differ from the clipping extents due to the
     * presence of other overlapping screen objects).  The getClip()
     * method should be used to determine which portions of the
     * drawing surface can actually be drawn to.
     */
    public Rectangle		getBounds();
    /**
     * Returns an implementation-specific object which describes a
     * handle to the physical drawing surface on which this screen
     * object resides.  Depending on the type of the return value,
     * this object may describe a Windows hWnd handle, an X11
     * drawable handle, a Mac GrafPtr structure, or some other
     * platform-specific handle.
     */
    public PhysicalDrawingSurface	getSurface();
    /**
     * Returns an explicit representation of the renderable portion
     * of the given screen object.  This clipping area will include
     * only those areas of the screen object that are not obscured
     * by other overlapping objects and which fall within the
     * boundaries of the physical drawing surface.  The area described
     * by the return value of this method can be directly used as a
     * graphics clip area to restrict rendering to the given screen
     * object.
     */
    public Shape			getClip();
}
