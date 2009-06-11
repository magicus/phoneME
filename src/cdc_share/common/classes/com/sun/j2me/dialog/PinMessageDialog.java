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

import com.sun.j2me.security.Token;

/**
 * Provides methods for PIN requests.
 */
public class PinMessageDialog {

    /**
     * Displays dialog with new PIN parameters.
     * @param token security token
     * @return array of strings or null if cancelled. Array contains new
     * PIN label and PIN value.
     * @throws InterruptedException  if interrupted
     */
    public static String[] enterNewPIN(Token token)
            throws InterruptedException {
        String label = ""; /* New pin's label */
        String pin1 = "";  /* New pin's value */
        String pin2 = "";  /* Confirmation of new pin's value */
        
        Dialog d = new Dialog(
            // "New PIN", 
            Resource.getString(
                ResourceConstants.JSR177_PINDIALOG_TITLE_NEWPIN),
            true);

        while (true) {
            System.out.println(// "PIN label ", 
                Resource.getString(
                    ResourceConstants.JSR177_PINDIALOG_LABEL));
            label = d.append();

            System.out.println(// "Enter PIN ", 
                Resource.getString(
                    ResourceConstants.JSR177_PINDIALOG_ENTERPIN));
            pin1 = d.append();

            System.out.println(// "Confirm PIN ", 
                Resource.getString(
                    ResourceConstants.JSR177_PINDIALOG_CONFIRMPIN));
            pin2 = d.append();

            if (d.waitForAnswer() == Dialog.CANCELLED) {
                return null;
            }

            if (label.equals("")) {
                System.out.println("Label is invalid");
                continue;
            }

            if (pin1.equals("") ||
                !pin1.equals(pin2) ||
                pin1.length() < 4) {
                System.out.println("Pins' values are invalid");
                continue;
            }
            return new String[] {label, pin1};
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
     * @param token security token
     * @return null if PIN entry was cancelled by user, otherwise an array
     * containing PIN value(s).
     * @throws InterruptedException  if interrupted
     */
    public static Object[] enterPins(String title,
                      String label1, boolean pin1IsNumeric, int pin1Length,
                      String label2, boolean pin2IsNumeric, int pin2Length,
                      Token token)
           throws InterruptedException {
        String data1 = "";  /* First pin to enter */
        String data2 = "";  /* Second pin to enter */
        
        Dialog d = new Dialog(title, true);

        while (true) {
            System.out.println(label1);
            data1 = d.append();

            if (pin2Length != 0) {
                System.out.println(label2);
                data2 = d.append();
            }

            if (d.waitForAnswer() == Dialog.CANCELLED) {
                return null;
            }

            if (data1.equals("")) {
                System.out.println("Pin1 value is invalid");
                continue;
            }

            if (pin2Length != 0) {
                if (data2.equals("")) {
                    System.out.println("Pin2 value is invalid");
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
