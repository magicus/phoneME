/*
 *  
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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
import java.lang.ref.WeakReference;
import java.util.Enumeration;
import java.util.Hashtable;

import javax.microedition.lcdui.Canvas;
import javax.microedition.lcdui.Displayable;

import com.sun.lcdui.VirtualKeyboardManager;

/**
 * A class that maps between Displayable and
 * VirtualKeyboardManager object. It is used due to
 * impossibility to access nonpublic fields of classes in
 * javax.microedition.lcdui domain.
 * 
 */
public class VirtualKeyboardManagerMap {

    /** A table to store the map */
    private static Hashtable managers = new Hashtable();

    /**
     * Returns keyboard manager for given Displayable.
     * <p> 
     * Canvas is the only class that can provide such a manager.
     * 
     * @param d Displayable that wants user input
     * 
     * @return VirtualKeyboard controller
     */
    public static VirtualKeyboardManager getInstance(Displayable d) {
        if (!(d instanceof Canvas)) {
            return null;
        }
        Enumeration e = managers.keys();
        while (e.hasMoreElements()) {
            WeakReference ref = (WeakReference)e.nextElement();
            if (null == ref.get()) {
                managers.remove(ref);
            } else if (ref.get() == d) {
                return (VirtualKeyboardManager)((WeakReference)managers.get(ref)).get();
            }
        }
        return null;
    }

    /**
     * Registers Canvas-Manager pair.
     * 
     * 
     * @param c canvas object where virtual keyboard is available
     * @param m VirtualKeyboardManager
     */
    public static void registerManager(Canvas c, VirtualKeyboardManager m) {
        WeakReference refC = new WeakReference(c);
        WeakReference refM = new WeakReference(m);
        managers.put(refC, refM);
    }
}
