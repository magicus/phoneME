/*
 * @(#)DemoFrame.java	1.5 06/10/10
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
package basis;

import java.awt.*;
import java.awt.event.*;
import java.util.ArrayList;

public class DemoFrame extends Frame {
    protected static final int DEFAULT_WIDTH = 240;
    protected static final int DEFAULT_HEIGHT = 320;
    protected static String className = "basis.DemoFrame";
    protected static String title = "J2ME Basis";
    protected static ArrayList demos = new ArrayList();
    protected static Dimension size;
    protected Builder builder;

    static {
        demos.add("basis.demos.GraphicsDemo");
        demos.add("basis.demos.FontDemo");
        demos.add("basis.demos.EventDemo");
        demos.add("basis.demos.MiscDemo");
        demos.add("basis.demos.GameDemo");
    }

    public static void main(String[] args) throws Exception {
        parse(args);
        Frame frame = new DemoFrame(demos, size);
        frame.setVisible(true);
    }

    protected static void parse(String[] args) {
        int width = DEFAULT_WIDTH;
        int height = DEFAULT_HEIGHT;
        if (args.length > 0) {
            for (int i = 0; i < args.length; i++) {
                if (args[i].equals("-d")) {
                    demos = new ArrayList();
                    demos.add(args[++i]);
                    continue;
                }
                if (args[i].equals("-w")) {
                    try {
                        width = Integer.parseInt(args[++i]);
                    } catch (NumberFormatException nfe) {
                        usage();
                        System.out.println("Invalid width: " + args[i]);
                        System.out.println("");
                        System.exit(1);
                    }
                    continue;
                }
                if (args[i].equals("-h")) {
                    try {
                        height = Integer.parseInt(args[++i]);
                    } catch (NumberFormatException nfe) {
                        usage();
                        System.out.println("Invalid height: " + args[i]);
                        System.out.println("");
                        System.exit(1);
                    }
                    continue;
                }
                usage();
                System.exit(1);
            }
        }
        size = new Dimension(width, height);
    }

    protected static void usage() {
        System.out.println("");
        System.out.println("USAGE:");
        System.out.println("    cvm " + className + " [-d demo] [-w width] [-h height]");
        System.out.println("");
        System.out.println("WHERE:");
        System.out.println("    demo = Full package name of specific demo");
        System.out.println("    width = Width of demo in pixels");
        System.out.println("    height = Height of demo in pixels");
        System.out.println("");
    }

    public DemoFrame(ArrayList demos, Dimension size) throws Exception {
        builder = new Builder(demos);
        builder.build(this);
        pack();
        setSize(size);
        Toolkit toolkit = getToolkit();
        Dimension screen = toolkit.getScreenSize();
        setLocation((screen.width - size.width) / 2, (screen.height - size.height) / 3);
        setTitle(title);
        addWindowListener(new WindowAdapter() {
                public void windowClosing(WindowEvent e) {
                    System.exit(0);
                }
            }
        );
    }
}

