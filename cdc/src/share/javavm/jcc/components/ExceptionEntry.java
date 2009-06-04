/*
 * @(#)ExceptionEntry.java	1.12 06/10/10
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

package components;
import java.io.DataOutput;
import java.io.IOException;
import consts.Const;
import util.DataFormatException;

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

    ExceptionEntry(int startPC, int endPC, int handlerPC,
                   ClassConstant catchType) {
	this.startPC = startPC;
	this.endPC = endPC;
	this.handlerPC = handlerPC;
	this.catchType = catchType;
    }

    public void write( DataOutput o ) throws IOException {
	o.writeShort( startPC );
	o.writeShort( endPC );
	o.writeShort( handlerPC );
	o.writeShort( (catchType==null) ? 0 : catchType.index );
    }

    public void countConstantReferences( ){
	if ( catchType != null )
	    catchType.incReference();
    }
}
