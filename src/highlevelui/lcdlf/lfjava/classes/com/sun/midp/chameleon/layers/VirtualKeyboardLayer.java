/*
 * $LastChangedDate: 2006-03-06 01:36:46 +0900 (월, 06 3 2006) $  
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package com.sun.midp.chameleon.layers;

import com.sun.midp.lcdui.*;
import com.sun.midp.configurator.Constants;
import com.sun.midp.chameleon.input.*;
import com.sun.midp.chameleon.skins.VirtualKeyboardSkin;
import com.sun.midp.chameleon.skins.ScreenSkin;
import com.sun.midp.chameleon.skins.AlertSkin;
import com.sun.midp.chameleon.skins.PTISkin;
import com.sun.midp.chameleon.skins.resources.VirtualKeyboardResources;
import com.sun.midp.chameleon.CLayer;
import com.sun.midp.chameleon.MIDPWindow;
import com.sun.midp.chameleon.ChamDisplayTunnel;
import com.sun.midp.i18n.Resource;

import javax.microedition.lcdui.*;
import java.util.Vector;

/**
 * This is a popup layer that handles a sub-popup within the text tfContext
 */
public class VirtualKeyboardLayer extends PopupLayer implements VirtualKeyboardListener {

    /** Instance of current input mode */
    private TextInputSession iSession;

    /** Instance of Displayable */
    private ChamDisplayTunnel tunnel = null;

    /** the instance of the virtual keyboard */
    private static VirtualKeyboard vk = null;


    /**
     * Create an instance of KeyboardLayer
     * @param inputSession current input session
     */
    public VirtualKeyboardLayer(TextInputSession inputSession) {
        super(VirtualKeyboardSkin.BG, AlertSkin.COLOR_BG);
        iSession = inputSession;
        Maps = VirtualKeyboard.prepareKeyMapTextField();
        vk = new VirtualKeyboard(Maps, this, true);
        setAnchor();
    }

    /**
     * Create an instance of KeyboardLayer
     * @param tunnel BodyLayer needs a "tunnel" class to cross the package
     *        protection boundary and access methods inside the
     *        javax.microedition.lcdui package
     */
    public VirtualKeyboardLayer(ChamDisplayTunnel tunnel) {
        super(AlertSkin.IMAGE_BG, PTISkin.COLOR_BG);
        this.tunnel = tunnel;
        Maps = VirtualKeyboard.prepareKeyMapCanvas();
        vk = new VirtualKeyboard(Maps, this, true);
        setAnchor();
    }

    /**
     * Finish initialization of this CLayer. This can
     * be extended by subclasses. The dimensions of the
     * CLayer are stored in its bounds[]. The 'x' and 'y'
     * coordinates are in the coordinate space of the
     * window which contains this layer. By default, a layer is
     * located at the origin and is as large as the screen size.
     *
     * The X and Y coordinates represent the upper left position
     * of this CLayer in the containing CWindow's coordinate space.
     *
     */
    protected void initialize() {
        super.initialize();
        VirtualKeyboardResources.load();
    }

    /**
     * Toggle the visibility state of this layer within its containing
     * window.
     *
     * @param visible If true, this layer will be painted as part of its
     *                containing window, as well as receive events if it
     *                supports input.
     */
    public void setVisible(boolean visible) {
        this.visible = visible;
    }

    /**
     * Sets the anchor constants for rendering operation.
     */
    private void setAnchor() {
        bounds[W] = vk.kbWidth;
        bounds[H] = vk.kbHeight;
        bounds[X] = bounds[X] = (ScreenSkin.WIDTH - bounds[W]) >> 1;
        bounds[Y] = ScreenSkin.HEIGHT - bounds[H];

    }

    /**
     * Handles key event in the open popup.
     *
     * @param type - The type of this key event (pressed, released)
     * @param code - The code of this key event
     * @return true always, since popupLayers swallow all key events
     */
    public boolean keyInput(int type, int code) {

        boolean ret = false;

        if ((type == EventConstants.PRESSED ||
             type == EventConstants.RELEASED ||
             type == EventConstants.REPEATED))
        {
            ret = vk.traverse(type,code);
        }
        return ret;
    }

    /**
     * Utility method to determine if this layer wanna handle
     * the given point. PTI layer handles the point if it
     * lies within the bounds of this layer.  The point should be in
     * the coordinate space of this layer's containing CWindow.
     *
     * @param x the "x" coordinate of the point
     * @param y the "y" coordinate of the point
     * @return true if the coordinate lies in the bounds of this layer
     */
    public boolean handlePoint(int x, int y) {
        return containsPoint(x, y);
    }

    /**
     * Handle input from a pen tap. Parameters describe
     * the type of pen event and the x,y location in the
     * layer at which the event occurred. Important : the
     * x,y location of the pen tap will already be translated
     * into the coordinate space of the layer.
     *
     * @param type the type of pen event
     * @param x the x coordinate of the event
     * @param y the y coordinate of the event
     */
    public boolean pointerInput(int type, int x, int y) {
        return vk.pointerInput(type,x,y);
    }

    /**
     * Paints the body of the popup layer.
     *
     * @param g The graphics context to paint to
     */
    public void paintBody(Graphics g) {
        vk.paint(g);
    }

    /**
     * Update bounds of layer
     * @param layers - current layer can be dependant on this parameter
     */
    public void update(CLayer[] layers) {
        super.update(layers);

        if (visible) {
            setAnchor();
            bounds[Y] -= (layers[MIDPWindow.BTN_LAYER].isVisible() ?
                    layers[MIDPWindow.BTN_LAYER].bounds[H] : 0) +
                    (layers[MIDPWindow.TICKER_LAYER].isVisible() ?
                            layers[MIDPWindow.TICKER_LAYER].bounds[H] : 0);

        }
    }


    // ********** package private *********** //

    /** Indicates if this popup layer is shown (true) or hidden (false). */
    boolean open = false;

    /** the list of available keys */
    //char[][] keys = null;
    Vector Maps = null;


    /**
     * key press callback
     * MIDlet that wants the receive events from the virtual
     * keyboard needs to implement this interface, and register as
     * a listener.
     * @param keyCode char selected by the user from the virtual keyboard
     *
     */
    public void virtualKeyPressed(int keyCode) {

        if (tunnel != null) {
            tunnel.callKeyPressed(keyCode);
        } else {
            iSession.processKey(keyCode, false);
        }
    }

    /**
     * key release callback
     * MIDlet that wants the receive events from the virtual
     * keyboard needs to implement this interface, and register as
     * a listener.
     * @param keyCode char selected by the user from the virtual keyboard
     *
     */
    public void virtualKeyReleased(int keyCode) {
        if (tunnel != null) {
            tunnel.callKeyReleased(keyCode);
        }
    }

    /**
     * repaint the virtual keyboard.
     */
    public void repaintVirtualKeyboard() {
        requestRepaint();
    }

}
