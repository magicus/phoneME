/*
 *  
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package javax.microedition.lcdui;

import com.sun.midp.chameleon.*;
import com.sun.midp.chameleon.layers.*;
import com.sun.midp.i3test.TestCase;
import com.sun.midp.util.LcduiTestCanvas;
import com.sun.midp.util.LcduiTestMIDlet;

/**
 * This test case does extensive testing on command sorting in chameleon. 
 * It validates the rules specified in the spec for sorting commands
 * in a particular order along with taking their priorities into 
 * consideration. For example, it tests the decision on what goes into the 
 * left soft button and what goes into the right soft button as the system 
 * menu list of commands.
 */
public class TestCommandSorting extends TestCase {

    /** Name of the test */
    static final String testName = "CommandSorting";
    
    /** The Display to use to view our test canvases */
    protected Display display;

    /** Reference to MIDPWindow, to help us gain access into 
     * SoftButtonLayer's attributes used in this test.
     */
    protected MIDPWindow window;
        
    /**
     * Simple constructor.
     */
    public TestCommandSorting() {	
    }
    
    /**
     * Utility method that checks that the left soft button is indeed the
     * intended one as passed in via parameter cmd, after making the canvas 
     * current.
     *
     * @param canvas the canvas being used to test the commands
     * @param cmd the command under test
     */
    protected void checkLeftBtn(LcduiTestCanvas canvas, Command cmd) {
        display.setCurrent(canvas);
        if (!canvas.awaitPaint()) {
            fail("test canvas not visible");
            return;
        }
	
        assertSame("Checking left button:", cmd, window.getSoftOne());
    }

    /**
     * This test is for a canvas with just the STOP command added. The STOP 
     * command should be the left soft button. (only negative commands are 
     * being tested)
     */
    protected void testOne() {
        declare(testName + " 1: STOP command added to the screen.");
        
        LcduiTestCanvas canvas = new LcduiTestCanvas();

        Command stopCmd = new Command("Stop", Command.STOP, 0);
        canvas.addCommand(stopCmd);
    
        checkLeftBtn(canvas, stopCmd);
    }
    
    /**
     * This test is for a canvas with the CANCEL & STOP commands added. The 
     * CANCEL command should be the left soft button. (only negative 
     * commands are being tested)
     */
    protected void testTwo() {
        declare(testName + " 2: CANCEL & STOP commands added to the screen.");
        
        LcduiTestCanvas canvas = new LcduiTestCanvas();

        Command cancelCmd = new Command("Cancel", Command.CANCEL, 0);
        Command stopCmd = new Command("Stop", Command.STOP, 0);
        
        // we add the commands in reverse order so that we are
        // sure the button layer is sorting it correctly
        canvas.addCommand(stopCmd);
        canvas.addCommand(cancelCmd);
    
        checkLeftBtn(canvas, cancelCmd);
    }

    /**
     * This test is for a canvas with the EXIT, CANCEL & STOP commands added.
     * The EXIT command should be the left soft button. (only negative
     * commands are being tested)
     */
    protected void testThree() {
        declare(testName+" 3: EXIT, CANCEL & STOP commands added to the screen.");
        
        LcduiTestCanvas canvas = new LcduiTestCanvas();

        Command exitCmd = new Command("Exit", Command.EXIT, 0);
        Command cancelCmd = new Command("Cancel", Command.CANCEL, 0);
        Command stopCmd = new Command("Stop", Command.STOP, 0);
    
        // we add the commands in reverse order so that we are
        // sure the button layer is sorting it correctly
        canvas.addCommand(stopCmd);
        canvas.addCommand(cancelCmd);
        canvas.addCommand(exitCmd);
        
        checkLeftBtn(canvas, exitCmd);
    }

    /**
     * This test is for a canvas with the BACK, EXIT, CANCEL & STOP
     * commands added. The BACK command should be the left soft button. 
     * (only negative commands are being tested)
     */
    protected void testFour() {
        declare(testName+" 4: BACK, EXIT, CANCEL & STOP commands added to the screen.");
        
        LcduiTestCanvas canvas = new LcduiTestCanvas();

        Command backCmd = new Command("Back", Command.BACK, 0);
        Command exitCmd = new Command("Exit", Command.EXIT, 0);
        Command cancelCmd = new Command("Cancel", Command.CANCEL, 0);
        Command stopCmd = new Command("Stop", Command.STOP, 0);
        
        // we add the commands in reverse order so that we are
        // sure the button layer is sorting it correctly
        canvas.addCommand(stopCmd);
        canvas.addCommand(cancelCmd);
        canvas.addCommand(exitCmd);
        canvas.addCommand(backCmd);
        checkLeftBtn(canvas, backCmd);
    }

    /**
     * This test is for a canvas with the BACK, STOP, CANCEL, EXIT, HELP, 
     * OK, SCREEN & ITEM commands added. The BACK command should be the 
     * left soft button. And all the other commands should be sorted 
     * correctly as defined in the spec on the right soft button as the 
     * system menu. (combination of positive & negative commands are
     * being tested)
     */
    protected void testFive() {
        declare(testName + " 5: BACK, STOP, CANCEL, EXIT, HELP, OK, SCREEN & ITEM commands added to the screen.");
        
        LcduiTestCanvas canvas = new LcduiTestCanvas();
	
        // we add the commands in reverse order so that we are
        // sure the button layer is sorting it correctly
        for (int i = 0; i < 2; i++) {
            canvas.addCommand(new Command("STOP" + i, Command.STOP, i));
            canvas.addCommand(new Command("CANCEL" + i, Command.CANCEL, i));
            canvas.addCommand(new Command("EXIT" + i, Command.EXIT, i));
            canvas.addCommand(new Command("HELP" + i, Command.HELP, i));
            canvas.addCommand(new Command("OK" + i, Command.OK, i));
            canvas.addCommand(new Command("SCREEN" + i, Command.SCREEN, i));
            canvas.addCommand(new Command("ITEM" + i, Command.ITEM, i));
        }
    
        Command backCmd = new Command("Back", Command.BACK, 0);
        canvas.addCommand(backCmd);
    
        checkLeftBtn(canvas, backCmd);
        Command[] cmds = window.getSoftTwo();
        
        assertTrue("ITEM", cmds[0].getCommandType() == Command.ITEM);
        assertTrue("ITEM", cmds[1].getCommandType() == Command.ITEM);
        assertTrue("ITEM PRIORITY", 
                   cmds[0].getPriority() < cmds[1].getPriority());
    
        assertTrue("SCREEN", cmds[2].getCommandType() == Command.SCREEN);
        assertTrue("SCREEN", cmds[3].getCommandType() == Command.SCREEN);
        assertTrue("SCREEN PRIORITY", 
                   cmds[2].getPriority() < cmds[3].getPriority());
    
        assertTrue("OK", cmds[4].getCommandType() == Command.OK);
        assertTrue("OK", cmds[5].getCommandType() == Command.OK);
        assertTrue("OK PRIORITY", 
                   cmds[4].getPriority() < cmds[5].getPriority());
    
        assertTrue("EXIT, ", cmds[6].getCommandType() == Command.EXIT);
        assertTrue("EXIT, ", cmds[7].getCommandType() == Command.EXIT);
        assertTrue("EXIT PRIORITY, ", 
                   cmds[6].getPriority() < cmds[7].getPriority());
    
        assertTrue("CANCEL, ", cmds[8].getCommandType() == Command.CANCEL);
        assertTrue("CANCEL, ", cmds[9].getCommandType() == Command.CANCEL);
        assertTrue("CANCEL PRIORITY, ", 
                   cmds[8].getPriority() < cmds[9].getPriority());
    
        assertTrue("STOP, ", cmds[10].getCommandType() == Command.STOP);
        assertTrue("STOP, ", cmds[11].getCommandType() == Command.STOP);
        assertTrue("STOP PRIORITY, ", 
                   cmds[10].getPriority() < cmds[11].getPriority());
    
        assertTrue("HELP, ", cmds[12].getCommandType() == Command.HELP);
        assertTrue("HELP, ", cmds[13].getCommandType() == Command.HELP);
        assertTrue("HELP PRIORITY, ", 
		           cmds[12].getPriority() < cmds[13].getPriority());
    }

    /**
     * This test is for a canvas with the HELP, OK, SCREEN & ITEM
     * commands added, without any negative commands and check to see
     * if the left soft button is mapped to the highest positive
     * command as expected. All the remaining positive
     * commands should be sorted correctly as defined in the spec on
     * the right soft button as the system menu. (only positive commands are
     * being tested)
     */
    protected void testSix() {
        declare(testName + " 6: HELP, OK, SCREEN & ITEM commands added to the screen.");
        
        LcduiTestCanvas canvas = new LcduiTestCanvas();
	
	Command helpCmd   = new Command("HELP",   Command.HELP,   0);
	Command okCmd     = new Command("OK",     Command.OK,     1);
	Command screenCmd = new Command("SCREEN", Command.SCREEN, 2);
	Command itemCmd   = new Command("ITEM",   Command.ITEM,   3);

	canvas.addCommand(helpCmd);
	canvas.addCommand(okCmd);
	canvas.addCommand(screenCmd);
	canvas.addCommand(itemCmd);

        checkLeftBtn(canvas, itemCmd); // ITEM command has highest priority
    }
    
    
    /**
     * Overridden from TestCase parent. This method will kick off each
     * individual test
     */
    public void runTests() throws Throwable {
        
        if (!LcduiTestMIDlet.invoke()) {
            throw new RuntimeException("can't start LcduiTestMIDlet");
        }

        try {
            display = LcduiTestMIDlet.getDisplay();
            window = display.getWindow();

            testOne();
            testTwo();
            testThree();
            testFour();
            testFive();
            testSix();
        } finally {
            LcduiTestMIDlet.cleanup();
        }
    }
}
