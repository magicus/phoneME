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

import com.sun.satsa.util.Utils;

import java.util.Vector;
import java.io.IOException;

/**
 * This class represents Access Control Entry.
 */
public class ACEntry {

    /**
     * The list of CA names that correspond to rootId element of ACE.
     */
    private String[] roots;

    /**
     * APDU permissions (command - mask pairs).
     */
    private int[] APDUPermissions;

    /**
     * JCRMI permissions.
     */
    private JCRMIPermission[] JCRMIPermissions;

    /**
     * Constructs ACE.
     * @param r reader for permissions file.
     * @param pin_info vector for PIN information.
     * @throws IOException if I/O error occurs.
     */
    ACEntry(ACLFileReader r, Vector pin_info)
            throws IOException {

        Vector t_roots = new Vector();
        Vector t_apdu = new Vector();
        Vector t_jcrmi = new Vector();

        r.checkWord("{");

        while (true) {

            String s = r.readWord();

            if (s.equals("}")) {
                break;
            }

            if (s.equals("root")) {
                t_roots.addElement(r.readLine());
                continue;
            }

            if (s.equals("apdu")) {
                readAPDUPermission(r, t_apdu);
                continue;
            }

            if (s.equals("jcrmi")) {
                readJCRMIPermission(r, t_jcrmi);
                continue;
            }

            if (s.equals("pin_apdu")) {
                readAPDUPIN(r, pin_info);
                continue;
            }

            if (s.equals("pin_jcrmi")) {
                readJCRMIPIN(r, pin_info);
                continue;
            }

            throw new IOException();
        }

        if (! t_roots.isEmpty()) {
            roots = new String[t_roots.size()];
            for (int i = 0; i < t_roots.size(); i++) {
                roots[i] = (String) t_roots.elementAt(i);
            }
        }

        if (! t_apdu.isEmpty()) {
            APDUPermissions = new int[t_apdu.size() * 2];
            for (int i = 0; i < t_apdu.size(); i++) {
                byte[] data = (byte[]) t_apdu.elementAt(i);
                APDUPermissions[i * 2] = Utils.getInt(data, 0);
                APDUPermissions[i * 2 + 1] = Utils.getInt(data, 4);
            }
        }

        if (! t_jcrmi.isEmpty()) {
            JCRMIPermissions = new JCRMIPermission[t_jcrmi.size()];
            t_jcrmi.copyInto(JCRMIPermissions);
        }
    }

    /**
     * Reads APDU permission from file and places it into the vector.
     * @param r reader for permissions file.
     * @param t_apdu vector for APDU permissions.
     * @throws IOException if I/O error occurs.
     */
    private static void readAPDUPermission(ACLFileReader r, Vector t_apdu)
            throws IOException {

        r.checkWord("{");

        String s = r.readWord();

        while (true) {

            if (s.equals("}")) {
                break;
            }

            byte[] data = new byte[8];

            for (int i = 0; i < 8; i++) {
                data[i] = (byte) Short.parseShort(s, 16);
                s = r.readWord();
            }
            t_apdu.addElement(data);
        }
    }

    /**
     * Reads JCRMI permission from file and places it into the vector.
     * @param r reader for permissions file.
     * @param t_jcrmi vector for JCRMI permissions.
     * @throws IOException if I/O error occurs.
     */
    private static void readJCRMIPermission(ACLFileReader r, 
					    Vector t_jcrmi) 
	throws IOException {

        Vector classes = new Vector();
        Vector methods = new Vector();
        String hashModifier = null;

        r.checkWord("{");

        while (true) {

            String s = r.readWord();

            if (s.equals("}")) {
                break;
            }

            if (s.equals("classes")) {
                r.checkWord("{");
                s = r.readWord();
                while (! s.equals("}")) {
                    classes.addElement(s);
                    s = r.readWord();
                }
            } else
            if (s.equals("hashModifier")) {
                hashModifier = r.readWord();
            } else
            if (s.equals("methods")) {
                r.checkWord("{");
                s = r.readWord();
                while (! s.equals("}")) {
                    methods.addElement(s);
                    s = r.readWord();
                }
            } else {
                throw new IOException();
            }
        }

        t_jcrmi.addElement(new JCRMIPermission(hashModifier, classes, methods));
    }

    /**
     * Reads PIN information from file and adds a new object into vector.
     * @param r reader for permissions file.
     * @param dest destination vector.
     * @throws IOException if I/O error occurs.
     */
    private static void readAPDUPIN(ACLFileReader r, Vector dest)
            throws IOException {

        r.checkWord("{");
        r.checkWord("id");
        int id = r.readByte();
        Integer[] commands = new Integer[ACLPermissions.CMD_COUNT];

        while (true) {

            String s = r.readWord();

            if (s.equals("}")) {
                break;
            }

            int index = getPINCommandIndex(s);

            int command = 0;
            for (int i = 0; i < 4; i++) {
                command = (command << 8) | r.readByte();
            }
            commands[index] = new Integer(command);
        }
        dest.addElement(new PINData(id, commands));
    }

    /**
     * Reads PIN information from file and adds a new object into vector.
     * @param r reader for permissions file.
     * @param dest destination vector.
     * @throws IOException if I/O error occurs.
     */
    private static void readJCRMIPIN(ACLFileReader r, Vector dest)
            throws IOException {

        r.checkWord("{");
        r.checkWord("id");
        int id = r.readByte();
        String[] commands = new String[ACLPermissions.CMD_COUNT];

        while (true) {

            String s = r.readWord();
            if (s.equals("}")) {
                break;
            }
            commands[getPINCommandIndex(s)] = r.readWord();
        }
        dest.addElement(new PINData(id, commands));
    }

    /**
     * Returns PIN operation identifier for given string.
     * @param s operation name.
     * @return PIN operation identifier.
     * @throws IOException if I/O error occurs.
     */
    private static int getPINCommandIndex(String s) throws IOException {

        if (s.equals("verify")) {
            return ACLPermissions.CMD_VERIFY;
        }
        if (s.equals("change")) {
            return ACLPermissions.CMD_CHANGE;
        }
        if (s.equals("disable")) {
            return ACLPermissions.CMD_DISABLE;
        }
        if (s.equals("enable")) {
            return ACLPermissions.CMD_ENABLE;
        }
        if (s.equals("unblock")) {
            return ACLPermissions.CMD_UNBLOCK;
        }
        throw new IOException("Invalid command: " + s);
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
            if (roots[i].equals(root)) {
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
