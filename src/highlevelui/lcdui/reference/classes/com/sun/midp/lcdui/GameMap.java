/*
 *   
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

package com.sun.midp.lcdui;

import javax.microedition.lcdui.Displayable;
import javax.microedition.lcdui.game.GameCanvas;
import java.util.Hashtable;

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
     * The GraphicsAccess tunnel instance handed out from
     * javax.microedition.lcdui package 
     */
    static private GraphicsAccess graphicsAccess;

    /**
     * Map contains pairs of GameCanvas and GameCanvasLFImpl
     */
    private static Hashtable gameCanvasImpl = new Hashtable();

    /**
     * Lock to ensure synchronized access to the displayable
     */
    static final private Object lock = new Object();

    /**
     * Associates the given Displayable and DisplayAccess.  This is a
     * one-way association.
     *
     * @param c The GameCanvas to store
     * @param d The DisplayAccess associated with the GameCanvas
     */
    public static void registerDisplayAccess(Displayable c, DisplayAccess d) {
        synchronized (lock) {
	        displayable = c;
	        displayAccess = d;
	    }
    }

    /**
     * Gets the DisplayAccess object for this Displayable.
     * @param c The Displayable to get the DisplayAccess for
     * @return DisplayAccess The DisplayAccess associated with the MIDlet
     */
    public static DisplayAccess getDisplayAccess(Displayable c) {
        synchronized (lock) {
  	        if (c == displayable) {
                return displayAccess;
  	        } else {
                return null;
	        }
        }
    }

    /**
     * Associates the given GameCanvas and GameCanvasLFImpl.
     *
     * @param c The GameCanvas to store
     */
    public static GameCanvasLFImpl registerGameCanvas(GameCanvas c) {
        GameCanvasLFImpl gameCanvasLF = new GameCanvasLFImpl(c);
        synchronized(lock) {
            if (!gameCanvasImpl.containsKey(c)) {
                gameCanvasImpl.put(c, gameCanvasLF);
                return gameCanvasLF;
            }
        }
        return null;
    }

    /**
     * Gets the GameCanvasLFImpl object for this GameCanvas.
     * @param c The GameCanvas to get the GameCanvasLFImpl for
     * @return GameCanvasLFImpl
     */
    public static GameCanvasLFImpl getGameCanvasImpl(GameCanvas c) {
        synchronized (lock) {
            if (gameCanvasImpl.containsKey(c)) {
                return (GameCanvasLFImpl) gameCanvasImpl.get(c);
            } else {
                return null;
            }
        }
    }

    /**
     * Sets graphics accessor instance from javax.microedition.lcdui package
     * to use extended package-private Image and Graphics APIs
     *
     * @param graphicsAccess graphics accessor tunnel
     */
    public static void registerGraphicsAccess(GraphicsAccess graphicsAccess) {
        synchronized (lock) {
            GameMap.graphicsAccess = graphicsAccess;
        }
    }

    /**
     * Gets GraphicsAccess instance needed to access extended
     * Image and Graphics APIs
     * 
     * @return GraphicsAccess tunnel instance
     */
    public static GraphicsAccess getGraphicsAccess() {
        synchronized (lock) {
            return graphicsAccess;
        }
    }
}
