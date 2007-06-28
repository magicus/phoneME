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
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import com.sun.j2me.io.FileAccess;

import javax.microedition.io.Connector;

import com.sun.j2me.security.TrustedClass;
import com.sun.j2me.security.Token;
import com.sun.satsa.security.SecurityInitializer;

/**
 * This class represents access control file that describes permissions for one
 * card slot.
 */
public class ACSlot{

    /*
     * Inner class to request security token from SecurityTokenInitializer.
     * SecurityTokenInitializer should be able to check this inner class name.
     */
    static private class SecurityTrusted
        implements TrustedClass { };

    /** This class has a different security domain than the App suite */
    private static Token securityToken =
        SecurityInitializer.requestToken(new SecurityTrusted());

    /**
     * Constructs an instance of an access control file object.
     */
    public ACSlot() {
    }
 
    /**
     * Load access control information.
     * @param slotNum card slot number.
     * @return object that contains access control information or null if
     * this information doesn't exist or contains errors.
     */
    public static ACSlot load(int slotNum) {

        FileAccess storage;
        InputStream permIS;

        try {
            String storeName = FileAccess.getStorageRoot(FileAccess.INTERNAL_STORAGE_ID) +
	        "acl_" + slotNum;
            storage = FileAccess.getInstance(storeName, securityToken);
            storage.connect(Connector.READ);
            permIS = storage.openInputStream();
        } catch (IOException e) {
            return null;
        }

        try {
            ACSlot f = new ACSlot();
            f.init(new ACLFileReader(new InputStreamReader(permIS)));
            return f;
        } catch (Exception e) {
            System.out.println("Error reading ACList " + e);
        } finally {
            try {
                storage.disconnect();
            } catch (Exception e) {
                // nothing we can do.
            }
        }
        return null;
    }

    /**
     * The list of ACL objects.
     */
    private Vector ACLists = new Vector();
    /**
     * The list of PIN data objects.
     */
    private Vector PINAttrs = new Vector();

    /**
     * Initializes ACF object.
     * @param r reader for permissions file.
     * @throws IOException if I/O error occurs.
     */
    private void init(ACLFileReader r) throws IOException {

        while (true) {

            ACList acl;
            try {

                String s = r.readWord();

                if (s == null) {
                    break;
                }

                if (s.equals("acf")) {
                    ACLists.addElement(new ACList(r));
                } else
                if (s.equals("pin_data")) {
                    PINAttrs.addElement(new PINAttributes(r));
                } else {
                    throw new Exception();
                }

            } catch (Exception e) {
                throw new IOException("Line " + r.lineNumber);
            }
        }
    }

    /**
     * Return PIN attributes.
     * @param id PIN identifier.
     * @return PIN attributes.
     */
    PINAttributes getPINAttributes(int id) {

        for (int j = 0; j < PINAttrs.size(); j++) {

            PINAttributes p = (PINAttributes) PINAttrs.elementAt(j);
            if (p.id == id) {
                return p;
            }
        }
        return null;
    }

    /**
     * Returns object that should be used for access control verification.
     * @param isAPDU true for APDU connection, false for JCRMI.
     * @param selectAPDU SELECT APDU command data.
     * @param root name of CA that authorized the suite.
     * @return object that can be used to check permissions.
     */
    ACLPermissions getACLPermissions(boolean isAPDU, byte[] selectAPDU,
                                            String root) {

        Vector permissions = new Vector();
        Vector pins = new Vector();
        boolean found = false;
        boolean allow = false;

        for (int i = 0; i < ACLists.size(); i++) {

            ACList acd = (ACList) ACLists.elementAt(i);

            if (! acd.match(selectAPDU)) {
                continue;
            }

            found = true;

            acd.getPINs(isAPDU, pins);

            Vector acl = acd.getACEntries();

            for (int j = 0; j < acl.size(); j++) {

                ACEntry ace = (ACEntry) acl.elementAt(j);

                if (! ace.verifyPrincipal(root)) {
                    continue;
                }

                if (! ace.hasPermissions()) {
                    allow = true;
                    continue;
                }

                ace.getPermissions(isAPDU, permissions);
            }
        }

        ACLPermissions perm;

        if (isAPDU) {
            perm =  new APDUPermissions(this);
        } else {
            perm = new JCRMIPermissions(this);
        }

        if (pins.size() != 0) {
            PINData[] data = new PINData[pins.size()];
            pins.copyInto(data);
            perm.setPINData(data);
        }

        if (! found || allow) {
            perm.setType(ACLPermissions.ALLOW);
        } else
        if (permissions.size() == 0) {
            throw new SecurityException("Access denied.");
        } else {
            perm.setPermissions(permissions);
            perm.setType(ACLPermissions.CHECK);
        }
        return perm;
    }
}
