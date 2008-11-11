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
import com.sun.midp.lcdui.EventConstants;

/**
 * Tests for the KeyConverter utility class.
 */
public class TestKeyConverter extends TestCase {

    /**
     * Checks one game action.  Ensures that a keycode exists for every 
     * game action defined by the specification.  Also checks that the keycode 
     * maps back to the same game action.
     */
    void checkGameAction(int ga) {
        int kc = KeyConverter.getKeyCode(ga);
        assertTrue("keycode is 0", kc != 0);

        int ga2 = KeyConverter.getGameAction(kc);
        assertTrue("game action is 0", ga2 != 0);
        assertTrue("game action is -1", ga2 != -1);

        assertEquals("keycode and gameaction not symmetric", ga, ga2);
    }

    /**
     * Tests existence and symmetry of every game action.  For every game
     * action, ensures that it is mapped to a keycode, and then ensures that
     * that keycode maps back to the original game action.  This test is
     * device-independent.
     */
    void testAllGameActions() {
        checkGameAction(Canvas.UP);
        checkGameAction(Canvas.DOWN);
        checkGameAction(Canvas.LEFT);
        checkGameAction(Canvas.RIGHT);
        checkGameAction(Canvas.FIRE);
        checkGameAction(Canvas.GAME_A);
        checkGameAction(Canvas.GAME_B);
        checkGameAction(Canvas.GAME_C);
        checkGameAction(Canvas.GAME_D);
    }

    /**
     * Tests getSystemKey().
     *
     * NOTE: this test includes device-dependent keycode values.
     */
    void testGetSystemKey() {
        assertEquals("mismatch: SYSTEM_KEY_POWER",
            EventConstants.SYSTEM_KEY_POWER, KeyConverter.getSystemKey(-12));
        assertEquals("mismatch: SYSTEM_KEY_SEND",
            EventConstants.SYSTEM_KEY_SEND, KeyConverter.getSystemKey(-10));
        assertEquals("mismatch: SYSTEM_KEY_END",
            EventConstants.SYSTEM_KEY_END, KeyConverter.getSystemKey(-11));
        assertEquals("mismatch: SYSTEM_KEY_CLEAR",
            EventConstants.SYSTEM_KEY_CLEAR, KeyConverter.getSystemKey(-8));
    }


    /**
     * Tests that none of the standard keys KEY_NUM0..KEY_NUM9, KEY_STAR,
     * KEY_POUND, and none of the game actions, is a system key, by
     * checking that getSystemKey returns 0 in each case.  This test is 
     * probably device-independent.
     */
    void testNonSystemKey() {
        assertEquals("KEY_NUM0", 0,
            KeyConverter.getSystemKey(Canvas.KEY_NUM0));
        assertEquals("KEY_NUM1", 0,
            KeyConverter.getSystemKey(Canvas.KEY_NUM1));
        assertEquals("KEY_NUM2", 0,
            KeyConverter.getSystemKey(Canvas.KEY_NUM2));
        assertEquals("KEY_NUM3", 0,
            KeyConverter.getSystemKey(Canvas.KEY_NUM3));
        assertEquals("KEY_NUM4", 0,
            KeyConverter.getSystemKey(Canvas.KEY_NUM4));
        assertEquals("KEY_NUM5", 0,
            KeyConverter.getSystemKey(Canvas.KEY_NUM5));
        assertEquals("KEY_NUM6", 0,
            KeyConverter.getSystemKey(Canvas.KEY_NUM6));
        assertEquals("KEY_NUM7", 0,
            KeyConverter.getSystemKey(Canvas.KEY_NUM7));
        assertEquals("KEY_NUM8", 0,
            KeyConverter.getSystemKey(Canvas.KEY_NUM8));
        assertEquals("KEY_NUM9", 0,
            KeyConverter.getSystemKey(Canvas.KEY_NUM9));
        assertEquals("KEY_STAR", 0,
            KeyConverter.getSystemKey(Canvas.KEY_STAR));
        assertEquals("KEY_POUND", 0,
            KeyConverter.getSystemKey(Canvas.KEY_POUND));

        assertEquals("UP", 0,
            KeyConverter.getSystemKey(
                KeyConverter.getKeyCode(Canvas.UP)));
        assertEquals("DOWN", 0,
            KeyConverter.getSystemKey(
                KeyConverter.getKeyCode(Canvas.DOWN)));
        assertEquals("LEFT", 0,
            KeyConverter.getSystemKey(
                KeyConverter.getKeyCode(Canvas.LEFT)));
        assertEquals("RIGHT", 0,
            KeyConverter.getSystemKey(
                KeyConverter.getKeyCode(Canvas.RIGHT)));
        assertEquals("FIRE", 0,
            KeyConverter.getSystemKey(
                KeyConverter.getKeyCode(Canvas.FIRE)));
        assertEquals("GAME_A", 0,
            KeyConverter.getSystemKey(
                KeyConverter.getKeyCode(Canvas.GAME_A)));
        assertEquals("GAME_B", 0,
            KeyConverter.getSystemKey(
                KeyConverter.getKeyCode(Canvas.GAME_B)));
        assertEquals("GAME_C", 0,
            KeyConverter.getSystemKey(
                KeyConverter.getKeyCode(Canvas.GAME_C)));
        assertEquals("GAME_D", 0,
            KeyConverter.getSystemKey(
                KeyConverter.getKeyCode(Canvas.GAME_D)));
    }

    /**
     * Tests getKeyName() for the set of numeric keys. The expected names for
     * these keys are probably device-independent.
     */
    void testGetKeyName1() {
        assertEquals("KEY_NUM0", "0",
            KeyConverter.getKeyName(Canvas.KEY_NUM0));
        assertEquals("KEY_NUM1", "1",
            KeyConverter.getKeyName(Canvas.KEY_NUM1));
        assertEquals("KEY_NUM2", "2",
            KeyConverter.getKeyName(Canvas.KEY_NUM2));
        assertEquals("KEY_NUM3", "3",
            KeyConverter.getKeyName(Canvas.KEY_NUM3));
        assertEquals("KEY_NUM4", "4",
            KeyConverter.getKeyName(Canvas.KEY_NUM4));
        assertEquals("KEY_NUM5", "5",
            KeyConverter.getKeyName(Canvas.KEY_NUM5));
        assertEquals("KEY_NUM6", "6",
            KeyConverter.getKeyName(Canvas.KEY_NUM6));
        assertEquals("KEY_NUM7", "7",
            KeyConverter.getKeyName(Canvas.KEY_NUM7));
        assertEquals("KEY_NUM8", "8",
            KeyConverter.getKeyName(Canvas.KEY_NUM8));
        assertEquals("KEY_NUM9", "9",
            KeyConverter.getKeyName(Canvas.KEY_NUM9));
        assertEquals("KEY_STAR", "*",
            KeyConverter.getKeyName(Canvas.KEY_STAR));
        assertEquals("KEY_POUND", "#",
            KeyConverter.getKeyName(Canvas.KEY_POUND));
    }

    /**
     * Tests getKeyName() for the set of keys mapped to game actions.
     * 
     * NOTE: the expected key names are device- and language-specific.
     */
    void testGetKeyName2() {
        assertEquals("UP", "Up",
            KeyConverter.getKeyName(
                KeyConverter.getKeyCode(Canvas.UP)));
        assertEquals("DOWN", "Down",
            KeyConverter.getKeyName(
                KeyConverter.getKeyCode(Canvas.DOWN)));
        assertEquals("LEFT", "Left",
            KeyConverter.getKeyName(
                KeyConverter.getKeyCode(Canvas.LEFT)));
        assertEquals("RIGHT", "Right",
            KeyConverter.getKeyName(
                KeyConverter.getKeyCode(Canvas.RIGHT)));
        assertEquals("FIRE", "Select",
            KeyConverter.getKeyName(
                KeyConverter.getKeyCode(Canvas.FIRE)));
        assertEquals("GAME_A", "Calendar",
            KeyConverter.getKeyName(
                KeyConverter.getKeyCode(Canvas.GAME_A)));
        assertEquals("GAME_B", "Addressbook",
            KeyConverter.getKeyName(
                KeyConverter.getKeyCode(Canvas.GAME_B)));
        assertEquals("GAME_C", "Menu",
            KeyConverter.getKeyName(
                KeyConverter.getKeyCode(Canvas.GAME_C)));
        assertEquals("GAME_D", "Mail",
            KeyConverter.getKeyName(
                KeyConverter.getKeyCode(Canvas.GAME_D)));
    }

    /**
     * Runs all tests.
     */
    public void runTests() {
        declare("testAllGameActions");
        testAllGameActions();
        declare("testGetSystemKey");
        testGetSystemKey();
        declare("testNonSystemKey");
        testNonSystemKey();
        declare("testGetKeyName1");
        testGetKeyName1();
        declare("testGetKeyName2");
        testGetKeyName2();
    }

}
