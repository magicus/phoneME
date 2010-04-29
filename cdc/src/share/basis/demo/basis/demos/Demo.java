/*
 * @(#)Demo.java	1.5 06/10/10
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

public class Demo extends Container {
    public void paint(Graphics g) {
        g.setColor(Color.black);
        Dimension d = getSize();
        g.drawRect(0, 0, d.width - 1, d.height - 1);
        // need this to display buttons in layout demos
        super.paint(g);
    }


    protected void setStatus(String text) {
        Container parent = getParent();
        while (parent != null) {
            if (parent instanceof Builder) {
                Builder builder = (Builder) parent;
                builder.setStatus(text);
                return;
            }
            parent = parent.getParent();
        }
    }
    protected String getStatus() {
        Container parent = getParent();
        while (parent != null) {
            if (parent instanceof Builder) {
                Builder builder = (Builder) parent;
                return builder.getStatus();
            }
            parent = parent.getParent();
        }
        return null;
    }
}

