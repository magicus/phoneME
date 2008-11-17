/*
 * @(#)NullLayoutDemo.java	1.5 06/10/10
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

public class NullLayoutDemo extends Demo {
    public NullLayoutDemo() {
        setLayout(null);
        DemoButton b1 = new DemoButton("1");
        DemoButton b2 = new DemoButton("2");
        DemoButton b3 = new DemoButton("three");
        DemoButton b4 = new DemoButton("4");
        DemoButton b5 = new DemoButton("5");
        DemoButton b6 = new DemoButton("6");
        DemoButton b7 = new DemoButton("seven");
        add(b1);
        add(b2);
        add(b3);
        add(b4);
        add(b5);
        add(b6);
        add(b7);
        b1.setBounds( 5,  5,  30, 20);
        b2.setBounds(40,  5,  30, 16);
        b3.setBounds(75,  5,  10, 80);
        b4.setBounds(75, 55,  30, 20);
        b5.setBounds(40, 55,  30, 20);
        b6.setBounds( 5, 55,  30, 20);
        b7.setBounds( 0, 30, 120, 20);
    }
}

