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

package vm;
import java.util.Hashtable;
import java.util.Enumeration;
import components.ClassInfo;
import components.MethodInfo;
import components.MethodConstant;
import jcc.Const;

/*
 * This class will build
 * the imethodtable, used by JDK1.1 for a quicker implementation
 * of invokeinterface_quick.
 * 
 * This class should be subclassed in a target-machine-specific way
 * to provide writing out the data structure we build here. 
 * See also InterfaceMethodFactory for the mechanism used to call back
 * into that target-specific body of code.
 * 
 * Each ClassInfo is already decorated with a vector of
 * all the interfaces it implements. In addition, each
 * ClassInfo's allInterfaces vector has its parent's vector as prefix.
 * So:
 * A) a class which implements the same number of interfaces as its parent
 *    necessarily implements the same set of interfaces, and can use its
 *    parent's table, by reference.
 * B) for those classes for which a table must indeed be built,
 *    Only the interface elements associated with the present
 *    ClassInfo need to have a method index table built.
 */
public abstract
class InterfaceMethodTable {

    public ClassClass	   parent;
    public String	   name;
    public InterfaceVector iv[];

    protected InterfaceMethodTable( ClassClass c, String n, InterfaceVector v[] ){
	parent = c;
	name = n;
	iv = v;
    }

    private static MethodInfo findSlot( MethodConstant t[], MethodInfo m ){
	if ( t != null ){
	    MethodInfo v;
	    int nt = t.length;
	    int ID = m.getID();
	    for( int i = 0; i < nt; i++ ){
		if ((v=t[i].find()).getID() == ID )
		    return v;
	    }
	}
	return null;
    }

    /*
     * Iterate through the methods of an interface.
     * For the non-static members (i.e. the non-<clinit> )
     * assign a zero-based index, and use the methodTableIndex field
     * to keep it in.
     */
    public void 
    indexInterfaceMethods( ClassInfo c ){
	if ( (c.access&Const.ACC_INTERFACE) == 0 ){
	    System.err.println("InterfaceMethodTable.indexInterfaceMethods: "+c.className
		+" is not an interface");
	    return; // this is bad
	}
	int j = 0;
	MethodInfo m[] = c.methods;
	if ( m == null ) return; // no methods here.
	int n = m.length;
	for( int i =0; i < n; i++ ){
	    MethodInfo t = m[i];
	    if ( (t.access&Const.ACC_STATIC) != 0 )
		continue; // don't number statics.
	    if ( (t.access&Const.ACC_ABSTRACT) == 0 ){
		System.err.println("InterfaceMethodTable.indexInterfaceMethods: "+t
		+" is not abstract");
		// but keep going anyway...
	    }
	    t.methodTableIndex = j++;
	}
    }

    private static InterfaceMethodTable
    generateTablesForInterface( ClassInfo c, String imtName, InterfaceMethodFactory imf ){
	//
	// We generate a thing like an imethodtable for interfaces, too.
	// But its different enough that I'm treating it separately, here.
	//
	// It is complex to share our imethotable with our superclass,
	// as it must at least include ourselves!
	//
	// struct imethodtable { 
	//     int icount;
	//     struct { 
	// 	ClassClass *classdescriptor;
	// 	unsigned long *offsets = 0;
	//     } itable[1];
	// }
	ClassClass me = c.vmClass;
	int ntotal = c.allInterfaces.size();
	InterfaceVector ivec[] = new InterfaceVector[ ntotal+1 ];

	// first the entry for ourselves.
	ivec[ 0 ] = new InterfaceVector( me, c, null );
	// now all the others (if any)
	for( int i = 0; i< ntotal; i++){
	    ClassInfo intf = (ClassInfo)c.allInterfaces.elementAt( i );
	    ivec[ i+1 ] = new InterfaceVector( me, intf, null );
	}
	return imf.newInterfaceMethodTable( me, imtName, ivec );
    }

    public static InterfaceMethodTable
    generateInterfaceTable( ClassClass cc, InterfaceMethodFactory imf ){
	ClassInfo c = cc.ci;
	if ( c.allInterfaces == null )
	    c.findAllInterfaces();
	String imtName = c.getGenericNativeName() + "_intfMethodtable";
	ClassInfo sup = c.superClassInfo;
	int ntotal = c.allInterfaces.size();
	int nsuper = 0;

	if ( (c.access&Const.ACC_INTERFACE) != 0 ){
	    return generateTablesForInterface( c, imtName, imf );
	}
	if ( sup != null ){
	    nsuper = sup.allInterfaces.size();
	    if ( nsuper == ntotal ){
		// use other class's tables entirely.
		// we have nothing further to add.
		return sup.vmClass.inf;
	    }
	}
	//
	// generate the offset tables, or symbolic references
	// to them.
	InterfaceVector vec[] = new InterfaceVector[ ntotal ];
	if ( nsuper != 0 ){
	    // borrow some from superclass. They are the same.
	    System.arraycopy( sup.vmClass.inf.iv, 0, vec, 0, nsuper );
	}

	// compute the rest of the thing ourselves.
	for( int i = nsuper; i < ntotal; i++ ){
	    ClassInfo intf = (ClassInfo)c.allInterfaces.elementAt( i );
	    MethodConstant mtab[] = intf.refMethodtable;
	    int nmethods = mtab.length;
	    short ivec[] = new short[ nmethods ];
	    for( int j = 0; j < nmethods; j++ ){
		MethodInfo target = mtab[j].find();
		if ( (target.access&Const.ACC_STATIC) != 0 ){
		    // should never happen.
		    System.err.println("Interface "+intf.className+" has a static method in its methodtable:! "+target);
		    continue;
		}
		MethodInfo v = findSlot( c.refMethodtable, target );
		if ( v == null ){
		    System.err.println("Class "+c.className+" does not implement interface "+intf.className+" because it lacks "+target);
		    ivec[ j ] = 0;
		} else {
		    ivec[ j ] = (short)v.methodTableIndex;
		}
	    }
	    vec[ i ] = new InterfaceVector( cc, intf, ivec );
	}

	return imf.newInterfaceMethodTable( cc, imtName, vec );
    }
}
