/*
 *  
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

import com.sun.midp.chameleon.layers.*;
import com.sun.midp.chameleon.skins.*;
import com.sun.midp.chameleon.skins.resources.*;
import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

import javax.microedition.lcdui.*;

/**
 * The MIDPWindow class is a concrete instance of a CWindow which
 * implements the MIDP specification and its graphical elements,
 * such as a title bar, soft buttons, ticker, etc.
 */
public class MIDPWindow extends CWindow {

    // The order of layers id is impotant during creation and updating

    /** Id of layer containing the alert wash */
    public static final int ALERT_WASH_LAYER = 0;

    /** Id of layer containing the alert displayable */
    public static final int ALERT_LAYER = 1;

    /** Id of layer containing the mail */
    public static final int WASH_LAYER = 2;
    
    /** Id of layer rendering the soft button controls */
    public static final int BTN_LAYER = 3;

    /** Id of layer containing the ticker of the current displayable */
    public static final int TICKER_LAYER = 4;

    /** Id of layer containing the title of the current displayable */
    public static final int TITLE_LAYER = 5;

    /** Id of layer containing the pti contents */
    public static final int PTI_LAYER = 6;

    /** Id of layer containing the current displayable's contents */
    public static final int BODY_LAYER = 7;

    /** Number of main layers*/
    public static final int LAST_LAYER = 8;

    /** Used to call back into the Display class from this package */
    ChamDisplayTunnel tunnel;

    // layout modes
    /**
     * Normal screen mode
     */
    private static final int NORMAL_MODE         = 0;

    /**
     * Full screen mode when the current displayable
     * is occupying as much screen as possible
     */
    private static final int FULL_SCR_MODE       = 1;

    /**
     * Current screen mode
     */
    int screenMode;

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

    CLayer[] mainLayers = new CLayer[LAST_LAYER];

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

        for (int i = LAST_LAYER - 1; i >= 0; i-- ) {
            createLayer(i);
        }
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
        if (((TitleLayer)mainLayers[TITLE_LAYER]).setTitle(title)) {
            resize();
        }
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
        if (((TickerLayer)mainLayers[TICKER_LAYER]).setText((ticker != null) ? ticker.getString() : null)) {
            resize();
        }
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
        mainLayers[BODY_LAYER].opaque =  (displayable instanceof Canvas);

        Ticker t = displayable.getTicker();
        ((TickerLayer)mainLayers[TICKER_LAYER]).setText((t != null) ? t.getString() : null);

        if (displayable instanceof Alert) {
            ((TickerLayer)mainLayers[TICKER_LAYER]).toggleAlert(true);
            ((SoftButtonLayer)mainLayers[BTN_LAYER]).toggleAlert(true);
            
            ((AlertLayer)mainLayers[ALERT_LAYER]).setAlert(true, (Alert)displayable, height);
            
            paintWash(false);
            addLayer(mainLayers[ALERT_LAYER]);
        } else {
            ((TitleLayer)mainLayers[TITLE_LAYER]).setTitle(displayable.getTitle());
	    mainLayers[BODY_LAYER].setVisible(true);
        }

        resize();
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
            ((SoftButtonLayer)mainLayers[BTN_LAYER]).toggleAlert(false);
            ((TickerLayer)mainLayers[TICKER_LAYER]).toggleAlert(false);
            
            ((AlertLayer)mainLayers[ALERT_LAYER]).setAlert(false, null, 0);
            paintWash(false);
            removeLayer(mainLayers[ALERT_LAYER]);
        } else {
            mainLayers[BODY_LAYER].setVisible(false);
        }
        
        ((SoftButtonLayer)mainLayers[BTN_LAYER]).dismissMenu();

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
        return ((SoftButtonLayer)mainLayers[BTN_LAYER]).systemMenuUp();
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
        if (mainLayers[ALERT_LAYER].visible) {
            mainLayers[ALERT_LAYER].addDirtyRegion(x, y, w, h);
        } else {
            mainLayers[BODY_LAYER].addDirtyRegion(x, y, w, h);
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
     * @return true if new layer was added, false otherwise
     */
    public boolean addLayer(CLayer layer) {
        boolean added = super.addLayer(layer);

        if (added && layer instanceof PopupLayer) {
            PopupLayer popup = (PopupLayer)layer;
            popup.dirty = true;
            popup.visible = true;

            Command[] cmds = popup.getCommands();
            if (cmds != null) {
                ((SoftButtonLayer)mainLayers[BTN_LAYER]).updateCommandSet(
                    null, 0, null, cmds, cmds.length,
                    popup.getCommandListener());
            }
        }

        if (added && layer instanceof PTILayer) {
            mainLayers[PTI_LAYER] = (PTILayer)layer;
            resize();
        }

        return added;
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
        if (super.removeLayer(layer)) {
            if (layer instanceof PopupLayer) {
                if (layer == mainLayers[PTI_LAYER]) {
                    mainLayers[PTI_LAYER] = null;
                    resize();
                }

                // Now we update the command set with either the
                // next top most popup or the original cached commands
                PopupLayer p = getTopMostPopup();
                if (p != null && p.getCommands() != null) {
                    Command[] cmds = p.getCommands();
                    ((SoftButtonLayer)mainLayers[BTN_LAYER]).updateCommandSet(
                        null, 0, null, cmds, cmds.length, p.getCommandListener());
                } else {
                    ((SoftButtonLayer)mainLayers[BTN_LAYER]).updateCommandSet(
                        itemCmdCache, itemCmdCount, itemCmdListener,
                        scrCmdCache, scrCmdCount, scrCmdListener);
                }
            } // instanceof
            return true;
        } // removeLayer
        return false;
    }

    /**
     * Return bounds of BodyLayer currently
     * @return array of bounds
     */
    public int[] getBodyLayerBounds() {
        int[] innerBounds = new int[4];
        System.arraycopy(((BodyLayer)mainLayers[BODY_LAYER]).bounds,0,innerBounds,0,4);
        return innerBounds;

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

        ((SoftButtonLayer)mainLayers[BTN_LAYER]).updateCommandSet(itemCommands, itemCmdCount,
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
        if (onOff) {
            setMode(FULL_SCR_MODE);
        } else {
            setMode(NORMAL_MODE);
        }
    }

    /**
     * Update the current layout
     */
    public void updateLayout() {
        resize();
        requestRepaint();
    }

    /**
     * Changes layout mode.
     *
     * @param mode the mode to be set
     */
    private void setMode(int mode) {
        screenMode = mode;
        updateLayout();
    }

    /**
     * Determines if window is in full screen mode.
     * 
     * @return true if in full screen mode
     */
    public boolean isInFullScreenMode() {
        return screenMode == FULL_SCR_MODE;
    }

    /**
     * Called to paint a wash over the background of this window.
     * Used by SoftButtonLayer when the system menu pops up, and
     * internally when an Alert is shown.
     *
     * @param onOff A flag indicating if the wash should be on or off
     */
    public void paintWash(boolean onOff) {
	if (mainLayers[ALERT_LAYER].visible) {
            addLayer(mainLayers[WASH_LAYER]);
            if (onOff) {
                addLayer(mainLayers[ALERT_WASH_LAYER]);
            } else {
                removeLayer(mainLayers[ALERT_WASH_LAYER]);

                // IMPL_NOTES: interface has to be fixed 
                ((BodyLayer)mainLayers[ALERT_LAYER]).setScrollInd(ScrollIndLayer.getInstance(ScrollIndSkin.MODE));
                
                // IMPL_NOTES: need to be removed as soon as removeLayer algorithm
                // takes into account layers interaction
                mainLayers[TICKER_LAYER].addDirtyRegion();
                mainLayers[ALERT_LAYER].addDirtyRegion();
            }
        } else {
            removeLayer(mainLayers[ALERT_WASH_LAYER]);
            if (onOff) {
                addLayer(mainLayers[WASH_LAYER]);
            } else {
                removeLayer(mainLayers[WASH_LAYER]);
                
                // IMPL_NOTES: interface has to be fixed 
                ((BodyLayer)mainLayers[BODY_LAYER]).setScrollInd(ScrollIndLayer.getInstance(ScrollIndSkin.MODE));
                
                // IMPL_NOTES: need to be removed as soon as removeLayer algorithm
                // takes into account layers interaction
                mainLayers[TICKER_LAYER].addDirtyRegion();
                mainLayers[TITLE_LAYER].addDirtyRegion();

                if (mainLayers[PTI_LAYER] != null) {
                    mainLayers[PTI_LAYER].addDirtyRegion();
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
        return ((SoftButtonLayer)mainLayers[BTN_LAYER]).getSoftOne();
    }

    /**
     * Returns the command array tied to the right soft button (two).
     *
     * @return the command array that's tied to the right soft button
     */
    public Command[] getSoftTwo() {
        return ((SoftButtonLayer)mainLayers[BTN_LAYER]).getSoftTwo();
    }

    /**
     * Returns true if the point lies in the bounds of commnad layer
     * @param x the "x" coordinate of the point
     * @param y the "y" coordinate of the point
     * @return true if the point lies in the bounds of commnad layer
     */
    public boolean belongToCmdLayers(int x, int y) {
        return ((SoftButtonLayer)mainLayers[BTN_LAYER]).belongToCmdLayers(x,y);
    }
    
    /**
     * Set the current vertical scroll position and proportion.
     *
     * @param scrollPosition vertical scroll position.
     * @param scrollProportion vertical scroll proportion.
     * @return true if set vertical scroll occues
     */
    public boolean setVerticalScroll(int scrollPosition, int scrollProportion) {
        CLayer scrollable = mainLayers[ALERT_LAYER].isVisible() ?
            mainLayers[ALERT_LAYER] : mainLayers[BODY_LAYER];
        
        return ((BodyLayer)scrollable).setVerticalScroll(scrollPosition, scrollProportion);
    }

    /**
     * Get the current x anchor coordinate for the body layer (the body
     * layer renders the contents of the current displayable).
     *
     * @return the x anchor coordinate of the body layer
     */
    public int getBodyAnchorX() {
        return mainLayers[BODY_LAYER].bounds[X];
    }

    /**
     * Get the current y anchor coordinate for the body layer (the body
     * layer renders the contents of the current displayable).
     *
     * @return the y anchor coordinate of the body layer
     */
    public int getBodyAnchorY() {
        return mainLayers[BODY_LAYER].bounds[Y];
    }

    /**
     * Get the current width of the body layer (the body
     * layer renders the contents of the current displayable).
     *
     * @return the width of the body layer
     */
    public int getBodyWidth() {
        return mainLayers[BODY_LAYER].bounds[W];
    }

    /**
     * Get the current height of the body layer (the body
     * layer renders the contents of the current displayable).
     *
     * @return the height of the body layer
     */
    public int getBodyHeight() {
        return mainLayers[BODY_LAYER].bounds[H];
    }

    /**
     * Utility method to determine if the given point lies within
     * the bounds of body layer. The point should be in the coordinate
     * space of this layer's containing CWindow.
     *
     * @param x the "x" coordinate of the point
     * @param y the "y" coordinate of the point
     * @return true if the coordinate lies in the bounds of this layer
     */
    public boolean bodyContainsPoint(int x, int y) {
        return mainLayers[BODY_LAYER].containsPoint(x, y);
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
        // Cache body layer instance for faster access
        CLayer bodyLayer = mainLayers[BODY_LAYER];

        // IMPL_NOTE: Only Canvas painting specially doesn't change
        // dirty state of the owner window, however it is not enough
        // to bypass the Chameleon paint engine. Body layer holding
        // the Canvas should be opaque and be not overlapped with
        // any visible higher layer also. The check for overlapping
        // is to be added later.
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
        if (bodyLayer.dirty) {
            if (bodyLayer.dirtyBounds[X] == -1) {
                g.setClip(bodyLayer.bounds[X], bodyLayer.bounds[Y],
                          bodyLayer.bounds[W], bodyLayer.bounds[H]);
                g.translate(bodyLayer.bounds[X], bodyLayer.bounds[Y]);

            } else {
                g.translate(bodyLayer.bounds[X], bodyLayer.bounds[Y]);
                g.setClip(bodyLayer.dirtyBounds[X], bodyLayer.dirtyBounds[Y],
                          bodyLayer.dirtyBounds[W], bodyLayer.dirtyBounds[H]);
            }
            bodyLayer.cleanDirty();
        } else {
            // NOTE: the layer can be not dirty, e.g. in the case an empty
            // area was requested for repaint, set empty clip area then.
            g.translate(bodyLayer.bounds[X], bodyLayer.bounds[Y]);
            g.setClip(0, 0, 0, 0);
        }

        return true;
    }

    /**
     * Internal method to resize window and its content layers
     * according to a size changes in the loaded skins.
     * This is important to re-calculate whenever things such as
     * titles, tickers, fullscreen mode, etc. change state.
     */
    public void resize() {
        super.resize();

        int oldHeight = mainLayers[BODY_LAYER].bounds[H];
        int oldWidth = mainLayers[BODY_LAYER].bounds[W];

        switch (screenMode) {
            case FULL_SCR_MODE:
                // TODO: scroll arrows (bar? ) indicator has to be hidden?
                mainLayers[TITLE_LAYER].visible = false;
                mainLayers[TICKER_LAYER].visible = false;
                mainLayers[BTN_LAYER].visible = 
                    ((SoftButtonLayer)mainLayers[BTN_LAYER]).isInteractive();
                break;
            case NORMAL_MODE:
                mainLayers[TITLE_LAYER].visible = 
                    (((TitleLayer)mainLayers[TITLE_LAYER]).getTitle() != null);
                mainLayers[TICKER_LAYER].visible = 
                    (((TickerLayer)mainLayers[TICKER_LAYER]).getText() != null);
                mainLayers[BTN_LAYER].visible = true;
                break;
            default:
                Logging.report(Logging.ERROR, LogChannels.LC_HIGHUI,
                    "MIDPWindow: screenMode=" + screenMode);
                return;
        }

        for (int i = 0; i < LAST_LAYER; i++) {
            if (mainLayers[i] != null) {
                mainLayers[i].update(mainLayers);
            }
        }

        if (mainLayers[BODY_LAYER].bounds[W] != oldWidth ||
            mainLayers[BODY_LAYER].bounds[H] != oldHeight) {
            setDirty();
            sizeChangedOccured = true;
        }
    }
    
    /**
     * Internal method to clear all current popups. This occurs if a
     * change of displayable occurs, as all popups are treated as belonging
     * to the current displayable.
     */
    protected void clearPopups() {
        synchronized (super.layers) {
            for (CLayerElement le = super.layers.getTop();
                    le != null; le = le.getLower()) {
                CLayer l = le.getLayer();
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
            for (CLayerElement le = super.layers.getTop();
                    le != null; le = le.getLower()) {
                CLayer l = le.getLayer();
                if (l instanceof PopupLayer) {
                    return (PopupLayer)l;
                }
            }
        }
        return null;
    }

    /**
     * create new layer by id and launch addLayer()
     * @param id - layer id
     */
    private void createLayer(int id) {
        switch (id) {
            case PTI_LAYER:
                break;
            case TITLE_LAYER:
                mainLayers[id] = new TitleLayer();
                addLayer(mainLayers[id]);
                break;
            case TICKER_LAYER:
                mainLayers[id] = new TickerLayer();
                addLayer(mainLayers[id]);
                break;
            case BTN_LAYER:
                mainLayers[id] = new SoftButtonLayer(tunnel);
                addLayer(mainLayers[id]);
                break;
            case ALERT_LAYER:
                mainLayers[id] = new AlertLayer(tunnel);
                break;
            case WASH_LAYER:
            case ALERT_WASH_LAYER:
                mainLayers[id] = new WashLayer();
                break;
            case BODY_LAYER:
                mainLayers[id] = new BodyLayer(tunnel);
                addLayer(mainLayers[id]);
                break;
        }
    }
}

