/*
 *   
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
 */

package components;
import java.io.DataOutput;
import java.io.IOException;
import util.DataFormatException;
import jcc.Const;

/*
 * An ExceptionEntry represents a range of Java bytecode PC values,
 * a Java exception type, and an action to take should that exception be
 * thrown in that range.
 *
 * Exception entries are read by components.MethodInfo, though perhaps
 * that code should be moved here. At least we know how to write ourselves
 * out.
 */

public
class ExceptionEntry
{
    public ClassConstant catchType;

    public int startPC, endPC;
    public int handlerPC;

    public static final int size = 8; // bytes in class files

    ExceptionEntry( int s, int e, int h, ClassConstant c ){
	startPC = s;
	endPC = e;
	handlerPC= h;
	catchType = c;
    }

    public void write( DataOutput o ) throws IOException {
	o.writeShort( startPC );
	o.writeShort( endPC );
	o.writeShort( handlerPC );
	o.writeShort( (catchType==null) ? 0 : catchType.index );
    }

    /*
     * A class referenced from an ExceptionEntry
     * is in the local constant pool, not the shared one.
     *
     * Thus it must not be externalized, but must be counted.
     * These decisions could be exposed at a higher level, for some
     * savings in performance, and should be when I have the
     * courage of my convictions.
     */
    public void externalize( ConstantPool p ){
	// do nothing.
    }

    public void countConstantReferences( ){
	if ( catchType != null )
	    catchType.incReference();
    }
}
