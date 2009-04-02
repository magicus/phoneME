/*
 *
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

package com.sun.ams.ui;


import javax.microedition.lcdui.List;
import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Displayable;
import javax.microedition.midlet.MIDlet;




public class TestMIDlet extends MIDlet implements CommandListener {
    private Display display;
    private List    testList;
    private Command exitCommand = new Command("Exit", Command.EXIT, 1);
    private Command runTestCommand = new Command("Run", Command.ITEM, 1);

    private static String getLabel(String testClassname) {
        return testClassname.substring(testClassname.lastIndexOf('.') + 1);
    }

    public TestMIDlet() {
        display     = Display.getDisplay(this);
        testList    = new List("Screens", List.IMPLICIT);

        BaseTest.screens = new ScreenStack(display);

        testList.setSelectCommand(runTestCommand);
        testList.addCommand(exitCommand);
        testList.setCommandListener(this);

        for (int i = 0; i != TestClassList.names.length; ++i) {
            testList.append(getLabel(TestClassList.names[i]), null);
        }
    }

    public void startApp() {
        display.setCurrent(testList);
    }

    public void pauseApp() {
    }

    public void destroyApp(boolean unconditional) {
    }

    public void commandAction(Command c, Displayable s) {
        if (c == exitCommand) {
            destroyApp(true);
            notifyDestroyed();
        } else if (c == runTestCommand) {
            String testClass = TestClassList.names[testList.getSelectedIndex()];
            try {
                Class.forName(testClass).newInstance();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }
}
