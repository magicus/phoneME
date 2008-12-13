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
import java.io.DataInput;
import java.io.IOException;
import util.DataFormatException;
import jcc.Const;

/*
 * A class to represent the StackMap attribute of a method's code.
 */

public
class StackMapAttribute extends Attribute
{
    public StackMapFrame	data[];

    public
    StackMapAttribute( UnicodeConstant name, int l, StackMapFrame d[] ){
	super( name, l );
	this.data = d;
    }

    public void
    externalize( ConstantPool p ){
	super.externalize( p );
 	for (int i = 0; i < data.length; i++) {
	    data[i].externalize(p);
	}
    }

    protected int
    writeData( DataOutput o ) throws IOException{
	int length = 2;
	o.writeShort(data.length);
	for ( int i = 0; i < data.length; i++ ){
	    length += data[i].writeData(o);
	}
	return length;
    }

    public void
    countConstantReferences( boolean isRelocatable ){
	super.countConstantReferences( isRelocatable );
	for (int i = 0; i < data.length; i++) {
	    data[i].countConstantReferences(isRelocatable);
	}
    }

    public static Attribute
    readAttribute( DataInput i, ConstantObject globals[] ) throws IOException{
	UnicodeConstant name;
	name = (UnicodeConstant)globals[i.readUnsignedShort()];
	return finishReadAttribute(i, name, globals );
    }

    //
    // for those cases where we already read the name index
    // and know that its not something requiring special handling.
    //
    public static Attribute
    finishReadAttribute(DataInput in, 
			UnicodeConstant name, 
			ConstantObject globals[] ) throws IOException {
	int length = in.readInt();
	// Read the number of frames
	int n = in.readUnsignedShort();
	StackMapFrame d[] = new StackMapFrame[n];
	// Read each frame
	for (int i = 0; i < n; i++) { 
	    d[i] = new StackMapFrame(in, globals);
	}
	return new StackMapAttribute(name, length, d);
    }

}
