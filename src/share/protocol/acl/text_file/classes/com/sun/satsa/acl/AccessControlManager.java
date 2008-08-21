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

import java.security.MessageDigest;
import com.sun.cdc.io.j2me.apdu.APDUManager;

/**
 * This class represents access control information manager.
 */
public class AccessControlManager {

    /** Access control information for card slots. */
    private static ACSlot[] ACLInfo;

    /**
     * Initialise ACL information.
     */
    private synchronized static void init() {
        if (ACLInfo != null) {
            return;
        }
        int maxSlot = APDUManager.getSlotCount();
        ACLInfo = new ACSlot[maxSlot];
        for (int i = 0; i < maxSlot; i++) {
            ACLInfo[i] = ACSlot.load(i);
        }
    }

    /**
     * Initialize ACL information.
     * @param slot int the slot number.
     */
    public synchronized static void init(int slot) {
        if (ACLInfo == null) {
            init();
        }
        if (ACLInfo != null) {
            ACLInfo[slot] = ACSlot.load(slot);
        }
    }

    /**
     * SHA-1 message digest object.
     */
    private static MessageDigest sha;

    /**
     * Synchronization object for message digest calculation.
     */
    private static Object shaSync = new Object();

    /**
     * Calculates hash value.
     * @param inBuf data buffer.
     * @param inOff offset of data in the buffer.
     * @param inLen length of data.
     * @return array containing SHA-1 hash.
     */
    public static byte[] getHash(byte[] inBuf, int inOff, int inLen) {

        synchronized (shaSync) {

            try {
                if (sha == null) {
                    sha = MessageDigest.getInstance("SHA-1");
                }
                byte[] hash = new byte[20];
                sha.update(inBuf, inOff, inLen);
                sha.digest(hash, 0, hash.length);
                return hash;
            }
            catch (Exception e) {
                e.printStackTrace();
                return null;
            }
        }
    }

    /**
     * Returns object that should be used for access control verification.
     * @param slot slot number.
     * @param selectAPDU SELECT APDU command data.
     * @param root name of CA that authorized the suite.
     * @return object that can be used to check permissions.
     */
    public static APDUPermissions getAPDUPermissions(int slot,
                                                     byte[] selectAPDU,
                                                     String root) {
        if (ACLInfo == null || ACLInfo[slot] == null) {
            APDUPermissions perm;
            perm = new APDUPermissions(null);
            perm.setType(ACLPermissions.ALLOW);
            return perm;
        }

        return (APDUPermissions) ACLInfo[slot].getACLPermissions(true,
               selectAPDU, root);
    }

    /**
     * Returns object that should be used for access control verification.
     * @param slot slot number.
     * @param selectAPDU SELECT APDU command data.
     * @param root name of CA that authorized the suite.
     * @return object that can be used to check permissions.
     */
    public static JCRMIPermissions getJCRMIPermissions(int slot,
                                                       byte[] selectAPDU,
                                                       String root) {
        if (ACLInfo == null || ACLInfo[slot] == null) {
            JCRMIPermissions perm = new JCRMIPermissions(null);
            perm.setType(ACLPermissions.ALLOW);
            return perm;
        }

        return (JCRMIPermissions) ACLInfo[slot].getACLPermissions(false,
               selectAPDU, root);
    }
}
