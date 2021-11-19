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
 * This class represents access control list (ACL).
 */
public class ACList {

    /**
     * AID for this ACL.
     */
    private byte[] AID;
    /**
     * The list of ACEs.
     */
    private Vector ACEntries = new Vector();

    /** The first ACF file number */
    private static final short FIRST_ACF_FAIL = 0x5310;
    /** number of ACF file */
    private short num;
    /** ACIF record */
    private TLV acifRec;

    /**
     * Constructs ACL object.
     * @param r reader for permissions file.
     * @param num path to ACIF.
     * @throws IOException if I/O error occurs.
     */
    ACList(ACLFileReader r, short num) throws IOException {

        byte [] aid = new byte[16];

        int count = 0;
        String s = r.readWord();
        while (! s.equals("{")) {
            aid[count++] = (byte) Short.parseShort(s, 16);
            s = r.readWord();
        }

        if (count > 0) {
            AID = new byte[count];
            System.arraycopy(aid, 0, AID,  0, count);
        }

        Vector pin_info = new Vector();

        while (true) {

            s = r.readWord();

            if (s.equals("}")) {
                break;
            }

            if (! s.equals("ace")) {
                throw new IOException();
            }

            ACEntries.addElement(new ACEntry(r, pin_info));
        }

        this.num = num;
        createACIFRec(AID, num);

    }

    /**
     * SEQUENCE OF {
     *   SEQUENCE {
     *     OCTET_STRING         // aid    (Optional)
     *     SEQUENCE {
     *       OCTET_STRING      // path
     *       INTEGER           // index
     *       CONTEXT_PRIMITIVE_0 {  ???
     *         INTEGER         // length
     *       }
     *     }
     *   }
     * }
     * AID ::= OCTET STRING
     */
    /**
     * Creates ACIF record.
     * @param AID aid of record
     * @param num path to ACIF
     */
    private void createACIFRec(byte[] AID, short num) {
        acifRec = TLV.createSequence();    /* SEQUENCE */
        TLV current = acifRec.setChild(new TLV(TLV.NULL_TYPE));
        TLV t;
        if (AID != null) {
            current = current.setNext(TLV.createOctetString(AID));
        }
        current = current.setNext(TLV.createSequence());
        t = TLV.createOctetString(Utils.shortToBytes(num)); /* path */
        current.setChild(t);

        acifRec.child = acifRec.child.next;
    }

    /**
     * Returns ACIF record
     * @return acifRec
     */
    public TLV getACIFRec() {
        return acifRec;
    }

    /**
     * Returns the list of ACEs.
     * @return the list of ACEs.
     */
    public Vector getACEntries() {
        return ACEntries;
    }

    /**
     * Returns the num value
     * @return the num value
     */
    short getNum() {
        return num;
    }
}
