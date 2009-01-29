/*
 * @(#)DemoApplet.java	1.5 06/10/10
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
package personal;

import java.applet.*;
import java.awt.*;
import java.awt.event.*;
import java.util.ArrayList;
import basis.Builder;

public class DemoApplet extends Applet {
    protected static ArrayList demos = new ArrayList();
    private Builder builder;

    static {
        demos.add("basis.demos.GraphicsDemo");
        demos.add("basis.demos.FontDemo");
        demos.add("basis.demos.EventDemo");
        demos.add("basis.demos.MiscDemo");
        demos.add("basis.demos.GameDemo");
        demos.add("personal.demos.WidgetDemo");
    }

    public static void main(String[] args) throws Exception {
        System.out.println("");
        System.out.println("USAGE:");
        System.out.println("   cvm sun.applet.AppletViewer personal/DemoApplet.html");
        System.out.println("");
        System.exit(1);
    }

    public DemoApplet() {
        builder = new Builder(demos);
    }

    public void init() {
        System.out.println("Applet: init");
        try {
            builder.build(this);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void start() {
        System.out.println("Applet: start");
        builder.showDemo("Color");
    }

    public void stop() {
        System.out.println("Applet: stop");
    }

    public void destroy() {
        System.out.println("Applet: destroy");
    }
}

