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

import java.util.*;
import java.io.*;
import kdp.Log;

/**
 * Main program to run my Java class file parser. 
 *
 *
 * Revision History
 *   $Log: ClassParser.java,v $
 *   Revision 1.2  2000/08/14 19:49:10  zmm3876
 *   *** empty log message ***
 *
 *   Revision 1.1  2000/08/09 16:11:49  zmm3876
 *   *** empty log message ***
 *
 *   Revision 1.1.1.1  2000/07/07 13:34:23  jrv
 *   Initial import of kdp code
 *
 *   Revision 1.1.1.1  2000/05/31 19:14:47  ritsun
 *   Initial import of kvmdt to CVS
 *
 *   Revision 1.2  2000/05/15 16:52:51  jpc1870
 *   added locationsOfLine() to refType()
 *
 *   Revision 1.1  2000/04/25 00:36:43  ritsun
 *   Initial revision
 *
 */
public class ClassParser {
    /** Name of the Java class file to read */
    private String        className;
    private String        baseName;
    /** line number to look up in the class file, -1 if we don't want to
    look up a specific line just print all available class information */
    private int                lineNumber = -1;
    /** stores the class file contents */
    public ClassFile          classFile; // changed to public -- jpc
    /* the class ID of this class as told to us by the KVM itself */
    private int classID;
    /* the type of class,  (class, interface, array) */
    private byte typeTag;
    
    
    /**
     * Constructor.
     *
     * @param        className      classname
     */
    public ClassParser (String className, byte typeTag) {
        //make sure they passed at least 1 arg
        int index;

    
        //store the class file name
        this.className = className;
        if ((index = className.lastIndexOf('/')) != -1) {
            this.baseName = className.substring(index+1);
        } else {
            this.baseName = className;
        }
        this.typeTag = typeTag;
    }

    public boolean equals( String newclassName ) {
        return className.equals( newclassName );
    }

    public String getBaseName() {
        return baseName;
    }

    public String getClassName() {
        return className;
    }

    /**
     * Returns the class filename
     */
    
    public String getClassFileName() {
        return (className + ".class");
    }

    public String getFullClassFileName() {
        return (className + ".class");
    }
    
    /**
     * Returns the classFile object 
     */
    
    public ClassFile getClassFile() {
        return classFile;
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

    /**
     * Main method.  Where it all begins... and ends too.
     *
     * @param        args[]            array of command line arguments
     */

    
    public static void main (String args[]) {
        int lineNumber = 0;
        ClassFile classfile;
        FileReference file = FileReference.create(args[0], args[1]);

        //declare a ClassParser object
      
        //create a new ClassFile object passing it the name of the
        //class file
        classfile = new ClassFile (file, args[1], (byte)'L');
        //read the class file
        try {
            classfile.readClassFile ();
        } catch (IOException e) {
            Log.LOGN(3, "Main: caught IOException " + e);
        }

        Log.LOGN(3, "\n");

        //if a line number was not specified then just output everything
        if (args.length < 3)
            classfile.print (System.out);
        else {
            //just output the information for the method the specified by line number
            lineNumber = Integer.parseInt (args[2]);
            MethodInfo    method = classfile.lookUpMethodByLineNumber (lineNumber);
            if (method != null) {
                Log.LOGN(3, "Associated the method \"" + method.getName () +
                         "\" with line number " + lineNumber + ".");
                Log.LOGN(3, "The following information if available for " +
                         method.getName () + ".");
                Log.LOGN(3, method.toString());

                //output the code array index for this method at which a breakpoint
                //should be placed in order to stop at the specified line in the
                //source file.
                Log.LOGN(3, "");
                Log.LOGN(3, "To put a breakpoint at line " + lineNumber +
                         " break at code index " +
                         method.getCodeIndex (lineNumber) + ".");
            
                Log.LOGN(3, "");
                Log.LOGN(3, "Breakpoints can be set at the following lines in " +
                         method.getName () + ":");
                Log.LOGN(3, "tSource Line Number\tBytecode Offset");
                Log.LOGN(3, "t------------------\t---------------");
            
                int     lineNumbers[][] = method.getBreakableLineNumbers ();
                if (lineNumbers == null) {
                    Log.LOGN(3, "Method contains no breakable lines.");
                } else {
                    for (int lcv = 0; lcv < lineNumbers.length; ++lcv) {
                        Log.LOGN(3, "t\t" + lineNumbers[lcv][0] + "\t\t\t" + lineNumbers[lcv][1]);
                    }
                }
            } else {
                //the specified line was not an executable line of code within a method
                Log.LOGN(3, "Line number " + lineNumber + " does not contain" +
                         " code within a method.");
            }
        }
        
        // classfile.getVariableTableForMethodName("main");
    }
} // ClassParser
