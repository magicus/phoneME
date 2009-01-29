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
import java.io.*;
import java.net.*;
import java.lang.reflect.*;
import sun.misc.*;

/*
 * A simple launcher class that loads the main test class
 * using the sun.misc.CDCAppClassLoader.
 */

public class Launcher {
    public static void main(String args[]) {
        int num = args.length;
        String classname = args[0];
        String mainArgs[] = new String[num - 1];
        for (int i = 0; i < num - 2; i++) {
            mainArgs[i] = args[i + 1];
        }

        Launcher launcher = new Launcher();
        launcher.startApp(classname, mainArgs);
    }

    public void startApp(String mainClassName, String mainArgs[]) {
         try {
             File path = new File("./");
             sun.misc.CDCAppClassLoader loader = new CDCAppClassLoader(
                          new URL[] {path.toURL()}, null);

	     Class [] args1 = {new String[0].getClass()};
	     Object [] args2 = {mainArgs};

	     Class mainClass = loader.loadClass(mainClassName);
	     Method mainMethod = mainClass.getMethod("main", args1);
	     mainMethod.invoke(null, args2);
	  } catch (InvocationTargetException i) {
             i.printStackTrace();
          } catch (Throwable e) {
             e.printStackTrace();
          }
    }
}
