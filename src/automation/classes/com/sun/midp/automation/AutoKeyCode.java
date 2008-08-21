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
    public final static AutoKeyCode BACKSPACE = 
        new AutoKeyCode("BACKSPACE");

    public final static AutoKeyCode UP = 
        new AutoKeyCode("UP");

    public final static AutoKeyCode DOWN = 
        new AutoKeyCode("DOWN");

    public final static AutoKeyCode LEFT = 
        new AutoKeyCode("LEFT");

    public final static AutoKeyCode RIGHT = 
        new AutoKeyCode("RIGHT");

    public final static AutoKeyCode SELECT = 
        new AutoKeyCode("SELECT");

    public final static AutoKeyCode SOFT1 = 
        new AutoKeyCode("SOFT1");

    public final static AutoKeyCode SOFT2 = 
        new AutoKeyCode("SOFT2");

    public final static AutoKeyCode CLEAR = 
        new AutoKeyCode("CLEAR");

    public final static AutoKeyCode SEND = 
        new AutoKeyCode("SEND");

    public final static AutoKeyCode END = 
        new AutoKeyCode("END");
 
    public final static AutoKeyCode POWER = 
        new AutoKeyCode("POWER");

    public final static AutoKeyCode GAMEA = 
        new AutoKeyCode("GAMEA");

    public final static AutoKeyCode GAMEB = 
        new AutoKeyCode("GAMEB");

    public final static AutoKeyCode GAMEC = 
        new AutoKeyCode("GAMEC");

    public final static AutoKeyCode GAMED = 
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
