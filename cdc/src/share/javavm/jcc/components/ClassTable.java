/*
 * @(#)ClassTable.java	1.9 06/11/10
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
import jcc.Util;
import jcc.Str2ID;
import consts.Const;
import util.*;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Set;
import java.util.Vector;

/*
 * The symbol table for classes.
 *
 * At any time there is only one classloader for the classes being 
 * processed.  The current classloader is maintained by the
 * ClassTable class. The only way to change the classloader is by calling
 * ClassTable.setClassLoader(), which happens when '-cl' option is seen.
 *
 * Example: btclasses.zip -cl:sys testclasses.zip
 *
 * The initial classloader is the boot classloader.  The
 * default parent for a new classloader is the previous classloader.
 * Classes are searched according to the classloader hierarchy.
 * Unresolved classes will be searched for in the search path specified
 * by -classpath.  Any classes found will be belong to the classloader
 * that was in effect when the search path was specified.
 *
 * Example:
 * btclasses.zip -classpath btdep.zip -cl:sys app.jar -classpath appdep.zip
 */

public
class ClassTable
{

    /**
     * We keep track of classes by hashing them by name when
     * we read them. They can be looked up using lookupClass,
     * which will take a classname string as parameter.
     */
    static Vector allClasses = new Vector();
    static ClassLoader bootLoader = new ClassLoader("boot", null);
    static ClassLoader loader = bootLoader;
    static ClassLoader allLoader = new ClassLoader("all", null);
    static Hashtable loaders = new Hashtable();
    static Hashtable classTable = loader.classes;
    static String classLoaderNames = "";
    static int numClassLoaders = 0;

    static {
	loaders.put("boot", bootLoader);		// ID 0
        loaders.put("all", allLoader);                  // ID 1
	allClasses = new Vector();
    }

    private static boolean primitivesDone = false;

    /**
     * Initializes the class table if it hasn't already been initialized.
     */
    public static void initIfNeeded(int verbosity) {
	if (!primitivesDone) {
	    vm.PrimitiveClassInfo.init(verbosity > 1);
	    primitivesDone = true;
	}
    }

    // Set search path for current classloader
    public static void
    setSearchPath(ClassFileFinder searchPath) {
	loader.setSearchPath(searchPath);
    }

    public static void
    setClassLoader(ClassLoader l) {
	loader = l;
	classTable = loader.classes;
	loaders.put(l.name, l);
	if (classLoaderNames.length() == 0) {
	    classLoaderNames = l.name;
	} else {
	    classLoaderNames = classLoaderNames + "," + l.name;
	}
	++numClassLoaders;
    }

    public static int
    getNumClassLoaders() {
	return numClassLoaders;
    }

    public static String
    getClassLoaderNames() {
	return classLoaderNames;
    }

    public static ClassLoader
    getClassLoader() {
	return loader;
    }

    public static ClassLoader
    getClassLoader(String name) {
	return (ClassLoader)loaders.get(name);
    }

    public static boolean
    enterClass(ClassInfo cinfo, ClassLoader loader) {
	loader.enterClass(cinfo);
	String className = cinfo.className;
	// Make sure a classvector hasn't been created yet.
	// (used to add, now we just assert that it isn't necessary).
	if (vm.ClassClass.hasClassVector()){
	    System.err.println(Localizer.getString(
                "classtable.class_vector_in_place", className));
	    return false;
	}
	allClasses.add(cinfo);
	return true;
    }

    public static boolean
    enterClass(ClassInfo cinfo){
	if (!(cinfo instanceof vm.ArrayClassInfo)) {
	    return enterClass(cinfo, loader);
	} else {
	    return enterClass(cinfo, cinfo.loader);
	}
    }

    private static Hashtable sigToPrimitive = new Hashtable();

    public static boolean
    enterPrimitiveClass(vm.PrimitiveClassInfo pci) {
	boolean success = enterClass(pci, bootLoader);
	if (success) {
	    sigToPrimitive.put(new Character(pci.signature), pci);
	}
	return success;
    }

    /* Enter classes associated with a specific classloader.
     * The classes are not added to 'allClass' like the 
     * other enterClasses() methods do. Also note, this
     * is not intended for array classes.
     */
    public static boolean
    enterClasses(Enumeration e, ClassLoader l){
	while(e.hasMoreElements()){
	    ClassInfo c = (ClassInfo)(e.nextElement());
            l.enterClass(c);
	}
	return true;
    }

    public static boolean
    enterClasses(Enumeration e){
	// This is one place where we could restrict user
	// classloaders from defining java.*, sun.*, etc.
	// classes
	while(e.hasMoreElements()){
	    ClassInfo c = (ClassInfo)(e.nextElement());
	    if (!enterClass(c)) {
		return false;
	    }
	}
	return true;
    }

    public static vm.PrimitiveClassInfo
    lookupPrimitiveClass(char sig){
	return (vm.PrimitiveClassInfo)sigToPrimitive.get(new Character(sig));
    }

    public static ClassInfo
    lookupClass(String key){
	return loader.lookupClass(key);
    }

    public static ClassInfo
    lookupClass(String key, ClassLoader l){
	return l.lookupClass(key);
    }

    public static int size(){
	return allClasses.size();
    }

    public static Enumeration elements(){
	return allClasses.elements();
    }

    public static ClassInfo[]
    allClasses()
    {
	Enumeration classEnum = elements();
	int nclasses = size();
	ClassInfo ary[] = new ClassInfo[ nclasses ];
	for (int i= 0; i < nclasses; i++){
	    ary[i] = (ClassInfo) classEnum.nextElement();
	}
	return ary;
    }

}
