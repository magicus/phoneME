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
import util.*;

/*
 * A class to represent the Exceptions table Attribute
 * of a method
 */

public
class ExceptionsAttribute extends Attribute
{
    public ClassConstant	data[];

    public
    ExceptionsAttribute( UnicodeConstant name, int l, ClassConstant d[] ){
	super( name, l );
	data = d;
    }

    public void 
    countConstantReferences( boolean isRelocatable ){
	super.countConstantReferences( isRelocatable );
	int n = data.length;
	for ( int i = 0; i < n; i++ ){
	    data[i].incReference();
	}
    }

    protected int
    writeData( DataOutput o ) throws IOException{
	int n = data.length;
	o.writeShort( n );
	for ( int i = 0; i < n; i++ ){
	    o.writeShort( data[i].index );
	    // debug
	    if ( data[i].index <= 0 ){
		System.err.println(Localizer.getString("exceptionsattribute.exceptions_table_references_negative_subscript", data[i]));
	    }
	    // end debug
	}
	return 2 + 2*n;
    }

    public static Attribute
    readAttribute( DataInput i, ConstantObject locals[], ConstantObject globals[] ) throws IOException{
	UnicodeConstant name;

	name = (UnicodeConstant)globals[i.readUnsignedShort()];
	return finishReadAttribute( i, name, locals );
    }

    //
    // for those cases where we already read the name index
    // and know that its not something requiring special handling.
    //
    public static Attribute
    finishReadAttribute( DataInput in, UnicodeConstant name, ConstantObject t[] ) throws IOException {
	int l;
	int n;
	ClassConstant d[];

	l  = in.readInt();
	n  = in.readUnsignedShort();
	d = new ClassConstant[ n ];
	for ( int i = 0; i < n; i++ ){
	    int index = in.readUnsignedShort();
	    d[i] = (ClassConstant)t[ index ];
	}
	return new ExceptionsAttribute( name, l, d );
    }

}
