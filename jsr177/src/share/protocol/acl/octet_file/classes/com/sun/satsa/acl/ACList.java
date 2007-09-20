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

import com.sun.satsa.util.*;
import com.sun.satsa.util.pkcs15.*;

import java.util.Vector;
import java.io.IOException;

/**
 * This class represents access control list (ACL).
 */
public class ACList extends PKCS15File {

    /**
     * AID for this ACL. Received from ACIF.
     */
    private byte[] AID;
    /** This vector contains parsed objects from ACF. */
    private Vector ACF;
    /**
     * The list of ACEs.
     */
    private ACEntry ACE;

    /** PIN information for APDU connection. */
    private PINData[] PINInfo;
    /** This vector contains PIN information */
    private Vector pin_info = new Vector();

    /**
     * Creates ACList object for the pointed aid, location and file system
     * @param aid byte[] application AID
     * @param location Location required location
     * @param files AclFileSystem requred file system
     */
    ACList(byte[] aid, Location location, AclFileSystem files) {
        super(location, files);
        AID = aid;
        ACF = new Vector();
        resetLoader(ACF, null, null);
        try {
            parseDF(location.path);
            if (ACF.size() != 0) {
                readACF();
            } else {
                ACE = null;
            }
        } catch (IOException e) {
            ACE = null;
        }
    }

    /**
     * Reads ACE data from the ACF vector.
     * @throws TLVException if TLV error occurs
     */
    private void readACF()
                         throws TLVException {
        ACE = new ACEntry(ACF, pin_info);
        PINInfo = new PINData[pin_info.size()];
        pin_info.copyInto(PINInfo);
    }

    /**
     * Verifies if this ACL describes permissions for AID in SELECT APDU.
     * @param selectAPDU SELECT APDU command data.
     * @return true if this ACL describes permissions for AID in SELECT APDU.
     */
    public boolean match(byte[] selectAPDU) {
        return (AID == null) ||
            (AID.length == selectAPDU[4] &&
             Utils.byteMatch(AID, 0, AID.length,
                             selectAPDU, 5, AID.length));
    }

    /**
     * Returns the list of ACEs.
     * @return the list of ACEs.
     */
    public ACEntry getACEntries() {
        return ACE;
    }

    /**
     * Places information about PINs into the vector.
     * @param isAPDU if true, place APDU PIN data, otherwise - JCRMI
     * PIN data.
     * @param result the vector for results
     */
    void getPINs(boolean isAPDU, Vector result) {
        for (int i = 0; i < PINInfo.length; i++) {
            if (isAPDU == (PINInfo[i].commands instanceof Integer[])) {
                result.addElement(PINInfo[i]);
            }
        }
    }
}
