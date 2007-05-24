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
 * This class represents the PKCS15 ODF file
 */
public class ODF extends PKCS15File {

    /*
     * ODF entries tags.
     * These tags means the context-specific constructed type and tag of
     * the PKCS15Objects ::= CHOICE,
     */
    /** PrivateKeys ODF entry tag. */
    public static final int ODFTAG_PRIVATE_KEYS         = 0xa0;
    /** PublicKeys ODF entry tag. */
    public static final int ODFTAG_PUBLIC_KEYS          = 0xa1;
    /** PublicKeys ODF entry tag. */
    public static final int ODFTAG_TRUSTED_PUBLIC_KEYS  = 0xa2;
    /** SecretKeys ODF entry tag. */
    public static final int ODFTAG_SECRET_KEYS          = 0xa3;
    /** Certificates ODF entry tag. */
    public static final int ODFTAG_CERTIFICATES         = 0xa4;
    /** Certificates ODF entry tag. */
    public static final int ODFTAG_TRUSTED_CERTIFICATES = 0xa5;
    /** Certificates ODF entry tag. */
    public static final int ODFTAG_USEFUL_CERTIFICATES  = 0xa6;
    /** DataObjects ODF entry tag. */
    public static final int ODFTAG_DATA_OBJECTS         = 0xa7;
    /** AuthObjects ODF entry tag. */
    public static final int ODFTAG_AUTH_OBJECTS         = 0xa8;

    /** PublicKeys objects */
    private Vector pukdfPath = new Vector();
    /** PrivateKeys objects */
    private Vector prkdfPath = new Vector();
    /** Certificates obiects */
    private Vector cdfPath = new Vector();
    /** SecretKeys objects */
    private Vector skdfPath = new Vector();
    /** AuthObjects objects */
    private Vector aodfPath = new Vector();
    /** DataObjects objects */
    private Vector dodfPath = new Vector();

    /** This vector contains parsed objects from DF(ODF). */
    private Vector ODF;

    /**
     * Creates the ODF object
     * @param files FileSystemAbstract file system that is used
     * to reading from file
     */
    public ODF(FileSystemAbstract files) {
        super(files);
    }

    /**
     * Seeks the DODF file for the pointed tag
     * @param tag byte[] required tag
     * @return Vector the OidDo objects from the DODF file
     * @throws IOException  any IO exceptions
     * @throws TLVException  any TLV exceptions
     */
    public Vector getDOFs(byte[] tag) throws IOException, TLVException {
        Vector v = new Vector();
        for (int i = 0; i < dodfPath.size(); i++) {
            DODF d = new DODF((Location)dodfPath.elementAt(i), files);
            d.load();
            for (int j = 0; j < d.getOidDoNumber(); j++) {
                if (Utils.byteMatch(tag, d.getOid(j))) {
                    v.addElement(d.getOidDoValueTLV(j));
                }
            }
        }
        return v;
    }

    /**
     * Returns a number of the AODF files
     * @return int number of the AODF files
     */
    public int getAODFCount() {
        return aodfPath.size();
    }
    /**
     * returns location of the AODF file
     * @param index int number uf the AODF file
     * @return Location
     */
    public Location getAODFPath(int index) {
        return ((Location)aodfPath.elementAt(index));
    }
    /**
     * Reads ODF .
     * @throws IOException if I/O error occurs
     * @throws TLVException if TLV error occurs
     * It is supposed that the ODF file contains information in the following
     * format:
     *   ODFTAG_xxx {
     *     Location
     *   }
     * ...
     *   ODFTAG_xxx {
     *     Location
     *   }
     */
    public void load() throws IOException, TLVException {

        ODF = new Vector();
        resetLoader(ODF, null, null);
        parseDF(new short[] {ODFPath});

        for (int i = 0; i < ODF.size(); i++) {
            TLV t = (TLV) ODF.elementAt(i);
            Location l = files.pathToLocation(t.child.child);
            switch (t.type) {
                case ODFTAG_PRIVATE_KEYS: {
                    prkdfPath.addElement(l);
                    break;
                }
                case ODFTAG_PUBLIC_KEYS          :
                case ODFTAG_TRUSTED_PUBLIC_KEYS  : {
                    pukdfPath.addElement(l);
                    break;
                }
                case ODFTAG_SECRET_KEYS          : {
                    skdfPath.addElement(l);
                    break;
                }
                case ODFTAG_CERTIFICATES         :
                case ODFTAG_TRUSTED_CERTIFICATES :
                case ODFTAG_USEFUL_CERTIFICATES  : {
                    cdfPath.addElement(l);
                    break;
                }
                case ODFTAG_DATA_OBJECTS         : {
                    dodfPath.addElement(l);
                    break;
                }
                case ODFTAG_AUTH_OBJECTS         : {
                    aodfPath.addElement(l);
                    break;
                }

            }
        }
    }
}
