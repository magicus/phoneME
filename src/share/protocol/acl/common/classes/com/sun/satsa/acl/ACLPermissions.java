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

import java.util.Vector;
import java.lang.SecurityException;

import com.sun.j2me.security.Token;

/**
 * This class represents a set of ACL permissions.
 */
public class ACLPermissions {

    /** PIN operation constant. */
    public static final int CMD_VERIFY = 0;
    /** PIN operation constant. */
    public static final int CMD_CHANGE = 1;
    /** PIN operation constant. */
    public static final int CMD_DISABLE = 2;
    /** PIN operation constant. */
    public static final int CMD_ENABLE = 3;
    /** PIN operation constant. */
    public static final int CMD_UNBLOCK = 4;
    /** The number of supported PIN commands. */
    public static final int CMD_COUNT = 5;

    /**
     * Flag that indicates that the object contains permissions.
     */
    static final int CHECK = 0;

    /**
     * Flag that indicates that MIDlet suite have full access.
     */
    static final int ALLOW = 1;
    /**
     * Flag that indicates that MIDlet suite have not access.
     */
    static final int DISALLOW = 2;

    /**
     * Verifier type.
     */
    int type = CHECK;
    /**
     * The list of permissions.
     */
    Vector permissions;

    /**
     * Array of PIN data for this permission.
     */
    private PINData[] pins;

    /** Parent ACSlot object, contains PIN attributes. */
    protected ACSlot parent;

    /**
     * Constructs new object.
     * @param parent parent ACSlot object.
     */
    public ACLPermissions(ACSlot parent) {
        this.parent = parent;
    }

    /**
     * Set the list of permissions.
     * @param permissions the list of permissions.
     */
    public void setPermissions(Vector permissions) {
        this.permissions = permissions;
    }

    /**
     * Set type of the permission.
     * @param type permission type.
     */
    public void setType(int type) {
        this.type = type;
    }

    /**
     * Set PIN data for this permission.
     * @param data PIN data for this permission.
     */
    public void setPINData(PINData[] data) {
        pins = data;
    }

    /** Attributes of the first PIN to be entered. */
    protected PINAttributes attr1;
    /** Attributes of the second PIN or null. */
    protected PINAttributes attr2;

    /**
     * Verifies that PIN operation is supported, finds PIN attributes.
     * @param pinID PIN identifier.
     * @param unblockPinID unblocking PIN identifier.
     * @param action PIN operation identifier.
     * @exception SecurityException if operation is not
     * supported.
     */
    protected void checkPINOperation(int pinID, int unblockPinID,
                                     int action) {

        if (pins == null) {
            throw new SecurityException();
        }

        boolean found = false;
        for (int i = 0; i < pins.length; i++) {
            found |= (pins[i].id == pinID);
        }
        attr1 = parent.getPINAttributes(pinID);

        if (! found || attr1 == null || ! attr1.check(action)) {
            throw new SecurityException();
        }

        if (action == CMD_UNBLOCK) {
            attr2 = parent.getPINAttributes(unblockPinID);
            if (attr2 == null || ! attr2.isUnblockingPIN()) {
                throw new SecurityException();
            }
        } else {
            attr2 = null;
        }
    }


    /**
     * Requests the user to enter the PIN value(s).
     * @param action PIN operation identifier.
     * @param token security token
     * @return null if operation was cancelled or the array that contains
     * byte array(s) with PIN value(s).
     */
    public Object[] enterPIN(int action, Token token) {
        PINEntryDialog dialog;

        try {
            dialog = new PINEntryDialog(action, attr1, attr2, token);
        } catch (InterruptedException e) {
            throw new SecurityException("Interrupted");
        }

        return dialog.getPINs();
    }

    /**
     * Get PIN command for specified ID and operation.
     * @param pinID PIN identifier.
     * @param action PIN operation identifier.
     * @return PIN data or null if not found.
     */
    Object getPINCommand(int pinID, int action) {

        for (int i = 0; i < pins.length; i++) {
            if (pins[i].id == pinID && pins[i].commands[action] != null) {
                return pins[i].commands[action];
            }
        }
        return null;
    }
}
