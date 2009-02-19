/*
 * @(#)CVMMemberNameEntry.java	1.8 06/10/10
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
 * Collect member names. Turn them into little integers.
 * Build a hash table of them for writing out to the romjava image.
 */
public class CVMMemberNameEntry{
/* instance members */
    public String		name;
    public CVMMemberNameEntry	next;
    public int			entryNo;

/* static members */
    public final static int NMEMBERNAMEHASH = 41*13;
    public static CVMMemberNameEntry hash[] = new CVMMemberNameEntry[ NMEMBERNAMEHASH ];
    static int 		nextEntryNo = 0;
    public static Vector table = new Vector();

/* constructor &c */

    static synchronized void enlist( CVMMemberNameEntry x, int xhash ){
	x.entryNo = nextEntryNo++;
	table.addElement( x );
	x.next = hash[xhash];
	hash[xhash] = x;
    }

    CVMMemberNameEntry( String myname, int myhash ){
	name = myname;
	enlist( this, myhash );
    }

/* static methods */
    /*
     * We use a very simple minded name hashing scheme.
     * Based on the LAST few letters of the name, which are
     * the most likely to differ, I believe.
     * Same as for CVMDataType.
     */
    public static int
    computeHash( String name ){
	int v = 0;
	int t = name.length();
	int n = Math.min( t, 7 );
	t -= n;
	while ( n--> 0 ){
	    v = (v*19) + name.charAt(t+n) - '/';
	}
	return v;
    }

    public static int lookupEnter( String name ){
	int hval = (int)(((long)(computeHash( name ))&0xffffffffL) % NMEMBERNAMEHASH);
	name = name.intern();
	CVMMemberNameEntry mne;
	for ( mne = hash[hval]; mne!=null; mne = mne.next ){
	    if ( mne.name == name ) return mne.entryNo;
	}
	return new CVMMemberNameEntry( name, hval).entryNo;
    }

    public static int tableSize(){
	return nextEntryNo;
    }
}
