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

package com.sun.midp.chameleon;

import javax.microedition.lcdui.*;

/**
 * This class is used to replace the window in a closed display so the
 * Display does not have to check the window for null before each use.
 */
public class MidpNullWindow extends MIDPWindow {
    /** The null window. */
    private static MidpNullWindow window;

    /**
     * Returns a MIDP null window.
     *
     * @return MIDP null window
     */
    public static MidpNullWindow getWindow() {
        if (window == null) {
            window = new MidpNullWindow();
        }

        return window;
    }

    /**
     * Construct a dummy MIDPWindow without a tunnel.
     */
    private MidpNullWindow() {
        super();
    }

    /**
     * Request a repaint. This method does not require any bounds
     * information as it is contained in each of the Chameleon layers.
     * This method simply results in a repaint event being placed in
     * the event queue for a future callback.
     */
    public void requestRepaint() {
    }
    
    /**
     * Request a hide notify. This method does not require any bounds
     * information as it is contained in each of the Chameleon layers.
     */
    public void requestHideNotify() {
    }
    
    /**
     * Request a show notify. This method does not require any bounds
     * information as it is contained in each of the Chameleon layers.
     */
    public void requestShowNotify() {
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
    }

    /**
     * Determines if the system menu is currently visible. This can be useful
     * in determining the current isShown() status of the displayable.
     *
     * @return true if the system menu is up
     */
    public boolean systemMenuUp() {
        return false;
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
    }

    /**
     * Return bounds of BodyLayer currently
     * @return array of bounds
     */
    public int[] getBodyLayerBounds() {
        int[] innerBounds = new int[4];

        innerBounds[0] = 0;
        innerBounds[1] = 0;
        innerBounds[2] = 0;
        innerBounds[3] = 0;

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
                                 CommandListener scrCmdListener) {
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
    }

    /**
     * Update the current layout
     */
    public void updateLayout() {
    }

    /**
     * Changes layout mode.
     *
     * @param mode the mode to be set
     */
    public void setMode(int mode) {
    }

    /**
     * Determines if window is in full screen mode.
     * 
     * @return true if in full screen mode
     */
    public boolean isInFullScreenMode() {
        return false;
    }

    /**
     * Called to paint a wash over the background of this window.
     * Used by SoftButtonLayer when the system menu pops up, and
     * internally when an Alert is shown.
     *
     * @param onOff A flag indicating if the wash should be on or off
     */
    public void paintWash(boolean onOff) {
    }

    /**
     * Returns the left soft button (one).
     *
     * @return the command that's tied to the left soft button
     */
    public Command getSoftOne() {
        return null;
    }

    /**
     * Returns the command array tied to the right soft button (two).
     *
     * @return the command array that's tied to the right soft button
     */
    public Command[] getSoftTwo() {
        return null;
    }

    /**
     * Called by soft button layer when interactive state of it
     * has been changed
     *
     * @param interactive if soft buttons are currently interactive.
     */
    public void onSoftButtonInteractive(boolean interactive) {
    }

    /**
     * Returns true if the point lies in the bounds of commnad layer
     * @param x the "x" coordinate of the point
     * @param y the "y" coordinate of the point
     * @return true if the point lies in the bounds of commnad layer
     */
    public boolean belongToCmdLayers(int x, int y) {
        return false;
    }
    
    /**
     * Set the current vertical scroll position and proportion.
     *
     * @param scrollPosition vertical scroll position.
     * @param scrollProportion vertical scroll proportion.
     * @return true if set vertical scroll occues
     */
    public boolean setVerticalScroll(int scrollPosition,
                                     int scrollProportion) {
        return false;
    }

    /**
     * Get the current x anchor coordinate for the body layer (the body
     * layer renders the contents of the current displayable).
     *
     * @return the x anchor coordinate of the body layer
     */
    public int getBodyAnchorX() {
        return 0;
    }

    /**
     * Get the current y anchor coordinate for the body layer (the body
     * layer renders the contents of the current displayable).
     *
     * @return the y anchor coordinate of the body layer
     */
    public int getBodyAnchorY() {
        return 0;
    }

    /**
     * Get the current width of the body layer (the body
     * layer renders the contents of the current displayable).
     *
     * @return the width of the body layer
     */
    public int getBodyWidth() {
        return 0;
    }

    /**
     * Get the current height of the body layer (the body
     * layer renders the contents of the current displayable).
     *
     * @return the height of the body layer
     */
    public int getBodyHeight() {
        return 0;
    }

    /**
     * Get the current width of the alert layer (the body
     * layer renders the contents of the current displayable).
     *
     * @return the width of the alert layer
     */
    public int getAlertWidth() {
        return 0;
    }

    /**
     * Get the current height of the alert layer (the body
     * layer renders the contents of the current displayable).
     *
     * @return the height of the alert layer
     */
    public int getAlertHeight() {
        return 0;
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
        return false;
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
        return false;
    }

    /**
     * Internal method to resize window and its content layers
     * according to a size changes in the loaded skins.
     * This is important to re-calculate whenever things such as
     * titles, tickers, fullscreen mode, etc. change state.
     */
    public void resize() {
    }

    /** Resize window and its background according to updated skin values */
    public void resize(int width, int height) {
    }

    /**
     * Add a new CLayer to the "deck" of layers associated
     * with this CWindow. This method will sequentially add
     * layers to the window, placing subsequently added layers
     * on top of previously added layers.
     *
     * @param layer the new layer to add to this window
     * @return true if new layer was added, false otherwise 
     */
    public boolean addLayer(CLayer layer) {
        return false;
    }

    /**
     * Remove a layer from this CWindow. This method will remove
     * the given layer from the "deck" of layers associated with
     * this CWindow. If successfull, this method will return true,
     * false otherwise (for example, if the layer does not belong
     * to this window).
     *
     * @param layer the layer to remove from this window
     * @return true if successful, false otherwise
     */
    public boolean removeLayer(CLayer layer) {
        return false;
    }

    /**
     * Move layer to anotger location
     * @param newBounds new bounds for this layer 
     * @param x New 'x' coordinate of the layer's origin
     * @param y New 'y' coordinate of the layer's origin
     * @param w New width of the layer
     * @param h New height of the layer

     * @return true if successful, false otherwise
     */
    public boolean relocateLayer(CLayer layer, int x, int y, int w, int h) {
        return false;
    }

    /**
     * Allow this window to process key input. The type of key input
     * will be press, release, repeat, etc. The key code will identify
     * which key generated the event. This method will return true if
     * the event was processed by this window or one of its layers,
     * false otherwise.
     *
     * @param type the type of key event (press, release, repeat)
     * @param keyCode the identifier of the key which generated the event
     * @return true if this window or one of its layers processed the event
     */
    public boolean keyInput(int type, int keyCode) {
        return false;
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
        return false;
    }

    /**
     * Handle input from some type of device-dependent
     * input method. This could be input from something
     * such as T9, or a phonebook lookup, etc.
     *
     * @param str the text to handle as direct input
     * @return true if this window or one of its layers processed the event
     */
    public boolean methodInput(String str) {
        return false;
    }

    /**
     * Check whether layer is overlapped with a higher visible layer
     * in the layer stack of the window
     *
     * @param l layer to check overlapping
     * @return true if overlapped, false otherwise
     */
    public boolean isOverlapped(CLayer l) {
        return false;
    }

    /**
     * Update dirty regions of all visible layers in the stack regarding
     * the entire area of the given layer as being dirty. The method is
     * needed to perform layer move/resize/remove opertion, since other
     * layers should be informed of changed area.
     *
     * @param layer the layer whose area should be reported as dirty to
     *   other stack layers
     * @return element of the window layers list that contains swept
     *   layer, it can be used for further layer processing
     */
    CLayerElement sweepLayer(CLayer layer) {
        return null;
    }

    /**
     * Sets all visible layers to dirty state.
     * The method is needed on system events like screen rotation,
     * when generic layers system is not capabel to properly analyze
     * layers changes, e.g. of move/resize kind. It could be fixed in
     * the future and this method will be out of use. 
     */
    public void setAllDirty() {
    }

    /**
     * Paint this window. This method should not generally be overridden by
     * subclasses. This method carefully stores the clip, translation, and
     * color before calling into subclasses. The graphics context should be
     * translated such that it is in this window's coordinate space (0,0 is
     * the top left corner of this window). 
     *
     * @param g The graphics object to use to paint this window.
     * @param refreshQ The custom queue which holds the set of refresh
     *        regions needing to be blitted to the screen
     */
    public void paint(Graphics g, CGraphicsQ refreshQ) {
    }

    /**
     * Establish a background. This method will evaluate the parameters
     * and create a background which is appropriate. If the image is non-null,
     * the image will be used to create the background. If the image is null,
     * the values for the colors will be used and the background will be
     * painted in fill color instead. If the image is null, and the background
     * color is a negative value, this layer will become transparent and no
     * background will be painted.
     *
     * @param bgImage the image to use for the background tile (or null)
     * @param bgColor if the image is null, use this color as a background
     *                fill color
     */
    void setBackground(Image bgImage, int bgColor) {
    }
    
    /**
     * Returns true if any layer of this window is in need of repainting.
     *
     * @return true if any layer of this window is marked as 'dirty' 
     *         and needs repainting.
     */
    public boolean isDirty() {
        return false;
    }
    
    /**
     * Mark this window as being dirty and requiring a repaint.
     */    
    public void setDirty() {
    }
}
