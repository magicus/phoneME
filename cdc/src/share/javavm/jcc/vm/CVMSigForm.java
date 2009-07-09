/*
 * @(#)CVMSigForm.java	1.13 06/10/10
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

/*
 * A "form" is the same as a terse signature.
 * It allows sharing of parts of method signatures,
 * as well as facilitating JNI parameter passing.
 */

public
class CVMSigForm {
/* instance data is all public for printing purposes */
    public CVMSigForm		hashNext; // hash chain
    public CVMSigForm		listNext; // enumeration chain
    public int			nSyllables;
    public int			data[];
    public int			listNo;
    public int			entryNo;

/* static data, public for same reason */

    // the two hash tables we use to look these things up.
    // the first is for instances for which data.length==1
    // the second is for all others.

    public final static int	NSINGLETONHASH  = 100;
    public static CVMSigForm	singletonHash[] = new CVMSigForm[NSINGLETONHASH];

    public final static int	NBIGHASH	= 1;
    public static CVMSigForm	bigHash[]	= new CVMSigForm[NBIGHASH];

    public final static int	SYLLABLESPERDATUM= 8;

    // the multiple lists of forms,
    // which is how the writer would just as soon see them.
    // make sure to call "enumerate" to fill in formNo.

    public static CVMSigForm	formList[] = new CVMSigForm[2];
    public static int		listLength[] = new int[2];


    // for computing the "formNo", we assume that if data.length==1,
    // a form takes a single slot. Then for each wordSizeIncrement additional
    // words (or parts thereof), we allocate an additional slot.
    // It would be nice if we had target-system information (such as
    // the size of the structs we're union-ing with) at this point,
    // but we don't so make some assumptions.

    static int			wordSizeIncrement = 3;

    // statistics
    static int			nSingletonForms = 0;
    static int			nBigForms       = 0;

/* constructor and constructor-related */

    private CVMSigForm( int nSyl, int datawords[] ){
	nSyllables = nSyl;
	// COPY the data, as it is probably in a reusable temporary
	// buffer
	data	   = new int[ (nSyl+SYLLABLESPERDATUM-1) / SYLLABLESPERDATUM ];
	System.arraycopy( datawords, 0, data, 0, data.length );
	enlist( this );
    }

    private static synchronized void enlist( CVMSigForm thing ){
	int listNo = thing.nSlots()-1;
	if ( formList.length <= listNo ){
	    CVMSigForm newList[] = new CVMSigForm[ listNo+1 ];
	    System.arraycopy( formList, 0, newList, 0, formList.length );
	    formList = newList;
	    int newLength[] = new int[ listNo+1 ];
	    System.arraycopy( listLength, 0, newLength, 0, listLength.length );
	    listLength = newLength;
	}
	thing.listNext = formList[listNo];
	formList[listNo] = thing;
	listLength[listNo] += 1;
	thing.listNo = listNo;
    }

/* instance methods */
    public int
    nSlots() {
	int nDataWords = (this.nSyllables+SYLLABLESPERDATUM-1) / SYLLABLESPERDATUM;
	return nDataWords;
    }

/* static methods */

    public static void setWordSizeIncrement( int n ){
	wordSizeIncrement = n;
    }

    public static CVMSigForm lookup( int nSyl, int d[] ){
	int 	   hashbucket;
	CVMSigForm hashTable[];
	CVMSigForm f;
	/*
	 * If we cared more about performance, we'd specialize
	 * the singleton search further. For now, just get it right!
	 */
	if ( nSyl <= SYLLABLESPERDATUM ){
	    hashbucket = (int)(((long)d[0]&0xffffffffL) % NSINGLETONHASH);
	    hashTable  = singletonHash;
	} else {
	    hashbucket = 0; // kill me.
	    hashTable  = bigHash;
	}
    searchloop:
	for ( f = hashTable[hashbucket]; f != null; f = f.hashNext ){
	    if ( f.nSyllables != nSyl ) continue;
	    for ( int i = 0; i < f.data.length; i++ ){
		if ( f.data[i] != d[i] ){
		    continue searchloop;
		}
	    }
	    // found it!
	    return f;
	}
	// fell out of the loop because it wasn't there.
	// put it there.
	f = new CVMSigForm( nSyl, d );
	f.hashNext = hashTable[hashbucket];
	hashTable[hashbucket] = f;
	if ( nSyl <= SYLLABLESPERDATUM ){
	    nSingletonForms += 1;
	} else {
	    nBigForms += 1;
	}
	return f;
    }

}
