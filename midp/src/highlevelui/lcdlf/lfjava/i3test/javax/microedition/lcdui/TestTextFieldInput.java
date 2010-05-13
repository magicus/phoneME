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

import com.sun.midp.chameleon.*;
import com.sun.midp.chameleon.input.*;
import java.util.Vector;

import com.sun.midp.events.*;
import com.sun.midp.lcdui.EventConstants;
import com.sun.midp.configurator.Constants;
import com.sun.midp.util.SerialCallback;

/**
 * This test case covers Input Session management code in TextField and
 * TextBox. 
 * Bugs it tests against is :
 *         6297297 - IllegalStateException in event processing thread
 */
public class TestTextFieldInput extends TestCase {
    /** instance of display */
    Display display;
    /** callback serialization adaptor */
    SerialCallback scb;
    /** instance of form */
    Form form;
    /** instance of text field */
    TextField tf;
    /** sample of the long text string */ 
    static final String longtext = "some" +
        " long long long long long long long long" +
                " long long long long long long long long" +
                " long long long long long long long long" +
                " text";


    /**
     * Checks the command in Menu
     * 
     * @param c Command
     * @return true if the command is in menu, false otherwise
     */
    boolean isCommandInMenu(Command c) {
        Command[] soft2 = display.getWindow().getSoftTwo();
        if (soft2 == null) {
            return false;
        }
        for (int i = 0; i < soft2.length; i++) {
            if (soft2[i] == c) {
                return true;
            }
        }
        return false;  
    }
    

    /**
     * Checks the input states
     */
    void verifyInputStates() {
/*
        TextFieldLFImpl lf = (TextFieldLFImpl)tf.textFieldLF;
        if (lf.hasFocus) {
            if (lf.editable) {
                // Input mode indicator layer exists
                assertSame("Indicator layer", 
                           display.getWindow().getTopMostPopup(),
                           lf.inputModeIndicator);
                
                // Input session should have been started
                assertTrue("Input session",
                           lf.inputSession.getCurrentInputMode() != null);
                
                // At least one input mode available for constaint ANY
                assertTrue("Available IMs",
                           lf.inputSession.getAvailableModes().length >= 1);
                
                // Input mode selection command exists in menu
                assertTrue("InputSubMenu", isCommandInMenu(lf.inputMenu));
                
                // cursor is visible
                assertTrue("Cursor", lf.cursor.visible);
                
                // No active autoscrolling task
                assertTrue("AutoScrolling", lf.textScrollPainter == null);
                
            } else {
                // cursor is invisible
                assertTrue("Cursor", !lf.cursor.visible);
                
                // if contents too long, autoscrolling should be activated
                assertTrue("AutoScrolling",
                           lf.textWidth <= lf.scrollWidth ||
                           lf.textScrollPainter != null);
                
                // input session is ended
                assertTrue("Input session",
                           lf.inputSession.getCurrentInputMode() == null);
                
                // Input mode selection command doesn't exist in menu
                assertTrue("InputSubMenu", !isCommandInMenu(lf.inputMenu));
            }
        } else {
            // autoscrolling is not activated
            assertTrue("AutoScrolling", lf.textScrollPainter == null);
            
            // input mode selection command doesn't exist in menu
            assertTrue("InputSubMenu", !isCommandInMenu(lf.inputMenu));
        }
  */         
    }
    
    /**
     * Verifies available input modes
     * @param isNumericOnly if only numeric only, false otherwise  
     */
    void verifyAvailableInputModes(boolean isNumericOnly) {
        TextFieldLFImpl lf = (TextFieldLFImpl)tf.textFieldLF;
        Command[] cmds = lf.inputMenu.getSubCommands();
        boolean hasAlpha = false, hasNumeric = false;
        AlphaNumericInputMode alphaIM = new AlphaNumericInputMode();
        NumericInputMode numericIM = new NumericInputMode();
        
        if (cmds != null) {
            for (int i = 0; i < cmds.length; i++) {
                if (alphaIM.getName().equals(cmds[i].getLabel())) {
                    hasAlpha = true;
                } else if (numericIM.getName().equals(cmds[i].getLabel())) {
                    hasNumeric = true;
                }
            }
        }
        
        assertTrue("Numeric IM", hasNumeric);
        assertTrue("AlphaNumeric IM", isNumericOnly || hasAlpha);
    }
    
    /**
     * Starts the test
     */
    void setUp() {
        display = new StubDisplay();
        scb = new SerialCallback(display);
        
        form = new Form("Form");
        tf = new TextField("TextField", longtext, 256, TextField.ANY);
        form.append(tf);

        form.append(new DateField("DateField", DateField.TIME));
    }
    
    /**
     * Show the form on the display
     */
    void testShow() {
        declare("testShow");
        
        // Make form shown
        display.setCurrent(form);
        scb.invokeAndWait();
        assertTrue(form.isShown());
        
        // Expect textfield1 in focus
        verifyInputStates();
        // Expect both AlphaNumeric and Numeric input modes are available
        verifyAvailableInputModes(false);
    }
    
    /**
     * Tests setConstraints method
     */
    void testSetConstraints() {
        declare("testSetUneditable");
        
        tf.setConstraints(TextField.ANY | TextField.UNEDITABLE);
        verifyInputStates();
        
        declare("testSetNumeric");
        
        // Change input constraints from ANY to NUMERIC editable
        tf.setConstraints(TextField.NUMERIC);
        verifyInputStates();
        // Expect AlphaNumeric input mode does not exist
        verifyAvailableInputModes(true);
    }
    
    /**
     * Tests traverse in/ traverse out
     */
    void testTraverse() {
        EventQueue eventQueue = EventQueue.getEventQueue();
        NativeEvent event;

        // Restore textfield to ANY editable
        tf.setConstraints(TextField.ANY);
        tf.setString(longtext);

        FormLFImpl formlf = (FormLFImpl)form.formLF;
        TextFieldLFImpl tfLF = (TextFieldLFImpl)tf.textFieldLF;
        
        declare("testTraverseOut");
        event = new NativeEvent(EventTypes.KEY_EVENT);
        event.intParam1 = EventConstants.PRESSED;
        event.intParam2 = Constants.KEYCODE_DOWN;
        event.intParam4 = display.displayId;
        eventQueue.post(event);
        
        // wait till the focus is transferred
        scb.invokeAndWait();

        assertFalse(tfLF.hasFocus);
        verifyInputStates();
        
        declare("testTraverseIn");
        event = new NativeEvent(EventTypes.KEY_EVENT);
        event.intParam1 = EventConstants.PRESSED;
        event.intParam2 = Constants.KEYCODE_UP;
        event.intParam4 = display.displayId;
        eventQueue.post(event);

        // wait till the focus is transferred
        scb.invokeAndWait();

        assertTrue(tfLF.hasFocus);
        verifyInputStates();
    }
    
    /**
     * destroy test
     */
    void tearDown() {
    }
    
    /**
     * Overridden from TestCase parent. This method will kick off each
     * individual test
     */
    public void runTests() {
        setUp();
        
        testShow();
        testSetConstraints();
        testTraverse();

        tearDown();
    }
}
