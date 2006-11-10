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

package com.sun.midp.chameleon;

import com.sun.midp.chameleon.*;
import com.sun.midp.chameleon.layers.*;
import com.sun.midp.chameleon.skins.*;
import com.sun.midp.chameleon.skins.resources.*;

import javax.microedition.lcdui.*;

/**
 * The MIDPWindow class is a concrete instance of a CWindow which
 * implements the MIDP specification and its graphical elements,
 * such as a title bar, soft buttons, ticker, etc.
 */
public class MIDPWindow extends CWindow {

    /** Used to call back into the Display class from this package */
    ChamDisplayTunnel tunnel;

    /**
     * True, if the current displayable is occupying
     * as much screen as possible
     */
    boolean hasFullScreen;

    /** Cache of screen commands */
    Command[] scrCmdCache;

    /** Number of screen commands in the cache */
    int scrCmdCount;

    /** Listener to notify when a screen command is selected */
    CommandListener scrCmdListener;

    /** Cache of selected item commands */
    Command[] itemCmdCache;

    /** Number of item commands in the cache */
    int itemCmdCount;

    /** Listener to notify when an item command is selected */
    ItemCommandListener itemCmdListener;

    /** Layer containing the current displayable's contents */
    BodyLayer bodyLayer;

    /** Layer containing the pti contents */
    PTILayer ptiLayer;

    /** Layer containing the title of the current displayable */
    TitleLayer titleLayer;

    /** Layer containing the ticker of the current displayable */
    TickerLayer tickerLayer;

    /** Layer rendering the soft button controls */
    SoftButtonLayer btnLayer;

    /** The layer that displays alert */
    AlertLayer alertLayer;

    /** The layer which displays scroll indicators */
    ScrollIndLayer scrollLayer;

    /**
     * The 'wash' layer which shades the background when the menu or
     * an alert is visible
     */
    WashLayer washLayer;

    /** Determines whether area of the window has been changed */
    boolean sizeChangedOccured = false;

    /**
     * Construct a new MIDPWindow given the tunnel to the desired
     * MIDP Display instance
     *
     * @param tunnel the "tunnel" to make calls from this java package
     *               back into the Display object in another package
     */
    public MIDPWindow(ChamDisplayTunnel tunnel) {
        super(ScreenSkin.IMAGE_BG, ScreenSkin.COLOR_BG);

        this.tunnel = tunnel;

        bodyLayer = new BodyLayer(tunnel);
        addLayer(bodyLayer);

        titleLayer = new TitleLayer();
        addLayer(titleLayer);

        scrollLayer = new ScrollIndLayer();

        btnLayer = new SoftButtonLayer(tunnel, scrollLayer);
        addLayer(btnLayer);

        addLayer(scrollLayer);

        tickerLayer = new TickerLayer();
        addLayer(tickerLayer);

        washLayer = new WashLayer();
        addLayer(washLayer);
    }

    /**
     * Request a repaint. This method does not require any bounds
     * information as it is contained in each of the Chameleon layers.
     * This method simply results in a repaint event being placed in
     * the event queue for a future callback.
     */
    public void requestRepaint() {
        if (tunnel != null) {
            tunnel.scheduleRepaint();
        }
    }

    /**
     * Set the title of this MIDPWindow. This would typically
     * correspond to the title of the current displayable, and
     * may result in the title layer appearing or disappearing.
     *
     * @param title the value of the title. null indicates there
     *              is no title.
     */
    public void setTitle(String title) {
        titleLayer.setTitle(title);
        setBodyBounds();
        requestRepaint();
    }

    /**
     * Set the ticker of this MIDPWindow. This would typically
     * correspond to the ticker of the current displayable, and
     * may result in the ticker layer appearing or disappearing.
     *
     * @param ticker the current Ticker object. null indicates there
     *              is no ticker.
     */
    public void setTicker(Ticker ticker) {
        tickerLayer.setText((ticker != null) ? ticker.getString() : null);
        setBodyBounds();
        requestRepaint();
    }

    /**
     * Alert this MIDPWindow that the given displayable is now current
     * and should be shown on the screen.
     *
     * This will establish the given displayable on the screen,
     * as well as reflect the displayable's title and ticker (if any).
     * Special circumstances may occur if the displayable is an Alert,
     * such as maintaining the current screen contents and showing the
     * Alert in a popup.
     *
     * @param displayable the newly current displayable to show
     * @param height the preferred height of the new displayable
     */
    public void showDisplayable(Displayable displayable, int height) {
        bodyLayer.dirty = true;

        Ticker ticker = displayable.getTicker();

        bodyLayer.opaque =  (displayable instanceof Canvas);

        if (displayable instanceof Alert) {
	    bodyLayer.setVisible(false);
            tickerLayer.toggleAlert(true);
            btnLayer.toggleAlert(true);
            if (alertLayer == null) {
                alertLayer = new AlertLayer(tunnel);
                addLayer(alertLayer);
            }
            alertLayer.setAlert(true, (Alert)displayable, height);
            paintWash(true);
        } else {
	    bodyLayer.setVisible(true);
            btnLayer.toggleAlert(false);
            tickerLayer.toggleAlert(false);
            titleLayer.setTitle(displayable.getTitle());
            paintWash(false);
        }

        setTicker(ticker);

        setBodyBounds();
        if (displayable instanceof Alert) {
            // Special case. Normally we always repaint the title
            // layer, but in the case of Alert, it has its own, so
            // we override the standard behavior and make sure the
            // title is not painted. The softbutton and ticker
            // layers are used by Alert so we leave them alone.
            titleLayer.dirty = false;
        }
        requestRepaint();
    }

    /**
     * Alert this MIDPWindow that the given displayable is no longer
     * current and should be removed from the screen.
     *
     * Special circumstances may occur if the displayable is an Alert,
     * such as removing the popup and re-instating the previous
     * displayable which was visible before the Alert popped up.
     *
     * @param displayable the newly current displayable to show
     */
    public void hideDisplayable(Displayable displayable) {
        if (displayable instanceof Alert) {
	    bodyLayer.setVisible(false);
            tickerLayer.toggleAlert(false);
            btnLayer.toggleAlert(false);
            alertLayer.setVisible(false);
        }

        btnLayer.dismissMenu();

        // Make sure that not of the popups are shown
        clearPopups();
    }

    /**
     * Determines if the system menu is currently visible. This can be useful
     * in determining the current isShown() status of the displayable.
     *
     * @return true if the system menu is up
     */
    public boolean systemMenuUp() {
        return btnLayer.systemMenuUp();
    }

    /**
     * Request a repaint of a region of the current displayable.
     * This method specifically marks a region of the body layer
     * (which renders the displayable's contents) as dirty and
     * results in a repaint request being scheduled. The coordinates
     * are in the space of the displayable itself - that is, 0,0
     * represents the top left corner of the body layer.
     *
     * @param x the x coordinate of the dirty region
     * @param y the y coordinate of the dirty region
     * @param w the width of the dirty region
     * @param h the height of the dirty region
     */
    public void repaintDisplayable(int x, int y, int w, int h) {
        // We mark the body layer as dirty
        if (alertLayer != null && alertLayer.visible) {
            alertLayer.addDirtyRegion(x, y, w, h);
        } else {
            bodyLayer.addDirtyRegion(x, y, w, h);
        }
        requestRepaint();
    }

    /**
     * Add the given layer to this window. This method is
     * overridden from CWindow in order to special case
     * popup layers. Popup layers can have their own commands
     * which supercede those of the current displayable.
     *
     * @param layer the CLayer to add to this window
     * @return the index of the new layer in the overall layer stack
     */
    public int addLayer(CLayer layer) {
        int i = super.addLayer(layer);

        if (i > 0 && layer instanceof PopupLayer) {
            PopupLayer popup = (PopupLayer)layer;
            popup.dirty = true;

            Command[] cmds = popup.getCommands();
            if (cmds != null) {
                btnLayer.updateCommandSet(null, 0, null, cmds, cmds.length,
                                          popup.getCommandListener());
            }
        }

        if (i > 0 && layer instanceof PTILayer) {
            ptiLayer = (PTILayer)layer;
            setBodyBounds();
            // move wash layer at the top
            removeLayer(washLayer);
            addLayer(washLayer);
        }

        return i;
    }

    /**
     * Remove the given layer from this window. This method is
     * overridden from CWindow in order to special case popup
     * layers. Popup layers can have their own commands which
     * supercede those of the current displayable. In this case,
     * the popup is removed and the commands in the soft button
     * bar are restored to either the next top-most popup layer
     * or the current displayable itself.
     *
     * @param layer the CLayer to remove from this window
     * @return true if the layer was able to be removed
     */
    public boolean removeLayer(CLayer layer) {
        if (super.removeLayer(layer) && layer instanceof PopupLayer) {
            if (layer == ptiLayer) {
                ptiLayer = null;
                setBodyBounds();
            }

            PopupLayer popup = (PopupLayer)layer;

            if (popup instanceof MenuLayer) {
                bodyLayer.addDirtyRegion();
            }

            // We need to mark lower layers as dirty now
            CLayer l;
            for (int i = 0; i < layers.size(); i++) {
                l = (CLayer)layers.elementAt(i);
                if (l.visible && l.intersectsRegion(popup.bounds)) {
                    if (CGraphicsQ.DEBUG) {
                        System.err.println(
                            "Adding dirty region from popup to " + l.layerID);
                    }
                    l.addDirtyRegion(popup.bounds[X] - l.bounds[X],
                                     popup.bounds[Y] - l.bounds[Y],
                                     popup.bounds[W], popup.bounds[H]);
                }
            }

            // Now we update the command set with either the
            // next top most popup or the original cached commands
            PopupLayer p = getTopMostPopup();
            if (p != null && p.getCommands() != null) {
                Command[] cmds = p.getCommands();
                btnLayer.updateCommandSet(null, 0,
                                          null,
                                          cmds, cmds.length,
                                          p.getCommandListener());
            } else {
                btnLayer.updateCommandSet(itemCmdCache, itemCmdCount,
                                          itemCmdListener,
                                          scrCmdCache, scrCmdCount,
                                          scrCmdListener);
            }
            return true;
        }
        return false;
    }

    /**
     * Update this MIDPWindow's current command set to match the
     * current displayable and possibly item selection.
     *
     * @param itemCommands the set of item specific commands
     * @param itemCmdCount the number of item commands
     * @param itemCmdListener the notification listener for item commands
     * @param scrCommands the set of screen specific commands
     * @param scrCmdCount the number of screen commands
     * @param scrCmdListener the notification listener for screen commands
     */
    public void updateCommandSet(Command[] itemCommands,
                                 int itemCmdCount,
                                 ItemCommandListener itemCmdListener,
                                 Command[] scrCommands,
                                 int scrCmdCount,
                                 CommandListener scrCmdListener)
    {
        // We cache commands to easily reset them when a
        // popup takes precedence and then is dismissed
        this.itemCmdCache = itemCommands;
        this.itemCmdCount = itemCmdCount;
        this.itemCmdListener = itemCmdListener;
        this.scrCmdCache = scrCommands;
        this.scrCmdCount = scrCmdCount;
        this.scrCmdListener = scrCmdListener;

        btnLayer.updateCommandSet(itemCommands, itemCmdCount,
                                  itemCmdListener,
                                  scrCommands, scrCmdCount,
                                  scrCmdListener);
    }

    /**
     * Set this MIDPWindow's displayable to "fullscreen" mode. This
     * will expand the region occupied by the current displayable to
     * include the area previously occupied by the title and ticker
     * if present
     *
     * @param onOff true if the displayable should be in fullscreen mode
     */
    public void setFullScreen(boolean onOff) {
        hasFullScreen = onOff;
        setBodyBounds();
        requestRepaint();
    }

    /**
     * Called to paint a wash over the background of this window.
     * Used by SoftButtonLayer when the system menu pops up, and
     * internally when an Alert is shown.
     *
     * @param onOff A flag indicating if the wash should be on or off
     */
    public void paintWash(boolean onOff) {
	if (alertLayer != null && alertLayer.visible) {
	    washLayer.setVisible(true);
	    if (!onOff) {
		tickerLayer.addDirtyRegion();
		alertLayer.addDirtyRegion();
	    }
	} else {
	    washLayer.setVisible(onOff);
	    if (!onOff) {
		tickerLayer.addDirtyRegion();
		bodyLayer.addDirtyRegion();
		titleLayer.addDirtyRegion();
		if (ptiLayer != null) {
                    ptiLayer.addDirtyRegion();
                }
	    }
	}
    }

    /**
     * Returns the left soft button (one).
     *
     * @return the command that's tied to the left soft button
     */
    public Command getSoftOne() {
        return btnLayer.getSoftOne();
    }

    /**
     * Returns the command array tied to the right soft button (two).
     *
     * @return the command array that's tied to the right soft button
     */
    public Command[] getSoftTwo() {
        return btnLayer.getSoftTwo();
    }

    /**
     * Set the current vertical scroll position and proportion.
     *
     * @param scrollPosition vertical scroll position.
     * @param scrollProportion vertical scroll proportion.
     */
    public void setVerticalScroll(int scrollPosition, int scrollProportion) {
        // IMPL NOTE: add btnLayer.sysMenuUp()

        if (alertLayer != null) {
            scrollLayer.setVerticalScroll(alertLayer.visible, scrollPosition,
                                          scrollProportion);
        } else {
            scrollLayer.setVerticalScroll(false, scrollPosition,
                                          scrollProportion);
        }
    }

    /**
     * Get the current x anchor coordinate for the body layer (the body
     * layer renders the contents of the current displayable).
     *
     * @return the x anchor coordinate of the body layer
     */
    public int getBodyAnchorX() {
        return bodyLayer.bounds[X];
    }

    /**
     * Get the current y anchor coordinate for the body layer (the body
     * layer renders the contents of the current displayable).
     *
     * @return the y anchor coordinate of the body layer
     */
    public int getBodyAnchorY() {
        return bodyLayer.bounds[Y];
    }

    /**
     * Get the current width of the body layer (the body
     * layer renders the contents of the current displayable).
     *
     * @return the width of the body layer
     */
    public int getBodyWidth() {
        return bodyLayer.bounds[W];
    }

    /**
     * Get the current height of the body layer (the body
     * layer renders the contents of the current displayable).
     *
     * @return the height of the body layer
     */
    public int getBodyHeight() {
        return bodyLayer.bounds[H];
    }

    /**
     * MIDPWindow overrides the parent paint method in order to
     * do special effects such as paint a "wash" over the background
     * when a dialog is up. Also in an effort to call
     * {@link javax.microedition.lcdui.Displayable#sizeChanged }
     * method before painting. This implementation determine whether size
     * has been changed and calls <code>sizeChanged()</code> if it's so.
     * Anyway it invokes the base class's {@link CWindow#paint} method.
     *
     * @param g The graphics object to use to paint this MIDP window.
     * @param refreshQ The chameleon graphics queue.
     */
    public void callPaint(Graphics g, CGraphicsQ refreshQ) {
        if (sizeChangedOccured) {
            if (tunnel != null) {
                int w = getBodyWidth();
                int h = getBodyHeight();
                tunnel.callSizeChanged(w, h);
                sizeChangedOccured = false;
            }
        }
        super.paint(g, refreshQ);
    }

    /**
     * This method is an optimization which allows Display to bypass
     * the Chameleon paint engine logic and directly paint an animating
     * canvas. Display will call this method with the graphics context
     * and this method will either return false, indicating the Chameleon
     * paint engine should not be bypassed, or will return true and will
     * setup the graphics context for the canvas to be painted directly.
     *
     * @param g the graphics context to setup
     * @return true if Chameleon's paint logic can be bypassed and the
     *         canvas can be rendered directly.
     */
    public boolean setGraphicsForCanvas(Graphics g) {
        if (super.dirty || !bodyLayer.opaque) {
            return false;
        }

        // NOTE: note the two different orders of clip and translate
        // below. That is because the layer's bounds are stored in
        // the coordinate space of the window. But its internal dirty
        // region is stored in the coordinate space of the layer itself.
        // Thus, for the first one, the clip can be set and then translated,
        // but in the second case, the translate must be done first and then
        // the clip set.
        if (bodyLayer.dirtyBounds[X] == -1) {
            g.setClip(bodyLayer.bounds[X], bodyLayer.bounds[Y],
                      bodyLayer.bounds[W], bodyLayer.bounds[H]);
            g.translate(bodyLayer.bounds[X], bodyLayer.bounds[Y]);
        } else {
            g.translate(bodyLayer.bounds[X], bodyLayer.bounds[Y]);
            g.setClip(bodyLayer.dirtyBounds[X], bodyLayer.dirtyBounds[Y],
                      bodyLayer.dirtyBounds[W], bodyLayer.dirtyBounds[H]);
        }

        return true;
    }

    /**
     * Internal method to establish the bounds of the body layer.
     * This is important to re-calculate whenever things such as
     * titles, tickers, fullscreen mode, etc. change state.
     */
    protected void setBodyBounds() {
        int oldHeight = bodyLayer.bounds[H];
        int oldWidth = bodyLayer.bounds[W];
        int oldX = bodyLayer.bounds[X];
        int oldY = bodyLayer.bounds[Y];

        if (hasFullScreen) {
            bounds[H] = ScreenSkin.FULLHEIGHT;
            titleLayer.visible = false;
            tickerLayer.visible = false;
            // Just hide soft button layer. Key input remains supported.
            btnLayer.visible = false;
        } else {
            bounds[H] = ScreenSkin.HEIGHT;
            titleLayer.visible = (titleLayer.getTitle() != null);
            tickerLayer.visible = (tickerLayer.getText() != null);
            btnLayer.visible = true;
        }

        // IMPL NOTE: The below calculations don't take into account
        // the placement of the ticker and title!!

        bodyLayer.bounds[W] = bounds[W];
        bodyLayer.bounds[H] = bounds[H];

        if (btnLayer.visible) {
            bodyLayer.bounds[H] -= btnLayer.bounds[H];
            btnLayer.addDirtyRegion();
        }

        scrollLayer.setAnchor();
        scrollLayer.addDirtyRegion();

        if (titleLayer.visible) {
            bodyLayer.bounds[Y] = titleLayer.bounds[H];
            bodyLayer.bounds[H] -= titleLayer.bounds[H];
            titleLayer.addDirtyRegion();
        } else {
            bodyLayer.bounds[Y] = 0;
        }

        if (tickerLayer.visible) {
            bodyLayer.bounds[H] -= tickerLayer.bounds[H];
            tickerLayer.bounds[Y] = btnLayer.bounds[Y] - tickerLayer.bounds[H];
            tickerLayer.addDirtyRegion();
        }

        if (ptiLayer != null && ptiLayer.isVisible()) {
            bodyLayer.bounds[H] -= ptiLayer.bounds[H];
            ptiLayer.bounds[Y] = bounds[H] - ptiLayer.bounds[H];
            ptiLayer.bounds[Y] -= (btnLayer.visible ?
                                   btnLayer.bounds[H] : 0) +
                (tickerLayer.visible ?
                 tickerLayer.bounds[H] : 0);
            ptiLayer.addDirtyRegion();
        }

        if (bodyLayer.bounds[W] != oldWidth ||
            bodyLayer.bounds[H] != oldHeight) {
            sizeChangedOccured = true;
            bodyLayer.addDirtyRegion();
        } else if ( bodyLayer.bounds[X] != oldX ||
                bodyLayer.bounds[Y] != oldY) {
            bodyLayer.addDirtyRegion();
        }
    }

    /**
     * Internal method to clear all current popups. This occurs if a
     * change of displayable occurs, as all popups are treated as belonging
     * to the current displayable.
     */
    protected void clearPopups() {
        synchronized (super.layers) {
            for (int i = super.layers.size() - 1; i >= 0; i--) {
                CLayer l = (CLayer)layers.elementAt(i);
                if (l instanceof PopupLayer) {
                    removeLayer(l);
                }
            }
        }
    }

    /**
     * Gets the "top" most Popup layer added to this body layer.
     * If there are no popups, this method returns null.
     *
     * @return the top most popup layer, or null if there are none.
     */
    public PopupLayer getTopMostPopup() {
        synchronized (super.layers) {
            for (int i = super.layers.size() - 1; i >= 0; i--) {
                if (super.layers.elementAt(i) instanceof PopupLayer) {
                    return (PopupLayer)super.layers.elementAt(i);
                }
            }
        }
        return null;
    }
}

