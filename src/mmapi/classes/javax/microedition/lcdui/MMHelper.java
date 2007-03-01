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

package javax.microedition.lcdui;

import javax.microedition.lcdui.Graphics;

/**
 *
 * @author ng159318
 */
public abstract class MMHelper {
    
    private static MMHelper mmHelper = null;
    
    public static MMHelper getInstance() {
        if (mmHelper == null) {
            try {
                mmHelper = (MMHelper)Class.forName
                        ("javax.microedition.lcdui.MMHelperImpl").newInstance();
            }
            catch (Exception e) {
            }
        }
        return mmHelper;
    }
    
    /**
     * Notify video renderers that canvas becomes invisible.
     * 
     * @param video The video painter which is hidden.
     */
    public abstract void hideVideo(Object video);

    /**
     * Calls paint on the video players that are drawing to the canvas.
     * 
     * @param video The Video painter implementation
     * @param g The Graphics object of the current paint call
     */
    public abstract void paintVideo(Object video, Graphics g);

    /**
     * Notify video renderers that canvas becomes visible.
     * 
     * @param video The video painter which is shown.
     */
    public abstract void showVideo(Object video);
 
    /**
     * Get Display being used for Item painting. Platform-dependent.
     */
    public abstract Display getItemDisplay(Item item);
}
