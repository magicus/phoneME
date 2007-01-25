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

    private static Thread tunnelThread = null;
    private static int num_open_players = 0;
    private static boolean pcmdev_acquired = false;
    private static long toneStopTime = 0;

    native static int nInit(int isolateId);
    native static int nPlayBack(int isolateId);
    native static int nStop(int isolateId);

    native static int nStartMixer(int isolateId);
    native static int nStopMixer(int isolateId);

    public AudioTunnel() {
        if (tunnelThread == null) {
            /* 1) CREATE SHARED MEMORY */
            /* 2) SEND SHARED MEMORY HANDLER TO DRIVER */
            nInit(AppIsolate.getIsolateId());
            tunnelThread = new Thread() {
                public void run() {
                    int s;
                    do {
                        while (pcmdev_acquired) {
                            s = nPlayBack(AppIsolate.getIsolateId());
                            if(s > 0) {
                                try{Thread.sleep(s);}catch(Exception e){}
                            }
                            synchronized (this) {
                                if (num_open_players == 0 && pcmdev_acquired == true) {
                                    if (toneStopTime < System.currentTimeMillis()) {
                                        nStopMixer(AppIsolate.getIsolateId());
                                        pcmdev_acquired = false;
                                        toneStopTime = 0;
                                    }
                                }
                            }
                        }
                        synchronized (this) {
                            try {wait(1000); } catch (Exception ex) {};
                       }
                    } while(true);
                }
                protected void finalize() {
                    nStop(AppIsolate.getIsolateId());
                }
            };
            tunnelThread.setPriority(Thread.MAX_PRIORITY);
            tunnelThread.start();
        }
    }
    
    static void start() {
        synchronized (tunnelThread) {
            num_open_players++;
            if (num_open_players == 1) {
                nStartMixer(AppIsolate.getIsolateId());
                pcmdev_acquired = true;
            }
        
            tunnelThread.notify();
        }
    }
    
    static void stop() {
        synchronized (tunnelThread) {
            if (num_open_players > 0) {
                num_open_players--;
/*                if (num_open_players == 0) {
                    nStopMixer(AppIsolate.getIsolateId());
                    pcmdev_acquired = false;
                }*/
            }
        }
    }

    static void start(long delay) {
        if (num_open_players == 0) {
            toneStopTime = System.currentTimeMillis()+ delay;
            nStartMixer(AppIsolate.getIsolateId());
            pcmdev_acquired = true;
        }
    }

    
}