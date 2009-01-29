/*
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
 */

package com.sun.jump.module.installer;

import com.sun.jump.module.installer.JUMPInstallerModule;
import com.sun.jump.module.installer.JUMPInstallerModuleFactory;
import com.sun.jump.common.JUMPContent;
import java.util.Vector;

/**
 * <code>JUMPInstallerFilter</code> provides the ability to filter
 * types of content from all installers.
 */
public class JUMPInstallerFilter {

    /**
     * Get all installed content of type. 
     */
    public static JUMPContent[] getContentOfType(Class contentClass) {
        if (!JUMPContent.class.isAssignableFrom(contentClass)) {
	    return null;
	}
	JUMPInstallerModule[] installers = 
	    JUMPInstallerModuleFactory.getInstance().getAllInstallers();
	Vector v = new Vector();
	for (int i = 0; i < installers.length; i++) {
	    JUMPInstallerModule inst = installers[i];
	    JUMPContent[] c = inst.getInstalled();
	    for (int j = 0; j < c.length; j++) {
		/* filter any content that is a subclass of the "contentClass"
		   parameter */
		if (contentClass.isAssignableFrom(c[j].getClass())) {
		    v.addElement(c);
		}
	    }
	}
	return (JUMPContent[])v.toArray();
    }

}
