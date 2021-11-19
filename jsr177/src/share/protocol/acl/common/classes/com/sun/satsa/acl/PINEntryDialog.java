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

package com.sun.satsa.acl;

import com.sun.j2me.i18n.Resource;
import com.sun.j2me.i18n.ResourceConstants;
import com.sun.j2me.dialog.PinMessageDialog;

import com.sun.j2me.security.Token;

/** Implements PIN entry dialog. */
public class PINEntryDialog {

    /** Attributes of the 1st PIN. */
    private PINAttributes pin1;
    /** Attributes of the 2nd PIN. */
    private PINAttributes pin2;
    /** PIN data. */
    private Object[] data;

    /**
     * Construct PIN dialog.
     *
     * @param action PIN entry operation identifier.
     * @param attr1 1st PIN attributes
     * @param attr2 2nd PIN attributes
     * @param token security token
     * @exception InterruptedException if another thread interrupts the
     *   calling thread while this method is waiting to preempt the
     *   display.
     */
    public PINEntryDialog(int action,
                          PINAttributes attr1, PINAttributes attr2, Token token)
            throws InterruptedException {

        String title = null;

        String label1 = attr1.label;
        String label2 = null;
        pin1 = attr1;
        pin2 = null;

        switch (action) {
            case ACLPermissions.CMD_VERIFY: {
                // "PIN verification"
                title = Resource.getString(ResourceConstants.
                    JSR177_PINDIALOG_TITLE_VERIFY);
                break;
            }
            case ACLPermissions.CMD_CHANGE: {
                // "Change PIN"
                title = Resource.getString(ResourceConstants.
                    JSR177_PINDIALOG_TITLE_CHANGE);
                // "New value" -> "Enter new PIN"
                label2 = Resource.getString(ResourceConstants.
                    JSR177_PINDIALOG_ENTERPIN);
                pin2 = attr1;
                break;
            }
            case ACLPermissions.CMD_DISABLE: {
                // "Disable PIN"
                title = Resource.getString(ResourceConstants.
                    JSR177_PINDIALOG_TITLE_DISABLE);
                break;
            }
            case ACLPermissions.CMD_ENABLE: {
                // "Enable PIN";
                title = Resource.getString(ResourceConstants.
                    JSR177_PINDIALOG_TITLE_ENABLE);
                break;
            }
            case ACLPermissions.CMD_UNBLOCK: {
                // "Unblock PIN"
                title = Resource.getString(ResourceConstants.
                    JSR177_PINDIALOG_TITLE_UNBLOCK);
                label1 = attr2.label;
                label2 = attr1.label + " - " + 
                    // "new value"
                    Resource.getString(ResourceConstants.
                        JSR177_PINDIALOG_NEWVALUE);
                pin1 = attr2;
                pin2 = attr1;
                break;
            }
        }

        int pin2Length;
        boolean pin2IsNumeric;
        if (pin2 == null) {
            pin2Length = 0;
            pin2IsNumeric = false;
        }
        else {
            pin2Length = pin2.getMaxLength();
            pin2IsNumeric = pin2.isNumeric();
        }

        data = PinMessageDialog.enterPins(title,
                                       label1, pin1.isNumeric(), pin1.getMaxLength(),
                                       label2, pin2IsNumeric, pin2Length, token);
    }

    /**
     * Get the entered values.
     * @return null if PIN entry was cancelled by user, otherwise an array
     * containing PIN value(s).
     */
    public Object[] getPINs() {

        if (data == null) {
            return null;
        }

        byte[] data1 = pin1.transform((String)data[0]);
        if (data1 == null) {
            // PIN can't be formatted, pass empty PIN to update counter
            data1 = new byte[8];
        }

        byte[] data2;
        if ((pin2 != null) && (data.length == 2)) {
            data2 = pin2.transform((String)data[1]);
        } else {
            data2 = null;
        }

        if (data2 != null) {
            return new Object[] { data1, data2 };
        }
        else {
            return new Object[] { data1 };
        }
    }
}
