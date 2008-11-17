/*
 * @(#)DrawDemo.java	1.5 06/10/10
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
import basis.Builder;

public class DrawDemo extends Demo {
    private int x;
    private int y;
    private int w;
    private int h;
    private int i;

    public void paint(Graphics g) {
        Dimension d = getSize();
        i = (d.width + d.height) / 100;
        // lines
        x = 0;
        y = 0;
        w = d.width / 4 - i;
        h = d.height - 1;
        g.setColor(Builder.SUN_BLUE);
        g.drawLine(x, y, x + w, y + h);
        g.drawLine(x + w / 2, y, x + w / 2, y + h);
        g.drawLine(x + w, y, x, y + h);
        g.drawLine(x, y + h / 2, x + w, y + h / 2);
        // rects
        x = 0;
        x = d.width / 4;
        y = 0;
        w = d.width / 4 - i;
        h = d.height - 1;
        g.setColor(Builder.SUN_BLUE);
        g.drawRect(x, y, w, h);
        adjust();
        g.fillRect(x, y, w, h);
        g.setColor(Builder.SUN_YELLOW);
        adjust();
        g.draw3DRect(x, y, w, h, true);
        adjust();
        g.fill3DRect(x, y, w, h, true);
        g.setColor(Builder.SUN_BLUE);
        adjust();
        g.drawRoundRect(x, y, w, h, 10, 10);
        adjust();
        g.fillRoundRect(x, y, w, h, 10, 10);
        adjust();
        g.clearRect(x, y, w, h);
        // circles
        x = 2 * d.width / 4;
        y = 0;
        w = d.width / 4 - i;
        h = d.height - 1;
        g.setColor(Builder.SUN_BLUE);
        g.drawOval(x, y, w, h);
        adjust();
        g.fillOval(x, y, w, h);
        g.setColor(Builder.SUN_YELLOW);
        adjust();
        g.drawArc(x, y, w, h, 135, 270);
        adjust();
        g.fillArc(x, y, w, h, 135, 270);
        // polys
        x = 3 * d.width / 4;
        y = 0;
        w = d.width / 12 - i;
        h = d.height - 1;
        g.setColor(Builder.SUN_BLUE);
        g.drawPolygon(array(x, x + w, x, x + w), array(y, y + h / 4, y + 3 * h / 4, y + h), 4);
        x = 10 * d.width / 12;
        g.fillPolygon(array(x, x + w, x, x + w), array(y, y + h / 4, y + 3 * h / 4, y + h), 4);
        x = 11 * d.width / 12;
        g.drawPolyline(array(x, x + w, x, x + w), array(y, y + h / 4, y + 3 * h / 4, y + h), 4);
    }

    private void adjust() {
        x += i;
        y += i;
        w -= 2 * i;
        h -= 2 * i;
    }

    private int[] array(int i1, int i2, int i3, int i4) {
        int[] array = {i1, i2, i3, i4};
        return array;
    }
}

