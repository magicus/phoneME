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

public class FocusMgmtTest extends TestCase { 

    public void test_getKeyboardFocusManager() {
        GetKFMTestLet testlet1 = new GetKFMTestLet(); 
        GetKFMTestLet testlet2 = new GetKFMTestLet(); 
        assertTrue(testlet1.getResult() != testlet2.getResult());
    }

    static class GetKFMTestLet extends TestLet {
        protected Object doTest() {
            return KeyboardFocusManager.getCurrentKeyboardFocusManager();
        }
    }

    public void test_getActiveWindow() {
        TestLet testlet = new TestLet() {
            protected Object doTest() {
                KeyboardFocusManager kfm = 
                KeyboardFocusManager.getCurrentKeyboardFocusManager();
                return new Boolean(kfm.getActiveWindow() == null);
            }
        };

        // the boolean value indicates if the active window is null (true)
        // or not (false)
        boolean b = ((Boolean)(testlet.getResult())).booleanValue();
        assertTrue(b);
    }
}         

