/*
 * @(#)ByteToCharISO2022KR.java	1.12 06/10/10
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

public class ByteToCharISO2022KR extends ByteToCharISO2022 {
    public ByteToCharISO2022KR() {
        SODesignator = new String[1];
        SODesignator[0] = "$)C";
        SOConverter = new ByteToCharConverter[1];
        try {
            SOConverter[0] = ByteToCharConverter.getConverter("KSC5601");
        } catch (Exception e) {};
    }

    // Return the character set id
    public String getCharacterEncoding() {
        return "ISO2022KR";
    }
}
