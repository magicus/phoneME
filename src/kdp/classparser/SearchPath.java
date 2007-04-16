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

import kdp.Log;
import java.io.*;
import java.util.*;
import java.lang.*;

public class SearchPath {

    private String pathString;

    private String[] pathArray;

    public SearchPath(String searchPath) {
        //### Should check searchpath for well-formedness.
        StringTokenizer st = new StringTokenizer(searchPath, File.pathSeparator);
        List dlist = new ArrayList();
        while (st.hasMoreTokens()) {
            dlist.add(st.nextToken());
        }
        pathString = searchPath;
        Log.LOGN(3,  "!!!" + pathString );
        pathArray = (String[])dlist.toArray(new String[dlist.size()]);
        Log.LOGN(3, "Path array length is :"+pathArray.length);
        for(int i=0; i<pathArray.length; i++) {
            Log.LOGN(3, pathArray[i]);
        }
    }

    public boolean isEmpty() {
        return (pathArray.length == 0);
    }

    public String asString() {
        return pathString;
    }
    
    public String[] asArray() {
        return (String[])pathArray.clone();
    }
    
    public int path_array_length() {
        return pathArray.length;
    }

    public FileReference resolve(String relativeFileName) {
        Log.LOGN(4, "relative filename = " + relativeFileName);
        Log.LOGN(4, "path array length in resolve is " +pathArray.length);
        if (!relativeFileName.endsWith(".class"))
            relativeFileName += ".class";
        Log.LOGN(4, "relative filename now = " + relativeFileName);
        for (int i = 0; i < pathArray.length; i++) {
            //            Log.LOGN(3, "path array = " + pathArray[i]);
            //            Log.LOGN(3, "relativeFileName);
            Log.LOGN(5,  "pa=" + pathArray[ i ] + " " +
                     "rfa=" + relativeFileName );
            FileReference path = FileReference.create(pathArray[i], relativeFileName);
            if (path.exists()) {
                Log.LOGN(4,  "  exists" );
                return path;
            }
        }
        return null;
    }
    
    //### return List?

    public String[] children(String relativeDirName, FilenameFilter filter) {
        // If a file appears at the same relative path
        // with respect to multiple entries on the classpath,
        // the one corresponding to the earliest entry on the
        // classpath is retained.  This is the one that will be
        // found if we later do a 'resolve'.
        SortedSet s = new TreeSet();  // sorted, no duplicates
        for (int i = 0; i < pathArray.length; i++) {
            FileReference path = FileReference.create(pathArray[i], relativeDirName);
            if (path.exists()) {
                String[] childArray = path.list(filter);
                if (childArray != null) {
                    for (int j = 0; j < childArray.length; j++) {
                        if (!s.contains(childArray[j])) {
                            s.add(childArray[j]);
                        }
                    }
                }
            }
        }
        return (String[])s.toArray(new String[s.size()]);
    }

}
