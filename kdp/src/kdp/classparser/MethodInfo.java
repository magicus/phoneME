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
import kdp.Log;
import java.io.*;
import java.util.*;

/**
 * Encapsulates a Method of a Java class file.
 *
 *
 * Revision History
 *   $Log: MethodInfo.java,v $
 *   Revision 1.2  2000/08/14 19:49:11  zmm3876
 *   *** empty log message ***
 *
 *   Revision 1.1  2000/08/09 16:11:50  zmm3876
 *   *** empty log message ***
 *
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
public class MethodInfo
{
    /** access flag if this method is public */
    private static final int          ACC_PUBLIC         = 0x0001;
    /** access flag if this method is private */
    private static final int        ACC_PRIVATE            = 0x0002;
    /** access flag if this method is protected */
    private static final int        ACC_PROTECTED        = 0x0004;
    /** access flag if this method is static */
    private static final int        ACC_STATIC            = 0x0008;
    /** access flag if this method is final */
    private static final int        ACC_FINAL            = 0x0010;
    /** access flag if this method is synchronized */
    private static final int        ACC_SYNCHRONIZED    = 0x0020;
    /** access flag if this method is native */
    private static final int        ACC_NATIVE            = 0x0100;
    /** access flag if this method is abstract */
    private static final int        ACC_ABSTRACT        = 0x0400;
    /** access flag if this method is strict */
    private static final int        ACC_STRICT            = 0x0800;

    /** access flags of this method */
    private int                    accessFlags;
    /** index into constant pool containing name of method */
    private int                     nameIndex;
    /** index into constant pool table containing method signature */
    private int                     descriptorIndex;
    /** number of attributes this method has */
    private int                     attributesCount;
    /** array of attributes for this method */
    private AttributeInfo        attributes[];
    /** reference to class file's constant pool */
    private ConstantPoolInfo[]   constantPool;

   /**
    * Constructor.
    *
    * @param        iStream        input stream to read from
    * @param        constantPool   constant pool of this class
    *
    * @exception    IOException    just pass IOExceptions up
    */
   public MethodInfo (DataInputStream iStream,
                      final ConstantPoolInfo[] constantPool) throws IOException
    {
        accessFlags = iStream.readUnsignedShort ();
        nameIndex = iStream.readUnsignedShort ();
        descriptorIndex = iStream.readUnsignedShort ();
        attributesCount = iStream.readUnsignedShort ();

        //allocate space for and read in all the attributes
        attributes = new AttributeInfo[attributesCount];

        for (int lcv = 0; lcv < attributesCount; ++lcv) {
            attributes[lcv] = new AttributeInfo (iStream, constantPool);
        }

        this.constantPool = constantPool;
    }

    /**
     * Gets the source file line numbers and code array indices of all
     * lines in this method that a breakpoint can be set at.
     *
     * @return       int[][]         Array of breakable lines in source file
     *                               and corresponding code array indices.
     *                               int[source_file_line_#s][code_array_indices]
     */
    public int[][] getBreakableLineNumbers ()
    {
        //get this method's code array
        CodeAttribute                code = getCodeAttribute ();
        if (code == null) {
            // jpc
            Log.LOGN(2,  "code is null!"); 
            return null;
        }

        //get the code array's line number table
        LineNumberTableAttribute     lines = code.getLineNumberTable ();
        if (lines == null) {
            // jpc
            Log.LOGN(2,  "lines is null!"); 
            return null;
        }

        return (lines.getLineNumbersAndIndicesAsArray ());
    }

    /**
     * Retrieves the Code attribute for this method.
     *
     * @return       CodeAttribute       this methods code attribute or null
     *                                   if it does not have one
     */
    public CodeAttribute getCodeAttribute ()
    {
        int           attrIndex = 0;
        boolean       found = false;

        //loop through the attributes until the Code attribute is found or
        //there are no attributes left
        while ((!found) && (attrIndex < attributesCount)) {
            if (attributes[attrIndex].getType () == AttributeInfo.ATTR_CODE) {
                found = true;
            } else {
                ++attrIndex;
            }
        }

        if (found) {
            //found the Code attribute so return it
            AttributeInfo   attrInfo = attributes[attrIndex];
            CodeAttribute   code = (CodeAttribute) attrInfo.getInfo ();
            return code;
        } else {
            return null; //didn't find it... just return null
        }
    }
    
    /**
     * Determine if this method contains the specified source file line number.
     *
     * @param        lineNumber          source file line number
     *
     * @return       boolean             true if this method contains that
     *                                   line number, false otherwise
     */
    public boolean containsLine (int lineNumber)
    {
        //get the code attribute
        CodeAttribute      code = getCodeAttribute ();

        //look up if this line number is in the code attribute's LineNumberTable
        if (code != null) {
            return (code.containsLine (lineNumber));
        } else {
            return false;
        }
    }

    /**
     * Retrieves the starting index in the code array that equates to the
     * specified line number in the source file.
     *
     * @param        lineNumber          source file number
     *
     * @return       int                 the index in this method's code array
     *                                   that equates to the specified source
     *                                   file line number.
     */
    public int getCodeIndex (int lineNumber)
    {
        boolean       found = false;
        int           index = 0;

        //get the code attribute
        CodeAttribute      code = getCodeAttribute ();
        if (code == null) {
            return -1;
        }

        //get the LineNumberTable for this method
        LineNumberTableAttribute    lines = code.getLineNumberTable ();
        if (lines == null) {
            return -1;
        }
        
        //query the line number table for the code array index that
        //contains the specified line number
        return lines.getCodeIndexBySourceLineNumber (lineNumber);
    }

    /**
     * Obtain the access flags as an int
     *
     * @return       int        the access flags for this method
     *
     */
    public int getAccessFlags()
    {
        return accessFlags;
    }

    /**
     * Query the access permissions of this method.
     *
     * @return       String         return the access permissions as an easy
     *                              to read string
     */
    public String getAccess ()
    {
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

        if ((accessFlags & ACC_SYNCHRONIZED) > 0) {
            s = s + "synchronized ";
        }

        if ((accessFlags & ACC_NATIVE) > 0) {
            s = s + "native ";
        }

        if ((accessFlags & ACC_ABSTRACT) > 0) {
            s = s + "abstract ";
        }

        if ((accessFlags & ACC_STRICT) > 0) {
            s = s + "strict";
        }

        if (s.length () == 0) {
            s = "Not explicitly specified.";
        }

        return s;
    }

    /**
     * Retrieve the parameter list of this method.
     *
     * @return       LinkedList          the parameters of this method
     *                                   as a linked list of string or null
     *                                   if there are no parameters
     */
    public LinkedList getParameterList ()
    {
        ConstantUtf8Info   utf8Info;
        String             s = new String ("");
      
        //look up the method signature in the constant pool
        utf8Info = (ConstantUtf8Info) constantPool[descriptorIndex];

        //parse the parameter list
        LinkedList         paramList = StringParser.getParametersAsLL (utf8Info.toString ());

        return paramList;
    }

    /**
     * Query the return value of this method.
     *
     * @return       String         the return value of this method as a string
     */
    public String getReturnValue ()
    {
        ConstantUtf8Info           utf8Info;

        //look up the return value in the constant pool
        utf8Info = (ConstantUtf8Info) constantPool[descriptorIndex];

        //convert the encoded return value into it's full form
        return (StringParser.getReturnValue (utf8Info.toString ()));
    }

    /**
     * Retrieve the name of this method.
     *
     * @return       String         the name of this method
     */
    public String getName ()
    {
        ConstantUtf8Info        utf8Info;
        String                  methodName;

        //look up the name in the constant pool
        utf8Info = (ConstantUtf8Info) constantPool[nameIndex];

        methodName = utf8Info.toString ();

        return (methodName);
    }

    /**
     * Retrieve the full signature as specified in the class file 
     * (params and return value) of this method
     *
     * @return       String         this methods signature
     */
    public String getSignatureRaw ()
    {
        ConstantUtf8Info           utf8Info;

        //look up the signature in the constant pool
        utf8Info = (ConstantUtf8Info) constantPool[descriptorIndex];

        return (utf8Info.toString ());
    }

    /**
     * Retrieve the full signature (params and return value) of this method
     *
     * @return       String         this methods signature
     */
    public String getSignature ()
    {
        ConstantUtf8Info           utf8Info;

        //look up the signature in the constant pool
        utf8Info = (ConstantUtf8Info) constantPool[descriptorIndex];

        //parse the encoded form into it's full form
        return (StringParser.parseSignature (utf8Info.toString ()));
    }

    /**
     * Formats information about this method in a nice easy to read
     * string.
     *
     * @return       String              nicely formatted string containing
     *                                   method information about this method
     */
    public String toString ()
    {
        String s = new String ("");

        s = s + "Method:\n\tAccess Flags=\t";
        s = s + getAccess ();

        s = s + "\n\tName=\t\t";
        s = s + getName ();

        s = s + "\n\tSignature=\t";
        s = s + getSignature ();
        s = s + "\n\tRaw signature=\t";
        s = s + getSignatureRaw();
      
        for (int lcv = 0; lcv < attributesCount; ++lcv) {
            s = s + "\n\t" + attributes[lcv].toString ();
        }

        s = s + "\n\t BreakableLines=\t";
      
        int list[][] = getBreakableLineNumbers();

        if (list == null) {
            s = s + "No breakable lines\n";
        } else {
            for( int i = 0; i< list.length; i++) {
                s = s + "\tSourceLine: " + list[i][0] + " CodeIndex:  " +
                    list[i][1] + "\n";
            }
        }

        return s;
    }
     
    public List getLocalVariables(){
        AttributeInfo[] ai = null;
        int attribCount = 0;
        Attribute attribute = null;
            
        for(int i = 0; i < attributesCount ; i++ ){
            if( attributes[i].getType() == AttributeInfo.ATTR_CODE){
                attribute = attributes[i].getInfo();
                ai = ((CodeAttribute)attribute).getAttributes();
                attribCount = ((CodeAttribute)attribute).getAttributeCount();
                break;        
            }
    
        }
        int i = 0;
        for(i = 0; i < attribCount ; i++ ){
            if( ai[i].getType() == AttributeInfo.ATTR_LOCALVAR){     
                attribute = ai[i].getInfo();
                LocalVariableTable[] localVariableTable = 
                    ((LocalVariableTableAttribute)attribute).getLocalVariableTable();
                ConstantUtf8Info            utf8Info;
                List localVariableList = new ArrayList();
                for ( i = 0; i < localVariableTable.length; ++i) {
                    utf8Info = (ConstantUtf8Info)
                        constantPool[localVariableTable[i].nameIndex];
                    String name = utf8Info.toString ();
                    utf8Info = (ConstantUtf8Info)
                        constantPool[localVariableTable[i].descriptorIndex];
                    String type = StringParser.parseDataType (utf8Info.toString ());
                    localVariableList.add( 
                                          new LocalVariable( name,type, localVariableTable[i].startPC,
                                                             localVariableTable[i].length,localVariableTable[i].index)); 
                    
                }
                return localVariableList;
            }
        }   
        // should not reach here
        return null;
    }

    public int getArgCount() {
        //get argcount; look up the method signature in the constant pool
        ConstantUtf8Info utf8Info = (ConstantUtf8Info)constantPool[descriptorIndex];

        //parse the parameter list
        int argCount = StringParser.getParameterCount (utf8Info.toString ());
        return (argCount + (is_static() ? 0 : 1));
    }   

    public boolean is_static() {
        return ((accessFlags & ACC_STATIC) != 0);
    }
}
