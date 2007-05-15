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
 * Represents a single entry in the ExceptionsAttribute's 
 * exception table.
 *
 *
 * Revision History
 *   $Log: ExceptionTable.java,v $
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
public class ExceptionTable
{
    /** index into the code array which denotes the
        location the exception handler becomes active */    
    private int            startPC;
    /** index into the code array which denotes the
        location the exception handler ceases to be active */
    private int            endPC;
    /** index into the code array denoting the start of
        the exception handler */
    private int            handlerPC;
    /** index into the constant pool table identifying
        the class of exceptions this exception handler
        is designed to catch */
    private int            catchType;
   
    /** 
     * Constructor.  Reads an ExceptionTable from the specified
     * data source.
     *
     * @param        iStream            source of the ExceptionTable
     *
     * @exception    IOException        pass IOExceptions up
     */
    public ExceptionTable (DataInputStream iStream) throws IOException
    {
        //read in all the fields from the DataInputStream
        startPC = iStream.readUnsignedShort ();
        endPC = iStream.readUnsignedShort ();
        handlerPC = iStream.readUnsignedShort ();
        catchType = iStream.readUnsignedShort ();
    }
   
    /**
     * Returns the contents of the ExceptionTable as a
     * String in an easy to read format.
     *
     * @return        String            the contents of the exception
     *                            table as a String
     */  
    public String toString ()
    {
        return ("Exception Table:\n\tstartPC=" + startPC +
                "\n\tendPC=" + endPC + "\n\thandlerPC=" + 
                handlerPC + "\n\tcatchType=" + catchType);
    }
}
