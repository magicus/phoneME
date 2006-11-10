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

import com.sun.midp.lcdui.EventConstants;

import com.sun.midp.lcdui.*;
import com.sun.midp.configurator.Constants;
import com.sun.midp.chameleon.skins.DateEditorSkin;
import com.sun.midp.chameleon.skins.ChoiceGroupSkin;
import com.sun.midp.chameleon.layers.PopupLayer;

/**
 * This is a popup layer that handles a sub-popup within the date editor,
 * which is also a popup layer.
 */
class DEPopupLayer extends PopupLayer {
    /**
     * Constructs a date editor sub-popup layer, which behaves like a
     * popup-choicegroup, given a string array of elements that constitute
     * the available list of choices to select from.
     *
     * @param editor The DateEditor that triggered this popup layer.
     * @param elements String array holding the list of choices.
     * @param selectedIndex the index to place the initial highlight on.
     * @param circularTraversal true if traversal past the last item should
     *                          jump to the beginning 
    */
    DEPopupLayer(DateEditor editor, String[] elements, int selectedIndex,
                 boolean circularTraversal) {
        super((Image)null, DateEditorSkin.COLOR_POPUPS_BG);
        this.editor = editor;
        
        setContent(elements, selectedIndex);
        this.circularTraversal = circularTraversal;
    }        

    /**
     * Populates this sub-popup layer with new elements.
     * The number of elements before and after should be the same
     * if the popup already existed.
     *
     * @param newElements String array holding the list of choices.
     * @param selectedIndex the index to place the initial highlight on.
     */
    protected void setContent(String[] newElements, int selectedIndex) {
        if (newElements != null) {
            numElements = newElements.length;
            elements = new String[numElements];
            for (int i = 0; i < numElements; i++) {
                elements[i] = newElements[i];
            }
            hilightedIndex = selectedIndex;
        }
        startIndex = 0;
    }

    /**
     * Initializes the popup layer.
     */
    protected void initialize() {
        super.initialize();
        viewport = new int[4];
    }        
    
    /**
     * Sets the bounds of the popup layer.
     *
     * @param x the x-coordinate of the popup layer location
     * @param y the y-coordinate of the popup layer location
     * @param w the width of this popup layer in open state
     * @param h the height of this popup layer in open state
     */
    public void setBounds(int x, int y, int w, int h) {
        super.setBounds(x, y, w, h);
        transparent = false;
        
            // set viewport in popup's coordinate system
        viewport[X] = 2;
        viewport[Y] = 0;
        viewport[W] = bounds[W] - 3;
        viewport[H] = bounds[H] - 3;

        elementsToFit = viewport[H] / elementHeight;
        if (elementsToFit < numElements) {
            sbVisible = true;
        } else {
            elementsToFit = numElements;
            sbVisible = false;
        }
    }

    /**
     * Handles key event in the open popup.
     *
     * @param type - The type of this key event (pressed, released)
     * @param code - The code of this key event
     * @return true always, since popupLayers swallow all key events
     */
    public boolean keyInput(int type, int code) {
        if ((type == EventConstants.PRESSED ||
             type == EventConstants.REPEATED) && editor != null) 
        {
            switch (code) {
                case Constants.KEYCODE_SELECT:
                    editor.keyInput(type, code);
                    break;
                case Constants.KEYCODE_UP:
                case Constants.KEYCODE_DOWN:
                case Constants.KEYCODE_LEFT:
                case Constants.KEYCODE_RIGHT:
                    traverseInPopup(code);
                    break;
            }
        }
        // PopupLayers always swallow all key events
        return true;
    }

    /**
     * Paints popup background (including borders) and scrollbar
     * if it is present.
     * @param g - The graphics object to paint background on
     */
    public void paintBackground(Graphics g) {
        super.paintBackground(g);
        g.setColor(DateEditorSkin.COLOR_BORDER);
        g.drawRect(0, -1, bounds[W] - 1, bounds[H]);
        
        
        if (sbVisible) {
            int sbX = bounds[W] - 6;
            int sbY = 5;
            int sbH = bounds[H] - 12;
            int thumbY = sbY - 4 + 
                ((((hilightedIndex + 1) * 100) / numElements) * sbH) / 100;
            g.setColor(DateEditorSkin.COLOR_BORDER);
        
            // draw scrollbar
            g.drawLine(sbX, sbY, sbX, sbY + sbH - 1);
        
            // draw scrollbar thumb
            g.fillRect(sbX - (3 / 2), thumbY, 3, 4);
        }
    }

    /**
     * Paints the body of the popup layer.
     *
     * @param g The graphics context to paint to
     */
    public void paintBody(Graphics g) {
        boolean hilighted = false;
        int translatedY = 0;
        
        int transY = elementHeight;
        g.translate(2, 0);
        
        endIndex = startIndex + (elementsToFit - 1);
        
        if (hilightedIndex > endIndex) {
            endIndex = hilightedIndex;
            startIndex = endIndex - (elementsToFit - 1);
        }
        
        g.setFont(DateEditorSkin.FONT_POPUPS);
        for (int i = startIndex; i <= endIndex; i++) {
            hilighted = (i == hilightedIndex);
        
            if (hilighted) {
                g.setColor(DateEditorSkin.COLOR_TRAVERSE_IND);
                g.fillRect(0, 0, elementWidth - 7, elementHeight);
            }
        
            g.setColor(0);
            g.drawString(elements[i], 2, 0, 0);
            g.translate(0, transY);
            translatedY += transY;
        }
        
        g.translate(-2, -translatedY); 
    }


    // ********** package private *********** //

    /**
     * Gets currently selected index.
     *
     * @return currently selected index
     */
    int getSelectedIndex() {
        return hilightedIndex;
    }
    
    /**
     * Set the choice element size (width and height).
     *
     * @param w width of the element
     * @param h height of the element
     */
    void setElementSize(int w, int h) {
        elementWidth = w;
        elementHeight = h;
    }

    /**
     * Handle traversal in the open popup.
     *
     * @param code the code of the key event
     * @return true always, since popupLayers swallow all key events
     */
    boolean traverseInPopup(int code) {
        if (code == Constants.KEYCODE_UP) {
            if (hilightedIndex > 0) {
                hilightedIndex--;
                if (hilightedIndex < startIndex) {
                    startIndex--;
                }
                requestRepaint();
            } else if (circularTraversal) {
                // jump to the last element
                hilightedIndex = numElements - 1;
                startIndex = hilightedIndex - elementsToFit + 1;
                requestRepaint();
            }
        } else if (code == Constants.KEYCODE_DOWN) {
            if (hilightedIndex < (numElements - 1)) {
                hilightedIndex++;
                if (hilightedIndex > endIndex) {
                    startIndex++;
                }
                requestRepaint();
            } else if (circularTraversal) {
                // jump to the first element
                hilightedIndex = 0;
                startIndex = 0;
                requestRepaint();
            }
        }
        
        return true;
    }

    // ********* attributes ********* //

    /** 
     * The DateEditor that triggered this popup layer. 
     */
    DateEditor editor;

    /**
     * The viewport setting inside this popup (X, Y, W, H).
     * It is set in this layer's coordinate system.
     */
    private int viewport[];

    /**
     * Indicates if this popup layer is shown (true) or hidden (false).
     */
    boolean open;

    /**
     * Number of elements (list of choices) that constitute this 
     * popup layer.
     */
    private int numElements;

    /**
     * The list of choices in this popup layer.
     */
    private String[] elements;

    /**
     * The width of an element.
     */
    private int elementWidth;

    /**
     * The height of an element.
     */
    private int elementHeight;
    
    /**
     * Number of elements that can be shown within the viewport.
     */
    private int elementsToFit;
    
    /** 
     * Indicates whether we do/do not need to draw a scrollbar in 
     * this popup layer.
     */
    private boolean sbVisible; //  = false;

    /**
     * The start index of the chosen list of choices from the complete
     * list, to be displayed within the viewport.
     */
    private int startIndex = 0;

    /**
     * The end index of the chosen list of choices from the complete
     * list, to be displayed within the viewport.
     */
    private int endIndex = 0;

    /**
     * The index that is currently highlighted, is taken as the selected
     * index when popup closes.
     */
    private int hilightedIndex;    

    /**
     * True if traversal past the last item in the popup should jump to the
     * beginning and false if attempts to traverse past the last or the first 
     * items will have no effect.
     */
    private boolean circularTraversal; // = false
}
