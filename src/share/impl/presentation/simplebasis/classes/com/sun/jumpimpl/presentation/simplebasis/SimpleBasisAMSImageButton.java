/*
 * Portions Copyright  2000-2006 Sun Microsystems, Inc. All Rights Reserved.
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

/**
 * Copyright(c) 1997 DTAI, Incorporated (http://www.dtai.com)
 *
 *                        All rights reserved
 *
 * Permission to use, copy, modify and distribute this material for
 * any purpose and without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies, and that the name of DTAI, Incorporated not be used in
 * advertising or publicity pertaining to this material without the
 * specific, prior written permission of an authorized representative of
 * DTAI, Incorporated.
 *
 * DTAI, INCORPORATED MAKES NO REPRESENTATIONS AND EXTENDS NO WARRANTIES,
 * EXPRESS OR IMPLIED, WITH RESPECT TO THE SOFTWARE, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR ANY PARTICULAR PURPOSE, AND THE WARRANTY AGAINST
 * INFRINGEMENT OF PATENTS OR OTHER INTELLECTUAL PROPERTY RIGHTS.  THE
 * SOFTWARE IS PROVIDED "AS IS", AND IN NO EVENT SHALL DTAI, INCORPORATED OR
 * ANY OF ITS AFFILIATES BE LIABLE FOR ANY DAMAGES, INCLUDING ANY
 * LOST PROFITS OR OTHER INCIDENTAL OR CONSEQUENTIAL DAMAGES RELATING
 * TO THE SOFTWARE.
 */

package com.sun.jumpimpl.presentation.simplebasis;

import java.awt.AWTEventMulticaster;
import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Graphics;
import java.awt.Image;
import java.awt.Insets;
import java.awt.MediaTracker;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.FocusEvent;
import java.awt.event.FocusListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.image.FilteredImageSource;
import java.awt.image.RGBImageFilter;

public class SimpleBasisAMSImageButton
        extends Component implements FocusListener, MouseListener {
    
    public static final int UNPRESSED = 0;
    public static final int DEPRESSED = 1;
    public static final int OVER = 2;
    public static final int DISABLED = 3;
    
    private static Font defaultFont = new Font("Serif", Font.BOLD, 12);
    private static final SimpleBasisAMSBorder defaultUnpressedBorder =
            new DefaultJUMPGuiAMSImageButtonBorder(false);
    private static final SimpleBasisAMSBorder defaultArmedBorder =
            new DefaultJUMPGuiAMSImageButtonBorder(true);
    
    private String label = null;
    private int labelX = -1;
    private int labelY = -1;
    private Dimension prefSize = null;
    private boolean paintBorders = false;
    private boolean labelDisplay = true;
    private MediaTracker tracker = null;
    private Image images[] = new Image[4];
    private SimpleBasisAMSBorder borders[] = new SimpleBasisAMSBorder[4];
    private boolean generatedDisabled = false;
    private int buttonState = UNPRESSED;
    private ActionListener actionListener = null;
    private Color textColor = Color.white;
    private Font currentFont = null;
    private boolean textShadow = false;
    private boolean mousedown = false;
    private boolean lastFocused = false;
    private boolean focusable = true;
    
    /**
     * Constructs an SimpleBasisAMSImageButton
     */
    public SimpleBasisAMSImageButton() {
        tracker = new MediaTracker(this);
        setUnpressedBorder(defaultUnpressedBorder);
        setDepressedBorder(defaultArmedBorder);
        addFocusListener(this);
        addMouseListener(this);
    }
    
    /**
     * Constructs an SimpleBasisAMSImageButton with the given image.
     *
     * @param image         the image for all states of the button
     *                      (until other images are assigned)
     */
    public SimpleBasisAMSImageButton(Image image) {
        this();
        setUnpressedImage(image);
    }
    
    public SimpleBasisAMSImageButton(String label) {
        this();
        setLabel(label);
    }
    
    public SimpleBasisAMSImageButton(Image image, String label) {
        this(image);
        setLabel(label);
    }
    
    public void doAction() {
        ActionEvent ae = new ActionEvent(this,
                ActionEvent.ACTION_PERFORMED,
                "");
        actionListener.actionPerformed(ae);
    }
    
    public void setFocusable(boolean val) {
        focusable = val;
    }
    
    public boolean isFocusable() {
        return focusable;
    }
    
    public boolean isEnabled() {
        return true;
    }
    
    public boolean lastFocused() {
        return lastFocused;
    }
    
    public void focusGained(FocusEvent e) {
        //paintBorders = true;
        repaint();
        lastFocused = true;
    }
    
    public void focusLost(FocusEvent e) {
        //paintBorders = false;
        repaint();
    }
    
    /*
        public SimpleBasisAMSImageButton(Image image, int width, int height) {
            this(image);
            prefSize = new Dimension(width, height);
        }
     */
    
    /*
     public SimpleBasisAMSImageButton(Image image, String label, int width, int height) {
            this(image);
            setLabel(label);
            prefSize = new Dimension(width, height);
        }
     */
    
    /**
     * Used internally to add the Image to the array and the MediaTracker,
     * start loading the image if necessary via the tracker's "checkID", and
     * repaint if necessary.
     *
     * @param id        the buttonState id (also used as image id for the MediaTracker)
     * @param image     the image, which is not supposed to be null
     */
    private synchronized void setImage(int id, Image image) {
        if (images[id] != image) {
            images[id] = image;
            if (image != null) {
                tracker.addImage(image, id);
                tracker.checkID(id, true);
            }
            if (buttonState == id) {
                repaint();
            }
        }
    }
    
    public void setTextShadow(boolean val) {
        textShadow = val;
    }
    
    public void setFont(Font f) {
        currentFont = f;
    }
    
    public void setTextColor(Color c) {
        textColor = c;
        repaint();
    }
    
    public void setPaintBorders(boolean val) {
        paintBorders = val;
    }
    
    public void setLabelDisplay(boolean val) {
        labelDisplay = val;
    }
    
    /**
     * Sets the image to display when the button is not pressed or hilited
     * because of a mouse-over.  This image is also used in those other cases
     * if no alternative image is requested.
     *
     * @param image     the unarmed image
     */
    public void setUnpressedImage(Image image) {
        setImage(UNPRESSED, image);
        if (images[DEPRESSED] == null) {
            setDepressedImage(image);
        }
        if (images[OVER] == null) {
            setOverImage(image);
        }
        if ( (images[DISABLED] == null) ||
                generatedDisabled) {
            setDisabledImage(null);
        }
    }
    
    /**
     * Sets the image to display when the button is pressed and the mouse
     * is still over the button.
     *
     * @param image     the armed image
     */
    public void setDepressedImage(Image image) {
        if (image != null) {
            setImage(DEPRESSED, image);
        } else {
            setImage(DEPRESSED, images[UNPRESSED]);
        }
    }
    
    /**
     * Sets the image to display when the button is not pressed and the mouse
     * is over the button.
     *
     * @param image     the over image
     */
    public void setOverImage(Image image) {
        if (image != null) {
            setImage(OVER, image);
        } else {
            setImage(OVER, images[UNPRESSED]);
        }
    }
    
    /**
     * Sets the image to display when the button is disabled.
     *
     * @param image     the disabled image
     */
    public void setDisabledImage(Image image) {
        generatedDisabled = false;
        if ( (image == null) &&
                (images[UNPRESSED] != null)) {
            generatedDisabled = true;
            image = createImage(new FilteredImageSource(images[UNPRESSED].
                    getSource(),
                    new DisableImageFilter()));
        }
        setImage(DISABLED, image);
    }
    
    /**
     * Gets the image to display when the button is not pressed or hilited
     * because of a mouse-over.  This image is also used in those other cases
     * if no alternative image is requested.
     *
     * @return     the unarmed image
     */
    /*
        public Image getUnpressedImage() {
            return (images[UNPRESSED]);
        }
     */
    
    /**
     * Gets the image to display when the button is pressed and the mouse
     * is still over the button.
     *
     * @return     the armed image
     */
    /*
        public Image getDepressedImage() {
            return (images[DEPRESSED]);
        }
     */
    
    /**
     * Gets the image to display when the button is not pressed and the mouse
     * is over the button.
     *
     * @return     the over image
     */
    /*
        public Image getOverImage() {
            return (images[OVER]);
        }
     */
    
    /**
     * Gets the image to display when the button is disabled.
     *
     * @return     the armed image
     */
    /*
        public Image getDisabledImage() {
            return (images[DISABLED]);
        }
     */
    
    /**
     * Used internally to add the Border to the array and repaint if necessary.
     *
     * @param   id      the buttonState, used to index the array
     * @param   border  the Border, which is not supposed to be null
     */
    private synchronized void setBorder(int id, SimpleBasisAMSBorder border) {
        if (borders[id] != border) {
            borders[id] = border;
            if (buttonState == id) {
                repaint();
            }
        }
    }
    
    /**
     * Sets the border to display when the button is not pressed or hilited
     * because of a mouse-over.  This border is also used in those other cases
     * if no alternative border is requested.
     *
     * @param border     the unarmed border
     */
    public void setUnpressedBorder(SimpleBasisAMSBorder border) {
        setBorder(UNPRESSED, border);
        if (borders[DEPRESSED] == null) {
            setDepressedBorder(border);
        }
        if (borders[OVER] == null) {
            setOverBorder(border);
        }
        if (borders[DISABLED] == null) {
            setDisabledBorder(border);
        }
    }
    
    /**
     * Sets the border to display when the button is pressed and the mouse
     * is still over the button.
     *
     * @param border     the armed border
     */
    public void setDepressedBorder(SimpleBasisAMSBorder border) {
        if (border != null) {
            setBorder(DEPRESSED, border);
        } else {
            setBorder(DEPRESSED, borders[UNPRESSED]);
        }
    }
    
    /**
     * Sets the border to display when the button is not pressed and the mouse
     * is over the button.
     *
     * @param border     the over border
     */
    public void setOverBorder(SimpleBasisAMSBorder border) {
        if (border != null) {
            setBorder(OVER, border);
        } else {
            setBorder(OVER, borders[UNPRESSED]);
        }
        setBorder(OVER, border);
    }
    
    /**
     * Sets the border to display when the button is disabled.
     *
     * @param border     the disabled border
     */
    public void setDisabledBorder(SimpleBasisAMSBorder border) {
        if (border != null) {
            setBorder(DISABLED, border);
        } else {
            setBorder(DISABLED, borders[UNPRESSED]);
        }
        if (buttonState == DISABLED) {
            repaint();
        }
    }
    
    /**
     * Gets the border to display when the button is not pressed or hilited
     * because of a mouse-over.  This border is also used in those other cases
     * if no alternative border is requested.
     *
     * @return     the unarmed border
     */
    /*
        public Border getUnpressedBorder() {
            return (borders[UNPRESSED]);
        }
     */
    
    /**
     * Gets the border to display when the button is pressed and the mouse
     * is still over the button.
     *
     * @return     the armed border
     */
    /*
        public Border getDepressedBorder() {
            return (borders[DEPRESSED]);
        }
     */
    
    /**
     * Gets the border to display when the button is not pressed and the mouse
     * is over the button.
     *
     * @return     the over border
     */
    /*
        public Border getOverBorder() {
            return (borders[OVER]);
        }
     */
    
    /**
     * Gets the border to display when the button is disabled.
     *
     * @return     the armed border
     */
    /*
        public Border getDisabledBorder() {
            return (borders[DISABLED]);
        }
     */
    
    /**
     * Gets the current buttonState id for the button
     *
     * @return     the button state integer id
     */
    /*
        public int getButtonState() {
            return buttonState;
        }
     */
    
    /**
     * Sets the current buttonState id for the button
     *
     * @param   buttonState     the button state integer id
     */
    protected void setButtonState(int buttonState) {
        if (buttonState != this.buttonState) {
            this.buttonState = buttonState;
            repaint();
        }
    }
    
    public void setLabel(String str) {
        label = str;
    }
    
    public void setLabel(String str, int x, int y) {
        label = str;
        labelX = x;
        labelY = y;
    }
    
    public String getLabel() {
        return label;
    }
    
    /**
     * Overrides awt.Component.disable() to also set the button state.
     */
    /*
        public void disable() {
            setButtonState(DISABLED);
            super.disable();
        }
     */
    
    /**
     * Overrides awt.Component.enable() to also set the button state.
     */
    /*
        public void enable() {
            setButtonState(UNPRESSED);
            super.enable();
        }
     */
    
    /**
     * Overrides awt.Component.paint() to paint the current border and image.
     *
     * @param     g     The Graphics in which to draw
     */
    public void paint(Graphics g) {
        
        Dimension size = getSize();
        if (hasFocus()) {
            g.setColor(Color.black);
        } else {
            g.setColor(getForeground());
        }
        g.fillRect(0, 0, size.width, size.height);
        
        try {
            if (!tracker.checkID(buttonState)) {
                tracker.waitForID(buttonState);
            }
            if (!tracker.isErrorID(buttonState)) {
                Insets insets = borders[buttonState].getInsets();
                if (images[buttonState] != null) {
                    int imageWidth = images[buttonState].getWidth(this);
                    int imageHeight = images[buttonState].getHeight(this);
                    int x = insets.left +
                            ( ( (size.width - (insets.left + insets.right)) -
                            imageWidth) / 2);
                    int y = insets.top +
                            ( ( (size.height - (insets.top + insets.bottom)) -
                            imageHeight) / 2) - 5;
                    g.drawImage(images[buttonState], x, y, this);
                }
            } else {
                trace("ERROR: Invalid image used for button: " + label);
            }
        } catch (Exception e) {
            trace(e.getMessage());
        }
        
        // for label
        if (label != null) {
            FontMetrics fm = null;
            if (currentFont != null) {
                g.setFont(currentFont);
                fm = getFontMetrics(currentFont);
            } else {
                g.setFont(defaultFont);
                fm = getFontMetrics(defaultFont);
            }
            
            int labelWidth = (int) fm.stringWidth(label);
            int buttonWidth = size.width;
            String displayLabel = label;
            String tmpLabel = null;
            while (labelWidth > size.width) {
                displayLabel = displayLabel.substring(0, displayLabel.length() - 1);
                tmpLabel = displayLabel + "...";
                labelWidth = (int) fm.stringWidth(tmpLabel);
            }
            
            if (tmpLabel != null) {
                displayLabel = tmpLabel;
            }
            
            int height = (int) fm.getHeight();
            if (labelX == -1) {
                labelX = ( (size.width - labelWidth) / 2);
                if (labelX < 0) {
                    labelX = 0;
                }
            }
            if (labelY == -1) {
                if (images[buttonState] != null && !tracker.isErrorID(buttonState)) {
                    labelY = (int) (size.height - 5);
                } else {
                    labelY = (int) (size.height / 2) + (height / 2) ;
                }
            }
            
            g.setColor(textColor);
            if (labelDisplay) {
                if (textShadow) {
                    drawTextShadowString(g, displayLabel, labelX, labelY);
                } else {
                    g.drawString(displayLabel, labelX, labelY);
                }
            }
        }
        
        if (paintBorders) {
            borders[buttonState].paint(g, getBackground(), 0, 0, size.width,
                    size.height);
        }
    }
    
    private void trace(String str) {
        SimpleBasisAMS.trace(str);
    }
    
    private void drawTextShadowString(Graphics g, String str, int x, int y) {
        Color origColor = g.getColor();
        g.setColor(Color.BLACK);
        g.drawString(str, x + 1, y + 1);
        g.setColor(origColor);
        g.drawString(str, x, y);
    }
    
    /*
        public Dimension getMinimumSize() {
            return getPreferredSize();
        }
     */
    
    /*
        public Dimension getMaximumSize() {
            return getPreferredSize();
        }
     */
    
    /**
     * Overrides awt.Component.preferredSize() to return the preferred size of the button.
     * This assumes the images (if more than one) are all the same size.  It also calculates
     * the maximum insets from all borders and adds them to the image dimensions.
     *
     * @param     g     The Graphics in which to draw
     */
    public Dimension getPreferredSize() {
        
        if (prefSize != null) {
            return prefSize;
        } else {
            return new Dimension(120, 70);
        }
        /*
                if (prefSize != null) {
                    return prefSize;
                }
         
                Dimension pref = new Dimension();
                try {
                    if (!tracker.checkID(buttonState)) {
                        tracker.waitForID(buttonState);
                    }
                    if (!tracker.isErrorID(buttonState)) {
                        Dimension size = size();
                        pref.width = images[buttonState].getWidth(this);
                        pref.height = images[buttonState].getHeight(this);
                    }
                    int maxWidthAdd = 0;
                    int maxHeightAdd = 0;
                    for (int i = 0; i < DISABLED; i++) {
                        Insets insets = borders[i].getInsets();
         maxWidthAdd = Math.max(maxWidthAdd, insets.left + insets.right);
                        maxHeightAdd = Math.max(maxHeightAdd,
                                                insets.top + insets.bottom);
                    }
                    pref.width += maxWidthAdd;
                    pref.height += maxHeightAdd;
                }
                catch (InterruptedException ie) {
                }
                return pref;
         */
    }
    
    public void processMouseEvent(MouseEvent e) {
        if (actionListener != null) {
            if (e.getID() == MouseEvent.MOUSE_RELEASED) {
                ActionEvent ae = new ActionEvent(this,
                        ActionEvent.ACTION_PERFORMED,
                        "");
                actionListener.actionPerformed(ae);
            }
        }
        
    }
    
    public synchronized void addActionListener(ActionListener l) {
        actionListener = AWTEventMulticaster.add(actionListener, l);
    }
    
    public synchronized void removeActionListener(ActionListener l) {
        actionListener = AWTEventMulticaster.remove(actionListener, l);
    }
    
    /*
        public synchronized ActionListener getActionListener() {
            return actionListener;
        }
     */
    
    public void mouseClicked(MouseEvent e) {
        requestFocusInWindow();
    }
    
    public void mousePressed(MouseEvent e) {
    }
    
    public void mouseReleased(MouseEvent e) {
    }
    
    public void mouseEntered(MouseEvent e) {
    }
    
    public void mouseExited(MouseEvent e) {
    }
    
}

/**
 * DisableImageFilter - an RGBImageFilter that "greys out" an image by "blanking out"
 * every other pixel.
 */
class DisableImageFilter
        extends RGBImageFilter {
    /**
     * Constructs a DisableImageFilter.  The canFilterIndexColorModel is set to false
     * because the pixel index is important during filtering.
     */
    public DisableImageFilter() {
        canFilterIndexColorModel = false;
    }
    
    /**
     * Called when a disabled image is created to alter the pixels to be blanked out.
     *
     * @param   x   the x position of the pixel
     * @param   y   the y position of the pixel
     * @param   rgb the rgb value of the pixel
     */
    public int filterRGB(int x, int y, int rgb) {
        if ( ( (x % 2) ^ (y % 2)) == 1) {
            return (rgb & 0xffffff);
        } else {
            return rgb;
        }
    }
}

/**
 * DefaultJUMPGuiAMSImageButtonBorder - a Border, subclassed to set the default border values.
 */
class DefaultJUMPGuiAMSImageButtonBorder
        extends SimpleBasisAMSBorder {
    
    public DefaultJUMPGuiAMSImageButtonBorder(boolean armed) {
        setJUMPGuiAMSBorderThickness(2);
        if (armed) {
            setType(THREED_IN);
            setMargins(4, 4, 2, 2);
        } else {
            setType(THREED_OUT);
            setMargins(3);
        }
    }
}
