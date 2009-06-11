/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.j2me.dialog;

/** MIDP dependencies - public API */
import javax.microedition.lcdui.TextField;
import javax.microedition.lcdui.ChoiceGroup;
import javax.microedition.lcdui.Choice;
import javax.microedition.lcdui.StringItem;

import com.sun.j2me.security.Token;

/**
 * Provides methods for simple messages and requests.
 */
public class MessageDialog {

    /**
     * Displays dialog with informational message or confirmation
     * request.
     * @param title dialog title
     * @param message message text
     * @param withCancel show Cancel button
     * @param token security token
     * @return -1 if user cancelled dialog, 1 otherwise
     * @throws InterruptedException if interrupted
     */
    public static int showMessage(String title,
                           String message, boolean withCancel,
                           Token token)
            throws InterruptedException {

        Dialog d = new Dialog(title, withCancel);
        d.append(new StringItem(message, null));
        return d.waitForAnswer(token);
    }

    /**
     * Displays dialog with input field.
     * @param title dialog title
     * @param message message text
     * @param token security token.
     * @return strings or null if cancelled. String contains
     * user input.
     * @throws InterruptedException  if interrupted
     */
    public static String promptMessage(String title,
                           String message, String defaultValue,
                           Token token)
            throws InterruptedException {

        Dialog d = new Dialog(title, true);
        d.append(new StringItem(message, null));
        
        TextField userInput = new TextField(" ",
            defaultValue, 32, 0);
        d.append(userInput);

        while (true) {
            if (d.waitForAnswer(token) == Dialog.CANCELLED) {
                return null;
            }

            String s = userInput.getString().trim();
            if (s.equals("")) {
                continue;
            }
            return s;
        }
    }

    /**
     * Displays dialog that allows user to select one element
     * from the list.
     * @param title dialog title
     * @param label list label
     * @param list elements of the list
     * @param token security token
     * @return -1 if user cancelled dialog, index of chosen item
     * otherwise
     * @throws InterruptedException if interrupted
     */
    public static int chooseItem(String title,
                          String label, String[] list, Token token)
            throws InterruptedException {

        Dialog d = new Dialog(title, true);
        ChoiceGroup choice =
                   new ChoiceGroup(label, Choice.EXCLUSIVE, list, null);
        d.append(choice);
        return d.waitForAnswer(token) == Dialog.CANCELLED ?
                   -1 : choice.getSelectedIndex();
    }

}
