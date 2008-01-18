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

package acl_data;


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
    /** ASN context specific constructed explicit flag used in types (0xA4). */
    public static final int CONTEXT_CONSTRUCTED_4 = CONTEXT_CONSTRUCTED_0 + 4;
    /** ASN context specific constructed explicit flag used in types (0xA5). */
    public static final int CONTEXT_CONSTRUCTED_5 = CONTEXT_CONSTRUCTED_0 + 5;
    /** ASN context specific constructed explicit flag used in types (0xA6). */
    public static final int CONTEXT_CONSTRUCTED_6 = CONTEXT_CONSTRUCTED_0 + 6;
    /** ASN context specific constructed explicit flag used in types (0xA7). */
    public static final int CONTEXT_CONSTRUCTED_7 = CONTEXT_CONSTRUCTED_0 + 7;
    /** ASN context specific constructed explicit flag used in types (0xA8). */
    public static final int CONTEXT_CONSTRUCTED_8 = CONTEXT_CONSTRUCTED_0 + 8;

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
    /** ASN context specific primitive explicit flag used in types (0x85). */
    public static final int CONTEXT_PRIMITIVE_5 = CONTEXT_PRIMITIVE_0 + 5;
    /** ASN context specific primitive explicit flag used in types (0x86). */
    public static final int CONTEXT_PRIMITIVE_6 = CONTEXT_PRIMITIVE_0 + 6;
    /** ASN context specific primitive explicit flag used in types (0x87). */
    public static final int CONTEXT_PRIMITIVE_7 = CONTEXT_PRIMITIVE_0 + 7;
    /** ASN context specific primitive explicit flag used in types (0x88). */
    public static final int CONTEXT_PRIMITIVE_8 = CONTEXT_PRIMITIVE_0 + 8;

    /** The list of CA names that correspond to rootId element of ACE. */
    private String[] roots;
    /** ACE record */
    private TLV aceRec = null;

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

        TLV perm = new TLV(CONTEXT_CONSTRUCTED_1);       /* the Permissions */
        perm.setChild(new TLV(TLV.NULL_TYPE));
        TLV tr = perm.child;
        TLV pin = new TLV(CONTEXT_CONSTRUCTED_2); /* the UserAuthentications */
        pin.setChild(new TLV(TLV.NULL_TYPE));
        TLV tp = pin.child;
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
                readAPDUPermission(r, t_apdu, tr);
                tr = tr.next;
                continue;
            }

            if (s.equals("jcrmi")) {
                readJCRMIPermission(r, t_jcrmi, tr);
                tr = tr.next;
                continue;
            }

            if (s.equals("pin_apdu")) {
                readAPDUPIN(r, pin_info, tp.setNext(TLV.createSequence()));
                tp = tp.next;
                continue;
            }

            if (s.equals("pin_jcrmi")) {
                readJCRMIPIN(r, pin_info, tp.setNext(TLV.createSequence()));
                tp = tp.next;
                continue;
            }

            throw new IOException();
        }
        aceRec = TLV.createSequence();                 /* ACE content */
        if (! t_roots.isEmpty()) {
            TLV t;
            roots = new String[t_roots.size()];
            t = new TLV(CONTEXT_CONSTRUCTED_0);       /* the Principals */
            t.setChild(new TLV(TLV.NULL_TYPE));
            for (int i = 0; i < t_roots.size(); i++) {
                roots[i] = (String) t_roots.elementAt(i);
                createACERoot(t.child.child, roots[i]);
            }
            t.child = t.child.next;
            aceRec.setChild(t);
        }

        if ((! t_apdu.isEmpty()) || (! t_jcrmi.isEmpty())) {
            perm.child = perm.child.next;
            if (aceRec.child != null) {
                aceRec.child.setNext(perm);
            } else {
                aceRec.setChild(perm);
            }
        }
        if (!pin_info.isEmpty()) {
            pin.child = pin.child.next;
            if (aceRec.child != null) {
                aceRec.child.setNext(pin);
            } else {
                aceRec.setChild(pin);
            }
        }

        if (aceRec != null) {
            aceRec.getValue();
        }

    }

    /**
     * SEQUENCE OF {                sequence of ACE
     *   SEQUENCE {                   ACE contents
     *     CONTEXT_CONSTRUCTED_0 {      the Principals    (opt)
     *       SEQUENCE OF {                Principal contents
     *         CONTEXT_CONSTRUCTED_0        rootID
     *           OCTET_STRING
     *         CONTEXT_CONSTRUCTED_1        endEntityID
     *           OCTET_STRING
     *         CONTEXT_CONSTRUCTED_2        domain
     *           OBJECT_IDENTIFIER
     *       }
     *     }
     *     CONTEXT_CONSTRUCTED_1 {      the Permissions   (opt)
     *       SEQUENCE OF {                Permissions content
     *         SEQUENCE {                   Permission content
     *           CONTEXT_CONSTRUCTED_0 {        APDUMaskPermission choice
     *             SEQUENCE {            APDUPermission
     *               OCTET_STRING (SIZE(4))  APDUHeader
     *               OCTET_STRING (SIZE(4))  APDUMask
     *               }
     *             }
     *           }
     *           CONTEXT_CONSTRUCTED_1 {        JCRMIPermission choice
     *             SEQUENCE {            JCRMIPermission
     *               SEQUENCE OF {         ClassList
     *                 UTF8_STRING           Class
     *               }
     *               UTF8_STRING           hashModifier (opt)
     *               SEQUENCE OF {         MethodIDList (opt)
     *                 OCTET_STRING (SIZE(4))  MethodID
     *               }
     *             }
     *           }
     *         }
     *       }
     *     }
     *     CONTEXT_CONSTRUCTED_2 {      the userAuthentications    (opt)
     *       < read user authetications contents >
     *     }
     *   }
     */
    /**
     * Creates ACE root structure
     * @param t current pointer to TLV.
     * @param root required root.
     */
    private void createACERoot(TLV t, String root) {

        t.setNext(new TLV(CONTEXT_CONSTRUCTED_0)).
               setChild(TLV.createOctetString(Utils.stringToBytes(root)));
    }

    /**
     * Returns ACE record
     * @return TLV
     */
    public TLV getACERec() {
        return aceRec;
    }

    /**
     * Reads APDU permission from file and places it into the vector.
     * @param r reader for permissions file.
     * @param t_apdu vector for APDU permissions.
     * @throws IOException if I/O error occurs.
     *     CONTEXT_CONSTRUCTED_1 {      the Permissions   (opt)
     *       SEQUENCE OF {                Permissions content
     *           CONTEXT_CONSTRUCTED_0 {        APDUMaskPermission choice
     *             SEQUENCE {            APDUPermission
     *               OCTET_STRING (SIZE(4))  APDUHeader
     *               OCTET_STRING (SIZE(4))  APDUMask
     *               }
     *             }
     *           }
     */
    private static void readAPDUPermission(ACLFileReader r,
                                           Vector t_apdu, TLV t)
            throws IOException {
        TLV ta = t;

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

            ta = ta.setNext(new TLV(CONTEXT_CONSTRUCTED_0));
            byte[] ah = new byte[4];
            byte[] am = new byte[4];
            for (int i = 0; i < 4; i++) {
                ah[i] = data[i];
                am[i] = data[i + 4];
            }
            ta.setChild(TLV.createOctetString(ah)).
                setNext(TLV.createOctetString(am));
        }
    }

    /**
     * Reads JCRMI permission from file and places it into the vector.
     * @param r reader for permissions file.
     * @param t_jcrmi vector for JCRMI permissions.
     * @throws IOException if I/O error occurs.
     *             SEQUENCE OF {         ClassList
     *               UTF8_STRING           Class
     *             }
     *             UTF8_STRING           hashModifier (opt)
     *             SEQUENCE OF {         MethodIDList (opt)
     *               OCTET_STRING (SIZE(4))  MethodID
     *             }     */
    private static void readJCRMIPermission(ACLFileReader r,
					    Vector t_jcrmi, TLV t)
	throws IOException {

        Vector classes = new Vector();
        Vector methods = new Vector();
        String hashModifier = null;

        r.checkWord("{");

        TLV tj = new TLV(CONTEXT_CONSTRUCTED_1);
        t.setNext(tj);

        TLV c = TLV.createSequence();
        tj.setChild(c);
        c = c.setChild(new TLV(TLV.NULL_TYPE));
        TLV m = null;
        TLV h = null;
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
                    c.setNext(TLV.createUTF8String(s));
                    c = c.next;
                    s = r.readWord();
                }
            } else
            if (s.equals("hashModifier")) {
                hashModifier = r.readWord();
                h = TLV.createUTF8String(hashModifier);
                tj.child.setNext(h);
            } else
            if (s.equals("methods")) {
                if (m == null) {
                    m = TLV.createSequence();
                    m.setChild(new TLV(TLV.NULL_TYPE));
                    if (h != null) {
                        h.setNext(m);
                    } else {
                        tj.child.setNext(m);
                    }
                    m = m.child;
                }
                r.checkWord("{");
                s = r.readWord();
                while (! s.equals("}")) {
                    methods.addElement(s);
                    m.setNext(TLV.createOctetString(
                            getMethodId(hashModifier, s), 0, 4));
                    m = m.next;
                    s = r.readWord();
                }
            } else {
                throw new IOException();
            }
        }
        if (m != null) {
            if (h != null) {
                h.next.child = h.next.child.next;
            } else {
                tj.child.next.child = tj.child.next.child.next;
            }
        }

        tj.child.child = tj.child.child.next;
        t_jcrmi.addElement(new JCRMIPermission(hashModifier, classes, methods));
    }

    /**
     * Calculates method ID for given hash modifier and method name.
     * @param hashModifier hash modifier value.
     * @param method method name and signature.
     * @return the identifier.
     */
    private static byte[] getMethodId(String hashModifier, String method) {

        if (hashModifier != null) {
            method = hashModifier + method;
        }
        byte data[] = Utils.stringToBytes(method);
        data = Utils.getHash(data, 0, data.length);
        return data;
    }

    /**
     * Reads PIN information from file and adds a new object into vector.
     * @param r reader for permissions file.
     * @param dest destination vector.
     * @param t current pointer to TLV.
     * @throws IOException if I/O error occurs.
     */
    private static void readAPDUPIN(ACLFileReader r, Vector dest, TLV t)
            throws IOException {

        r.checkWord("{");
        r.checkWord("id");
        int id = r.readByte();
        Integer[] commands = new Integer[ACLPermissions.CMD_COUNT];
        t = t.setChild(TLV.createOctetString(new byte[] { (byte)id })).
            setNext(new TLV(CONTEXT_CONSTRUCTED_0));
        TLV ts = t;
        ts = ts.setChild(new TLV(TLV.NULL_TYPE));
        while (true) {

            String s = r.readWord();

            if (s.equals("}")) {
                break;
            }

            int index = getPINCommandIndex(s);

            int command = 0;
            byte [] b = new byte[4];
            for (int i = 0; i < 4; i++) {
                b[i] = (byte)r.readByte();
                command = (command << 8) | b[i];
            }
            commands[index] = new Integer(command);
            ts.setNext(new TLV(0x80+index, b));
            ts = ts.next;
        }
        t.child = t.child.next;
        dest.addElement(new PINData(id, commands));
    }

    /**
     * Reads PIN information from file and adds a new object into vector.
     * @param r reader for permissions file.
     * @param dest destination vector.
     * @param t current pointer to TLV.
     * @throws IOException if I/O error occurs.
     */
    private static void readJCRMIPIN(ACLFileReader r, Vector dest, TLV t)
            throws IOException {

        r.checkWord("{");
        r.checkWord("id");
        int id = r.readByte();
        String[] commands = new String[ACLPermissions.CMD_COUNT];
        t = t.setChild(TLV.createOctetString(new byte[] { (byte)id })).
            setNext(new TLV(CONTEXT_CONSTRUCTED_1));
        TLV ts = t;
        ts = ts.setChild(new TLV(TLV.NULL_TYPE));
        while (true) {

            String s = r.readWord();
            if (s.equals("}")) {
                break;
            }
            int index = getPINCommandIndex(s);
            commands[index] = r.readWord();
            ts.setNext(new TLV(0x80+index,
                               Utils.stringToBytes(commands[index])));
            ts = ts.next;
        }
        t.child = t.child.next;
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
}
