/*
 * @(#)FontDemo.java	1.6 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
package basis.demos;

import java.awt.*;
import java.util.HashMap;
import basis.Builder;

public class FontDemo extends Demo {
    private static String[] fontNames = new String[] {
        "Serif",
        "SansSerif",
        "Monospaced",
        "Dialog",
        "DialogInput",
        "Symbol"
    };
    private static HashMap cache;
    private static Image logo;
    private int fontSize;
    private int largeFontSize;
    private int smallFontSize;
    private char character = 'g';
    private int factor = 0;

    public FontDemo() {
        if (logo == null) {
            logo = ImageDemo.loadImage(this, "images/logo.gif");
        }
    }

    public void paint(Graphics g) {
        Dimension d = getSize();
        int w = d.width - 1;
        int h = d.height - 1;
        resize();
        ((Graphics2D) g).setComposite(AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.2f));
        g.drawImage(logo, 0, 0, w, h, this);
        ((Graphics2D) g).setComposite(AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 1.0f));
        Font font = new Font("monospaced", Font.PLAIN, fontNames.length * fontSize);
        g.setFont(font);
        setStatus("Font " + fontSize);
        g.setColor(Color.black);
        font = new Font("monospaced", Font.PLAIN, fontSize);
        FontMetrics fm = g.getFontMetrics(font);
        int charWidth = fm.charWidth('O');
        int y = fontSize + 2;
        for (int i = 0; i < fontNames.length; i++) {
            int x = 2;
            font = new Font(fontNames[i], Font.PLAIN, fontSize);
            g.setFont(font);
            g.drawString(fontNames[i], x, y);
            x += 11 * charWidth;
            font = new Font(fontNames[i], Font.BOLD, fontSize);
            g.setFont(font);
            g.drawString("Bold", x, y);
            x += 5 * charWidth;
            font = new Font(fontNames[i], Font.ITALIC, fontSize);
            g.setFont(font);
            g.drawString("Ital", x, y);
            x += 4 * charWidth;
            font = new Font(fontNames[i], Font.BOLD + Font.ITALIC, fontSize);
            g.setFont(font);
            g.drawString("BItal", x, y);
            y += (fontSize + 2);
        }
        y = fontNames.length * (fontSize + 2);
        largeFontSize = (h - y) / 2;
        int max = 2 * w / 3;
        largeFontSize = largeFontSize < max ? largeFontSize : max;
        font = new Font("Dialog", Font.PLAIN, largeFontSize);
        g.setFont(font);
        fm = g.getFontMetrics(font);
        charWidth = fm.charWidth(character);
        int ascent = fm.getAscent();
        int descent = fm.getDescent();
        int height = fm.getHeight();
        int leading = fm.getLeading();
        int maxAdvance = fm.getMaxAdvance();
        int maxAscent = fm.getMaxAscent();
        int maxDescent = fm.getMaxDescent();
        int x = 4;
        g.drawString("" + character, x, y + ascent);
        g.drawLine(x, y + ascent, x + charWidth, y + ascent);
        g.drawLine(x, y, x, y + ascent + descent);
        w = (d.width - charWidth) / 12;
        h = (d.height - y) / 9;
        smallFontSize = w < h ? w : h;
        x = charWidth + 8;
        font = new Font("monospaced", Font.PLAIN, smallFontSize);
        g.setFont(font);
        y += smallFontSize;
        g.drawString("size: " + largeFontSize, x, y);
        y += smallFontSize;
        g.drawString("ascent: " + ascent, x, y);
        y += smallFontSize;
        g.drawString("descent: " + descent, x, y);
        y += smallFontSize;
        g.drawString("height: " + height, x, y);
        y += smallFontSize;
        g.drawString("leading: " + leading, x, y);
        y += smallFontSize;
        g.drawString("maxAdvance: " + maxAdvance, x, y);
        y += smallFontSize;
        g.drawString("maxAscent: " + maxAscent, x, y);
        y += smallFontSize;
        g.drawString("maxDescent: " + maxDescent, x, y);
        y += smallFontSize;
        g.drawString("charWidth: " + charWidth, x, y);
    }

    private void resize() {
        Dimension d = getSize();
        if (factor == 0) {
            int guess = d.width / 40;
            Graphics g = getGraphics();
            if (g == null) {
                return;
            }
            fontSize = getFontSize(g, d.width, guess);
            factor = d.width / fontSize;
        }
        fontSize = d.width / factor;
        int max = d.height / 2 / fontNames.length;
        fontSize = fontSize < max ? fontSize : max;
    }

    private static int getFontSize(Graphics g, int width, int guess) {
        while (true) {
            Font font = new Font("monospaced", Font.BOLD + Font.ITALIC, guess + 1);
            FontMetrics fm = g.getFontMetrics(font);
            int fw = fm.stringWidth("Monospaced Bold Ital BItal");
            if (++guess >= width/10) {
                break;
            }
            if (fw <= width) {
                continue;
            } else {
                break;
            }
        }
        return guess;
    }
}

