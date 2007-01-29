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

import com.sun.j2me.i18n.Resource;
import com.sun.j2me.i18n.ResourceConstants;

/**
 * Provides methods for simple messages and requests.
 */
public class MessageDialog {

    /**
     * Displays dialog with informational message or confirmation
     * request.
     * @param token security token.
     * @param title dialog title
     * @param message message text
     * @param withCancel show Cancel button
     * @return -1 if user cancelled dialog, 1 otherwise
     * @throws InterruptedException if interrupted
     */
    public static int showMessage(String title,
                           String message, boolean withCancel)
            throws InterruptedException {

        Dialog d = new Dialog(title, withCancel);
//        d.append(new StringItem(message, null));
        return d.waitForAnswer();
    }

    /**
     * Displays dialog that allows user to select one element
     * from the list.
     * @param token security token.
     * @param title dialog title
     * @param label list label
     * @param list elements of the list
     * @return -1 if user cancelled dialog, index of chosen item
     * otherwise
     * @throws InterruptedException if interrupted
     */
    public static int chooseItem(String title,
                          String label, String[] list)
            throws InterruptedException {

        Dialog d = new Dialog(title, true);
//        ChoiceGroup choice =
//                   new ChoiceGroup(label, Choice.EXCLUSIVE, list, null);
//        d.append(choice);
        return d.waitForAnswer() == Dialog.CANCELLED ?
                   -1 : /*choice.getSelectedIndex()*/0;
    }

    /**
     * Displays dialog with new PIN parameters.
     * @param token security token.
     * @return array of strings or null if cancelled. Array contains new
     * PIN label and PIN value.
     * @throws InterruptedException  if interrupted
     */
    public static String[] enterNewPIN()
            throws InterruptedException {

//        Dialog d = new Dialog(
//            // "New PIN", 
//            Resource.getString(
//                ResourceConstants.JSR177_PINDIALOG_TITLE_NEWPIN),
//            true);
//
//        TextField label = new TextField(
//            // "PIN label ", 
//            Resource.getString(
//                ResourceConstants.JSR177_PINDIALOG_LABEL) + " ",
//            "", 32, 0);
//        TextField pin1 = new TextField(
//            // "Enter PIN ", 
//            Resource.getString(
//                ResourceConstants.JSR177_PINDIALOG_ENTERPIN) + " ",
//            "", 8, TextField.PASSWORD | TextField.NUMERIC);
//        TextField pin2 = new TextField(
//            // "Confirm PIN ", 
//            Resource.getString(
//                ResourceConstants.JSR177_PINDIALOG_CONFIRMPIN) + " ",
//            "", 8, TextField.PASSWORD | TextField.NUMERIC);
//
//        d.append(label);
//        d.append(pin1);
//        d.append(pin2);
//
//        while (true) {
//            if (d.waitForAnswer(token) == -1) {
//                return null;
//            }
//
//            String s = label.getString().trim();
//            if (s.equals("")) {
//                continue;
//            }
//
//            String h1 = pin1.getString().trim();
//            String h2 = pin2.getString().trim();
//
//            if (h1.equals("") ||
//                ! h1.equals(h2) ||
//                h1.length() < 4) {
//                pin1.setString("");
//                pin2.setString("");
//                continue;
//            }
//            return new String[] {s, h1};
//        }
            return null;
    }
}
