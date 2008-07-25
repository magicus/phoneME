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

public interface AutoKeyEvent extends AutoEvent {
    public final static int KEY_PRESSED  = 0;
    public final static int KEY_REPEATED = 1;
    public final static int KEY_RELEASED = 2;
    
    public final static int KEY_INVALID      = 0;
    public final static int KEY_BACKSPACE    = 8;
    public final static int KEY_UP           = -1;
    public final static int KEY_DOWN         = -2;
    public final static int KEY_LEFT         = -3;
    public final static int KEY_RIGHT        = -4;
    public final static int KEY_SELECT       = -5;
    public final static int KEY_SOFT1        = -6;
    public final static int KEY_SOFT2        = -7;
    public final static int KEY_CLEAR        = -8;
    public final static int KEY_SEND         = -10;
    public final static int KEY_END          = -11;
    public final static int KEY_POWER        = -12;
    public final static int KEY_GAMEA        = -13;
    public final static int KEY_GAMEB        = -14;
    public final static int KEY_GAMEC        = -15;
    public final static int KEY_GAMED        = -16;
    public final static int KEY_GAME_UP      = -17;
    public final static int KEY_GAME_DOWN    = -18;
    public final static int KEY_GAME_LEFT    = -19;
    public final static int KEY_GAME_RIGHT   = -20;

    public int getKeyState();
    public int getKeyCode();
}
