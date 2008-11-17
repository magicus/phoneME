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

//
// Checks the InputEvents/FocusEvent delivery for 2 xlets
// to see whether events are delivered by the correct ThreadGroup.
// Since this uses Robot, make sure to provide additional perms
// to the test run (Ex: -Djava.security.policy=allperm.policy)
//

import com.sun.xlet.XletManager;
import com.sun.xlet.XletLifecycleHandler;

import java.io.IOException;

public class TestRunner {
    public static void main(String args[]) {

        XletLifecycleHandler xlet1 = null;
        XletLifecycleHandler xlet2 = null;

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
               "TestXlet1Auto", path, null);
            System.out.println("TestRunner created xlet 1");
            xlet2 = XletManager.createXlet(
               "TestXlet2Auto", path, null);
            System.out.println("Created xlet 2");
        } catch (IOException ex) {
            ex.printStackTrace();
            System.exit(1);
        }

        System.out.println("TestRunner initializing xlet 1");
        xlet1.postInitXlet();
        System.out.println("TestRunner starting xlet 1");
        xlet1.postStartXlet();
        sleep(3000);

        if (xlet1.getState() == XletLifecycleHandler.DESTROYED) {
           System.out.println("Test failed");
           System.exit(1);          
        }

        System.out.println("\nTestRunner initializing xlet 2");
        xlet2.postInitXlet();
        System.out.println("TestRunner starting xlet 2");
        xlet2.postStartXlet();
        sleep(3000);

        if (xlet2.getState() == XletLifecycleHandler.DESTROYED) {
           System.out.println("Test failed");
           System.exit(1);          
        }

        System.out.println("\nTestRunner pausing xlet 1");
        xlet1.postPauseXlet();
        sleep(500);

        System.out.println("TestRunner restarting xlet 1");
        xlet1.postStartXlet();
        sleep(3000);

        if (xlet1.getState() == XletLifecycleHandler.DESTROYED) {
           System.out.println("Test failed");
           System.exit(1);          
        }

        System.out.println("\nTestRunner destroying xlet 1");
        xlet1.postDestroyXlet(true);
        System.out.println("TestRunner destroying xlet 2");
        xlet2.postDestroyXlet(true);

        sleep(1000);
        System.out.println("Done testing.");
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
