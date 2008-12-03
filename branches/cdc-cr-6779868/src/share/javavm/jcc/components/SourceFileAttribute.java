/*
 * @(#)SourceFileAttribute.java	1.11 06/10/10
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
import java.io.DataInput;
import java.io.IOException;
import util.DataFormatException;

/*
 * A class to represent the SourceFile Attribute
 * of a class
 */

public
class SourceFileAttribute extends Attribute
{
    public UnicodeConstant	sourceName;

    public
    SourceFileAttribute( UnicodeConstant name, int l,
			 UnicodeConstant sourceName ) {
	super( name, l );
	this.sourceName = sourceName;
    }

    protected int
    writeData( DataOutput o ) throws IOException{
	o.writeShort( sourceName.index );
	return 2;
    }

    public void
    countConstantReferences( boolean isRelocatable ){
	super.countConstantReferences( isRelocatable );
	if (isRelocatable) {
	    sourceName.incReference();
        }
    }

    /*
     * This method can only be called at the very end of processing,
     * just before output writing.  We call sourceName.validate to make
     * sure that the UnicodeConstant is not in the constant pool.
     * Wc call super.validate so it can take care of the superclass fields.
     */
    public void validate(){
	super.validate();
	sourceName.validate();
    }

    public static Attribute
    readAttribute(DataInput i, ConstantObject constants[]) throws IOException{
	UnicodeConstant name;

	name = (UnicodeConstant)constants[i.readUnsignedShort()];
	return finishReadAttribute(i, name, constants);
    }

    //
    // for those cases where we already read the name index
    // and know that its not something requiring special handling.
    //
    public static Attribute
    finishReadAttribute(
	DataInput i,
	UnicodeConstant name,
	ConstantObject constants[])
	throws IOException
    {
	int l;
	int n;
	UnicodeConstant d;

	l  = i.readInt();
	n  = i.readUnsignedShort();
	d  = (UnicodeConstant)constants[n];
	return new SourceFileAttribute(name, l, d);
    }

}
