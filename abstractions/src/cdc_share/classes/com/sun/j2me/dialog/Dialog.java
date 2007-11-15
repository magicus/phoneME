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

import java.io.IOException;

/**
 * This class represents simple dialog form.
 */
public class Dialog {

    /** Answer that indicates that the dialog was cancelled. */
    public static final int CANCELLED = -1;
    /** Answer that indicates successful completion. */
    public static final int CONFIRMED = 1;

    /* Confirmation string. */
    private String okCmd = ":o";//Resource.getString(ResourceConstants.OK);
    
    /* "Cancel" string. */
    private String cancelCmd = ":q";//Resource.getString(ResourceConstants.CANCEL);

    private boolean withCancel;

    /**
     * Construct message dialog for informational message or
     * confirmation request.
     * @param title dialog title
     * @param withCancel show Cancel button
     */
    public Dialog(String title, boolean withCancel) {
        System.out.println();
        System.out.println(title);
        System.out.println("Print '" + okCmd + "' for operation confirmation");
        if (withCancel) {
            System.out.println("Print '" + cancelCmd + "' for operation cancellation");
            this.withCancel = withCancel;
        }
        System.out.println();
    }

    /**
     * Adds an Item into the Form.
     * @return input string
     */
    String append() {
        String strInput = "";
        
        while (true) {
            try {
                int tmp = System.in.read();
                if (tmp == -1) break;
                strInput = strInput + (char)tmp;
            }
            catch (IOException e) {
            }
        }

        System.out.println();
        return strInput;
    }

    /**
     * Waits for the user's answer.
     * @return user's answer
     */
    public int waitForAnswer() {
        if (withCancel) {
            while (true) {
                String strInput = "";
                System.out.println("Please enter result of operation: " + okCmd + "/" + cancelCmd);
                strInput = append();
                if (strInput.equals(okCmd)) {
                    return CONFIRMED;
                }
                if (strInput.equals(cancelCmd)) {
                    return CANCELLED;
                }
                System.out.println("Result is invalid");
            }
        }
        else {
            return CONFIRMED;
        }
    }
}
