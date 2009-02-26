/*
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

/*
 * A classloader that restricts access to any of the class in
 * the JSRRestrictedClasses.txt configuration file. 
 */

package sun.misc;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.HashSet;

public class CDCAppClassLoader extends URLClassLoader {
    // Classes that are not allowed to be accessed by CDC apps
    private static HashSet restrictedClasses = new HashSet();

    static {
        getRestrictedClasses();
    }

    private static boolean getRestrictedClasses() {
        String filename = System.getProperty("java.home") +
                File.separator + "lib" + File.separator +
                "JSRRestrictedClasses.txt";
        BufferedReader in;

        try {
            in = new BufferedReader(new FileReader(filename));
        } catch(IOException e) {
            System.err.println("Could not open "+filename);
            return false;
        }

        try {
            while (true) {
                String s = in.readLine();
                if (s == null) {
                    break; // eof
                }
                if (s.length() == 0) {
                    continue;
	        }
                if (s.charAt(0) == '#') {
                    continue; // comment
                }
                // it's a classname, add it to the restricted set
                restrictedClasses.add(s);
	    }
            in.close();
	} catch (IOException e) {
            System.err.println("Failed to read " + filename);
            return false;
        }
        return true;
    }

    public CDCAppClassLoader(URL[] urls, ClassLoader parent) {
        super(urls, parent);
    }

    public Class loadClass(String name)
        throws ClassNotFoundException
    {
        // First check if the class is restricted
        if (restrictedClasses.contains(name)) {
            throw new ClassNotFoundException(name);
	}

        return super.loadClass(name);
    }
    
}
