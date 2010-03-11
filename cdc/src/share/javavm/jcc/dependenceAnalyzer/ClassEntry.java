/*
 * @(#)ClassEntry.java	1.27 06/10/10
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

package dependenceAnalyzer;
import util.*;
import java.util.*;
import components.*;
import consts.Const;

/*
 * Class ClassEntry is for use with Member-level dependence analysis.
 * It is not only a part of the dependence relations but also
 * a container for MemberDependenceNodes, and thus
 * a part of the naming mechanism.
 *
 * This is where class members are parsed, and thus
 * their dependences discovered and exposed.
 */

public class ClassEntry extends DependenceNode implements MemberArcTypes {
    /*
     * public parts.
     */
    public boolean signatureFlag = false;
    public ClassEntry	superClass(){ return superClassEntry; }
    public Enumeration  subclasses(){
	return (subclassSet == null ) ? EmptyEnumeration.instance : subclassSet.elements();
    }
    public Enumeration	members(){
	return (memberSet == null ) ? EmptyEnumeration.instance : memberSet.elements();
    }
    public Enumeration interfaces(){
	return (interfaceSet == null ) ? EmptyEnumeration.instance : interfaceSet.elements();
    }

    public MemberDependenceNode lookupMember( MemberName mn ){
	if ( memberSet == null ) return null;
	return (MemberDependenceNode)(memberSet.get( mn ));
    }

    private MemberName t = new MemberName( this, "" );
    
    public  MemberDependenceNode lookupMember( String mname ){
	// ... won't work multithreaded!...
	t.name = mname.intern();
	return lookupMember( t );
    }

    public MemberDependenceNode lookupAddMember( MemberName mn, int mflags ){
	MemberDependenceNode mnode = lookupMember( mn );
	if ( mnode == null ){
	    mnode = new MemberDependenceNode( mn, mflags );
	    if ( memberSet == null )
		memberSet = new Hashtable();
	    memberSet.put( mn, mnode );
	} else if ( mflags != 0 ){
	    mnode.flags = mflags;
	}
	if ( (this.flags&EXCLUDED) != 0 ){
	    // class is excluded, and so are all members.
	    mnode.flags |= MemberDependenceNode.EXCLUDED;
	}

	return mnode;
    }

    public ClassEntry( String name ){
	super( name );
	className = name;
	nodeState = UNANALYZED;
    }

    public boolean analyzeClass( ClassDictionary cd, boolean f ){
	signatureFlag = f;	
	return (analyzeClass(cd));
    }

    public boolean analyzeClass( ClassDictionary cd ){
	if ( nodeState != UNANALYZED ) return(nodeState!=ERROR); // already done.
	nodeState = ANALYZED; // do this early to prevent excess recursion
	cdict = cd; // share the dictionary with the rest of the code.

	ClassInfo ci = cdict.findClassInfo( className );
	if ( ci == null ){
	    // cannot find class info to analyze.
	    // this is not good.
	    nodeState = ERROR;
	} else {
	    nodeState = parseClassInfo( ci );
	}
	return (nodeState!=ERROR);
    }

    /*
     * A class which is marked as an interface does not have to be
     * a Java TM Interface. It merely has to define a contract between
     * two software subsystems. java.applet.Applet is a good example of
     * an interface.
     * An interface is always EXCLUDED.
     * An interface has additional, imputed, members, which
     * are inhereted from superclasses and interfaces, and which
     * form part of the contract. makeInterface creates these after
     * inspecting all superclasses and Java interfaces. This is so that
     * our override-based dependence analysis works.
     * makeInterface returns true on success, false on any adversary
     * Adversary includes:
     *	inability to find class file
     *  class is not EXCLUDED
     *  inability to find superclasses or interfaces
     */
    public boolean makeInterface( ClassDictionary cd ){
	cdict = cd;
	if ( nodeState == UNANALYZED ) 
	    analyzeClass( cd );
	if ( nodeState == ERROR )
	    return false;
	if ( ( flags & EXCLUDED ) == 0 ) return false;
	if ( ( flags & DEFINES_INTERFACE ) != 0 ) return true; // done!
	if ( ! imputeMembers( superClassEntry )) return false;
	if ( interfaceSet != null ){
	    Enumeration ilist = interfaceSet.elements();
	    while ( ilist.hasMoreElements() ){
		if ( ! imputeMembers( (ClassEntry)(ilist.nextElement()) ) )
		    return false;
	    }
	}
	flags |= DEFINES_INTERFACE;
	return true;
    }

    /*
     * Flags. This is pretty much up to the user, except for this one
     * which will govern how things work.
     */
    public static final int EXCLUDED		= MemberDependenceNode.EXCLUDED;
    public static final int DEFINES_INTERFACE	= 2;
    public static final int HAS_OVERRIDING	= 4;


    /*
     * Implementation parts.
     */
    private String	className;
    private ClassEntry	superClassEntry;
    private Vector	subclassSet;
    private Dictionary	memberSet;
    private Vector	interfaceSet = new Vector();

    // 
    // I feel rather silly having this here, as there is
    // no requirement that a ClassEntry be associated with only
    // one dictionary. But the fact is, this variable is
    // referenced in many many places in the analysis routines,
    // and I'd otherwise have to keep passing it around...
    private ClassDictionary cdict;

    private int
    parseClassInfo( ClassInfo ci ){
	//
	// This class cannot be understood without
	// first understanding its super class
	//
	int resultState = ANALYZED;

	if ( ci.superClass != null ){
	    ClassEntry sup =  cdict.lookupAdd( ci.superClass.name.string );
	    if ( sup.nodeState == UNANALYZED )
		sup.analyzeClass( cdict ); // recurse.
	    if ( sup.nodeState != ERROR ){
		superClassEntry = sup;
		sup.addSubclass( this );
	    }
	}

	// interfaces, if any.
	if ( ci.interfaces != null ){
	    for ( int i = 0; i < ci.interfaces.length; i++ ){
		ClassEntry sup =  cdict.lookupAdd( ci.interfaces[i].name.string );
		interfaceSet.addElement( sup );
		sup.addSubclass( this );
		if ( sup.nodeState == UNANALYZED )
		    sup.analyzeClass( cdict ); // recurse.
	    }
	}

	// Take Care the Inner class as well.
        if ( ci.classAttributes != null ){
            for ( int i = 0; i < ci.classAttributes.length; i++ ){
		if (ci.classAttributes[i].name.string.equals("InnerClasses")) {
		    for (int j = 0; j < ci.innerClassAttr.getInnerClassCount(); 
                                                                         j++) {
		    String name = ci.innerClassAttr.getFullName(j);
		    if (name.equals("ICA-ReferingToSelf"))
			continue;

                    ClassEntry inner =  cdict.lookupAdd( name );
                    if ( inner.nodeState == UNANALYZED )
			// if -signatureFlag is set, get api dependency as well.
                        inner.analyzeClass( cdict, signatureFlag); // recurse.
		    }
		}
	    }
        }

	MemberDependenceNode mnode;

	// all the methods.
	if ( ci.methods != null ){
	    for ( int i = 0; i < ci.methods.length; i++ ){
		mnode = overrideAnalysis( parseMethodInfo( ci.methods[i] ) );
		if ( mnode == null ){
		    resultState = ERROR; // bad news.
		} else {
		    mnode.nodeState = DependenceNode.ANALYZED;
		}
	    }
	}

	// and the fields.
	if ( ci.fields != null ){
	    for ( int i = 0; i < ci.fields.length; i++ ){
		mnode = parseFieldInfo( ci.fields[i] );
		if ( mnode == null ){
		    resultState = ERROR; // bad news.
		} else {
		    mnode.nodeState = DependenceNode.ANALYZED;
		}
	    }
	}

	return resultState;
    }

    private MemberDependenceNode overrideAnalysis( MemberDependenceNode mn ){
	if ( mn == null ) return null; // must have been some error
	if ( (mn.flags&MemberDependenceNode.NO_OVERRIDING) != 0 )
	    return mn; // these don't override.
	MemberName nm = (MemberName)((MemberName)(mn.name())).clone(); // make a dirty copy.
	int anyOverriding = 0;
	if ( superClassEntry != null )
	    anyOverriding += superClassEntry.overrideCheckUp( mn, nm );
	Enumeration ife = interfaceSet.elements();
	while( ife.hasMoreElements() ){
	    ClassEntry iface = (ClassEntry)(ife.nextElement() );
	    anyOverriding += iface.overrideCheckUp( mn, nm );
	}
	if ( anyOverriding != 0 ){
	    this.flags |= HAS_OVERRIDING;
	}
	return mn;
    }

    private int overrideCheckUp( MemberDependenceNode mn,
	MemberName nameCopy ){

	if ( this.nodeState != ANALYZED ) return 0; // error or unanalyzed interface

	int nOverrides = 0;

	nameCopy.classEntry = this;
	MemberDependenceNode target = lookupMember( nameCopy );
	if ( target != null ){
	    // have a case of overriding.
	    // various sets may not exist yet, so be careful.
	    if ( target.memberOverriddenBy == null ){
		target.memberOverriddenBy = new util.Set();
	    }
	    target.memberOverriddenBy.addElement( mn );
	    target.flags |= MemberDependenceNode.OVERRIDDEN;
	    if ( mn.memberOverrides == null ){
		mn.memberOverrides = new util.Set();
	    }
	    mn.memberOverrides.addElement( target );
	    nOverrides += 1;
	}
	if ( superClassEntry != null )
	    nOverrides += superClassEntry.overrideCheckUp( mn, nameCopy );
	Enumeration ife = interfaceSet.elements();
	while( ife.hasMoreElements() ){
	    ClassEntry iface = (ClassEntry)(ife.nextElement() );
	    nOverrides += iface.overrideCheckUp( mn, nameCopy );
	}

	return nOverrides;
    }

    private static int getUnsignedShort( byte code[], int w ){
	return  (( (int)code[w] &0xff ) << 8 ) | ( (int)code[w+1] &0xff );
    }

    private ClassEntry conditionalClassReference( MemberDependenceNode referring, ClassConstant ref, int referenceType ){
	String classname = ref.name.string;
	if ( className.equals( classname ) ){
	    return this;
	}
	ClassEntry cer = cdict.lookupAdd( classname );
	DependenceArc de = new DependenceArc( referring, cer, referenceType );
	referring.nodeDependsOn.addElement( de );
	cer.nodeDependedOn.addElement( de );
	return cer;
    }

    private void addCrossReference( MemberDependenceNode referring, FMIrefConstant fref, int arctype ){
	ClassEntry targetClass = conditionalClassReference( referring, fref.clas, ARC_CLASS );
	String refname;
	if ( arctype ==  ARC_CALLS ){
	    // for methods, name includes signature.
	    refname = fref.sig.name.string+fref.sig.type.string;
	} else {
	    // for data, name is just name.
	    refname = fref.sig.name.string;
	}
	MemberDependenceNode targetNode =
	    targetClass.lookupAddMember( 
		new MemberName( targetClass, refname ),
		0
	    );
	DependenceArc de = new DependenceArc( referring, targetNode, arctype );
	referring.nodeDependsOn.addElement( de );
	targetNode.nodeDependedOn.addElement( de );
    }

    private ClassEntry javaLangString;
    private MemberDependenceNode stringInit;

    private void makeString( MemberDependenceNode referring ){
	if ( javaLangString == null ){
	    javaLangString = cdict.lookupAdd( "java/lang/String" );
	    stringInit = javaLangString.lookupAddMember(
		new MemberName( javaLangString, "<init>([C)V" ),
		0
	    );
	}
	DependenceArc da = new DependenceArc( referring, stringInit, ARC_CALLS );
	referring.nodeDependsOn.addElement( da );
	stringInit.nodeDependedOn.addElement( da );
    }

    private void doSignatureDepend(String str, MemberDependenceNode referring) {
	ParamSignatureParser csi;

        csi = new ParamSignatureParser(str, referring);
        try {
            csi.iterate();
        } catch ( util.DataFormatException e ){
	    e.printStackTrace();
        }
    }  

    private MemberDependenceNode
    parseMethodInfo( MethodInfo mi ){

	MemberDependenceNode md = parseClassMemberInfo( mi, MemberDependenceNode.METHOD );

	if ( (md.flags&MemberDependenceNode.EXCLUDED ) != 0 ){
	    return md; // we have all we need.
	}

	/* Should set an option -listSignatureDepend to turn this on. how??  */
	if (signatureFlag) 
	    doSignatureDepend(mi.type.string, md);
	

	if ( mi.exceptionTable != null ){
	    for ( int j = 0; j < mi.exceptionTable.length; j++ ){
		ExceptionEntry ee = mi.exceptionTable[j];
		if ( ee.catchType != null )
		    conditionalClassReference( md, ee.catchType, ARC_EXCEPTION );
	    }
	}
	if ( mi.exceptionsThrown != null ){
	    for ( int j = 0; j < mi.exceptionsThrown.length; j++ ){
		conditionalClassReference( md, mi.exceptionsThrown[j], ARC_EXCEPTION );
	    }
	}
    doCode:
	if ( mi.code != null ){
	    byte code[] = mi.code;
	    try {
		int  locs[] = mi.getLdcInstructions();
		ConstantObject cpool[] =
		    mi.parent.getConstantPool().getConstants();

		for ( int j = 0; j < locs.length; j++ ){
		    ConstantObject o = cpool[ (int)code[locs[j]+1]&0xff ];
		    if ( o instanceof StringConstant ){
			makeString( md );
		    } else if ( o instanceof ClassConstant ){
			conditionalClassReference( md,
			    (ClassConstant) o, ARC_CLASS );
		    }
		}
		locs = mi.getWideConstantRefInstructions();
		for ( int j = 0; j < locs.length; j++ ){
		    FMIrefConstant fref;

		    switch ( (int)code[locs[j] ] & 0xff ){
		    case Const.opc_anewarray:
		    case Const.opc_checkcast:
		    case Const.opc_multianewarray:
		    case Const.opc_new:
		    case Const.opc_instanceof: {
			conditionalClassReference( md,
			    (ClassConstant) cpool[ getUnsignedShort( code, locs[j]+1 ) ], ARC_CLASS );
			break;
		    }
		    case Const.opc_getfield:
		    case Const.opc_getstatic:
			fref = (FMIrefConstant) cpool[ getUnsignedShort( code, locs[j]+1 ) ];
			addCrossReference( md, fref, ARC_READS );
			break;
		    case Const.opc_putfield:
		    case Const.opc_putstatic:{
			fref = (FMIrefConstant) cpool[ getUnsignedShort( code, locs[j]+1 ) ];
			addCrossReference( md, fref, ARC_WRITES );
			break;
		    }
		    case Const.opc_ldc_w: {
			ConstantObject o = cpool[ getUnsignedShort( code, locs[j]+1 ) ];
			if ( o instanceof StringConstant ){
			    makeString( md );
			} else if ( o instanceof ClassConstant ){
			    conditionalClassReference( md,
				(ClassConstant) o, ARC_CLASS );
			}
			break;
		    }
		    case Const.opc_invokeinterface: 
		    case Const.opc_invokevirtual:
		    case Const.opc_invokespecial:
		    case Const.opc_invokestatic:
			fref = (FMIrefConstant) cpool[ getUnsignedShort( code, locs[j]+1 ) ];
			addCrossReference( md, fref, ARC_CALLS );
			break;
		    }
		}
	    } catch ( Exception e ){
		System.out.println(this);
		e.printStackTrace();
		break doCode;
	    }
	}
	return md;
    }

    private MemberDependenceNode parseFieldInfo( FieldInfo fi ){
	return parseClassMemberInfo( fi, MemberDependenceNode.FIELD );
    }
    
    private MemberDependenceNode parseClassMemberInfo( ClassMemberInfo mi, int flagval ){
	boolean isStatic = ( mi.isStaticMember() )
		|| ( mi.name.string.charAt(0) == '<' );

	if ( isStatic )
	    flagval |= MemberDependenceNode.STATIC;
	if ( mi.isPrivateMember() )
	    flagval |= MemberDependenceNode.PRIVATE;
	if ( mi.name.string.equals("<init>") )
	    flagval |= MemberDependenceNode.INIT;
	if ( mi.name.string.equals("<clinit>") )
	    flagval |= MemberDependenceNode.CLINIT;
	if (  (mi.access&Const.ACC_NATIVE) != 0 )
	    flagval |= MemberDependenceNode.NATIVE;

	String memberName= mi.name.string;
	if ( (flagval&MemberDependenceNode.METHOD ) != 0 )
	    memberName += mi.type.string;
	return lookupAddMember(
	    new MemberName( this, memberName ),
	    flagval );
    }

    private boolean imputeMembers( ClassEntry other ){
	if ( other == null ) return true; // vacuous
	if ( other.nodeState == UNANALYZED )
	    other.analyzeClass( cdict );
	if ( other.nodeState == ERROR ) return false;
	if ( other.memberSet != null ){
	    Enumeration omembers = other.memberSet.elements();
	    while ( omembers.hasMoreElements() ){
		MemberDependenceNode mn = (MemberDependenceNode)(omembers.nextElement());
		if ( (mn.flags&MemberDependenceNode.NO_OVERRIDING) != 0 )
		    continue; // never mind. Not part of interface.
		MemberName tname = (MemberName)(((MemberName)(mn.name())).clone());
		tname.classEntry = this;
		MemberDependenceNode ourmn = lookupAddMember( tname, 0 );
		if ( (ourmn.flags&~EXCLUDED) == 0 ){
		    // no flags set. That means that we just inserted it.
		    ourmn.flags |= mn.flags | MemberDependenceNode.IMPUTED;
		}
	    }
	}
	if ( ! imputeMembers( other.superClassEntry ) ) return false;
	if ( other.interfaceSet != null ){
	    Enumeration ilist = other.interfaceSet.elements();
	    while ( ilist.hasMoreElements() ){
		if ( ! imputeMembers( (ClassEntry)(ilist.nextElement()) ) )
		    return false;
	    }
	}
	return true;
    }

    private void addSubclass( ClassEntry newSub ){
	if ( subclassSet == null ){
	    subclassSet = new Vector();
	}
	subclassSet.addElement( newSub );
    }

    private boolean hasSubclass( ClassEntry other ){
	if ( subclassSet == null ) return false;
	Enumeration subs = subclassSet.elements();
	while ( subs.hasMoreElements() ){
	    if ( subs.nextElement() == other )
		return true;
	}
	subs = subclassSet.elements();
	while ( subs.hasMoreElements() ){
	    if ( ((ClassEntry)(subs.nextElement())).hasSubclass( other ) )
		return true;
	}
	return false;
    }

    class ParamSignatureParser extends util.SignatureIterator {
	String params[];
	MemberDependenceNode ref;

        public ParamSignatureParser( String sig, MemberDependenceNode r ){
	    super(sig);
	    ref = r;
        }

	public void do_array(int arrayDepth, int subTypeStart, int subTypeEnd) {
            ClassEntry cer;
            DependenceArc da;

	    if (subTypeStart == subTypeEnd)
		return;

            cer = cdict.lookupAdd(sig.substring(subTypeStart+1, subTypeEnd));
            da = new DependenceArc(ref, cer, ARC_CLASS);
            ref.nodeDependsOn.addElement(da);
            cer.nodeDependedOn.addElement(da);
	}
	
	public void do_object( int subTypeStart, int subTypeEnd ) {
	    ClassEntry cer;
	    DependenceArc da;

	    cer = cdict.lookupAdd(sig.substring(subTypeStart+1, subTypeEnd));
	    da = new DependenceArc(ref, cer, ARC_CLASS);
	    ref.nodeDependsOn.addElement(da);
	    cer.nodeDependedOn.addElement(da);	
	}

    }

}

