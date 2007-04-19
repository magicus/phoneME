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

import java.io.*;

/**
 * Represents an individual entry in the InnerClasses table of
 * a Java class file.
 *
 * This class has never been thoroughly tested.
 *
 *
 * Revision History
 *   $Log: InnerClassesTable.java,v $
 *   Revision 1.1.1.1  2000/07/07 13:34:24  jrv
 *   Initial import of kdp code
 *
 *   Revision 1.1.1.1  2000/05/31 19:14:47  ritsun
 *   Initial import of kvmdt to CVS
 *
 *   Revision 1.1  2000/04/25 00:30:39  ritsun
 *   Initial revision
 *
 */
public class InnerClassesTable
{
   /** Access flags for inner classes */
   private static final int        ACC_PUBLIC        = 0x0001;
   private static final int        ACC_PRIVATE        = 0x0002;
   private static final int        ACC_PROTECTED        = 0x0004;
   private static final int        ACC_STATIC        = 0x0008;
   private static final int        ACC_FINAL            = 0x0010;
   private static final int        ACC_INTERFACE        = 0x0200;
   private static final int        ACC_ABSTRACT        = 0x0400;

   /** constant pool index identifying the inner class */
   private int                       innerClassInfoIndex;
   /** constant pool index identifying the outer class */
   private int                       outerClassInfoIndex;
   /** the index of the name of the inner class */
   private int                       innerNameIndex;
   /** access flags of the inner class */
   private int                       innerClassAccessFlags;

   /**
    * Constructor.  Just read in all the fields.
    *
    * @param        iStream             input stream to read from
    *
    * @exception    IOException         just pass io exceptions up
    */
   public InnerClassesTable (DataInputStream iStream) throws IOException
     {
      innerClassInfoIndex = iStream.readUnsignedShort ();
      outerClassInfoIndex = iStream.readUnsignedShort ();
      innerNameIndex = iStream.readUnsignedShort ();
      innerClassAccessFlags = iStream.readUnsignedShort ();
     }

    /**
     * Convert the information we have into an easy to read format and return
     * as a string.
     *
     * @return       String              the info we have as a string
     */
    public String toString ()
    {
        String                s = new String ("");

        s = s + "\n\tinnerClassInfoIndex=" + innerClassInfoIndex;
        s = s + "\n\touterClassInfoIndex=" + outerClassInfoIndex;
        s = s + "\n\tinnerNameIndex=" + innerNameIndex;
        s = s + "\n\tinnerClassAccessFlags=";

        if ((innerClassAccessFlags & ACC_PUBLIC) > 0) {
            s = s + "public ";
        }

        if ((innerClassAccessFlags & ACC_PRIVATE) > 0) {
            s = s + "private ";
        }

        if ((innerClassAccessFlags & ACC_PROTECTED) > 0) {
            s = s + "protected ";
        }

        if ((innerClassAccessFlags & ACC_STATIC) > 0) {
            s = s + "static ";
        }

        if ((innerClassAccessFlags & ACC_FINAL) > 0) {
            s = s + "final ";
        }

        if ((innerClassAccessFlags & ACC_INTERFACE) > 0) {
            s = s + "interface ";
        }

        if ((innerClassAccessFlags & ACC_ABSTRACT) > 0) {
            s = s + "abstract";
        }

        return s;
    }
}
