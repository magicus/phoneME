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

public class KVMDebugProxy {

    Options options = null;

    public KVMDebugProxy() {
    }

    public boolean parseArgs( String args[] ) {
        int i = 0;

        options = new Options();

        options.setLocalPort(2801);
        options.setRemotePort(2800);
        options.setRemoteHost("localhost");
        options.setVerbosity(0);
        options.setClassPath("./");
        options.setProxyMode(false);

        if (args.length == 0) 
            options = null;

        try {
            while (i < args.length) {
                if ("-l".equals(args[i])) {
                    options.setLocalPort(Integer.parseInt(args[++i])); 
                } else if ("-r".equals(args[i])) {
                    options.setRemoteHost(args[++i]);  
                    options.setRemotePort(Integer.parseInt(args[++i]));  
                } else if ("-v".equals( args[ i ] ) ) {
                    options.setVerbosity(Integer.parseInt(args[++i]));  
                } else if ("-cp".equals(args[i])||
                            "-classpath".equals(args[i])) {
                    options.setClassPath(args[++i]);
                } else if ("-p".equals(args[i])) {
                    options.setProxyMode(true);
                } else if ("-m".equals(args[i])) {
                    options.setUseMVM(true);
                } else if ("-nb4".equals(args[i])) {
                    options.setNetbeans40compat(true);
                }
                i++;
            }
        } catch ( Exception e ) {
            options = null;
        }

        return options != null;
    }

    public void help() { 
        System.out.println( "Java ME Debug Agent." );
        System.out.println();
        System.out.println( "Usage: KVMDebugProxy -l <localport> -r <remotehost> <remoteport> [-p]");
        System.out.println("        [-v <level>] [-cp | - classpath <classpath" + File.pathSeparator + "classpath...>]" );
        System.out.println("Where:");
        System.out.println("  -l <localport> specifies the local port number that the debug agent will");
        System.out.println("     listen on for a connection from a debugger.");
        System.out.println("  -r <remotehost> <remoteport> is the hostname and port number that the debug");
        System.out.println("     agent will use to connect to the Java VM running the application");
        System.out.println("     being debugged");
        System.out.println("  -p enables proxy mode, the debug agent processes classfiles on behalf of the Java VM ");
        System.out.println("     running the application being debugged.  If not present, all commands");
        System.out.println("     from the debugger are passed down to the Java VM running the application");
        System.out.println("  -v <level> turns on verbose mode.  'level' specifies the amount of output,");
        System.out.println("     the larger the number the more output that is generated.");
        System.out.println("     'level' can be from 1-9.");
        System.out.println("  -m Enable support for Multitasking VM.");
        System.out.println("  -nb4 Enable use of Netbeans 4.x with older VMs (version < CLDC_VM 1.1.3)");
        System.out.println("  -cp or -classpath specifies a list of paths separated by " + File.pathSeparator + " where the");
        System.out.println("     debug agent can find copies of the class files.  Only needed if -p is set.");
        System.out.println();
    }

    public void go() {
        ServerSocket serverSocket = null;

        if ( options == null ) {
            return;
        }

        Log.SET_LOG(options.getVerbosity());
        try {
            serverSocket = new ServerSocket(options.getLocalPort());
            Log.LOGN(3, "KVMDebugProxy: opened server socket " + serverSocket);
        } catch (IOException e) {
            System.out.println("Caught exception " + e +
                               " while creating listener socket");
            return;
        }

        do {
            try {

            ClassManager manager =
                new ClassManager(new SearchPath(options.getClassPath()));

            /* The JDI code will now be trying to send handshaking information.
             * We must just pass this string over to the kvm. The kvm will then
             * send the same string back, which we must then pass to the 
             * debugger
             */

            // Two threads are required here so that we can listen to 
            // the debugger and the kvm at the same time. Each thread
            // knows how to talk to the other so that information can flow
            // over the proxy.
            //

            DebuggerListener dlisten = new DebuggerListener(serverSocket,
                                                            options);

            KVMListener klisten = new KVMListener();

            dlisten.set(klisten, manager);

            klisten.set(dlisten, manager);

            new Thread(klisten).start();

            dlisten.verbose(options.getVerbosity());
            klisten.verbose(options.getVerbosity());

            dlisten.connectToDebugger();

            //
            // At this point we have successfully connected the debugger 
            // through this proxy to the kvm. we can now sit back and
             // wait for packets to start flowing.
             //
            new Thread(dlisten).start();

        } catch (SecurityException e) {
            System.out.println("KVMDebugProxy: " + e.getMessage());
        }
        } while (options.getUseMVM());
    }

    public static void main( String args[] ) {

        KVMDebugProxy kdp = new KVMDebugProxy();

        if ( !kdp.parseArgs( args ) ) {
            kdp.help();
            System.exit( 1 ); 
        }

        kdp.go();
    }

} // KVMDebugProxy
