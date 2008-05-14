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

import com.sun.satsa.util.*;
import java.util.Vector;
import java.io.IOException;
import java.util.Enumeration;
import javax.microedition.io.ConnectionNotFoundException;

/**
 * This class represents the PKCS15 DIR file abstraction.
 *<pre>
 * It is assumed that the DIR file contains information about
 * files in the following format:
 *   APP_CONSTRUCTED_1 {          [APPLICATION 1]
 *       APP_PRIMITIVE_15           Aid (Octet_string)
 *       APP_PRIMITIVE_16           label (UTF8String / opt)
 *       APP_PRIMITIVE_17           path (octet_string)
 *       APP_PRIMITIVE_19 {          DDO (ddo / opt)
 *         SEQUENCE {
 *           oid OBJECT IDENTIFIER,
 *           odfPath Path OPTIONAL,
 *           tokenInfoPath [0] Path OPTIONAL,
 *           unusedPath [1] Path OPTIONAL,
 *         }
 *       }
 *   }
 * </pre>
 */

public class DIRF extends PKCS15File {
    /** Wrong DIR file status code */
    public static final int WRONG_DIR_FILE = -1;
    /** Success status code */
    public static final int SUCCESS = 0;
    /** AID is not found status code */
    public static final int AID_NOT_FOUND = 1;
    /** Root is not selected status code */
    public static final int ROOT_NOT_SELECTED = 2;

    /** The PKCS15 application AID (according to pkcs15 spec. */
    byte[] PKCS_AID = {(byte)0xa0, 0x0, 0x0, 0x0, 0x63, 0x50,
                        0x4b, 0x43, 0x53, 0x2d, 0x31, 0x35
    };
    /** The PKCS15 DIR file name */
    short[] DIR_FILE = {0x2F00};
    /** ASN application specific flag used in types (0x40). */
    public static final int APPLICATION = 0x40;
    /** ASN application specific constructed flag used in types (0x60). */
    public static final int APP_CONSTRUCTED_0 = APPLICATION +
                                                TLV.CONSTRUCTED;
    /** ASN application specific primitive flag */
    public static final int APP_PRIMITIVE_0 = APPLICATION;
    /** ASN application specific constructed flag used in types (0x60). */
    public static final int APP_CONSTRUCTED_1 =
            APP_CONSTRUCTED_0 + 1;
    /** ASN application specific flag for [APPLICATION 15] (aid) */
    public static final int APP_PRIMITIVE_15 =
            APP_PRIMITIVE_0 + 0xf;
    /** ASN application specific flag for [APPLICATION 16] (label) */
    public static final int APP_PRIMITIVE_16 =
            APP_PRIMITIVE_0 + 0x10;
    /** ASN application specific flag for [APPLICATION 17] (path) */
    public static final int APP_PRIMITIVE_17 =
            APP_PRIMITIVE_0 + 0x11;
    /** ASN application specific flag for [APPLICATION 19] (ddo) */
    public static final int APP_CONSTRUCTED_19 =
            APP_CONSTRUCTED_0 + 0x13;

    /** This vector contains parsed objects from DIR */
    private Vector DIR;

    /**
     * Creates the DIRF object
     * @param fs FileSystemAbstract file system that is used
     * to reading from file.
     */
    public DIRF(FileSystemAbstract fs) {
        super(fs);
    }

    /**
     * Reads DIR .
     * @throws IOException if I/O error occurs
     * @throws TLVException if TLV error occurs
     */
    public void load() throws IOException, TLVException {
        DIR = new Vector();
        resetLoader(DIR, null, null);
        parseDF(DIR_FILE);
    }

    /**
     * Seeks record in the DIR file according to the pointed AID
     * @param aid byte[] required AID
     * @return TLV containing the required record
     * @throws TLVException  if TLV error occurs
     */
    private TLV getApp(byte[] aid) throws TLVException {
        for (Enumeration e = DIR.elements(); e.hasMoreElements();) {
            TLV root = (TLV) e.nextElement();
            if (root.type != APP_CONSTRUCTED_1) {
                throw new TLVException("The first tag shall be APPLICATION 1");
            }
            while (root != null) {
                TLV t = root.child;  /* APP_PRIMITIVE_15 */
                if (t.type != APP_PRIMITIVE_15) {
                    throw new TLVException(
                            "The first tag shall be APPLICATION 15 (AID)");
                }
                if (Utils.byteMatch(aid, t.getValue())) {
                    return root;
                }
                root = root.next;
            }
        }
        return null;
    }

    /**
     * Sets a root for the PKCS15 application
     * @return int Status code.
     */
    public int setPKCSRoot() {
        return setRoot(PKCS_AID);
    }

    /**
     * Sets a root for the application with the pointed AID
     * @param aid byte[] required AID
     * @return int Status code.
     */
    public int setRoot(byte[] aid) {
        TLV root;
        try {
            root = getApp(aid);
        } catch (TLVException e) {
            return WRONG_DIR_FILE;
        }
        if (root == null) {
            return AID_NOT_FOUND;
        }
        TLV t = root.child;   /* AID */
        /* Find the path item */
        while ((t != null) && (t.type != APP_PRIMITIVE_17)) {
            t = t.next;
        }
        if (t == null) {
            return WRONG_DIR_FILE;
        }
        byte[] pth = t.getValue();
        short[] path = new short[pth.length / 2];
        for (int i = 0; i < pth.length; i = i+2) {
            path[i/2] = Utils.getShort(pth, i);
        }
        try {
            files.selectRoot(path);
        }  catch (IOException ie) {
            return ROOT_NOT_SELECTED;
        }
        return SUCCESS;
    }

}
