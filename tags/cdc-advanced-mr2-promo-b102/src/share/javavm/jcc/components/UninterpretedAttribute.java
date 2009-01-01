/*
 * @(#)UninterpretedAttribute.java	1.12 06/10/10
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
import util.*;

/*
 * A class to represent the general Attribute form
 * in its most uninterpreted form. "data" is just an
 * array of byte. It can also be other things, even
 * array of ConstantObject.
 */

public
class UninterpretedAttribute extends Attribute
{
    public byte		data[];

    public
    UninterpretedAttribute( UnicodeConstant name, int l, byte d[] ){
	super( name, l );
	data = d;
    }

    protected int
    writeData( DataOutput o ) throws IOException{
	int trueLength = 0;
	o.write( data, 0, data.length );
	return data.length;
    }

    /*
     * For manipulating byte data 
     */
    public int getInt( int w ){
	return	(  (int)data[w]   << 24 ) |
		(( (int)data[w+1] &0xff ) << 16 ) |
		(( (int)data[w+2] &0xff ) << 8 ) |
		 ( (int)data[w+3] &0xff );
    }

    public int getUnsignedShort( int w ){
	return	(( (int)data[w] &0xff ) << 8 ) | ( (int)data[w+1] &0xff );
    }

    private void putShort( int w, short v ){
	data[w]   = (byte)(v>>>8);
	data[w+1] = (byte)v;
    }


    public static UninterpretedAttribute
    readAttribute(DataInput i, ConstantObject constants[]) throws IOException{
	UnicodeConstant name;
	int l;
	byte d[];

	name = (UnicodeConstant)constants[i.readUnsignedShort()];
	l  = i.readInt();
	d = new byte[ l ];
	i.readFully( d );
	return new UninterpretedAttribute( name, l, d );
    }

    //
    // for those cases where we already read the name index
    // and know that its not something requiring special handling.
    //
    public static Attribute
    finishReadAttribute(DataInput i, UnicodeConstant name) throws IOException {
	int l;
	byte d[];

	l  = i.readInt();
	d = new byte[ l ];
	i.readFully( d );
	return new UninterpretedAttribute( name, l, d );
    }

    public static Attribute[] 
    readAttributes(DataInput i, ConstantObject t[], boolean verbose)
	throws IOException
    {
	int nattr =  i.readUnsignedShort();
	if ( verbose ){
	    System.out.println(Localizer.getString("uninterpretedattribute.reading_attributes", Integer.toString(nattr)));
	}
	if ( nattr == 0 ) return null;
	Attribute a[] = new Attribute[nattr];
	for ( int j = 0; j < nattr; j++ ){
	    a[j] = readAttribute( i, t );
	}
	return a;
    }

}
