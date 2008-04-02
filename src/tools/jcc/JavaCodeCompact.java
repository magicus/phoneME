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

import components.*;
import vm.*;
import runtime.*;
import util.*;
import jcc.*;

import java.io.PrintStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.File;
import java.util.Enumeration;
import java.util.Vector;
import java.util.Hashtable;

public class JavaCodeCompact extends LinkerUtil {
    int	         verbosity = 0;
    boolean      fail      = false;
    ConstantPool t = new ConstantPool();
    ClassFileFinder  searchPath;
    String	 firstFileName;
    String	 outName;

    Vector	arrayClasses = new Vector();
    boolean	classDebug = false;
    boolean	ROMout	 = true;
    boolean	outSet   = false;
    String	archName = "EVM";
    boolean	archSet  = false;
    boolean 	doShared = false;
    boolean     doNativesTableOnly = true; // we are interested only in building NativesTable.cpp for the VM
    Vector	romAttributes = new Vector();
    ClassReader rdr;
    MemberLoader memberLoader;

    Hashtable	headerDirs = new Hashtable(31);
    String	stubDestName;
    boolean	stubTraceMode = false;
    ClassnameFilterList nativeTypes = new ClassnameFilterList();
    ClassnameFilterList extraHeaders = new ClassnameFilterList();
    int		maxSegmentSize = -1;

    private void
    fileFound( String fname ){
	// currently, the only thing we do with file names
	// is make them into potential output file names.
	if ( firstFileName == null ) firstFileName = fname;
    }

    private void
    makeOutfileName(){
	if ( outName != null ) return; // already done by -o option.
	if (firstFileName==null) firstFileName = "ROMjava.c";
	int sepindex = firstFileName.lastIndexOf( File.separatorChar )+1;
	int suffindex = firstFileName.lastIndexOf( '.' );
	if ( suffindex < 0 ) suffindex = firstFileName.length();
	outName = firstFileName.substring( sepindex, suffindex)
		  + (ROMout ? ".c" : ".mclass");
    }

    private void
    readFile( String fileName, Vector classesProcessed ){

	if ( rdr == null ){
	    rdr = new ClassReader( t, verbosity );
	}
	try {
	    if (fileName.endsWith(".zip") || fileName.endsWith(".jar")){ 
		rdr.readZip( fileName, classesProcessed );
	    } else { 
		rdr.readFile( fileName, classesProcessed );
		fileFound( fileName );
	    }
	} catch ( IOException e ){
	    System.out.println(Localizer.getString("javacodecompact.could_not_read_file", fileName));
	    e.printStackTrace();
	    fail = true;
	}
    }

    private void
    loadMembers( String filename, Vector classlist ){
	if ( memberLoader == null ){
	    if ( rdr == null ){
		rdr = new ClassReader( t, verbosity );
	    }
	    if ( searchPath == null ){
		searchPath = new ClassFileFinder();
	    }
	    memberLoader = new MemberLoader( rdr, searchPath );
	}
	try {
	    memberLoader.readFromFile( filename, classlist );
	} catch ( IOException e ){
	    System.err.println("-memberlist "+filename);
	    e.printStackTrace();
	    fail = true;
	}
    }

    /*
     * Iterate through the shared constant pool, looking for
     * class constants that cannot be resolved. Make a list
     * of their names, as java Strings
     */
    public Vector
    unresolvedClassNames(){
	Vector names  = new Vector();
	Enumeration symbols = t.getEnumeration();
	while ( symbols.hasMoreElements() ){
	    Object o = symbols.nextElement();
	    if ( o instanceof ClassConstant ){
		ClassConstant cc = (ClassConstant)o;
		if ( ! cc.isResolved() ){
		    names.addElement( cc.name.string );
		    cc.forget(); // so 'unresolved' doesn't stick.
		}
	    }
	}
	Enumeration classes = ClassInfo.allClasses();
	while ( classes.hasMoreElements() ){
	    ClassInfo c = (ClassInfo) classes.nextElement();
	    ConstantObject constants[] = c.constants;
	    if ( constants == null ) 
		continue;
	    int nc = constants.length;
	    for ( int i = 0 ; i < nc; i++ ){
		if ( (constants[i] instanceof ClassConstant ) && ! constants[i].isResolved() ){
		    //
		    // so far, entries in the names array are
		    // unique. But because each class has a separate
		    // interfaces array, the current candidate
		    // may already be on it. Avoid adding it 
		    // a second time!
		    // Also, don't put arrays on this list!
		    ClassConstant intrf = (ClassConstant)constants[i];
		    String intname = intrf.name.string;
		    if ( (intname.charAt(0) != Const.SIGC_ARRAY ) && ! names.contains( intname ) )
			names.addElement( intname );
		    intrf.forget(); // so 'unresolved' doesn't stick.
		}
	    }
	}
	return names;
    }

    Vector  classesProcessed = new Vector();
    int     nclasses = 0;
    boolean buildTables = true;
    boolean quicken     = true;
    boolean doClosure   = false;
    boolean qlossless   = false;

    private boolean
    processOptions( String clist[] ){
	boolean success = true;

	for( int i = 0; i < clist.length; i++ ){
	    if ( clist[i].equals(/*NOI18N*/"-c") ){
		doClosure = true;
		continue;
	    } else if ( clist[i].equals(/*NOI18N*/"-t") ){
		buildTables = true;
		continue;
	    } else if ( clist[i].equals(/*NOI18N*/"-q") ){
		quicken     = true;
		continue;
	    } else if ( clist[i].equals(/*NOI18N*/"-nt") ){
		buildTables = false;
		continue;
	    } else if ( clist[i].equals(/*NOI18N*/"-nq") ){
		quicken     = false;
		continue;
	    } else if ( clist[i].equals(/*NOI18N*/"-qlossless") ){
		buildTables = true;
		quicken     = true;
		qlossless   = true;
		continue;
	    } else if ( clist[i].equals(/*NOI18N*/"-g") ){
		classDebug = true;
		ClassInfo.classDebug = true;
		continue;
	    } else if ( clist[i].equals(/*NOI18N*/"-imageAttribute") ){
		romAttributes.addElement( clist[ ++i ] );
		continue;
	    } else if ( clist[i].equals(/*NOI18N*/"-v") ){
		verbosity++;
		continue;
	    } else if ( clist[i].equals(/*NOI18N*/"-r") ){
		ROMout = false;
		continue;
	    } else if ( clist[i].equals(/*NOI18N*/"-o")  ){
		outName =  clist[ ++i ];
	    } else if ( clist[i].equals(/*NOI18N*/"-classpath")  ){
		if ( searchPath == null )
		    searchPath = new ClassFileFinder();
		searchPath.addToSearchPath( clist[ ++i ] );
	    } else if ( clist[i].equals(/*NOI18N*/"-arch") ||
                        clist[i].equals(/*NOI18N*/"-writer") ){
		String archArg = clist[ ++i ];
		archName = archArg; /* .toUpperCase(); */
		if ( archSet ){
		    System.err.println(Localizer.getString("javacodecompact.too_many_-arch_targetarchname_specifiers"));
		    success = false;
		}
		archSet = true;
		continue;
	    } else if (clist[i].equals(/*NOI18N*/"-s")){ 
		doShared = true;
	    } else if ( clist[i].equals("-memberlist") ){
		loadMembers( clist[++i], classesProcessed );
	    } else if ( clist[i].equals("-headersDir") ){
		String type = clist[++i];
		String dir = clist[++i];
		headerDirs.put(type, dir);
		buildTables = true; // got to.
	    } else if ( clist[i].equals("-stubs") ){
		stubDestName = clist[++i];
	    } else if ( clist[i].equals("-trace") ){
		stubTraceMode = true;
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
		    maxSegmentSize = (new Integer(arg)).intValue();
		} catch (NumberFormatException ex) {
		    System.err.println(Localizer.getString("javacodecompact.invalid_max_segment_size"));
		    success = false;
		}
	    } else { 
		readFile( clist[i], classesProcessed );
	    }
	}

	// Default classname filter for natives
	nativeTypes.addTypePatterns( "JDKPack", "-*" );

	return success;
    }

    public boolean process( String clist[] ){

	if ( ! processOptions( clist ) )
	    return false;
	makeOutfileName();
	if ( !ROMout ){
	    quicken = false;
	    buildTables = false;
	}
	    
	    

	if ( doClosure ){
	    while ( !fail ){
		Vector unresolved = unresolvedClassNames();
		if ( unresolved.size() == 0 )
		    break; // none left!
		Enumeration ulist = unresolved.elements();
		int nfound = 0;
		while ( ulist.hasMoreElements() ){
		    String uname = (String)ulist.nextElement();
		    nfound += rdr.readClass(uname, searchPath, classesProcessed);
		}
		if ( nfound == 0 ){
		    // the list now contains things which could
		    // not ever be resolved. Print it out for
		    // information and go on.
		    // this is not actually an error!
		    if ( verbosity > 0 ){
			ulist = unresolved.elements();
			System.out.println(Localizer.getString("javacodecompact.could_not_resolve_these_names"));
			while( ulist.hasMoreElements() ){
			    System.out.write('\t');
			    System.out.println( (String)ulist.nextElement() );
			}
		    }
		    break;
		}
	    }
	}

	if ( fail ){
	    return false;
	}

	if ( memberLoader != null ){
	    // then we saw a -memberlist option.
	    // go through all classes, hacking away
	    // all unmarked, unwanted elements.
	    memberLoader.deleteUnwantedMembers( classesProcessed );
	}

	nclasses = classesProcessed.size();
	ClassInfo c[] = new ClassInfo[ nclasses ];
	classesProcessed.copyInto( c );

	if ( buildTables ){
	    if ( verbosity != 0 )System.out.println(Localizer.getString("javacodecompact.resolving_superclass_hierarchy") );
	    if ( ! ClassInfo.resolveSupers() ){
		return false; // missing superclass is a fatal error.
	    }
	    for ( int i = 0; i < nclasses; i++ ){
		if ( verbosity != 0 )System.out.println(Localizer.getString("javacodecompact.building_tables_for_class", c[i].className));
		c[i].buildReferenceFieldtable( t );
		c[i].buildReferenceMethodtable( t );
	    }
	}
	for ( int i = 0; i < nclasses; i++ ){
	    c[i].findReferences();
	}

	//if ( fail ) System.exit(1);

	// now write the output
	if ( verbosity != 0 )System.out.println(Localizer.getString("javacodecompact.writing_output_file"));

        if (!doNativesTableOnly) {
            writeNativesHeaders( nativeTypes, c, nclasses );
            writeNativesHeaders( extraHeaders, c, nclasses );
        }

	if ( ROMout ){
	    return writeROMFile( outName, c, romAttributes );
	}

	return !fail;

    }

    public static void main( String clist[] ){
	boolean success = new JavaCodeCompact().process( clist );
	if ( !success ){
	    System.out.flush();
	    System.err.flush();
	    System.exit(1);
	}
	return;
    }

    /*
     * ALL THIS IS FOR ROMIZATION
     */

    /*
     * Iterate through all known classes.
     * Iterate through all constant pools.
     * Look at ClassConstants. If they are unbound,
     * and if they are references to array classes,
     * then instantiate the classes and rebind.
     */
    public boolean instantiateArrayClasses( ClassInfo classTable[], boolean verbose ){
	int nclasses = classTable.length;
	boolean good = true;
	// For EVM, make sure that all arrays of basic types
	// are instantiated!
	String basicArray[] = { "[C", "[S", "[Z", "[I", "[J", "[F", "[D", "[B", 
		"[Ljava/lang/Object;" // not strictly basic.
	};
	for ( int ino = 0; ino < basicArray.length; ino++ ){
	    if ( ! collectArrayClass( basicArray[ino], verbose )){
		good = false;
	    }
	}

	// Now dredge through all class constant pools.
	for ( int cno = 0; cno < nclasses; cno++ ){
	    ClassInfo c = classTable[cno];
	    ConstantObject ctable[] = c.constants;
	    if ( ctable == null ) continue;
	    int n = ctable.length;
	    for( int i = 0; i < n; i++ ){
		if ( ctable[i] instanceof ClassConstant ){
		    ClassConstant cc = (ClassConstant)ctable[i];
		    String        cname = cc.name.string;
		    if (cname.charAt(0) != Const.SIGC_ARRAY ){
			continue; // not interesting
		    }
		    if ( cc.isResolved() ){
			continue; // not interesting
		    }
		    if ( ! collectArrayClass( cname, verbose )){
			good = false;
		    }
		    cc.forget(); // forget the fact that we couldn't find it
		}
	    }
	}
	return good;
    }

    private boolean collectArrayClass( String cname, boolean verbose ){
	// cname is the name of an array class
	// make sure it doesn't exist ( it won't if it came from a 
	// class constant ), and instantiate it. For EVM, do the same with
	// any sub-array types.
	boolean good = true;
	do {
	    if ( ClassInfo.lookupClass( cname ) != null ){
		continue; // this one exists. But subclasses may not, so keep going.
	    }
	    try {
		arrayClasses.addElement( new ArrayClassInfo( verbose, cname ) );
	    } catch ( DataFormatException e ){
		e.printStackTrace();
		good = false;
		break; // out of do...while
	    }
	} while ( (cname = cname.substring(1) ).charAt(0) == Const.SIGC_ARRAY );
	return good;
    }

    /*
     * For each array class we just made up, process it like
     * all normal classes would have been processed up to this point.
     */
    public void processArrayClasses( boolean verbose, ConstantPool x ){
	Enumeration arrays = arrayClasses.elements();
	while( arrays.hasMoreElements() ){
	    ArrayClassInfo a = (ArrayClassInfo)arrays.nextElement();
	    if ( verbose ){
		System.out.println(Localizer.getString("javacodecompact.processing_array_class", a.className));
	    }
	    a.externalize(x);
	    //a.countReferences();
	    //a.smashConstantPool();
	}
    }

    /*
     * My first attempt at factoring out VM specific code
     * is to subclass ClassClass. Perhaps I should be subclassing
     * components.ClassInfo itself, I don't know.
     * Anyway, this is the EVM-specific class factory. This
     * should be dependent on a runtime switch, obviously.
     * >>IMPL_NOTE: consider whether it should be fixed<<.
     */
    VMClassFactory classMaker = new EVMClassFactory();

    public ClassClass[] finalizeClasses(){
	ClassClass classes[] = ClassClass.getClassVector(classMaker);
	int n = classes.length;

	// constant pool smashing has to be done after quickening,
	// else it doesn't make much difference!

	for ( int i = 0; i < n; i++ ){
	    ClassInfo c = classes[i].ci;
	    if ( verbosity != 0 )System.out.println(Localizer.getString("javacodecompact.reducing_constant_pool_of_class", c.className));
	    c.countReferences( !ROMout );
	    c.smashConstantPool();
	    c.relocateReferences();
	}

	for ( int i = 0; i < n ; i++ )
	    classes[i].ci.allocateFieldsFromFieldtable();
	
	/*
	 * EVM doesn't do inlining, yet.
	 * This last-minute preparation step might be generalized
	 * to something more useful.
	 */
	/*
	//for ( int i = 0; i < n; i++) 
	//    classes[i].getInlining();
	*/
	return classes;
    }

    private boolean writeROMFile( String outName, ClassInfo classTable[], Vector attributes ){
	boolean good = true;
	ConstantPool sharedConstant = null;

	//
	// did arg parsing.
	// now do work.
	//

	good = instantiateArrayClasses( classTable, verbosity>1 );
	processArrayClasses( verbosity>1, t );
	PrimitiveClassInfo.init( verbosity > 1, t);

	// is better to have this after instantiating Array classes, I think.
	ClassClass classes[] = finalizeClasses();
	int	   totalclasses = classes.length;
	// at this point, the classes array INCLUDES all the array
	// classes. classTable doesn't include these!
	// Since array classes CANNOT participate in sharing
	// (because of magic offsets) they are excluded from the
	// sharing calculation below. And because they don't have
	// any code...

        if (!doNativesTableOnly) {
            if (doShared) {
                // create a shared constant pool
                sharedConstant = new ConstantPool();
                for (int i = 0; i < classTable.length; i++) 
                    addConstant2SharedPool(classTable[i], sharedConstant);

                // sort the reference count
                sharedConstant.doSort();

                // run via the shared constant pool once.
                if (ClassClass.isPartiallyResolved(sharedConstant.getConstants())) {
                    sharedConstant = ClassClass.makeResolvable(sharedConstant);
                }
            } else {
                for (int i = 0; i < totalclasses; i++) {
                    classes[i].adjustSymbolicConstants();
                }
            }

            for (int i = 0; i < totalclasses; i++) {
                classes[i].ci.relocateAndPackCode();
            }
        }

	if ( ! good ) return false;

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
	    } catch ( Exception e ){
		System.err.println(Localizer.getString("javacodecompact.could_not_instantiate", writername));
		e.printStackTrace( );
		return false;
	    }
	}

	w.init(classDebug, nativeTypes, verbosity>0, maxSegmentSize);

	Enumeration attr = attributes.elements();
	while ( attr.hasMoreElements() ){
	    String val = (String)attr.nextElement();
	    if ( ! w.setAttribute( val ) ){
		System.err.println(Localizer.getString("javacodecompact.bad_attribute_value",val));
	    }
	}

	if (  w.open( outName ) != true ) {
	    w.printError( System.out );
	    good = false;
	} else {
	    good = w.writeClasses(t, sharedConstant);
	    w.printSpaceStats( System.out );
	    w.close();
	}
	return good;
    }

    /*
     * For writing header files. We just instantiate
     * a runtime.HeaderDump and let it do all the work for us.
     */
    private void
    writeNativesHeaders( ClassnameFilterList groups, ClassInfo c[], int nclasses ){
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
		} catch ( IOException e ){
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

    // This function update the reference count and put constantobject
    // to the shared constant pool.
    private void addConstant2SharedPool(ClassInfo cinfo, ConstantPool cp) {
        for (int j = 0; j < cinfo.constants.length; j++) {
            if (cinfo.constants[j] == null)
                continue;

            int count = cinfo.constants[j].references;
            if (count > 0) {
               cinfo.constants[j] = cp.appendElement(cinfo.constants[j]);
	    }
        }

        // update interfaces array
        if (cinfo.interfaces != null) {
            for (int k = 0 ; k < cinfo.interfaces.length; k++) {
               cinfo.interfaces[k] = (ClassConstant)
                               cp.appendElement(cinfo.interfaces[k]);
            }
        }

        // update exception table (catchType)
        for (int i = 0; i < cinfo.methods.length; i++) {
            if ( cinfo.methods[i].exceptionTable != null) {
                for (int j = 0; j < cinfo.methods[i].exceptionTable.length; j++) {
                    ClassConstant cc = cinfo.methods[i].exceptionTable[j].catchType;
                    if (cc != null) {
                        cinfo.methods[i].exceptionTable[j].catchType = (ClassConstant)                                                             cp.appendElement(cc);
                    }
                }
            }    
        }
    }

}
