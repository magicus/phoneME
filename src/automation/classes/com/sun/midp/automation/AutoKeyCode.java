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

public final class AutoKeyCode {
    public static AutoKeyCode BACKSPACE = 
        new AutoKeyCode("BACKSPACE");

    public static AutoKeyCode UP = 
        new AutoKeyCode("UP");

    public static AutoKeyCode DOWN = 
        new AutoKeyCode("DOWN");

    public static AutoKeyCode LEFT = 
        new AutoKeyCode("LEFT");

    public static AutoKeyCode RIGHT = 
        new AutoKeyCode("RIGHT");

    public static AutoKeyCode SELECT = 
        new AutoKeyCode("SELECT");

    public static AutoKeyCode SOFT1 = 
        new AutoKeyCode("SOFT1");

    public static AutoKeyCode SOFT2 = 
        new AutoKeyCode("SOFT2");

    public static AutoKeyCode CLEAR = 
        new AutoKeyCode("CLEAR");

    public static AutoKeyCode SEND = 
        new AutoKeyCode("SEND");

    public static AutoKeyCode END = 
        new AutoKeyCode("END");
 
    public static AutoKeyCode POWER = 
        new AutoKeyCode("POWER");

    public static AutoKeyCode GAMEA = 
        new AutoKeyCode("GAMEA");

    public static AutoKeyCode GAMEB = 
        new AutoKeyCode("GAMEB");

    public static AutoKeyCode GAMEC = 
        new AutoKeyCode("GAMEC");

    public static AutoKeyCode GAMED = 
        new AutoKeyCode("GAMED");


    public String getName() {
        return name;
    }

    int getMIDPKeyCode() {
        return midpKeyCode;
    }


    private AutoKeyCode(String name) {
        this.name = name;
        this.midpKeyCode = AutoKeyEventImpl.registerKeyCode(this);
    }

    private String name;   
    private int midpKeyCode;    
}
