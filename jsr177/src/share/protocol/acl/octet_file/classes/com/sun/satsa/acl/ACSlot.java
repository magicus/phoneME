/*
 *   
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

import java.io.IOException;

import java.util.Vector;
import com.sun.satsa.util.*;

import com.sun.satsa.util.pkcs15.*;
import com.sun.cdc.io.j2me.apdu.*;

import javax.microedition.io.ConnectionNotFoundException;
/**
 * This class represent the ACL Card slot abstraction
 */
public class ACSlot{

    /**
     * Value of OID from the spceification (A.4.2.1 Location of Access
     * Control Files)
     */
    private byte[] ACIFOID = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x2a, 0x02, 0x6e,
                             0x03, 0x01, 0x01, 0x01};

    /** Path to ODF. */
    short[] ODF = {0x5031};
    /** Pathes to the standard PKCS15 files */
    /** Path to PrKDF. */
    public static final short[] PrKDF = {0x5200};
    /** Path to PuKDF. */
    public static final short[] PuKDF = {0x5201};
    /** Path to Trusted PuKDF. */
    public static final short[] TPuKDF = {0x5202};
    /** Path to SeKDF. */
    public static final short[] SeKDF = {0x5203};
    /** Path to PuKDF. */
    public static final short[] CDF = {0x5204};
    /** Path to CDF for trusted certificates. */
    public static final short[] TrCDF = {0x5205};
    /** Path to CDF for useful certificates. */
    public static final short[] UsCDF = {0x5206};
    /** Path to DODF. */
    public static final short[] DODF = {0x5207};
    /** Path to AODF. */
    public static final short[] AODF = {0x5208};

    /** Path to ACIF */
    public static final short[] ACIFILE = {0x5300};
    /** path to the first ACF */
    public static final short[] ACFILE = {0x5310};

    /** Command for the PKCS15 application select */
    static byte[] selectPKCSApp = {
                                  0x00, (byte) 0xA4, 0x04, Constants.P2, 0x0C,
                                  (byte) 0xa0, 0x0, 0x0, 0x0, 0x63, 0x50, 0x4b,
                                  0x43, 0x53, 0x2d, 0x31, 0x35
    };
    /** Command for the MF select */
    static byte[] selectMF = {0x00, (byte) 0xA4, 0x00, Constants.P2, 0x02, 0x3F,
                              0x00,  0x7F};

    /** Command for the DIR file select */
    static byte[] selectDIR = {
        0x00, (byte)0xA4, Constants.P1, Constants.P2, 0x02, 0x2F, 0x00, 0x7F};
    /** File not found return code */
    static short FILE_NOT_FOUND = 0x6A82;
    /**
     * The list of ACL objects.
     */
    private Vector ACLists = new Vector();
    /**
     * The list of PIN data objects.
     */
    private Vector PINAttrs = new Vector();
    /** File system object. */
    private AclFileSystem files;
    /** Connection object. */
    private static Connection apdu;
    /** Indicates if all permissions granted */
    private boolean allGranted;
    /** Indicates if all permissions revoked */
    private boolean allRevoked;
    /**
     * Constructs an instance of an access control file object.
     */
    public ACSlot() {
    }
    /**
     * Creates new ACSlot object for pointed connection.
     * @param apdu Connection Required connection.
     * @throws IOException any IO exception
     */
    private ACSlot(Connection apdu) throws IOException {
        this.apdu = apdu;
        files = new AclFileSystem(apdu);
    }

    /**
     * Creates new ACSlot object with pointed state of permissions.
     * @param allGranted boolean state of all permissions:
     *        true - all permissions granted,
     *        false - all permissions revoked.
     */
    private ACSlot(boolean allGranted) {
        this.allGranted = allGranted;
        this.allRevoked = !allGranted;
    }

    /**
     * Load access control information.
     * @param slotNumber card slot number.
     * @return object that contains access control information or null if
     * this information doesn't exist or contains errors.
     */
    public static ACSlot load(int slotNumber) {
        Handle h;
        boolean isDIR = false;
        try {
            /* attempt to select the MF file */
            h = APDUManager.openACLConnection(selectMF, slotNumber);
            selectDIR[0] = (byte)(h.channel);
            byte[] res = APDUManager.exchangeAPDU(h, selectDIR);
            if (Utils.getShort(res, 0) == FILE_NOT_FOUND) {
                throw new ConnectionNotFoundException("DIR is not found");
            }
            isDIR = true;
        } catch (ConnectionNotFoundException ce) {
            /* DIR is not found */
            try {
                /* attempt to select the PKCS15 application */
                h = APDUManager.openACLConnection(selectPKCSApp, slotNumber);
                isDIR = false;
            } catch (ConnectionNotFoundException ce1) {
                /* PKCS15Application is not found */
                return null;  /* DIR & PKCS15App are not found => */
                              /* all permissions granted */
            } catch (Exception ie) { 
                /* Something wrong during ACL treatment => */
                /* all permissions revoked */
                ACSlot s = new ACSlot(false);
                return s;
            }
        } catch (Exception e) { /* Something wrong during ACL treatment => */
                                /* all permissions revoked */
            ACSlot s = new ACSlot(false);
            return s;
        }

        Connection con = null;
        try {
            con = new Connection(h);
            ACSlot s = new ACSlot(con);
            s.init(isDIR);
            return s;
        } catch (Exception e) { /* Something wrong during ACL treatment => */
                                /* all permissions revoked */
            ACSlot s = new ACSlot(false);
            return s;
        } finally {
            if (con != null) {
                con.done();
            }
        }
    }

    /**
     * Initializes ACSlot object.
     * @param dirIsUsed boolean indicates if the card file system and DIR file
     *                          is used
     * @throws IOException  any IO exception
     * @throws TLVException any TLV exception
     */
    private void init(boolean dirIsUsed) throws IOException, TLVException {
        if (dirIsUsed) {
            int res = files.DIR.setPKCSRoot();
            if (res < 0) {
                throw new IOException("Wrong DIR file");
            }
            allGranted = (res == files.DIR.AID_NOT_FOUND);
            allRevoked = (res == files.DIR.ROOT_NOT_SELECTED);
        }
        ODF odf = new ODF(files);
        odf.load();
        Vector ACIFs = odf.getDOFs(ACIFOID);
        for (int i = 0; i < ACIFs.size(); i++) {
            ACIF acif = new ACIF(files.pathToLocation((TLV) ACIFs.elementAt(i)),
                                 files);
            acif.load();
            for (int j = 0; j < acif.getACFCount(); j++) {
                ACLists.addElement(new ACList(acif.getAID(j),
                                              acif.getACFPath(j), files));
            }
        }
        /*  loadPINs(); */
        for (int i = 0; i < odf.getAODFCount(); i++) {
            AODF aodf = new AODF(odf.getAODFPath(i), files);
            aodf.load();
            for (int j = 0; j < aodf.getEntryCount(); j++) {
                PINAttrs.addElement(new PINAttributes(aodf.getEntry(j)));
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

        ACLPermissions perm;

        if (isAPDU) {
            perm = new APDUPermissions(this);
        } else {
            perm = new JCRMIPermissions(this);
        }

        if (allGranted) {
            /* Previously was decided to grant all permissions */
            perm.setType(ACLPermissions.ALLOW);
            return perm;
        }
        if (allRevoked) {
            /* Previously was decided to revoke all permissions */
            perm.setType(ACLPermissions.DISALLOW);
            return perm;
        }

        for (int i = 0; i < ACLists.size(); i++) {
            ACList acl = (ACList) ACLists.elementAt(i);
            if (!acl.match(selectAPDU)) {
                continue;
            }
            found = true;
            ACEntry ace = acl.getACEntries();
            if (ace == null) {
                /*
                 * Case when there is the aid identifies the application in the
                 * ACIF file, but an ACF related to the application is absent
                 * in this case the application does not have access to the card
                 */
                continue;
            }
            acl.getPINs(isAPDU, pins);
            if (!ace.verifyPrincipal(root)) {
                continue;
            }
            if (!ace.hasPermissions()) {
                allow = true;
                continue;
            }
            ace.getPermissions(isAPDU, permissions);
        }

        if (pins.size() != 0) {
            PINData[] data = new PINData[pins.size()];
            pins.copyInto(data);
            perm.setPINData(data);
        }

        if (!found || allow) {
            perm.setType(ACLPermissions.ALLOW);
        } else {
            if (permissions.size() == 0) {
                throw new SecurityException("Access denied.");
            } else {
                perm.setPermissions(permissions);
                perm.setType(ACLPermissions.CHECK);
            }
        }
        return perm;
    }
}
