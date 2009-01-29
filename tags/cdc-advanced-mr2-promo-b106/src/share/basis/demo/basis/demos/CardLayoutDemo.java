/*
 * @(#)CardLayoutDemo.java	1.5 06/10/10
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
import java.util.EventObject;
import basis.DemoButton;
import basis.DemoButtonListener;

public class CardLayoutDemo extends Demo implements DemoButtonListener {
    CardLayout cardLayout = new CardLayout();
    DemoButton[] buttons = new DemoButton[] {
        new DemoButton("one"),
        new DemoButton("2"),
        new DemoButton("threethreethree"),
        new DemoButton("four"),
        new DemoButton("five"),
        new DemoButton("six"),
        new DemoButton("seven")
    };

    public CardLayoutDemo() {
        setLayout(cardLayout);
        for (int i = 0; i < buttons.length; i++) {
            buttons[i].addDemoButtonListener(this);
            add(buttons[i], buttons[i].getLabel());
        }
    }

    public void buttonPressed(EventObject e) {
        DemoButton b = (DemoButton) e.getSource();
        for (int i = 0; i < buttons.length; i++) {
            int j = i + 1;
            if (j == buttons.length) {
                j = 0;
            }
            if (b == buttons[i]) {
                b = buttons[j];
                break;
            }
        }
        cardLayout.show(this, b.getLabel());
    }
}
