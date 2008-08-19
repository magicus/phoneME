/*
 * 
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

/*
 * This is a standalone launcher that launches MIDP using
 * the MIDPImplementationClassLoader.
 */

package sun.misc;

import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;
import java.io.File;
import java.io.FileInputStream;
import java.util.StringTokenizer;
import java.util.jar.Manifest;
import java.util.jar.JarFile;
import java.util.jar.Attributes;
import java.util.Map;
import java.util.HashMap;
import java.util.Iterator;

public final class MIDPLauncher {
    static private String suitePath[] = new String[0];

    public static String[] getMidletSuitePath() {
        return suitePath;
    }

    /* Process the properties of a Manifest or JAD file. */
    private static void processSuiteProperties(Manifest manifest, HashMap midletProperties) {
        try {
            Attributes attributes = manifest.getMainAttributes();
            Iterator keys = attributes.keySet().iterator();
            while (keys.hasNext()) {
                Attributes.Name key = (Attributes.Name)keys.next();
                midletProperties.put(key.toString(), attributes.get(key).toString());
            }
        }
        catch (Exception e) {
            System.err.println("Can't read manifest file");
            e.printStackTrace();
        }
    }

    public static void main(String args[]) {
        int i, j, num;
        File midppath[] = new File[0];
        String midppathString = null;
        String suitepathString = null; 
        String jadpathString = null; 
        StringTokenizer components;
        String mainArgs[];
        int numMainArgs = args.length;
        HashMap midletProperties = null;

        /* Process arguments */
        for (i = 0; i < args.length; i++) {
            String arg = args[i];
            if (arg.equals("-midppath")) {
                midppathString = args[++i];
                components = new StringTokenizer(midppathString,
                    System.getProperty("path.separator", ":"));
                num = components.countTokens();
                midppath = new File[num];
                for (j = 0; j < num; j++) {
                    midppath[j] = new File(components.nextToken());
                }
                args[i-1] = args[i] = null;
                numMainArgs -= 2;
            } else if (arg.equals("-suitepath")) {
                suitepathString = args[++i];
                components = new StringTokenizer(suitepathString,
                    System.getProperty("path.separator", ":"));
                num = components.countTokens();
                suitePath = new String[num];
                for (j = 0; j < num; j++) {
                    suitePath[j] = components.nextToken();
                }
                args[i-1] = args[i] = null;
                numMainArgs -= 2;
            } else if (arg.equals("-jadpath")) {
                jadpathString = args[++i];
                args[i-1] = args[i] = null;
                numMainArgs -= 2;
            }
        }

        midletProperties = new HashMap();
        try {
            if (suitepathString != null) {
                JarFile jarFile = new JarFile(suitepathString);
                processSuiteProperties(jarFile.getManifest(), midletProperties);
            }

            if (jadpathString != null) {
                FileInputStream fis = new FileInputStream(jadpathString);
                processSuiteProperties(new Manifest(fis), midletProperties);
                fis.close();
            }
        }
        catch (Exception e) {
            System.err.println("Can't read manifest file");
            e.printStackTrace();
        }

        /* Prepare args for the MIDletSuiteLoader.main() */
        int k = 0;
        mainArgs = new String[numMainArgs];
        for (j = 0; j < args.length; j++) {
            if (args[j] != null) {
                mainArgs[k++] = args[j];
            }
        }

        /* Create MIDPImplementationClassLoader */
        MIDPImplementationClassLoader midpImplCL = 
            MIDPConfig.newMIDPImplementationClassLoader(midppath);

        /* Load MIDletSuiteLoader using MIDPImplementationClassLoader
         * and invoke it's main() method. */
        String loaderName = null;
        try {
            loaderName = System.getProperty(
                "com.sun.midp.mainClass.name",
                "com.sun.midp.main.CdcMIDletSuiteLoader");
            Class suiteloader = midpImplCL.loadClass(loaderName);
            Class loaderArgs[] = {mainArgs.getClass(), midletProperties.getClass()};
            Method launchMethod = suiteloader.getMethod("launchUninstalledSuite", loaderArgs);
            Object args2[] = {mainArgs, midletProperties};
            launchMethod.invoke(null, args2);
        } catch (ClassNotFoundException ce) {
            System.err.println("Can't find " + loaderName);
        } catch (NoSuchMethodException ne) {
            System.err.println("Can't access MIDletSuiteLoader main()");
        } catch (IllegalAccessException ie) {
            System.err.println("Can't invoke MIDletSuiteLoader main()");
        } catch (InvocationTargetException ite) {
            ite.printStackTrace();
        }
    }
}

