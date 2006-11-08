/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

package com.sun.appmanager.mtask;

import java.io.*;
import java.net.*;
import java.util.Timer;
import java.util.TimerTask;
import java.util.Vector;

/**
 * A class that encapsulates communication with a <i>master JVM
 * instance</i>. The system-wide instance of this class can be
 * obtained via the <tt>getMtaskClient()</tt> method of class {@link
 * com.sun.appmanager.AppManager}. This instance can control all
 * aspects of application lifecycle.
 */
public class Client
{
    public static final int DEFAULT_SERVER_PORT = 7777;
    public static final String DEFAULT_SERVER_NAME = "localhost";
    private int serverPort;
    private String serverName;

    //
    // The handles for a conversation with the mtask server.
    // These are created on Client instantiation, and are used throughout
    // the session with the mtask server.
    //
    private PrintWriter serverOut = null;
    private BufferedReader serverIn = null;

    private int applistDelay = 2000;
    private Timer applistTimer = null;

    // Launched set of apps that have not been reaped.
    private Vector launchedApps = new Vector();

    private static boolean verbose = (System.getProperty("cdcams.verbose") != null) &&
        (System.getProperty("cdcams.verbose").toLowerCase().equals("true"));

    /**
     * Create a client that communicates with <tt>server:port</tt>
     */
    public Client(String server, int port)
    {
	this.serverName = server;
	this.serverPort = port;
	if (!connect()) {
	    throw new RuntimeException("FATAL ERROR: "+
				       "Could not find mtask server");
	}
	this.applistTimer = new Timer();
        applistTimer.schedule(new AppListCheckAction(this), 
			      (long) applistDelay,
                              (long) applistDelay);
    }

    //
    // Tracking launched apps
    //  
    private String[] getAppsArray(Vector apps) {
        String[] l = new String[apps.size()];
        apps.copyInto(l);
        return l;
    }

    public String[] getLaunchedApps() {
	synchronized(launchedApps) {
	    return getAppsArray(launchedApps);
	}
    }

    private void addLaunchedApp(String appId) {
        launchedApps.addElement(appId);
    }

    private void removeLaunchedApp(String appId) {
	if (appId == null) {
	    return;
	}
	synchronized(launchedApps) {
	    if (!launchedApps.contains(appId)) {
		// Someone beat us to it. That's OK.
		return;
	    }
	    launchedApps.removeElement(appId);
	}
    }
    //
    // Asynchronous cdcams task event listeners
    //
    private Vector listeners = new Vector(); // One per instance

    /**
     * Add task listener for task events
     */
    public void addListener(TaskListener l) {
	synchronized(listeners) {
	    listeners.add(l);
	}
    }

    /**
     * Remove task listener for task events
     */
    public void removeListener(TaskListener l) {
	synchronized(listeners) {
	    listeners.removeElement(l);
	}
    }

    private class AppListCheckAction
        extends TimerTask {

	Client mtaskClient;

        public AppListCheckAction(Client mtaskClient) {
            super();
	    this.mtaskClient = mtaskClient;
        }

	// App list parsing
	// We are getting a string of the form: PID=%d COMMAND=".."
	private String appIdOf(String listItem) {
	    int end = listItem.indexOf(' ');
	    int start = 4; // Right past PID=
	    return listItem.substring(start, end);
	}

        private void mapIds(String[] list) {
            for (int i = 0; i < list.length; i++) {
                list[i] = appIdOf(list[i]);
            }
        }

        private boolean inList(String e, String[] l) {
            for (int i = 0; i < l.length; i++) {
                if (l[i].equals(e)) {
                    return true;
                }
            }
            return false;
        }

        private Object[] getDeadApps() {
	    String[] list = null;
	    String[] launched = null;
	    Vector thedead = null;

	    //
	    // Block any new apps from being added to the launchedApps list
	    // while we check for app deaths
            //
	    synchronized(launchedApps) {
		// Our notion of active apps
		boolean exited = mtaskClient.childrenExited();
		if (!exited) {
		    return null;
		}
		
		thedead = new Vector();
		list = mtaskClient.list();
		//
		// Our current notion of who is alive
		// (which is now out of date)
		//
		launched = getLaunchedApps();
	    }
	    mapIds(list);
            // Compare this with the real list, and extract the dead
            for (int i = 0; i < launched.length; i++) {
                if (!inList(launched[i], list)) {
                    // Found dead app
                    if (verbose) {
                        System.err.println("DETECTED DEAD APP " + launched[i]);
                    }
                    thedead.addElement(launched[i]);
                }
            }
            return thedead.toArray();
        }

        public void run() {
            //System.err.println("######## CHECKING FOR DEAD APPS");
            Object[] deadApps = getDeadApps();
            if (deadApps == null) {
                // Nothing to do
                return;
            }

            for (int i = 0; i < deadApps.length; i++) {
                String app = (String) deadApps[i];
                //
                // Remove the app from our launched app records
                //
                removeLaunchedApp(app);

                //
                // Now notify any interested parties of app death
                //
                synchronized (listeners) {
                    Object[] l = listeners.toArray();
                    for (int j = 0; j < l.length; j++) {
                        TaskListener tl = (TaskListener) l[j];
                        tl.taskEvent(app, TaskListener.CDCAMS_TASK_KILLED);
                    }
                }
            }
        }
    }

    private boolean connect()
    {
	try {
	    Socket mvmSocket = new Socket(serverName, serverPort);
	    InputStream instream = mvmSocket.getInputStream();
	    OutputStream outstream = mvmSocket.getOutputStream();

	    serverIn = new BufferedReader(new InputStreamReader(instream));
	    serverOut = new PrintWriter(outstream, true); // auto-flush
	} catch (IOException e) {
            if (verbose) {
	        System.err.println("Cannot establish connection with mTASK server");
	        System.err.println("Is the mTASK server running at "+
		     	       serverName+":"+
			       serverPort+"?");
            }
	    return false;
	} catch (Throwable e) {
            if (verbose) {
	        System.err.println("Exception: "+e);
            }
	    e.printStackTrace();
	    return false;
	}
	return true;
    }

    /**
     * Create a client that communicates with
     * <tt>&lt;DEFAULT_SERVER_NAME&gt;:port</tt>
     */
    public Client(int port)
    {
	this(DEFAULT_SERVER_NAME, port);
    }

    /**
     * Create a client that communicates with
     * <tt>server:&lt;DEFAULT_SERVER_PORT&gt;</tt>
     */
    public Client(String server)
    {
	this(server, DEFAULT_SERVER_PORT);
    }

    /**
     * Create a client that communicates with
     * <tt>&lt;DEFAULT_SERVER_NAME&gt;:&lt;DEFAULT_SERVER_PORT&gt;</tt>
     */
    public Client()
    {
	this(DEFAULT_SERVER_NAME, DEFAULT_SERVER_PORT);
    }

    /**
     * Return a list of active applications
     */
    public String[] list()
    {
	return mvmMessageMultiline("LIST", null);
    }

    /**
     * Check whether any apps have exited since last time
     */
    private boolean childrenExited()
    {
	String response = mvmMessage("CHILDREN_EXITED", null);
	if (response.startsWith("YES")) {
	    return true;
	} else {
	    return false;
	}
    }

    /**
     * Set environment variable in the mTASK server. Subsequent
     * children will inherit this.
     */
    public boolean setenv(String keyValuePair)
    {
	String response;
	String[] args;

	args = new String[1];
	args[0] = keyValuePair;

	response = mvmMessage("SETENV", args);
	if ((response == null) || response.startsWith("Usage:")) {
            if (verbose) {
	        System.out.println("Setenv failed");
            }
	    return false;
	} else {
	    return true;
	}
    }

    
    /**
     * Launch using the right command.
     *
     */
    public String launch(String command, String[] args)
    {
	String response;
	String pidString;

	synchronized(launchedApps) {
	    response = mvmMessage(command, args);
	    if (response == null) {
		return null;
	    } else if (response.startsWith("CHILD PID=")) {
		pidString = response.substring(10);
		addLaunchedApp(pidString);
	    } else {
		pidString = null;
	    }
	}

        if (verbose) {
	    System.out.println("The server said pid="+pidString);
        }
	return pidString;
    }

    /**
     * Kill application
     */
    public boolean kill(String appHandle)
    {
	String response;
	String[] args;

	args = new String[1];
	args[0] = appHandle;

	response = mvmMessage("KILL", args);
	if ((response == null) || response.startsWith("Usage:")) {
            if (verbose) {
	        System.out.println("Kill failed");
            }
	    return false;
	} else {
	    return true;
	}
    }

    /**
     * Send a message to an application
     */
    public boolean message(String appHandle, String messageWord)
    {
	String response;
	String[] args;

	args = new String[2];
	args[0] = appHandle;
	args[1] = messageWord;

	response = mvmMessage("MESSAGE", args);
	if ((response == null) || response.startsWith("Usage:")) {
            if (verbose) {
	        System.out.println("Message failed");
            }
	    return false;
	} else {
	    return true;
	}
    }

    /**
     * Send a message to an application and expect a response
     */
    public String messageWithResponse(String appHandle, String messageWord)
    {
        String response;
        String[] args;
                        
        args = new String[2];
        args[0] = appHandle;
        args[1] = messageWord;
                              
        response = mvmMessage("MESSAGE_RESPONSE", args);
        if ((response == null) || response.startsWith("Usage:")) {
            if (verbose) {
                System.out.println("Message failed");
            }
            return null;
        } else {
            //System.out.println("RESPONSE="+response);
            return response;
        }
    }

    /**
     * Broadcast message to all running apps
     */
    public boolean broadcast(String messageWord)
    {
	String response;
	String[] args;

	args = new String[1];
	args[0] = messageWord;

	response = mvmMessage("BROADCAST", args);
	if ((response == null) || response.startsWith("Usage:")) {
            if (verbose) {
	        System.out.println("Broadcast failed");
            }
	    return false;
	} else {
	    return true;
	}
    }

    /**
     * Kill all running applications
     */
    public boolean killall()
    {
	String response;

	response = mvmMessage("KILLALL", null);
	if ((response == null) || response.startsWith("Usage:")) {
            if (verbose) {
	        System.out.println("Kill failed");
            }
	    return false;
	} else {
	    return true;
	}
    }

    /**
     * Tell the master JVM to exit
     */
    public boolean exit() {
        String response;

        response = mvmMessage("JEXIT", null);
        if ((response != null) && response.startsWith("Usage:")) {
            return false;
        } else {
            return true;
        }
    }

    /**
     * Put mtask in testing mode where output, error and exit codes
     * are dumped into files with prefix <filePrefix>
     */
    public boolean testingMode(String filePrefix)
    {
	String response;
	String[] args;

	args = new String[1];
	args[0] = filePrefix;

	response = mvmMessage("TESTING_MODE", args);
	if ((response == null) || response.startsWith("Usage:")) {
            if (verbose) {
	        System.out.println("Testing mode failed");
            }
	    return false;
	} else {
	    return true;
	}
    }

    /**
     * Source an arbitrary command in the mtask server
     */
    public void source(String[] args)
    {
	// Send this off to server
	String response = mvmMessage("S", args); // "S" stands for "source"
	/* Ignore response */
    }

    /**
     * Warmup the running master JVM server.
     */
    public void warmupLists(String classPath, String classNames,
			    String memberNames)
    {
	int nWarmupArgs;
	boolean somethingTodo = false;

	nWarmupArgs = 1;
	if (classPath != null) {
	    nWarmupArgs += 1;
	}

	if (classNames != null) {
	    nWarmupArgs += 2;
	    somethingTodo = true;
	}

	if (memberNames != null) {
	    nWarmupArgs += 2;
	    somethingTodo = true;
	}

	if (!somethingTodo) {
	    return;
	}

	String[] args = new String[nWarmupArgs];
	int j = 0;

	if (classPath != null) {
	    args[j] = "-Djava.class.path="+classPath;
	    j++;
	}
	args[j] = "sun.mtask.Warmup";
	j++;

	if (classNames != null) {
	    args[j] = "-initClasses";
	    args[j+1] = classNames;
	    j += 2;
	}
	if (memberNames != null) {
	    args[j] = "-precompileMethods";
	    args[j+1] = memberNames;
	    j += 2;
	}
	if (j != nWarmupArgs) {
	    throw new InternalError("Argument size mismatch: j="+j+
				    ", nWarmupArgs="+nWarmupArgs);
	}
	source(args);
    }

    private String mvmMessage(String command, String[] args)
    {
	String[] response = mvmMessageMultiline(command, args);
	if (response == null) {
	    return null;
	} else {
	    return response[0];
	}
    }

    private synchronized String[] mvmMessageMultiline(String command, 
						      String[] args)
    {
	//
	// serverIn and serverOut have been set up.
        //
	serverOut.print(command);
	if (verbose) {
	    System.err.print("MVM COMMAND=\""+command);
	}
	if (args != null) {
	    for (int i = 0; i < args.length; i++) {
		serverOut.print(" "+args[i]);
		if (verbose) {
		    System.err.print(" "+args[i]);
		}
	    }
	}
	// Send it off
	serverOut.println();
	if (verbose) {
	    System.err.println("\"");
	}
	//
	// Now read lines of response.  If the first line starts
	// with "MULTILINE", the next word is the number of lines to
	// read. The most typical example of this is the LIST command.
	//
	String[] response = null;
	String   line = null;
	int      responseLen = 1;

	try {
	    line = serverIn.readLine();
	} catch (IOException e) {
            if (verbose) {
	        System.err.println("IO Exception: "+e);
            }
	    e.printStackTrace();
	}
	if (line != null) {
	    if (!line.startsWith("MULTILINE ")) {
		response = new String[1];
		response[0] = line;
	    } else {
		String numLinesString = line.substring(10);
		try {
		    responseLen = Integer.parseInt(numLinesString);
		} catch (NumberFormatException e) {
		    responseLen = -1;
		}
		if (responseLen != -1) {
		    response = new String[responseLen];
		    for (int i = 0; i < responseLen; i++) {
			String thisline;
			try {
			    thisline = serverIn.readLine();
			} catch (IOException e) {
                            if (verbose) {
			        System.err.println("IO Exception: "+e);
                            }
			    e.printStackTrace();
			    responseLen = -1;
			    break;
			}
			response[i] = thisline;
		    }
		    if (responseLen == -1) {
			response = null;
		    }
		}
	    }
	}


	if (verbose && (response != null)) {
	    if (response.length == 0) {
		System.out.println("MVM RESPONSE=<EMPTY LIST>");
	    } else {
		System.out.println("MVM RESPONSE ("+response.length+" lines)[0]=\""+response[0]+"\"");
	    }
	}

	return response;
    }

}
