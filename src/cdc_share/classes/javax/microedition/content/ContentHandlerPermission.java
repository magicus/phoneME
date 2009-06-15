/*
 *  
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

package javax.microedition.content;

import java.security.Permission;
import java.util.Hashtable;

public class ContentHandlerPermission extends Permission {
	
	private static final String ACTION_REGISTER_STATIC = "register_static";
	private static final String ACTION_REGISTER_DYNAMIC = "register_dynamic";
	private static final int REGISTER_STATIC_FLAG = 0x0001;
	private static final int REGISTER_DYNAMIC_FLAG = 0x0002;
	private static final Hashtable map = new Hashtable();
	static {
		map.put(ACTION_REGISTER_STATIC, new Integer(REGISTER_STATIC_FLAG));
		map.put(ACTION_REGISTER_DYNAMIC, new Integer(REGISTER_DYNAMIC_FLAG));
	}
	private static final String actionStrings[] = new String[] {
		"",
		ACTION_REGISTER_STATIC,
		ACTION_REGISTER_DYNAMIC,
		ACTION_REGISTER_STATIC + "," + ACTION_REGISTER_DYNAMIC
	};
	
	private final int actionFlags;

	public ContentHandlerPermission(String actions) {
		super(null);
		actionFlags = parseActions(actions + ',');
	}
	
	private static int parseActions(String actions) {
		// assert( actions.charAt( actions.length() - 1 ) == ',' )
		int flags = 0;
		
		int idx = 0, cidx;
		while( (cidx = actions.indexOf(idx, ',')) != -1 ){
			String action = actions.substring(idx, cidx).trim();
			
			Integer flag = (Integer)map.get(action);
			if( flag == null )
				throw new IllegalArgumentException( "Unknown ContentHandlerPermission action: '" + action + "'" );
			flags |= flag.intValue();
			
			idx = cidx + 1;
		}
		return flags;
	}
						
	public boolean equals(Object obj) {
		if( !(obj instanceof ContentHandlerPermission) )
			return false;
		return actionFlags == ((ContentHandlerPermission)obj).actionFlags;
	}

	public String getActions() {
		return actionStrings[ actionFlags ];
	}

	public int hashCode() {
		return actionFlags;
	}

	public boolean implies(Permission p) {
		if( !(p instanceof ContentHandlerPermission) )
			return false;
		return (((ContentHandlerPermission)p).actionFlags & ~actionFlags) == 0;
	}

}
