/*
 * @(#)MIDletClassLoader.java   1.11 06/10/10
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
/*
 * @(#)MIDletClassLoader.java   1.5     03/07/09
 *
 * Class loader for midlets running on CDC/PP
 *
 * It loads from a single JAR file and deligates to other class loaders. 
 * It has a few of interesting properties:
 * 1) it is careful, perhaps overly so, about not loading classes that
 *    are in system packages from the JAR file persumed to contain the MIDlet
 *    code.
 * 2) it uses a MemberFilter to process classes for illegal field references.
 *    This is easiest to do after the constant pool is set up.
 * 3) it remembers which classes have failed the above test and refuses to
 *    load them, even though the system thinks they're already loaded.
 *
 * It lets the underlying URLClassLoader do all the hard work.
 *    
 */
package sun.misc;

import java.net.URL;
import java.net.URLConnection;
import java.net.URLClassLoader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashSet;
import java.security.CodeSource;
import java.security.PermissionCollection;
import java.security.AccessController;
import java.security.AccessControlContext;
import java.security.PrivilegedAction;


public class MIDletClassLoader extends URLClassLoader {

    URL     myBase[];
    String[]systemPkgs;
    private MemberFilter memberChecker; /* to check for amputated members */
    private PermissionCollection perms;
    private HashSet badMidletClassnames = new HashSet();
    private MIDPImplementationClassLoader implementationClassLoader;
    private AccessControlContext ac = AccessController.getContext();
    /*
     * If the filter is disabled, all classes on the bootclasspath
     * can be accessed from midlet.
     */
    private boolean enableFilter;
    private ClassLoader auxClassLoader;

    private MIDPBridgeInterface bridgeInterface;

    public MIDletClassLoader(
        URL base[],
        String systemPkgs[],
        PermissionCollection pc,
        MemberFilter mf,
        MIDPImplementationClassLoader  parent,
        boolean enableFilter,
        ClassLoader auxClassLoader,
        MIDPBridgeInterface bridgeInterface)
    {
        super(base, parent);
        myBase = base;
        this.systemPkgs = systemPkgs;
        memberChecker = mf;
        perms = pc;
        implementationClassLoader = parent;
        this.enableFilter = enableFilter;
        this.auxClassLoader = auxClassLoader;
        this.bridgeInterface = bridgeInterface;

        /* Register the classloader */
        String p = base[0].getPath();
        int idx = p.lastIndexOf('.');
        if (idx != -1) {
            String name = p.substring(
                p.lastIndexOf(File.separatorChar)+1, idx);
            CVM.Preloader.registerClassLoader(name, this);
        }
    }

    protected PermissionCollection getPermissions(CodeSource cs){
        URL srcLocation = cs.getLocation();
        for (int i=0; i<myBase.length; i++){
            if (srcLocation.equals(myBase[i])){
                return perms;
            }
        }
        return super.getPermissions(cs);
    }

    /* Check if class belongs to restricted system packages. */
    private boolean
    packageCheck(String pkg) {
        String forbidden[] = systemPkgs;
        int fLength = forbidden.length;

        /* First check the default list specified by MIDPConfig */
        for (int i=0; i< fLength; i++){
            if (pkg.startsWith(forbidden[i])){
                return true;
            }
        }

        /* Then Check with MIDPPkgChecker. The MIDPPkgChecker knows
         * the restricted MIDP and JSR packages specified in their
         * rom.conf files. */
        return MIDPPkgChecker.checkPackage(pkg);
    }

    private Class
    loadFromUrl(String classname) throws ClassNotFoundException
    {
        Class newClass;
        try {
            newClass = super.findClass(classname); // call URLClassLoader
        }catch(Exception e){
            /*DEBUG e.printStackTrace(); */
            // didn't find it.
            return null;
        }
        if (newClass == null )
            return null;

        /*
         * Found the requested class. Make sure it's not from
         * restricted system packages.
         */
        int idx = classname.lastIndexOf('.');
        if (idx != -1) {
            String pkg = classname.substring(0, idx);
            if (packageCheck(pkg)) {
                throw new ClassNotFoundException(classname +
                              ". Prohibited package name: " + pkg);
            }
        }

        /*
         * Check member access to make sure the class doesn't
         * access any hidden CDC APIs.
         */
        try {
            // memberChecker will throw an Error if it doesn't like
            // the class.
            if (enableFilter) {
                memberChecker.checkMemberAccessValidity(newClass);
            }
            return newClass;
        } catch (Error e){
            // If this happens, act as if we cannot find the class.
            // remember this class, too. If the MIDlet catches the
            // Exception and tries again, we don't want findLoadedClass()
            // to return it!!
            badMidletClassnames.add(classname);
            throw new ClassNotFoundException(e.getMessage());
        }
    }


    public synchronized Class
    loadClass(String classname, boolean resolve) throws ClassNotFoundException
    {
        Class resultClass;
        Throwable err = null;
        int i = classname.lastIndexOf('.');

        if (i != -1) {
            SecurityManager sm = System.getSecurityManager();
            if (sm != null) {
                sm.checkPackageAccess(classname.substring(0, i));
            }
        }

        classname = classname.intern();

        if (badMidletClassnames.contains(classname)){
            // the system thinks it successfully loaded this class.
            // But the member checker does not think we should be able
            // to use it. We threw an Exception before. Do it again.
            throw new ClassNotFoundException(classname.concat(
                        " contains illegal member reference"));
        }

        /* First, check to see if the class has already been loaded. */
        resultClass = findLoadedClass(classname);

        /* Class has not been loaded. Delegate to the parent classloader. */
        if (resultClass == null){
            try {
                /* Ask the parent to load the class for us. But first
                 * make sure the class is allowed to be accessed by
                 * midlet.
                 */
                if (!enableFilter ||
                    MIDPConfig.isClassAllowed(classname)) {
                    resultClass = implementationClassLoader.loadClass(
                                      classname, false);
                }
            } catch (ClassNotFoundException e) {
            } catch (NoClassDefFoundError e) {
            }
        }

        /* Try loading the class ourselves. */
        if (resultClass == null) {
            try {
                resultClass = loadFromUrl(classname);
            } catch (ClassNotFoundException e) {
                err = e;
            } catch (NoClassDefFoundError e) {
                err = e;
            }
        }

        /*
         * If MIDletClassLoader and the parents failed to
         * load the class, try the auxClassLoader.
         */
        if (resultClass == null && auxClassLoader != null) {
            resultClass = auxClassLoader.loadClass(classname);
        }

        if (resultClass == null && bridgeInterface != null) {
            resultClass = bridgeInterface.loadClass(classname);
        }

        if (resultClass == null) {
            if (err == null) {
                throw new ClassNotFoundException(classname);
            } else {
                if (err instanceof ClassNotFoundException) {
                    throw (ClassNotFoundException)err;
                } else {
                    throw (NoClassDefFoundError)err;
                }
            }
        }

        if (resolve) {
            resolveClass(resultClass);
        }

        return resultClass;
    }

   public InputStream
   getResourceAsStream(String name){
        // prohibit reading .class as a resource
        if (name.endsWith(".class")){
            return null; // not allowed!
        }
        
        int i;
        // Replace /./ with /
        while ((i = name.indexOf("/./")) >= 0) {
            name = name.substring(0, i) + name.substring(i + 2);
        }
        // Replace /segment/../ with /
        i = 0;
        int limit;
        while ((i = name.indexOf("/../", i)) > 0) {
            if ((limit = name.lastIndexOf('/', i - 1)) >= 0) {
                name = name.substring(0, limit) + name.substring(i + 3);
                i = 0;
            } else {
                i = i + 3;
            }
        }

        // The JAR reader cannot find the resource if the name starts with 
        // a slash.  So we remove the leading slash if one exists.
        if (name.startsWith("/") || name.startsWith(File.separator)) {      
            name = name.substring(1);
        }
        
        final String n = name;
        // do not delegate. We only use our own URLClassLoader.findResource to
        // look in our own JAR file. That is always allowed.
        // Nothing else is.
        InputStream retval;
        retval = (InputStream) AccessController.doPrivileged(
                new PrivilegedAction(){
                    public Object run(){
                        URL url = findResource(n);
                        try {
                            return url != null ? url.openStream() : null;
                        } catch (IOException e) {
                            return null;
                        }
                    }
                }, ac);
        return retval;
    }

}

