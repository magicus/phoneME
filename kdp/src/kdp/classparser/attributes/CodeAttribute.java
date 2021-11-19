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

import kdp.classparser.attributes.*;
import kdp.classparser.constantpoolclasses.*;

import java.io.*;

/**
 * Encapsulates the Code attribute of a Java class file.
 *
 *
 * Revision History
 *   $Log: CodeAttribute.java,v $
 *   Revision 1.2  2000/08/14 19:49:11  zmm3876
 *   *** empty log message ***
 *
 *   Revision 1.1  2000/08/09 15:51:44  zmm3876
 *   *** empty log message ***
 *
 *   Revision 1.1.1.1  2000/07/07 13:34:23  jrv
 *   Initial import of kdp code
 *
 *   Revision 1.1.1.1  2000/05/31 19:14:47  ritsun
 *   Initial import of kvmdt to CVS
 *
 *   Revision 1.1  2000/04/25 00:30:39  ritsun
 *   Initial revision
 *
 */
public class CodeAttribute extends Attribute
{
    /** index into constant pool table containing the name 
        of this class */
    private int                    attributeNameIndex;
    /** length of this attribute in bytes */
    private int                    attributeLength;
    /** the maximum depth of the operand stack of this method */
    private int                    maxStack;
    /** the number of local variables in the local variable 
        array allocated upon invocation of this method */
    private int                    maxLocals;
    /** the number of bytes in the code array for this method */
    private int                    codeLength;
    /** the bytes of Java virtual machine code that implement a method */
    private byte                code[];
    /** the number of entries in the exceptionTable table */
    private int                    exceptionTableLength;
    /** each entry describes one exception handler in the code array */
    private ExceptionTable         exceptionTable[];
    /** the number of attributes of the Code attribute */
    private int                    attributesCount;
    /** array of attributes for this CodeAttribute */
    private AttributeInfo              attributes[];

    /**
     * Constructor.  Reads the CodeAttribute attribute from
     * the class file.
     *
     * @param        iStream            the input stream on which to read
     *                                   the class file
     * @param        constantPool        constant pool of the class file
     * @param        attributeNameIndex    attributeNameIndex member of
     *                            attribute structure.
     * @param        attributeLength    attributeLength member of
     *                            attribute structure.
     *
     * @exception    IOException        pass IOExceptions up
     */
    public CodeAttribute (DataInputStream iStream,
                          final ConstantPoolInfo[] constantPool,
                          int attributeNameIndex, int attributeLength)
        throws IOException
    {
        this.attributeNameIndex = attributeNameIndex;
        this.attributeLength = attributeLength;

        //read in the maxStack and maxLocals items
        maxStack = iStream.readUnsignedShort ();
        maxLocals = iStream.readUnsignedShort ();

        //read in the length of the code array
        codeLength = iStream.readInt ();
      
        //allocate space for and read in the code array
        code = new byte[codeLength];
        for (int lcv=0; lcv < codeLength; ++lcv)
            code[lcv] = iStream.readByte ();
      
        //read in the number of items in the exception table
        exceptionTableLength = iStream.readUnsignedShort ();

        //allocate space for and read in the exception table      
        exceptionTable = new ExceptionTable[exceptionTableLength];
        for (int lcv=0; lcv < exceptionTableLength; ++lcv) {
            exceptionTable[lcv] = new ExceptionTable (iStream);
        }
      
        //read in the number of attributes
        attributesCount = iStream.readUnsignedShort ();

        //allocate space for and read in the attributes
        attributes = new AttributeInfo[attributesCount];
        for (int lcv=0; lcv < attributesCount; ++lcv) {
            attributes[lcv] = new AttributeInfo (iStream, constantPool);
        }
    }

    /**
     * Retrieves the line number table of this code attribute
     *
     * @return       LineNumberTableAttribute      this code attribute's
     *                                             line number table.
     */
    public LineNumberTableAttribute getLineNumberTable ()
    {
        int           attrIndex = 0;
        boolean       found = false;

        //loop through this code attribute's attributes until the line number
        //table is found or there are no more to look through
        while ((!found) && (attrIndex < attributesCount)) {
            if (attributes[attrIndex].getType () == AttributeInfo.ATTR_LINENUMBER) {
                found = true;
            } else {
                ++attrIndex;
            }
        }

        if (found) {
            //if we found the line number table return it
            AttributeInfo              attrInfo = attributes[attrIndex];
            LineNumberTableAttribute   lineNumberTable =
                (LineNumberTableAttribute) attrInfo.getInfo ();
            return lineNumberTable;
        } else {
            return null; //did not find the line number table so return null
        }
    }

    /**
     * Query this code attribute to see if it contains the source file
     * line number specified by lineNumber.
     *
     * @return       boolean        true if the line number is in this code
     *                              attribute, false otherwise
     */
    public boolean containsLine (int lineNumber)
    {
        //get the line number table
        LineNumberTableAttribute     lines = getLineNumberTable ();

        //if the line number table was found tell it to look up the line number
        if (lines != null) {
            return (lines.containsLine (lineNumber));
        } else {
            return false; //could not find the line number table so return false
        }
    }
     
    /**
     * Returns the byte code for the method 
     *
     * @return           byte[]       byte code
     */

    public byte[] getByteCodes() {
        byte[] ret = new byte[ code.length ];
        System.arraycopy( code, 0, ret, 0, code.length );
        return ret;
    }

    /**
     * Returns the CodeAttribute attribute in a nice easy to
     * read format as a string.
     *
     * @param        constantPool        constant pool of the class file
     *
     * @return         String            the attribute as a nice easy to
     *                            read String
     */
    public String toString (final ConstantPoolInfo[] constantPool)
    {
        ConstantUtf8Info            utf8Info;
        String                    s = new String ("");

        utf8Info = (ConstantUtf8Info) constantPool[attributeNameIndex];
        s = s + utf8Info.toString ();

        for (int lcv = 0; lcv < attributesCount; ++lcv) {
            s = s + "\n\t\t" + attributes[lcv].toString ();
        }
        return s;
    }
     
    public AttributeInfo[] getAttributes(){
        return attributes;
    }
     
    public int getCodeLength() {
        return codeLength;
    }   

    public int getAttributeCount(){
        return attributesCount;
    }
}
