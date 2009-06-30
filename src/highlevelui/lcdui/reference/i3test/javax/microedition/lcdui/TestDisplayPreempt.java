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

import com.sun.midp.i3test.*;

import javax.microedition.midlet.*;
import com.sun.midp.midlet.MIDletStateHandler;
import com.sun.midp.util.DummyMIDlet;
import com.sun.midp.util.DummyMIDlet2;
import com.sun.midp.lcdui.*;

/**
 * This test is designed to verify values returned by Displayable.isShown.
 * According to the spec isShown should be true only 
 * Displayable is visible in the foreground and no system screen/menu
 * is up.
 */
public class TestDisplayPreempt extends TestCase {

    MIDletStateHandler msHandler;
    DisplayEventHandlerImpl deHandler;
    MIDlet midlet[] = new MIDlet[2];
    Display display[] = new Display[2];
    Display preempt = null;

    SystemAlert A[][] = new SystemAlert[2][2];
    Form F[][] = new Form[2][2];
    Command C[][][] = new Command[2][2][3];
        
    boolean isShown(Display d, Displayable s) {
        return d.isShown(s.displayableLF);
    }
            
    public static void sleep(int time) {
        try {
            Thread.sleep(time);
        } catch (Exception e) {}
    }

    public void runTests() {
        int i,j;

        for (i=0; i<2; i++ ) {
            for (j=0; j<2; j++ ) {
                String suffix = "_" + i + "_" + j;
                //A[i][j] = new Alert("A" + suffix);
                F[i][j] = new Form("F" + suffix);
                
                C[i][j][0] = new Command("SCREEN" + suffix, Command.SCREEN, 1);
                C[i][j][1] = new Command("BACK" + suffix, Command.BACK, 1);
                C[i][j][2] = new Command("ITEM" + suffix, Command.ITEM, 1);
                
                F[i][j].addCommand(C[i][j][0]);
                F[i][j].addCommand(C[i][j][1]);
                F[i][j].addCommand(C[i][j][2]);
            }
        }

        msHandler = MIDletStateHandler.getMidletStateHandler();
        deHandler = Display.displayEventHandlerImpl;

        // -------------------------------------------------------
        
        test3();
        test1();
        test2();

        // -------------------------------------------------------
        
    }
    
    /**
     * starts DummyMIDlet 
     */
    protected void startMIDlet0() {
        try {
            // Start a new instance of DummyMIDlet
            msHandler.startMIDlet("com.sun.midp.util.DummyMIDlet",
                                  "DummyMIDlet");
            
            // Wait for async request to be processed 
            while (DummyMIDlet.midlet == null) {
                try { Thread.sleep(100); } catch (InterruptedException ie) {}
            }
            midlet[0] = DummyMIDlet.midlet;
            display[0] = Display.getDisplay(midlet[0]);
            display[0].setCurrent(null);

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * starts DummyMIDlet2 
     */
    protected void startMIDlet1() {
        try {
            // Start a new instance of DummyMIDlet
            msHandler.startMIDlet("com.sun.midp.util.DummyMIDlet2",
                                  "DummyMIDlet2");
            
            // Wait for async request to be processed 
            while (DummyMIDlet2.midlet == null) {
                try { Thread.sleep(100); } catch (InterruptedException ie) {}
            }
            midlet[1] = DummyMIDlet2.midlet;
            display[1] = Display.getDisplay(midlet[1]);
            display[1].setCurrent(null);

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Terminates DummyMIDlet
     */
    protected void endMIDlet0() {
        midlet[0].notifyDestroyed();
	// Since we are calling MIDlet.notifyDestroyed(), MIDlet.destroyApp()
	// will not be called by System. So make it up here.
	try {
	    ((DummyMIDlet)midlet[0]).destroyApp(true);
	} catch (MIDletStateChangeException ignore) {}
    }
    
    /**
     * Terminates DummyMIDlet2
     */
    protected void endMIDlet1() {
        midlet[1].notifyDestroyed();
	// Since we are calling MIDlet.notifyDestroyed(), MIDlet.destroyApp()
	// will not be called by System. So make it up here.
	try {
	    ((DummyMIDlet2)midlet[1]).destroyApp(true);
	} catch (MIDletStateChangeException ignore) {}
    }
    /**
     * test checks how SystemAlert preempts headless midlet
     */
    protected void test1() {
        declare("TestDisplayPreempt 1");
        
        // start midlet w/o screens - no modal dialogs started 
        startMIDlet0();
        display[0].setCurrent(null);
        midlet[0].resumeRequest();
        sleep(500);
        // modal dialog not yet exists - empty screen is in background
        assertFalse("MIDlet before Alert : Display 0 has foreground", display[0].hasForeground);
        sleep(500);
        // create modal dialog - midlet w/o screens exists
        A[0][1] = new SystemAlert(deHandler, "A_0_1", "Alert with MIDlet", null, AlertType.INFO);
        A[0][1].run();
        A[0][1].setTimeout(1500);
        // preempt = deHandler.preemptingDisplay.getDisplay();
        sleep(500);
        // modal dialog is active ... - empty screen is hidden 
        // assertTrue("MIDlet with Alert : Preempt Display has foreground", preempt.hasForeground);
        assertFalse("MIDlet with Alert : Display 0 has foreground", display[0].hasForeground);
        sleep(1500);
        // modal dialog has gone ... - empty screen appears in background again 
        // assertFalse("MIDlet after Alert : Preempt Display has foreground", preempt.hasForeground);
        assertFalse("MIDlet after Alert : Display 0 has foreground", display[0].hasForeground);

        // destroy midlet
        display[0].setCurrent(null);
        sleep(500);
        endMIDlet0();
    }
    
    /**
     * test checks how SystemAlert preempts normal midlet (with screens)
     */
    protected void test2() {
        declare("TestDisplayPreempt 2");
        
        // start midlet with screens - no modal dialogs started
        startMIDlet1();
        display[1].setCurrent(F[1][0]);
        midlet[1].resumeRequest();
        sleep(500);
        // modal dialog not yet exists - midlet screen is active 
        assertTrue("MIDlet before Alert : Display 1 has foreground", display[1].hasForeground);
        assertTrue("MIDlet before Alert : F 1 0 is shown", isShown(display[1], F[1][0]));
        sleep(500);
        // create modal dialog - midlet with screens exists
        A[1][1] = new SystemAlert(deHandler, "A_1_1", "Alert with MIDlet", null, AlertType.INFO);
        A[1][1].run();
        A[1][1].setTimeout(1500);
        // preempt = deHandler.preemptingDisplay.getDisplay();
        sleep(500);
        // assertTrue("MIDlet with Alert : Preempt Display has foreground", preempt.hasForeground);
        assertFalse("MIDlet with Alert : Display 0 has foreground", display[1].hasForeground);
        assertFalse("MIDlet with Alert : F 1 0 is shown", isShown(display[1], F[1][0]));
        // while modal dialog is active - change current midlet screen
        display[1].setCurrent(F[1][1]);
        sleep(500);
        // modal dialog is active ... - midlet screen is hidden 
        // assertTrue("Alert with MIDlet : Preempt Display has foreground", preempt.hasForeground);
        assertFalse("Alert with MIDlet : Display 1 has foreground", display[1].hasForeground);
        assertFalse("Alert with MIDlet : F 1 0 is shown", isShown(display[1], F[1][0]));
        assertFalse("Alert with MIDlet : F 1 1 is shown", isShown(display[1], F[1][1]));
        sleep(1500);
        // modal dialog has gone ... - midlet screen appears again 
        // assertFalse("MIDlet after Alert : Preempt Display has foreground", preempt.hasForeground);
        assertTrue("MIDlet after Alert : Display 0 has foreground", display[1].hasForeground);
        assertFalse("MIDlet after Alert : F 1 0 is shown", isShown(display[1], F[1][0]));
        assertTrue("MIDlet after Alert : F 1 1 is shown", isShown(display[1], F[1][1]));

        display[0].setCurrent(null);
        sleep(500);
        endMIDlet1();
    }
    
    /**
     * test checks how systemAlert w/o midlets works
     */
    protected void test3() {
        declare("TestDisplayPreempt 3");
        
        // create modal dialog (no midlets exist)
        A[0][0] = new SystemAlert(deHandler, "A_0_0", "Alert w/o MIDlet", null, AlertType.WARNING);
        A[0][0].run();
        A[0][0].setTimeout(1000);
        // preempt = deHandler.preemptingDisplay.getDisplay();
        sleep(500);
        // dialog is active 
        // assertTrue("Alert w/o MIDlet : Preempt Display has foreground", display[1].hasForeground);
        sleep(1000);
        // dialog has gone !
        // assertFalse("Alert w/o MIDlet : Preempt Display has foreground", display[1].hasForeground);
        
        sleep(500);
        
        assertTrue(true);  
    }
}
