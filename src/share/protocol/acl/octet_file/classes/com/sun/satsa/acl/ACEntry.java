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

import java.util.Vector;
import java.io.IOException;

/**
 * This class represents Access Control Entry.
 */
public class ACEntry {
    /** ASN context specific constructed explicit flag used in types (0xA0). */
    public static final int CONTEXT_CONSTRUCTED_0 = TLV.CONTEXT +
                                                               TLV.CONSTRUCTED;
    /** ASN context specific constructed explicit flag used in types (0xA1). */
    public static final int CONTEXT_CONSTRUCTED_1 = CONTEXT_CONSTRUCTED_0 + 1;
    /** ASN context specific constructed explicit flag used in types (0xA2). */
    public static final int CONTEXT_CONSTRUCTED_2 = CONTEXT_CONSTRUCTED_0 + 2;
    /** ASN context specific constructed explicit flag used in types (0xA3). */
    public static final int CONTEXT_CONSTRUCTED_3 = CONTEXT_CONSTRUCTED_0 + 3;
    /** ASN context specific constructed explicit flag used in types (0xA3). */
    public static final int CONTEXT_CONSTRUCTED_4 = CONTEXT_CONSTRUCTED_0 + 4;

    /** ASN context specific primitive explicit flag used in types (0x80). */
    public static final int CONTEXT_PRIMITIVE_0 = TLV.CONTEXT;
    /** ASN context specific primitive explicit flag used in types (0x81). */
    public static final int CONTEXT_PRIMITIVE_1 = CONTEXT_PRIMITIVE_0 + 1;
    /** ASN context specific primitive explicit flag used in types (0x82). */
    public static final int CONTEXT_PRIMITIVE_2 = CONTEXT_PRIMITIVE_0 + 2;
    /** ASN context specific primitive explicit flag used in types (0x83). */
    public static final int CONTEXT_PRIMITIVE_3 = CONTEXT_PRIMITIVE_0 + 3;
    /** ASN context specific primitive explicit flag used in types (0x84). */
    public static final int CONTEXT_PRIMITIVE_4 = CONTEXT_PRIMITIVE_0 + 4;

    /** Vector containing parsed the Access Control File */
    private Vector ACF;
    /** The list of CA names that correspond to rootId element of ACE. */
    private byte[][] roots;

    /** APDU permissions (command - mask pairs). */
    private int[] APDUPermissions;

    /** JCRMI permissions. */
    private JCRMIPermission[] JCRMIPermissions;

    /**
     * Creates new ACEntry object
     * @param v Vector containing parsed the Access Control File
     * @param pin_info Vector for pin information
     * @throws TLVException when some error in the TLV structures treatment
     */
    ACEntry(Vector v, Vector pin_info)
            throws TLVException {
        ACF = v;
        readACE(pin_info);

    }
    /**
     * <pre>
     * Reads one ACE.
     * It is assumed that the ACE is in the following format:
     *   SEQUENCE {                   ACE contents
     *     CONTEXT_CONSTRUCTED_0 {      the Principals    (opt)
     *       < read contents >
     *     }
     *     CONTEXT_CONSTRUCTED_1 {      the Permissions   (opt)
     *       < read permissions contents >
     *     }
     *     CONTEXT_CONSTRUCTED_2 {      the userAuthentications    (opt)
     *       < read user authetications contents >
     *     }
     *   }
     * </pre>
     * @param pin_info Vector for pin information.
     * @throws TLVException - exception in case any problems with TLV structure.
     */
    private void readACE(Vector pin_info)
            throws TLVException {

        Vector t_roots = new Vector();
        Vector t_apdu = new Vector();
        Vector t_jcrmi = new Vector();
        TLV root, t;
        root = (TLV)ACF.firstElement();
        while (root != null) {
            if (root.type != TLV.SEQUENCE_TYPE) { // ACE shall be a sequence
                throw new TLVException("The SEQUENCE_TYPE expected");
            }
            t = root.child; // level of the ACE root
            while (t != null) {
                switch (t.type) {
                case CONTEXT_CONSTRUCTED_0: { // principals
                    readPrincipals(t.child, t_roots);
                    break;
                }
                case CONTEXT_CONSTRUCTED_1: { // permissions
                    readPermissions(t.child, t_apdu, t_jcrmi);
                    break;
                }
                case CONTEXT_CONSTRUCTED_2: { // userAuthentications
                    readUserAuthentications(t.child, pin_info);
                    break;
                }
                }
                t = t.next;
            }
            root = root.next;
        }
        if (! t_roots.isEmpty()) {
            roots = new byte[t_roots.size()][];
            for (int i = 0; i < t_roots.size(); i++) {
                roots[i] = (byte[]) t_roots.elementAt(i);
            }
        }

        if (! t_apdu.isEmpty()) {
            APDUPermissions = new int[t_apdu.size()];
            for (int i = 0; i < t_apdu.size(); i++) {
                byte[] data = (byte[]) t_apdu.elementAt(i);
                APDUPermissions[i] = Utils.getInt(data, 0);
            }
        }

        if (! t_jcrmi.isEmpty()) {
            JCRMIPermissions = new JCRMIPermission[t_jcrmi.size()];
            t_jcrmi.copyInto(JCRMIPermissions);
        }
    }

    /**
     * Reads UserAuthentications information from ACF vector.
     * <pre>
     * It is assumed that the ACE is in the following format:
     *     CONTEXT_CONSTRUCTED_2 {      the UserAuthentications
     *           OCTET_STRING                 AuthID
     *           CONTEXT_CONSTRUCTED_0 {        APDUPinEntry choice
     *             < readApduPINs >
     *           }
     *           CONTEXT_CONSTRUCTED_1 {        JCRMIPinEntry choice
     *             < readJcrmiPINs >
     *           }
     *     }
     * </pre>
     * @param t current position in the ACF vector.
     * @param pin_info Vector for the PINs information.
     * @throws TLVException if I/O error occurs.
     */
    private void readUserAuthentications(TLV t, Vector pin_info)
                                                   throws TLVException {
        TLV root = t;

        while (root != null) {
            TLV u = root.child; // authID
            if (u.type != TLV.OCTETSTR_TYPE) { // shall be Octet string type
                throw new TLVException("The OCTETSTR_TYPE expected");
            }
            int id = u.getId();     /* Strictly speaking, the spec. defines */
                                    /* this field as OCTET STRING, but from */
                                    /* other side in methods where this field */
                                    /* is used, it is defined as int. Now */
                                    /* it is supposed that it is integer */

            u = u.next;   // UserAuthenticationMethod CHOICE
            if (u.type == CONTEXT_CONSTRUCTED_0) {  // apduPINEntry
                pin_info.addElement(new PINData(id, readApduPINs(u.child)));
            }
            if (u.type == CONTEXT_CONSTRUCTED_1) {  // jcrmiPINEntry
                pin_info.addElement(new PINData(id, readJcrmiPINs(u.child)));
            }
            root = root.next;
        }
    }
    /**
     * Reads PINs information from ACF vector.
     * <pre>
     *             CONTEXT_CONSTRUCTED_0 {
     *               OCTET STRING (SIZE(4))       verifyPinAPDU (opt)
     *             }
     *             CONTEXT_CONSTRUCTED_1 {
     *               OCTET STRING (SIZE(4))       changePinAPDU (opt)
     *             }
     *             CONTEXT_CONSTRUCTED_2 {
     *               OCTET STRING (SIZE(4))       disablePinAPDU (opt)
     *             }
     *             CONTEXT_CONSTRUCTED_3 {
     *               OCTET STRING (SIZE(4))       enablePinAPDU (opt)
     *             }
     *             CONTEXT_CONSTRUCTED_4 {
     *               OCTET STRING (SIZE(4))       unblockPinAPDU (opt)
     *             }
     * </pre>
     * @param t current position in the ACF vector.
     * @return Integer[] commands.
     * @throws TLVException if I/O error occurs.
     */
    private Integer[] readApduPINs(TLV t)
            throws TLVException {
        TLV root = t;
        byte[] b;
        int command;
        Integer[] commands = new Integer[ACLPermissions.CMD_COUNT];
        int comIndex = 0;
        while (root != null) {
            switch (root.type) {
                case CONTEXT_PRIMITIVE_0: {    /* verifyPinAPDU */
                    comIndex = ACLPermissions.CMD_VERIFY;
                    break;
                }
                case CONTEXT_PRIMITIVE_1: {    /* changePinAPDU */
                    comIndex = ACLPermissions.CMD_CHANGE;
                    break;
                }
                case CONTEXT_PRIMITIVE_2: {    /* disablePinAPDU */
                    comIndex = ACLPermissions.CMD_DISABLE;
                    break;
                }
                case CONTEXT_PRIMITIVE_3: {    /* enablePinAPDU */
                    comIndex = ACLPermissions.CMD_ENABLE;
                    break;
                }
                case CONTEXT_PRIMITIVE_4: {    /* unblockPinAPDU */
                    comIndex = ACLPermissions.CMD_UNBLOCK;
                    break;
                }
            }
            b = root.getValue();
            command = 0;
            for (int i = 0; i < 4; i++) {
                command = (command << 8) | b[i];
            }
            commands[comIndex] = new Integer(command);
            root = root.next;
        }
        return commands;
    }
    /**
     * Reads PINs information from ACF vector.
     * <pre>
     *             CONTEXT_CONSTRUCTED_0 {
     *               UTF8_STRING           verifyPinMethodID (opt)
     *             }
     *             CONTEXT_CONSTRUCTED_0 {
     *               UTF8_STRING           changePinMethodID (opt)
     *             }
     *             CONTEXT_CONSTRUCTED_0 {
     *               UTF8_STRING           disablePinMethodID (opt)
     *             }
     *             CONTEXT_CONSTRUCTED_0 {
     *               UTF8_STRING           enablePinMethodID (opt)
     *             }
     *             CONTEXT_CONSTRUCTED_0 {
     *               UTF8_STRING           unblockPinMethodID (opt)
     *             }
     * </pre>
     * @param t current position in the ACF vector.
     * @return String[] commands.
     * @throws TLVException if I/O error occurs.
     */
    private String[] readJcrmiPINs(TLV t)
            throws TLVException {
        TLV root = t;
        String[] commands = new String[ACLPermissions.CMD_COUNT];
        int comIndex = 0;
        while (root != null) {
            switch (root.type) {
                case CONTEXT_PRIMITIVE_0: {    /* verifyPinMethodID */
                    comIndex = ACLPermissions.CMD_VERIFY;
                    break;
                }
                case CONTEXT_PRIMITIVE_1: {    /* changePinMethodID */
                    comIndex = ACLPermissions.CMD_CHANGE;
                    break;
                }
                case CONTEXT_PRIMITIVE_2: {    /* disablePinMethodID */
                    comIndex = ACLPermissions.CMD_DISABLE;
                    break;
                }
                case CONTEXT_PRIMITIVE_3: {    /* enablePinMethodID */
                    comIndex = ACLPermissions.CMD_ENABLE;
                    break;
                }
                case CONTEXT_PRIMITIVE_4: {    /* unblockPinMethodID */
                    comIndex = ACLPermissions.CMD_UNBLOCK;
                    break;
                }
            }
            commands[comIndex] = root.getUTF8();
            root = root.next;
        }
        return commands;
    }

    /**
     * Reads the Permissions information from the ACF vector.
     * <pre>
     *           CONTEXT_CONSTRUCTED_0 {        APDUMaskPermission choice
     *             < readAPDUPermissions >
     *           }
     *           CONTEXT_CONSTRUCTED_1 {        JCRMIPermission choice
     *             < readJCRMIPermissions >
     *           }
     * </pre>
     * @param t current position in the ACF vector.
     * @param apdu Vector for the apdu permission information.
     * @param jcrmi Vector for the jcrmi permission information.
     * @throws TLVException if I/O error occurs.
     */
    private void readPermissions(TLV t, Vector apdu, Vector jcrmi)
                                                          throws TLVException {
        TLV root = t;
        while (root != null) {
            if (root.type == CONTEXT_CONSTRUCTED_0) {
                readAPDUPermissions(root.child, apdu);
            }
            if (root.type == CONTEXT_CONSTRUCTED_1) {
                readJCRMIPermissions(root.child, jcrmi);
            }
            root = root.next;
        }
    }

    /**
     * Reads the JCRMI Permissions information from the ACF vector.
     * <pre>
     *               SEQUENCE OF {         ClassList
     *                 UTF8_STRING           Class
     *               }
     *               UTF8_STRING           hashModifier (opt)
     *               SEQUENCE OF {         MethodIDList (opt)
     *                 OCTET_STRING (SIZE(4))  MethodID
     *               }
     * </pre>
     * @param t current position in the ACF vector.
     * @param jcrmi Vector for the jcrmi permission information.
     * @throws TLVException if I/O error occurs.
     */
    private void readJCRMIPermissions(TLV t, Vector jcrmi)
                                                   throws TLVException {
        Vector classes = new Vector();
        Vector methods = new Vector();
        String hashModifier = null;

        TLV root = t;
        /* reading of classes */
        if (root.type != TLV.SEQUENCE_TYPE) {
                // classes shall be a sequence of UTF8String
            throw new TLVException("The SEQUENCE_TYPE expected");
        }
        TLV c = root.child;
        while (c != null) {
            classes.addElement(c.getUTF8());
            c = c.next;
        }
        root = root.next;
        if (root != null) {
            /* reading of hashModifier */
            if (root.type == TLV.UTF8STR_TYPE) {
                    // hashModifier is optional and has UTF8String type
                hashModifier = root.getUTF8();
                root = root.next;
            }
            if (root != null) {
                /* reading of methods */
                if (root.type != TLV.SEQUENCE_TYPE) {
                        // methods shall be sequence of OCTET_STRING
                    throw new TLVException("The SEQUENCE_TYPE expected");
                }
                TLV m = root.child;
                while (m != null) {
                    methods.addElement(m.getValue());
                    m = m.next;
                }
            }
        }
        jcrmi.addElement(new JCRMIPermission(hashModifier, classes, methods));
    }

    /**
     * Reads the APDU Permissions information from the ACF vector.
     * <pre>
     *               OCTET_STRING (SIZE(4))  APDUHeader
     *               OCTET_STRING (SIZE(4))  APDUMask
     *             }
     * </pre>
     * @param t current position in the ACF vector.
     * @param apdu Vector for the apdu permission information.
     * @throws TLVException if I/O error occurs.
     */
    private void readAPDUPermissions(TLV t, Vector apdu)
                                                 throws TLVException {
        TLV root = t;
        byte[] data;

        while (root != null) {
            data = root.getValue();
            apdu.addElement(data);
            root = root.next;
        }
    }

    /**
     * Reads the Principals information from the ACF vector.
     * <pre>
     * It is assumed that the ACE is in the following format:
     *     CONTEXT_CONSTRUCTED_0 {      the Principals
     *       SEQUENCE OF {                Principal contents
     *         CONTEXT_CONSTRUCTED_0        rootID
     *           OCTET_STRING
     *         CONTEXT_CONSTRUCTED_1        endEntityID
     *           OCTET_STRING
     *         CONTEXT_CONSTRUCTED_2        domain
     *           OBJECT_IDENTIFIER
     *       }
     *     }
     * </pre>
     * @param t current position in the ACF vector.
     * @param v Vector for the principals information.
     * @throws TLVException if I/O error occurs.
     *
     */
    private void readPrincipals(TLV t, Vector v)
                                               throws TLVException {
        TLV root = t;
        while (root != null) {
            v.addElement(root.child.getValue());    // UNIVERSAL type level
            root = root.next;
        }
    }

    /**
     * Verifies if this ACE describes permissions for this CA.
     * @param root name of CA that authorized the suite.
     * @return true if this ACE describes permissions for this CA.
     */
    boolean verifyPrincipal(String root) {
        if (roots == null) {
            return true;
        }

        for (int i = 0; i < roots.length; i++) {
            if (root.equals(new String(roots[i]))) {
                return true;
            }
        }

        return false;
    }

    /**
     * Verifies if the ACE contains permissions.
     * @return true if the ACE contains permissions.
     */
    boolean hasPermissions() {
        return (APDUPermissions != null || JCRMIPermissions != null);
    }

    /**
     * Places permissions from this ACE to the vector.
     * @param isAPDU if true, place APDU permissions, otherwise - JCRMI
     * permissions
     * @param permissions the vector for results
     */
    void getPermissions(boolean isAPDU, Vector permissions) {

        if (isAPDU) {
            if (APDUPermissions != null) {
                permissions.addElement(APDUPermissions);
            }
        } else {
            if (JCRMIPermissions != null) {
                for (int k = 0; k < JCRMIPermissions.length; k++) {
                    permissions.addElement(JCRMIPermissions[k]);
                }
            }
        }
    }
}
