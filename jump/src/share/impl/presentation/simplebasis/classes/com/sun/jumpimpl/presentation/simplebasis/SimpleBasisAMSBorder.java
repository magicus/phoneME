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

import java.awt.Color;
import java.awt.Graphics;
import java.awt.Insets;

public class SimpleBasisAMSBorder
    implements Cloneable {

    public static final int NONE = 0;
    public static final int LINE = 1;
    public static final int THREED_IN = 2;
    public static final int THREED_OUT = 3;
    public static final int ETCHED_IN = 4;
    public static final int EMBOSSED_OUT = 5;
    public static final int ROUND_RECT = 6;

    private int type = NONE;
    private Color JUMPGuiAMSBorder = null;
    private int topMargin = 0;
    private int leftMargin = 0;
    private int bottomMargin = 0;
    private int rightMargin = 0;
    private int minMargin = 0;
    private int maxMargin = 0;
    private int JUMPGuiAMSBorderThickness = 0;

    private static final double BRIGHTER_FACTOR = 0.8;
    private static final double DARKER_FACTOR = 0.65;

    /**
     * Returns a brighter version of this color.
     * Replaces the "awt.Color" function brighter to use, in my opinion,
     * a better factor (awt used FACTOR = 0.7)
     *
     * @param color     the color to which to apply the factor
     * @return          the new, brighter color
     */
    public static Color brighter(Color color) {
        Color newcolor = new Color(Math.min( (int) (color.getRed() *
            (1 / BRIGHTER_FACTOR)), 255),
                                   Math.min( (int) (color.getGreen() *
            (1 / BRIGHTER_FACTOR)), 255),
                                   Math.min( (int) (color.getBlue() *
            (1 / BRIGHTER_FACTOR)), 255));
        if (newcolor.equals(color)) {
            return Color.white;
        }
        return newcolor;
    }

    /**
     * Returns a darker version of this color.
     * Replaces the "awt.Color" function brighter to use, in my opinion,
     * a better factor (awt used FACTOR = 0.7)
     *
     * @param color     the color to which to apply the factor
     * @return          the new, darker color
     */
    public static Color darker(Color color) {
        Color newcolor = new Color(Math.max( (int) (color.getRed() *
            DARKER_FACTOR), 0),
                                   Math.max( (int) (color.getGreen() *
            DARKER_FACTOR), 0),
                                   Math.max( (int) (color.getBlue() *
            DARKER_FACTOR), 0));
        if (newcolor.equals(color)) {
            return Color.black;
        }
        return newcolor;
    }

    /**
     * Returns a clone of this JUMPGuiAMSBorder
     *
     * @return      the new, identical JUMPGuiAMSBorder
     */
/*
    public Object clone() {
        SimpleBasisAMSBorder newJUMPGuiAMSBorder = new SimpleBasisAMSBorder();
        newJUMPGuiAMSBorder.type = type;
        newJUMPGuiAMSBorder.SimpleBasisAMSBorder = SimpleBasisAMSBorder;
        newJUMPGuiAMSBorder.topMargin = topMargin;
        newJUMPGuiAMSBorder.leftMargin = leftMargin;
        newJUMPGuiAMSBorder.bottomMargin = bottomMargin;
        newJUMPGuiAMSBorder.rightMargin = rightMargin;
        newJUMPGuiAMSBorder.minMargin = minMargin;
        newJUMPGuiAMSBorder.maxMargin = maxMargin;
        newJUMPGuiAMSBorder.JUMPGuiAMSBorderThickness = JUMPGuiAMSBorderThickness;
        return newJUMPGuiAMSBorder;
    }
*/

    /**
     * sets all margins and thicknesses to zero
     */
/*
    public void setNoInsets() {
        topMargin = 0;
        leftMargin = 0;
        bottomMargin = 0;
        rightMargin = 0;
        minMargin = 0;
        maxMargin = 0;
        JUMPGuiAMSBorderThickness = 0;
    }
*/

    /**
     * Returns the JUMPGuiAMSBorder type (e.g., JUMPGuiAMSBorder.LINE)
     *
     * @return      the JUMPGuiAMSBorder type id
     */
/*
    public int getType() {
        return type;
    }
*/

    /**
     * Sets the SimpleBasisAMSBorder type (e.g., to SimpleBasisAMSBorder.LINE)
     * 
     * @param type     the SimpleBasisAMSBorder type
     */
    public synchronized void setType(int type) {
        this.type = type;
    }

    /**
     * Returns the current JUMPGuiAMSBorder color.  If null (the default), a JUMPGuiAMSBorder color is
     * dynamically derived from the current background (passed to the "paint" function).
     *
     * @return     the current Color value for the JUMPGuiAMSBorder
     */
/*
    public Color getJUMPGuiAMSBorder() {
        return SimpleBasisAMSBorder;
    }
*/

    /**
     * Sets the current JUMPGuiAMSBorder color.  If null (the default), a JUMPGuiAMSBorder color is
     * dynamically derived from the current background (passed to the "paint" function).
     *
     * @param JUMPGuiAMSBorder     the new JUMPGuiAMSBorder color
     */
/*
    public synchronized void setJUMPGuiAMSBorder(Color SimpleBasisAMSBorder) {
        this.SimpleBasisAMSBorder = SimpleBasisAMSBorder;
    }
*/

    /**
     * Returns the top margin.
     *
     * @return          the top margin
     */
/*
    public int getTopMargin() {
        return topMargin;
    }
*/

    /**
     * Returns the left margin.
     *
     * @return          the left margin
     */
/*
    public int getLeftMargin() {
        return leftMargin;
    }
*/

    /**
     * Returns the bottom margin.
     *
     * @return          the bottom margin
     */
/*
    public int getBottomMargin() {
        return bottomMargin;
    }
 */

    /**
     * Returns the right margin.
     *
     * @return          the right margin
     */
    public int getRightMargin() {
        return rightMargin;
    }

    /**
     * Used internally to calculate the minimum/maximum margin values.
     */
    private void resetMinMaxMargin() {
        maxMargin = topMargin;
        maxMargin = Math.max(maxMargin, leftMargin);
        maxMargin = Math.max(maxMargin, bottomMargin);
        maxMargin = Math.max(maxMargin, rightMargin);
        minMargin = topMargin;
        minMargin = Math.min(minMargin, leftMargin);
        minMargin = Math.min(minMargin, bottomMargin);
        minMargin = Math.min(minMargin, rightMargin);
    }

    /**
     * Sets all margins (top, left, bottom, and right) to the same, given value.
     *
     * @param margin         the margin
     */
    public synchronized void setMargins(int margin) {
        topMargin = margin;
        leftMargin = margin;
        bottomMargin = margin;
        rightMargin = margin;
        resetMinMaxMargin();
    }

    /**
     * Sets all margins (top, left, bottom, and right) to the given values.
     *
     * @param top           the top margin
     * @param left          the left margin
     * @param bottom        the bottom margin
     * @param right         the right margin
     */
    public synchronized void setMargins(int top, int left, int bottom,
                                        int right) {
        topMargin = top;
        leftMargin = left;
        bottomMargin = bottom;
        rightMargin = right;
        resetMinMaxMargin();
    }

    /**
     * Sets the top margin
     *
     * @param top           the top margin
     */
    /*
    public synchronized void setTopMargin(int margin) {
        topMargin = margin;
        resetMinMaxMargin();
    }
*/

    /**
     * Sets the left margin
     *
     * @param left          the left margin
     */
/*
    public synchronized void setLeftMargin(int margin) {
        leftMargin = margin;
        resetMinMaxMargin();
    }
*/

    /**
     * Sets the bottom margin
     *
     * @param bottom        the bottom margin
     */
/*
    public synchronized void setBottomMargin(int margin) {
        bottomMargin = margin;
        resetMinMaxMargin();
    }
*/

    /**
     * Sets the right margin
     *
     * @param right         the right margin
     */
/*
    public synchronized void setRightMargin(int margin) {
        rightMargin = margin;
        resetMinMaxMargin();
    }
*/

    /**
     * Returns the current setting for the JUMPGuiAMSBorder thickness
     *
     * @return    the JUMPGuiAMSBorder thickness
     */
/*
    public int getJUMPGuiAMSBorderThickness() {
        return JUMPGuiAMSBorderThickness;
    }
*/

    /**
     * Sets the SimpleBasisAMSBorder thickness
     * 
     * @param JUMPGuiAMSBorderThickness         the SimpleBasisAMSBorder thickness
     */
    public synchronized void setJUMPGuiAMSBorderThickness(int JUMPGuiAMSBorderThickness) {
        this.JUMPGuiAMSBorderThickness = JUMPGuiAMSBorderThickness;
    }

    /**
     * returns the left, right, top, and bottom insets of the background of the
     * current style, taking into account thickness and margins.
     *
     * @return  an Insets object containing the inset values
     */
    public Insets getInsets() {
        int top = JUMPGuiAMSBorderThickness + topMargin;
        int left = JUMPGuiAMSBorderThickness + leftMargin;
        int bottom = JUMPGuiAMSBorderThickness + bottomMargin;
        int right = JUMPGuiAMSBorderThickness + rightMargin;
        return new Insets(top, left, bottom, right);
    }

    /**
     * returns the left, right, top, and bottom insets of the background of the
     * current style, taking into account thickness and margins.
     * 
     * @param g           the Graphics in which to paint
     * @param background  the current background color (or null), used if available to
     *                      calculate the SimpleBasisAMSBorder color if that is null.
     * @param x           the x location of the upper-left point of the SimpleBasisAMSBorder
     * @param y           the y location of the upper-left point of the SimpleBasisAMSBorder
     * @param width       the width of the rectangle in which to draw the SimpleBasisAMSBorder
     * @param height      the height of the rectangle in which to draw the SimpleBasisAMSBorder
     */
    public void paint(Graphics g, Color background, int x, int y, int width,
                      int height) {
        if (JUMPGuiAMSBorder == null) {
            if ( (type == LINE) ||
                (type == ROUND_RECT)) {
                if (background == Color.black) {
                    JUMPGuiAMSBorder = Color.white;
                }
                else {
                    JUMPGuiAMSBorder = Color.black;
                }
            }
            else {
                if (background == null) {
                    JUMPGuiAMSBorder = Color.lightGray;
                }
                else {
                    JUMPGuiAMSBorder = background;
                }
            }
        }

        if (JUMPGuiAMSBorder != null) {
            g.setColor(JUMPGuiAMSBorder);

            Color brighter = null;
            Color darker = null;

            switch (type) {

                case THREED_IN:
                case THREED_OUT:
                case ETCHED_IN:
                case EMBOSSED_OUT: {
                    brighter = brighter(JUMPGuiAMSBorder);
                    darker = darker(JUMPGuiAMSBorder);
                    break;
                }
            }

            for (int idx = 0; idx < JUMPGuiAMSBorderThickness; idx++) {
                if (type == LINE) {
                    g.drawRect(x + idx, y + idx,
                               ( (width - 1) - (idx * 2)),
                               ( (height - 1) - (idx * 2)));
                }
                else if (type == ROUND_RECT) {

                    int arcSize = (minMargin * 8 - (idx * 2));
                    int rrx = x + idx;
                    int rry = y + idx;
                    int rrw = (width - 1) - (idx * 2);
                    int rrh = (height - 1) - (idx * 2);

                    g.drawRoundRect(x + idx, y + idx, rrw, rrh, arcSize,
                                    arcSize);
                    if ( (idx + 1) < JUMPGuiAMSBorderThickness) {
                        g.drawRoundRect(rrx, rry, rrw, rrh - 1, arcSize - 1,
                                        arcSize);
                        g.drawRoundRect(rrx + 1, rry, rrw, rrh - 1, arcSize - 1,
                                        arcSize);
                        g.drawRoundRect(rrx, rry, rrw - 1, rrh, arcSize,
                                        arcSize - 1);
                        g.drawRoundRect(rrx, rry + 1, rrw - 1, rrh, arcSize,
                                        arcSize - 1);
                    }
                }
                else {
                    Color top = brighter;
                    Color bottom = darker;

                    if ( (type == THREED_IN) ||
                        (type == ETCHED_IN)) {
                        top = darker;
                        bottom = brighter;
                    }

                    if ( (type == ETCHED_IN) ||
                        (type == EMBOSSED_OUT)) {
                        if (idx >= (JUMPGuiAMSBorderThickness / 2)) {
                            Color temp = top;
                            top = bottom;
                            bottom = temp;
                        }

                    }

                    if ( (idx == (JUMPGuiAMSBorderThickness - 1)) &&
                        (type == THREED_IN)) {
                        g.setColor(darker(top));
                    }
                    else {
                        g.setColor(top);
                    }

                    g.drawLine(x + idx, y + idx,
                               x + idx,
                               y + ( (height - 1) - (idx)));
                    g.drawLine(x + idx, y + idx,
                               x + ( (width - 1) - (idx)),
                               y + idx);

                    if ( ( (idx == (JUMPGuiAMSBorderThickness - 1)) &&
                          (type == THREED_IN)) ||
                        ( (idx == 0) &&
                         (type == THREED_OUT))) {
                        g.setColor(darker(bottom));
                    }
                    else {
                        g.setColor(bottom);
                    }

                    g.drawLine(x + idx,
                               y + ( (height - 1) - (idx)),
                               x + ( (width - 1) - (idx)),
                               y + ( (height - 1) - (idx)));
                    g.drawLine(x + ( (width - 1) - (idx)),
                               y + idx,
                               x + ( (width - 1) - (idx)),
                               y + ( (height - 1) - (idx)));
                }
            }
        }
    }
}
