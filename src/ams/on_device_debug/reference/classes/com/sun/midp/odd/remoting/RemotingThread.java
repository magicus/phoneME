package com.sun.midp.odd.remoting;

import java.io.IOException;

import com.sun.jme.remoting.NamedObjectRegistry;
import com.sun.jme.remoting.RemotingHandler;
import com.sun.midp.odd.ProgressScreen;

import javax.microedition.io.Connector;
import javax.microedition.io.ServerSocketConnection;
import javax.microedition.io.SocketConnection;

public class RemotingThread implements Runnable {

    public static final int DEFAULT_PORT_NUMBER = 55123;

    private final NamedObjectRegistry namedObjectRegistry;

    private Thread thread;

    private RemotingHandler handler;

    private SocketConnection socket;

    private ServerSocketConnection server;

    private ProgressScreen screen;

    /** Indicates if the server socket has been closed. */
    private boolean running;

    public RemotingThread(NamedObjectRegistry registry, ProgressScreen sc) {
        this.namedObjectRegistry = registry;
        this.running = false;
        this.screen = sc;
    }

    public synchronized void start() {
        thread = new Thread(this);
        thread.start();
        running = true;
    }

    public void stop() {
        if (!running) {
            return;
        }
        if (handler != null) {
            // a client is connected, terminate connection first
            handler.stopDispatchingLoop();
            try {
                thread.join();
            } catch (InterruptedException e) { /* ignore */
            }
            thread = null;
        } else {
            // no active client, just close the listening socket
            closeSockets();
        }
        running = false;
    }

    public void run() {
        try {
            server = (ServerSocketConnection) Connector.open("socket://:"
                    + getPortNumber());
            // process one client at a time
            while (server != null) {
                socket = (SocketConnection) server.acceptAndOpen();
                handler = RemotingHandler.createInstance(namedObjectRegistry,
                        socket.openInputStream(), socket.openOutputStream());
                handler.executeDispatchingLoop();
                handler = null;
                socket = null;
            }
            closeSockets();
        } catch (IOException e) {
            e.printStackTrace();
            // socket was unexpectedy closed, cleanup and close
            if (handler != null) {
                handler = null;
            }
            closeSockets();
        }
    }

    private void closeSockets() {
        if (socket != null) {
            try {
                socket.close();
            } catch (IOException ex) { /* ignore */
            }
            socket = null;
        }
        if (server != null) {
            try {
                server.close();
            } catch (IOException ex) { /* ignore */
            }
            server = null;
        }
    }

    private int getPortNumber() {
        String portNumberStr = System.getProperty("remoting.port.number");
        int portNumber;
        if (portNumberStr != null) {
            portNumber = Integer.parseInt(portNumberStr);
        } else {
            portNumber = DEFAULT_PORT_NUMBER;
        }
        return portNumber;
    }

    public boolean isRunning() {
        return running;
    }
}
