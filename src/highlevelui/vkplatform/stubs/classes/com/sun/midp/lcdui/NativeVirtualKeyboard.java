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
package com.sun.midp.lcdui;

import javax.microedition.lcdui.Graphics;

/**
 * Default class for Virtual Keyboard that do nothing when java virtual keyboard isn't supported
 */
public class NativeVirtualKeyboard implements VirtualKeyboard {

    /**
     * Return instance of VirtualKeyboard class
     * @param listener - listener for handling virtual keyboard events
     * @return null
     */
    public static VirtualKeyboard getInstance(VirtualKeyboardListener listener) {
        return null;
    }
    
    /**
     * Set listener
     *
     * @param listener  new virtual keyboard listener
     */
    public void setListener(VirtualKeyboardListener listener) {
    }


    /**
     * Change type of keyboard
     * @param newType type of new shown keyboard
     */
    public void changeKeyboard(String newType) {
    }

    /**
     * Handle input from a pen tap. Parameters describe
     * the type of pen event and the x,y location in the
     * layer at which the event occurred. Important : the
     * x,y location of the pen tap will already be translated
     * into the coordinate space of the layer.
     *
     * @param type the type of pen event
     * @param x    the x coordinate of the event
     * @param y    the y coordinate of the event
     */
    public boolean pointerInput(int type, int x, int y) {
        return false;
    }

    /**
     * paint the virtual keyboard on the screen
     *
     * @param g The graphics context to paint to
     */
    public void paint(Graphics g) {
    }

    /**
     * Set up new coefficients of shrink and resize keyboard. Move keys in new coordinates.
     * @param kshrinkX - coefficient of shrink on X-dimension
     * @param kshrinkY - coefficient of shrink on Y-dimension
     */
    public void resize(double kshrinkX, double kshrinkY) {
    }   

    /**
     * Runs native virtual keyboard to edit the given text.
     * Blocks until edit session is not complete.
     * Returns changed text.
     * @param text
     *  initial text or <code>null</code>;
     * @param maxChars
     *  maximum number of characters in the return string;
     * @param constraint
     *  text editing constraint id;
     * @throws IllegalArgumentException
     *  if number of characters in the initial text is larger then <code>maxChars</code> value;
     *  if <code>maxChars</code> is zero;
     *  if <code>modes</code> or <code>constraint</code> are not recognized;
     * @throws RuntimeException
     *  if editing session was interrupted; e.g. if vm is suspended editing session is interrupted.
     */
    public String editText(String text, int maxChars,
                           int constraints) throws InterruptedException {
        return null;
    }

}
