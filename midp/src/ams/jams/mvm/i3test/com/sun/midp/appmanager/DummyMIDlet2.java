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

package com.sun.midp.appmanager;

import javax.microedition.lcdui.Alert;
import javax.microedition.lcdui.Display;

import javax.microedition.midlet.MIDlet;

/**
 * A dummy MIDlet that displays set its current displayable to an alert when
 * active and sets its displayable to null when paused.
 */
public class DummyMIDlet2 extends MIDlet {
    public void startApp() {
        Alert a = new Alert("MIDlet 2");
        a.setTimeout(Alert.FOREVER);
        Display.getDisplay(this).setCurrent(a);
    }
   
    public void pauseApp() {
        Display.getDisplay(this).setCurrent(null);
    }

    public void destroyApp(boolean unconditional) {
    }
}
