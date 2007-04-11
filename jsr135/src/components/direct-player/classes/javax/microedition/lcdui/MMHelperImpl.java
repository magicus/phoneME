/*
 *
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

package javax.microedition.lcdui;

import javax.microedition.lcdui.Canvas;
import com.sun.mmedia.VidCtrlPainter;
import com.sun.mmedia.VidCtrlCanvasBuddy;
import java.util.Vector;

/**
 * This is a helper class to communicate between the LCDUI canvas and
 * the MMAPI video players. This class replaces the stub class by the
 * same name in the MIDP workspace.
 */
final class MMHelperImpl implements com.sun.mmedia.MMHelper {

    public CanvasLFImpl canvasLFImpl;
    private Vector map;
    private static MMHelperImpl instance = null;
    
    /**
     * Single use constructor. Cannot be instantiated twice. The instance
     * object is held by CanvasLFImpl as well as VidCtrl implementation.
     */
    public MMHelperImpl() {
        if (instance != null) {
            throw new RuntimeException("Cannot instantiate MMHelperImpl twice");
        }
        VidCtrlCanvasBuddy.setMMHelper(this);
        instance = this;
    }

    /**
     * Returns the only instance of this class.
     * @return the instance of this class
     */
    static MMHelperImpl getInstance() {
        return instance;
    }

    /**
     * Informs the players in Canvas <code>c</code> to draw into Graphics
     * <code>g</code>
     *
     * @param c The canvas which originates this paint call.
     * @param g The graphics object to pass to the video player.
     */
     void paintVideo(Canvas c, Graphics g) {
        if (map == null) {
            // Woah! Shouldn't happen.
            return;
        }
        int size = map.size();
        for (int i = 0; i < size; i += 2) {
            if (c == map.elementAt(i)) {
                VidCtrlPainter vcp = (VidCtrlPainter) map.elementAt(i+1);
                vcp.paintVideo(instance, g);
            }
        }
    }

    /**
     * Hide video preview that intersect with bounds
     * if bounds is null, hide all video preview
     */
    void hideVideoPreview(Canvas c, int[] bounds) {
        if (map == null) {
            // Woah! Shouldn't happen.
            return;
        }
        int size = map.size();
        for (int i = 0; i < size; i += 2) {
            if (c == map.elementAt(i)) {
                VidCtrlPainter vcp = (VidCtrlPainter) map.elementAt(i+1);
                vcp.hideVideoPreview(instance, bounds);
            }
        }
    }

    /**
     * Show all video preview
     */
    void showVideoPreview(Canvas c) {
        if (map == null) {
            // Woah! Shouldn't happen.
            return;
        }
        int size = map.size();
        for (int i = 0; i < size; i += 2) {
            if (c == map.elementAt(i)) {
                VidCtrlPainter vcp = (VidCtrlPainter) map.elementAt(i+1);
                vcp.showVideoPreview(instance);
            }
        }    
    }

    /****************************************************************
     * MMHelper implementation
     ****************************************************************/

    /**
     * Registers a video control (which implements VidCtrlPainter) with
     * the corresponding Canvas where the video is to show up.
     */
    public synchronized void registerPlayer(Canvas c, VidCtrlPainter vc) {
        if (map == null) {
            map = new Vector(6);
        }
        
        map.addElement(c);
        map.addElement(vc);
        canvasLFImpl.incEmbeddedPlayers(1);
        
    }

    /**
     * Unregisters a video control so that it doesn't get paint callbacks
     * anymore after the player is closed. This also reduces load on the
     * Canvas repaint mechanism.
     */
    public synchronized void unregisterPlayer(VidCtrlPainter vc) {
        if (map == null)
            return;
        int size = map.size();
        // Note, checking alternate values since its a c,v,c,v sequence
        for (int i = 1; i < size; i += 2) {
            if (map.elementAt(i) == vc) {
                // Get the canvas
                Object c = map.elementAt(i - 1);
                // Unregister this player from the canvas
             
                ((CanvasLFImpl)c).incEmbeddedPlayers(-1);
                
                // Remove the canvas and player from the map.
                // Note: dont interchange these two statements
                map.removeElementAt(i);
                map.removeElementAt(i-1);
                // Only one Canvas per VidCtrl
                break;
            }
        }
    }
}
