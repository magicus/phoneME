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
 * This class represents the PKCS15 AODF file abstraction
 */
public class AODF extends PKCS15File {
    /** This vector contains parsed objects from DF(AODF). */
    private Vector AODF;

    /** Number of thr PIN entries */
    private int size = 0;
    /** Array for the PIN entries */
    private TLV[] Entries;

    /**
     * Creates AODF object for the pointed location and file system
     * @param location Location required location
     * @param files FileSystemAbstract requred file system
     */
    public AODF(Location location, FileSystemAbstract files) {
        super(location, files);
    }

    /**
     * Loads DODF object from the file system
     * @throws IOException if I/O error occurs
     * @throws TLVException if TLV error occurs
     */
    public void load() throws IOException, TLVException {
        AODF = new Vector();
        resetLoader(AODF, null, null);
        parseDF(location.path);
        readAODF();
    }

    /**
     * Reads AODF data from the AODF vector.
     * @throws TLVException if TLV error occurs
     */
    /*
     * It is assumed that the AODF file contains information
     * about AUF files in the following format:
     *     AuthenticationType,
     *     ...
     *     AuthenticationType,
     */
    private void readAODF() throws TLVException {
        TLV root = (TLV)AODF.firstElement();    /* SEQUENCE OF */
        size = AODF.size();
        Entries = new TLV[size];
        for (int i = 0; i < size; i++) {
            Entries[i] = (TLV)AODF.elementAt(i);
        }
    }
    /**
     * Returns number of the Authentication objects
     * @return int
     */
    public int getEntryCount() {
        return size;
    }
    /**
     * Returns pointed authentication object
     * @param index int index of the authentication object
     * @return TLV required authentication object
     */
    public TLV getEntry(int index) {
        return Entries[index];
    }

}
