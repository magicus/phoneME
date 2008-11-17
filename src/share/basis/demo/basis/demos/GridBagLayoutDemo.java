/*
 * @(#)GridBagLayoutDemo.java	1.5 06/10/10
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
import basis.DemoButton;

public class GridBagLayoutDemo extends Demo {
    private GridBagLayout gbl;
    private GridBagConstraints gbc;

    public GridBagLayoutDemo() {
        gbl = new GridBagLayout();
        gbc = new GridBagConstraints();
        setLayout(gbl);
        int cols = 7;
        double weightx = 1.0;
        double weighty = 0.0;
        for (int x = 0; x < cols; x++) {
            DemoButton b = new DemoButton("" + x);
            add(b, x, 0, 1, 1, GridBagConstraints.BOTH, 0, 0, new Insets(0, 0, 0, 0), GridBagConstraints.CENTER, weightx, weighty);
        }
        DemoButton aC = new DemoButton("x");
        DemoButton aN = new DemoButton("x");
        DemoButton aS = new DemoButton("x");
        DemoButton aE = new DemoButton("x");
        DemoButton aW = new DemoButton("x");
        DemoButton aNW = new DemoButton("x");
        DemoButton aNE = new DemoButton("x");
        DemoButton aSE = new DemoButton("x");
        DemoButton aSW = new DemoButton("x");
        add(aC,  0, 1, 3, 3, GridBagConstraints.NONE, 0, 0, new Insets(0, 0, 0, 0), GridBagConstraints.CENTER, weightx, weighty);
        add(aN,  0, 1, 3, 3, GridBagConstraints.NONE, 0, 0, new Insets(0, 0, 0, 0), GridBagConstraints.NORTH, weightx, weighty);
        add(aS,  0, 1, 3, 3, GridBagConstraints.NONE, 0, 0, new Insets(0, 0, 0, 0), GridBagConstraints.SOUTH, weightx, weighty);
        add(aE,  0, 1, 3, 3, GridBagConstraints.NONE, 0, 0, new Insets(0, 0, 0, 0), GridBagConstraints.EAST, weightx, weighty);
        add(aW,  0, 1, 3, 3, GridBagConstraints.NONE, 0, 0, new Insets(0, 0, 0, 0), GridBagConstraints.WEST, weightx, weighty);
        add(aNW, 0, 1, 3, 3, GridBagConstraints.NONE, 0, 0, new Insets(0, 0, 0, 0), GridBagConstraints.NORTHWEST, weightx, weighty);
        add(aNE, 0, 1, 3, 3, GridBagConstraints.NONE, 0, 0, new Insets(0, 0, 0, 0), GridBagConstraints.NORTHEAST, weightx, weighty);
        add(aSE, 0, 1, 3, 3, GridBagConstraints.NONE, 0, 0, new Insets(0, 0, 0, 0), GridBagConstraints.SOUTHEAST, weightx, weighty);
        add(aSW, 0, 1, 3, 3, GridBagConstraints.NONE, 0, 0, new Insets(0, 0, 0, 0), GridBagConstraints.SOUTHWEST, weightx, weighty);
        DemoButton normal = new DemoButton("n");
        DemoButton padded = new DemoButton("p");
        DemoButton insets = new DemoButton("i");
        add(normal, 3, 1, 1, 1, GridBagConstraints.NONE, 0, 0, new Insets(0, 0, 0, 0), GridBagConstraints.CENTER, weightx, weighty);
        add(padded, 3, 2, 1, 1, GridBagConstraints.NONE, 8, 8, new Insets(0, 0, 0, 0), GridBagConstraints.CENTER, weightx, weighty);
        add(insets, 3, 3, 1, 1, GridBagConstraints.NONE, 0, 0, new Insets(8, 8, 8, 8), GridBagConstraints.CENTER, weightx, weighty);
        DemoButton wide = new DemoButton("two");
        DemoButton remainder = new DemoButton("rem");
        DemoButton relative = new DemoButton("rel");
        add(wide,      4, 1, 2,                            1, GridBagConstraints.HORIZONTAL, 0, 0, new Insets(0, 0, 0, 0), GridBagConstraints.CENTER, weightx, weighty);
        add(remainder, 4, 2, GridBagConstraints.REMAINDER, 1, GridBagConstraints.HORIZONTAL, 0, 0, new Insets(0, 0, 0, 0), GridBagConstraints.CENTER, weightx, weighty);
        add(relative,  4, 3, GridBagConstraints.RELATIVE,  1, GridBagConstraints.HORIZONTAL, 0, 0, new Insets(0, 0, 0, 0), GridBagConstraints.CENTER, weightx, weighty);
    }

    private void add(Component component, int gridx, int gridy,
        int gridwidth, int gridheight, int fill,
        int ipadx, int ipady, Insets insets, int anchor,
        double weightx, double weighty) {
        gbc.gridx = gridx;
        gbc.gridy = gridy;
        gbc.gridwidth = gridwidth;
        gbc.gridheight = gridheight;
        gbc.fill = fill;
        gbc.ipadx = ipadx;
        gbc.ipady = ipady;
        gbc.insets = insets;
        gbc.anchor = anchor;
        gbc.weightx = weightx;
        gbc.weighty = weighty;
        gbl.setConstraints(component, gbc);
        add(component);
    }
}

