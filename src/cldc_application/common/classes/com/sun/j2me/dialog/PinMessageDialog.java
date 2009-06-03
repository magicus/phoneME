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

/** MIDP dependencies - public API */
import javax.microedition.lcdui.TextField;
import javax.microedition.lcdui.StringItem;

import com.sun.j2me.security.Token;

/**
 * Provides methods for PIN requests.
 */
public class PinMessageDialog {

    /**
     * Displays dialog with new PIN parameters.
     * @param token security token.
     * @return array of strings or null if cancelled. Array contains new
     * PIN label and PIN value.
     * @throws InterruptedException  if interrupted
     */
    public static String[] enterNewPIN(Token token)
            throws InterruptedException {

        Dialog d = new Dialog(
            // "New PIN", 
            Resource.getString(
                ResourceConstants.JSR177_PINDIALOG_TITLE_NEWPIN),
            true);

        TextField label = new TextField(
            // "PIN label ", 
            Resource.getString(
                ResourceConstants.JSR177_PINDIALOG_LABEL) + " ",
            "", 32, 0);
        TextField pin1 = new TextField(
            // "Enter PIN ", 
            Resource.getString(
                ResourceConstants.JSR177_PINDIALOG_ENTERPIN) + " ",
            "", 8, TextField.PASSWORD | TextField.NUMERIC);
        TextField pin2 = new TextField(
            // "Confirm PIN ", 
            Resource.getString(
                ResourceConstants.JSR177_PINDIALOG_CONFIRMPIN) + " ",
            "", 8, TextField.PASSWORD | TextField.NUMERIC);

        d.append(label);
        d.append(pin1);
        d.append(pin2);

        while (true) {
            if (d.waitForAnswer(token) == Dialog.CANCELLED) {
                return null;
            }

            String s = label.getString().trim();
            if (s.equals("")) {
                continue;
            }

            String h1 = pin1.getString().trim();
            String h2 = pin2.getString().trim();

            if (h1.equals("") ||
                ! h1.equals(h2) ||
                h1.length() < 4) {
                pin1.setString("");
                pin2.setString("");
                continue;
            }
            return new String[] {s, h1};
        }
    }

    /**
     * Displays dialog with new PIN parameters.
     * @param title dialog title
     * @param label1 PIN1 label
     * @param pin1IsNumeric PIN1 is a number
     * @param pin1Length length of the PIN1 text field
     * @param label2 PIN2 label
     * @param pin2IsNumeric PIN2 is a numbere     
     * @param pin2Length length of the PIN2 text field
     * @param token security token.
     * @return null if PIN entry was cancelled by user, otherwise an array
     * containing PIN value(s).
     * @throws InterruptedException  if interrupted
     */
    public static Object[] enterPins(String title,
                      String label1, boolean pin1IsNumeric, int pin1Length,
                      String label2, boolean pin2IsNumeric, int pin2Length,
                      Token token)
           throws InterruptedException {

        Dialog d = new Dialog(title, true);

        int flags = TextField.PASSWORD;
        if (pin1IsNumeric) {
            flags |= TextField.NUMERIC;
        }
        TextField tf1 = new TextField(label1, "", pin1Length, flags);
        d.append(tf1);

        TextField tf2 = null;
        if (pin2Length != 0) {

            flags = TextField.SENSITIVE | TextField.NON_PREDICTIVE;
            if (pin2IsNumeric) {
                flags |= TextField.NUMERIC;
            }
            tf2 = new TextField(label2, "", pin2Length, flags);
            d.append(tf2);

        }

        while (true) {
            if (d.waitForAnswer(token) == Dialog.CANCELLED) {
                return null;
            }

            String data1 = tf1.getString();            
            if (data1.trim().equals("")) {
                tf1.setString("");
                continue;
            }

            if (pin2Length != 0) {
                String data2 = tf2.getString();

                if (data2.trim().equals("")) {
                    tf2.setString("");
                    continue;
                }

                return new Object[] { data1, data2 };
            }
            else {
                return new Object[] { data1 };
            }
        }
    }
}
