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
package kdp.classparser.attributes;

import kdp.classparser.constantpoolclasses.*;
import kdp.classparser.attributes.*;
import kdp.classparser.*;

import java.lang.*;
import java.io.*;

/**
 * Encapsulates the Exceptions attribute of a Java class file.
 *
 *
 * Revision History
 *   $Log: ExceptionsAttribute.java,v $
 *   Revision 1.1.1.1  2000/07/07 13:34:23  jrv
 *   Initial import of kdp code
 *
 *   Revision 1.1.1.1  2000/05/31 19:14:48  ritsun
 *   Initial import of kvmdt to CVS
 *
 *   Revision 1.1  2000/04/25 00:30:39  ritsun
 *   Initial revision
 *
 */
public class ExceptionsAttribute extends Attribute
{
    /** index into the constant pool table containing the name 
        of this class */
    private int                attributeNameIndex;
    /** length of this attribute in bytes */
    private int                attributeLength;
    /** the number of exceptions represented by this exceptions attribute */
    private int                numberOfExceptions;
    /** the table of exceptions represented by this exceptions attribute */
    private int                exceptionIndexTable[];

    /**
     * Constructor.  Reads the ExceptionsAttribute attribute from
     * the class file.
     *
     * @param        iStream            the input stream on which to
     *                            read the class file
     * @param        attributeNameIndex    attributeNameIndex member of
     *                            attribute structure.
     * @param        attributeLength    attributeLength member of
     *                            attribute structure.
     *
     * @exception    IOException        pass IOExceptions up
     */
    public ExceptionsAttribute (DataInputStream iStream,
                                int attributeNameIndex, int attributeLength)
        throws IOException
    {
        this.attributeNameIndex = attributeNameIndex;
        this.attributeLength = attributeLength;

        numberOfExceptions = iStream.readUnsignedShort ();

        exceptionIndexTable = new int[numberOfExceptions];
        for (int lcv = 0; lcv < numberOfExceptions; ++lcv) {
            exceptionIndexTable[lcv] = iStream.readUnsignedShort ();
        }
    }

    /**
     * Returns the ExceptionsAttribute attribute in a nice easy to
     * read format as a string.
     *
     * @param        constantPool        constant pool of the class file
     *
     * @return         String            the attribute as a nice easy to
     *                            read String
     */
    public String toString (final ConstantPoolInfo[] constantPool)
    {
        ConstantClassInfo            c;
        ConstantUtf8Info            utf8Info;
        String                        s = new String ("");

        utf8Info = (ConstantUtf8Info) constantPool[attributeNameIndex];
        s = s + utf8Info.toString ();

        for (int lcv = 0; lcv < numberOfExceptions; ++lcv) {
            int index = exceptionIndexTable[lcv];
            c = (ConstantClassInfo) constantPool[index];
            utf8Info = (ConstantUtf8Info) constantPool[c.getNameIndex ()];

            s = s + "\n\t\t\tThrows\t";
            s = s + StringParser.parseClassName (utf8Info.toString ());
        }
        return s;
    }
}
