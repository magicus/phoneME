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
 */

package vm;

import consts.Const;

/**
 * Tracks the generic configuration settings for the target VM.
 */
public class VMConfig
{
    static String targetJDKVersion;
    static int minSupportedClassfileVersion;
    static int maxSupportedClassfileVersion;
    static int maxSupportedClassfileMinorVersion;
    
    static {
        minSupportedClassfileVersion = Const.JAVA_MIN_SUPPORTED_VERSION;
        maxSupportedClassfileVersion = Const.JAVA_MAX_SUPPORTED_VERSION;
        maxSupportedClassfileMinorVersion =
            Const.JAVA_MAX_SUPPORTED_MINOR_VERSION;
        targetJDKVersion = "1." + (maxSupportedClassfileVersion - 44);
    }
    public static boolean setJDKVersion(String jdkVersion) {
        int newMaxVersion;
        String temp;

        // Check for the '1' in front:
        if (jdkVersion.charAt(0) != '1') {
            return false;
        }

        // Check for the 1st '.':
        int idxOfFirstDot = jdkVersion.indexOf('.');
        if (idxOfFirstDot < 0) {
            return false;
        }

        temp = jdkVersion.substring(idxOfFirstDot + 1);
        if (temp.equals("")) {
            return false;
        }

        // Check for the 2nd '.':
        int idxOfSecondDot = temp.indexOf('.');
        if (idxOfSecondDot > 0) {
            // Strip from the 2nd '.' onwwards:
            temp = temp.substring(0, idxOfSecondDot);
        }

        // Convert JDK major version to Classfile major version.
        try {
            newMaxVersion = Integer.parseInt(temp) + 44;
        } catch (NumberFormatException ex) {
            return false;
        }

        // Ensure that the new max version is within the supported range of
        // version numbers:
        if ((newMaxVersion < Const.JAVA_MIN_SUPPORTED_VERSION) ||
            (newMaxVersion > Const.JAVA_MAX_SUPPORTED_VERSION)) {
            return false;
        }

        /* Set the new max version number: */
        maxSupportedClassfileVersion = newMaxVersion;
        maxSupportedClassfileMinorVersion = 0;
        targetJDKVersion = jdkVersion;
        return true;
    }
    public static String getJDKVersion() {
        return targetJDKVersion;
    }

    public static int getMinSupportedClassfileVersion() {
        return minSupportedClassfileVersion;
    }
    public static int getMaxSupportedClassfileVersion() {
        return maxSupportedClassfileVersion;
    }
    public static int getMaxSupportedClassfileMinorVersion() {
        return maxSupportedClassfileMinorVersion;
    }

}
