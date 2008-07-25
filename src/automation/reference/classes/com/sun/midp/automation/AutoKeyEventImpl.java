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

package com.sun.midp.automation;
import java.util.*;

final class AutoKeyEventImpl implements AutoKeyEvent {
    public final static String KEY_INVALID_NAME      = "INVALID";
    public final static String KEY_BACKSPACE_NAME    = "BACKSPACE";
    public final static String KEY_UP_NAME           = "UP";
    public final static String KEY_DOWN_NAME         = "DOWN";
    public final static String KEY_LEFT_NAME         = "LEFT";
    public final static String KEY_RIGHT_NAME        = "RIGHT";
    public final static String KEY_SELECT_NAME       = "SELECT";
    public final static String KEY_SOFT1_NAME        = "SOFT1";
    public final static String KEY_SOFT2_NAME        = "SOFT2";
    public final static String KEY_CLEAR_NAME        = "CLEAR";
    public final static String KEY_SEND_NAME         = "SEND";
    public final static String KEY_END_NAME          = "END";
    public final static String KEY_POWER_NAME        = "POWER";
    public final static String KEY_GAMEA_NAME        = "GAMEA";
    public final static String KEY_GAMEB_NAME        = "GAMEB";
    public final static String KEY_GAMEC_NAME        = "GAMEC";
    public final static String KEY_GAMED_NAME        = "GAMED";
    public final static String KEY_GAME_UP_NAME      = "UP";
    public final static String KEY_GAME_DOWN_NAME    = "DOWN";
    public final static String KEY_GAME_LEFT_NAME    = "LEFT";
    public final static String KEY_GAME_RIGHT_NAME   = "RIGHT";

    public int getType() {
        return AutoEvent.TYPE_KEYBOARD;
    }

    public int getKeyCode() {
        return keyCode;
    }

    public int getKeyState() {
        return keyState;
    }    

    private final static String PREFIX = "keyboard";

    private class EventFromStringCreator 
        implements AutoEventFactoryImpl.EventFromStringCreator {

        public String getPrefix() {
            return PREFIX;
        }

        public AutoEvent createFromString(String str, int offset, 
            Integer newOffset) throws IllegalArgumentException {

            return null;
        }
    }

    private void registerEventFromStringCreator() {
        AutoEventFactoryImpl f = AutoEventFactoryImpl.getInstanceImpl();
        f.registerEventFromStringCreator(new EventFromStringCreator());        
    }

    private int nameToKeyCode(String name) {
        int keyCode = KEY_INVALID;

        if (namesToKeyCodes == null) {
            namesToKeyCodes = new Hashtable();

            namesToKeyCodes.put(KEY_INVALID_NAME, new Integer(KEY_INVALID));
        }

        Integer n = (Integer)namesToKeyCodes.get(name);
        if (n != null) {
            keyCode = n.intValue();
        }

        return keyCode;
    }

    static {
        new AutoKeyEventImpl().registerEventFromStringCreator();
    }
    
    private static Hashtable namesToKeyCodes = null;

    private int keyState;
    private int keyCode = KEY_INVALID;
}
