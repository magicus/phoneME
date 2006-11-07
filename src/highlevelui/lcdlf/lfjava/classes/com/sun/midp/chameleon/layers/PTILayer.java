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

package com.sun.midp.chameleon.layers;

import com.sun.midp.chameleon.*;
import javax.microedition.lcdui.*;
import javax.microedition.lcdui.game.Sprite;
import java.util.*;
import com.sun.midp.lcdui.Text;
import com.sun.midp.chameleon.skins.PTISkin;
import com.sun.midp.chameleon.skins.ScreenSkin;
import com.sun.midp.chameleon.skins.SoftButtonSkin;
import com.sun.midp.util.ResourceHandler;
import com.sun.midp.lcdui.EventConstants;
import com.sun.midp.configurator.Constants;
import com.sun.midp.chameleon.input.*;

/**
 * A "PTILayer" layer is a special kind of layer which can
 * be visible when the predictive text input mode is active.
 * This layer is added to a MIDPWindow when more than one match
 * exists for the predictive input method. This layer lists the
 * possible words to give user a chance to select word you like.
 * User can traverse the list of words using up/down navigation
 * keys. User may press select bhutton to accept highlighted word.
 */
public class PTILayer extends PopupLayer {
    /** Options have to be listed in the popup dialog */
    private String[] list;

    /** Selected option number */
    private int selId;

    /** Instance of current input mode */
    private TextInputSession iSession;

    /** max text width visible on the screen */
    private int widthMax; 

    /** separator character between words within the list */
    private static final String SEPARATOR = " ";

    /**
     * Create an instance of PTILayer
     * @param inputSession current input session
     */
    public PTILayer(TextInputSession inputSession) {
        super(PTISkin.IMAGE_BG, PTISkin.COLOR_BG);
        layerID = "PTILayer";
        iSession = inputSession;
    }

    /**
     * The setVisible() method is overridden in PTILayer
     * so as not to have any effect. PopupLayers are always
     * visible by their very nature. In order to hide a
     * PopupLayer, it should be removed from its containing
     * MIDPWindow.
     * @param visible if true the pti layer has to be shown,
     * if false the layer has to be hidden
     */
    public void setVisible(boolean visible) {
        this.visible = visible;
    }

    /** PTI layer initialization: init selected id, calculate available size */
    protected void initialize() {
        super.initialize();

        bounds[W] = PTISkin.WIDTH;
        bounds[H] = PTISkin.HEIGHT;
        bounds[X] = (ScreenSkin.WIDTH - bounds[W]) >> 1;
        bounds[Y] = ScreenSkin.HEIGHT - SoftButtonSkin.HEIGHT - bounds[H];
        widthMax = bounds[W] - PTISkin.MARGIN;
        if (PTISkin.LEFT_ARROW != null && PTISkin.RIGHT_ARROW != null) {
            widthMax -= 4 * PTISkin.MARGIN +
                PTISkin.LEFT_ARROW.getWidth() +
                PTISkin.RIGHT_ARROW.getWidth();
        }
        selId = 0;
    }

    /**
     * Set list of matches
     * @param l list of matches
     */
    public synchronized void setList(String[] l) {
        list = new String[l.length];
        System.arraycopy(l, 0, list, 0, l.length);
        visible = (list != null && list.length > 1);
        // IMPL_NOTE: has to be set externally as parameter 
        selId = 0;
        dirty = true;
    }

    /**
     * Get list of matches
     * @return list of matches
     */
    public synchronized String[] getList() {
        return list;
    }

    /**
     * Handle key input from a keypad. Parameters describe
     * the type of key event and the platform-specific
     * code for the key. (Codes are translated using the
     * lcdui.Canvas) UP/DOWN/SELECT key press are processed if 
     * is visible. 
     *
     * @param type the type of key event
     * @param keyCode the numeric code assigned to the key
     * @return true if key has been handled by PTI layer, false otherwise
     */
    public boolean keyInput(int type, int keyCode) {
        boolean ret = false;
        String[] l = getList(); 
        if (type == EventConstants.PRESSED && visible) {
            switch (keyCode) {
            case Constants.KEYCODE_UP:
            case Constants.KEYCODE_LEFT:
                selId = (selId - 1 + l.length) % l.length;
                iSession.processKey(Canvas.UP, false);
                ret = true;
                break;
            case Constants.KEYCODE_DOWN:
            case Constants.KEYCODE_RIGHT:
                selId = (selId + 1) % l.length;
                iSession.processKey(Canvas.DOWN, false);
                ret = true;
                break;
            case Constants.KEYCODE_SELECT:
                // IMPL_NOTE: handle select action
                iSession.processKey(keyCode, false);
                ret = true;
                break;
            default:
                break;
            }
        }
        // process key by input mode 
        requestRepaint();
        return ret;
    }

    /**
     * Allow this window to process pointer input. The type of pointer input
     * will be press, release, drag, etc. The x and y coordinates will 
     * identify the point at which the pointer event occurred in the coordinate
     * system of this window. This window will translate the coordinates
     * appropriately for each layer contained in this window. This method will
     * return true if the event was processed by this window or one of its 
     * layers, false otherwise.
     *
     * @param type the type of pointer event (press, release, drag)
     * @param x the x coordinate of the location of the event
     * @param y the y coordinate of the location of the event
     * @return true if this window or one of its layers processed the event
     */
    public boolean pointerInput(int type, int x, int y) {
        // IMPL_NOTE: handle focus change inside of list 
        return false;
    }

    /**
     * Paint layer body.
     * @param g - Graphics
     */
    protected void paintBody(Graphics g) {
        String[] l = getList();
        if (l == null || l.length < 1)
            return;

        // draw outer frame
        g.setColor(PTISkin.COLOR_BDR);
        g.drawRect(0, 0, bounds[W] - 1, bounds[H] - 1);

        // draw arrows
        if (PTISkin.LEFT_ARROW != null) {
            g.drawImage(PTISkin.LEFT_ARROW, PTISkin.MARGIN, bounds[H] >> 1,
                        Graphics.VCENTER | Graphics.LEFT);
        }
        
        if (PTISkin.RIGHT_ARROW != null) {
            g.drawImage(PTISkin.RIGHT_ARROW, bounds[W] - PTISkin.MARGIN,
                        bounds[H] >> 1, Graphics.VCENTER | Graphics.RIGHT);
        }

        int x = 0, y = 0;

        String text_b = "", text_a = "";
        
        for (int i = -1; ++i < l.length; ) {
            if (i < selId) {
                text_a += l[i] + SEPARATOR;
            } else if (i > selId) {
                text_b += l[i] + SEPARATOR;
            }
        }

        g.translate((bounds[W] - widthMax) >> 1, 0);
        g.setClip(0, 0, widthMax, bounds[H]);
        
        x = 0;
        y = PTISkin.FONT.getHeight() < bounds[H] ?
            (bounds[H] - PTISkin.FONT.getHeight()) >> 1 : 0;
        
        // draw before words 
        if (text_a.length() > 0) {
            g.setColor(PTISkin.COLOR_FG);
            g.drawString(text_a, x, y, Graphics.LEFT | Graphics.TOP);
            x += PTISkin.FONT.stringWidth(text_a); 
        }

        if (l[selId].length() > 0) {
            // draw highlighted word
            // draw highlighted fill rectangle
            g.setColor(PTISkin.COLOR_BG_HL);
            
            g.fillRect(x - PTISkin.FONT.stringWidth(SEPARATOR) / 2,
                       y < PTISkin.MARGIN ? y : PTISkin.MARGIN,
                       PTISkin.FONT.stringWidth(l[selId] + SEPARATOR),
                       bounds[H] - (y < PTISkin.MARGIN ? y :
                                    PTISkin.MARGIN) * 2);
            
            g.setColor(PTISkin.COLOR_FG_HL);
            g.drawString(l[selId] + SEPARATOR, x, y,
                         Graphics.LEFT | Graphics.TOP);
            x += PTISkin.FONT.stringWidth(l[selId] + SEPARATOR);
        }

        // draw after words

        if (text_b.length() > 0) {
            g.setColor(PTISkin.COLOR_FG);
            g.drawString(text_b, x, y, Graphics.LEFT | Graphics.TOP);
        }

        g.translate(-((bounds[W] - widthMax) >> 1), 0);
        g.setClip(0, 0, bounds[W], bounds[H]);
    }
}


