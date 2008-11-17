/*
 * @(#)ByteToCharISO2022CN.java	1.13 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

package sun.io;

/**
 * @author Tom Zhou
 */
public class ByteToCharISO2022CN extends ByteToCharISO2022 {
    public ByteToCharISO2022CN() {
        SODesignator = new String[3];
        SODesignator[0] = "$A";
        SODesignator[1] = "$)A";
        SODesignator[2] = "$)G";
        SS2Designator = new String[1];
        SS2Designator[0] = "$*H";
        SS3Designator = new String[1];
        SS3Designator[0] = "$+I";
        SOConverter = new ByteToCharConverter[3];
        SS2Converter = new ByteToCharConverter[1];
        SS3Converter = new ByteToCharConverter[1];
        try {
            SOConverter[0] = SOConverter[1] 
                            = ByteToCharConverter.getConverter("GB2312");
            SOConverter[2] = SS2Converter[0] = SS3Converter[0] 
                                    = ByteToCharConverter.getConverter("CNS11643");
        } catch (Exception e) {};
    }

    // Return the character set id
    public String getCharacterEncoding() {
        return "ISO2022CN";
    }
}
