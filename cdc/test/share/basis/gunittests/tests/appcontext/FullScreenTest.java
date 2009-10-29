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


package tests.appcontext ;

import java.awt.*;                                                          
import java.awt.event.*;
import junit.framework.*;

public class FullScreenTest extends TestCase { 
    public void test_getFullScreenWindow() {
        // note : In PBP, once we create a Frame() instance there is no
        // way to create another, even if we dispose() it, so we are stuck
        // with a single test.

        TestLet testlet = new TestLet() {
            protected Object doTest() {
                Frame f = new Frame() ;
                GraphicsEnvironment ge = 
                    GraphicsEnvironment.getLocalGraphicsEnvironment();
                GraphicsDevice gd = ge.getDefaultScreenDevice();
                gd.setFullScreenWindow(f);
                return gd.getFullScreenWindow();
            }
        };

        // verifies that only code within the same appcontext that set 
        // the fullscreen window can get it
        assertNotNull(testlet.getResult());

        GraphicsEnvironment ge = 
            GraphicsEnvironment.getLocalGraphicsEnvironment();
        GraphicsDevice gd = ge.getDefaultScreenDevice();
        Window w = gd.getFullScreenWindow();

        // verifies that any other appcontext code (system context in the 
        // testcase) cannot get to the fullscreen window
        assertNull(w);
    }
}         

