/*
 * @(#)CVMStringTable.java	1.38 06/10/22
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
import java.util.Enumeration;
import consts.Const;
import components.*;
import vm.*;
import util.*;

public class CVMStringTable extends vm.StringTable {

    CVMStringIntern internTable;
    private CVMInitInfo initInfo;

    CVMStringTable(CVMInitInfo initInfo) 
    {
	this.initInfo = initInfo;
    }
    
    final boolean writeStringData( String dataName, String charArrayClassBlockName, CCodeWriter out ){
	int n = arrangeStringData();

	if ( n == 0 ) return false; // nothing to do here.

	char v[] = new char[n];
	this.fillInChars( 0, n, v, 0 );

	//
	// first typedef the structure we're about to create
	/* 
	 * The following structure has to reflect CVMArrayOfChar which
	 * has a padding field for 64 bit platforms to be sure
	 * that the membler elems has the same offset for all
	 * CVMArrayOf<type> structures.
	 */
	out.print("const struct { CVMObjectHeader hdr;\n    CVMJavaInt	length;\n#ifdef CVM_64\n    CVMUint32 pad;\n#endif\n    CVMJavaChar stringData["+n+"]; } ");
	out.print( dataName );
	out.println( " = {" );
	//
	// fill in the header information
	// hdr.various = {{0}} ??
	int hashCode = 0;
	out.println("	CVM_ROM_OBJECT_HDR_INIT0("+charArrayClassBlockName+
	    ","+hashCode+"),");
	out.println("	"+n+",\n#ifdef CVM_64\n	0,\n#endif\n	{");
	//
	// finally write the char array.
	int mod = 0;
	for (int i = 0; i < n; i++) { 
	    if (mod == 0) 
	        out.write('\t');
	    out.printHexInt(v[i]);
	    out.write(',');
	    if (++mod == 12) { 
	        out.write('\n');
		mod = 0;
	    }
	} 
	if (mod != 0)
	    out.write('\n');
	out.print(" } };\n\n");

	return true;

    }

    private final static int maxCom = 20;
    private static byte cbuf[] = new byte[maxCom + 3];
    private void commentary( String s, CCodeWriter out ){
	int m = Math.min( maxCom, s.length() );
	for ( int i = 0; i < m; i++ ){
	    char c = s.charAt(i);
	    if ( ' ' <= c && c <= '~' ) {
		if (c == '*') {
		    /*
		     * Prevent damage to C comment by replacing * with ?.
		     */
		    c = '?';
		}
		cbuf[i] = (byte) c;
	    } else {
		cbuf[i] = (byte)'?';
	    }
	}
	out.write( out.commentLeader, 0, out.commentLeader.length );
	out.write( cbuf, 0, m );
	if ( m < s.length() ){
	    out.write('.');
	    out.write('.');
	    out.write('.');
	}
	out.write( out.commentTrailer, 0, out.commentTrailer.length );
    }

    public int writeStrings(CCodeWriter out, String tableName ) {
	String dataName = tableName+"_data";

	ClassInfo charArrayClass = ClassTable.lookupClass("[C");
	if ( charArrayClass == null ){
	    System.err.println(Localizer.getString("javacodecompact.cannot_find_array_of_char"));
	    return 0;
	}

	if ( ! writeStringData( dataName, ((CVMClass)(charArrayClass.vmClass)).getNativeName()+"_Classblock" , out ) )
	    return 0;

	out.println("const CVMUint16 " + tableName + "[] = {");

	StringConstant stringTable[] = allStrings();

	internTable = new CVMStringIntern( initInfo, stringTable.length );

	int currentLength = -1;
	int currentLengthCount = -1;
	int numCompressedEntries = 0;
	
	for ( int i = 0; i < stringTable.length; i++ ){
	    StringConstant s = stringTable[i];
	    String string = s.str.string;
	    int hashCode = 0;
	    if (currentLength != string.length()) {
		if (currentLengthCount >= 0) {
		    out.println("/* COUNTS:   "+
				currentLengthCount+
				" instances of "+
				currentLength+
				"-character-long strings */");
		    out.println("\t"+currentLengthCount+", "+
				currentLength+",");
		    numCompressedEntries++;
		}		
		currentLength = string.length();
		currentLengthCount = 0;		
	    }
	    currentLengthCount++;
	    out.print("\t");
	    commentary(string, out);
	    internTable.internStringConstant( s );
	}
	if (currentLengthCount >= 0) {
	    out.println("/* COUNTS:   "+
			currentLengthCount+
			" instances of "+
			currentLength+
			"-character-long strings */");
	    out.println("\t"+currentLengthCount+", "+
			currentLength+",");
	    numCompressedEntries++;
	}		
	out.println("};\n\n");

	out.println("const int CVM_nROMStringsCompressedEntries = "+numCompressedEntries+";");

	return stringTable.length;
    }

    /* 
     * Moved global variables to CVMROMGlobals
     * That makes it necessary to pass the global header out file into writeInternTable.
     */
    public void writeStringInternTable( CCodeWriter out, CCodeWriter globalHeaderOut, String tableName, String stringArrayName ){
	internTable.writeInternTable( out, globalHeaderOut, tableName, stringArrayName );
    }
}



class CVMInternSegment {
    int 	     capacity;
    int		     content = 0;
    StringConstant   data[];

    private static final int primeFactors[] = {
	3, 5, 7, 11, 13, 37 };
    private static final int nPrimeFactors = primeFactors.length;

    CVMInternSegment( int size ){
	/*
	 * apply our usual emptyness parameter.
	 * then make sure its relatively prime to
	 * the secondary hash h1, where 1 <= h1 <= 16
	 * and also to 37. This means testing against
	 * 2, 3, 5, 7, 11, 13, and 37.
	 * This loop WILL terminate, because eventually
	 * we will hit either a relative prime to all these
	 * factors, or an actual prime.
	 */
	size = ( size*100 ) / 65; 
	size |= 1; /* not a multiple of 2 */
	boolean changed = true;
	while ( changed ) {
	    changed = false;
	    for ( int i = 0; i < nPrimeFactors; i++ ){
		if (size%primeFactors[i] == 0 ){
		    size += 2;
		    changed = true;
		    break; /* out of for loop */
		}
	    }
	}
	capacity = size;
	data = new StringConstant[ size ];
    }

    void writeSegment(
	String segmentName,
	String stringArrayName,
	String link,
	CCodeWriter out )
    {
	//
	// first typedef the array we're about to create
	out.print("const struct { CVM_INTERN_SEGMENT_HEADER\n\tCVMStringICell data[");
	out.print( capacity );
	out.print( "];\n\tCVMUint8	refCount[" );
	out.print( capacity );
	out.print( "]; } " );
	out.print( segmentName );
	out.print(" = {\n    &" );
	/* 
	 * Moved globals to CVMROMGlobals.
	 */
	out.print("CVMROMGlobals." + link);
	out.print(", ");
	out.print(capacity);
	out.print(", ");
	out.print(content);
	out.println(", 1, {");

	//
	// write the String reference array.
	for (int i = 0; i < capacity; i++) { 
	    StringConstant sc = data[i];
	    if ( sc == null ){
		out.println("    {0},");
	    } else {
		/* 
		 * Moved globals to CVMROMGlobals.
		 */
		out.println("    {(CVMObject*)&CVMROMGlobals."+stringArrayName+"["+sc.unicodeIndex+"]},");
	    }
	}
	out.print("}, {\n    ");
	// now the reference counts: all either unused or sticky
	int perLine = 0;
	for (int i = 0; i < capacity; i++) { 
	    StringConstant sc = data[i];
	    if ( sc == null ){
		out.print("StrU, ");
	    } else {
		out.print("StrS, ");
	    }
	    if (++perLine > 8 ){
		out.print("\n    ");
		perLine = 0;
	    }
	}
	out.println("\n}};");
    }

}

class CVMStringIntern{

    CVMInternSegment internTable;
    CVMInitInfo      initInfo;

    CVMStringIntern( CVMInitInfo initInfo, int maxSize ){
	this.initInfo = initInfo;
	internTable = new CVMInternSegment( maxSize );
    }

    StringConstant internStringConstant( StringConstant val ){
	String s = val.str.string;
	CVMInternSegment curseg = internTable;

	int h = 0, n;
	// Hash function.
	// Identical to the one in the runtime interning code.
	n = Math.min( s.length(), 16 );
	for ( int i = 0; i < n; i++ ){
	    h = (h*37) + s.charAt(i);
	}


	int h1 = (h&0x7fffffff) % curseg.capacity;
	int h2 = (h&15)+1;
	int i = h1;
	StringConstant candidate;

	while ( (candidate=curseg.data[i]) != null ){
	    if ( val.equals( candidate ) ){
		return candidate;
	    }
	    i += h2;
	    if ( i >= curseg.capacity ){
		i -= curseg.capacity;
	    }
	}
	// not found in this table.
	// insert at current location.
	curseg.data[i] = val;
	curseg.content += 1;
	return val;
    }

    /* 
     * Moved global variables to CVMROMGlobals
     * That makes it necessary to pass the global header out file into writeInternTable.
     */
    public void writeInternTable(
	CCodeWriter out,
	CCodeWriter globalHeaderOut,
	String tableName,
	String stringArrayName )
    {
	/* the StringIntern class has no EMV equivalent -- it becomes
	 * part of java.lang.String. Only the InternSegment exists.
	 * Go write it out.
	 */
	String linkname;
	// Note : define more compact representation of
	// CVMInternUnused and CVMInternSticky to reduce our
	// output volumn
	out.print("#undef StrS\n#undef StrU\n");
	out.print("#define StrS	CVMInternSticky\n");
	out.print("#define StrU	CVMInternUnused\n");
	//
	// write out the indirect next cell, and see that it gets
	// initialized.
	linkname = tableName+"NextCell";
	/* 
	 * Moved global variables to CVMROMGlobals
	 */
	globalHeaderOut.println("    struct CVMInternSegment * "+linkname+";");
	initInfo.addInfo("NULL", "&CVMROMGlobals."+linkname, "sizeof (CVMROMGlobals."+linkname+")" );
	internTable.writeSegment( tableName, stringArrayName, linkname, out );
    }
}
