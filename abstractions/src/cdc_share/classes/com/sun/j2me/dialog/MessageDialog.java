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
        System.out.println(message);        
        return d.waitForAnswer();
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
        while (true) {
            System.out.println("Choose one of the items");
            for (int i = 0; i < list.length; i++) {
                System.out.println(i + ". " + list[i]);
            }
            
            String choice = d.append(); /* User's choice */
            try {
                return d.waitForAnswer() == Dialog.CANCELLED ?
                       -1 : Integer.decode(choice).intValue();
            }
            catch (NumberFormatException e) {
                System.out.println("Value for chosen item is incorrect");
            }
        }
    }
}
