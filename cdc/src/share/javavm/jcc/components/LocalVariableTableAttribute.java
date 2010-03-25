/*
 * @(#)LocalVariableTableAttribute.java	1.10 06/10/10
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
 * A class to represent the LocalVariable table Attribute
 * of a method's code.
 */

public
class LocalVariableTableAttribute extends Attribute
{
    public LocalVariableTableEntry	data[];

    public
    LocalVariableTableAttribute( UnicodeConstant name, int l, LocalVariableTableEntry d[] ){
	super( name, l );
	this.data = d;
    }

    protected int
    writeData( DataOutput o ) throws IOException{
	int n = data.length;
	o.writeShort( n );
	for ( int i = 0; i < n; i++ ){
	    data[i].write( o );
	}
	return 2+LocalVariableTableEntry.size*n;
    }

    public void
    countConstantReferences(boolean isRelocatable) {
	super.countConstantReferences(isRelocatable);
	if (isRelocatable) {
	    for (int i = 0; i < data.length; i++) {
		data[i].name.incReference();
		data[i].sig.incReference();
	    }
	}
    }


    public static Attribute
    readAttribute(DataInput i, ConstantPool cp) throws IOException{
	UnicodeConstant name;

	name = (UnicodeConstant)cp.elementAt(i.readUnsignedShort());
	return finishReadAttribute( i, name, cp );
    }

    //
    // for those cases where we alread read the name index
    // and know that its not something requiring special handling.
    //
    public static Attribute
    finishReadAttribute(
	DataInput in,
	UnicodeConstant name,
	ConstantPool cp)
	throws IOException
    {
	int l;
	int n;
	LocalVariableTableEntry d[];

	l  = in.readInt();
	n  = in.readUnsignedShort();
	d = new LocalVariableTableEntry[ n ];
	ConstantObject[] constants = cp.getConstants();
	for ( int i = 0; i < n; i++ ){
	    d[i] = new
		LocalVariableTableEntry( in.readUnsignedShort(),
					 in.readUnsignedShort(),
					 (UnicodeConstant)
					 constants[in.readUnsignedShort()],
					 (UnicodeConstant)
					 constants[in.readUnsignedShort()],
					 in.readUnsignedShort()
					 );
	}
	return new LocalVariableTableAttribute( name, l, d );
    }
}
