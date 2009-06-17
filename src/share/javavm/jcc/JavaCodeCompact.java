/*
 * @(#)JavaCodeCompact.java	1.96 06/11/07
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

import consts.*;
import components.*;
import vm.*;
import runtime.*;
import util.Assert;
import util.*;
import jcc.*;

import java.io.FileReader;
import java.io.BufferedReader;
import java.io.PrintStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.File;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.Hashtable;
import java.util.Set;
import java.util.Vector;
import components.ClassLoader;

public class JavaCodeCompact extends LinkerUtil {
    int	         verbosity = 0;
    //ConstantPool classNameConstants = new ConstantPool();
    ClassFileFinder  searchPath;
    String	 firstFileName;
    static String   outName;

    /** If true, JCC should include class debug info in generated class data.
     *  This includes line number tables, local var info, etc. */
    boolean	classDebug = false;
    boolean	outSet   = false;
    String	archName = "CVM";
    boolean	archSet  = false;
    /** If true, JCC will attempt to share the same constant pool for all the
     *  classes that it romizes. */
    boolean 	useSharedCP = false;
    boolean	validate = false;
    boolean	unresolvedOk = false;
    Vector	romAttributes = new Vector();
    ClassReader rdr;

    Hashtable	headerDirs = new Hashtable(31);
    String	stubDestName;
    boolean	stubTraceMode = false;
    ClassnameFilterList nativeTypes = new ClassnameFilterList();
    ClassnameFilterList extraHeaders = new ClassnameFilterList();
    int		maxSegmentSize = -1;
    boolean	firstTimeOnly = true;

    /* arguments for JavaAPILister */
    static String      APIListerArgs = null;

    private void
    fileFound(String fname) {
	// currently, the only thing we do with file names
	// is make them into potential output file names.
	if (firstFileName == null) firstFileName = fname;
    }

    private void
    makeOutfileName(){
	if ( outName != null ) return; // already done by -o option.
	if (firstFileName==null) firstFileName = "ROMjava.c";
	int sepindex = firstFileName.lastIndexOf( File.separatorChar )+1;
	int suffindex = firstFileName.lastIndexOf( '.' );
	if ( suffindex < 0 ) suffindex = firstFileName.length();
	outName = firstFileName.substring( sepindex, suffindex) + ".c";
    }

    private boolean
    readFile(String fileName, Vector classesRead) {

	if (rdr == null){
	    rdr = new ClassReader(verbosity);
	}
	try {
	    if (fileName.endsWith(".zip") || fileName.endsWith(".jar")){ 
		rdr.readZip(fileName, classesRead);
	    } else { 
		rdr.readFile(fileName, classesRead);
	    }
	    fileFound(fileName);
	} catch (IOException e) {
	    System.out.println(Localizer.getString(
                "javacodecompact.could_not_read_file", fileName));
	    e.printStackTrace();
	    return false;
	}
	return true;
    }

    /* Return the outName string. */
    public static String getOutName()
    {
        return outName;
    }

    /**
     * Iterate through the classes looking for unresolved
     * class references.
     */
    protected Set
    findUnresolvedClassNames(Enumeration e){
        Set undefinedClassNames = new HashSet();
	ClassInfo cinfo;
	while (e.hasMoreElements()){
	    cinfo = (ClassInfo)e.nextElement();
	    cinfo.findUndefinedClasses(undefinedClassNames);
	}
        return undefinedClassNames;
    }

    /*
     * Find the unresolved class names.
     * Make a list of them, as java Strings.
     */
    public String[]
    unresolvedClassNames(Set undefinedClassNames){
	int nUndefined;
	nUndefined = undefinedClassNames.size();
	if (nUndefined == 0)
	    return null;
	String names[] = new String[nUndefined];
	names = (String[])undefinedClassNames.toArray(names);
	return names;
    }

    Vector  classesProcessed = new Vector();
    int     nclasses = 0;
    /** If true, JCC is not allowed to do lossy quickening. */
    boolean qlossless   = false;
    /** If true, JCC needs to generate JIT friendly output. */
    boolean jitOn       = false;
    /** If true, JCC is not allowed to apply optimizations that can compact
     *  the code.  Example of such compaction includes removing nop bytecodes,
     *  and useless gotos. */
    boolean noCodeCompaction = false;

    /**
     * Processes all the JCC command line arguments into internal flags.  Also
     * checks for the validity of the arguments.
     *
     * NOTE: processOptions() also loads and processes all the classfiles of
     * the initial set to be romized.  Later on, process() will pull in all
     * other classes needed for the full transitive closure of class
     * references from the constant pools of the classes.  processOptions()
     * does not resolve any of the constant pool entries (not even those in
     * the initial set of classes).  The only CP entries that can be resolved
     * here are the UTF8 and value constants that do not require binding to
     * and other classes.
     *
     * @return  false if an error was encountered.
     */
    private boolean
    processOptions(String clist[]) throws Exception {
	boolean success = true;

	for( int i = 0; i < clist.length; i++ ){
	    if ( clist[i].equals(/*NOI18N*/"-jit") ){
		jitOn = true; // Generate JIT friendly output.
		continue;
	    } else if ( clist[i].equals(/*NOI18N*/"-qlossless") ){
		qlossless = true; // Disallow lossy quickening.
		continue;
	    } else if ( clist[i].equals(/*NOI18N*/"-noCodeCompaction") ){
		noCodeCompaction = true; // Disallow code compaction.
		continue;
	    } else if ( clist[i].equals(/*NOI18N*/"-g") ){
		classDebug = true; // Include class debug info.
		ClassInfo.classDebug = true;
		continue;
	    } else if ( clist[i].equals(/*NOI18N*/"-imageAttribute") ){
		romAttributes.addElement( clist[ ++i ] );
		continue;
	    } else if ( clist[i].equals(/*NOI18N*/"-v") ){
		verbosity++;
		continue;
	    } else if ( clist[i].equals(/*NOI18N*/"-o")  ){
		outName =  clist[ ++i ];
	    } else if ( clist[i].equals(/*NOI18N*/"-classpath")  ){
		if ( searchPath == null ) {
		    searchPath = new ClassFileFinder();
		    ClassTable.setSearchPath(searchPath);
		}
		searchPath.addToSearchPath( clist[ ++i ] );
	    } else if ( clist[i].equals(/*NOI18N*/"-arch") ){
		String archArg = clist[ ++i ];
		archName = archArg.toUpperCase();
		if ( archSet ){
		    System.err.println(Localizer.getString(
                        "javacodecompact.too_many_-arch_targetarchname_specifiers"));
		    success = false;
		}
		archSet = true;
		continue;
            } else if (clist[i].equals("-target")){ 
                if (!VMConfig.setJDKVersion(clist[++i])) {
		    System.err.println(Localizer.getString(
                        "javacodecompact.invalid_target_version", clist[i]));
		    return false;
                }
            } else if (clist[i].equals("-sharedCP")){ 
		useSharedCP = true;
	    } else if ( clist[i].equals("-headersDir") ){
		String type = clist[++i];
		String dir = clist[++i];
		headerDirs.put(type, dir);
/* These not supported by CVM
	    } else if ( clist[i].equals("-stubs") ){
		stubDestName = clist[++i];
	    } else if ( clist[i].equals("-trace") ){
		stubTraceMode = true;
*/
	    } else if ( clist[i].equals("-f") ){
		try {
		    success = processOptions( parseOptionFile( clist[++i] ) );
		} catch ( java.io.IOException e ){
		    e.printStackTrace();
		    success = false;
		}
	    } else if ( clist[i].equals("-nativesType") ){
		String name = clist[++i];
		String patterns = clist[++i];
		nativeTypes.addTypePatterns( name, patterns );
	    } else if ( clist[i].equals("-extraHeaders") ){
		String name = clist[++i];
		String patterns = clist[++i];
		extraHeaders.addTypePatterns( name, patterns );
	    } else if ( clist[i].equals("-maxSegmentSize") ){
		String arg = clist[++i];
		try {
		    maxSegmentSize = Integer.parseInt(arg);
		} catch (NumberFormatException ex) {
		    System.err.println(Localizer.getString(
                        "javacodecompact.invalid_max_segment_size"));
		    success = false;
		}
	    } else if ( clist[i].equals("-validate") ){
		// validate data structures before writing output
		validate = true;
            } else if ( clist[i].equals("-excludeFile") ){
                // A file containing a list of methods & fields to exclude
                readExcludeFile(clist[++i]);
            } else if (clist[i].equals("-allowUnresolved")){
		unresolvedOk = true;
            } else if (clist[i].startsWith("-cl:")){
		process(false);
		searchPath = null;
		// user classloader
		String arg = clist[i].substring("-cl:".length());
		String name;
		ClassLoader parent = null;
		int sepIndex = arg.indexOf(':');
		if (sepIndex > 0) {
		    name = arg.substring(0, sepIndex);
		    String parentName =
			arg.substring(sepIndex + 1, arg.length());
		    parent = ClassTable.getClassLoader(parentName);
		    if (parent == null) {
			/* parent classloader not defined */
			System.err.println(Localizer.getString(
                            "javacodecompact.parent_classloader_not_defined", parentName) );
			return false;
		    }
		} else {
		    name = arg;
		    parent = ClassTable.getClassLoader();
		}
		if (ClassTable.getClassLoader(name) != null) {
		    /* classloader name already used */
		    System.err.println(Localizer.getString("javacodecompact.classloader_already_defined", name) );
		    return false;
		}
		ClassLoader l = new components.ClassLoader(name, parent);
		ClassTable.setClassLoader(l);
            } else if (clist[i].startsWith("-listapi:")) {
                APIListerArgs = clist[i];
	    } else {
                Vector classesRead = new Vector();

                /* Read in classes that need to be ROMized. */ 
		classesRead.clear();
		if (!readFile(clist[i], classesRead)) {
		    success = false;
		    // but keep going to process rest of options anyway.
		}
		classesProcessed.addAll(classesRead);
		ClassTable.initIfNeeded(verbosity);
		if (!ClassTable.enterClasses(classesRead.elements())) {
		    success = false;
		}
	    }
	}

	// Default classname filter for natives
	nativeTypes.addTypePatterns( "JNI", "-*" );

	return success;
    }

    private boolean loadClass(ClassLoader cl, String classname,
	Vector oneClass) throws Exception
    {
	ClassLoader parent = cl.getParent();
	if (parent != null) {
	    if (loadClass(parent, classname, oneClass)) {
		return true;
	    }
	}
	ClassFileFinder searchPath = cl.getSearchPath();
	if (searchPath == null) {
	    // If the search path is empty, then there is no place else
	    // to look for the class.  So, fail:
	    return false;
	}

	int nfound = rdr.readClass(classname, searchPath, oneClass);

	if (nfound == 1) {
	    // Add class to the appropriate classloader
	    ClassInfo ci = (ClassInfo)oneClass.elementAt(0);

	    if (!ClassTable.enterClass(ci, cl)) {
		throw new Exception();
	    }
	    return true;
	} else {
	    return false;
	}
    }

    /**
     * Do closure on CP entry class constant references until no unresolved
     * constants remain.
     * NOTE: doClosure() only loads ClassInfos for each class that is
     * referenced from unresolved CP entries.  After doClosure() is done
     * (assuming full transitive closure is desired), then all the CP
     * entries for class constants will reference a ClassInfo object.
     * NOTE: resolving CP entry class constants to ClassInfo does not
     * include quickening of bytecodes and other types of VM specific
     * processing associated with resolution yet.  That part is done
     * later.
     */
    private boolean doClosure() {
	// do closure on references until none remain.
	while (true) {
            Assert.disallowClassloading();
            Set undefinedClassNames =
                findUnresolvedClassNames(classesProcessed.elements());
	    String unresolved[] = unresolvedClassNames(undefinedClassNames);
            Assert.allowClassloading();
            if (unresolved == null)
		break; // none left!
	    int nfound = 0;
	    Vector processedThisTime = new Vector();
	    for( int i=0; i < unresolved.length; i++){
		try {
		    Vector oneClass = new Vector();
		    loadClass(ClassTable.getClassLoader(), unresolved[i],
			oneClass);
		    processedThisTime.addAll(oneClass);
		} catch (Exception e) {
		    return false;
		}
	    }

	    // If we have gone through an iteration when we weren't able to
	    // resolve any more classes, then we have resolved everything
	    // we can.  Hence, break out of here.
	    if (processedThisTime.isEmpty()) {
		break;
	    }

	    // Otherwise, continue to resolve the new classes that we've found.
	    classesProcessed.addAll(processedThisTime);
	}
	return true;
    }

    /**
     * Performs the work for JCC.  This is the root processing method in JCC
     * which in turn will call all other processing methods.
     * NOTE: process() is responsible for resolving all the constant pool
     * entries of the loaded classes if possible.  If full transitive closure
     * is required, then this is the place where the work will be done.
     * 
     * @return false if an error was discovered.
     */
    private boolean process(boolean doWrite) throws Exception {

 	/* Do closure on references until none remain.
         * NOTE: doClosure() only loads ClassInfos for each class that is
         * referenced from unresolved CP entries.  After doClosure() is done
         * (assuming full transitive closure is desired), then all the CP
         * entries for class constants will reference a ClassInfo object.
         * NOTE: resolving CP entry class constants to ClassInfo does not
         * include quickening of bytecodes and other types of VM specific
         * processing associated with resolution yet.  That part is done
         * later.
         */
        if (!doClosure()) {
	    return false;
	}

	ClassInfo c[] = ClassTable.allClasses();
	nclasses = c.length;

	if (verbosity != 0) System.out.println(Localizer.getString(
		"javacodecompact.resolving_superclass_hierarchy") );
	if (! ClassInfo.resolveSupers()){
	    return false; // missing superclass is a fatal error.
	}
	for (int i = 0; i < nclasses; i++){
            ClassInfo cinfo = c[i];
	    if (!(cinfo instanceof PrimitiveClassInfo) &&
		!(cinfo instanceof ArrayClassInfo))
	    {
		if (verbosity != 0) System.out.println(Localizer.getString(
			"javacodecompact.building_tables_for_class",
			cinfo.className));
		cinfo.buildFieldtable();
		cinfo.buildMethodtable();
	    }
	}

        // Warn if fields or methods marked for exclusion were not found
        checkExcludedClassEntries();

	if (!doWrite) {
	    return true;
	}

	// now write the output
	if (verbosity != 0) System.out.println(Localizer.getString(
		"javacodecompact.writing_output_file"));

	writeNativeHeaders( nativeTypes, c, nclasses );
	writeNativeHeaders( extraHeaders, c, nclasses );

	if (stubDestName != null){
	    writeCStubs( c, nclasses );
	}

	boolean good = true;
	if (firstTimeOnly) {
	    // For CVM, make sure that all arrays of basic types
	    // are instantiated!
	    good = instantiateBasicArrayClasses(verbosity > 1);
	    firstTimeOnly = false;
	}

        // prepareClasses() is responsible for quickening bytecodes,
        // optimizing the bytecode, constantpools, etc.  This is where
        // CP resolution as the VM knows it is done.
        if (!prepareClasses(c) || !good) {
	    return false;
	}

	if (doWrite) {
	    makeOutfileName();
	}

	good = writeROMFile( outName, c, romAttributes, doWrite );

        /* Don't destroy class vector. The JavaAPILister
         * needs to access class typeids, which come
         * from CVMClass and CVMClass come from the class
         * vector.
         */
	//ClassClass.destroyClassVector();

	return good;
    }

    public static void main( String clist[] ){
	boolean success = false;
	try {
	    try {
                JavaCodeCompact jcc = new JavaCodeCompact();
		// Parse the command line arguments and check for malformed
                // arguments or a file read error?
                // Note that processOptions also loads all the initial
                // classes to be romized specified at the JCC command line.
                // These classes will be loaded, but their constant pool
                // entries wil not be resolved yet.
		if (jcc.processOptions(clist)) {
                    // If we got here, then the arguments were parsed
                    // successfully.  Now, we are ready to do some
                    // additional work.  This includes resolving the
                    // constant pool entries, quickening bytecodes, etc.
		    success = jcc.process(true);
		}

                /* We are done with processing ROMized classes. Now run 
                 * JavaAPILister, which need to access some of the data
                 * such as class and member typeid that we created during
                 * Romizing classes.
                 */
                if (APIListerArgs != null) {
                    new JavaAPILister().process(APIListerArgs);
                }
	    }finally{
		System.out.flush();
		System.err.flush();
	    }
	}catch (Throwable t){
	    t.printStackTrace();
	}
	if (!success){
	    // process threw error or failed
	    System.exit(1);
	}
	return;
    }

    /*
     * ALL THIS IS FOR ROMIZATION
     */

    public boolean instantiateBasicArrayClasses(boolean verbose)
    {
	boolean good = true;
	// For CVM, make sure that all arrays of basic types
	// are instantiated!
	String basicArray[] = { "[C", "[S", "[Z", "[I", "[J", "[F", "[D", "[B", 
		"[Ljava/lang/Object;" // not strictly basic.
	};
	for ( int ino = 0; ino < basicArray.length; ino++ ){
		if (!ArrayClassInfo.collectArrayClass(basicArray[ino],
			        ClassTable.getClassLoader("boot"), verbose))
		{
		good = false;
	    }
	}
	return good;
    }

    /*
     * Iterate through all known classes.
     * Iterate through all constant pools.
     * Look at ClassConstants. If they are unbound,
     * and if they are references to array classes,
     * then instantiate the classes and rebind.
     */
    public boolean instantiateArrayClasses(
	ClassInfo classTable[],
	boolean verbose)
    {
	int nclasses = classTable.length;
	boolean good = true;
	// Now dredge through all class constant pools.
	for ( int cno = 0; cno < nclasses; cno++ ){
	    ClassInfo c = classTable[cno];
	    ConstantObject ctable[] = c.getConstantPool().getConstants();
	    if ( ctable == null ) continue;
	    int n = ctable.length;
	    for( int i = 1; i < n; i++ ){
		if ( ctable[i] instanceof ClassConstant ){
		    ClassConstant cc = (ClassConstant)ctable[i];
		    String        cname = cc.name.string;
		    if (cname.charAt(0) != Const.SIGC_ARRAY ){
			continue; // not interesting
		    }
		    if (cc.isResolved()){
			continue; // not interesting
		    }
		    if (!vm.ArrayClassInfo.collectArrayClass(cname, c.loader, verbose)) {
			good = false;
		    }
		    cc.forget(); // forget the fact that we couldn't find it
		}
	    }
        
            // We might just want to check the code as well.
            for (int j = 0; j < c.methods.length; j++) {
                MethodInfo m = c.methods[j];
                m.collectArrayForAnewarray(ctable, c.className);
	    }
	}
	return good;
    }

    /*
     * We attempt to factor out VM specific code
     * by subclassing ClassClass. Perhaps we should be subclassing
     * components.ClassInfo itself.
     * Anyway, this is the CVM-specific class factory. This
     * would better be dependent on a runtime switch.
     */
    VMClassFactory classMaker = new CVMClassFactory();

    public ClassClass[]
    finalizeClasses() throws Exception{
	ClassClass classes[] = ClassClass.getClassVector(classMaker);
        int numberOfClasses = classes.length;

	CodeHacker ch = new CodeHacker( qlossless, jitOn, verbosity >= 2 );
	for (int i = 0; i < numberOfClasses; i++) {
	    if (verbosity != 0) {
                System.out.println(Localizer.getString(
		    "javacodecompact.quickening_code_of_class",
                    classes[i].classInfo.className));
            }
            if (!ch.quickenAllMethodsInClass(classes[i].classInfo)) {
                throw new Exception(Localizer.getString(
		    "javacodecompact.quickening_code_of_class",
                    classes[i].classInfo.className));
	    }
	}

	// constant pool smashing has to be done after quickening,
	// else it doesn't make much difference!

	for (int i = 0; i < numberOfClasses; i++) {
	    ClassInfo cinfo = classes[i].classInfo;
	    if (verbosity != 0) {
                System.out.println(Localizer.getString(
                    "javacodecompact.reducing_constant_pool_of_class",
                    cinfo.className));
            }
	    cinfo.countReferences(false);
	    cinfo.smashConstantPool();
	    cinfo.relocateReferences();
	}

	/*
	 * This last-minute preparation step might be generalized
	 * to something more useful.
	 */
	if (!noCodeCompaction && !qlossless) {
            for (int i = 0; i < numberOfClasses; i++) {
                classes[i].getInlining();
            }
	}
	return classes;
    }

    private void
    validateClasses(ClassClass classes[], Vector sharedConstantPools){
	int totalclasses = classes.length;
	for (int i = 0; i < totalclasses; i++){
	    ClassInfo ci = classes[i].classInfo;
	    ci.validate(null);
	}
    }

    Vector sharedConstantPools = null;

    private boolean
    prepareClasses(ClassInfo classTable[]) throws Exception
    {
	UnresolvedReferenceList missingObjects = new UnresolvedReferenceList();
	boolean anyMissingConstants = false;

	boolean good = instantiateArrayClasses( classTable, verbosity>1 );
	// is better to have this after instantiating Array classes, I think.
	ClassClass classes[] = finalizeClasses();
	int	   totalclasses = classes.length;
	// at this point, the classes array INCLUDES all the array
	// classes. classTable doesn't include these!
	// Since array classes CANNOT participate in sharing
	// (because of magic offsets) they are excluded from the
	// sharing calculation below. And because they don't have
	// any code...

        ConstantPool sharedConstant;
        int i;

	if (useSharedCP) {
            int idx = 0;
	    // create a shared constant pool
            sharedConstantPools = new Vector();
	    sharedConstant = new ConstantPool();
            sharedConstantPools.add(sharedConstant);
	    for (i = 0; i < classTable.length; i++) {
                boolean needSplit = false;
		needSplit = mergeConstantsIntoSharedPool(
                                classTable[i], sharedConstant, idx);
                if (needSplit) {
                    idx ++;
                    sharedConstant = new ConstantPool();
                    sharedConstantPools.add(sharedConstant);
                    mergeConstantsIntoSharedPool(
                                classTable[i], sharedConstant, idx);
                }
            }
	   
	    // sort the reference count
            for (i = 0; i < sharedConstantPools.size(); i++) {
                sharedConstant = (ConstantPool)sharedConstantPools.get(i);
	        sharedConstant.doSort();

	        // run via the shared constant pool once.
	        if (ClassClass.isPartiallyResolved(sharedConstant)) {
		    sharedConstant = classMaker.makeResolvable(
		        sharedConstant, missingObjects, "shared constant pool");
	        }
                sharedConstantPools.set(i, sharedConstant);
            }
	} else {
	    for (i = 0; i < totalclasses; i++){
		if (! classes[i].adjustSymbolicConstants(missingObjects))
		    anyMissingConstants = true;
	    }
	    if ( anyMissingConstants == true ){
		System.err.println(Localizer.getString(
                    "javacodecompact.classes_referred_to_missing_classes"));
		for (i = 0; i < totalclasses; i++){
		    if ( classes[i].impureConstants ){
			System.err.println("	" +
                                           classes[i].classInfo.className);
		    }
		}
	    }
	}
	if (missingObjects.hasUnresolvedReferences()){
	    if (verbosity > 0 || !unresolvedOk) {
		missingObjects.print(System.err);
	    }
	    if (!unresolvedOk){
		System.err.println(Localizer.getString(
		    "javacodecompact.unresolved_references_not_allowed"));
		return false;
	    }
	}

	for (i = 0; i < totalclasses; i++) {
            classes[i].classInfo.relocateAndPackCode(noCodeCompaction);
	    if (useSharedCP) {
		classes[i].classInfo.setConstantPool(sharedConstantPools);
	    }
	}

	if ( ! good ) return false;
	if ( validate ){
	    if (useSharedCP){
                for (i = 0; i < sharedConstantPools.size(); i++) {
                    sharedConstant = (ConstantPool)sharedConstantPools.get(i);
		    sharedConstant.validate();
                }
	    }
	    validateClasses(classes, sharedConstantPools);
	}
	return true;
    }

    private boolean
    writeROMFile(
	String outName,
	ClassInfo classTable[],
	Vector attributes,
	boolean doWrite) throws Exception
    {
	CoreImageWriter w;

	{
	    String writername = "runtime."+archName+"Writer";
	    Class writerClass = null;
	    try {
		writerClass = Class.forName( writername );
	    } catch ( ClassNotFoundException ee ){
		System.err.println(Localizer.getString("javacodecompact.not_supported", archName));
		return false;
	    }
	    try {
		w = (CoreImageWriter)(writerClass.newInstance());
	    } catch (Exception e){
		System.err.println(Localizer.getString("javacodecompact.could_not_instantiate", writername));
		e.printStackTrace();
		return false;
	    }
	}

	//
	// NOTE: mb's marked always mutable for now, since the stackmaps
	// need to be written into the mb. It's possible to fix
	// if there is a stackmaps cache, and the mb->stackmapsX field
	// is removed.
	//
	w.init(classDebug, qlossless, nativeTypes, verbosity>0, 
	       maxSegmentSize, true);

	Enumeration attr = attributes.elements();
	while ( attr.hasMoreElements() ){
	    String val = (String)attr.nextElement();
	    if (! w.setAttribute(val)){
		System.err.println(Localizer.getString(
			    "javacodecompact.bad_attribute_value",val));
		return false;
	    }
	}

	if (doWrite && ! w.open(outName)){
	    w.printError(System.out);
	    return false;
	} else {
	    boolean good = w.writeClasses(sharedConstantPools, doWrite);
	    w.printSpaceStats(System.out);
	    if (doWrite) {
		w.close();
	    }
	    return good;
	}
    }

    /*
     * For writing header files. We just instantiate
     * a runtime.HeaderDump and let it do all the work for us.
     */
    private void
    writeNativeHeaders( ClassnameFilterList groups, ClassInfo c[], int nclasses ){
	Hashtable dumpers = new Hashtable(7);

	for ( int i = 0; i < nclasses; i++ ){
	    ClassInfo ci = c[i];
	    String classname = ci.className;

	    String[] types =  groups.getTypes( classname );
	    for ( int j = 0; j < types.length; ++j) {
		String type = types[j];
		HeaderDump hd = type != null ?
			(HeaderDump)dumpers.get(type) : null;
		if (hd == null) {
		    try {
			Class dumperClass =
			    Class.forName("runtime." + type + "Header");
			hd = (HeaderDump)dumperClass.newInstance();
			dumpers.put(type, hd);
		    } catch (Exception e) {
			e.printStackTrace();
			continue;
		    }
		}
		String classFilename = hd.filename( classname );
		String destFilename = classFilename+".h";

		String nativesHeaderDestDir = (String)headerDirs.get(type);
		File nativesDestFile = new File(nativesHeaderDestDir,
						destFilename);
		File nativesDumpFile;

		boolean didWorkForNatives;

		if ( nativesDestFile.exists() ){
		    nativesDumpFile =
			new File( nativesHeaderDestDir, classFilename+".TMP" );
		} else {
		    nativesDumpFile = nativesDestFile;
		}

		try {
		    PrintStream o = new BufferedPrintStream( new FileOutputStream( nativesDumpFile ) );
		    didWorkForNatives = hd.dumpHeader( ci, o );
		    o.close();
		} catch (IOException e){
		    e.printStackTrace();
		    continue;
		}

		if ( didWorkForNatives ){
		    if ( nativesDestFile != nativesDumpFile ){
			// copy and delete
			FileCompare.conditionalCopy( nativesDumpFile,
						     nativesDestFile );
			nativesDumpFile.delete();
		    }
		} else {
		    nativesDumpFile.delete();
		}
	    }
	}
    }

    /*
     * For writing a C stub file. We just instantiate
     * a runtime.CStubGenerator and let it do all the work for us.
     */
    private void
    writeCStubs( ClassInfo c[], int nclasses ){
	// (conditional file creation copied from above)
	File destFile = new File( stubDestName );
	File dumpFile;

	if ( destFile.exists() ){
	    dumpFile = new File( stubDestName+".TMP" );
	} else {
	    dumpFile = destFile;
	}
	try {
	    PrintStream o = new BufferedPrintStream( new FileOutputStream( dumpFile ) );
	    CStubGenerator cs = new CStubGenerator( stubTraceMode, o );
	    cs.writeStubs( c, nclasses, nativeTypes );
	    o.close();
	} catch ( IOException e ){
	    e.printStackTrace();
	    return;
	}
	if ( destFile != dumpFile ){
	    // copy and delete
	    FileCompare.conditionalCopy( dumpFile, destFile );
	    dumpFile.delete();
	}
    }

    // This function updates the reference count and puts all constants
    // associated with a ClassInfo to the shared constant pool.
    private boolean mergeConstantsIntoSharedPool(ClassInfo cinfo, 
					         ConstantPool sharedCP,
                                                 int sharedCPIdx) {
        ConstantPool cp = cinfo.getConstantPool();

        if (sharedCP.getLength() + cp.getLength() > 0xffff) {
            return true;
        }

        cinfo.setSharedCPIdx(sharedCPIdx);

	// Get the CP that is to be added to the shared CP:
	ConstantObject[] constants = cinfo.getConstantPool().getConstants();

	// For each CP entry that is referenced, add it to the shared CP:
        for (int j = 0; j < constants.length; j++) {
	    ConstantObject thisConst = constants[j];
            if (thisConst == null)
                continue;

            int count = thisConst.getReferences();
	    if (count > 0) {
		// Add into sharedCP, and replace the original in the cinfo:
		constants[j] = sharedCP.add(thisConst);
	    }
        }

	// For each constant in the interface array, add it to the shared CP
	// and update the interface array with the new constant values:
        if (cinfo.interfaces != null) {
            for (int k = 0 ; k < cinfo.interfaces.length; k++) {
               cinfo.interfaces[k] = (ClassConstant)
                   sharedCP.add(cinfo.interfaces[k]);
            }
        }

        // update exception table (catchType)
	// and also thrown exceptions table
        for (int i = 0; i < cinfo.methods.length; i++) {
	    MethodInfo mi = cinfo.methods[i];
	    
            // NOTE: Investigate the possibility of removing this scan
            // of the exception tables.  Since references to exception
            // classes from the exception tables are already counted in
            // the individual class' constant pool, adding the constants
            // from the class' constant pool should be adequate.  We
            // might not need to scan these exception tables anymore.
            if ( mi.exceptionTable != null) {
                for (int j = 0; j < mi.exceptionTable.length; j++) {
                    ClassConstant cc = mi.exceptionTable[j].catchType;
                    if (cc != null) {
                        mi.exceptionTable[j].catchType = 
			    (ClassConstant)sharedCP.add(cc);
                    }
                }
            }
	    if (mi.exceptionsThrown != null) {
                for (int j = 0; j < mi.exceptionsThrown.length; j++) {
                    ClassConstant cc = mi.exceptionsThrown[j];
		    mi.exceptionsThrown[j] = 
			(ClassConstant) sharedCP.add(cc);
                }
	    }
        }
	if (cinfo.innerClassAttr != null) {
	    cinfo.innerClassAttr.mergeConstantsIntoSharedPool(sharedCP);
	}

	/* TODO :: BEGIN experimental code for future signature support.
	if (cinfo.signatureAttr != null) {
	    cinfo.signatureAttr.mergeConstantsIntoSharedPool(sharedCP);
	}
	// TODO :: END */

        /* Just to make sure we are not exceeding the limit */
        if (sharedCP.getLength() > 0xffff) {
	    throw new Error("Constant pool overflow: 64k constants"+
		            " allowed, have " + sharedCP.getLength());
        }

        return false; /* not need to split the shared constnat pool */
    }

    /*
     * Read the exclude file, if any, to determine fields and
     * methods to exclude. File format is one entry per line
     * (lines beginning with '#' are ignored), a keyword of
     * "METHOD" or "FIELD" followed by the signature:
     * METHOD classname.methodname(type)
     * FIELD classname.fieldname
     * Only record the lists of fields and methods if we read the
     * file without incident.
     */
    Vector fieldVector = new Vector();
    Vector methodVector = new Vector();
    private void readExcludeFile( String fileName )
    {
        BufferedReader r;
        try {
            r = new BufferedReader(new FileReader(fileName));
            String line, sig, clazz, method, field, type;
            int dotIndex, typeIndex, closeIndex;
            while((line = r.readLine()) != null) {
                if (line.length() == 0 || line.startsWith("#")) {
                    continue;
                } else if (line.startsWith("METHOD ")) {
                    sig = line.substring("METHOD ".length());
                    dotIndex = sig.indexOf('.');
                    typeIndex = sig.indexOf('(', dotIndex);
                    closeIndex = sig.indexOf(')',typeIndex);
                    if (dotIndex > 0 &&
                        typeIndex > dotIndex &&
                        closeIndex > typeIndex) {
                        clazz = sig.substring(0, dotIndex);
                        method = sig.substring(dotIndex+1, typeIndex);
                        type = sig.substring(typeIndex, closeIndex+1);
                        methodVector.add(
                            new MemberNameTriple(clazz,
                                                 method,
                                                 type));
                    } else {
                        System.err.println(Localizer.getString("javacodecompact.exclude_parse_error",line));
                    }
                } else if (line.startsWith("FIELD ")) {
                    sig = line.substring("FIELD ".length());
                    dotIndex = sig.indexOf('.');
                    if (dotIndex > 0 ) {
                        clazz = sig.substring(0, dotIndex);
                        field = sig.substring(dotIndex+1);
                        fieldVector.add(
                           new MemberNameTriple(clazz,
                                                field,
                                                null));
                    } else {
                        System.err.println(Localizer.getString("javacodecompact.exclude_parse_error",line));
                    }
                }
            }
            r.close();
            // We may be repetitively handing the same vector
            // to ClassInfo if multiple exclude files are
            // specified, but this shouldn't hurt.
            ClassInfo.setExcludeLists( methodVector, fieldVector );
        } catch ( IOException ioe ) {
            System.err.println(Localizer.getString("javacodecompact.bad_exclude_file",fileName));
        }
        return;
    }

    // Ensure that we don't have any fields or methods marked for
    // exclusion which weren't found.
    private void checkExcludedClassEntries()
    {
        int i;
        for (i = 0 ; i < methodVector.size(); i++) {
            System.err.println(Localizer.getString("javacodecompact.excluded_method_not_found",((MemberNameTriple)methodVector.get(i)).toString()));
        }
        for (i = 0 ; i < fieldVector.size(); i++) {
            System.err.println(Localizer.getString("javacodecompact.excluded_field_not_found",((MemberNameTriple)fieldVector.get(i)).toString()));
        }
        return;
    }
}
