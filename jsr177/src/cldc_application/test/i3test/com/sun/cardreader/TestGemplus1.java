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

package com.sun.cardreader;

import com.sun.midp.i3test.TestCase;
import java.io.IOException;

/**
 * This test case tests basic PlatformCardReader 
 * functionality, primarily configuration
 * properties.
 */
public class TestGemplus1 extends TestCase {

    /**
     * Test class creation and initialization.
     */
    private void testInit() 
            throws java.io.IOException, 
                   ClassNotFoundException, 
                   InstantiationException,
                   CardDeviceException,
                   IllegalAccessException {
                       
        boolean stub_flag = false;
        CardDevice cd = 
            (CardDevice)Class
                .forName("com.sun.cardreader.PlatformCardDevice")
                .newInstance();
        assertNotNull(cd); /* Creation successful */
        
        try {
            cd.init();
        } 
        catch (CardDeviceException e) {
            if (e.getMessage().equals("stub")) {
                stub_flag = true;
            } else {
                throw e;
            }
        }
        
        if (!stub_flag) {
                /* Init successful (no exceptions) */
            assertTrue(true); 
            cd.close();
        } else {
            assertTrue(true);
        }
    }

    /**
     * Test resetting device.
     */
    private void testReset() 
            throws java.io.IOException, 
                   ClassNotFoundException, 
                   InstantiationException,
                   CardDeviceException,
                   IllegalAccessException {
                       
        boolean stub_flag = false;
        CardDevice cd = 
            (CardDevice)Class
                .forName("com.sun.cardreader.PlatformCardDevice")
                .newInstance();
        
        try {
            cd.init();
        } 
        catch (CardDeviceException e) {
            if (e.getMessage().equals("stub")) {
                stub_flag = true;
            } else {
                throw e;
            }
        }
        
        if (!stub_flag) {
            boolean is_null = true;
            
            cd.lock();
            cd.reset();
            byte[] atr = cd.getATR();
            for (int i = 0; i < atr.length; i++) {
                if (atr[i] != 0)
                    is_null = false;
            }
                /* OK, non-empty ATR received */
            assertFalse("Bad ATR received", is_null); 
            
            cd.close();
        } else {
            assertTrue(true);
        }
        
    }

    /**
     * Test Xfer data.
     */
    private void testXfer() 
            throws java.io.IOException, 
                   ClassNotFoundException, 
                   InstantiationException,
                   CardDeviceException,
                   IllegalAccessException {
                       
        boolean stub_flag = false;
        CardDevice cd = 
            (CardDevice)Class
                .forName("com.sun.cardreader.PlatformCardDevice")
                .newInstance();
        
        try {
            cd.init();
        } 
        catch (CardDeviceException e) {
            if (e.getMessage().equals("stub")) {
                stub_flag = true;
            } else {
                throw e;
            }
        }
        
        if (!stub_flag) {
            cd.lock();
            cd.reset();
    
            boolean is_null = true;
            byte[] request = {(byte)0x00, (byte)0xA4, (byte)0x00, (byte)0x00};
            
            byte[] response = new byte[2];
            
                /* Try SELECT FILE (master file) */
            int received = cd.cmdXfer(request, response); 
            
            for (int i = 0; i < received; i++) {
                if (response[i] != 0)
                    is_null = false;
            }
                /* OK, non-empty status received */
            assertFalse("null response received", (received >= 2 && is_null)); 
                /* OK, master file exists */
            assertTrue("bad response received", 
                (response[received - 2] == (byte)0x61  ||
                 response[received - 2] == (byte)0x90)); 

            if (response[received - 2] == (byte)0x61) {
                int resp_len = response[1];
                response = new byte[resp_len + 2];
                request = new byte[] {
                    (byte)0x00, (byte)0xC0, (byte)0x00, 
                    (byte)0x00, (byte)resp_len
                };
                cd.cmdXfer(request, response);
                
                    /* Master file is received successfuly */
                assertTrue("bad response received", 
                    (response[response.length-2] == (byte)0x90 &&
                    response[response.length-1] == (byte)0x00)); 
            }
                    
            cd.close();
        } else {
            assertTrue(true);
        }
    }
     
    /**
     * Test locking and unlocking features.
     */
    private void testLocks() 
            throws java.io.IOException, 
                   ClassNotFoundException, 
                   InstantiationException,
                   CardDeviceException,
                   IllegalAccessException {
                       
        /**
         * Thread class.
         */
        class TestThread extends Thread {
            private int delay;
            private CardDevice device;
            private String name;
            
            TestThread(String name, CardDevice cd, int delay) {
                super();
                this.name = name;
                this.delay = delay;
                device = cd;
            }
            
            public void run() {
                long start = System.currentTimeMillis();
                long cur;
                boolean error = false;

                try {
                    device.lock();
                    do {
                        cur = System.currentTimeMillis();
                    } while (cur < start + delay*1000);
                    device.unlock();
                } catch (IOException e) {
                    error = true;
                }
                    /* Lock/unlock pair works right */
                assertFalse("Thread " + name + " throws IOException", error);
            }
        }
        boolean stub_flag = false;
        
        CardDevice cd = 
            (CardDevice)Class
                .forName("com.sun.cardreader.PlatformCardDevice")
                .newInstance();
        
        try {
            cd.init();
        }
        catch (CardDeviceException e) {
            if (e.getMessage().equals("stub")) {
                stub_flag = true;
            } else {
                throw e;
            }
        }
        if (!stub_flag) {
            TestThread t1 = new TestThread("First task", cd, 5);
            TestThread t2 = new TestThread("Second task", cd, 3);
            TestThread t3 = new TestThread("Third task", cd, 3);
            boolean except = false;
            
            try {
                cd.reset();
            }
            catch (IOException e) {
                except = true;
            }
                /* RESET/XFER commands should not allowed without locking */
            assertTrue("Reset is allowed without locking", except);
            
            t1.start();
            t2.start();
            t3.start();
            
            long start = System.currentTimeMillis();
            long cur;
            
            do {
                cur = System.currentTimeMillis();
            } while (cur < start + 1000);
            
            cd.lock();
            
            except = false;
            try {
                cd.reset();
            } 
            catch (IOException e) {
                except = true;
            }
            cd.unlock();
                /* RESET command should work with locking */
            assertFalse("Reset with lock fails", except);
            
            cd.close();
            
            try {
                t1.join();
                t2.join();
                t3.join();
            } catch (InterruptedException e) {}
            
        } else {
            assertTrue(true);
        }
    }

    /**
     * Run tests.
     */
    public void runTests() {
        try {
            declare("testInit");
            testInit();

            declare("testLocks");
            testLocks();
            
            declare("testReset");
            testReset();

            declare("testXfer");
            testXfer();
        }
        catch (Throwable t) {
            fail("" + t);
        }
    }
    
    public void assertNotEmpty(String message, byte[] arr) {

        boolean is_null = true;
        for (int i = 0; i < arr.length; i++) {
            if (arr[i] != (byte)0) {
                is_null = false;
            }
        }
        assertFalse(message, is_null);
        
    }
}

