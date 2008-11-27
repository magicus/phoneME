/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

/**
 * The test checks for the following statement (from an email exchange) 
 * 
 * The ContentStore should ensure the following
 * - If there are bunch of read-only handles on a store and a call comes
 *   for exclusive handle, it should block till all the non-exclusive
 *   handles are closed. At which point the (first) call for exclusive
 *   handle will go through fine.  When there is caller with an exclusive
 *   handle, there are no read-only handles.
 *
 * The test logic is as follows:
 * 
 * Thread 1                             Thread 2
 *---------                             --------
 *                                      (wait) 
 * 1. Exclusive handle #1 open
 *    (notify T2, pause)     
 *                                      2. Read handle #1 open - blocked?
 * 3. Exclusive handle #1 close               
 *    (wait)
 *                                      4  Check read handle #1 unblocked?
 *                                      5. Read handle #1 close
 *                                         (notify T1)
 *                                         (wait)
 * 6. Read handle #2 open
 * 7. Read handle #3 open
 *    (notify T2)
 *                                      8. Exclusive handle #2 open - blocked?
 * 9. Read handle #2 close                 
 *                                     10. Check exclusive handle #2 still blocked?
 * 11. Read handle #3 close                 
 *    (wait)
 *                                     12. Check Exclusive handle #2 unblocked?
 *                                         (notify T1, pause)
 * 13. Exclusive handle #3 open - blocked? 
 *                                     14. Exclusive handle #2 close
 * 14. Check Exclusive handle #3 unblocked?
 * 15. Exclusive handle #3 close
 *
 * Done.
**/

import java.util.Map;
                                                                                     
import com.sun.jump.module.contentstore.*;
                                                                                     
public class StoreHandleModeTest extends JUMPContentStore {

   Object lockObject = new Object();

   // All the handles used for testing.
   JUMPStoreHandler exclusive_1 = null;  
   JUMPStoreHandler exclusive_2 = null;  
   JUMPStoreHandler exclusive_3 = null;  
   JUMPStoreHandler readonly_1  = null;  
   JUMPStoreHandler readonly_2  = null;  
   JUMPStoreHandler readonly_3  = null;  

   boolean debug = true;

   public static void main(String[] args) {
      // This one line should be called by the executive in real impl
      new com.sun.jumpimpl.module.contentstore.StoreFactoryImpl();

      new StoreHandleModeTest();
   }

   void trace(String str) {
      if (debug)
          System.out.println(str);
   }

   void check(boolean condition, String description) {
      if (!condition) {
         System.out.println("FAILED for: " + description);
         System.exit(1);
      }
   }

   public StoreHandleModeTest() {

      trace("Main: starting the test run");

      Thread t1 = new Thread2Runnable();
      Thread t2 = new Thread1Runnable();

      t2.start();
      t1.start();

      while (t1.isAlive() && t2.isAlive()) {
         try {
            Thread.sleep(1000);
         } catch (Exception e) {}
      }

      // If the code got here, then the test completed without hang
      // or a vm exit due to some error. 
      trace("Main: done with the test.  Completed successfully.");
   }

   class Thread1Runnable extends Thread { 
       public void run() {
          try {

             trace("Thread1 starting");
             trace("Thread1 requesting exclusive handle 1");
             exclusive_1 = new JUMPStoreHandler(openStore(true));
             trace("Thread1 received exclusive handle 1");

             synchronized(lockObject) {
                lockObject.notify();
             }

             pause();

             trace("Thread1 closing exclusive handle 1");
             exclusive_1.close();

             synchronized(lockObject) {
                lockObject.wait();
             }

             trace("Thread1 requesting read only handle 2");
             readonly_2 = new JUMPStoreHandler(openStore(false));
             trace("Thread1 received read only handle 2");

             pause();
 
             trace("Thread1 requesting read only handle 3");
             readonly_3 = new JUMPStoreHandler(openStore(false));
             trace("Thread1 received read only handle 3");

             synchronized(lockObject) {
                lockObject.notify();
             }
       
             pause();

             trace("Thread1 closing read only handle 2");
             readonly_2.close();

             pause();
      
             trace("Thread1 closing read only handle 3");
             readonly_3.close();

             synchronized(lockObject) {
                lockObject.wait();
             }

             trace("Thread1 requesting exclusive handle 3");
             exclusive_3 = new JUMPStoreHandler(openStore(true)); // should block
             trace("Thread1 received exclusive handle 3");

             check(exclusive_2.isClosed(), 
                   "prohibit two exclusive handles to be opened");
             trace("Thread1 closing exclusive handle 3");
             exclusive_3.close();

             trace("Thread1 done, exiting");
          
          } catch (InterruptedException e) {
             e.printStackTrace();
             System.exit(1);
          }
       }
   }

   class Thread2Runnable extends Thread  { 
       public void run() {
          try {
             trace("Thread2 starting");
             synchronized(lockObject) {
                lockObject.wait();
             }

             trace("Thread2 requesting read only handle 1");
             readonly_1 = new JUMPStoreHandler(openStore(false));  // should block
             trace("Thread2 received read only handle 1");
             check(exclusive_1.isClosed(), 
                   "block read handle while exclusive access is taken");

             trace("Thread2 Closing read only handle 1");
             readonly_1.close();
      
             synchronized(lockObject) {
                lockObject.notify();
                lockObject.wait();
             }

             trace("Thread2 requesting exclusive handle 2");
             exclusive_2 = new JUMPStoreHandler(openStore(true)); // should block
             trace("Thread2 received exclusive handle 2");
             check((readonly_2.isClosed() && readonly_2.isClosed()),
                   "block exclusive handle while read only handles are taken");

             synchronized(lockObject) {
                lockObject.notify();
             }

             pause();
 
             trace("Thread2 closing exclusive handle 2");
             exclusive_2.close();

             trace("Thread2 done, exiting"); 

          } catch (InterruptedException e) {
             e.printStackTrace();
             System.exit(1);
          }
       }
   }

   // Wait for 1 second.
   public void pause() {
       try {
          Thread.sleep(1000);
       } catch (InterruptedException e) {}
   }

   // A utility class that keeps track of whether the handle is closed or not.
   class JUMPStoreHandler {
      JUMPStoreHandle handle;   
      boolean isStoreOpen = true;
      public JUMPStoreHandler(JUMPStoreHandle handle) {
         this.handle = handle; 
      }
      boolean isExclusive() {
         return handle.isExclusive();
      }
      public synchronized boolean isClosed() {
         return !isStoreOpen;
      }
      public synchronized void close() {
         isStoreOpen = false;
         closeStore(handle);
      }
   }

   // JUMPContentStore methods.
   protected JUMPStore getStore() {
      return JUMPStoreFactory.getInstance().getModule(
                                 JUMPStoreFactory.TYPE_FILE);
   }
   public void load(Map map) {}
   public void unload() {}
}
