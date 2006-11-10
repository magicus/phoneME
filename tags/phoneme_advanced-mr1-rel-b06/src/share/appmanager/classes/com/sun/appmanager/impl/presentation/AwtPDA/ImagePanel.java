/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

package com.sun.appmanager.impl.presentation.AwtPDA;

import java.awt.*;
import java.net.URL;

public class ImagePanel
    extends Container {
    MediaTracker tracker = new MediaTracker(this);
    String backgroundImageFile = null;
    URL bgImgURL = null;
    Image backgroundImage = null;
    String label = null;
    private int label_x = -1;
    private int label_y = -1;
    private Color textColor = Color.white;
    static private Font defaultFont = new Font("Serif", Font.BOLD, 16);
    private Font currentFont = null;
    private boolean labelDisplay = true;
    private boolean textShadow = false;

    public ImagePanel() {
        super();
    }

    public ImagePanel(Image img) {
        super();
        setImage(img);
    }

    public ImagePanel(String imageFile) {
        super();
        setImage(imageFile);
    }

    public void setImage(String imageFile) {
        backgroundImageFile = imageFile;
        bgImgURL = getClass().getResource(backgroundImageFile);
        backgroundImage =
            Toolkit.getDefaultToolkit().createImage(bgImgURL);

        try {
            tracker.addImage(backgroundImage, 0);
            tracker.waitForID(0);
        }
        catch (InterruptedException e) {
            e.printStackTrace();

            return;
        }
    }

    public void setImage(Image img) {
        backgroundImage = img;
    }

    public void setLabel(String str) {
        label = str;
    }

    public void setLabel(String str, int x, int y) {
        label = str;
        label_x = x;
        label_y = y;
    }

    public Dimension getPreferredSize() {
        return new Dimension( (int) backgroundImage.getWidth(null),
                             (int) backgroundImage.getHeight(null));
    }

    public void paint(Graphics g) {
        Dimension size = getSize();

        g.drawImage(backgroundImage, 0, 0, null);
        // for label
        if (label != null) {
            FontMetrics fm = getFontMetrics(getFont());
            int width = (int) fm.stringWidth(label);

            if (label_x == -1) {
                label_x = (int) (size.width - width) / 2;
            }
            if (label_y == -1) {
                label_y = (int) (size.height - (size.height - fm.getHeight())) / 2;
            }

            g.setColor(textColor);
            if (labelDisplay) {
//                if (currentFont != null) {
//                    g.setFont(currentFont);
//                }
//                else {
//                    g.setFont(defaultFont);
//                }

                if (textShadow) {
                    drawTextShadowString(g, label, label_x, label_y);
                }
                else {
                    g.drawString(label, label_x, label_y);
                }
            }
        }

        super.paint(g);
    }

    private void drawTextShadowString(Graphics g, String str, int x, int y) {
        Color origColor = g.getColor();
        g.setColor(Color.BLACK);
        g.drawString(str, x + 1, y + 1);
        g.setColor(origColor);
        g.drawString(label, x, y);
    }

}
