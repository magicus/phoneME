/*
 * @(#)JavaMemberDepend.java	1.12 06/10/10
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

import util.*;
import util.Set;
import java.util.Enumeration;
import java.util.Vector;
import dependenceAnalyzer.*;
import java.io.PrintStream;
import java.io.FileOutputStream;
import components.*;

class JavaMemberDepend extends DepgenUtil{

    boolean hadError = false;
    boolean details = false;
    boolean classlistonly = false;
    int     verbosity= 0;

    /*
     * Java linking.
     * The member linker determines which class members
     * are required to satisfy the references of the
     * members named, taking into account the problems
     * introduced by method overriding and references from
     * unanalysed sources, as directed by command line options.
     *
     * The member linker works off a list of things that
     * must be included, iteratively analysing the members
     * on this list, and building a new list of required
     * members.
     *
     * This operation is controlled by a number of command-line
     * options:
     * -classpath and -exclude, as decribed below.
     *
     * -load full-member-name
     *     to put the named member on the work list.
     * -loadClass full-classname
     *     to put all members of the named class on the
     *     work list. This is especially useful for classes
     *     that will be used by dynamically-loaded code
     *     in ways that cannot be analysed at this time.
     * -interface full-classname
     *     to indicate that the named class, whether a
     *     Java class or a Java interface, defines an
     *     interface contract which must be met by its
     *     subclasses, even if the named class, itself
     *     is excluded from the load. This is especially
     *     useful when packaging up something, such as
     *     a subclass of java.lang.Applet, for dynamic
     *     loading, and it must meet the named interface
     *     contract.
     */

    /*
     * These are the flags that control member loading:
     * REQ -- the marked element is required to run. Either
     *        it was specified with -load or -loadClass, or
     *        was determined to be necessary for something
     *        that was.
     * INITREQ -- constructor required. Marks a class if
     *        at least one of its constructors is marked REQ.
     * INTF -- interface. Marks a member of a class specified
     *	      with -interface.
     * LISTED -- just a tag for members listed by the user
     *        as needing loading. Either as individuals or
     *        as a member of a listed class.
     */

    static final int REQ    =(1<<29);
    static final int INITREQ=(1<<28);
    static final int INTF   =(1<<27);
    static final int ANY    =0xffffffff;

    static final int LISTED =(1<<26);

    Vector initialList = new Vector();
    Vector loaded;

    PrintStream nativelist = null;

    private void loadNode( DependenceNode node ){
	node.flags |= LISTED;
	initialList.addElement( node );
    }

    /*
     * loadMember -- triggered by the command-line option
     * -load full-member-name
     */
    private void loadMember( String membername ){
	MemberDependenceNode node = stringToNode( membername );
	if ( node == null ){
	    System.out.println(
		Localizer.getString("javafilter.cannot_find_member",
		membername));
	    hadError = true;
	    return;
	}
	loadNode( node );
	loadNode( ((MemberName)(node.name())).classEntry );
    }
    /*
     * loadConstructor -- triggered by the command-line option
     * -new full-class-name or 
     * -new full-class-name(signature)V
     */
    private void loadConstructor( String constructorname ){
	MemberDependenceNode node = stringToConstructorNode( constructorname );
	if ( node == null ){
	    System.out.println(
		Localizer.getString("javafilter.cannot_find_constructor",
		constructorname)
	    );
	    hadError = true;
	    return;
	}
	loadNode( node );
	loadNode( ((MemberName)(node.name())).classEntry );
    }
    /*
     * loadClass -- triggered by the command-line option
     * -loadClass full-classname
     */
    private void loadClass( String classname ){
	ClassEntry c = stringToClass( classname );
	if ( c == null ){
	    System.out.println(
		Localizer.getString("javafilter.cannot_find_class",
		classname)
	    );
	    hadError = true;
	    return;
	}
	loadNode( c );
	allClassMembers( classname );
    }

    /*
     * interfaceClass -- triggered by the command-line option
     * -interface full-classname
     */
    private boolean interfaceClass( String classname ){
	if ( ! this.filter.accept( null, sanitizeClassname( classname ) ) ){
	    // oops. Not excluded.
	    System.err.println(
		Localizer.getString("javafilter.class_interface_but_not_exclude"
		,classname)
	    );
	    loadClass( classname );
	    return true;
	}
	ClassEntry c = stringToClass( classname );
	if ( c == null ){
	    System.out.println(
		Localizer.getString("javafilter.cannot_find_class",
		classname)
	    );
	    hadError = true;
	    return false;
	}
	if ( ! analyzer.makeInterface( c ) ){ // big magic here!
	    System.err.println(
		Localizer.getString("javafilter.could_not_make_interface",
		classname));
	    hadError = true;
	    return false;// something bad happened here.
	}

	Enumeration memberlist = c.members();
	if ( memberlist == null ) return true; // no work.
	while ( memberlist.hasMoreElements() ){
	     DependenceNode node = (DependenceNode)( memberlist.nextElement() );   
	     node.flags |= INTF;
	}
	return true;
    }

    /*
     * -wholeClass classname 
     * -wholeClass packagename.*
     * also implicit in -loadClass
     */
    private void allClassMembers( String name ){
	if ( wholeClass == null )
	    wholeClass = new ClassnameFilter();
	wholeClass.includeName( sanitizeClassname( name ) );
    }

    /*
     * -wholeData classname
     * -wholeData packagename.*
     */
    private void allClassData( String name ){
	if ( wholeData == null )
	    wholeData = new ClassnameFilter();
	wholeData.includeName( sanitizeClassname( name ) );
    }

    class DependenceRelation{
	String fromName;
	String verb;
	String toName;
    
	DependenceRelation( String f, String v, String t ){
	    fromName = f;
	    verb     = v;
	    toName   = t;
	}
    }

    /*
     * addedDepencence -- called in response to a command-line
     * option such as
     *	-dependence full-member-name verb full-member-name
     */
    private void addDependence( DependenceRelation r ){
	DependenceNode fromNode = stringToNode( r.fromName );
	MemberDependenceNode destMember = null;
	DependenceNode       destClass = null;
	boolean toClass = false;
	int     arcType;
	if ( r.verb.equalsIgnoreCase("reads") ){
	    toClass = false;
	    arcType = ARC_READS;
	} else if ( r.verb.equalsIgnoreCase("writes") ){
	    toClass = false;
	    arcType = ARC_WRITES;
	} else if ( r.verb.equalsIgnoreCase("calls") ){
	    toClass = false;
	    arcType = ARC_CALLS;
	} else if ( r.verb.equalsIgnoreCase("classref") ){
	    toClass = true;
	    arcType = ARC_CLASS;
	} else {
	    System.out.println(
		Localizer.getString("javafilter.unrecognized_dependence_verb",
		r.verb));
	    hadError = true;
	    return;
	}
	try {
	    if ( toClass ){
		destClass = stringToClass( r.toName );
	    } else {
		destMember = stringToNode( r.toName );
		destClass = ((MemberName)(destMember.name())).classEntry;
		analyzer.addDependenceArc( fromNode, destMember, arcType );
	    }
	    analyzer.addDependenceArc( fromNode, destClass, ARC_CLASS );
	} catch ( NullPointerException e ){
	    /*
	     * This is extra-cautious. I don't believe any of these
	     * situations can occur using the current DependenceAnalyzers.
	     * I believe that, barring catastrophies such as running out of memory,
	     * data structures will be created on-the-fly by the stringToXXX
	     * methods.
	     */
	    if ( fromNode == null ){
		System.out.println(
		    Localizer.getString("javafilter.cannot_find",
		    r.fromName));
		hadError = true;
		return;
	    }
	    if ( !toClass ){
		if ( destMember == null ){
		    System.out.println(
			Localizer.getString("javafilter.cannot_find",
			r.toName));
		    hadError = true;
		    return;
		}
	    }
	    if ( destClass == null ){
		System.out.println(
		    Localizer.getString("javafilter.cannot_find",
		    r.toName));
		hadError = true;
		return;
	    }
	    // else, none of the above
	    // propigate the error, as it is not one we want to deal with.
	    throw e;
	}
    }

    /*
     * doLoad(). Called at the end of argument processing.
     */
    private void doLoad(){
	// first, work off the lists we collected
	// during argument processing.
	// all the -classpath and -exclude stuff
	// has been taken care of.
	// add interfaces and dependences first.
	// then things to load
	Enumeration work = interfaceClasses.elements();
	while ( work.hasMoreElements() ){
	    interfaceClass( (String) ( work.nextElement() ) );
	}
	work = dependences.elements();
	while ( work.hasMoreElements() ){
	    addDependence( (DependenceRelation)( work.nextElement() ) );
	}
	work = membersToLoad.elements();
	while ( work.hasMoreElements() ){
	    loadMember( (String)( work.nextElement() ) );
	}
	work = constructorsToLoad.elements();
	while ( work.hasMoreElements() ){
	    loadConstructor( (String)( work.nextElement() ) );
	}
	work = classesToLoad.elements();
	while ( work.hasMoreElements() ){
	    loadClass( (String)( work.nextElement() ) );
	}
	work = paths.elements();
	while ( work.hasMoreElements() ){
	    PathPair pp  = (PathPair)(work.nextElement());
	    loadMember( (String) pp.src  );
	    loadMember( (String) pp.dest  );
	}
	work = showClasses.elements();
	while ( work.hasMoreElements() ){
            loadClass( (String)( work.nextElement() ) );
        }
	if ( hadError ) return; // oops.
	if ( initialList.size() == 0) return; // vacuous!
	if ( verbosity > 0 ){
	    System.err.println(
		Localizer.getString("javafilter.loading_based_on_these_requirements"));
	    printList( System.err, initialList );
	}
	if ( wholeClass == null )
	    wholeClass = new NeverAccept();
	if ( wholeData == null )
	    wholeData = new NeverAccept();
	loaded = processList( initialList, REQ, INITREQ, REQ|INTF, verbosity );
	if ( loaded == null ) {
	    System.err.println(Localizer.getString("javafilter.nothing_to_load"));
	} else if (verbosity > 0 ){
	    System.out.println("These members loaded:");
	    printList( System.out, loaded );
	}
    }
	
    static void printDependences( DependenceNode node ){
        if ( node.state() != DependenceNode.ANALYZED ){
            if (node.state() == DependenceNode.ERROR)
                System.out.println( node.name()+" unanalysed");
            return;
        }
        System.out.println( node.name()+" depends on:");
        Enumeration e = node.dependsOn();
        while ( e.hasMoreElements() ){
            DependenceArc a = (DependenceArc)(e.nextElement());
	    Object mn = a.to().name();
            String dep = (String) mn.toString();
/*
            if (dep.startsWith("java/io") ||
                dep.startsWith("java/lang") ||
                dep.startsWith("java/net") ||
                dep.startsWith("java/util") )
                continue;
*/
            System.out.println("        "+dep );
        }
     }

    static void printClassList( DependenceNode node ){
        if ( node.state() != DependenceNode.ANALYZED ){
            // if (node.state() == DependenceNode.ERROR)
            // System.out.println( node.name()+" unanalysed");
            return;
        }

	String str = node.name().toString();

	/* Ignore array */
	if (str.startsWith("["))
	    return;

	int index = str.indexOf('.');
        if (index > 0)
            System.out.println(str.substring(0, index) + ".class");
        else
            System.out.println(str + ".class");

	Enumeration e = node.dependsOn();
        while ( e.hasMoreElements() ){
            DependenceArc a = (DependenceArc)(e.nextElement());
            Object mn = a.to().name();
            str = (String) mn.toString();
	    if (str.startsWith("["))
		continue;
	    index = str.indexOf('.');
	    if (index > 0)
                System.out.println(str.substring(0, index) + ".class");
	    else
	        System.out.println(str + ".class");
	}
     }   
   
     private void showOne( String classname ){
	MemberDependenceNode mdn;
	ClassEntry c = stringToClass( classname.replace('.', '/') );
	mdn = c.lookupMember( staticInitializerName+staticInitializerSig );
	if (mdn != null)
	    printDependences(mdn);
	
	Enumeration members = c.members();
        if ( members != null ){
            while ( members.hasMoreElements() ){
                mdn = (MemberDependenceNode)(members.nextElement() );
		printDependences(mdn);
	    }
	}
     }

     private void showAll() {
        analyzer.analyzeAllDependences();
        Enumeration e = analyzer.allNodes();
        int nClasses = 0; 
        while ( e.hasMoreElements()){
            DependenceNode node = (DependenceNode)(e.nextElement());
            printDependences( node );
            nClasses++;
        }
        System.out.println("TOTAL OF "+nClasses+" CLASSES");
     }

     private void showClassList() {
        analyzer.analyzeAllDependences();
        Enumeration e = analyzer.allNodes();
        while ( e.hasMoreElements()){
            DependenceNode node = (DependenceNode)(e.nextElement());
            printClassList( node );
        }
     }

    private void showPath( String srcmembername, String destmembername ){
	MemberDependenceNode src = stringToNode( srcmembername );
	MemberDependenceNode dest = stringToNode( destmembername );
	if ( src.state() != DependenceNode.ANALYZED ){
	    System.err.println( srcmembername+" not part of graph");
	    return;
	}
	if ( dest.state() != DependenceNode.ANALYZED ){
	    System.err.println( destmembername+" not part of graph");
	    return;
	}
	clearUserFlags( PathComponent.INSET );
	Set tipset = new Set();
	PathComponent r = new PathComponent( null, src );
	tipset.addElement( r );
	for ( int i = 0; i < 50; i++ ){
	    Set newtips = new Set();
	    Enumeration newGrowth = tipset.elements();
	    while ( newGrowth.hasMoreElements() ){
		PathComponent t = (PathComponent)(newGrowth.nextElement() );
		if ( t.link.state() == DependenceNode.UNANALYZED ){
		    System.err.println("Potential path element unanalyzed: "+t.link.name() );
		} else {
		    t.grow( newtips );
		}
	    }
	    if ( newtips.size() == 0 ){
		// exhaused the space without finding it.
		System.out.println("No path from "+srcmembername+" to "+destmembername );
		return;
	    }
	    // now see if we got to our destination.
	    newGrowth = newtips.elements();
	    boolean foundDest = false;
	    while ( newGrowth.hasMoreElements() ){
		PathComponent t = (PathComponent)(newGrowth.nextElement() );
		if ( t.link == dest ){
		    t.print( System.out );
		    System.out.println();
		    foundDest = true;
		}
		t.link.flags |= PathComponent.INSET;
	    }
	    if ( foundDest ){
		return;
	    }
	    // round and round.
	    tipset = newtips;
	}
	System.out.println("Path from "+srcmembername+" to "+destmembername+" is too long" );
    }

    private void printList( java.io.PrintStream o, Vector elementList ){
	printFlaggedList( o, elementList, ANY ); 
    }

    private void printFlaggedList( java.io.PrintStream o, Vector elementList, int flagmask ){
	if ( elementList == null ) {
	    o.println("	<nothing>");
	    return;
	}
	Enumeration loadlist = elementList.elements();
	while ( loadlist.hasMoreElements() ){
	    DependenceNode thing = (DependenceNode)(loadlist.nextElement());
	    if ( (thing.flags&flagmask) != 0 ){
		o.println( thing.name() );
	    }
	}
	o.flush();
    }


    private void processArgs( String args[] ){
	for ( int i = 0; i < args.length; i++ ){
	    String a = args[i];
	    /*
	     * -classpath is common to all uses of
	     * dependence analysis. The argument is a
	     * :-separated list of places to look for
	     * class files. This can include jar/zip files
	     * and directories. This can be given multiple
	     * times, and is cumulative.
	     */
	    if ( a.equals("-classpath") ){
		finder.addToSearchPath( args[++i] );
		continue;
	    }
	    /*
	     * -exclude is common to all uses of dependence
	     * analysis. The argument is a full class name,
	     * for a class which is not to be analysed
	     * (except for the names it exports and
	     * superclass relationships). If the last 
	     * component of the name is '*', then this
	     * represents a whole package to be excluded.
	     */
	    if ( a.equals("-exclude") ){
		excludeClass( args[++i] );
		continue;
	    }
	    /*
	     * -f filename
	     * to name a file containing more commands.
	     * It is used mostly for including canned
	     * sequences of commands, such as those naming
	     * all the classes required by the implementation.
	     * They are processed in order, immediately.
	     */
	    if ( a.equals("-f" ) ){
		String filename = args[++i];
		try {
		    String newargs[] = parseOptionFile( filename );
		    processArgs( newargs ); // just call us recursively.
		} catch ( java.io.IOException ioe ){
		    System.out.println(
			Localizer.getString("javafilter.could_not_process", 
			filename)
		    );
		    ioe.printStackTrace();
		    hadError = true;
		}
		continue;
	    }
	    /*
	     * -v increases verbosity.
	     */
	    if ( a.equals("-v" ) ){
		verbosity += 1;
		continue;
	    }

	    /*
	     * -nativelist filename
	     * to have the names of all native methods included
	     * written on the named file.
	     */
	    if ( a.equals("-nativelist") ){
		try {
		    nativelist = new BufferedPrintStream( new FileOutputStream( args[++i] ) );
		} catch ( Exception e ){
		    System.err.println(
			Localizer.getString("javafilter.cannot_open_nativelist", args[i])
		    );
		    hadError = true;
		}
		continue;
	    }

	    /*
	     * Dependence loading.
	     *     -load full-member-name to specify a member.
	     */
	    if ( a.equals("-load") ){
		membersToLoad.addElement( args[++i] );
		continue;
	    }
	    /*
	     *    -new full-class-name 
	     *    -new full-class-name(sig)V
	     * 		A shorthand for naming constructors
	     *		-load full-class-name.<init>()V or
	     *		-load full-class-name.<init>(sig)V
	     */
	    if ( a.equals("-new") ){
		constructorsToLoad.addElement( args[++i] );
		continue;
	    }
	    /*
	     *    -main full-class-name is shorthand for
	     *		-load full-class-name.main(Ljava/lang/String;)V
	     */
	    if ( a.equals("-main") ){
		membersToLoad.addElement( args[++i] + "." + mainName + mainSig  );
		continue;
	    }
	    /*
	     *     -loadClass full-class-name to specify all
	     *			members of a class.
	     */
	    if ( a.equals("-loadClass") ){
		classesToLoad.addElement( args[++i] );
		continue;
	    }
	    /*
	     *	   -interface full-class-name to specify a
	     *		contract that must be honored by
	     *		subclasses of the named class or
	     *		implementors of an interface.
	     */
	    if ( a.equals("-interface") ){
		interfaceClasses.addElement( args[++i] );
		continue;
	    }
	    /*
	     *	    -wholeClass full-class-name or
	     *	    -wholeClass packagename.*
	     *		load the whole class.
	     */
	    if ( a.equals("-wholeClass") ){
		allClassMembers( args[++i] );
		continue;
	    }
	    /*
	     *	    -wholeData full-class-name or
	     *	    -wholeData packagename.*
	     *		load all the data.
	     */
	    if ( a.equals("-wholeData") ){
		allClassData( args[++i] );
		continue;
	    }
	    /*
	     *	-dependence full-member-name verb full-member-name
	     *	To describe to the program dependences that it 
	     *	cannot be understood. Chiefly to describe
	     *	dependences from native-code methods, which cannot
	     *	otherwise be analysed. "verb" is one of:
	     *	    reads
	     *	    writes
	     *	    calls
	     *	    classRef, in which case the 3rd operand is actually
	     *		a full-class-name
	     */
	    if ( a.equals("-dependence") ){
		dependences.addElement(
		    new DependenceRelation( args[++i], args[++i], args[++i] )
		);
		continue;
	    }
	
	    if ( a.equals("-help")) {
		printUsage();
	        break;
	    }

	    if ( a.equals("-listSignatureDepend")) {
		analyzer.useSignatureDependence(true);
		continue;
	    }		

	    /*
	     * -path a z
	     * To show the how a dependes on z. Prints all dependence chains of the
	     * shortest length. Probably does not deal in overloading issues, yet.
	     */
	    if ( a.equals("-path") ){
		paths.addElement( new PathPair( args[++i], args[++i] ) );
		continue;
	    }

	    if ( a.equals("-showAll")) {
		details = true;
		continue;
	    }

	    if ( a.equals("-showClassList")) {
		classlistonly = true;
		continue;
	    }
	
	    if ( a.equals("-showDirect") ) {
		showClasses.addElement( args[++i] );
		break;
	    }
	    System.out.println(Localizer.getString("javafilter.command_not_recognized", args[i]));
	    hadError = true;
	}
    }

    private void printUsage() {
	System.out.println("printUsage");
    }

    private Vector interfaceClasses;
    private Vector dependences;
    private Vector membersToLoad;
    private Vector constructorsToLoad;
    private Vector classesToLoad;
    private Vector paths;
    private Vector showClasses;

    private boolean process( String args[] ){
	interfaceClasses = new Vector();
	dependences = new Vector();
	membersToLoad = new Vector();
	constructorsToLoad = new Vector();
	classesToLoad = new Vector();
	paths = new Vector();
	showClasses = new Vector();

	processArgs( args );
	if ( ! hadError )
	    doLoad();

	/* showPath must do after the nodes are analyzed. */
	Enumeration p = paths.elements();
	if ( (! hadError) && ( p!=null) ){
	    while (p.hasMoreElements() ){
		PathPair pp  = (PathPair)(p.nextElement());
		showPath( pp.src, pp.dest );
	    }
	}
	p = showClasses.elements();
	if ( (! hadError) && ( p!=null) ){
            while (p.hasMoreElements() ){
		showOne((String)p.nextElement());
            }
        }

	if (details)
	    showAll();

	if (classlistonly) 
	    showClassList();
	return hadError;
    }

    public static void main( String args[] ){
	boolean fail = (new JavaMemberDepend().process( args ));
	if ( fail ){
	    System.exit( 1 );
	}
    }

    class NeverAccept extends util.ClassnameFilter{
        public boolean accept( java.io.File dir, String className ){
            return false;
        }
    }

    class PathPair {
        String src;
        String dest;

        PathPair( String s, String d ){
            src = s;
            dest = d;
        }
    }
}
