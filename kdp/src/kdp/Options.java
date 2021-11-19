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

package kdp;

import kdp.classparser.*;
import java.net.*;
import java.io.*;

class Options {

    private static int localport, remoteport;
    private static String remotehost;

    private static String classpath;
    private static int verbosity = 0;
    private static boolean proxyMode = false;
    private static boolean mvm = false;
    private static boolean nb40hack = false;

    public static void setLocalPort( int port ) { localport = port; }
    public static int  getLocalPort() { return localport; }

    public static void setRemoteHost( String host ) { remotehost = host; }
    public static String getRemoteHost() { return remotehost; }

    public static void setRemotePort( int port ) { remoteport = port; }
    public static int  getRemotePort() { return remoteport; }

    public static void setClassPath( String path ) { classpath = path; }
    public static String getClassPath() { return classpath; }

    public static void setVerbosity( int lvl ) { verbosity = lvl; }
    public static int getVerbosity() { return verbosity; }

    public static void setProxyMode( boolean on ) { proxyMode = on; }
    public static boolean getProxyMode() { return proxyMode; };

    public static void setUseMVM(boolean on) {mvm = on; }
    public static boolean getUseMVM() { return mvm; }

    public static void setNetbeans40compat(boolean on) { nb40hack = on; }
    public static boolean getNetbeans40compat() { return nb40hack;}


} // Options
