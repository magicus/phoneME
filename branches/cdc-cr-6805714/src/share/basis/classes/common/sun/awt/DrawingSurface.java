/*
 * @(#)DrawingSurface.java	1.10 06/10/10
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

/**
 * Objects which control or maintain a portion of screen real
 * estate or other renderable surface (such as an off-screen
 * image buffer) can implement this interface to allow other
 * Java modules or native libraries to gain direct access to
 * their pixels.
 *
 * @version 	1.6, 08/19/02
 * @author 	Jim Graham
 */
public
interface DrawingSurface {
    /**
     * Return the Info object which describes how to directly access
     * the screen drawing surface of this object.
     */
    public DrawingSurfaceInfo	getDrawingSurfaceInfo();
}
