/*
 * @(#)ExtensionInfo.java	1.11 06/10/10
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

package sun.misc;

import java.util.StringTokenizer;
import java.util.jar.Attributes;
import java.util.jar.Attributes.Name;

/**
 * This class holds all necessary information to install or
 * upgrade a extension on the user's disk
 *
 * @author  Jerome Dochez
 * @version 1.5, 02/02/00
 */
public class ExtensionInfo {

    /**
     * <p>
     * public static values returned by the isCompatible method
     * </p>
     */
    public static final int COMPATIBLE = 0;
    public static final int REQUIRE_SPECIFICATION_UPGRADE = 1;
    public static final int REQUIRE_IMPLEMENTATION_UPGRADE = 2;
    public static final int REQUIRE_VENDOR_SWITCH = 3;
    public static final int INCOMPATIBLE = 4;

    /**
     * <p>
     * attributes fully describer an extension. The underlying described 
     * extension may be installed and requested.
     * <p>
     */
    public String title;
    public String name;
    public String specVersion;
    public String specVendor;
    public String implementationVersion;
    public String vendor;
    public String vendorId;
    public String url;

    /**
     * <p>
     * Create a new unintialized extension information object
     * </p>
     */
    public ExtensionInfo() {
    }

    /**
     * <p>
     * Create and initialize an extension information object.
     * The initialization uses the attributes passed as being
     * the content of a manifest file to load the extension
     * information from.
     * Since manifest file may contain information on several
     * extension they may depend on, the extension key parameter
     * is prepanded to the attribute name to make the key used
     * to retrieve the attribute from the manifest file
     * <p>
     * @param extensionKey unique extension key in the manifest
     * @param attr Attributes of a manifest file
     */
    public ExtensionInfo(String extensionKey, Attributes attr) 
	throws NullPointerException 
    {
	String s;
	if (extensionKey!=null) {
	    s = extensionKey + "-";
	} else {
	    s ="";
	}
	String attrKey = s + Name.EXTENSION_NAME.toString();
	name = attr.getValue(attrKey);
	attrKey = s + Name.SPECIFICATION_TITLE.toString();
	title = attr.getValue(attrKey);
	attrKey = s + Name.SPECIFICATION_VERSION.toString();
	specVersion = attr.getValue(attrKey);
	attrKey = s + Name.SPECIFICATION_VERSION.toString();
	specVersion = attr.getValue(attrKey);
        attrKey = s + Name.IMPLEMENTATION_VERSION.toString();
	implementationVersion = attr.getValue(attrKey);
        attrKey = s + Name.IMPLEMENTATION_VENDOR.toString();	
	vendor = attr.getValue(attrKey);
        attrKey = s + Name.IMPLEMENTATION_VENDOR_ID.toString();	
	vendorId = attr.getValue(attrKey);
        attrKey =s + Name.IMPLEMENTATION_URL.toString();	
	url = attr.getValue(attrKey);	
    }

    /**
     * <p>
     * @return true if the extension described by this extension information
     * is compatible with the extension described by the extension 
     * information passed as a parameter
     * </p>
     *
     * @param the requested extension information to compare to
     */
    public int isCompatibleWith(ExtensionInfo ei) {
	
	if (name == null || ei.name == null)
	    return INCOMPATIBLE;
	if (name.compareTo(ei.name)==0) {
	    System.out.println("Potential match");
	    
	    // is this true, if not spec version is specified, we consider 
	    // the value as being "any".
	    if (specVersion == null || ei.specVersion == null) 
		return COMPATIBLE;
	    
	    int version = compareExtensionVersion(specVersion, ei.specVersion);
	    if (version<0) {
		// this extension specification is "older"
		if (vendorId != null && ei.vendorId !=null) {
		    if (vendorId.compareTo(ei.vendorId)!=0) {
			return REQUIRE_VENDOR_SWITCH;
		    }
		}
		return REQUIRE_SPECIFICATION_UPGRADE;
	    } else {
		// the extension spec is compatible, let's look at the 
		// implementation attributes
		if (vendorId != null && ei.vendorId != null) {
		    // They care who provides the extension
		    if (vendorId.compareTo(ei.vendorId)!=0) {
			// They want to use another vendor implementation
			return REQUIRE_VENDOR_SWITCH;
		    } else {
			// Vendor matches, let's see the implementation version
			if (implementationVersion != null && ei.implementationVersion != null) {
			    // they care about the implementation version 
			    version = compareExtensionVersion(implementationVersion, ei.implementationVersion);
			    if (version<0) {
				// This extension is an older implementation
				return REQUIRE_IMPLEMENTATION_UPGRADE;
			    }
			}
		    }
		}
		// All othe cases, we consider the extensions to be compatible
		return COMPATIBLE;
	    }
	}
	return INCOMPATIBLE;
    }

    /**
     * <p>
     * helper method to print sensible information on the undelying described 
     * extension
     * </p>
     */
    public String toString() {
	return "Extension : " + title + "(" + name + "), spec version(" + 
	    specVersion + "), impl version(" + implementationVersion +
	    ") from " + vendor + "(" + vendorId + ")";
    }

    /*
     * <p>
     * helper method to compare two versions. 
     * version are in the x.y.z.t pattern.
     * </p>
     * @param source version to compare to
     * @param target version used to compare against
     * @return < 0 if source < version
     *         > 0 if source > version
     *         = 0 if source = version
     */
    private int compareExtensionVersion(String source, String target)
	throws NumberFormatException
    {
	StringTokenizer stk = new StringTokenizer(source, ".,");
	StringTokenizer ttk = new StringTokenizer(target, ".,");

	while(stk.hasMoreTokens() || ttk.hasMoreTokens()) {
	    int sver, tver;
	    
	    if (stk.hasMoreTokens())
		sver = Integer.parseInt(stk.nextToken());
	    else
		sver = 0;

	    if (ttk.hasMoreTokens())
		tver = Integer.parseInt(ttk.nextToken());
	    else
		tver = 0;

	    if (sver<tver)
		return -1;
	    if (sver>tver)
		return 1;
	    // equality, continue with the next version number
	}
	// complete equality
	return 0;
    }


}
