/*
 * @(#)AppletSecurityException.java	1.18 06/10/10
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

package sun.applet;

/**
 * An applet security exception.
 *
 * @version 	1.14, 08/19/02
 * @author 	Arthur van Hoff
 */
public
class AppletSecurityException extends SecurityException {
    private String key = null;
    private Object msgobj[] = null;
    public AppletSecurityException(String name) {
        super(name);
        this.key = name;
    }

    public AppletSecurityException(String name, String arg) {
        this(name);
        msgobj = new Object[1];
        msgobj[0] = (Object) arg;
    }

    public AppletSecurityException(String name, String arg1, String arg2) {
        this(name);
        msgobj = new Object[2];
        msgobj[0] = (Object) arg1;
        msgobj[1] = (Object) arg2;
    }

    public String getLocalizedMessage() {
        if (msgobj != null)
            return amh.getMessage(key, msgobj);
        else
            return amh.getMessage(key);
    }
    private static AppletMessageHandler amh = new AppletMessageHandler("appletsecurityexception");
}
