/*
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
package kdp.classparser.constantpoolclasses;

import java.io.*;

/**
 * Encapsulates a CONSTANT_Double in a Java class file
 * constant pool.
 *
 *
 * Revision History
 *   $Log: ConstantDoubleInfo.java,v $
 *   Revision 1.1.1.1  2000/07/07 13:34:24  jrv
 *   Initial import of kdp code
 *
 *   Revision 1.1.1.1  2000/05/31 19:14:48  ritsun
 *   Initial import of kvmdt to CVS
 *
 *   Revision 1.1  2000/04/25 00:34:06  ritsun
 *   Initial revision
 *
 */
public class ConstantDoubleInfo extends ConstantPoolInfo
{
    /** The high bytes of this double */
    private int        highBytes;
    /** The low bytes of this double */
    private int        lowBytes;
   
    /**
     * Constructor.  Reads in appropriate data.
     *
     * @param        iStream        input stream to read from
     *
     * @exception    IOException    just pass IOExceptions up
     */
    public ConstantDoubleInfo (DataInputStream iStream) throws IOException
    {
        tag = ConstantPoolInfo.CONSTANT_Double;

        highBytes = iStream.readInt ();
        lowBytes = iStream.readInt ();
    }

    /**
     * Returns the value of this double as a string by converting the
     * high and low bytes to a double using the method specified in the
     * JVM Specification.
     *
     * @return       String         The double as a string.
     */
    public String toString ()
    {
        long          longValue = ((long) highBytes << 32) + lowBytes;

        if (longValue == 0x7ff0000000000000L) {
            return ("CONSTANT_Double=\t" + "Positive Infinity");
        }

        if (longValue == 0xfff0000000000000L) {
            return ("CONSTANT_Double=\t" + "Negative Infinity");
        }

        if (((longValue >= 0x7ff0000000000001L) &&
             (longValue <= 0x7fffffffffffffffL)) ||
            ((longValue >= 0xfff0000000000001L) &&
             (longValue <= 0xffffffffffffffffL))) {
            return ("CONSTANT_Double=\t" + "NaN");
        }

        int s = ((longValue >> 63) == 0) ? 1 : -1;
        int e = (int)((longValue >> 52) & 0x7ffL);
        long m = (e == 0) ? (longValue & 0xfffffffffffffL) << 1 :
            (longValue & 0xfffffffffffffL) | 0x10000000000000L;

        double value = s * m * (2^(e - 1075));

        return ("CONSTANT_Double=\t" + value);
    }
}
