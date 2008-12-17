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
 * Represents key event.
 */
public interface AutoKeyEvent extends AutoEvent {
    /**
     * Gets key state.
     *
     * @return AutoKeyState representing key state
     */
    public AutoKeyState getKeyState();

    /**
     * Gets key code.
     *
     * @return AutoKeyCode representing key code if event has a key code, 
     * or null, if event has key char instead
     */
    public AutoKeyCode  getKeyCode();

    /**
     * Gets key char.
     *
     * @return key char if event has it, or unspecified,
     * if event has key code instead
     */
    public char getKeyChar();

    /**
     * Gets string representation of this event. The format is following:
     * <br>&nbsp;&nbsp;
     * <b>key code:</b> <i>code_value</i><b>, state:</b> <i>state_value</i>
     * <br>
     * where <i>code_value</i> and <i>state_value</i> are string 
     * representation of key code (key char) and key state.
     * <br>
     * For example:
     * <br>&nbsp;&nbsp;
     * <b>key code: soft1, state: pressed</b>
     * <br>&nbsp;&nbsp;
     * <b>key code: a, state: pressed</b>
     */
    public String toString();
}
