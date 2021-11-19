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
 * This class represents the SATSA ACIF file abstraction
 */
public class ACIF extends PKCS15File {

    /** This vector contains parsed objects from ACIF. */
    private Vector ACIF;
    /** This vector contains the entries from the ACIF file */
    private Vector ACFs = new Vector();

    /**
     * Creates AODF object for the pointed location and file system
     * @param location Location required location
     * @param files AclFileSystem requred file system
     */
    public ACIF(Location location, AclFileSystem files) {
        super(location, files);
    }

    /**
     * Loads ACIF object from the file system
     * @throws IOException if I/O error occurs
     * @throws TLVException if TLV error occurs
     */
    public void load() throws IOException, TLVException {
        ACIF = new Vector();
        resetLoader(ACIF, null, null);
        parseDF(location.path);
        readACIF();
    }

    /**
     * Reads ACIF data from the ACIF vector.
     * @throws IOException if I/O error occurs
     * @throws TLVException if TLV error occurs
     * <pre>
     * It is assumed that the ACIF file contains information about ACF files
     * in the following format:
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
     * </pre>
     */
    private void readACIF() throws IOException, TLVException {
        for (int i = 0; i < ACIF.size(); i++) {
            ACF acf = new ACF();
            TLV t = ((TLV)ACIF.elementAt(i)).child;
            if (t.type == TLV.OCTETSTR_TYPE) {         /* aid */
                acf.setAID(t.getValue());
                t = t.next;
            }
            if (t.type == TLV.SEQUENCE_TYPE) {       /* location */
                acf.setPath(files.pathToLocation(t));
            }
            ACFs.addElement(acf);
        }
    }

    /**
     * Returns number of entries in the ACIF file
     * @return int
     */
    public int getACFCount() {
        return ACFs.size();
    }

    /**
     * Returns AID of the pointed entry (ACF)
     * @param index int index of required entry
     * @return byte[] AID of the required entry
     */
    public byte[] getAID(int index) {
        return ((ACF)ACFs.elementAt(index)).getAID();
    }

    /**
     * Returns location of the pointed ACF
     * @param index int index of the required ACF
     * @return Location of the required ACF
     */
    public Location getACFPath(int index) {
        return ((ACF)ACFs.elementAt(index)).getPath();
    }

    /**
     * This class represents the entry of the ACIF file
     *
     */
    private class ACF {
        /** AID of the entry */
        private byte[] AID;
        /** location of the ACF */
        private Location location;

        /**
         * Sets AID for the entry
         * @param aid byte[] required AID
         */
        public void setAID(byte[] aid) {
            AID = aid;
        }

        /**
         * Returns AID of the entry
         * @return byte[] required AID
         */
        public byte[] getAID() {
            return AID;
        }

        /**
         * Sets location for the entry
         * @param location Location required location
         */
        public void setPath(Location location) {
            this.location = location;
        }

        /**
         * Returns location of the entry
         * @return Location
         */
        public Location getPath() {
            return location;
        }
    }
}
