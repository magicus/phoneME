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
 * Encapsulates a Constant_Fieldref item of a Java class
 * file constant pool.
 *
 *
 * Revision History
 *   $Log: ConstantFieldrefInfo.java,v $
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
public class ConstantFieldrefInfo extends ConstantPoolInfo
{
    /** constant pool index containing ConstantClassInfo structure
        identifying class of this field. */
    private int        classIndex;
    /** constant pool index describing the type of this field */
    private int        nameAndTypeIndex;
   
    /**
     * Constructor.   Creates the ConstantFieldInfo object
     *
     * @param        iStream        input stream to read from
     *
     * @exception    IOException    just pass IOExceptions up
     */
    public ConstantFieldrefInfo (DataInputStream iStream) throws IOException
    {
        tag = ConstantPoolInfo.CONSTANT_Fieldref;
      
        classIndex = iStream.readUnsignedShort ();
        nameAndTypeIndex = iStream.readUnsignedShort ();
    }

    /**
     * Returns the fields of this ConstantFieldrefInfo in a string for
     * displaying.
     */
    public String toString ()
    {
        return ("CONSTANT_Fieldref" + "\t\tclassIndex=" + classIndex + 
                "\tnameAndTypeIndex=" + nameAndTypeIndex);
    }
}
