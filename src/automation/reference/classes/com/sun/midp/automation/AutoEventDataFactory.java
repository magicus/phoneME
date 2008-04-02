/*
 *   
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
 */

package com.sun.midp.automation;
import java.io.*;

/**
 * Factory creating event's data for various types
 * of events.
 */
public final class AutoEventDataFactory {
    /** Key event: key is pressed */
    public static final int KEY_PRESSED = 0;

    /** Key event: key is repeated */
    public static final int KEY_REPEATED = 1;

    /** Key event: key is released */
    public static final int KEY_RELEASED = 2;
    
    /** Pen event: pen is pressed */
    public static final int PEN_PRESSED = 3;

    /** Pen event: pen is dragged */
    public static final int PEN_DRAGGED = 4;

    /** Pen event: pen is released */
    public static final int PEN_RELEASED = 5;    
    
    /** 
     * Creates event's data for key event.
     *
     * @param type type of event: KEY_PRESSED, KEY_REPEATED, KEY_RELEASED
     * @param code key's code
     *
     * @return AutoEventData representing event's data for key event 
     */
    public static AutoEventData createKeyEventData(int type, int code) { 
        return null;
    }
    
    /** 
     * Creates event's data for input method event.
     *
     * @param str entered string
     *
     * @return AutoEventData representing event's data for IM event 
     */
    public static AutoEventData createInputMethodEventData(String str) {
        return null;
    }    
    
    /** 
     * Creates event's data for pen event.
     *
     * @param type type of event: PEN_PRESSED, PEN_DRAGGED, PEN_RELEASED
     * @param x x coordinate of the pen
     * @param y y coordinate of the pen
     *
     * @return AutoEventData representing event's data for pen event 
     */
    public static AutoEventData createPenEventData(int type, int x, int y) {
        return null;
    }
    

    /**
     * Creates event's data for command event.
     *
     * @param type type of command event
     * @return AutoEventData representing event's data for command event 
     */
    public static AutoEventData createCommandEventData(int type) {
        return null;
    }

    /**
     * Creates event's data from input stream
     *
     * @param stream stream to read data from.
     * 
     * @return AutoEventData constructed from stream.
     */
    public static AutoEventData createFromStream(DataInputStream stream) {
        return null;
    }


    /**
     * Private constructor to prevent user from creating an instance.
     */
    private AutoEventDataFactory() {
    }
}
