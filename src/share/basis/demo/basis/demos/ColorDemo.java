/*
 * @(#)ColorDemo.java	1.5 06/10/10
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
import java.awt.event.*;
import basis.Builder;

public class ColorDemo extends Demo implements MouseListener, MouseMotionListener {
    private static Color[] rgb;
    private static Color[] bw;
    private static Color[] colors;
    private static Color[] shades;
    private static Color[] alphas;
    private static Color[] systemColors;
    private static String[] systemColorNames;
    private String oldStatus;

    public ColorDemo() {
        addMouseListener(this);
        addMouseMotionListener(this);
    }

    public void paint(Graphics g) {
        Dimension d = getSize();
        g.setColor(Color.black);
        for (int y = 2; y < d.height - 3; y += 4) {
            g.drawLine(0, y, d.width, y);
        }
        draw(rgb, 0);
        draw(bw, 1);
        draw(colors, 2);
        draw(shades, 3);
        draw(alphas, 4);
        draw(systemColors, 5);
    }

    private void draw(Color[] array, int offset) {
        Graphics g = getGraphics();
        Dimension d = getSize();
        int h = d.height / 6;
        int x = 0;
        int y = offset * h;
        if (d.width < array.length) {
            for (x = 0; x < d.width; x++) {
                int index = x * array.length / d.width;
                g.setColor(array[index]);
                g.drawLine(x, y, x, y + h);
            }
        } else {
            int w = d.width / array.length;
            int r = d.width % array.length;
            for (int i = 0; i < array.length; i++) {
                int fill = r-- > 0 ? 1 : 0;
                g.setColor(array[i]);
                g.fillRect(x, y, w + fill, h);
                x += w + fill;
            }
        }
    }

    public void mouseClicked(MouseEvent e) {
        showSystemColorName(e.getX(), e.getY());
    }

    public void mouseEntered(MouseEvent e) {
        if (oldStatus == null) {
            oldStatus = getStatus();
        }
    }

    public void mouseExited(MouseEvent e) {
        setStatus(oldStatus);
    }

    public void mousePressed(MouseEvent e) {
        showSystemColorName(e.getX(), e.getY());
    }

    public void mouseReleased(MouseEvent e) {
    }

    public void mouseDragged(MouseEvent e) {
        showSystemColorName(e.getX(), e.getY());
    }

    public void mouseMoved(MouseEvent e) {
        showSystemColorName(e.getX(), e.getY());
    }

    private void showSystemColorName(int x, int y) {
        Dimension d = getSize();
        int w = d.width - 1;
        int h = d.height - 1;
        if (x >= 0 && x < w && y > (5 * h) / 6 && y < h) {
            int i = x * systemColorNames.length / w;
            setStatus(systemColorNames[i]);
        } else {
            setStatus(oldStatus);
        }
    }

    static {
        rgb = new Color[1000];
        int[] array = new int[rgb.length];
        int sixth = rgb.length / 6;
        for (int i = 0; i < array.length; i++) {
            if (i <= 2 * sixth) {
                array[i] = 0;
            } else if (i > 2 * sixth && i < 3 * sixth) {
                array[i] = (i - 2 * sixth) * 255 / sixth;
            } else if (i >= 3 * sixth && i <= 5 * sixth) {
                array[i] = 255;
            } else if (i > 5 * sixth && i < 6 * sixth) {
                array[i] = 255 - (i - 5 * sixth) * 255 / sixth;
            } else if (i >= 6 * sixth) {
                array[i] = 0;
            }
        }
        for (int i = 0; i < rgb.length; i++) {
            int r = array[(i + 4 * sixth) % rgb.length];
            int g = array[(i + 2 * sixth) % rgb.length];
            int b = array[i];
            rgb[i] = new Color(r, g, b);
        }
        bw = new Color[256];
        for (int i = 0; i < bw.length; i++) {
            int x = i * 255 / bw.length;
            bw[i] = new Color(x, x, x);
        }
        colors = new Color[] {
            Color.red,
            Color.green,
            Color.blue,
            Color.cyan,
            Color.magenta,
            Color.yellow,
            Color.orange,
            Color.pink,
            Color.black,
            Color.darkGray,
            Color.gray,
            Color.lightGray,
            Color.white
        };
        shades = new Color[] {
            new Color(0xAA0000).darker(),
            new Color(0xAA0000),
            new Color(0xAA0000).brighter(),
            new Color(0x00AA00).darker(),
            new Color(0x00AA00),
            new Color(0x00AA00).brighter(),
            new Color(0x0000AA).darker(),
            new Color(0x0000AA),
            new Color(0x0000AA).brighter(),
            new Color(0xAAAAAA).darker(),
            new Color(0xAAAAAA),
            new Color(0xAAAAAA).brighter()
        };
        alphas = new Color[] {
            new Color(1.0f, 0.0f, 0.0f, 0.0f),
            new Color(1.0f, 0.0f, 0.0f, 0.1f),
            new Color(1.0f, 0.0f, 0.0f, 0.2f),
            new Color(1.0f, 0.0f, 0.0f, 0.3f),
            new Color(1.0f, 0.0f, 0.0f, 0.4f),
            new Color(1.0f, 0.0f, 0.0f, 0.5f),
            new Color(1.0f, 0.0f, 0.0f, 0.6f),
            new Color(1.0f, 0.0f, 0.0f, 0.7f),
            new Color(1.0f, 0.0f, 0.0f, 0.8f),
            new Color(1.0f, 0.0f, 0.0f, 0.9f),
            new Color(1.0f, 0.0f, 0.0f, 1.0f),
            new Color(0.0f, 1.0f, 0.0f, 0.0f),
            new Color(0.0f, 1.0f, 0.0f, 0.1f),
            new Color(0.0f, 1.0f, 0.0f, 0.2f),
            new Color(0.0f, 1.0f, 0.0f, 0.3f),
            new Color(0.0f, 1.0f, 0.0f, 0.4f),
            new Color(0.0f, 1.0f, 0.0f, 0.5f),
            new Color(0.0f, 1.0f, 0.0f, 0.6f),
            new Color(0.0f, 1.0f, 0.0f, 0.7f),
            new Color(0.0f, 1.0f, 0.0f, 0.8f),
            new Color(0.0f, 1.0f, 0.0f, 0.9f),
            new Color(0.0f, 1.0f, 0.0f, 1.0f),
            new Color(0.0f, 0.0f, 1.0f, 0.0f),
            new Color(0.0f, 0.0f, 1.0f, 0.1f),
            new Color(0.0f, 0.0f, 1.0f, 0.2f),
            new Color(0.0f, 0.0f, 1.0f, 0.3f),
            new Color(0.0f, 0.0f, 1.0f, 0.4f),
            new Color(0.0f, 0.0f, 1.0f, 0.5f),
            new Color(0.0f, 0.0f, 1.0f, 0.6f),
            new Color(0.0f, 0.0f, 1.0f, 0.7f),
            new Color(0.0f, 0.0f, 1.0f, 0.8f),
            new Color(0.0f, 0.0f, 1.0f, 0.9f),
            new Color(0.0f, 0.0f, 1.0f, 1.0f),
            new Color(0.0f, 0.0f, 0.0f, 0.0f),
            new Color(0.0f, 0.0f, 0.0f, 0.1f),
            new Color(0.0f, 0.0f, 0.0f, 0.2f),
            new Color(0.0f, 0.0f, 0.0f, 0.3f),
            new Color(0.0f, 0.0f, 0.0f, 0.4f),
            new Color(0.0f, 0.0f, 0.0f, 0.5f),
            new Color(0.0f, 0.0f, 0.0f, 0.6f),
            new Color(0.0f, 0.0f, 0.0f, 0.7f),
            new Color(0.0f, 0.0f, 0.0f, 0.8f),
            new Color(0.0f, 0.0f, 0.0f, 0.9f),
            new Color(0.0f, 0.0f, 0.0f, 1.0f),
            new Color(0.5f, 0.5f, 0.5f, 0.0f),
            new Color(0.5f, 0.5f, 0.5f, 0.1f),
            new Color(0.5f, 0.5f, 0.5f, 0.2f),
            new Color(0.5f, 0.5f, 0.5f, 0.3f),
            new Color(0.5f, 0.5f, 0.5f, 0.4f),
            new Color(0.5f, 0.5f, 0.5f, 0.5f),
            new Color(0.5f, 0.5f, 0.5f, 0.6f),
            new Color(0.5f, 0.5f, 0.5f, 0.7f),
            new Color(0.5f, 0.5f, 0.5f, 0.8f),
            new Color(0.5f, 0.5f, 0.5f, 0.9f),
            new Color(0.5f, 0.5f, 0.5f, 1.0f),
            new Color(1.0f, 1.0f, 1.0f, 0.0f),
            new Color(1.0f, 1.0f, 1.0f, 0.1f),
            new Color(1.0f, 1.0f, 1.0f, 0.2f),
            new Color(1.0f, 1.0f, 1.0f, 0.3f),
            new Color(1.0f, 1.0f, 1.0f, 0.4f),
            new Color(1.0f, 1.0f, 1.0f, 0.5f),
            new Color(1.0f, 1.0f, 1.0f, 0.6f),
            new Color(1.0f, 1.0f, 1.0f, 0.7f),
            new Color(1.0f, 1.0f, 1.0f, 0.8f),
            new Color(1.0f, 1.0f, 1.0f, 0.9f),
            new Color(1.0f, 1.0f, 1.0f, 1.0f)
        };

        systemColors = new Color[] {
            SystemColor.activeCaption,
            SystemColor.activeCaptionBorder,
            SystemColor.activeCaptionText,
            SystemColor.control,
            SystemColor.controlDkShadow,
            SystemColor.controlHighlight,
            SystemColor.controlShadow,
            SystemColor.controlText,
            SystemColor.desktop,
            SystemColor.inactiveCaption,
            SystemColor.inactiveCaptionBorder,
            SystemColor.inactiveCaptionText,
            SystemColor.info,
            SystemColor.infoText,
            SystemColor.menu,
            SystemColor.menuText,
            SystemColor.scrollbar,
            SystemColor.text,
            SystemColor.textHighlight,
            SystemColor.textHighlightText,
            SystemColor.textInactiveText,
            SystemColor.textText,
            SystemColor.window,
            SystemColor.windowBorder,
            SystemColor.windowText
        };
        systemColorNames = new String[] {
            "activeCaption",
            "activeCaptionBorder",
            "activeCaptionText",
            "control",
            "controlDkShadow",
            "controlHighlight",
            "controlShadow",
            "controlText",
            "desktop",
            "inactiveCaption",
            "inactiveCaptionBorder",
            "inactiveCaptionText",
            "info",
            "infoText",
            "menu",
            "menuText",
            "scrollbar",
            "text",
            "textHighlight",
            "textHighlightText",
            "textInactiveText",
            "textText",
            "window",
            "windowBorder",
            "windowText"
        };
    }
}

