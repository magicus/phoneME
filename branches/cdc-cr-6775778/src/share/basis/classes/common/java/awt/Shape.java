/*
 * @(#)Shape.java	1.12 06/10/10
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

package java.awt;

/**
 * The interface for objects which represent some form of geometric
 * shape.
 * <p>
 * This interface will be revised in the upcoming Java2D project.
 * It is meant to provide a common interface for various existing
 * geometric AWT classes and methods which operate on them.  Since
 * it may be superseded or expanded in the future, developers should
 * avoid implementing this interface in their own classes until it
 * is completed in a later release.
 *
 * @version 1.8 08/19/02
 * @author Jim Graham
 */

public interface Shape {
    /**
     * Return the bounding box of the shape.
     */
    public Rectangle getBounds();
}
