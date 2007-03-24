/*
 *
 *  Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
 *  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License version
 *  2 only, as published by the Free Software Foundation. 
 *  
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  General Public License version 2 for more details (a copy is
 *  included at /legal/license.txt). 
 *  
 *  You should have received a copy of the GNU General Public License
 *  version 2 along with this work; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 *  02110-1301 USA 
 *  
 *  Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 *  Clara, CA 95054 or visit www.sun.com if you need additional
 *  information or have any questions. 
 */
package com.sun.mmedia;
import com.sun.j2me.app.AppIsolate;

/**
 *  Description of the Class
 *
 */
public class AudioTunnel {

    private TunnelThread tunnelThread = null;
    private int num_open_players = 0;
    private boolean pcmdev_acquired = false;
    private static AudioTunnel tunnel = null;

    native static int nInit(int isolateId);
    native static int nPlayBack(int isolateId);
    native static int nStop(int isolateId);

    native static int nStartMixer(int isolateId);
    native static int nStopMixer(int isolateId);

    private AudioTunnel() {
            if (nInit(AppIsolate.getIsolateId()) == 0) {
                tunnelThread = new TunnelThread();
                nStartMixer(AppIsolate.getIsolateId());
                pcmdev_acquired = true;
            }
    }
    
    synchronized static AudioTunnel getInstance() {
        if (tunnel == null) {
            tunnel = new AudioTunnel();
        } else if(!tunnel.pcmdev_acquired) {
            try {
                tunnel.tunnelThread.join();
            } catch(InterruptedException e) {}
            tunnel = null;
            tunnel = new AudioTunnel();
        }
        return tunnel;
    }
    
    void start() {
        if (tunnelThread != null) {
            synchronized (tunnelThread) {
                num_open_players++;
                tunnelThread.notify();
            }
        }
    }
    
    void stop() {
        if (tunnelThread != null) {
            synchronized (tunnelThread) {
                if (num_open_players > 0) {
                    num_open_players--;
                }
            }
        }
    }

    protected void finalize() {
		num_open_players = 0;
    }

    private class TunnelThread extends Thread {
        private boolean terminate = false;
        public TunnelThread() {
            setPriority(Thread.MAX_PRIORITY);
            start();
        }

        public void run() {
            int s;
            long stopTime = 0;
            while(!terminate) {
                while (pcmdev_acquired) {
                    s = nPlayBack(AppIsolate.getIsolateId());
                    if(s > 0) {
                        try{Thread.sleep(s);}catch(Exception e){}
                        stopTime = 0;
                    } else {
                        synchronized (this) {
                            if (num_open_players == 0 && pcmdev_acquired == true) {
                                if (stopTime == 0) {
                                    stopTime = System.currentTimeMillis() + 5000; /* 5 sec */
                                } else if (stopTime < System.currentTimeMillis()) {
                                    nStopMixer(AppIsolate.getIsolateId());
                                    pcmdev_acquired = false;
                                    stopTime = 0;
                                }
                            }
                        }
                    }
                }
                if (num_open_players == 0) {
                    terminate = true;
                    nStop(AppIsolate.getIsolateId());
                } else {
                    synchronized (this) {
                        try {wait(1000); } catch (Exception ex) {};
                    }
                }
            };
        }

        protected void finalize() {
            num_open_players = 0;
        }
		
    } 
    
}