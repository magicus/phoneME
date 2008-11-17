/*
 * @(#)CVMMethodType.java	1.14 06/10/10
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
package vm;
import java.util.Vector;

/*
 * Representation of signatures.
 * They're little numbers, which are indices into
 * a table.
 */
/*
 * CVM has a complex scheme for representing types as 16-bit
 * cookies. This code mimics the run-time code and thus builds the
 * corresponding data structures, to be ROMed.
 */
public
class CVMMethodType implements CVMTypeCode, util.ClassFileConst {

/* instance fields */
    public int		  entryNo = -1;
    public CVMMethodType  next;	// hash chain
    public CVMSigForm	  form;
    public int		  nDetails;
    public int		  details[];    // array of scalar-type cookies.
    public int		  detailOffset; // for use by writer.
    public CVMMethodType  listNext;     // for variety management.

/* static fields */
    public static Vector methodTypes   = new Vector();

    /* 
     * When you change this, also change the constant NMETHODTYPEHASH
     * in typeid_impl.h 
     */
    public final static int NHASH = 13*37; // arbitrary number, contains no 2
    public static CVMMethodType hashTable[] = new CVMMethodType[ NHASH ];

/* constructor */

    CVMMethodType( CVMSigForm f, int nd, int d[] ){
	form = f;
	nDetails = nd;
	// COPY the data, as it is probably in a reusable temporary
	// buffer
	details	   = new int[ nd ];
	System.arraycopy( d, 0, details, 0, nd );
	synchronized( methodTypes ){
	    methodTypes.addElement( this );
	}
    }

/* static methods */

    private static CVMMethodType
    lookup( CVMSigForm f, int nd, int d[] ){
	int hashBucket = (int)(((long)((f.data[0]<<16) + (nd>0?d[0]:0))&0xffffffffL) % NHASH);
	CVMMethodType mt;
    searchloop:
	for ( mt = hashTable[hashBucket]; mt != null; mt = mt.next ){
	    if ( mt.form != f ) continue;
	    for ( int i = 0; i < nd; i++ ){
		if ( mt.details[i] != d[i] ){
		    continue searchloop;
		}
	    }
	    // found one.
	    return mt;
	}
	/* not in table */
	mt = new CVMMethodType( f, nd, d );
	mt.next = hashTable[hashBucket];
	hashTable[hashBucket] = mt;
	return mt;
    }

    /*
     * static buffers used by parseSignature (and its parser) only 
     */
    static int detailsbuffer[] = new int[257];
    static int formSyllables[] = new int[ CVMSignatureParser.NDATAWORDS ];

    public static CVMMethodType
    parseSignature( String sig ){
	CVMSignatureParser p = new CVMSignatureParser( sig, detailsbuffer, formSyllables );
	p.parse();

	CVMSigForm f = CVMSigForm.lookup( p.nSyllables(), formSyllables );
	return lookup( f, p.nDetails(), detailsbuffer );
    }

}

class CVMSignatureParser extends util.SignatureIterator implements CVMTypeCode, util.ClassFileConst {

/* instance data */
    int		details[];
    int		formData[];
    int		detailNo = 0;
    int		sylNo    = 0;

/* static data */
    public static final int NDATAWORDS = 1+257/CVMSigForm.SYLLABLESPERDATUM;

/* constructor */
    public
    CVMSignatureParser( String sig, int detailArray[], int formDataArray[] ){
	super( sig );
	details = detailArray;
	formData = formDataArray;
    }

/* instance methods */
    /* to return the number of details, form syllables */

    public int nDetails(){ return detailNo; }
    public int nSyllables(){ return sylNo; }

    /* to run the whole show, in the right order */
    public boolean parse(){
	for ( int i = 0; i < NDATAWORDS; i++ ){
	    formData[i] = 0;
	}
	try {
	    iterate_returntype();
	    iterate_parameters();
	    insertSyllable( CVM_T_ENDFUNC );
	} catch ( util.DataFormatException e ){
	    e.printStackTrace();
	    return false;
	}
	return true;
    }

    /* to manipulate the form syllables */
    private void insertSyllable( int v ){
	int wordno = sylNo / CVMSigForm.SYLLABLESPERDATUM;
	int nshift = 4*( sylNo % CVMSigForm.SYLLABLESPERDATUM );
	formData[wordno] |= v << nshift;
	sylNo+=1;
    }

    /* called back from iterate_parameters and iterate_returntype */
    public void do_void(){ insertSyllable( CVM_T_VOID ); }
    public void do_int() { insertSyllable( CVM_T_INT ); }
    public void do_boolean(){ insertSyllable( CVM_T_BOOLEAN ); }
    public void do_char(){ insertSyllable( CVM_T_CHAR ); }
    public void do_short(){ insertSyllable( CVM_T_SHORT ); }
    public void do_long(){ insertSyllable( CVM_T_LONG ); }
    public void do_double(){ insertSyllable( CVM_T_DOUBLE ); }
    public void do_float(){ insertSyllable( CVM_T_FLOAT ); }
    public void do_byte(){ insertSyllable( CVM_T_BYTE ); }

    public void do_array( int depth, int tstart, int tend ){
	int cookie = CVMDataType.parseSignature( sig.substring( tstart-depth, tend+1 ) );
	insertSyllable( CVM_T_OBJ );
	details[detailNo++] = cookie;
    }

    public void do_object( int tstart, int tend ){
	do_array( 0, tstart, tend );
    }
}
