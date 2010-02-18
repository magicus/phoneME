/*
 * @(#)ExtensionInstallationProvider.java	1.11 06/10/10
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

/**
 * This interface defines the contract a extension installation capable
 * provided to the extension installation dependency mechanism to 
 * install new extensions on the user's disk
 *
 * @author  Jerome Dochez
 * @version 1.5, 02/02/00
 */
public interface ExtensionInstallationProvider {
    
    /*
     * <p>
     * Request the installation of an extension in the extension directory
     * </p>
     *
     * @param requestExtInfo information on the extension that need to be 
     * installed
     * @param installedExtInfo information on the current compatible installed
     * extension. Can be null if no current installation has been found.
     * @return true if the installation is successful, false if the 
     * installation could not be attempted.
     * @exception ExtensionInstallationException if an installation was 
     * attempted but did not succeed.
     */
    boolean installExtension(ExtensionInfo requestExtInfo, 
			     ExtensionInfo installedExtInfo) 
	throws ExtensionInstallationException;
}
