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

import javax.microedition.lcdui.*;

/**
 * This is a helper interface to communicate between the LCDUI <code>Canvas
 * </code> and
 * the MMAPI video players. It has methods to register and unregister
 * <code>MIDPVideoPainter</code>s with an LCDUI Canvas.
 */
public interface MMHelper {

    /**
     * Registers a video control (which implements MIDPVideoPainter) with
     * the corresponding Canvas where the video is to show up.
     */
    void registerPlayer(Canvas c, MIDPVideoPainter vp);

    /**
     * Unregisters a video control so that it doesn't get paint callbacks
     * anymore after the player is closed. This also reduces load on the
     * Canvas repaint mechanism.
     */
    void unregisterPlayer(Canvas c, MIDPVideoPainter vp);


    /**
     * Get Display being used for Item painting. Platform-dependent.
     */
    Display getItemDisplay(Item item);
}
