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

// CR 6254044 (Inconsistency between unbind() and unbindAll() in IXCRegistry)

import javax.microedition.xlet.*;
import javax.microedition.xlet.ixc.*;
import java.rmi.*;
import java.awt.Container;

import com.sun.xlet.XletSecurity;

public class IxcRegisTest2 {
   public static void main(String[] args) {
      new IxcRegisTest2();
   }

   String validName1 = "a/b";
   String validName2 = "a/c";
   String invalidName1 = "a/d";

   public IxcRegisTest2() {

      IxcRegistry regis1 = IxcRegistry.getRegistry(new DummyXletContext());
      IxcRegistry regis2 = IxcRegistry.getRegistry(new DummyXletContext());
      try {
         regis1.bind(validName1, new DummyRemoteObject());
         regis1.bind(validName2, new DummyRemoteObject());
      } catch (Exception e) { 
         System.out.println("Shoulnd't happen: " + e);
      }

      try {
         regis1.unbind(invalidName1);
      } catch (NotBoundException nbe) { 
         System.out.println("OK:  " + nbe + " as expected");
      } catch (Exception e) { 
         System.out.println("Shoulnd't happen: " + e);
      }

      try {
         regis2.unbind(invalidName1);
      } catch (NotBoundException nbe) { 
         System.out.println("OK:  " + nbe + " as expected");
      } catch (Exception e) { 
         System.out.println("Shouldn't happen: " + e);
      }

      try {
         regis2.unbind(validName1);
      } catch (AccessException ae) { 
         System.out.println("OK:  " + ae + " as expected");
      } catch (Exception e) { 
         System.out.println("Shouldn't happen: " + e);
      }
      
      System.out.println("Installing SecurityManager");
      System.setSecurityManager(new XletSecurity());

      try {
         regis2.unbind(validName2);
      } catch (SecurityException se) { 
         System.out.println("OK:  " + se + " as expected");
      } catch (Exception e) { 
         System.out.println("WRONG: regis2.unbind(valid): " + e);
      }

      try {
         regis2.unbind(invalidName1);
      } catch (SecurityException se) { 
         System.out.println("OK:  " + se + " as expected");
      } catch (Exception e) { 
         System.out.println("WRONG: regis2.unbind(invalid): " + e);
      }

      try {
         regis1.unbind(invalidName1);
      } catch (SecurityException se) { 
         System.out.println("OK:  " + se + " as expected");
      } catch (Exception e) { 
         System.out.println("WRONG: regis1.unbind(invalidName): " + e);
      }

      try {
         regis1.unbind(validName1);
      } catch (Exception e) { 
         System.out.println("WRONG: regis1.unbind(valid): " + e);
      }


      System.out.println("Done.");
   }
}
