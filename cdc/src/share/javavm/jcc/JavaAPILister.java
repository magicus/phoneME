/*
 * @(#)JavaAPILister.java	1.11	06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.  
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

import consts.*;
import components.*;
import util.*;
import jcc.*;

import java.io.PrintStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.File;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.Hashtable;
import java.util.Set;
import java.util.Vector;

public class JavaAPILister extends LinkerUtil {
    int	         verbosity = 0;
    String	 commandOptions[];

    ClassFileFinder  searchPath;

    ClassReader rdr;

    ClassnameFilter	includeClasses;

    PrintStream		memberOut = System.out;
    PrintStream		classOut = null;

    private static String
    memberFileHeader[] = {
	"#",
	"# %"+"W%	%"+"E%", // use constant expressions to fool SCCS
	"# List of visible class members",
	"#"
    };

    private static String
    classFileHeader[] = {
	"#",
	"# %"+"W%	%"+"E%", // use constant expressions to fool SCCS
	"# List of visible classes",
	"#"
    };

    private boolean
    readFile( String fileName, Vector classesProcessed ){

	if (rdr == null){
	    rdr = new ClassReader(0);
	}
	try {
	    if (fileName.endsWith(".zip") || fileName.endsWith(".jar")){ 
		rdr.readZip(fileName, classesProcessed);
	    } else { 
		rdr.readFile(fileName, classesProcessed);
	    }
	} catch ( IOException e ){
	    System.out.println(Localizer.getString("javacodecompact.could_not_read_file", fileName));
	    e.printStackTrace();
	    return false;
	}
	return true;
    }


    Vector  classesProcessed = new Vector();

    private void
    addToIncludes( String pattern ){
	if (includeClasses == null){
	    includeClasses = new ClassnameFilter();
	}
	includeClasses.includeName(pattern);
	if (verbosity > 0){
	    System.err.print("Adding to include list: ");
	    System.err.println(pattern);
	}
    }

    private boolean
    isClassIncluded(String classname){
	if (includeClasses == null){
	    return true;
	}
	return includeClasses.accept(null, classname);
    }

    private static BufferedPrintStream 
    openOutputFile(String name){
	File outFile = new File(name);
	try {
	    return new BufferedPrintStream( new FileOutputStream( outFile ) );
	}catch(java.io.IOException e){
	    System.err.println("Could not open file "+name);
	    return null;
	}
    }

    private boolean
    processOptions( String clist[] ){
	boolean success = true;
	Vector classesThisRead = new Vector();

	for( int i = 0; i < clist.length; i++ ){
	    if ( clist[i].equals(/*NOI18N*/"-v") ){
		verbosity++;
		continue;
	    }else if (clist[i].equals("-i")){
		addToIncludes(clist[++i]);
		continue;
	    } else if ( clist[i].equals(/*NOI18N*/"-classpath")  ){
		if ( searchPath == null )
		    searchPath = new ClassFileFinder();
		searchPath.addToSearchPath( clist[ ++i ] );
	    } else if ( clist[i].equals(/*NOI18N*/"-mout")  ){
		memberOut = openOutputFile(clist[ ++i ]);
		if (memberOut == null){
		    // message already written
		    success = false;
		    continue;
		}
	    } else if ( clist[i].equals(/*NOI18N*/"-cout")  ){
		classOut = openOutputFile(clist[ ++i ]);
		if (classOut == null){
		    // message already written
		    success = false;
		    continue;
		}
	    } else if ( clist[i].equals("-f") ){
		try {
		    success = processOptions( parseOptionFile( clist[++i] ) );
		} catch ( java.io.IOException e ){
		    e.printStackTrace();
		    success = false;
		}
	    } else { 
		classesThisRead.clear();
		if (!readFile( clist[i], classesThisRead )){
		    success = false;
		    // but keep going to process rest of options anyway.
		}
		classesProcessed.addAll(classesThisRead);
		ClassTable.enterClasses(classesThisRead.elements(), false);
	    }
	}

	return success;
    }


    private void
    printFileHeader(PrintStream out, String header[]){
	int hLength = header.length;
	for (int i=0; i<hLength; i++){
	    out.println(header[i]);
	}
	out.print("# Automatically derived using JavaAPILister ");
	hLength = commandOptions.length;
	for (int i=0; i<hLength; i++){
	    out.print(commandOptions[i]);
	    out.print(' ');
	}
	out.println("\n#");
    }

    private void
    collectClassComponents(
	Vector components,
	ClassInfo thisClass,
	Vector classesAlreadySeen,
	boolean methods,
	boolean superIsFinal,
	boolean leafClass)
    {
	boolean classIsFinal;
	int     permissionMask;
	ClassMemberInfo cma[];
	// if this class has already been seen,
	// don't go looking at it again.
	if (classesAlreadySeen.contains(thisClass)){
	    return;
	}
	classesAlreadySeen.addElement(thisClass);
	// look at all the elements in this class and
	// see if we want to add any of them. See if
	// they have already been added.
	permissionMask = Const.ACC_PUBLIC;
	classIsFinal = superIsFinal || 
	    ((thisClass.access & Const.ACC_FINAL) != 0);
	if (!classIsFinal){
	    permissionMask |= Const.ACC_PROTECTED;
	}
	cma = (methods) ? ((ClassMemberInfo[])thisClass.methods)
			: ((ClassMemberInfo[])thisClass.fields);
	if (cma != null){
	    int nMembers = cma.length;
	    for (int i = 0; i < nMembers; i++){
		ClassMemberInfo cm = cma[i];
		if ((cm.access & permissionMask) == 0)
		    continue;
		if (cm.name.string.equals("<clinit>"))
		    continue;
		if (!leafClass){
		    // this is a superclass or interface.
		    if (cm.isStaticMember()){
			// subclasses don't inherit our statics
			continue;
		    }
		    if (cm.name.string.equals("<init>")){
			// subclasses don't inherit our constructors
			continue;
		    }
		}
		// this looks like a candidate.
		// see if its already on the list.
		ClassMemberInfo otherCm;
		Enumeration componentsE = components.elements();
		boolean doPrint = true;
		while (componentsE.hasMoreElements()){
		    otherCm = (ClassMemberInfo)componentsE.nextElement();
		    if (!cm.name.string.equals(otherCm.name.string)){
			// names differ.
			// keep looking.
			continue;
		    }
		    // names the same
		    if (methods){
			if (!cm.type.string.equals(otherCm.type.string)){
			    // is a method and types are not the same.
			    // methods override on name and type.
			    // otherCm does not override this one.
			    continue;
			}
		    }
		    //
		    // otherCm is a field with the same name
		    // or a method with the same name and sig.
		    // cm has been overridden and should not be
		    // printed again.
		    doPrint = false;
		    break;
		}
		if (doPrint){
		    // we want to include this one on the list.
		    components.add(cm);
		}
	    }
	}
	// look at superclass
	String otherName;
	ClassInfo otherClass;
	ClassConstant superConstant = thisClass.superClass;
	if (superConstant != null){
	    // recurse into superclass
	    otherName = superConstant.name.string;
	    if ((otherClass = ClassTable.lookupClass(otherName,false))
		!= null){
		collectClassComponents(components, otherClass,
		    classesAlreadySeen, methods, classIsFinal, false);
	    }
	}
	// look at interfaces. This should only matter
	// when the current class is an interface or abstract.
	if (thisClass.interfaces != null){
	    int nInterfaces = thisClass.interfaces.length;
	    for (int i=0; i<nInterfaces; i++){
		otherName = thisClass.interfaces[i].name.string;
		if ((otherClass = ClassTable.lookupClass(otherName,false))
		    != null){
		// recurse into interfaces
		collectClassComponents(components, otherClass,
		    classesAlreadySeen, methods, classIsFinal, false);
		}
	    }
	}
    }

    private void
    printVector(String title, Vector v){
	if (v.isEmpty())
	    return;
	memberOut.println(title);
	Enumeration e = v.elements();
	while (e.hasMoreElements()){
	    ClassMemberInfo cm = (ClassMemberInfo)(e.nextElement());
	    memberOut.print('\t');
	    memberOut.print(cm.name.string);
	    memberOut.print(':');
	    memberOut.println(cm.type.string);
	}
    }

    private void
    printClassComponents(ClassInfo thisClass)
    {
	Vector classesVisited;
	Vector components;

	// print fields
	components = new Vector();
	collectClassComponents(components, thisClass, new Vector(), 
			       false, false, true);
	printVector("   FIELDS", components);

	// print methods
	components = new Vector();
	collectClassComponents(components, thisClass, new Vector(), 
			       true, false, true);
	printVector("   METHODS", components);
    }

    private boolean
    includedAndVisible(ClassInfo c){
	String classname = c.className;
	int access = c.access;
	if (!isClassIncluded(classname)){
	    return false;
	}
	if ((access & Const.ACC_PUBLIC) == 0){
	    return false;
	}
	return true;
    }

    private void
    printClassInfo( ClassInfo c ){
	if (!includedAndVisible(c)){
	    if (verbosity > 0){
		System.err.print("Excluded class ");
		System.err.println(c.className);
	    }
	    return;
	}
	memberOut.print("\nCLASS ");
	memberOut.println(c.className);
	printClassComponents(c);
    }

    private void
    printClassName( ClassInfo c ){
	if (!includedAndVisible(c)){
	    return;
	}
	String name = c.className.replace('/','.');
	classOut.println(name);
    }

    private boolean process( String clist[] ) throws Exception {

	commandOptions = clist;
	if (! processOptions( clist )){
	    // malformed command-line argument or file read error
	    return false;
	}

	Enumeration classEnum = classesProcessed.elements();

	/*
	 * first do the member file
	 */
	printFileHeader(memberOut, memberFileHeader);
	while (classEnum.hasMoreElements() ){
	    printClassInfo( (ClassInfo)(classEnum.nextElement()));
	}
	memberOut.flush();

	/*
	 * Now if a classlist output is specified do it too.
	 */
	if (classOut != null){
	    printFileHeader(classOut, classFileHeader);
	    classEnum = classesProcessed.elements();
	    while (classEnum.hasMoreElements() ){
		printClassName( (ClassInfo)(classEnum.nextElement()));
	    }
	    classOut.flush();
	}
	return true;
    }

    public static void main( String clist[] ){
	boolean success;
	try {
	    try {
		success = new JavaAPILister().process( clist );
	    }finally{
		System.out.flush();
		System.err.flush();
	    }
	}catch (Throwable t){
	    success = false;
	    t.printStackTrace();
	}
	if (!success){
	    // process threw error or failed
	    System.exit(1);
	}
	return;
    }
}
