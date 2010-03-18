/*
 * @(#)CoreImageWriter.java	1.22 06/10/22
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
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
 *
 */

package runtime;
import components.ConstantPool;
import util.ClassnameFilterList;
import java.util.Vector;

/*
 * This is the interface that a machine-independent
 * (ROM) image writer must implement in order to be called
 * by the JavaCodeCompact driver.
 * Additionally, the image writer must have a public constructor
 * of no parameters.
 */
public
interface CoreImageWriter {

    void    init( boolean uselinenumbers, boolean lossless, ClassnameFilterList nativeTypes, boolean verbose, int maxSegmentSize, boolean jitOn);
	/*
	 * Finish initialization. Parameters:
	 *	uselinenumbers: produce data structures giving correspondence
	 *		between bytecode PC and source line numbers
	 *	nativeTypes: classifies native method interface for a class
	 *	verbose: print informative messages on System.out.
	 */

    boolean setAttribute( String attributeValue );
	/*
	 * Process target-specific flags from the command line.
	 * Return value: false: flag unrecognized or malformed
	 *		 true:  flag recognized and well-formed.
	 */

    boolean open( String filename );

	/*
	 * Write class and other data structures to output file.
         * Parameter:
         *      doWrite: actually write the output file
	 * Return value: false: any error was encountered. See printError.
	 *		 true: otherwise.
	 */

    boolean writeClasses( Vector sharedconsts, boolean doWrite );
        /*
         * Write class and other data structures to output file.
         * Parameter:
         *      sharedconsts: shared constant pool for all classes
         * Parameter:
         *      doWrite: actually write the output file
         * Return value: false: any error was encountered. See printError.
         *               true: otherwise.   
         */

    void    printSpaceStats( java.io.PrintStream log );
	/*
	 * Print informative message upon normal completion.
	 * This is usually a set of numbers, such as the amout
	 * or read-only and read-write memory that will be consumed by
	 * this image.
	 * Parameter:
	 *	log: PrintStream to use for output
	 */

    void    close();
	/*
	 * Close output file.
	 */

    void    printError( java.io.PrintStream o );
	/*
	 * Print information about any error encountered during processing.
	 * To be called if either open or writeClasses returns false.
	 * Often, this is the contents of an exception thrown and caught
	 * internally.
	 * Parameter:
	 *	o: PrintStream to use for output
	 */

}
