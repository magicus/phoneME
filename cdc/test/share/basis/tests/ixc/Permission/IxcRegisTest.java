/*
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
 */


// Checks for CR 6238983 
// (IxcRegistry.list() and unbind() should be security constrained)

import javax.microedition.xlet.*;
import javax.microedition.xlet.ixc.*;
import java.rmi.*;
import java.awt.Container;

import com.sun.xlet.XletSecurity;

public class IxcRegisTest {
   public static void main(String[] args) {
      new IxcRegisTest();
   }

   public IxcRegisTest() {

      IxcRegistry regis = IxcRegistry.getRegistry(new DummyXletContext());
      try {
         regis.bind("a/b", new DummyRemoteObject());
         regis.bind("a/b/c", new DummyRemoteObject());
         regis.bind("a/b/c/d", new DummyRemoteObject());
         regis.bind("a/d/c", new DummyRemoteObject());
         regis.bind("a/d/c/d", new DummyRemoteObject());
      } catch (Exception e) { e.printStackTrace(); }

      String[] list = regis.list();
      System.out.println("IxcRegistry.list() returned " + list.length + " items");
      for (int i = 0; i < list.length; i++) {
         System.out.println(i + ": " + list[i]);
      }

      System.out.println("Installing SecurityManager");
      System.setSecurityManager(new XletSecurity());

      String[] list2 = regis.list();
       
      System.out.println("IxcRegistry.list() returned " + list2.length + " items");
      for (int i = 0; i < list2.length; i++) {
         System.out.println(i + ": " + list2[i]);
      }
      if (list2.length != 2) {
         System.out.println("FAILED, should get 2 names");
         System.out.println("(Did you run the test with -Djava.security.policy==IxcTest.policy option?)\n");
         return;
      }
   
      int count = 0;
      for (int i = 0; i < list.length; i++) {
         try {
            regis.unbind(list[i]);
         } catch (SecurityException e) { 
            System.out.println("For " + list[i] + ", caught " + e);
            count++;
         } catch (Exception e) {
            System.out.println("Unexpected exception for " + list[i] 
                               + " " + e);
         }
      }
 
      if (count != 0) {    
         System.out.println("FAILED, got SecurityException for objects exported by himself");
         return;
      }

      System.out.println("\nDone, OK\n");
   }
}
