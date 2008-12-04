/*
 * @(#)CVMInterfaceMethodTable.java	1.23 06/10/10
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
import vm.*;
import components.*;
import consts.Const;
import java.util.Hashtable;

/*
 * This is the CVM-specific subclass of vm.InterfaceMethodTable
 * It knowns how to write out these data structures for the CVM
 * This is similar to the CInterfaceMethodTable used for
 * JDK1.1-based systems. The substantial difference is
 * the ordering requirement: rather than having two representations
 * of interfaces, CVM has only one. Those this class implements
 * directly come first, followed by those inhereted.
 * Also note that the count of interfaced declared by the class is part of this
 * structure. This would seem to limit the ability of a child class to share data
 * with its parent in the case where the parent declares an interface
 * but the child does not. The CVM recognizes this case though, so this
 * sharing is allowed. Here's an odd example:
 * 	class a implements Serializeable {}
 *	class b extends a {}
 *		// b shares a's interface table
 *	class c extends a implements Serializeable {}
 *		// c cannot share a's interface table
 *		// though they are identical!
 */

class CVMInterfaceMethodTable extends vm.InterfaceMethodTable {

    // names we generate
    public static final String vectorName = "CVMinterfaceVector";

    int				implementsCount;
    boolean			generated;

    public
    CVMInterfaceMethodTable( ClassClass c, String n, InterfaceVector v[] ){
	super( c, n, v );
	implementsCount = (c.classInfo.interfaces == null) ?
                              0 : c.classInfo.interfaces.length;
	generated = false;
    }

    private static void
    sortInterfaceVectors( ClassInfo c, InterfaceVector v[], boolean isInterface ){
	/*
	 * Each of these interface vectors is correct, BUT order matters
	 * We want to make sure that the entries for interfaces declared directly
	 * by us come first in the list, AND that they are in the order declared!
	 * so do a simple sort of them here.
	 */
	ClassConstant interfaces[] = c.interfaces;
	int implementsCount = interfaces.length;
	int l = v.length;
	for ( int i = 0; i < implementsCount; i++ ){
	    ClassInfo thisInterface = interfaces[i].find();
	    for ( int j = i; j < l; j++ ){
		if ( v[j].intf == thisInterface ){
		    if ( i != j ){
			// swap
			InterfaceVector t = v[i];
			v[i] = v[j];
			v[j] = t;
		    }
		    break;
		}
	    }
	}
	if ( isInterface ){
	    /*
	     * put this class itself next.
	     */
	    // this will fail badly if this class is not present.
	    int j = implementsCount;
	    while ( v[j].intf != c ){
		j++;
	    }
	    if ( j != implementsCount ){
		// swap
		InterfaceVector t = v[implementsCount];
		v[implementsCount] = v[j];
		v[j] = t;
	    }
	}
    }

    private static InterfaceMethodTable
    generateTablesForInterface( ClassInfo c ){
	//
	// We generate a thing like an imethodtable for interfaces, too.
	// But its different enough that I'm treating it separately, here.
	//
	// Cannot share our imethotable with our superclass,
	// as it must at least include ourselves!
	//
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
	sortInterfaceVectors( c, ivec, true );
	return new CVMInterfaceMethodTable(
	    c.vmClass, "CVM_"+c.getGenericNativeName()+"_intfMethodtable", ivec );
    }


    /*
     * Modified version from vm.InterfaceMethodTable for 
     * handling the special ordering and sharing needs of CVM.
     */
    public static InterfaceMethodTable
    generateInterfaceTable( ClassClass cc ){
	ClassInfo c = cc.classInfo;
	if ( c.allInterfaces == null )
	    c.findAllInterfaces();
	ClassInfo sup = c.superClassInfo;
	int ntotal = c.allInterfaces.size();
	int nsuper = 0;
	int implementsCount = (c.interfaces==null)?0:c.interfaces.length;

	if ( (c.access&Const.ACC_INTERFACE) != 0 ){
	    return generateTablesForInterface( c );
	}
	if ( sup != null ){
	    if ( sup.vmClass.inf == null ){   
		// generate parental information,
		// that we might borrow it.
		sup.vmClass.inf = generateInterfaceTable( sup.vmClass );
	    }
	    if ( implementsCount == 0 ){
		// CVM allows sharing with parent if we have no
		// declared interfaces ourselves.
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
	    vec[ i ] = generateInterfaceVector( c, intf );
	}

	/*
	 * Each of these interface vectors is correct, BUT order matters
	 * We want to make sure that the entries for interfaces declared directly
	 * by us come first in the list, AND that they are in the order declared!
	 * so do a simple sort of them here.
	 */
	sortInterfaceVectors( c, vec, false );

	return new CVMInterfaceMethodTable(
	    cc, "CVM_"+c.getGenericNativeName()+"_intfMethodtable", vec );
    }

    static void writeInterfaceTables(ClassClass[] classes, CCodeWriter out, CCodeWriter headerOut) {

        int n = classes.length;
	// generate the tables. generateInterfaceTable must work
	// recursively, of course!
	for ( int i = 0; i < n; i++ ){
	    ClassClass c = classes[i];
	    if ( c.inf == null )
		c.inf = generateInterfaceTable(c );
	}

	//
	// print the vectors first.
	//
	out.println( "STATIC const CVMUint16 " + vectorName + "[] = {");
	int offset = 0;
	boolean emptyVector = true;
	for (int i = 0; i < n; i++) { 
	    ClassClass c = classes[i];
	    CVMInterfaceMethodTable simt = (CVMInterfaceMethodTable)c.inf;
	    if (simt == null || simt.parent != c) 
	        continue;
	    InterfaceVector iv[] = simt.iv;
	    for (int j = 0; j < iv.length; j++) { 
	        InterfaceVector vec = iv[j];
		if ( vec.generated )
		    continue; // done.
		vec.generated = true;
		vec.offset = offset;
		short vector[] = vec.v;
		if ( (vector==null) || (vector.length == 0) ){
		    continue;
		}
		int num = vector.length;
		emptyVector = false;
		out.comment(simt.parent.classInfo.className +
                            " " + vec.intf.className);
		for (int k = 0, mod = 0; k < num; k++) { 
		    if (mod == 0) out.write('\t');
		    out.printHexInt( vector[k] );
		    out.write(',');
		    if (++mod == 8) { 
		        out.write('\n'); mod = 0;
		    }
		}
		if ((num % 8) != 0) 
		  out.write('\n');
		offset += num;
	    }
	}
	if ( emptyVector ){
	    out.write('0');// make it non-empty!
	}
	out.println("};\n");
	// end vectors.
	
	//
	// now write out the CVMInterfaces structures, which
	// roughly correspond to the InterfaceMethodTables.
	// But not quite.
	//
	for (int i = 0; i < n; i++) { 
	    ClassClass c = classes[i];
	    CVMInterfaceMethodTable simt = (CVMInterfaceMethodTable)c.inf;
	    if ( simt != null ){
	        simt.write(out, headerOut);
	    }
	}
    }

    private static boolean
    needVector( InterfaceVector vec ){
	if ( vec.v == null || vec.v.length == 0 ) return false; // don't bother.
	return true;
    }

    private static java.util.BitSet	interfaceSizes = new java.util.BitSet();
    private static final String		interfaceStructName = "CVMinterface";
	
    //struct CVMInterfaces {
    //    CVMUint16      interfaceCount;
    //    CVMUint16      implementsCount;
    //    union {
    //        CVMUint16* interfaces;
    //        struct {
    //            CVMClassBlock* interfacecb;
    //            CVMUint16*     index;
    //        } interfaceTable[1];
    //    } intfInfo;
    //}; 

    private static String declareInterfaceStruct( int n, CCodeWriter out ){
	String interfaceFlavor = "union "+interfaceStructName+n;
	if ( ! interfaceSizes.get( n ) ){
	    out.println(interfaceFlavor+" {");
	    out.println("    struct {");
	    out.println("        CVMUint16 interfaceCount; CVMUint16 implementsCount;");
	    out.println("        struct { const CVMClassBlock* interfacecb;" +
			" union {CVMUint16 * addr; CVMClassTypeID ti;} ext;" +
			"} interfaceTable["+n+"];");
	    out.println("    } dontAccess;");
	    out.println("    CVMInterfaces interfaces;");
	    out.println("};");
	    interfaceSizes.set( n );
	}
	return interfaceFlavor;
    }

    public void
    write( CCodeWriter out, CCodeWriter headerOut){
        if (this.generated) 
	    return;
	this.generated = true;
	if ( iv.length == 0 ){
	    // omit structures of length 0
	    return;
	}    
	String structType = declareInterfaceStruct( iv.length, headerOut );
	/* 
	 * Added 'extern' keyword, so that the header file does not
	 * declare an (unnecessary) global variable for each struct
	 * declaration (which would never be used anyway) */
	headerOut.println("extern const "+structType+" "+name+";");
	out.print("const "+structType+" "+name);
	out.print(" = {{\n\t");
	out.print( iv.length );
	out.print(", ");
	out.print( implementsCount );
	out.println(", {");
	for( int i = 0; i < iv.length; i++ ){
	    InterfaceVector entry = iv[i];
	    ClassInfo intf = entry.intf;
	    String intfName = intf.getGenericNativeName();
	    out.print("\t{ ");
	    out.print("(CVMClassBlock*)&"+intfName);
	    out.print("_Classblock.classclass,");
	    if ( needVector( entry ) ){
		/*
		 * Cast to first type in union (CVMUint16 *).
		 */
		out.print("{(CVMUint16 *)&"+vectorName+"["+entry.offset+"]}");
	    } else {
		/*
		 * Cast to first type in union (CVMUint16 *).
		 */
		out.print("{(CVMUint16 *)0}");
	    }
	    out.println(" },");
	}
	out.write('}');
	out.println("}};");
    }
}
