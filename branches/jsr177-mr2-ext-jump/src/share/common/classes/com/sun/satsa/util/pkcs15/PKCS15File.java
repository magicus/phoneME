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

/**
 * This class represents PKCS15 file abstraction
 */
public class PKCS15File {
    /** ODF path. */
    protected static final short ODFPath        = 0x5031;
    /** EF(TokenInfo) path. */
    protected static final short TokenInfoPath  = 0x5032;
    /** EF(UnusedSpace) path. */
    protected static final short UnusedSpace    = 0x5033;

    /** This vector contains objects loaded from directory file. */
    protected Vector loaderObjects;
    /** This vector contains locations of loaded objects. */
    protected Vector loaderLocations;
    /**
     * This vector contains locations of free space in directory
     * files. */
    protected Vector loaderFreeBlocks;
    /** Location */
    protected Location location;
    /** File system object. */
    protected FileSystemAbstract files;

    /**
     * Create the object for the pointed file system and location
     * @param location Location required location
     * @param files FileSystemAbstract required file system
     */
    protected PKCS15File(Location location, FileSystemAbstract files) {
        this.location = location;
        this.files = files;
    }

    /**
     * Create the object for the pointed file system
     * @param files FileSystemAbstract required file system
     */
    protected PKCS15File(FileSystemAbstract files) {
        this.files = files;
    }

    /**
     * Initialises object loader.
     * @param objects vector for loaded objects or null
     * @param locations vector for object locations or null
     * @param freeBlocks vector for unused block locations or null
     */
    protected void resetLoader(Vector objects,
                             Vector locations,
                             Vector freeBlocks) {
        loaderObjects = objects;
        loaderLocations = locations;
        loaderFreeBlocks = freeBlocks;
    }

    /**
     * Parses directory file. Places results into vectors specified
     * <code>resetLoader</code> method.
     * @param path path to directory file
     * @throws TLVException if parsing error occurs
     * @throws IOException if I/O error occurs
     */
    protected void parseDF(short[] path) throws TLVException, IOException {
        if (path.length > 1) {
            files.select(path);
        } else {
            files.select(path[0]);
        }
        doParseDF(files.readFile(), path,
                      loaderObjects, loaderLocations, loaderFreeBlocks);
    }

    /**
     * Parses EF(DF).
     * @param data data to be parsed
     * @param path file path
     * @param objects method places objects from file into this vector.
     *        Can be null. Contains values of TLV type
     * @param locations method places location of objects into this
     *        vector. Can be null. Contains values of type Location.
     * @param freeBlocks method places locations of free memory in
     *        EF(DF) into this vector. Can be null. Contains values of
     *        type Location.
     * @throws TLVException if parsing error occurs
     */
    protected static void doParseDF(byte[] data, short[] path,
                           Vector objects, Vector locations, Vector freeBlocks)
                throws TLVException
    {
        int start = 0;
        int current = 0;
        while (current < data.length) {
                // free space - skip
            if (data[current] == (byte) 0xff) {
                current++;
                continue;
            }
            // TLV object
            TLV t = new TLV(data, current);
            // empty one - skip
            if (t.type == 0) {
                current = t.valueOffset + t.length;
                continue;
            }
            // real object
            if (objects != null) {
                objects.addElement(t);
            }
            if (locations != null) {
                locations.addElement(new Location(path, current,
                               t.valueOffset + t.length - current));
            }
            if (freeBlocks != null && start < current) {
                freeBlocks.addElement(
                    new Location(path, start, current - start));
            }
            current = t.valueOffset + t.length;
            start = current;
        }
        if (start < current && freeBlocks != null) {
            freeBlocks.addElement(
                        new Location(path, start, current - start));
        }
    }

}
