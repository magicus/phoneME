/*
 *   
 *
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

package com.sun.midp.jadtool;

import java.io.*;
import java.util.*;
import java.security.cert.X509Certificate;

/**
 * The JadTool is a command line interface to the AppDescriptor class.
 *
 * @see AppDescriptor
 * @see SignCert
 * @see java.security.KeyStore
 */
public class JadTool {
    /** Usage text. */
    private static String USAGE = 
        "\nJadTool arguments:\n" +
        "-help\n" +
        "-addcert\n" +
        "\t-alias <key alias> [-storepass <password>] " +
        "[-keystore <keystore>]\n" +
        "\t[-certnum <number>] [-chainnum <number>]\n" +
        "\t[-encoding <encoding>] -inputjad <filename> " +
        "-outputjad <filename>\n" +
        "-addjarsig\n" +
        "\t[-jarfile <filename>] -keypass <password> -alias <key alias>\n" +
        "\t-storepass <password> [-keystore <keystore>] " +
        "[-encoding <encoding>]\n" +
        "\t-inputjad <filename> -outputjad <filename>\n" +
        "-showcert\n" +
        "\t[([-certnum <number>] [-chainnum <number>]) | [-all]]\n" +
        "\t[-encoding <encoding>] -inputjad <filename>\n" +
        "\n" +
        "The default for -encoding is UTF-8.\n" +
        "The default for -jarfile is the MIDlet-Jar-URL " +
        "property in the JAD.\n" +
        "The default for -keystore is \"$HOME/.keystore\".\n" +
        "The default for -certnum is 1.\n" +
        "The default for -chainnum is 1.\n";

    /** Holds the command given on the command line. */
    private String command = null;
    /** Holds the JAD encoding given on the command line. */
    private String encoding = null;
    /** Holds the input JAD filename given on the command line. */
    private String infile = null;
    /** Holds the output JAD filename given on the command line. */
    private String outfile = null;
    /** Holds the JAR filename given on the command line. */
    private String jarfile = null;
    /** Holds the keystore filename given on the command line. */
    private String keystore = null;
    /** Holds the key alias given on the command line. */
    private String alias = null;
    /** Holds the certificate number given on the command line. */
    private String certnum = null;
    /** Holds the certificate chain number given on the command line. */
    private String chainNum = null;
    /** Holds the keystore password given on the command line. */
    private char[] storepass = null;
    /** Holds the private key password given on the command line. */
    private char[] keypass = null;

    /** The converted certificate number. */
    private int certIndex = 1;
    /** The converted certificate chain number. */
    private int chainIndex = 1;
    /** The in-memory JAD. */
    private AppDescriptor appdesc = null;
    /** Stream to the output JAD. */
    private OutputStream outstream;

    /** Keeps this class from being instantiated by the public. */
    private JadTool() { }

    /**
     * Performs the command specified in the command line arguments.
     * <p>
     * Exits with a 0 status if the command was successful.
     * Exits and prints out an error message with a -1 status if the command
     * failed.</p>
     * <pre>
     * Command Summary
     * ---------------
     *
     * -help         - print a usage summary
     *
     * -addjarsig    - add a SHA1withRSA PKCS v1.5 signature of a jarfile to an
     *                 app descriptor file.
     *
     * -addcert      - add a https or content provider certificate
     *                 to an app descriptor file.
     *
     * -showcert     - show a certificate from the app descriptor 
     *                 file in human readable form.
     *
     * Options 
     * -------
     *
     * These options are valid (and in some cases required to be use) with
     * the commands above.
     *
     * -inputjad      <original app descriptor file> 
     *
     * -outputjad     <output app descriptor file, can be the same as infile> 
     *
     * -encoding   <encoding type> 
     *             (default is UTF-8)
     *
     * -jarfile    <jar file>
     *
     * -keystore   <keystore file to use>
     *             (default is .keystore - same as with keytool)
     *
     * -storepass  <password to unlock chosen keystore>
     *
     * -alias      <alias of  key or certificate in keystore>
     *
     * -keypass    <password to unlock signing key>
     *
     * -certnum    <number> (default is "1" or calculated for addcert)
     *
     * -chainnum   <number> (default is "1")
     *
     * -all        show all certificates
     *
     *
     * Command Descriptions
     * --------------------
     * help:          Shows a list of use options.
     *
     * addcert:       Given a certificate and an unsigned jad file
     *                JadTool will create a base64 encoding of the 
     *                certificate file and include it in the jad file as  
     *
     *                   MIDlet-Certificate-1-1: <base64 string> 
     *                                
     *                "base64 string" is a base64 encoding of the certificate. 
     *                If the certificate already exists in the jad file
     *                it will not be duplicated.
     *
     * showcert:      Usage: java JadTool -showcert -certnum <num>
     *                Allows choice of certificate to view using this command.
     *
     * addjarsig:     Given the alias of a key (stored by the J2SE "keytool"
     *                utility" and an unsigned jad file, the JadTool will
     *                check to see that the jad file contains a 
     *                a  MIDlet-Certificate-1-1
     *                field.  It will then sign the jar file.
     *                In the event that no target jar file is specified
     *                JadTool will hash the jar file located at the URL 
     *                specified by the MIDlet-Jar-URL property if no 
     *                explicit jar file is given.
     *</pre>
     *
     * @param args command line arguments given by user.
     */
    public static void main(String[] args) {
        int exitStatus = -1;

        try {
            new JadTool().run(args);
            exitStatus = 0;
        } catch (Exception e) {
            System.err.println("\n" + e.getMessage() + "\n");
        }

        System.exit(exitStatus);
    }
    
    /**
     * Parses the command line arguments and runs the selected command. 
     * <p>
     * Does not return.
     * Exits with a 0 status if the command was successful.
     * Exits and prints out an error message with a -1 status if the command
     * failed.</p>
     *
     * @param args The command line arguments given to main.
     *
     * @exception Exception if there are any errors
     */
    private void run(String [] args) throws Exception {
        if (args.length == 0) {
            usageError("No command given");
        }

        command = args[0];

        try {
            if (command.equals("-addjarsig")) {
                performAddJarSigCommand(args);
                return;
            }

            if (command.equals("-addcert")) {
                performAddCertCommand(args);
                return;
            }

            if (command.equals("-showcert")) {
                performShowCertCommand(args);
                return;
            }

            if (command.equals("-help")) {
                for (int i = 1; i < args.length; i++) {
                    usageError("Illegal option for " + command + ": " +
                               args[i]);
                }

                // help exits
                help();
            }

            usageError("Illegal command: " + command);
        } finally {
            // zero-out passwords
            if (storepass != null) {
                Arrays.fill(storepass, ' ');
                storepass = null;
            }

            if (keypass != null) {
                Arrays.fill(keypass, ' ');
                keypass = null;
            }

            try {
                if (outstream != null) {
                    outstream.close();
                }
            } catch (IOException ioe) {
                // do nothing.  
            }
        }
    }
    
    /**
     * Perform the -addjarsig command, including parsing the line arguments
     * for the -addjarsig command.
     * <p>
     * If there is a problem parsing an argument, print the error,
     * print the usage, and exit with a -1.
     *
     * @param args The command line arguments given to main.
     *
     * @exception Exception if there are any errors
     */
    private void performAddJarSigCommand(String[] args) throws Exception {
        int i = 1;

        try {
            for (i = 1; i < args.length; i++) {

                if (args[i].equals("-encoding")) {
                    encoding = args[++i];
                } else if (args[i].equals("-keystore")) {
                    keystore = args[++i];
                } else if (args[i].equals("-storepass")) {
                    storepass = args[++i].toCharArray();
                } else if (args[i].equals("-keypass")) {
                    keypass = args[++i].toCharArray();
                } else if (args[i].equals("-alias")) {
                    alias = args[++i];
                } else if (args[i].equals("-jarfile")) {
                    jarfile = args[++i];
                } else if (args[i].equals("-inputjad")) {
                    infile = args[++i];
                } else if (args[i].equals("-outputjad")) {
                    outfile = args[++i];
                } else {
                    usageError("Illegal option for " + command +
                               ": " + args[i]);
                }
            }
        } catch (ArrayIndexOutOfBoundsException aiobe) {
            usageError("Missing value for " + args[--i]);
        }

        if (keypass == null) {
            usageError(command + " requires -keypass");
        }

        // these methods will check for the presence of the args they need
        initJadUtil();
        openKeystoreAndOutputJad();

        if (jarfile != null) {
            // a jar file was specified for use
            FileInputStream jarinput;

            try {
                jarinput = new FileInputStream(jarfile);
            } catch (FileNotFoundException fnfe) {
                throw new Exception("JAR does not exist: " + jarfile);
            }

            try {
                appdesc.addJarSignature(alias, keypass, jarinput);
            } catch (Exception e) {
                throw new Exception(command + " failed: " + e.toString());
            }

            try {
                jarinput.close();
            } catch (Exception e) {
                // ignore
            }
        } else {
            // Use the JAR at MIDlet-Jar-URL in the JAD
            try {
                appdesc.addJarSignature(alias, keypass);
            } catch (Exception e) {
                throw new Exception(command + " failed: " + e.toString());
            }
        }

        appdesc.store(outstream, encoding);
        return;
    }

    /**
     * Perform the -addcert command, including parsing the line arguments
     * for the -addcert command.
     * <p>
     * If there is a problem parsing an argument, print the error,
     * print the usage, and exit with a -1.
     *
     * @param args The command line arguments given to main.
     *
     * @exception Exception if there are any errors
     */
    private void performAddCertCommand(String[] args) throws Exception {
        int i = 1;

        // change the default for cert number for this command
        certIndex = 0;

        try {
            for (i = 1; i < args.length; i++) {

                if (args[i].equals("-encoding")) {
                    encoding = args[++i];
                } else if (args[i].equals("-keystore")) {
                    keystore = args[++i];
                } else if (args[i].equals("-storepass")) {
                    storepass = args[++i].toCharArray();
                } else if (args[i].equals("-alias")) {
                    alias = args[++i];
                } else if (args[i].equals("-certnum")) {
                    certnum = args[++i];
                } else if (args[i].equals("-chainnum")) {
                    chainNum = args[++i];
                } else if (args[i].equals("-inputjad")) {
                    infile = args[++i];
                } else if (args[i].equals("-outputjad")) {
                    outfile = args[++i];
                } else  {
                    usageError("Illegal option for " + command +
                               ": " + args[i]);
                }
            }
        } catch (ArrayIndexOutOfBoundsException aiobe) {
            usageError("Missing value for " + args[--i]);
        }

        // these methods will check for the presence of the args they need
        checkCertAndChainNum();
        initJadUtil();
        openKeystoreAndOutputJad();

        try {
            appdesc.addCert(alias, chainIndex, certIndex);
            appdesc.store(outstream, encoding);
            return;
        } catch (Exception e) {
            throw new Exception(command + " failed: " + e.toString());
        }
    }

    /**
     * Perform the -showcert command, including parsing the line arguments
     * for the -showcert command.
     * <p>
     * If there is a problem parsing an argument, print the error,
     * print the usage, and exit with a -1.
     *
     * @param args The command line arguments given to main.
     *
     * @exception Exception if there are any errors
     */
    private void performShowCertCommand(String[] args) throws Exception {
        int i = 1;
        X509Certificate c;
        boolean listAll = false;

        try {
            for (i = 1; i < args.length; i++) {

                if (args[i].equals("-encoding")) {
                    encoding = args[++i];
                } else if (args[i].equals("-certnum")) {
                    certnum = args[++i];
                } else if (args[i].equals("-chainnum")) {
                    chainNum = args[++i];
                } else if (args[i].equals("-all")) {
                    listAll = true;
                } else if (args[i].equals("-inputjad")) {
                    infile = args[++i];
                } else {
                    usageError("Illegal option for " + command +
                               ": " + args[i]);
                }
            }
        } catch (ArrayIndexOutOfBoundsException aiobe) {
            usageError("Missing value for " + args[--i]);
        }

        if (listAll && (chainNum != null || certnum != null)) {
            usageError("-all cannot be used with -certnum or -chainnum");
        }

        // these methods will check for the presence of the args they need
        checkCertAndChainNum();
        initJadUtil();

        if (listAll) {
            Vector certs = appdesc.getAllCerts();

            if (certs.size() == 0) {
                System.out.println("\nNo certificates found in JAD.\n");
                return;
            }

            System.out.println();

            for (i = 0; i < certs.size(); i++) {
                Object[] temp = (Object[])certs.elementAt(i);

                System.out.println((String)temp[AppDescriptor.KEY] + ":");

                displayCert((X509Certificate)temp[AppDescriptor.CERT]);
            }

            return;
        }

        try {
            c = appdesc.getCert(chainIndex, certIndex);
        } catch (Exception e) {
            throw new Exception("-showcert failed: " + e.toString());
        }

        if (c == null) {
            throw new Exception("Certificate " + chainIndex + "-" +
                                certIndex + " not in JAD");
        }

        try {
            displayCert(c);
            return;
        } catch (Exception e) {
            throw new Exception("-showcert failed: " + e.toString());
        }
    }

    /**
     * Check the format of the certificate and chain numbers. If there is a
     * problem, print the error, print usage, and exit with -1.
     */
    private void checkCertAndChainNum() {
        if (certnum != null) {
            try {
                certIndex = (Integer.valueOf(certnum)).intValue();
                if (certIndex <= 0) {
                    usageError("-certnum must be a positive number");
                }
            } catch (NumberFormatException nfe) {
                usageError("-certnum must be a positive number");
            }
        } 

        if (chainNum != null) {
            try {
                chainIndex = (Integer.valueOf(chainNum)).intValue();
                if (chainIndex <= 0) {
                    usageError("-chainnum must be a positive number");
                }
            } catch (NumberFormatException nfe) {
                usageError("-chainnum must be a positive number");
            }
        }
    }

    /**
     * Initializes an instance of the AppDescriptor class.
     * <p>
     * If the input file has not been specified, print an error and the usage,
     * then exit with a -1
     *
     * @exception Exception if there are any errors
     */
    private void initJadUtil() throws Exception {
        InputStream instream;

        if (infile == null) {
            usageError(command + " requires an input JAD");
        }

        try {
            FileInputStream fis = new FileInputStream(infile);
            instream = new BufferedInputStream(fis);
        } catch (FileNotFoundException fnfe) {
            throw new Exception("Input JAD does not exist: " + infile);
        }

        try {
            appdesc = new AppDescriptor();
            appdesc.load(instream, encoding);
        } catch (UnsupportedEncodingException uee) {
            throw new Exception("Encoding type " + encoding +
                                " not supported");
        } catch (IOException ioe) {
            throw new Exception("Error parsing input JAD: " + infile);
        } finally {
            try {
                // close now so the input and output JAD can be the same.
                instream.close();
            } catch (Exception e) {
                // ignore
            }
        }
    }

    /**
     * Open the keystore and output JAD file.
     * <p>
     * If the key alias or output file has not been specified, print an
     * error and the usage, then exit with a -1
     *
     * @exception Exception if there are any errors
     */
    private void openKeystoreAndOutputJad() throws Exception {
        File ksfile;
        FileInputStream ksstream;

        if (alias == null) {
            usageError(command + " requires -alias");
        }

        if (outfile == null) {
            usageError(command + " requires an output JAD");
        }

        if (keystore == null) {
            keystore = System.getProperty("user.home") + File.separator
                       + ".keystore";
        }

        try {
            ksfile = new File(keystore);
            // Check if keystore file is empty
            if (ksfile.exists() && ksfile.length() == 0) {
                throw new Exception("Keystore exists, but is empty: " +
                                    keystore);
            }

            ksstream = new FileInputStream(ksfile);
        } catch (FileNotFoundException fnfe) {
            throw new Exception("Keystore does not exist: " + keystore);
        }

        try {
            try {
                // the stream will be closed later
                outstream = new FileOutputStream(outfile);
            } catch (IOException ioe) {
                throw new Exception("Error opening output JAD: " +
                                    outfile);
            }

            try {
                // load the keystore into the AppDescriptor
                appdesc.loadKeyStore(ksstream, storepass);
            } catch (Exception e) {
                throw new Exception("Keystore could not be loaded: " +
                                    e.toString());
            }
        } finally {
            try {
                ksstream.close();
            } catch (IOException e) {
                // ignore
            }
        }
    }
    
    /**
     * Prints the usage cases of this tool and exits with 0 status.
     */
    private void help() {
        usage(0);
    }

    /**
     * Prints the usage cases of this tool and exits with -1 status.
     *
     * @param error usage error message to print
     */
    private void usageError(String error) {
        System.err.println("\n" + error);
        usage(-1);
    }

    /**
     * Prints the usage cases of this tool and exits.
     *
     * @param exitStatus status to exit with
     */
    private void usage(int exitStatus) {
        System.out.println(USAGE);
        System.exit(exitStatus);
    }

    /**
     * Display a certificate.
     *
     * @param c certificate to display
     *
     * @exception Exception if there are any errors
     */
    private void displayCert(X509Certificate c) throws Exception {
        String digest;

        System.out.println();

        System.out.println("Subject: " + c.getSubjectDN().getName());

        System.out.println("Issuer : " + c.getIssuerDN().getName());

        System.out.println("Serial number: " +
                               c.getSerialNumber().toString(16));

        System.out.println("Valid from " + c.getNotBefore() +
                           " to " + c.getNotAfter());
            
        System.out.println("Certificate fingerprints:");

        System.out.print("  MD5: ");
        digest = AppDescriptor.createFingerprint(c.getEncoded(), "MD5");
        System.out.println(digest);

        System.out.print("  SHA: ");
        digest = AppDescriptor.createFingerprint(c.getEncoded(), "SHA");
        System.out.println(digest);

        System.out.println();
    }
}
