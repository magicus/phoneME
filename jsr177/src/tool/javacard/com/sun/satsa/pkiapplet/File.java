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

package com.sun.satsa.pkiapplet;

/**
 * This class represents card file.
 */
class File {

    /** File type constant - dedicated file (DF). */
    static final byte DIR        = 1;
    /**
     *  File type constant - DF where PINs reside, must be unique for
     * this implementation. */
    static final byte PIN        = 3;
    /**
     * File type constant - elementary file (EF) where private key
     * reside. */
    static final byte PrivateKeyFile        = 4;
    /** File type constant - root DF for WIM application. */
    static final byte WIM        = 9;
    /** File type constant - EF with read permission. */
    static final byte READ       = 16;
    /** File type constant - EF with read/write permission. */
    static final byte UPDATE     = 48;
    /** File attribute constant - EF is mostly empty . */
    static final byte EMPTY       = 64;

    /** Parent DF. */
    DFile parent;
    /** File identifier. */
    short id;
    /** File type. */
    short type;

    /**
     * Returns true if this file is DF.
     * @return true.
     */
    boolean isDF() {
        return true;
    }
}

/**
 * This class represents dedicated card file.
 */
class DFile extends File {

    /**
     * Files contained in this DF.
     */
    File[] files;

    /**
     * Constructs new DFile oblect.
     * @param parent parent DF
     * @param id file identifier
     * @param type file type
     */
    public DFile(DFile parent, short id, short type) {

        this.parent = parent;
        this.id = id;
        this.type = type;
    }

    /**
     * This method is used in file selection.
     * @param id file identifier
     * @return this file if it has specified identifier, one of the
     * files in this DF or null if not found
     */
    public File getFile(short id) {

        if (this.id == id) {
            return this;
        }

        for (short i = 0; i < files.length; i++) {

            if (files[i].id == id) {
                return files[i];
            }
        }
        return null;
    }
}

/**
 * This class represents elementary card file.
 */
class EFile extends File {

    /** Offset of file data in <code>Data.Files</code> array. */
    short offset;
    /** File length. */
    short length;
    /** File data. */
    byte[] data;

    /**
     * Constructs new EFile object.
     * @param parent parent DF
     * @param id file identifier
     * @param type file type
     * @param offset offset of file data in <code>data</code> array
     * @param length file length
     * @param data array where file data stored
     */
    public EFile(DFile parent, short id, short type, short offset,
                 short length, byte[] data) {

        this.parent = parent;
        this.id = id;
        this.type = type;
        this.offset = offset;
        this.length = length;
        this.data = data;
    }

    /**
     * Returns true if this file is DF.
     * @return false
     */
    boolean isDF() {
        return false;
    }
}
