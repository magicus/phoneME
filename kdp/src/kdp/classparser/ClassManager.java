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

package kdp.classparser;

import java.io.*;
import java.util.*;
import kdp.Log;
import kdp.VMConstants;

public class ClassManager {

    SearchPath path;
    public Map classMap = new HashMap(128, 0.75f);

    public ClassManager() {
        path = null;
    }

    public ClassManager( SearchPath path ) {
        this();
        this.path = path;
    }

    private ClassFile loadClass(String className, 
                                FileReference file, 
                                byte typetag) {

        Log.LOGN(3, "loadclass: " + file );
        
        ClassFile cf = new ClassFile(file, className, typetag);

        try { 
            cf.readClassFile();
        }
        catch ( Exception e ) { 
            Log.LOGN(2,  "Error loading: " + file );
            cf = null;
        }

        return cf; 
    }

    public ClassFile findClass(byte typeTag, String className) {

        ClassFile cf=null;

        Collection cVals = classMap.values();
        Iterator cIter = cVals.iterator();
        while (cIter.hasNext()) {
            cf = (ClassFile)cIter.next();
            if (cf.equals(className)) {
                return cf;
            }
        }
        return null;
    }

    public ClassFile findClass(int cid, String className,
                               byte typetag, int status ) {
        ClassFile cf=null;

        if ((cf = (ClassFile)classMap.get(new Integer(cid))) != null) {
            cf.setClassStatus(status);
            return cf;
        }

        if (typetag == VMConstants.TYPE_TAG_ARRAY) {
            Log.LOGN(4,  "findclass: Array class " + className );
            cf = new ClassFile(null, className, typetag);
            classMap.put(new Integer(cid), cf);
            cf.setClassID(cid);
            cf.setClassStatus(status);
            return cf;
        }
        if ( path != null ) {
            FileReference file;

            Log.LOGN(4,  "findclass: finding " + className );
            if ( ( file = path.resolve( className ) ) != null ) {
                cf = loadClass(className, file, typetag);
                if (cf != null) {
                    classMap.put(new Integer(cid), cf);
                    cf.setClassID(cid);
                    cf.setClassStatus(status);
                    return cf;
                }
            } 
        }
        return null;
    }
} // ClassManager
