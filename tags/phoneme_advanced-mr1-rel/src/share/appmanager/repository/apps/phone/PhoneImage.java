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

import java.awt.*;

public class PhoneImage
    extends ImagePanel {

    public String numbers = "";
    public KeyLocationHash klh;
    Font displayRegionFont = null;
    boolean doesntFitDisplayRegion = false;
    int shift = 0;

    public PhoneImage(String imageFile, KeyLocationHash klh) {
        super(imageFile);
        this.klh = klh;
        displayRegionFont = new Font("Monospaced", Font.PLAIN,
                                     24);
    }

    public void paint(Graphics g) {
        ( (Graphics2D) g).setComposite(AlphaComposite.Src);
        super.paint(g);
        g.setColor(Color.white);
        g.setFont(displayRegionFont);
        ( (Graphics2D) g).setComposite(AlphaComposite.SrcOver);
        int charWidth = g.getFontMetrics().charWidth('1');
        String displayRegionStr = numbers;

        if (shift > 0) {
            if (shift >= displayRegionStr.length()) {
                displayRegionStr = "";
            }
            else {
                displayRegionStr = displayRegionStr.substring(0,
                    displayRegionStr.length() - shift);
            }
        }

        int stringWidth = charWidth * displayRegionStr.length();
        doesntFitDisplayRegion =
            ( (klh.displayRegion.x_max - klh.displayRegion.x_min) < stringWidth);

        while ( (klh.displayRegion.x_max - klh.displayRegion.x_min) <
               stringWidth) {
            displayRegionStr = displayRegionStr.substring(1,
                displayRegionStr.length());
            stringWidth = charWidth * displayRegionStr.length();
        }

        System.out.println("drawString(" + displayRegionStr + ", " +
                           Integer.
                           toString(klh.displayRegion.x_max - stringWidth) +
                           ", " + klh.displayRegion.y_max + ")");
        g.drawString(displayRegionStr,
                     klh.displayRegion.x_max - stringWidth,
                     klh.displayRegion.y_max);
    }

}
