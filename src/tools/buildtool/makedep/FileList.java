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

package makedep;

import java.io.*;
import java.util.*;

/** This class implements the java.util.List interface as well as
    providing functionality specific to keeping track of lists of
    files. See the documentation for the Database class to see how
    these are used. Each FileList must only contain other FileLists
    (although that is not currently enforced in the mutators). */

public class FileList extends Vector {
    private String name; // (also the file name)
    private boolean beenHere;
    private boolean mayBeCycle;
    private boolean isCycle;
    /** Put in list because a file can refuse to */
    private boolean useGrandInclude;
    private String platformDependentInclude;
    private int count;
    private Platform plat;

    public FileList(String n, Platform plat) {
	super();
	this.plat = plat;
	beenHere = mayBeCycle = isCycle = false;
	platformDependentInclude = null;
	name = n;
	count = 0;
	useGrandInclude = plat.haveGrandInclude();
    }

    // Change definition of equality from AbstractList so remove() works properly
    public boolean equals(Object o) {
      return ((Object) this) == o;
    }

    // Necessary accessors
    public String getName() {
	return name;
    }

    public void setPlatformDependentInclude(String arg) {
	platformDependentInclude = arg;
    }

    public String getPlatformDependentInclude() {
	return platformDependentInclude;
    }

    public boolean getUseGrandInclude() {
	return useGrandInclude;
    }

    public void setUseGrandInclude(boolean arg) {
	useGrandInclude = arg;
    }

    public void incrementCount() {
	count++;
    }

    public int getCount() {
	return count;
    }

    public FileList listForFile(String fileName) {
	for (Iterator iter = iterator(); iter.hasNext(); ) {
	    FileList fl = (FileList) iter.next();
	    if (plat.fileNameStringEquality(fl.name, fileName)) {
		plat.fileNamePortabilityCheck(fl.name, fileName);
		return fl;
	    }
	}
	plat.fileNamePortabilityCheck(fileName);
	FileList newList = new FileList(fileName, plat);
	add(newList);
	return newList;
    }

    public boolean hasListForFile(String fileName) {
	for (Iterator iter = iterator(); iter.hasNext(); ) {
	    FileList fl = (FileList) iter.next();
	    if (plat.fileNameStringEquality(fl.name, fileName)) {
		plat.fileNamePortabilityCheck(fl.name, fileName);
		return true;
	    }
	}
	return false;
    }

    public boolean compareLists(FileList s) {
	Iterator myIter = iterator();
	Iterator hisIter = s.iterator();

	while (myIter.hasNext() &&
	       hisIter.hasNext()) {
	    // crude: order dependent
	    FileList myElement = (FileList) myIter.next();
	    FileList hisElement = (FileList) hisIter.next();
	    if (!plat.fileNameStringEquality(myElement.name,
					     hisElement.name)) {
		return false;
	    }
	}
	
	if (myIter.hasNext() != hisIter.hasNext()) {
	    // One ended earlier
	    return false;
	}
	
	return true;
    }

    public void addIfAbsent(FileList s) {
	for (Iterator iter = iterator(); iter.hasNext(); ) {
	    if (iter.next() == s) {
		return;
	    }
	}
	add(s);
    }

    public void sortByName() {
	Collections.sort(this, new Comparator() {
		public int compare(Object o1, Object o2) {
		    FileList fl1 = (FileList) o1;
		    FileList fl2 = (FileList) o2;
		    return fl1.getName().compareTo(fl2.getName());
		}
	    });
    }

    public void setFirstFile(FileList s) {
      // Remove the file list if it's already here
      remove(s);
      add(0, s);
    }

    public void setLastFile(FileList s) {
      // Remove the file list if it's already here
      remove(s);
      add(s);
    }

    public boolean doFiles(FileList s) {
	boolean result = true;
	for (Iterator iter = iterator(); iter.hasNext(); ) {
	    FileList h = (FileList) iter.next();
	    if (h.platformDependentInclude != null) {
		System.err.println("Error: the source for " +
				   h.platformDependentInclude +
				   " is " + h.name + ".");
		System.err.println("\tIt shouldn't be included directly by " +
				   name + ".");
		h.platformDependentInclude = null; // report once per file
		result = false;
	    }
	    h.doHFile(s);
	}
	return result;
    }
    
    public void traceCycle(FileList s) {
	if (isCycle) // already traced
	    return;
	isCycle = true;
	System.err.println("\ttracing cycle for " + name);
	// IMPL_NOTE: must return status in caller routine
	// exitCode = 1;
	for (Iterator iter = iterator(); iter.hasNext(); ) {
	    FileList q = (FileList) iter.next();
	    if (q.mayBeCycle) {
		if (s == q) {
		    plat.fatalError("\tend of cycle for " + s.getName());
		} else {
		    q.traceCycle(s);
		}
	    }
	}
    }
    
    public void doHFile(FileList s) {
	if (beenHere) {
	    if (mayBeCycle) {
		traceCycle(this);
	    }
	    return;
	}
	beenHere = true;
	mayBeCycle = true;
	doFiles(s);
	mayBeCycle = false;
	s.add(this);
    }

    public FileList doCFile() {
	FileList s = new FileList(name, plat);
	s.useGrandInclude = useGrandInclude; // propagate this
	doFiles(s);
	for (Iterator iter = s.iterator(); iter.hasNext(); ) {
	    FileList l = (FileList) iter.next();
	    l.beenHere = false;
	}
	return s;
    }
    
    /** if .h file is included thresh times, put it in the grand
        include file */
    public void putInclFile(Database db)
	throws IOException {
        boolean needline = true;
	FileName inclName = plat.getInclFileTemplate().copyStem(name);

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        OutputStreamWriter writer = new OutputStreamWriter(baos);
    	PrintWriter inclFile = new PrintWriter(writer);

	if (plat.haveGrandInclude() && plat.includeGIInEachIncl()) {
	    inclFile.println("# include \"" +
			     plat.getGIFileTemplate().dirPreStemAltSuff() +
			     "\"");
	    needline = false;
	}
	for (Iterator iter = iterator(); iter.hasNext(); ) {
	    FileList hfile = (FileList) iter.next();
	    if (!db.hfileIsInGrandInclude(hfile, this)) {
                String incname = plat.getInclFileTemplate().getInvDir() + 
				 db.tryGetFullPath(hfile.name);
                incname = plat.translateFileName(incname);
		inclFile.println("#include \"" + incname + "\"");
	        needline = false;
	    }
	}

	// Solaris C++ in strict mode warns about empty files

	if (needline) {
	    inclFile.println();
	}
        inclFile.flush();
        Database.updateFile(inclName.dirPreStemSuff(), baos);
	inclFile.close();
    }
}
