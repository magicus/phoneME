/*
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

import com.sun.xlet.XletManager;
import com.sun.xlet.XletLifecycleHandler;

import java.io.IOException;

public class TestRunner {
    public static void main(String args[]) {

        XletLifecycleHandler xlet1 = null;
        //XletLifecycleHandler xlet2 = null;

        System.setProperty("package.restrict.access.sun.awt", "false");

        String[] path;
        if (args == null || args.length == 0) {
            System.out.println("TestRunner - no argument given."); 
            System.out.println("Searching for the test in the current directory.");
            path = new String[]{"."};
        } else {
            path = new String[]{args[0]};
        }

        try {
            xlet1 = XletManager.createXlet(
               "ContextClsLoaderXlet", path, null);
            System.out.println("TestRunner created xlet 1");
            //xlet2 = XletManager.createXlet(
            //   "ContextClsLoaderXlet", path, null);
            //System.out.println("Created xlet 2");
        } catch (IOException ex) {
            ex.printStackTrace();
            System.exit(1);
        }

        System.out.println("TestRunner initializing xlet");
        xlet1.postInitXlet();
        System.out.println("TestRunner starting xlet");
        xlet1.postStartXlet();
        sleep(10000);

        System.out.println("TestRunner pausing xlet");
        xlet1.postPauseXlet();
        sleep(1000);

        System.out.println("TestRunner destroying xlet");
        xlet1.postDestroyXlet(true);

        sleep(5000);
        System.out.println("Done testing");
        System.exit(0);
    }

    private static void sleep(long delay) {
        long goal = System.currentTimeMillis() + delay;
        while (delay > 0) {
            try {
                Thread.sleep(1000);
            } catch (InterruptedException ignored) {}
            delay = goal - System.currentTimeMillis();
        }
    }

}
