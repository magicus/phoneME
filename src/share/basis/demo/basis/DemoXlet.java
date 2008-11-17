/*
 * @(#)DemoXlet.java	1.5 06/10/10
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
import java.util.ArrayList;
import javax.microedition.xlet.*;

public class DemoXlet implements Xlet {
    protected static ArrayList demos = new ArrayList();
    private XletContext context;
    private Container container;
    private Builder builder;

    static {
        demos.add("basis.demos.GraphicsDemo");
        demos.add("basis.demos.FontDemo");
        demos.add("basis.demos.EventDemo");
        demos.add("basis.demos.MiscDemo");
        demos.add("basis.demos.GameDemo");
    }

    public static void main(String[] args) throws Exception {
        System.out.println("");
        System.out.println("USAGE:");
        System.out.println("    cvm com.sun.xlet.XletRunner -name basis.DemoXlet -path .");
        System.out.println("");
        System.exit(1);
    }

    public DemoXlet() {
        builder = new Builder(demos);
     }

    public void initXlet(XletContext context) {
        System.out.println("initXlet");
        this.context = context;
        try {
            container = context.getContainer();
            builder.build(container);
            Dimension d = container.getSize();
            d.width = d.width > 240 ? d.width : 240;
            d.height = d.height > 320 ? d.height : 320;
            container.setSize(d.width, d.height);
            container.validate();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void startXlet() {
        System.out.println("startXlet");
        container.setVisible(true);
    }

    public void pauseXlet() {
        System.out.println("pauseXlet");
    }

    public void destroyXlet(boolean unconditional) {
        System.out.println("destroyXlet");
    }
}
