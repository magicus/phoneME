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

import kdp.classparser.constantpoolclasses.*;
import kdp.classparser.attributes.*;
import kdp.Log;

import java.io.*;
import java.util.*;

/**
 * Encapsulation of a Java class file.  Stores all
 * the contents of a class file in memory.  
 * Has methods for reading a class file and displaying
 * the contents of a class file in an easy to read
 * manner.
 *
 * It is assumed the reader has read and understands
 * Sun's documentation of the Java class file format.
 * It is available in the Java VM Specification documentation.
 *
 *
 * Revision History
 *   $Log: ClassFile.java,v $
 *   Revision 1.2  2000/08/14 19:49:10  zmm3876
 *   *** empty log message ***
 *
 *   Revision 1.1  2000/08/09 16:11:48  zmm3876
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
public class ClassFile  {

    /** Numeric ID of java.lang.Object's super class */
    private static final int     OBJECT_SUPER_CLASS_ID      = 0;
   
    /** Tag type from VMConstants.java */
    final byte TYPE_TAG_ARRAY                               = 3;

    /** base name of class */
    private String                baseName;
    /** className of this class */
    private String                className;
    /** name of the class file this represents */
    private String                          classSignature;
    /** signature of this class name */
    private FileReference                   classFile;
    /** identifies class file format, always 0xCAFEBABE */
    private int                    magic;
    /** version number of the class file */
    private int                    majorVersion, minorVersion;
    /** number of items in the constant pool */
    private int                       constantPoolCount;
    /** array of items in the constant pool */
    private ConstantPoolInfo        constantPool[];
    /** access flags of this class */
    private AccessFlags               accessFlags;
    /** name of this class */
    private int                       thisClass;
    /** name of super class */
    private int                       superClass;
    /** number of interfaces this class implements */
    private int                       interfacesCount;
    /** array of indices into the constant pool table that
        contain the names of the interfaces this class
        implements */
    private int                       interfaces[];
    /** number of fields of this class */
    private int                       fieldsCount;
    /** array of FieldInfo objects containing information
        about all the fields of this class */
     private FieldInfo               fieldInfo[];
     /** number of methods of this class */
    private int                       methodsCount;
    /** array of MethodInfo objects containing information
        about all the methods of this class */
    private MethodInfo               methodInfo[];
    /** number of attributes of this class */
    private int                       attributesCount;
    /** attributes of this class */
    private AttributeInfo        attributes[];
    /** JDWP type type tag */
    private byte typeTag;
    /** the class ID of this class as told to us by the KVM itself */
    int classID;
    int classStatus;

    /**
     * Constructor.
     *
     * @param classFile reference to the class file this object will represent
     *
     *
     */
    public ClassFile (FileReference classFile, String className, byte typeTag)
    {
        int index;

        this.classFile = classFile;
        this.className = className;
        if ((index = className.lastIndexOf('/')) != -1)
            this.baseName = className.substring(index+1);
        else
            this.baseName = className;
        this.typeTag = typeTag;
        if (typeTag == TYPE_TAG_ARRAY) {
            /* restore the className which already includes its signature */ 
            classSignature = className;
        } else {
            /* Make up the signature for this class 
             * Add 'L' at the beginning, and the ';' at the end.
             */ 
            classSignature = "L" + className.replace('.', '/') + ";";
        }
      }

    public boolean equals(String newclassName) {
        return className.equals(newclassName);
    }

    public String getBaseName() {
        return baseName;
    }

    public String getClassName() {
        return className;
    }

    public String getClassSignature() {
        return classSignature;
    }

    /**
     * Returns the class filename
     */
    
    public String getClassFileName() {
        if (classFile == null) {
            return "NULL";
        } else {
            return (classFile.toString());
        }
    }

    public byte getJDWPTypeTag() {
        return typeTag;
    }

    public void setClassID(int classID) {
        this.classID = classID;
    }
    public int getClassID() {
        return (classID);
    }

    public void setClassStatus(int status) {
        this.classStatus = status;
    }

    public int getClassStatus() {
        return (classStatus);
    }

    /**
     * Reads a Java class file from the filename passed to the constructor.
     */
    public void readClassFile () throws IOException {
       DataInputStream        iStream;

       try {
           //create the data input stream and start reading in the
           //class file
           iStream = new DataInputStream (classFile.getInputStream());

           magic = iStream.readInt ();

           minorVersion = iStream.readUnsignedShort ();
           majorVersion = iStream.readUnsignedShort ();

           readConstantPool (iStream);

           accessFlags = new AccessFlags (iStream);

           thisClass = iStream.readUnsignedShort ();
           superClass = iStream.readUnsignedShort ();

           readInterfaces (iStream);

           readFieldInfo (iStream);

           readMethodInfo (iStream);

           readAttributes (iStream);

           iStream.close ();
       } catch (IOException e) {
           //something bad happened with the IO.
           System.out.println ("Caught IO Exception - " + e.getMessage ());
           throw(e);
       }
    }

    /**
     * Reads in the constant pool from a Java class file.
     *
     * @param    iStream        the stream on which to read data
     *
     * @return    int        the number of items in the
     *                constant pool
     *
     * @exception IOException    just pass any IOExceptions up
     */
    private int readConstantPool (DataInputStream iStream) throws IOException
    {
        //read the number of items in the constant pool
        constantPoolCount = iStream.readUnsignedShort ();

        //allocate space for the constant pool
        constantPool = new ConstantPoolInfo[constantPoolCount];

        //read in all the items of the constant pool
        for (int lcv=1; lcv < constantPoolCount; ++lcv) {
            //the tag identifies what type of constant pool item
            //this is so read it in, then branch and create the
            //appropriate object
            byte        tag = iStream.readByte ();

            switch (tag) {
            case ConstantPoolInfo.CONSTANT_Class :
                constantPool[lcv] = new ConstantClassInfo (iStream);
                break;

            case ConstantPoolInfo.CONSTANT_Fieldref :
                constantPool[lcv] = new ConstantFieldrefInfo (iStream);
                break;

            case ConstantPoolInfo.CONSTANT_Methodref :
                constantPool[lcv] = new ConstantMethodrefInfo (iStream);
                break;

            case ConstantPoolInfo.CONSTANT_InterfaceMethodref :
                constantPool[lcv] = new ConstantInterfaceMethodrefInfo (iStream);
                break;

            case ConstantPoolInfo.CONSTANT_String :
                constantPool[lcv] = new ConstantStringInfo (iStream);
                break;

            case ConstantPoolInfo.CONSTANT_Integer :
                constantPool[lcv] = new ConstantIntegerInfo (iStream);
                break;

            case ConstantPoolInfo.CONSTANT_Float :
                constantPool[lcv] = new ConstantFloatInfo (iStream);
                break;

            case ConstantPoolInfo.CONSTANT_Long :
                constantPool[lcv] = new ConstantLongInfo (iStream);
                ++lcv;     //longs "take" two constant pool entries
                break;

            case ConstantPoolInfo.CONSTANT_Double :
                constantPool[lcv] = new ConstantDoubleInfo (iStream);
                ++lcv;     //doubles "take" two constant pool entries
                break;

            case ConstantPoolInfo.CONSTANT_NameAndType :
                constantPool[lcv] = new ConstantNameAndTypeInfo (iStream);
                break;

            case ConstantPoolInfo.CONSTANT_Utf8 :
                constantPool[lcv] = new ConstantUtf8Info (iStream);
                break;

            default :
                break;
            }
        }
        return constantPoolCount;
    }

    /**
     * Read in the interfaces from the class file.
     *
     * @param        iStream                the stream on which to read data
     *
     * @return        int                    the number of interfaces
     *                                        implemented by this class
     *
     * @exception    IOException            just pass any IOExceptions up
     */
    private int readInterfaces (DataInputStream iStream) throws IOException
    {
        interfacesCount = iStream.readUnsignedShort ();

        //if there are no interfaces just return
        if (interfacesCount == 0)
            return interfacesCount;

        //allocate space for the interfaces and read them in
        interfaces = new int[interfacesCount];

        for (int lcv=0; lcv < interfacesCount; ++lcv) {
            interfaces[lcv] = iStream.readUnsignedShort ();
        }
        return interfacesCount;
    }

    /**
     * Read in the fields of this class.
     *
     * @param        iStream                the stream on which to read data
     *
     * @return        int                    the number of fields in this class
     *
     * @exception    IOException            just pass any IOExceptions up
     */
    private int readFieldInfo (DataInputStream iStream) throws IOException
    {
        //read in the number of fields
        fieldsCount = iStream.readUnsignedShort ();

        //allocate space for the fields and read them in
        fieldInfo = new FieldInfo[fieldsCount];

        for (int lcv=0; lcv < fieldsCount; ++lcv) {
            fieldInfo[lcv] = new FieldInfo (iStream, constantPool);
        }

        return fieldsCount;
    }

    /**
     * Finds the method that contains the line in the source file with the
     * specified line number.
     *
     * @param        int                      a source file line number
     *
     * @return       MethodInfo               if a method contained code at
     *                                        the specified line number then
     *                                        the method's MethodInfo is returned
     *                                        otherwise null
     */
    public MethodInfo lookUpMethodByLineNumber (int lineNumber)
    {
        boolean            found = false;
        int                index = 0;

        //loop through all the methods until we find it or run out of methods
        while ((!found) && (index < methodsCount)) {
            if (methodInfo[index].containsLine (lineNumber)) {
                found = true;
            } else {
                ++index;
            }
        }

        return (found) ? methodInfo[index] : null;
    }

    /**
     * Read in the methods of this class.
     *
     * @param        iStream                the stream on which to read data
     *
     * @return        int                    the number of methods in this class
     *
     * @exception    IOException            just pass any IOExceptions up
     */
    private int readMethodInfo (DataInputStream iStream) throws IOException
    {
        //read in the number of methods
        methodsCount = iStream.readUnsignedShort ();

        //allocate space for the methods and read them in
        methodInfo = new MethodInfo[methodsCount];

        for (int lcv=0; lcv < methodsCount; ++lcv) {
            methodInfo[lcv] = new MethodInfo (iStream, constantPool);
        }

        return methodsCount;
    }

    /**
     * Read in the attributes of this class.
     *
     * @param        iStream                the stream on which to read data
     *
     * @return        int                    the number of attributes of
     *                                        this class
     *
     * @exception    IOException            just pass any IOExceptions up
     */
    private int readAttributes (DataInputStream iStream) throws IOException
    {
        //read in the number of attributes
        attributesCount = iStream.readUnsignedShort ();

        //allocate space for the attributes and read them in
        attributes = new AttributeInfo[attributesCount];

        for (int lcv=0; lcv < attributesCount; ++lcv) {
            attributes[lcv] = new AttributeInfo (iStream, constantPool);
        }

        return attributesCount;
    }

    /**
     * Get a methodinfo given a method name 
     */

    public MethodInfo getMethodInfoByName(String methodName, String sig) {

        for (int i = 0; i < methodsCount; i++) {
            if (   methodInfo[i].getName().equals(methodName)
                   && methodInfo[i].getSignatureRaw().equals( sig ) )
                return methodInfo[i];
        }
        return null;
    }

    /**
     * Get a methodinfo given a method index 
     */

    public MethodInfo getMethodInfoByIndex(int methodIndex) {

        if (methodInfo == null || methodIndex > methodInfo.length) {
            return null;
        } else {
            return methodInfo[methodIndex];
        }
    }

     /**
      * Returns a list of MethodInfo objects
      */

     public List getAllMethodInfo() {
         Vector vector = new Vector();

         for ( int i=0; methodInfo !=null && i < methodInfo.length; i++ ) {
             vector.add( methodInfo[ i ] );
         }
         return vector;
     }

     /**
      * Returns a list of FieldInfo objects
      */

     public List getAllFieldInfo() {
         Vector vector = new Vector();

         for ( int i=0; fieldInfo != null && i < fieldInfo.length; i++ ) {
             vector.add( fieldInfo[ i ] );
         }

         return vector;
     }

     /**
      * Returns a list of class names that are interfaces implemented by
      * this class
      */

    public List getAllInterfaces() {
        Vector vector = new Vector();
        int  nameIndex;
        ConstantUtf8Info utf8Info;

        for ( int i=0; i < interfacesCount; i++ ) {
            nameIndex =
                ((ConstantClassInfo)constantPool[interfaces[ i ]]).getNameIndex();
            utf8Info = (ConstantUtf8Info)constantPool[nameIndex];
            Log.LOGN(3, "interface: " + utf8Info.toString());
            vector.add(utf8Info.toString());
        }
        return vector;
    }

    public String getSuperClass() {

        if (superClass == OBJECT_SUPER_CLASS_ID) {
            return ("");
        } else {
            ConstantClassInfo c = (ConstantClassInfo) constantPool[superClass];
            int nameIndex = c.getNameIndex ();
            ConstantUtf8Info utf8Info = (ConstantUtf8Info) constantPool[nameIndex];
            return utf8Info.toString();
        }
    }

    /**
     * Print the contents of this class file in a nice easy to
     * read manner to the specified PrintStream
     *
     * @param        out                    where to output the contents
     *                                of the class file
     */
    public void print (PrintStream out)
    {
        ConstantClassInfo            c;
        int                    nameIndex;
        ConstantUtf8Info                utf8Info;

        //output the access flags
        out.println (accessFlags);

        //get the name of this class out of the constant pool table
        //and output it
        c = (ConstantClassInfo) constantPool[thisClass];
        nameIndex = c.getNameIndex ();
        utf8Info = (ConstantUtf8Info) constantPool[nameIndex];
        out.println ("This Class:\t\t" + StringParser.parseClassName (utf8Info.toString ()));

        //get the name of the super class out of the constant pool
        //table and output it
        String sc = getSuperClass();
        out.print("Superclass: ");
        if (sc.equals("")) {
            out.println("None");
        } else {
            out.println(sc);
        }
        //if this class implements any interfaces look them up in
        //the constant pool table and output them
        if (interfacesCount > 0) {
            for (int lcv = 0; lcv < interfacesCount; ++lcv) {
                c = (ConstantClassInfo) constantPool[interfaces[lcv]];
                nameIndex = c.getNameIndex ();
                utf8Info = (ConstantUtf8Info) constantPool[nameIndex];

                out.println ("Interfaces:\t\t" + StringParser.parseClassName (utf8Info.toString ()));
            }
        }
        // output the constant pool
        out.println("Constant Pool: ");
        for (int lcv = 1; lcv < constantPool.length;) {
            out.println(constantPool[lcv].toString());
            if (constantPool[lcv] instanceof kdp.classparser.constantpoolclasses.ConstantLongInfo ||
                constantPool[lcv] instanceof kdp.classparser.constantpoolclasses.ConstantDoubleInfo) {
                lcv +=2;
            } else {
                lcv++;
            }
        }
        //output the fields of this class file
        for (int lcv = 0; lcv < fieldsCount; ++lcv) {
            out.println (fieldInfo[lcv].toString ());
        }

        //output the methods of this class file
        for (int lcv = 0; lcv < methodsCount; ++lcv) {
            out.println (methodInfo[lcv].toString ());
        }
 
        //output the attributes of this class file
        for (int lcv = 0; lcv < attributesCount; ++lcv) {
            out.println (attributes[lcv].toString ());
        }
    }
      
    public List getVariableTableForMethodIndex(int methodIndex){
        MethodInfo mi = null;
        int i = 0;
        int attributeCount = 0;
        List list = null;
        if( (mi = getMethodInfoByIndex(methodIndex)) != null ){
            list = mi.getLocalVariables();
// DEBUG
//       if (list != null) {
//              for( i = 0; i < list.size();i++)
//                 ((LocalVariable)list.get(i)).print();
//       }
// END DEBUG
        }
        return list;
    }
    
    public int getRawAccessFlags() {
        return (accessFlags.getRawAccessFlags());
    }

    /**
     * Retrieves the Sourcefile attribute for this method.
     *
     * @return       SourceFileAttribute    this methods code attribute or null
     *                                   if it does not have one
     */
    public SourceFileAttribute getSourceAttribute ()
    {
        int           attrIndex = 0;
        boolean       found = false;

        //loop through the attributes until the Source attribute is found or
        //there are no attributes left
        while ((!found) && (attrIndex < attributesCount)){
            if (attributes[attrIndex].getType () == AttributeInfo.ATTR_SOURCEFILE) {
                found = true;
            } else {
                ++attrIndex;
            }
        }

        if (found) {
            //found the SourceFile attribute so return it
            AttributeInfo   attrInfo = attributes[attrIndex];
            return (SourceFileAttribute) attrInfo.getInfo ();
        } else {
            return null; //didn't find it... just return null
        }
    }

}
