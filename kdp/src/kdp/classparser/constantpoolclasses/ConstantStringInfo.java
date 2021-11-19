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
 * Encapsulates a CONSTANT_String item a Java class file 
 * constant pool. 
 *
 *
 * Revision History
 *   $Log: ConstantStringInfo.java,v $
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
public class ConstantStringInfo extends ConstantPoolInfo
{
    /** index into constant pool containing a UTF8Info structure */
    private int        stringIndex;

    /**
     * Constructor.  Creates a ConstantStringInfo object.
     *
     * @param        iStream        input stream to read from
     *
     * @exception    IOException    just pass IOExceptions up
     */
    public ConstantStringInfo (DataInputStream iStream) throws IOException
    {
        tag = ConstantPoolInfo.CONSTANT_String;
        
        stringIndex = iStream.readUnsignedShort ();
    }

    /**
     * Returns this ConstantStringInfo's information as a string.
     *
     * @return       String         info as a string
     */
    public String toString ()
    {
        return ("CONSTANT_String\t" + "stringIndex=\t" + stringIndex);
    }

    /**
     * Returns the string referenced by this ConstantStringInfo object
     * from the constant pool.
     *
     * @param        constantPool   class's constant pool
     *
     * @return       String         referenced string
     */
    public String toString (final ConstantPoolInfo[] constantPool)
    {
        ConstantUtf8Info        utf8Info = (ConstantUtf8Info) constantPool[stringIndex];

        return ("CONSTANT_String=\t" + utf8Info.toString ());
    }
}
