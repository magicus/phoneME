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
package com.sun.satsa.util.pkcs15;

import java.util.Vector;
import java.io.IOException;
import com.sun.satsa.util.*;

/**
 * This class represents the PKCS15 DODF file.
 * <pre>
 * It is assumed that the DODF file has the following content:
 * opaqueDO:       <value>
 * externalIDO:    0x80 <len> <value>
 * oidDO:          0xA1 <len+2>
 *                       TLV.CONSTRUCTED+TLV.SEQUENCE_TYPE <len>
 *                           TLV.OID_TYPE
 *                           TLV.OCTETSTR_TYPE or something else
 * </pre>
 */
public class DODF extends PKCS15File {
    /**
     * <pre>
     * DODF entries tags.
     * These tags means the context-specific constructed type and tag of
     * DataType ::= CHOICE {
     *      opaqueDO    DataObject {Opaque},
     *      externalIDO [0] DataObject {ExternalIDO},
     *      oidDO       [1] DataObject {OidDO},
     * }
     * </pre>
     */
    /** opaqueDO DODF entry tag. */
    public static final int DODFTAG_OPAQUEDO         = TLV.SEQUENCE_TYPE;
    /** externalIDO DODF entry tag. */
    /* It is assumed that this type is primitive (not constructed) */
    public static final int DODFTAG_EXTIDO         = 0x80;
    /** oidDO DODF entry tag. */
    public static final int DODFTAG_OIDDO         = 0xA1;

    /** This vector contains parsed objects from DODF. */
    private Vector DODF;
    /** Staff variable used in the readDODF function */
    private TLV typeTLV;
    /** Staff variable used for the OidDo object store */
    private TLV oidTLV;
    /** Staff variable used for the ExtIDo object store */
    private TLV valueTLV;

    /** This vector contains parsed OidDo objects from DODF. */
    private Vector OidDo = new Vector();
    /** This vector contains parsed ExtIDo objects from DODF. */
    private Vector ExtIDo = new Vector();
    /** This vector contains parsed OpaqueDo objects from DODF. */
    private Vector OpaqueDo = new Vector();

    /**
     * Creates DODF object for the pointed location and file system
     * @param location Location required location
     * @param files FileSystemAbstract requred file system
     */
    public DODF(Location location, FileSystemAbstract files) {
        super(location, files);
    }

    /**
     * Loads DODF object from the file system
     * @throws IOException if I/O error occurs
     * @throws TLVException if TLV error occurs
     */
    public void load() throws IOException, TLVException {
        DODF = new Vector();
        resetLoader(DODF, null, null);
        parseDF(location.path);
        readDODF();
    }

    /**
     * <pre>
     * Reads DODF data from the DODF vector.
     * The DODF file contains information about DO files in the following
     * format:
     *   CONTEXT_CONSTRUCTED_0 {      the ExternalIDO
     *     <Object value>
     *   }
     *   CONTEXT_CONSTRUCTED_1 {      the oidDO
     *         OID_TYPE                Object ID
     *         <Object value>
     *     }
     *   }
     * }
     * </pre>
     */
    private void readDODF() {
        TLV typeTLV = (TLV) DODF.firstElement();

        while (typeTLV != null) {
            switch (typeTLV.type) {
                case DODFTAG_EXTIDO: {
                    valueTLV = typeTLV;
                    ExtIDo.addElement(valueTLV);
                    break;
                }
                case DODFTAG_OIDDO: {
                    oidTLV = typeTLV.child.next.next.child.child;
                    OidDo.addElement(oidTLV);
                    break;
                }
                default: {
                    OpaqueDo.addElement(typeTLV);
                    break;
                }
            }
            typeTLV = typeTLV.next;
        }
    }
    /**
     * Returns the number of the OidDo objects in the DODF file
     * @return int number
     */
    public int getOidDoNumber() {
        return OidDo.size();
    }
    /**
     * Returns OID of the pointed OidDo object
     * @param index int index of the OidDo object
     * @return byte[] OID
     */
    public byte[] getOid(int index) {
        return ((TLV)OidDo.elementAt(index)).getValue();
    }
    /**
     * Returns value of the pointed OidDo object
     * @param index int index of the OidDo object
     * @return TLV contained the required object
     */
    public TLV getOidDoValueTLV(int index) {
        return ((TLV)OidDo.elementAt(index)).next.child;
    }

}
