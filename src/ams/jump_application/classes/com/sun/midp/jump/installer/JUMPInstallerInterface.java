/*
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.jump.installer;

import java.io.IOException;
import com.sun.midp.installer.InvalidJadException;
import com.sun.midp.midletsuite.MIDletSuiteLockedException;

public interface JUMPInstallerInterface {

    /**
     * Verifies and installs an MIDlet suite based on a jad.  
     * It is expected that the .jad and .jar files are already downloaded from 
     * the server to the local filesystem by the time this method is invoked.
     *
     * @param jadSourceUrl the original server location of the jad that got
     * downloaded.
     * @param encoding Character encoding of the jad.
     * @param tempJadFileName The location of the jad file in the
     * local filesystem
     * @param tempJarFileName The location of the jar file in the
     * local filesystem
     * @param isUpdate true if this is an update, false if this is a fresh
     * install.
     *
     * @return the installed midlet suite ID.
     */
    int verifyAndStoreSuite(String jadSourceUrl, String encoding,
			    String tempJadFileName, String tempJarFileName,
			    boolean isUpdate)
	throws IOException, SecurityException, 
	       InvalidJadException, MIDletSuiteLockedException;
    
    /**
     * Verifies and installs an MIDlet suite based on a jar.  
     * It is expected that the .jar file are already downloaded from 
     * the server to the local filesystem by the time this method is invoked.
     *
     * @param jarSourceUrl the original server location of the jar that
     * got downloaded.
     * @param tempJarFileName The location of the jar file in the
     * local filesystem
     * @param suiteName The name of the midlet suite to install.
     * @param isUpdate true if this is an update, false if this is a
     * fresh install.
     *
     * @return the installed midlet suite ID.
     */
    int verifyAndStoreSuite(String jarSourceUrl, String tempJarFileName, 
			    String suiteName, boolean isUpdate)
	throws IOException, SecurityException, 
	       InvalidJadException, MIDletSuiteLockedException;
    
}
