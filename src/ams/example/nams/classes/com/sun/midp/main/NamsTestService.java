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

package com.sun.midp.main;

import com.sun.midp.security.SecurityToken;
import com.sun.midp.io.j2me.serversocket.Socket;
import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.events.EventTypes;
import com.sun.midp.events.EventQueue;
import com.sun.midp.events.EventListener;
import com.sun.midp.events.Event;
import com.sun.midp.events.NativeEvent;

import java.io.DataInputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.util.Vector;

import javax.microedition.io.ServerSocketConnection;
import javax.microedition.io.SocketConnection;

/**
 * A service for testing the Native AMS (nams). Listens on a socket and
 * processes commands by calling native methods that invoke the native AMS
 * API. Receives Native AMS callbacks and sends the information out through
 * the socket.
 */
class NamsTestService implements EventListener, Runnable {

    static final int PORT = 13322;
    static final String PFX = "namstestsvc: ";

    /**
     * Prints string s to stdout, prefixed by PFX.
     */
    static void d(String s) {
        System.out.print(PFX + s + "\n");
    }

    /**
     * Initializes the nams service. Sets up translation of native callbacks
     * into events, opens up a socket, and creates the service.
     */
    public static void init(SecurityToken token, EventQueue eq) {
        ServerSocketConnection serv;

        initialize(MIDletSuiteUtils.getIsolateId());

        try {
            Socket s = new Socket();
            s.open(PORT, token);
            serv = (ServerSocketConnection) s;
        } catch (Throwable t) {
            d("failed " + t.toString());
            t.printStackTrace();
            return;
        }

        new NamsTestService(serv, eq);
    }

    ServerSocketConnection serv;
    DataInputStream in;
    PrintStream out;
    boolean reading;

    /**
     * Constructor for the test service. Starts the listener thread and
     * registers itself as a listener for test events.
     */
    NamsTestService(ServerSocketConnection s, EventQueue eq) {
        serv = s;
        new Thread(this).start();

        eq.registerEventListener(EventTypes.TEST_EVENT, this);
    }

    /**
     * The socket listener loop.  Awaits a connection, processes requests from
     * the connection, then goes back to waiting, forever.
     */
    public void run() {
        SocketConnection sock;

        while (true) {
            d("listening on port " + PORT);
            try {
                sock = (SocketConnection)serv.acceptAndOpen();
            } catch (IOException ioe) {
                d("accept failed: " + ioe);
                try {
                    serv.close();
                } catch (IOException ignored) { }
                return;
            }

            d("connected");
            readSocket(sock);
            d("disconnected");
        }
    }

    /**
     * Opens input and output streams from a fresh connection, then processes
     * input from the socket.  Closes and cleans up after input processing
     * completes.
     */
    void readSocket(SocketConnection sock) {
        try {
            in = sock.openDataInputStream();
        } catch (IOException ioe) {
            d("input stream failed: " + ioe);
            try {
                sock.close();
            } catch (IOException ignored) { }
            return;
        }

        try {
            out = new PrintStream(sock.openDataOutputStream());
        } catch (IOException ioe) {
            d("output stream failed: " + ioe);

            try {
                in.close();
            } catch (IOException ignored) { }
            in = null;

            try {
                sock.close();
            } catch (IOException ignored) { }

            return;
        }

        out.println("> NAMS Test Service ready.");
        readLines();
        out.close();
        out = null;

        try {
            in.close();
        } catch (IOException ignored) { }
        in = null;

        try {
            sock.close();
        } catch (IOException ignored) { }
    }

    /**
     * Reads lines of input from the socket and processes them. Reads as long
     * as the 'reading' boolean is true, then returns.
     */
    void readLines() {
        StringBuffer sb = new StringBuffer(100);

        reading = true;
        while (reading) {
            int b;

            try {
                b = in.read();
            } catch (IOException ioe) {
                break;
            }

            if (b == -1 && sb.length() == 0) {
                break;
            }

            if (b == -1 || b == '\n') {
                int len = sb.length();
                if (len > 0 && sb.charAt(len-1) == '\r') {
                    sb.setLength(len-1);
                }
                processLine(tokenize(sb.toString()));
                sb.setLength(0);
            } else {
                sb.append((char)(b & 0xff));
            }
        }
    }

    /**
     * Tokenizes a string in a simple fashion. Given a line of input, returns
     * an array of strings containing tokens. A token consists of a sequence
     * of nonblank characters. Tokens are separated by one or more blanks.
     */
    String[] tokenize(String st) {
        Vector vec = new Vector();
        int start = -1;
        int cur = 0;
        int len = st.length();

        while (true) {
            while (cur < len && st.charAt(cur) == ' ') {
                cur += 1;
            }

            start = cur;

            while (cur < len && st.charAt(cur) != ' ') {
                cur += 1;
            }

            if (start >= len) {
                break;
            }

            vec.addElement(st.substring(start, cur));
        }

        String[] arr = new String[vec.size()];
        vec.copyInto(arr);
        return arr;
    }

    /**
     * A simple argument checking function. Given an argument array argv,
     * ensures that it is exactly rqd elements long, and then attempts to
     * parse argument idx as an integer. If all of these are successful, the
     * parsed integer value is returned in an Integer object. Otherwise, null
     * is returned.
     */
    Integer check(String[] argv, int rqd, int idx) {
        if (argv.length != rqd) {
            reply("> ?");
            return null;
        }

        try {
            return Integer.valueOf(argv[idx]);
        } catch (NumberFormatException nfe) {
            reply("> ?");
            return null;
        }
    }

    void echo(String[] argv, String pfx, int argStart) {
        String s = pfx + argv[argStart];
        for (int i = argStart+1; i < argv.length; i++) {
            s += " " + argv[i];
        }
        reply(s);
    }

    void reply(String s) {
        d(s);

        // IMPL_NOTE not thread-safe
        // IMPL_NOTE if no connection, should buffer up events
        // and send them all out when a connection arrives

        if (out != null) {
            out.println(s);
        }
    }

    /**
     * Processes a command and its arguments.
     */
    void processLine(String[] sa) {
        if (sa.length == 0) {
            return;
        }

        echo(sa, "< ", 0);

        if ("quit".equals(sa[0])) {
            reading = false;
        } else if ("echo".equals(sa[0])) {
            // no need to do anything
            // if (sa.length > 1) {
            //    echo(sa, "> ", 1);
            // }
        } else if ("createstart".equals(sa[0])) {
            Integer app = check(sa, 4, 3);
            if (app != null) {
                int suiteId = MIDletSuite.UNUSED_SUITE_ID;
                try {
                    suiteId = Integer.parseInt(sa[1]);
                } catch (NumberFormatException nfe) {
                  // Intentionally ignored
                }
                NamsAPIWrapper.midletCreateStart(suiteId,
                    sa[2],
                    app.intValue());
            }
        } else if ("resume".equals(sa[0])) {
            Integer app = check(sa, 2, 1);
            if (app != null) {
                NamsAPIWrapper.midletResume(app.intValue());
            }
        } else if ("pause".equals(sa[0])) {
            Integer app = check(sa, 2, 1);
            if (app != null) {
                NamsAPIWrapper.midletPause(app.intValue());
            }
        } else if ("destroy".equals(sa[0])) {
            Integer app = check(sa, 3, 1);
            if (app != null) {
                int timeout = -1;
                try {
                    timeout = Integer.parseInt(sa[2]);
                } catch (NumberFormatException nfe) {
                  // Intentionally ignored
                }
                NamsAPIWrapper.midletDestroy(app.intValue(),
                    timeout);
            }
        } else if ("setfg".equals(sa[0])) {
            Integer app = check(sa, 2, 1);
            if (app != null) {
                NamsAPIWrapper.midletSetForeground(app.intValue());
            }
        } else if ("stop".equals(sa[0])) {
            NamsAPIWrapper.midpSystemStop();
        } else if ("help".equals(sa[0])) {
            for (int ii = 0; ii < helpmsg.length; ii++) {
                reply("> " + helpmsg[ii]);
            }
        } else {
            reply("> ?");
        }
    }

    /**
     * Help message strings.
     */
    String helpmsg[] = {
        "createstart suite-name class-name app-id",
        "destroy app-id timeout",
        "echo [args ...]",
        "pause app-id",
        "quit",
        "resume app-id",
        "setfg app-id",
        "stop"
    };

    // -------------------- interface EventListener --------------------

    /**
     * Preprocesses events. Does no preprocessing, so always returns true.
     */
    public boolean preprocess(Event event, Event waitingEvent) {
        return true;
    }

    /**
     * Processes test events. Decodes the event and sends the information out
     * through the socket.
     */
    public void process(Event event) {
        NativeEvent e = (NativeEvent)event;
        int appId = e.intParam1;
        int callbackId = e.intParam2;
        String state = getStateByValue(e.intParam3);
        int reason = e.intParam4;
        String s;

        switch (callbackId) {
        case 0:         // background
            reply("> background, appId = " + appId + ", state = " + state +
                  ", reason = " + reason);
            break;
        case 1:         // foreground
            reply("> foreground, appId = " + appId + ", state = " + state +
                  ", reason = " + reason);
        case 2:         // state change
            reply("> state, appId = " + appId + ", state = " + state +
                  ", reason = " + reason);
            break;
        default:
            reply(
                "> callbackId=" + callbackId +
                " appId=" + appId +
                " state=" + state +
                " reason=" + reason);
            break;
        }
    }

    /**
     * Converts a midlet state (including foreground/background states) value
     * into readable string.
     *
     * @param state state value to convert
     *
     * @return a string representing the given state value
     */
    private String getStateByValue(int state) {
        /* IMPL_NOTE: see midpNativeAppManager.h for the definitions */
        final String[] stateStrings = {
            "MIDP_MIDLET_STATE_ACTIVE",
            "MIDP_MIDLET_STATE_PAUSED",
            "MIDP_MIDLET_STATE_DESTROYED",
            "MIDP_MIDLET_STATE_ERROR",
            "MIDP_DISPLAY_STATE_FOREGROUND",
            "MIDP_DISPLAY_STATE_BACKGROUND",
            "MIDP_DISPLAY_STATE_FOREGROUND_REQUEST",
            "MIDP_DISPLAY_STATE_BACKGROUND_REQUEST"
        };

        if (state >= 1 && state < stateStrings.length) {
            return stateStrings[state - 1];
        }

        return "unknown (" + state + ")";
    }

    // -------------------- natives --------------------

    /**
     * Initializes the native portion of the NAMS Test Service.
     *
     * @param isolateId the isolateId to which test events are sent
     */
    private native static void initialize(int isolateId);

    /**
     * Cleans up the native portion of the NAMS Test Service.
     */
    private native static void cleanup();
}
