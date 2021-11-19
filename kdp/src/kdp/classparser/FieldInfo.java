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
package kdp.classparser;

import kdp.classparser.attributes.*;
import kdp.classparser.constantpoolclasses.*;

import java.io.*;

/**
 * Encapsulates a field of a Java class file.
 *
 *
 * Revision History
 *   $Log: FieldInfo.java,v $
 *   Revision 1.1.1.1  2000/07/07 13:34:23  jrv
 *   Initial import of kdp code
 *
 *   Revision 1.1.1.1  2000/05/31 19:14:47  ritsun
 *   Initial import of kvmdt to CVS
 *
 *   Revision 1.1  2000/04/25 00:36:43  ritsun
 *   Initial revision
 *
 */
public class FieldInfo
{
    /** Access flag for a public class field */
    private static final int     ACC_PUBLIC        = 0x0001;
    /** Access flag for a private class field */
    private static final int    ACC_PRIVATE        = 0x0002;
    /** Access flag for a protected class field */
    private static final int    ACC_PROTECTED        = 0x0004;
    /** Access flag for a static class field */
    private static final int    ACC_STATIC        = 0x0008;
    /** Access flag for a final class field */
    private static final int    ACC_FINAL            = 0x0010;
    /** Access flag for a volatile class field */
    private static final int    ACC_VOLATILE        = 0x0040;
    /** Access flag for a transient class field */
    private static final int    ACC_TRANSIENT        = 0x0080;

    /** access flags for this field */
    private int                accessFlags;
    /** index into constant pool with name of this field */
    private int                nameIndex;
    /** index into constant pool with type of this field */
    private int                descriptorIndex;
    /** number of attributes this field has */
    private int                attributesCount;
    /** array holding attributes of this field */
    private AttributeInfo        attributes[];

    /** the constant pool for this class */
    private ConstantPoolInfo[] constantPool;

    /**
     * Constructor.
     *
     * @param        iStream        input stream to read from
     * @param        constantPool   this classes constant pool
     *
     * @exception    IOException    just pass IO exceptions up
     */
    public FieldInfo (DataInputStream iStream,
                      final ConstantPoolInfo[] constantPool)    throws IOException
    {
        accessFlags = iStream.readUnsignedShort ();
        nameIndex = iStream.readUnsignedShort ();
        descriptorIndex = iStream.readUnsignedShort ();
        attributesCount = iStream.readUnsignedShort ();

        //allocate space for the attributes and read them in
        attributes = new AttributeInfo[attributesCount];

        for (int lcv = 0; lcv < attributesCount; ++lcv)
            attributes[lcv] = new AttributeInfo (iStream, constantPool);

        this.constantPool = constantPool;
    }

    /**
     * Returns the access permissions of this field in a text form.
     *
     * @return       String         access permissions of this field as text
     */
    public String getAccess () {
        String        s = new String ("");

        if ((accessFlags & ACC_PUBLIC) > 0) {
            s = s + "public ";
        }

        if ((accessFlags & ACC_PRIVATE) > 0) {
            s = s + "private ";
        }

        if ((accessFlags & ACC_PROTECTED) > 0) {
            s = s + "protected ";
        }

        if ((accessFlags & ACC_STATIC) > 0) {
            s = s + "static ";
        }

        if ((accessFlags & ACC_FINAL) > 0) {
            s = s + "final ";
        }

        if ((accessFlags & ACC_VOLATILE) > 0) {
            s = s + "volatile ";
        }

        if ((accessFlags & ACC_TRANSIENT) > 0) {
            s = s + "transient";
        }

        if (s.length () == 0) {
            s = "Not explicitly specified.";
        }
        
        return s;
    }

    /**
     * Query the type of this field (int, float, etc)
     *
     * @return       String         full form of this field's type
     */
    public String getType ()
    {
        //look up the encoded type in the constant pool and parse it into
        //it's full form
        ConstantUtf8Info        utf8Info;

        utf8Info = (ConstantUtf8Info) constantPool[descriptorIndex];

        return (utf8Info.toString ());
    }

    /**
     * Query the name of this field.
     *
     * @return       String         the name of this field
     */
    public String getName ()
    {
        //look up in the constant pool and return the name of this field
        ConstantUtf8Info        utf8Info;

        utf8Info = (ConstantUtf8Info) constantPool[nameIndex];

        return utf8Info.toString ();
    }

    /**
     * Converts this field into a nice easy to read format for
     * displaying.
     *
     * @return       String         this field as a nice easy to read string
     */
    public String toString ()
    {
        String                s = new String ("");
        ConstantUtf8Info        utf8Info;

        //first determine what access flags are set for this field
        s = s + "Field:\n\tAccess Flags=\t";
        s = s + getAccess ();

        //get the name of this field
        s = s + "\n\tName=\t\t" + getName ();
        
        //get the type of this field
        s = s + "\n\tClass=\t\t" + getType ();

        //display all the attributes
        for (int lcv = 0; lcv < attributesCount; ++lcv) {
            s = s + "\n\t" + attributes[lcv].toString ();
        }

        return s;
    }
    public int getAccessFlags() {
        return accessFlags;
    }
}
