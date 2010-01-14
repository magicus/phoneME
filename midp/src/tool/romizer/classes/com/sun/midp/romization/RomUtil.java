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
 *
 */


/**
 * This tool takes a binary file as an input and produces a C file
 * with the romized data.
 */

package com.sun.midp.romization;

import java.io.*;

/**
 * Base class for romizing utilities.
 */
public class RomUtil {
    /** Character output file writer */
    protected PrintWriter writer = null;

    /**
     * Generates a C code defining a byte array having the given name
     * and containing the given data.
     *
     * @param data data that will be placed into the array
     * @param arrayName name of the array in the output C file
     */
    public void writeByteArrayAsCArray(byte[] data, String arrayName) {
        pl("/** Romized " + arrayName + " */");
        pl("static const unsigned char " + arrayName + "[] = {");
        if (data != null && data.length > 0) {
            new RomizedByteArray(data).printDataArray(writer, "        ", 11);
        } else {
            pl("    0");
        }
        pl("};");
    }

    /**
     * Short-hand for printint a line into the output file
     *
     * @param s line to print
     */
    protected void pl(String s) {
        writer.println(s);
    }

    /**
     *  Writes copyright banner.
     */
    protected void writeCopyright() {
        pl("/**");
        pl(" * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.");
        pl(" * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER");
        pl(" * ");
        pl(" * This program is free software; you can redistribute it and/or");
        pl(" * modify it under the terms of the GNU General Public License version");
        pl(" * 2 only, as published by the Free Software Foundation.");
        pl(" * ");
        pl(" * This program is distributed in the hope that it will be useful, but");
        pl(" * WITHOUT ANY WARRANTY; without even the implied warranty of");
        pl(" * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU");
        pl(" * General Public License version 2 for more details (a copy is");
        pl(" * included at /legal/license.txt).");
        pl(" * ");
        pl(" * You should have received a copy of the GNU General Public License");
        pl(" * version 2 along with this work; if not, write to the Free Software");
        pl(" * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA");
        pl(" * 02110-1301 USA");
        pl(" * ");
        pl(" * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa");
        pl(" * Clara, CA 95054 or visit www.sun.com if you need additional");
        pl(" * information or have any questions.");
        pl(" * ");
        pl(" * NOTE: DO NOT EDIT. THIS FILE IS GENERATED. If you want to ");
        pl(" * edit it, you need to modify the corresponding XML files.");
        pl(" */");
    }
}
