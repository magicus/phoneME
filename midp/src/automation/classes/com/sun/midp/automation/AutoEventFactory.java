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


package com.sun.midp.automation;

/**
 * Factory for events creation.
 */
public interface AutoEventFactory {
    /**
     * Creates event sequence from string representation.
     *
     * @param sequenceString string representation of event sequence
     * @param offset offset into sequenceString
     * @return AutoEventSequence corresponding to the specified string 
     * representation
     * @throws IllegalArgumentException if specified string isn't valid 
     * string representation of events sequence
     */
    public AutoEventSequence createFromString(String sequenceString, int offset)
        throws IllegalArgumentException;

    /**
     * Creates event sequence from string representation.
     *
     * @param sequenceString string representation of event sequence
     * @return AutoEventSequence corresponding to the specified string 
     * representation
     * @throws IllegalArgumentException if specified string isn't valid 
     * string representation of events sequence
     */
    public AutoEventSequence createFromString(String sequenceString)
        throws IllegalArgumentException;


    /**
     * Creates key event.
     *
     * @param keyCode key code not representable as character 
     * (soft key, for example) 
     * @param keyState key state
     * @return AutoKeyEvent instance representing key event
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public AutoKeyEvent createKeyEvent(AutoKeyCode keyCode, 
            AutoKeyState keyState)
        throws IllegalArgumentException;

    /**
     * Creates key event.
     *
     * @param keyChar key char (letter, digit)
     * @param keyState key state
     * @return AutoKeyEvent representing key event
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public AutoKeyEvent createKeyEvent(char keyChar, AutoKeyState keyState)
        throws IllegalArgumentException;

    /**
     * Creates pen event.
     *
     * @param x x coord of pen tip
     * @param y y coord of pen tip
     * @param penState pen state
     * @return AutoPenEvent representing pen event
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public AutoPenEvent createPenEvent(int x, int y, AutoPenState penState) 
        throws IllegalArgumentException;

    /**
     * Creates delay event.
     *
     * @param msec delay value in milliseconds
     * @return AutoDelayEvent representing delay event
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public AutoDelayEvent createDelayEvent(int msec)
        throws IllegalArgumentException;
}
