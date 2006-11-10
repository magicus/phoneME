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

/* import  javax.microedition.lcdui.KeyConverter; */

import com.sun.midp.lcdui.EventConstants;
import com.sun.midp.lcdui.Text;
import com.sun.midp.lcdui.*;
import com.sun.midp.configurator.Constants;
import com.sun.midp.chameleon.skins.ChoiceGroupSkin;
import com.sun.midp.chameleon.skins.resources.ChoiceGroupResources;
import com.sun.midp.chameleon.skins.ScreenSkin;
import com.sun.midp.chameleon.layers.PopupLayer;
import com.sun.midp.chameleon.CGraphicsUtil;


/**
 * This is the Look &amps; Feel implementation for ChoiceGroupPopup.
 */
class ChoiceGroupPopupLFImpl extends ChoiceGroupLFImpl {

    /**
     * Creates ChoiceGroupPopupLF for the passed in choiceGroup of
     * Choice.POPUP type.
     * @param choiceGroup the ChoiceGroup object associated with this view
     */
    ChoiceGroupPopupLFImpl(ChoiceGroup choiceGroup) {
        super(choiceGroup);
        
        ChoiceGroupResources.load();
        
        viewable = new int[4];
        popupLayer = new CGPopupLayer(this);
    }

    /**
     * Sets the content size in the passed in array.
     * Content is calculated based on the availableWidth.
     * size[WIDTH] and size[HEIGHT] should be set by this method.
     * @param size The array that holds Item content size and location 
     *             in Item internal bounds coordinate system.
     * @param width The width available for this Item
     */
    public void lGetContentSize(int size[], int width) {

        // no label and empty popup => nothing is drawn
        // no elements => only label is drawn
        if (cg.numOfEls == 0) {
            size[WIDTH] = size[HEIGHT] = 0;
            return;
        }

        int w = getAvailableContentWidth(Choice.POPUP, width);
        int maxContentWidth = getMaxElementWidth(w);
        if (ChoiceGroupSkin.IMAGE_BUTTON_ICON != null) {
            maxContentWidth += ChoiceGroupSkin.IMAGE_BUTTON_ICON.getWidth();
        } else {
            maxContentWidth += 11;
        }

        viewable[HEIGHT] = calculateHeight(w);

        int s = (selectedIndex < 0) ? 0 : selectedIndex;
        size[HEIGHT] = cg.cgElements[s].getFont().getHeight() + 
            (2 * ChoiceGroupSkin.PAD_V);

        if (maxContentWidth < w) {
            // note that (width - w) = extra padding used
            size[WIDTH] = maxContentWidth;
        } else {
            size[WIDTH] = width;
        }
        viewable[WIDTH] = size[WIDTH];        
    }

    // *****************************************************
    //  Package private methods
    // *****************************************************

    /**
     * Paints the content area of the ChoiceGroup POPUP. 
     * Graphics is translated to contents origin.
     * @param g The graphics where Item content should be painted
     * @param width The width available for the Item's content
     * @param height The height available for the Item's content
     */
    void lPaintContent(Graphics g, int width, int height) {
        // paint closed state of the popup

        // if there are no elements, we are done
        if (cg.numOfEls == 0) {
            return;
        }
        
        // draw background
        if (ChoiceGroupSkin.IMAGE_BG != null) {
            CGraphicsUtil.draw9pcsBackground(g, 0, 0, width, height,
                ChoiceGroupSkin.IMAGE_BG);
        } else {
            // draw widget instead of using images
            CGraphicsUtil.drawDropShadowBox(g, 0, 0, width, height,
                ChoiceGroupSkin.COLOR_BORDER,
                ChoiceGroupSkin.COLOR_BORDER_SHD, 
                ChoiceGroupSkin.COLOR_BG);
        }
        
        if (hasFocus && !popUpOpen) {
            // hilight the background
            g.setColor(ScreenSkin.COLOR_TRAVERSE_IND);
            g.fillRect(2, 2, width - 3, height - 3);
        } 
                    
        // draw icon
        if (ChoiceGroupSkin.IMAGE_BUTTON_ICON != null) {
            int w = ChoiceGroupSkin.IMAGE_BUTTON_ICON.getWidth();
            int yOffset = height - 
                ChoiceGroupSkin.IMAGE_BUTTON_ICON.getHeight();
            if (yOffset > 0) {
                yOffset = (int)(yOffset / 2);
            } else {
                yOffset = 0;
            }
            width -= (w + 1);
            if (ChoiceGroupSkin.IMAGE_BUTTON_BG != null) {
                CGraphicsUtil.draw9pcsBackground(
                    g, width, 1, w, height - 2,
                    ChoiceGroupSkin.IMAGE_BUTTON_BG);
            }
            g.drawImage(ChoiceGroupSkin.IMAGE_BUTTON_ICON,
                        width, yOffset + 1, 
                        Graphics.LEFT | Graphics.TOP);
            width -= ChoiceGroupSkin.PAD_H;
        }
        
        g.translate(ChoiceGroupSkin.PAD_H, ChoiceGroupSkin.PAD_V);        
            
        int s = selectedIndex < 0 ? 0 : selectedIndex;
        
        // paint value

        int textOffset = 0;
        
        if (cg.cgElements[s].imageEl != null) {
            int iX = g.getClipX();
            int iY = g.getClipY();
            int iW = g.getClipWidth();
            int iH = g.getClipHeight();
           
            g.clipRect(0, 0,
                       ChoiceGroupSkin.WIDTH_IMAGE, 
                       ChoiceGroupSkin.HEIGHT_IMAGE);
            g.drawImage(cg.cgElements[s].imageEl,
                        0, 0,
                        Graphics.LEFT | Graphics.TOP);
            g.setClip(iX, iY, iW, iH); 
            textOffset = ChoiceGroupSkin.WIDTH_IMAGE + 
                ChoiceGroupSkin.PAD_H;
        }
                    
        Text.paint(g, cg.cgElements[s].stringEl, 
                   cg.cgElements[s].getFont(), 
                   (hasFocus) ? ScreenSkin.COLOR_FG_HL :    
                        ChoiceGroupSkin.COLOR_FG, 
                   0, width, 
                   cg.cgElements[s].getFont().getHeight(), 
                   textOffset, Text.TRUNCATE, null);

        g.translate(-ChoiceGroupSkin.PAD_H, -ChoiceGroupSkin.PAD_V);
    }

    /**
     * Called by the system to indicate traversal has left this Item
     * This function simply calls lCallTraverseOut() after obtaining LCDUILock.
     */
    void uCallTraverseOut() {
        super.uCallTraverseOut();
        Form form = null;
        
        synchronized (Display.LCDUILock) {
            if (popUpOpen) {
                popUpOpen = false;
                hilightedIndex = 0;
                getCurrentDisplay().hidePopup(popupLayer);
            }
        }
    }

    /**
     * Handle traversal within this ChoiceGroup
     *
     * @param dir the direction of traversal
     * @param viewportWidth the width of the viewport
     * @param viewportHeight the height of the viewport
     * @param visRect the in/out rectangle for the internal traversal location
     * @return true if traversal occurred within this ChoiceGroup
     */
    boolean lCallTraverse(int dir, int viewportWidth, int viewportHeight,
                         int[] visRect) {

        super.lCallTraverse(dir, viewportWidth, viewportHeight, visRect);

        // If we have no elements, or if the user pressed left/right,
        // don't bother with the visRect and just return false
        if (cg.numOfEls == 0) {
            return false;
        }

        // If we are a closed popup, don't bother with the visRect
        // and return true on the initial traverse, false on subsequent
        // traverses
        if (!popUpOpen) {
            if (!traversedIn) {
                traversedIn = true;
                return true;
            } else {
                return false;
            }
        }

        return true;
    }

    /**
     * Handle traversal in the open popup
     * @param dir - the direction of traversal (Canvas.UP, Canvas.DOWN)
     * @param viewportWidth - the width of the viewport
     * @param viewportHeight - the height of the viewport
     * @return true if traverse event was handled, false - otherwise
     */
    boolean traverseInPopup(int dir, int viewportWidth, int viewportHeight) {

        if (!popUpOpen) {
            return false;
        }

        if (cg.numOfEls == 1) {
            return true;
        }

        if (dir == Canvas.UP) {
            if (hilightedIndex > 0) {
                int hilightY = 0;
                for (int i = 0; i < hilightedIndex; i++) {
                    hilightY += elHeights[i];
                }
                hilightedIndex--;
                hilightY -= elHeights[hilightedIndex];
                if (hilightY < viewable[Y]) {
                    viewable[Y] -= (viewable[Y] - hilightY);
                }
            } else {
                // jump to the last element
                hilightedIndex = cg.numOfEls - 1;
                viewable[Y] = viewable[HEIGHT] - viewportHeight;
            }
            lRequestPaint();

        } else if (dir == Canvas.DOWN) {
            if (hilightedIndex < (cg.numOfEls - 1)) {
                int hilightY = 0;
                for (int i = 0; i < hilightedIndex; i++) {
                    hilightY += elHeights[i];
                }
                hilightY += elHeights[hilightedIndex];
                hilightedIndex++;
                if (hilightY + elHeights[hilightedIndex]>
                    viewable[Y] + viewportHeight) {
                    viewable[Y] += hilightY + elHeights[hilightedIndex] -
                        (viewable[Y] + viewportHeight);
                }
            } else {
                // jump to the first element
                hilightedIndex = 0;
                viewable[Y] = 0;
            }
            lRequestPaint();
        }

        return true;
    }

    /**
     * Handle a key press event
     *
     * @param keyCode the key which was pressed
     */
    void uCallKeyPressed(int keyCode) {

        Form form = null;

        synchronized (Display.LCDUILock) {

            if (cg.numOfEls == 0) {
                return;
            }
            
            if (!popUpOpen) {
                if (keyCode != Constants.KEYCODE_SELECT) {
                    return;
                }
                // show popup
                
                ScreenLFImpl sLF = (ScreenLFImpl)cg.owner.getLF();
                int x = getInnerBounds(X) - sLF.viewable[X] + contentBounds[X];
                int y = getInnerBounds(Y) - sLF.viewable[Y] + contentBounds[Y];
                hilightedIndex = selectedIndex > 0 ? selectedIndex : 0;
                
                popupLayer.show(x, y, 
                                contentBounds[WIDTH], contentBounds[HEIGHT],
                                viewable[WIDTH], viewable[HEIGHT],
                                y, 
                                sLF.viewport[HEIGHT] - 
                                y - contentBounds[HEIGHT]);
                
                popUpOpen = !popUpOpen;
                
            } else {
                // popup is closed when SELECT, LEFT or RIGHT is pressed;
                // popup selection is changed only when SELECT is pressed
                if (keyCode != Constants.KEYCODE_SELECT &&
                    keyCode != Constants.KEYCODE_LEFT && 
                    keyCode != Constants.KEYCODE_RIGHT) {
                    return;
                }

                popUpOpen = !popUpOpen;

                // IMPL_NOTE Check if we need notification if selected element 
                // did not change
                if (keyCode == Constants.KEYCODE_SELECT) {
                    if (selectedIndex >= 0) {
                        lSetSelectedIndex(hilightedIndex, true);
                        form = (Form)cg.owner; // To be called outside the lock
                    }
                }
                hilightedIndex = 0;
                getCurrentDisplay().hidePopup(popupLayer);
            }
            lRequestPaint();
        } // synchronized
        
        // Notify itemStateListener if necessary
        if (form != null) {
            form.uCallItemStateChanged(cg);
        }
    }

    // *****************************************************
    //  Private methods
    // *****************************************************

    /** The state of the popup ChoiceGroup (false by default) */
    private boolean popUpOpen; // = false;

    /** The PopupLayer that represents open state of this ChoiceGroup POPUP. */
    CGPopupLayer popupLayer;

    /** 
     * Content of the popupLayer is drawn in ChoiceGroupPopupLFImpl.
     * That content can be taller then the layer itself.
     * viewable holds information of the current scroll position
     * and the size of the content (X, Y, WIDTH, HEIGHT)
     */
    int viewable[];

    // *****************************************************
    //  Inner class
    // *****************************************************
    /**
     * The following is the implementation of the ChoiceGroup POPUP
     * open state. The popup is shown and hidden on a KEYCODE_SELECT.
     * It is placed above or below the ChoiceGroup POPUP button (closed
     * state) depending on the space available. If there are too
     * many elements to display in the popup 
     * scrollbar will be added on the right.
     * If possible popup should be displayed below the button.
     * If possible the entire content of the popup should be seen.
     */
    class CGPopupLayer extends PopupLayer {
        
        /**
         * CGPopupLayer constructor. Sets ChoiceGroupPopupLFImpl that
         * is associated with this CGPopupLayer.
         * @param lf - The ChoiceGroupPopupLFImpl associated with this 
         *             CGPopupLayer
         */
        CGPopupLayer(ChoiceGroupPopupLFImpl lf) {
            super(ChoiceGroupSkin.IMAGE_POPUP_BG, 
                  ChoiceGroupSkin.COLOR_BG);
            this.lf = lf;
        }
                
        /**
         * Initializes internal structures of CGPopupLayer.
         */
        protected void initialize() {
            super.initialize();
            viewport =  new int[4];
        }
        
        /**
         * Handles key event in the open popup
         * @param type - The type of this key event (pressed, released)
         * @param code - The code of this key event
         * @return true if the key event was handled and false - otherwise
         */
        public boolean keyInput(int type, int code) {
            if (type == EventConstants.PRESSED && lf != null) {

                if (code == Constants.KEYCODE_UP
                    || code == Constants.KEYCODE_DOWN) 
                {
                    if (lf.traverseInPopup(KeyConverter.getGameAction(code),
                                           viewport[WIDTH], 
                                           viewport[HEIGHT])) 
                                           {
                                               dirty = true;
                                               return true;
                                           }
                }

                lf.uCallKeyPressed(code);
            }
            // PopupLayers always swallow all key events
            return true;
        }

        /**
         * Paints popup background (including borders) and scrollbar
         * if it is present
         * @param g - The graphics object to paint background on
         */
        protected void paintBackground(Graphics g) {
            super.paintBackground(g);
           
            // draw border if there is no background image
            if (bgImage == null) {
                g.setColor(ChoiceGroupSkin.COLOR_BORDER);
                g.drawRect(0, 0, bounds[W]-1, bounds[HEIGHT]-1);
              
                g.setColor(ChoiceGroupSkin.COLOR_BORDER_SHD);
                g.drawLine(1, 1, 1, bounds[HEIGHT]-2); 
           }

            if (sbVisible) {
                int sbX = bounds[WIDTH] - 
                    (ChoiceGroupSkin.WIDTH_SCROLL / 2) - 1;
                int sbY = ChoiceGroupSkin.PAD_V;
                int sbH = bounds[HEIGHT] - (2 * ChoiceGroupSkin.PAD_V);
                int thumbY = sbY + 4 + 
                    ((lf.viewable[Y] * 
                      (sbH - 8 - ChoiceGroupSkin.HEIGHT_THUMB)) /
                     (lf.viewable[HEIGHT] - viewport[HEIGHT]));
                
                if (bgImage == null) {                    
                    // draw scrollbar with arrrows
                    g.setColor(ChoiceGroupSkin.COLOR_SCROLL);

                    int sbY2 = sbY + sbH - 1;
                    g.drawLine(sbX, sbY, sbX, sbY2);

                    g.drawLine(sbX - 2, sbY + 2, sbX - 1, sbY + 1);
                    g.drawLine(sbX + 1, sbY + 1, sbX + 2, sbY + 2);
                    g.drawLine(sbX - 2, sbY2 - 2, sbX - 1, sbY2 - 1);
                    g.drawLine(sbX + 1, sbY2 - 1, sbX + 2, sbY2 - 2);
                }

                // draw scrollbar thumb
                g.setColor(ChoiceGroupSkin.COLOR_THUMB);
                g.fillRect(sbX - (ChoiceGroupSkin.WIDTH_THUMB / 2), 
                           thumbY,
                           ChoiceGroupSkin.WIDTH_THUMB,
                           ChoiceGroupSkin.HEIGHT_THUMB);
            }
        }

        /**
         * Paints the content area of ChoiceGroup popup
         * @param g - The Graphics object to paint content on
         */
        protected void paintBody(Graphics g) {

            g.clipRect(viewport[X], viewport[Y], 
                       viewport[WIDTH], viewport[HEIGHT]);

            g.translate(viewport[X] - lf.viewable[X],
                        viewport[Y] - lf.viewable[Y]);

            
            lf.lPaintElements(g, bounds[WIDTH], viewable[HEIGHT]);
            
            g.translate(-viewport[X] + lf.viewable[X], 
                        -viewport[Y] + lf.viewable[Y]);
        }

        /**
         * Shows popup for the ChoiceGroup POPUP button that
         * is drawn at the passed in location. 
         * It will determine if popup will be drawn above or
         * below the ChoiceGroup POPUP button depending on the
         * passed in info.
         * @param buttonX - the x location of ChoiceGroup POPUP button
         *                  in BodyLayer's coordinate system.
         * @param buttonY - the y location of ChoiceGroup POPUP button
         *                  in BodyLayer's coordinate system.
         * @param buttonW - the width of ChoiceGroup POPUP button
         * @param buttonH - the height of ChoiceGroup POPUP button
         * @param elementsWidth - the width of the widest element in 
         *                        the popup
         * @param elementsHeight - the height of all elements if they are
         *                         drawn vertically one after another
         * @param top - the amount of space available for popup above the
         *              ChoiceGroup POPUP button
         * @param bottom - the amount of space available for popup below the
         *                 ChoiceGroup POPUP button
         */
        void show(int buttonX, int buttonY,
                  int buttonW, int buttonH,
                  int elementsWidth, int elementsHeight,
                  int top, int bottom) {
            
            // popup with all elements displayed fits under the popup button
            if (elementsHeight + 1 <= bottom - ChoiceGroupSkin.PAD_V) {
                setBounds(buttonX, 
                          buttonY + buttonH - 1, // hide top border
                          buttonW,
                          elementsHeight + 2); // border width
                popupDrawnDown = true;
                sbVisible = false;

            // popup with all elements displayed fits above the popup button
            } else if (elementsHeight + 1 <= top - ChoiceGroupSkin.PAD_V) {
                setBounds(buttonX, 
                          buttonY - elementsHeight - 1, // show top border
                          buttonW, 
                          elementsHeight + 2); // border width
                popupDrawnDown = false;
                sbVisible = false;

            } else if (bottom > top) { // there is more space at the bottom
                setBounds(buttonX, 
                          buttonY + buttonH - 1, // hide top border width
                          buttonW, 
                          bottom - ChoiceGroupSkin.PAD_V);
                popupDrawnDown = true;
                sbVisible = true;

            } else { // there is more space at the top 
                setBounds(buttonX, 
                          buttonY - elementsHeight - 1, // show top border
                          buttonW, 
                          top - ChoiceGroupSkin.PAD_V);
                popupDrawnDown = false;
                sbVisible = true;
            }
            
            // set viewport in popup's coordinate system
            viewport[X] = 2; // border width
            viewport[Y] = 1; // border width
            viewport[WIDTH]  = viewable[WIDTH];
            viewport[HEIGHT] = bounds[HEIGHT] - 2; // border width

            // ASSERT: since we are receiving key events, 
            //         currentDisplay cannot be null.
            lf.getCurrentDisplay().showPopup(popupLayer);
        }

        /** The ChoiceGroupPopupLFImpl associated with this popup */
        ChoiceGroupPopupLFImpl lf; // = null;

        /**
         * The viewport setting inside this popup (X, Y, WIDTH, HEIGHT).
         * It is set in layer's coordinate system.
         */
        private int viewport[]; // = null;

        /** True if popup is drawn below Popup button, false - otherwise */
        boolean popupDrawnDown; // = false;

        /** True if sb is present in the Popup layer, false - otherwise */
        private boolean sbVisible; // = false;
    }
}

