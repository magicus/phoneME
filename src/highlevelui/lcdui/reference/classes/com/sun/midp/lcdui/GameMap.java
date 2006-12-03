/*
 *   
 *
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

package com.sun.midp.lcdui;

import javax.microedition.lcdui.Displayable;

/**
 * A class that maps between DisplayAccess objects and Displayable, GameCanvas.
 * In future versions of the MIDP spec, GameCanvas may have been
 * moved to lcdui package, in which case this class isn't needed.
 */
public class GameMap {
    /**
     * The Displayable associated with the DisplayAccess
     */
    static private Displayable displayable;
    /**
     * The DisplayAccess associated with the GameCanvas
     */
    static private DisplayAccess displayAccess;

    /**
     * Lock to ensure synchronized access to the displayable
     */
    static final private Object lock = new Object();

    /**
     * Associate the given Displayable and DisplayAccess.  This is a
     * one-way association.
     *
     * @param c The GameCanvas to store
     * @param d The DisplayAccess associated with the GameCanvas
     */
    public static void register(Displayable c, DisplayAccess d) {
        synchronized (lock) {
	    displayable = c;
	    displayAccess = d;
	}
    }


    /**
     * Get the DisplayAccess object for this Displayable.
     * @param c The Displayable to get the DisplayAccess for
     * @return DisplayAccess The DisplayAccess associated with the MIDlet
     */
    public static DisplayAccess get(Displayable c) {
        synchronized (lock) {
  	    if (c == displayable) {
                return displayAccess;
  	    } else {
                return null;
	    }
        }
    }

}
