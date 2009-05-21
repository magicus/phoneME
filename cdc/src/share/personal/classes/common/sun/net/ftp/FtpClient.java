/*
 * @(#)FtpClient.java	1.55 06/10/10
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

package sun.net.ftp;

import java.util.StringTokenizer;
import java.io.*;
import java.net.*;
import sun.net.TransferProtocolClient;
import sun.net.TelnetInputStream;
import sun.net.TelnetOutputStream;

/**
 * This class implements the FTP client.
 *
 * @version	1.49, 08/19/02
 * @author	Jonathan Payne
 */

public class FtpClient extends TransferProtocolClient {
    public static final int FTP_PORT = 21;
    static int	FTP_SUCCESS = 1;
    static int	FTP_TRY_AGAIN = 2;
    static int	FTP_ERROR = 3;
    /** socket for data transfer */
    private Socket	dataSocket = null;
    private boolean	replyPending = false;
    private boolean	binaryMode = false;
    /** user name for login */
    String		user = null;
    /** password for login */
    String		password = null;
    /** last command issued */
    String		command;
    /** The last reply code from the ftp daemon. */
    int			lastReplyCode;
    /** Welcome message from the server, if any. */
    public String	welcomeMsg;
    /* The following three data members are left in for binary */
    
    /* backwards-compatibility.  Unfortunately, HotJava sets them directly */
    
    /* when it wants to change the settings.  The new design has us not */
    
    /* cache these, so this is unnecessary, but eliminating the data members */
    
    /* would break HJB 1.1 under JDK 1.2. */
    
    /* */
    
    /* These data members are not used, and their values are meaningless. */
    
    /* TODO:  Take them out for JDK 2.0! */
    
    /**
     * @deprecated
     */
    public static boolean useFtpProxy;
    /**
     * @deprecated
     */
    public static String ftpProxyHost;
    /**
     * @deprecated
     */
    public static int ftpProxyPort;
    /* these methods are used to determine whether ftp urls are sent to */
    
    /* an http server instead of using a direct connection to the */
    
    /* host. They aren't used directly here. */
    
    /**
     * @return if the networking layer should send ftp connections through
     *		a proxy
     */
    public static boolean getUseFtpProxy() {
        // if the ftp.proxyHost is set, use it!
        return (getFtpProxyHost() != null);
    }

    /**
     * @return the host to use, or null if none has been specified
     */
    public static String getFtpProxyHost() {
        return (String) java.security.AccessController.doPrivileged(
                new java.security.PrivilegedAction() {
                    public Object run() {
                        String result = System.getProperty("ftp.proxyHost");
                        if (result == null) {
                            result = System.getProperty("ftpProxyHost");
                        }
                        if (result == null) {
                            // as a last resort we use the general one if ftp.useProxy
                            // is true
                            if (Boolean.getBoolean("ftp.useProxy")) {
                                result = System.getProperty("proxyHost");
                            }
                        }
                        return result;
                    }
                }
            );
    }

    /**
     * @return the proxy port to use.  Will default reasonably if not set.
     */
    public static int getFtpProxyPort() {
        final int result[] = {80};
        java.security.AccessController.doPrivileged(
            new java.security.PrivilegedAction() {
                public Object run() {
                    String tmp = System.getProperty("ftp.proxyPort");
                    if (tmp == null) {
                        // for compatibility with 1.0.2
                        tmp = System.getProperty("ftpProxyPort");
                    }
                    if (tmp == null) {
                        // as a last resort we use the general one if ftp.useProxy
                        // is true
                        if (Boolean.getBoolean("ftp.useProxy")) {
                            tmp = System.getProperty("proxyPort");
                        }
                    }
                    if (tmp != null) {
                        result[0] = Integer.parseInt(tmp);
                    }
                    return null;
                }
            }
        );
        return result[0];
    }

    /**
     * issue the QUIT command to the FTP server and close the connection.
     */
    public void closeServer() throws IOException {
        if (serverIsOpen()) {
            issueCommand("QUIT");
            super.closeServer();
        }
    }

    protected int issueCommand(String cmd) throws IOException {
        command = cmd;
        int reply;
        if (replyPending) {
            if (readReply() == FTP_ERROR)
                System.out.print("Error reading FTP pending reply\n");
        }
        replyPending = false;
        do {
            sendServer(cmd + "\r\n");
            reply = readReply();
        }
        while (reply == FTP_TRY_AGAIN);
        return reply;
    }

    protected void issueCommandCheck(String cmd) throws IOException {
        if (issueCommand(cmd) != FTP_SUCCESS)
            throw new FtpProtocolException(cmd);
    }

    protected int readReply() throws IOException {
        lastReplyCode = readServerResponse();
        switch (lastReplyCode / 100) {
        case 1:
            replyPending = true;

            /* falls into ... */

        case 2:
        case 3:
            return FTP_SUCCESS;

        case 5:
            if (lastReplyCode == 530) {
                if (user == null) {
                    throw new FtpLoginException("Not logged in");
                }
                return FTP_ERROR;
            }
            if (lastReplyCode == 550) {
                throw new FileNotFoundException(command + ": " + getResponseString());
            }
        }
        /* this statement is not reached */
        return FTP_ERROR;
    }

    protected Socket openDataConnection(String cmd) throws IOException {
        ServerSocket portSocket;
        String	    portCmd;
        InetAddress myAddress = InetAddress.getLocalHost();
        byte	    addr[] = myAddress.getAddress();
        int	    shift;
        IOException e;
        portSocket = new ServerSocket(0, 1);
        portCmd = "PORT ";
        /* append host addr */
        for (int i = 0; i < addr.length; i++) {
            portCmd = portCmd + (addr[i] & 0xFF) + ",";
        }
        /* append port number */
        portCmd = portCmd + ((portSocket.getLocalPort() >>> 8) & 0xff) + ","
                + (portSocket.getLocalPort() & 0xff);
        if (issueCommand(portCmd) == FTP_ERROR) {
            e = new FtpProtocolException("PORT");
            portSocket.close();
            throw e;
        }
        if (issueCommand(cmd) == FTP_ERROR) {
            e = new FtpProtocolException(cmd);
            portSocket.close();
            throw e;
        }
        dataSocket = portSocket.accept();
        portSocket.close();
        return dataSocket;
    }

    /* public methods */

    /** open a FTP connection to host <i>host</i>. */
    public void openServer(String host) throws IOException {
        int port = FTP_PORT;
        /*
         String source = Firewall.verifyAccess(host, port);

         if (source != null) {
         Firewall.securityError("Applet at " +
         source +
         " tried to open FTP connection to "
         + host + ":" + port);
         return;
         }
         */
        openServer(host, port);
    }

    /** open a FTP connection to host <i>host</i> on port <i>port</i>. */
    public void openServer(String host, int port) throws IOException {
        /*
         String source = Firewall.verifyAccess(host, port);

         if (source != null) {
         Firewall.securityError("Applet at " +
         source +
         " tried to open FTP connection to "
         + host + ":" + port);
         return;
         }
         */
        super.openServer(host, port);
        if (readReply() == FTP_ERROR)
            throw new FtpProtocolException("Welcome message");
    }

    /**
     * login user to a host with username <i>user</i> and password
     * <i>password</i>
     */
    public void login(String user, String password) throws IOException {
        /* It shouldn't send a password unless it
         needs to. */

        if (!serverIsOpen())
            throw new FtpLoginException("not connected to host");
        this.user = user;
        this.password = password;
        if (issueCommand("USER " + user) == FTP_ERROR)
            throw new FtpLoginException("user");
        if (password != null && issueCommand("PASS " + password) == FTP_ERROR)
            throw new FtpLoginException("password");
            // keep the welcome message around so we can
            // put it in the resulting HTML page.
        String l;
        for (int i = 0; i < serverResponse.size(); i++) {
            l = (String) serverResponse.elementAt(i);
            if (l != null) {
                if (l.charAt(3) != '-') {
                    break;
                }
                // get rid of the "230-" prefix
                l = l.substring(4);
                if (welcomeMsg == null) {
                    welcomeMsg = l;
                } else {
                    welcomeMsg += l;
                }
            }
        }
    }

    /** GET a file from the FTP server */
    public TelnetInputStream get(String filename) throws IOException {
        Socket	s;
        try {
            s = openDataConnection("RETR " + filename);
        } catch (FileNotFoundException fileException) {
            /* Well, "/" might not be the file delimitor for this
             particular ftp server, so let's try a series of
             "cd" commands to get to the right place. */
            StringTokenizer t = new StringTokenizer(filename, "/");
            String	    pathElement = null;
            while (t.hasMoreElements()) {
                pathElement = t.nextToken();
                if (!t.hasMoreElements()) {
                    /* This is the file component.  Look it up now. */
                    break;
                }
                try {
                    cd(pathElement);
                } catch (FtpProtocolException e) {
                    /* Giving up. */
                    throw fileException;
                }
            }
            if (pathElement != null) {
                s = openDataConnection("RETR " + pathElement);
            } else {
                throw fileException;
            }
        }
        return new FtpInputStream(this, s.getInputStream(), binaryMode);
    }

    /** PUT a file to the FTP server */
    public TelnetOutputStream put(String filename) throws IOException {
        Socket s = openDataConnection("STOR " + filename);
        return new TelnetOutputStream(s.getOutputStream(), binaryMode);
    }

    /** LIST files on a remote FTP server */
    public TelnetInputStream list() throws IOException {
        Socket s = openDataConnection("LIST");
        return new TelnetInputStream(s.getInputStream(), binaryMode);
    }

    /** CD to a specific directory on a remote FTP server */
    public void cd(String remoteDirectory) throws IOException {
        issueCommandCheck("CWD " + remoteDirectory);
    }

    /** Set transfer type to 'I' */
    public void binary() throws IOException {
        issueCommandCheck("TYPE I");
        binaryMode = true;
    }

    /** Set transfer type to 'A' */
    public void ascii() throws IOException {
        issueCommandCheck("TYPE A");
        binaryMode = false;
    }

    /** New FTP client connected to host <i>host</i>. */
    public FtpClient(String host) throws IOException {
        super();
        openServer(host, FTP_PORT);
    }

    /** New FTP client connected to host <i>host</i>, port <i>port</i>. */
    public FtpClient(String host, int port) throws IOException {
        super();
        openServer(host, port);
    }

    /** Create an uninitialized FTP client. */
    public FtpClient() {}
}
