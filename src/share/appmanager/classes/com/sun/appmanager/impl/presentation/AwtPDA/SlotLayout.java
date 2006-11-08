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

/*
 * @(#)SlotLayout.java	1.3 05/10/20
 */

package com.sun.appmanager.impl.presentation.AwtPDA;

import java.awt.*;

class SlotLayout
    implements LayoutManager {

    public SlotLayout() {
    }

    public void layoutContainer(Container parent) {
        Insets insets = parent.getInsets();
        int numComponents = parent.getComponentCount();
        int x = 0;
        int y = insets.top;

        for (int i = 0; i < numComponents; i++) {
            Component child = parent.getComponent(i);
            Dimension prefSize = child.getPreferredSize();
            x = insets.left;
            child.setBounds(x, y, prefSize.width, prefSize.height);
            y += prefSize.height;
        }
    }

    public Dimension preferredLayoutSize(Container parent) {
        return layoutSize(parent);
    }

    public Dimension minimumLayoutSize(Container parent) {
        return layoutSize(parent);
    }

    protected Dimension layoutSize(Container parent) {
        int numComponents = parent.getComponentCount();
        Insets insets = parent.getInsets();
        int width = 0;
        int height = 0;

        for (int i = 0; i < numComponents; i++) {
            Component child = parent.getComponent(i);
            Dimension d = child.getPreferredSize();
            if (d.width > width) {
                width = d.width;
            }
            height += d.height;
        }

        width += insets.left + insets.right;
        height += insets.top + insets.bottom;
        return new Dimension(width, height);
    }

    public void addLayoutComponent(String constraint, Component comp) {}

    public void removeLayoutComponent(Component comp) {}

}
