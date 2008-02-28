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

/**
 * This class represents a set of APDU permissions.
 */
public class APDUPermissions extends ACLPermissions {

    /**
     * Constructs new object.
     * @param parent parent ACFile object.
     */
    public APDUPermissions(ACSlot parent) {
        super(parent);
    }

    /**
     * Verifies that the MIDlet have permission for this APDU command.
     * @param apdu the command header.
     * @throws SecurityException if access denied
     */
    public void checkPermission(int apdu) {

        if (type == ALLOW) {
            return;
        }

        if (type == DISALLOW) {
            throw new SecurityException("Access denied");
        }

        for (int i = 0; i < permissions.size(); i++) {

            int[] data = (int[]) permissions.elementAt(i);

            for (int k = 0; k < data.length; k = k + 2) {
                if ((apdu & data[k + 1]) == data[k]) {
                    return;
                }
            }
        }
        throw new SecurityException("Access denied: " +
                                    Integer.toHexString(apdu));
    }

    /**
     * Contains default APDU headers for PIN operations.
     */
    private static int[] defaultPINCommand = {
        0x80200000,    // verify
        0x80240000,    // change
        0x80260000,    // disable
        0x80280000,    // enable
        0x802C0000     // unblock
    };

    /**
     * Returns APDU header for given PIN and PIN operation.
     * Initializes internal variables, verifies that operation is
     * supported and permitted.
     * @param id PIN identifier.
     * @param uid unblocking PIN identifier.
     * @param action PIN operation code.
     * @return APDU header encoded as integer.
     * @throws java.lang.SecurityException if access denied
     */
    public int preparePIN(int id, int uid, int action) {

        checkPINOperation(id, uid, action);
        Integer result = (Integer) getPINCommand(id, action);
        int command = result != null ? result.intValue() :
                defaultPINCommand[action] | (attr1.pinReference & 0xff);
        checkPermission(command);
        return command;
    }
}
