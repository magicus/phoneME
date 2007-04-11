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

/**
 * This is a helper class that gets the MMHelper instance from
 * the javax.microedition.lcdui package. It hides the MIDPVideoPainter class
 * so that the entire MMAPI classes are not loaded in during MIDlet
 * startup.
 */
public class MIDPRendererCanvasBuddy {
    static private MMHelper mmh = null;

    /**
     * This is the link to the LCDUI canvas implementation for special
     * repaint events. This is called by javax.microedition.lcdui.MMHelperImpl.
     */
    public static void setMMHelper(MMHelper mmhelper) {
        // Safeguard to make sure its called only once
        if (mmh == null)
            mmh = mmhelper;
    }

    /**
     * This method is called by MIDPVideoPainter implementation
     * to get a hold of the MMHelper...
     */
    static MMHelper getMMHelper() {
        return mmh;
    }

}
