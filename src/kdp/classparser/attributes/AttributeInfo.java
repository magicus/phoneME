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
import kdp.Log;
import java.io.*;

/**
 * Encapsulates an attribute of a class, field, or method of a
 * Java class file.
 *
 *
 * Revision History
 *   $Log: AttributeInfo.java,v $
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
public class AttributeInfo
{
    /** The possible types of attributes this AttributeInfo can represent */
    public static final int    ATTR_CODE           = 0;
    public static final int    ATTR_CONST          = 1;
    public static final int    ATTR_DEPRECATED     = 2;
    public static final int    ATTR_EXCEPTIONS     = 3;
    public static final int    ATTR_INNERCLASS     = 4;
    public static final int    ATTR_LINENUMBER     = 5;
    public static final int    ATTR_LOCALVAR       = 6;
    public static final int    ATTR_SOURCEFILE     = 7;
    public static final int    ATTR_SYNTHETIC      = 8;

    /** index into the constant pool table containing the name
        of this class */
    private int                attributeNameIndex;
    /** length of this attribute in bytes */
    private int                attributeLength;
    /** the attribute itself */
    private Attribute        info;

    /** the type of attribute this is */
    private int                attrType;
    /** reference to the class's constant pool */
    private ConstantPoolInfo[] constantPool;

    /**
     * Constructor.  Creates an AttributeInfo object by reading
     * in the information about the attribute and the attribute.
     *
     * @param        iStream        input on which to read the data
     * @param        constantPool    the constant pool of the class file
     *
     * @exception    IOException    just pass IOExceptions up
     */
    public AttributeInfo (DataInputStream iStream,
                          final ConstantPoolInfo[] constantPool)
        throws IOException
    {
        //read the name index and the attribute length
        attributeNameIndex = iStream.readUnsignedShort ();
        attributeLength = iStream.readInt ();

        //get the name of the attribute out of the constant pool
        ConstantUtf8Info    utf8Info = (ConstantUtf8Info)
            constantPool[attributeNameIndex];
        String            s = utf8Info.toString ();

        //branch based on the type of attribute
        //and create the appropriate attribute object
        //and store the attribute type
        if (s.equals ("ConstantValue")) {
            info = new ConstantValueAttribute (iStream, attributeNameIndex,
                                               attributeLength);
            attrType = ATTR_CONST;
        } else if (s.equals ("Synthetic")) {
            info = new SyntheticAttribute (iStream, attributeNameIndex,
                                           attributeLength);
            attrType = ATTR_SYNTHETIC;
        } else if (s.equals ("Deprecated")) {
            info = new DeprecatedAttribute (iStream, attributeNameIndex,
                                            attributeLength);
            attrType = ATTR_DEPRECATED;
        } else if (s.equals ("Code")) {
            info = new CodeAttribute (iStream, constantPool,
                                      attributeNameIndex, attributeLength);
            attrType = ATTR_CODE;
        } else if (s.equals ("Exceptions")) {
            info = new ExceptionsAttribute (iStream, attributeNameIndex,
                                            attributeLength);
            attrType = ATTR_EXCEPTIONS;
        } else if (s.equals ("InnerClasses")) {
            info = new InnerClassesAttribute (iStream, attributeNameIndex,
                                              attributeLength);
            attrType = ATTR_INNERCLASS;
        } else if (s.equals ("LineNumberTable")) {
            info = new LineNumberTableAttribute (iStream,
                                                 attributeNameIndex,
                                                 attributeLength);
            attrType = ATTR_LINENUMBER;
        } else if (s.equals ("SourceFile")) {
            info = new SourceFileAttribute (iStream, constantPool,
                                            attributeNameIndex,
                                            attributeLength);
            attrType = ATTR_SOURCEFILE;
        } else if (s.equals ("LocalVariableTable")) {
            info = new LocalVariableTableAttribute (iStream,
                                                    attributeNameIndex, attributeLength);
            attrType = ATTR_LOCALVAR;
        } else {
            Log.LOGN(8, "Unknown attribute: " + s);
            for (int lcv = 0; lcv < attributeLength; ++lcv) {
                iStream.readByte ();
            }
        }
                          
        this.constantPool = constantPool;
    }

    /**
     * Retrieves the info that this AttributeInfo represents.
     *
     * @return       Attribute      the info represented by this AttributeInfo
     */
    public Attribute getInfo ()
    {
        return info;
    }
     
    /**
     * Retrieves the type of this AttributeInfo.
     *
     * @return       int            the type of this attribute from the list
     *                              specified above
     */
    public int getType ()
    {
        return attrType;
    }

    /**
     * Returns the attribute in an easy to read format as a String.
     *
     * @return        String            the attribute in a nice string format
     */
    public String toString ()
    {
        if (info == null) {
            return ("AttributeInfo:\n\t\t" + "Unrecognized attribute");
        } else {
            return ("AttributeInfo:\n\t\t" + info.toString (constantPool));
        }
    }
}
